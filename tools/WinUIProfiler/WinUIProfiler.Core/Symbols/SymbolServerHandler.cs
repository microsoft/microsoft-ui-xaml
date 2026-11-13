// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.Concurrent;
using System.ComponentModel;
using System.Diagnostics;
using System.Globalization;
using System.Net;
using System.Net.Http.Headers;
using System.Text.Json;

namespace WinUIProfiler.Engine.Symbols;

// Sends SymbolReader's symbol server requests.
// - Requests to https://symweb.azurefd.net carry a token from the Azure CLI ('az account get-access-token'). Readers
//   share the token and get a new one before it expires. Without one, requests go anonymously and symweb refuses them.
// - Other servers get the user's Windows credentials when they ask for them.
// - A server that cannot be reached, or does not answer within 30 seconds, is not asked again. A dead server then
//   costs one timeout rather than one per PDB.
// - Stop fails new requests, cancels those waiting for an answer, and disposes the answers whose content is still
//   being read (SymbolReader reads it synchronously, and drops a partial download).
internal sealed class SymbolServerHandler(Action<string>? warn, CancellationToken stop = default)
    : DelegatingHandler(new HttpClientHandler { UseDefaultCredentials = true })
{
    // The Azure application symweb accepts tokens for.
    private const string SymwebScope = "af9e1c69-e5e9-4331-8cc5-cdf93d57bafa/.default";

    private static readonly TimeSpan RequestTimeout = TimeSpan.FromSeconds(30);
    private static readonly TimeSpan TokenTimeout = TimeSpan.FromSeconds(30);
    private static readonly TimeSpan RefreshBeforeExpiry = TimeSpan.FromMinutes(5);
    private static readonly TimeSpan MinimumTokenUse = TimeSpan.FromMinutes(1);
    private static readonly TimeSpan UnknownTokenLifetime = TimeSpan.FromMinutes(10);
    private static readonly string[] LocalExpiryFormats = ["yyyy-MM-dd HH:mm:ss.ffffff", "yyyy-MM-dd HH:mm:ss"];
    private static readonly SemaphoreSlim TokenGate = new(1, 1);
    private static SymwebToken? cachedToken;

    private readonly ConcurrentDictionary<string, bool> deadServers = new(StringComparer.OrdinalIgnoreCase);
    private volatile bool tokenFailed;
    private int tokenWarned;

    private sealed record SymwebToken(string Value, DateTimeOffset RefreshUtc);

    protected override async Task<HttpResponseMessage> SendAsync(HttpRequestMessage request, CancellationToken cancellationToken)
    {
        stop.ThrowIfCancellationRequested();
        var uri = request.RequestUri ?? throw new InvalidOperationException("The symbol request has no URI.");
        if (deadServers.ContainsKey(uri.Authority))
        {
            throw new HttpRequestException($"{uri.Authority} is skipped because it did not respond earlier.");
        }

        using var stopping = CancellationTokenSource.CreateLinkedTokenSource(cancellationToken, stop);
        SymwebToken? token = null;
        if (!tokenFailed &&
            request.Headers.Authorization is null &&
            uri.Scheme == Uri.UriSchemeHttps &&
            uri.Host.Equals(SymbolPaths.SymwebHost, StringComparison.OrdinalIgnoreCase))
        {
            token = await GetSymwebTokenAsync(stopping.Token).ConfigureAwait(false);
            if (token is not null)
            {
                request.Headers.Authorization = new AuthenticationHeaderValue("Bearer", token.Value);
            }
        }

        HttpResponseMessage response;

        // SymbolReader reads the content after this returns, so the timeout covers the server's first answer only. A
        // stopped request is not the server's fault, so it does not mark the server dead.
        using (var timeout = CancellationTokenSource.CreateLinkedTokenSource(stopping.Token))
        {
            timeout.CancelAfter(RequestTimeout);
            try
            {
                response = await base.SendAsync(request, timeout.Token).ConfigureAwait(false);
            }
            catch (HttpRequestException error) when (error.StatusCode is null && !stopping.IsCancellationRequested)
            {
                MarkDead(uri, error.Message);
                throw;
            }
            catch (OperationCanceledException) when (!stopping.IsCancellationRequested)
            {
                var message = $"no answer within {RequestTimeout.TotalSeconds:0} seconds";
                MarkDead(uri, message);
                throw new HttpRequestException($"{uri.Authority} sent {message}.");
            }
        }

        // The registration lasts as long as the stop token's source, which is one symbolizing run.
        stop.Register(response.Dispose);

        if (token is not null && response.StatusCode is HttpStatusCode.Unauthorized or HttpStatusCode.Forbidden)
        {
            Interlocked.CompareExchange(ref cachedToken, null, token);
            tokenFailed = true;
            WarnToken(
                $"{SymbolPaths.SymwebHost} refused the Azure CLI token ({(int)response.StatusCode}). " +
                "Run 'az login' with an account that can read symweb.");
        }

        return response;
    }

    private void MarkDead(Uri uri, string reason)
    {
        if (deadServers.TryAdd(uri.Authority, true))
        {
            warn?.Invoke($"Symbol server {uri.Authority} is not reachable ({reason}); no more symbols are requested from it.");
        }
    }

    private void WarnToken(string message)
    {
        if (Interlocked.Exchange(ref tokenWarned, 1) == 0)
        {
            warn?.Invoke(message);
        }
    }

    private void FailToken(string reason)
    {
        tokenFailed = true;
        WarnToken(
            $"Could not get a token for {SymbolPaths.SymwebHost} from the Azure CLI ({reason}). " +
            "Its PDBs will not download; install the Azure CLI and run 'az login'.");
    }

    private static bool IsFresh(SymwebToken? token) => token is not null && token.RefreshUtc > DateTimeOffset.UtcNow;

    private async Task<SymwebToken?> GetSymwebTokenAsync(CancellationToken cancellationToken)
    {
        var token = Volatile.Read(ref cachedToken);
        if (IsFresh(token))
        {
            return token;
        }

        await TokenGate.WaitAsync(cancellationToken).ConfigureAwait(false);
        try
        {
            // Another request may have got a token, or failed to, while this one waited.
            token = Volatile.Read(ref cachedToken);
            if (IsFresh(token))
            {
                return token;
            }

            if (tokenFailed)
            {
                return null;
            }

            token = await RequestTokenAsync(cancellationToken).ConfigureAwait(false);
            if (token is not null)
            {
                Volatile.Write(ref cachedToken, token);
            }

            return token;
        }
        finally
        {
            TokenGate.Release();
        }
    }

    // Runs 'az account get-access-token' through cmd.exe, because az is a batch file that Process.Start cannot run
    // directly.
    private async Task<SymwebToken?> RequestTokenAsync(CancellationToken cancellationToken)
    {
        var startInfo = new ProcessStartInfo(Environment.GetEnvironmentVariable("ComSpec") ?? "cmd.exe")
        {
            UseShellExecute = false,
            CreateNoWindow = true,
            RedirectStandardOutput = true,
            RedirectStandardError = true,
        };
        string[] arguments = ["/d", "/c", "az", "account", "get-access-token", "--scope", SymwebScope, "--output", "json"];
        foreach (var argument in arguments)
        {
            startInfo.ArgumentList.Add(argument);
        }

        string output;
        try
        {
            using var process = Process.Start(startInfo) ?? throw new InvalidOperationException("cmd.exe did not start.");
            var outputRead = process.StandardOutput.ReadToEndAsync(cancellationToken);
            var errorRead = process.StandardError.ReadToEndAsync(cancellationToken);
            using (var timeout = CancellationTokenSource.CreateLinkedTokenSource(cancellationToken))
            {
                timeout.CancelAfter(TokenTimeout);
                try
                {
                    await process.WaitForExitAsync(timeout.Token).ConfigureAwait(false);
                }
                catch (OperationCanceledException)
                {
                    Kill(process);
                    cancellationToken.ThrowIfCancellationRequested();
                    FailToken($"'az account get-access-token' did not finish within {TokenTimeout.TotalSeconds:0} seconds");
                    return null;
                }
            }

            output = await outputRead.ConfigureAwait(false);
            var errors = await errorRead.ConfigureAwait(false);
            if (process.ExitCode != 0)
            {
                // az prints warnings before the error, such as an available update.
                var lines = errors.Split('\n', StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries);
                var reason = lines.FirstOrDefault(line => line.StartsWith("ERROR", StringComparison.OrdinalIgnoreCase)) ?? lines.FirstOrDefault();
                FailToken(reason ?? $"az exited with code {process.ExitCode}");
                return null;
            }
        }
        catch (Exception error) when (error is Win32Exception or InvalidOperationException or IOException)
        {
            FailToken(error.Message);
            return null;
        }

        try
        {
            return ParseToken(output);
        }
        catch (Exception error) when (error is JsonException or ArgumentException or InvalidOperationException)
        {
            FailToken($"its output is not a token: {error.Message}");
            return null;
        }
    }

    private SymwebToken? ParseToken(string output)
    {
        using var document = JsonDocument.Parse(output);
        var root = document.RootElement;
        var value = root.TryGetProperty("accessToken", out var accessToken) ? accessToken.GetString() : null;
        if (string.IsNullOrEmpty(value))
        {
            FailToken("its output has no accessToken");
            return null;
        }

        var now = DateTimeOffset.UtcNow;
        var expires = now + UnknownTokenLifetime;

        // expires_on (seconds since 1970) is in newer CLIs; expiresOn (local time) is in all of them.
        if (root.TryGetProperty("expires_on", out var epoch) && EpochSeconds(epoch) is { } seconds)
        {
            expires = DateTimeOffset.FromUnixTimeSeconds(seconds);
        }
        else if (root.TryGetProperty("expiresOn", out var local) &&
            local.ValueKind == JsonValueKind.String &&
            DateTime.TryParseExact(local.GetString(), LocalExpiryFormats, CultureInfo.InvariantCulture, DateTimeStyles.AssumeLocal, out var localTime))
        {
            expires = new DateTimeOffset(localTime).ToUniversalTime();
        }

        var refresh = expires - RefreshBeforeExpiry;
        return new SymwebToken(value, refresh > now + MinimumTokenUse ? refresh : now + MinimumTokenUse);
    }

    private static long? EpochSeconds(JsonElement element) => element.ValueKind switch
    {
        JsonValueKind.Number when element.TryGetInt64(out var seconds) => seconds,
        JsonValueKind.String when long.TryParse(element.GetString(), NumberStyles.None, CultureInfo.InvariantCulture, out var seconds) => seconds,
        _ => null,
    };

    private static void Kill(Process process)
    {
        try
        {
            process.Kill(entireProcessTree: true);
        }
        catch (Exception error) when (error is InvalidOperationException or Win32Exception or NotSupportedException)
        {
            Trace.TraceWarning($"Could not stop az: {error.Message}");
        }
    }
}
