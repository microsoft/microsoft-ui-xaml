// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Diagnostics.CodeAnalysis;
using System.Globalization;
using System.Net.Http.Headers;
using System.Security.Cryptography;
using System.Text;
using System.Text.Json;
using Microsoft.Identity.Client;
using Microsoft.Identity.Client.Broker;

namespace WinUITools.Telemetry;

/// <summary>
/// Sends a few usage events to the WinUI tools telemetry endpoint, which forwards them to Application Insights.
/// Telemetry is off unless WINUI_TOOLS_TELEMETRY_ENDPOINT is set or the tool turns it on by default, and
/// WINUI_TOOLS_TELEMETRY_OPTOUT=1 turns it off. Each send carries a Microsoft Entra token that Windows gives the
/// signed-in work account without a prompt; where it has none, nothing is sent. No method throws, waits for the network
/// on the caller's thread, or writes to standard output, which carries the MCP protocol.
/// </summary>
/// <remarks>This file is shared with other WinUI tools; keep the copies identical.</remarks>
public static class UsageTelemetry
{
    private const string EndpointVariable = "WINUI_TOOLS_TELEMETRY_ENDPOINT";
    private const string OptOutVariable = "WINUI_TOOLS_TELEMETRY_OPTOUT";
    private const string ChannelVariable = "WINUI_TOOLS_CHANNEL";

    // The endpoint holds the Application Insights key, so the tools hold none. It accepts only tokens of the app
    // registration "WinUI Tools Telemetry" in the Microsoft tenant.
    private const string Endpoint = "https://app-winui-tools-telemetry-dnh3c7fdazhycfe7.westus3-01.azurewebsites.net/";
    private const string ClientId = "20fc6284-b18f-4874-bbf1-52f6a17fc2a7";
    private const string TenantId = "72f988bf-86f1-41af-91ab-2d7cd011db47";
    private static readonly string[] Scopes = [$"api://{ClientId}/Telemetry.Send"];
    private const string Notice = "WinUI Tools send usage data to the WinUI team to help improve the tools. " +
        "The data does not include your code, file paths, prompts or app content. To turn it off, set " +
        "WINUI_TOOLS_TELEMETRY_OPTOUT=1.";

    private static readonly TimeSpan SendTimeout = TimeSpan.FromSeconds(2);
    private static readonly object Gate = new();
    private static readonly HashSet<Task> Pending = [];
    private static Session? current;

    // Each event waits in folder\pending\<endpoint> until that endpoint takes it: as name.sending while a process sends
    // it, and as name.pending after a send that may succeed later failed. The endpoint refuses events older than two days.
    private const string PendingFolder = "pending";
    private const int MaxSaved = 500;
    private const int MaxBatchEvents = 50;
    private const int MaxBatchBytes = 60_000;
    private static readonly TimeSpan MaxSavedAge = TimeSpan.FromHours(48);
    private static readonly TimeSpan AbandonedAfter = TimeSpan.FromMinutes(1);

    public static bool IsEnabled => Volatile.Read(ref current) is not null;

