// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Diagnostics.CodeAnalysis;
using System.Globalization;
using System.Net.Http.Headers;
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

    /// <summary>Sends one event in the background and returns at once. A failed send is dropped.</summary>
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
            var send = Task.Run(() => SendAsync(session, body));
            lock (Gate)
            {
                Pending.Add(send);
            }

            _ = send.ContinueWith(Forget, TaskScheduler.Default);
        }
        catch (Exception)
        {
            // Telemetry never affects the tool.
        }
    }

    /// <summary>Waits at most maxWait for the events that are still being sent.</summary>
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
            Volatile.Write(ref current, new Session(client, endpoint, token ?? SignIn(), installId,
                Guid.NewGuid().ToString(), folder, version, new Dictionary<string, string>
                {
                    ["tool"] = tool,
                    ["component"] = component,
                    ["toolVersion"] = version,
                    ["channel"] = string.IsNullOrWhiteSpace(channel) ? "unknown" : channel,
                    ["isCI"] = IsSet("TF_BUILD") || IsSet("GITHUB_ACTIONS") ? "true" : "false",
                }));
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
    // such as on a machine signed in with no Microsoft work account. The broker caches it, so each send asks again.
    private static Func<Task<string?>> SignIn()
    {
        var app = PublicClientApplicationBuilder.Create(ClientId)
            .WithAuthority(AzureCloudInstance.AzurePublic, TenantId)
            .WithBroker(new BrokerOptions(BrokerOptions.OperatingSystems.Windows))
            .Build();
        return async () =>
        {
            try
            {
                var result = await app.AcquireTokenSilent(Scopes, PublicClientApplication.OperatingSystemAccount)
                    .ExecuteAsync().ConfigureAwait(false);
                return result.AccessToken;
            }
            catch (Exception)
            {
                return null;
            }
        };
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

    private static async Task SendAsync(Session session, byte[] body)
    {
        try
        {
            if (await session.Token().ConfigureAwait(false) is not { } token)
            {
                return;
            }

            using var request = new HttpRequestMessage(HttpMethod.Post, session.Endpoint)
            {
                Content = new ByteArrayContent(body),
            };
            request.Content.Headers.ContentType = new MediaTypeHeaderValue("application/json");
            request.Headers.Authorization = new AuthenticationHeaderValue("Bearer", token);
            using var response = await session.Client.SendAsync(request).ConfigureAwait(false);
        }
        catch (Exception)
        {
            // Dropped; there is no retry.
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
}
