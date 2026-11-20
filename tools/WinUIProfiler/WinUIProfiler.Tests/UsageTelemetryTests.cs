// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Diagnostics;
using System.Net;
using System.Text.Json.Nodes;
using WinUITools.Telemetry;

// The usage telemetry helper: off without a connection string or when opted out, the Application Insights envelope with
// the common properties first, one install ID and one notice for each user, and no waiting, throwing or standard output.
internal static class UsageTelemetryTests
{
    private const string ConnectionStringVariable = "WINUI_TOOLS_TELEMETRY_CONNECTION_STRING";
    private const string OptOutVariable = "WINUI_TOOLS_TELEMETRY_OPTOUT";
    private const string ChannelVariable = "WINUI_TOOLS_CHANNEL";
    private const string Key = "00000000-0000-0000-0000-000000000001";
    private const string ConnectionString =
        $"InstrumentationKey={Key};IngestionEndpoint=https://telemetry.invalid/;LiveEndpoint=https://live.invalid/";

    internal static void Run()
    {
        var saved = new[] { ConnectionStringVariable, OptOutVariable, ChannelVariable }
            .ToDictionary(name => name, Environment.GetEnvironmentVariable);
        var output = Console.Out;
        using var standardOutput = new StringWriter();
        Console.SetOut(standardOutput);
        try
        {
            OffWithoutConnectionString();
            OffWhenOptedOutOrInvalid();
            SendsTheEnvelope();
            NeverWaitsForTheNetwork();
        }
        finally
        {
            Console.SetOut(output);
            UsageTelemetry.Reset();
            foreach (var (name, value) in saved)
            {
                Environment.SetEnvironmentVariable(name, value);
            }
        }

        Expect(standardOutput.ToString().Length == 0, "Usage telemetry must not write to standard output.");
        Console.WriteLine("Passed: usage telemetry is off by default and when opted out, sends one envelope per event, " +
            "and never waits for the network, throws or writes to standard output.");
    }

    private static void OffWithoutConnectionString()
    {
        var folder = NewFolder();
        var handler = new RecordingHandler(hang: false);
        Environment.SetEnvironmentVariable(ConnectionStringVariable, null);
        Environment.SetEnvironmentVariable(OptOutVariable, null);
        UsageTelemetry.Reset();
        UsageTelemetry.Initialize("profiler", "cli", "0.1.0", folder, handler);
        UsageTelemetry.Track("Profiler.CliCommand");
        UsageTelemetry.FlushAsync(TimeSpan.FromSeconds(1)).GetAwaiter().GetResult();
        Expect(!UsageTelemetry.IsEnabled && UsageTelemetry.TakeNotice() is null && handler.Requests.Count == 0 &&
            !Directory.Exists(folder), "Without a connection string, usage telemetry must be off and create no files.");
    }

    private static void OffWhenOptedOutOrInvalid()
    {
        foreach (var (connectionString, optOut) in new (string, string?)[]
        {
            (ConnectionString, "1"),
            ($"InstrumentationKey={Key};IngestionEndpoint=http://telemetry.invalid/", null),
            ("IngestionEndpoint=https://telemetry.invalid/", null),
            ($"InstrumentationKey=not-a-key;IngestionEndpoint=https://telemetry.invalid/", null),
            ($"InstrumentationKey={Key}", null),
        })
        {
            var folder = NewFolder();
            var handler = new RecordingHandler(hang: false);
            Environment.SetEnvironmentVariable(ConnectionStringVariable, connectionString);
            Environment.SetEnvironmentVariable(OptOutVariable, optOut);
            UsageTelemetry.Reset();
            UsageTelemetry.Initialize("profiler", "cli", "0.1.0", folder, handler);
            UsageTelemetry.Track("Profiler.CliCommand");
            UsageTelemetry.FlushAsync(TimeSpan.FromSeconds(1)).GetAwaiter().GetResult();
            Expect(!UsageTelemetry.IsEnabled && handler.Requests.Count == 0 && !Directory.Exists(folder),
                $"Usage telemetry must stay off for '{connectionString}' with opt-out '{optOut}'.");
        }

        Environment.SetEnvironmentVariable(OptOutVariable, null);
    }

