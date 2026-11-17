// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Diagnostics.CodeAnalysis;
using System.Globalization;
using System.Net.Http.Headers;
using System.Text.Json;

namespace WinUITools.Telemetry;

/// <summary>
/// Sends a few usage events to one Application Insights resource. Telemetry is off unless
/// WINUI_TOOLS_TELEMETRY_CONNECTION_STRING is set, and WINUI_TOOLS_TELEMETRY_OPTOUT=1 turns it off. No method throws,
/// waits for the network on the caller's thread, or writes to standard output, which carries the MCP protocol.
/// </summary>
/// <remarks>This file is shared with other WinUI tools; keep the copies identical.</remarks>
public static class UsageTelemetry
{
    private const string ConnectionStringVariable = "WINUI_TOOLS_TELEMETRY_CONNECTION_STRING";
    private const string OptOutVariable = "WINUI_TOOLS_TELEMETRY_OPTOUT";
    private const string ChannelVariable = "WINUI_TOOLS_CHANNEL";
    private const string Notice = "WinUI Tools collect anonymous usage data to help the WinUI team improve the tools. " +
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
    public static void Initialize(string tool, string component, string toolVersion) =>
        Initialize(tool, component, toolVersion,
            Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "WinUITools"), null);

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

    // folder holds install-id and notice-shown; handler replaces the network in tests.
    internal static void Initialize(string tool, string component, string toolVersion, string folder,
        HttpMessageHandler? handler)
    {
        try
        {
            if (IsEnabled || Environment.GetEnvironmentVariable(OptOutVariable) == "1" ||
                !TryParse(Environment.GetEnvironmentVariable(ConnectionStringVariable), out var key, out var endpoint))
            {
                return;
            }

            var installId = LoadInstallId(folder);
            var version = string.IsNullOrWhiteSpace(toolVersion) ? "unknown" : toolVersion.Split('+')[0];
            var channel = Environment.GetEnvironmentVariable(ChannelVariable);
            var client = handler is null ? new HttpClient() : new HttpClient(handler, disposeHandler: false);
            client.Timeout = SendTimeout;
            Volatile.Write(ref current, new Session(client, endpoint, key, installId, Guid.NewGuid().ToString(), folder,
                version, new Dictionary<string, string>
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

    private static bool TryParse(string? connectionString, [NotNullWhen(true)] out string? key,
        [NotNullWhen(true)] out Uri? endpoint)
    {
        key = null;
        endpoint = null;
        if (string.IsNullOrWhiteSpace(connectionString))
        {
            return false;
        }

        string? ingestion = null;
        foreach (var part in connectionString.Split(';', StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries))
        {
            var separator = part.IndexOf('=');
            if (separator <= 0)
            {
                continue;
            }

            var name = part[..separator].Trim();
            var value = part[(separator + 1)..].Trim();
            if (name.Equals("InstrumentationKey", StringComparison.OrdinalIgnoreCase))
            {
                key = value;
            }
            else if (name.Equals("IngestionEndpoint", StringComparison.OrdinalIgnoreCase))
            {
                ingestion = value.EndsWith('/') ? value : value + "/";
            }
        }

        if (!Guid.TryParse(key, out _) || !Uri.TryCreate(ingestion, UriKind.Absolute, out var root) ||
            root.Scheme != Uri.UriSchemeHttps)
        {
            key = null;
            return false;
        }

        endpoint = new Uri(root, "v2/track");
        return true;
    }

    // A random ID shared by the tools of one user; never the alias, machine name or a hardware ID.
    private static string LoadInstallId(string folder)
    {
        var path = Path.Combine(folder, "install-id");
        if (File.Exists(path) && Guid.TryParse(File.ReadAllText(path).Trim(), out var saved))
        {
            return saved.ToString();
        }

        Directory.CreateDirectory(folder);
        var created = Guid.NewGuid().ToString();
        File.WriteAllText(path, created);
        return created;
    }

    private static async Task SendAsync(Session session, byte[] body)
    {
        try
        {
            using var content = new ByteArrayContent(body);
            content.Headers.ContentType = new MediaTypeHeaderValue("application/json");
            using var response = await session.Client.PostAsync(session.Endpoint, content).ConfigureAwait(false);
        }
        catch (Exception)
        {
            // Dropped; there is no retry.
        }
    }

    // The Application Insights event envelope. The common properties come first and cannot be replaced.
    private static byte[] Serialize(Session session, string eventName, IDictionary<string, string>? properties,
        IDictionary<string, double>? measurements)
    {
        using var stream = new MemoryStream();
        using (var json = new Utf8JsonWriter(stream))
        {
            json.WriteStartObject();
            json.WriteString("name", "Microsoft.ApplicationInsights.Event");
            json.WriteString("time", DateTime.UtcNow.ToString("o", CultureInfo.InvariantCulture));
            json.WriteString("iKey", session.Key);
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

    private sealed record Session(HttpClient Client, Uri Endpoint, string Key, string InstallId, string Id, string Folder,
        string Version, IReadOnlyDictionary<string, string> Common);
}