    /// <summary>Turns telemetry on when the settings allow it.</summary>
    /// <param name="tool">"profiler" or "migrate".</param>
    /// <param name="component">"app", "cli" or "mcp".</param>
    /// <param name="toolVersion">The tool's version; build metadata after a '+' is removed.</param>
    /// <param name="onByDefault">Send to the WinUI tools telemetry endpoint when WINUI_TOOLS_TELEMETRY_ENDPOINT is not set,
    /// instead of staying off.</param>
    public static void Initialize(string tool, string component, string toolVersion, bool onByDefault = false) =>
        Initialize(tool, component, toolVersion,
            Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "WinUITools"), null,
            onByDefault: onByDefault);

    /// <summary>
    /// Returns the first-run notice while telemetry is on and the notice has not been shown to this user, and records
    /// that it was shown; otherwise null.
    /// </summary>
    public static string? TakeNotice()
    {
        try
        {
            if (Volatile.Read(ref current) is not { } session)
            {
                return null;
            }

            var marker = Path.Combine(session.Folder, "notice-shown");
            if (File.Exists(marker))
            {
                return null;
            }

            try
            {
                File.WriteAllBytes(marker, []);
            }
            catch (Exception)
            {
                // The notice is shown now and again next time.
            }

            return Notice;
        }
        catch (Exception)
        {
            return null;
        }
    }

    /// <summary>
    /// Sends one event in the background and returns at once. An event that cannot be sent now is saved, and a later
    /// run of a WinUI tool sends it.
    /// </summary>
    public static void Track(string eventName, IDictionary<string, string>? properties = null,
        IDictionary<string, double>? measurements = null)
    {
        try
        {
            if (Volatile.Read(ref current) is not { } session)
            {
                return;
            }

            var body = Serialize(session, eventName, properties, measurements);
            Start(() => SendAsync(session, body));
        }
        catch (Exception)
        {
            // Telemetry never affects the tool.
        }
    }

    /// <summary>Waits at most maxWait for the events that are still being sent; the rest stay saved.</summary>
    public static async Task FlushAsync(TimeSpan maxWait)
    {
        try
        {
            Task[] pending;
            lock (Gate)
            {
                pending = [.. Pending];
            }

            if (pending.Length > 0)
            {
                await Task.WhenAny(Task.WhenAll(pending), Task.Delay(maxWait)).ConfigureAwait(false);
            }
        }
        catch (Exception)
        {
            // Telemetry never affects the tool.
        }
    }

    // folder holds install-id and notice-shown; handler replaces the network and token the sign-in in tests.
    internal static void Initialize(string tool, string component, string toolVersion, string folder,
        HttpMessageHandler? handler, Func<Task<string?>>? token = null, bool onByDefault = false)
    {
        try
        {
            var address = Environment.GetEnvironmentVariable(EndpointVariable);
            if (IsEnabled || Environment.GetEnvironmentVariable(OptOutVariable) == "1" ||
                !TryParse(string.IsNullOrWhiteSpace(address) && onByDefault ? Endpoint : address, out var endpoint))
            {
                return;
            }

            if (LoadInstallId(folder) is not { } installId)
            {
                return;
            }

            var version = string.IsNullOrWhiteSpace(toolVersion) ? "unknown" : toolVersion.Split('+')[0];
            var channel = Environment.GetEnvironmentVariable(ChannelVariable);
            var client = handler is null ? new HttpClient() : new HttpClient(handler, disposeHandler: false);
            client.Timeout = SendTimeout;
            var session = new Session(client, endpoint, token ?? SignIn(), installId, Guid.NewGuid().ToString(), folder,
                version, new Dictionary<string, string>
                {
                    ["tool"] = tool,
                    ["component"] = component,
                    ["toolVersion"] = version,
                    ["channel"] = string.IsNullOrWhiteSpace(channel) ? "unknown" : channel,
                    ["isCI"] = IsSet("TF_BUILD") || IsSet("GITHUB_ACTIONS") ? "true" : "false",
                });
            Volatile.Write(ref current, session);
            Start(() => ResendAsync(session));
        }
        catch (Exception)
        {
            // Telemetry stays off.
        }
    }

    // Turns telemetry off and forgets the events still being sent, so that a test can initialize again.
    internal static void Reset()
    {
        var session = Interlocked.Exchange(ref current, null);
        lock (Gate)
        {
            Pending.Clear();
        }

        session?.Client.Dispose();
    }

    private static bool IsSet(string variable) => !string.IsNullOrEmpty(Environment.GetEnvironmentVariable(variable));

    // Runs work in the background, where FlushAsync can wait for it.
    private static void Start(Func<Task> work)
    {
        var task = Task.Run(work);
        lock (Gate)
        {
            Pending.Add(task);
        }

        _ = task.ContinueWith(Forget, TaskScheduler.Default);
    }

    private static void Forget(Task send)
    {
        lock (Gate)
        {
            Pending.Remove(send);
        }
    }

    // The endpoint's track URL, from its https base address.
    private static bool TryParse(string? address, [NotNullWhen(true)] out Uri? endpoint)
    {
        endpoint = null;
        var text = address?.Trim();
        if (string.IsNullOrEmpty(text))
        {
            return false;
        }

        if (!Uri.TryCreate(text.EndsWith('/') ? text : text + "/", UriKind.Absolute, out var root) ||
            root.Scheme != Uri.UriSchemeHttps)
        {
            return false;
        }

        endpoint = new Uri(root, "v2/track");
        return true;
    }

    // A token for the endpoint from the account Windows is signed in with, without a prompt; null where there is none,
    // such as on a machine signed in with no Microsoft work account. It throws when signing in may work later, such as
    // offline, so that the event is kept. One request runs at a time, and its token serves every send until five
    // minutes before it expires; without a token, the next send asks again.
    private static Func<Task<string?>> SignIn()
    {
        var app = PublicClientApplicationBuilder.Create(ClientId)
            .WithAuthority(AzureCloudInstance.AzurePublic, TenantId)
            .WithBroker(new BrokerOptions(BrokerOptions.OperatingSystems.Windows))
            .Build();
        var gate = new object();
        Task<SignInResult>? request = null;
        return () =>
        {
            Task<SignInResult> current;
            lock (gate)
            {
                if (request is null || (request.IsCompleted &&
                    (request.Result.Token is not { } done || done.ExpiresOn < DateTimeOffset.UtcNow.AddMinutes(5))))
                {
                    request = AcquireAsync(app);
                }

                current = request;
            }

            return AccessTokenAsync(current);
        };
    }

    // No account to sign in with silently is final; no network, an outage or throttling is not.
    private static async Task<SignInResult> AcquireAsync(IPublicClientApplication app)
    {
        try
        {
            return new(await app.AcquireTokenSilent(Scopes, PublicClientApplication.OperatingSystemAccount)
                .ExecuteAsync().ConfigureAwait(false), TryLater: false);
        }
        catch (MsalUiRequiredException)
        {
            return new(null, TryLater: false);
        }
        catch (MsalException error)
        {
            return new(null, error.IsRetryable);
        }
        catch (Exception)
        {
            return new(null, TryLater: true);
        }
    }

    private static async Task<string?> AccessTokenAsync(Task<SignInResult> request)
    {
        var result = await request.ConfigureAwait(false);
        return result.TryLater ? throw new TimeoutException("Signing in is not possible now.") : result.Token?.AccessToken;
    }

    // A random ID shared by the tools of one user; never the alias, machine name or a hardware ID. It is read, and written
    // when the file holds none, under one exclusive lock, so tools that start together on first use share one ID. Null
    // when the file stays locked: telemetry then stays off rather than count a second install.
    internal static string? LoadInstallId(string folder)
    {
        var path = Path.Combine(folder, "install-id");
        Directory.CreateDirectory(folder);
        for (var attempt = 0; attempt < 20; attempt++)
        {
            try
            {
                using var file = new FileStream(path, FileMode.OpenOrCreate, FileAccess.ReadWrite, FileShare.None);
                using (var reader = new StreamReader(file, Encoding.UTF8, detectEncodingFromByteOrderMarks: true,
                    bufferSize: 256, leaveOpen: true))
                {
                    if (Guid.TryParse(reader.ReadToEnd().Trim(), out var saved))
                    {
                        return saved.ToString();
                    }
                }

                var created = Guid.NewGuid().ToString();
                file.SetLength(0);
                file.Position = 0;
                file.Write(Encoding.ASCII.GetBytes(created));
                return created;
            }
            catch (IOException)
            {
                // Another tool holds the file for a moment.
                Thread.Sleep(25);
            }
        }

        return null;
    }

    // Saved first, so that an event a crash, a slow sign-in or an unreachable endpoint interrupts is sent by a later run.
    private static async Task SendAsync(Session session, byte[] body)
    {
        var saved = Save(session, body);
        try
        {
            if (await session.Token().ConfigureAwait(false) is not { } token)
            {
                // Without a Microsoft work account nothing is ever sent, so nothing is kept.
                Settle(saved, Outcome.Refused);
                return;
            }

            Settle(saved, await PostAsync(session, token, body).ConfigureAwait(false));
        }
        catch (Exception)
        {
            // Signing in may work later.
            Settle(saved, Outcome.Retry);
        }
    }

    // body is one envelope or an array of them.
    private static async Task<Outcome> PostAsync(Session session, string token, byte[] body)
    {
        try
        {
            using var request = new HttpRequestMessage(HttpMethod.Post, session.Endpoint)
            {
                Content = new ByteArrayContent(body),
            };
            request.Content.Headers.ContentType = new MediaTypeHeaderValue("application/json");
            request.Headers.Authorization = new AuthenticationHeaderValue("Bearer", token);
            using var response = await session.Client.SendAsync(request).ConfigureAwait(false);

            // The endpoint refuses for good what is malformed, unknown or too large. A 403 is not final: App Service also
            // answers it while the site is stopped or over a quota.
            return response.IsSuccessStatusCode ? Outcome.Sent
                : (int)response.StatusCode is 400 or 413 ? Outcome.Refused
                : Outcome.Retry;
        }
        catch (Exception)
        {
            return Outcome.Retry;
        }
    }

    // One file for each event, named by the UTC time it was saved; null when it cannot be saved.
    private static string? Save(Session session, byte[] body)
    {
        try
        {
            var pending = Directory.CreateDirectory(SavedFolder(session.Folder, session.Endpoint)).FullName;
            var path = Path.Combine(pending, DateTime.UtcNow.ToString("yyyyMMddHHmmssfff", CultureInfo.InvariantCulture) +
                "-" + Guid.NewGuid().ToString("N") + ".sending");
            File.WriteAllBytes(path, body);
            return path;
        }
        catch (Exception)
        {
            return null;
        }
    }

    // The folder of the events saved for an endpoint, so that a run sends only those meant for its own endpoint.
    private static string SavedFolder(string folder, Uri endpoint) => Path.Combine(folder, PendingFolder,
        Convert.ToHexStringLower(SHA256.HashData(Encoding.UTF8.GetBytes(endpoint.AbsoluteUri)))[..16]);

    // The same, for an endpoint setting; for tests.
    internal static string SavedFolder(string folder, string endpoint) =>
        TryParse(endpoint, out var uri) ? SavedFolder(folder, uri) : throw new ArgumentException("Not an endpoint.", nameof(endpoint));

    // Deletes the file of an event that was sent or refused, and keeps one that may be sent later as name.pending.
    private static void Settle(string? path, Outcome outcome)
    {
        try
        {
            if (path is null)
            {
                return;
            }

            var waiting = Path.ChangeExtension(path, ".pending");
            if (outcome != Outcome.Retry)
            {
                File.Delete(path);
            }
            else if (!string.Equals(path, waiting, StringComparison.OrdinalIgnoreCase))
            {
                File.Move(path, waiting, overwrite: true);
            }
        }
        catch (Exception)
        {
            // A file left behind is sent, or dropped, by a later run.
        }
    }

    // Asks for the token at once, so that it is ready when the tool sends its events, often just before it exits. Then
    // sends the events earlier runs saved for this run's endpoint, oldest first and in batches: those whose send failed,
    // and those a run left mid-send. One process at a time does this for an endpoint, so no event is sent twice. Events
    // the endpoint would refuse as too old are dropped for every endpoint, and the oldest beyond MaxSaved for this one.
    private static async Task ResendAsync(Session session)
    {
        try
        {
            var signIn = session.Token();
            var root = Path.Combine(session.Folder, PendingFolder);
            if (!Directory.Exists(root))
            {
                return;
            }

            var now = DateTime.UtcNow;
            var folder = SavedFolder(session.Folder, session.Endpoint);
            foreach (var other in Directory.EnumerateDirectories(root).Append(root))
            {
                if (!string.Equals(Path.GetFullPath(other), Path.GetFullPath(folder), StringComparison.OrdinalIgnoreCase))
                {
                    foreach (var path in Directory.EnumerateFiles(other).Where(path => Expired(path, now)))
                    {
                        Delete(path);
                    }
                }
            }

            if (!Directory.Exists(folder))
            {
                return;
            }

            using var gate = Lock(Path.Combine(folder, "resend.lock"));
            if (gate is null)
            {
                return;
            }

            var waiting = new List<string>();
            foreach (var path in Directory.EnumerateFiles(folder).Order(StringComparer.Ordinal))
            {
                var extension = Path.GetExtension(path);
                if (extension is not (".pending" or ".sending"))
                {
                    continue;
                }

                if (Expired(path, now))
                {
                    Delete(path);
                }
                else if (extension == ".pending" || now - File.GetLastWriteTimeUtc(path) > AbandonedAfter)
                {
                    waiting.Add(path);
                }
            }

            foreach (var oldest in waiting.Take(waiting.Count - MaxSaved))
            {
                Delete(oldest);
            }

            waiting = [.. waiting.Skip(Math.Max(0, waiting.Count - MaxSaved))];
            if (waiting.Count == 0 || await signIn.ConfigureAwait(false) is not { } token)
            {
                return;
            }

            for (var next = 0; next < waiting.Count;)
            {
                var batch = new List<string>();
                var bodies = new List<byte[]>();
                var bytes = 2;
                while (next < waiting.Count && batch.Count < MaxBatchEvents)
                {
                    var body = Read(waiting[next]);
                    if (body is null)
                    {
                        Delete(waiting[next++]);
                        continue;
                    }

                    if (batch.Count > 0 && bytes + body.Length + 1 > MaxBatchBytes)
                    {
                        break;
                    }

                    batch.Add(waiting[next++]);
                    bodies.Add(body);
                    bytes += body.Length + 1;
                }

                if (batch.Count == 0)
                {
                    continue;
                }

                var outcome = await PostAsync(session, token, Join(bodies)).ConfigureAwait(false);
                foreach (var path in batch)
                {
                    Settle(path, outcome);
                }

                if (outcome == Outcome.Retry)
                {
                    // The endpoint cannot take them now; a later run tries again.
                    return;
                }
            }
        }
        catch (Exception)
        {
            // What is left is sent by a later run.
        }
    }

    // A saved event the endpoint would refuse as too old, by the UTC time its file name starts with. One whose name holds
    // no time counts as expired too, so that it is cleaned up; other files, such as the lock, never do.
    private static bool Expired(string path, DateTime now)
    {
        var name = Path.GetFileName(path);
        if (Path.GetExtension(path) is not (".pending" or ".sending"))
        {
            return false;
        }

        return name.Length < 17 || !DateTime.TryParseExact(name[..17], "yyyyMMddHHmmssfff", CultureInfo.InvariantCulture,
            DateTimeStyles.AssumeUniversal | DateTimeStyles.AdjustToUniversal, out var saved) || now - saved > MaxSavedAge;
    }

    // A saved envelope, or null when the file is unreadable or holds no JSON object.
    private static byte[]? Read(string path)
    {
        try
        {
            var body = File.ReadAllBytes(path);
            using var envelope = JsonDocument.Parse(body);
            return envelope.RootElement.ValueKind == JsonValueKind.Object ? body : null;
        }
        catch (Exception)
        {
            return null;
        }
    }

    private static byte[] Join(List<byte[]> envelopes)
    {
        using var stream = new MemoryStream();
        stream.WriteByte((byte)'[');
        for (var i = 0; i < envelopes.Count; i++)
        {
            if (i > 0)
            {
                stream.WriteByte((byte)',');
            }

            stream.Write(envelopes[i]);
        }

        stream.WriteByte((byte)']');
        return stream.ToArray();
    }

    // An exclusive handle on path, or null while another process holds it.
    private static FileStream? Lock(string path)
    {
        try
        {
            return new FileStream(path, FileMode.OpenOrCreate, FileAccess.ReadWrite, FileShare.None);
        }
        catch (Exception)
        {
            return null;
        }
    }

    private static void Delete(string path)
    {
        try
        {
            File.Delete(path);
        }
        catch (Exception)
        {
            // A later run tries again.
        }
    }

    // The Application Insights event envelope, without a key: the endpoint adds its own. The common properties come
    // first and cannot be replaced.
    private static byte[] Serialize(Session session, string eventName, IDictionary<string, string>? properties,
        IDictionary<string, double>? measurements)
    {
        using var stream = new MemoryStream();
        using (var json = new Utf8JsonWriter(stream))
        {
            json.WriteStartObject();
            json.WriteString("name", "Microsoft.ApplicationInsights.Event");
            json.WriteString("time", DateTime.UtcNow.ToString("o", CultureInfo.InvariantCulture));
            json.WriteStartObject("tags");
            json.WriteString("ai.user.id", session.InstallId);
            json.WriteString("ai.session.id", session.Id);
            json.WriteString("ai.application.ver", session.Version);
            json.WriteEndObject();
            json.WriteStartObject("data");
            json.WriteString("baseType", "EventData");
            json.WriteStartObject("baseData");
            json.WriteNumber("ver", 2);
            json.WriteString("name", eventName);
            json.WriteStartObject("properties");
            foreach (var (name, value) in session.Common)
            {
                json.WriteString(name, value);
            }

            if (properties is not null)
            {
                foreach (var (name, value) in properties)
                {
                    if (!session.Common.ContainsKey(name))
                    {
                        json.WriteString(name, value);
                    }
                }
            }

            json.WriteEndObject();
            json.WriteStartObject("measurements");
            if (measurements is not null)
            {
                foreach (var (name, value) in measurements)
                {
                    if (double.IsFinite(value))
                    {
                        json.WriteNumber(name, value);
                    }
                }
            }

            json.WriteEndObject();
            json.WriteEndObject();
            json.WriteEndObject();
            json.WriteEndObject();
        }

        return stream.ToArray();
    }

    private sealed record Session(HttpClient Client, Uri Endpoint, Func<Task<string?>> Token, string InstallId, string Id,
        string Folder, string Version, IReadOnlyDictionary<string, string> Common);

    // No Token with TryLater unset means there is no account to sign in with silently.
    private sealed record SignInResult(AuthenticationResult? Token, bool TryLater);

    private enum Outcome
    {
        Sent,
        Refused,
        Retry,
    }
}