    private static void SendsTheEnvelope()
    {
        var folder = NewFolder();
        try
        {
            Environment.SetEnvironmentVariable(ConnectionStringVariable, ConnectionString);
            Environment.SetEnvironmentVariable(ChannelVariable, "test");
            var handler = new RecordingHandler(hang: false);
            UsageTelemetry.Reset();
            UsageTelemetry.Initialize("profiler", "mcp", "0.1.0+0123456789abcdef", folder, handler);
            Expect(UsageTelemetry.IsEnabled, "A valid connection string must turn usage telemetry on.");
            Expect(UsageTelemetry.TakeNotice() is { Length: > 0 } && UsageTelemetry.TakeNotice() is null,
                "The notice must be shown once.");
            UsageTelemetry.Track("Profiler.McpToolCall",
                new Dictionary<string, string> { ["mcpTool"] = "frames", ["outcome"] = "success", ["tool"] = "other" },
                new Dictionary<string, double> { ["durationMs"] = 412, ["unmeasured"] = double.NaN });
            UsageTelemetry.FlushAsync(TimeSpan.FromSeconds(10)).GetAwaiter().GetResult();
            Expect(handler.Requests.Count == 1, "Each event must be sent once.");

            var (uri, body) = handler.Requests[0];
            Expect(uri?.AbsoluteUri == "https://telemetry.invalid/v2/track",
                "Events must be posted to v2/track of the connection string's ingestion endpoint.");
            var envelope = JsonNode.Parse(body)!;
            var tags = envelope["tags"]!;
            var data = envelope["data"]!;
            var baseData = data["baseData"]!;
            var installId = File.ReadAllText(Path.Combine(folder, "install-id"));
            Expect((string?)envelope["name"] == "Microsoft.ApplicationInsights.Event" && (string?)envelope["iKey"] == Key &&
                Guid.TryParse(installId, out _) && (string?)tags["ai.user.id"] == installId &&
                Guid.TryParse((string?)tags["ai.session.id"], out _) && (string?)tags["ai.application.ver"] == "0.1.0" &&
                (string?)data["baseType"] == "EventData" && (int?)baseData["ver"] == 2 &&
                (string?)baseData["name"] == "Profiler.McpToolCall",
                "Events must use the Application Insights envelope with the install ID, a session ID and the version.");

            var isCI = !string.IsNullOrEmpty(Environment.GetEnvironmentVariable("TF_BUILD")) ||
                !string.IsNullOrEmpty(Environment.GetEnvironmentVariable("GITHUB_ACTIONS"));
            var properties = string.Join(",", baseData["properties"]!.AsObject().Select(pair => $"{pair.Key}={pair.Value}"));
            Expect(properties == $"tool=profiler,component=mcp,toolVersion=0.1.0,channel=test,isCI={(isCI ? "true" : "false")}," +
                "mcpTool=frames,outcome=success", "Events must carry the common properties, which their own cannot replace.");
            var measurements = baseData["measurements"]!.AsObject();
            Expect(measurements.Count == 1 && (double?)measurements["durationMs"] == 412,
                "Only finite measurements must be sent.");

            UsageTelemetry.Reset();
            UsageTelemetry.Initialize("migrate", "cli", "0.2.4", folder, handler);
            UsageTelemetry.Track("Migrate.Command");
            UsageTelemetry.FlushAsync(TimeSpan.FromSeconds(10)).GetAwaiter().GetResult();
            Expect(handler.Requests.Count == 2 &&
                (string?)JsonNode.Parse(handler.Requests[1].Body)!["tags"]!["ai.user.id"] == installId &&
                UsageTelemetry.TakeNotice() is null, "The tools of one user must share one install ID and one notice.");
        }
        finally
        {
            UsageTelemetry.Reset();
            Environment.SetEnvironmentVariable(ChannelVariable, null);
            Delete(folder);
        }
    }

    // With no network, Track returns at once and FlushAsync gives up after its wait.
    private static void NeverWaitsForTheNetwork()
    {
        var folder = NewFolder();
        try
        {
            Environment.SetEnvironmentVariable(ConnectionStringVariable, ConnectionString);
            var handler = new RecordingHandler(hang: true);
            UsageTelemetry.Reset();
            UsageTelemetry.Initialize("profiler", "cli", "0.1.0", folder, handler);
            var watch = Stopwatch.StartNew();
            UsageTelemetry.Track("Profiler.CliCommand");
            var tracked = watch.Elapsed;
            UsageTelemetry.FlushAsync(TimeSpan.FromSeconds(2)).GetAwaiter().GetResult();
            Expect(tracked < TimeSpan.FromMilliseconds(500) && watch.Elapsed < TimeSpan.FromSeconds(3.5),
                $"Without a network, Track must return at once and FlushAsync within its wait ({tracked}, {watch.Elapsed}).");
        }
        finally
        {
            UsageTelemetry.Reset();
            Delete(folder);
        }
    }

    private static string NewFolder() =>
        Path.Combine(Path.GetTempPath(), $"WinUIProfiler-UsageTelemetry-{Guid.NewGuid():N}");

    private static void Delete(string folder)
    {
        if (Directory.Exists(folder))
        {
            Directory.Delete(folder, recursive: true);
        }
    }

    private static void Expect(bool condition, string message)
    {
        if (!condition)
        {
            throw new InvalidOperationException(message);
        }
    }

    // Records each request instead of sending it; when hang is set, never answers, like an unreachable network.
    private sealed class RecordingHandler(bool hang) : HttpMessageHandler
    {
        private readonly List<(Uri? Uri, string Body)> requests = [];

        public IReadOnlyList<(Uri? Uri, string Body)> Requests
        {
            get
            {
                lock (requests)
                {
                    return [.. requests];
                }
            }
        }

        protected override async Task<HttpResponseMessage> SendAsync(HttpRequestMessage request,
            CancellationToken cancellationToken)
        {
            var body = request.Content is null ? "" : await request.Content.ReadAsStringAsync(cancellationToken);
            lock (requests)
            {
                requests.Add((request.RequestUri, body));
            }

            if (hang)
            {
                await Task.Delay(Timeout.Infinite, cancellationToken);
            }

            return new HttpResponseMessage(HttpStatusCode.OK);
        }
    }
}
