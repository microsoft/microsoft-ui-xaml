// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Globalization;
using System.Text.Json;
using WinUIProfiler.Cli;
using WinUIProfiler.Engine.Events;
using WinUIProfiler.Engine.Storage;
using WinUIProfiler.Tests.Mcp;

namespace WinUIProfiler.Tests;

internal static class CliQueryTests
{
    internal static void Run()
    {
        var directory = Path.Combine(Path.GetTempPath(), $"WinUIProfiler-CliQueries-{Guid.NewGuid():N}");
        Directory.CreateDirectory(directory);
        try
        {
            var path = SessionFixtures.Write(Path.Combine(directory, "events.sqlite"),
                [.. SessionFixtures.MainEvents, SessionFixtures.Tl(31, SessionFixtures.UiThread, XamlProvider.MicrosoftUIXaml,
                    "WideFields", new("Unsigned", ulong.MaxValue), new("Signed", long.MinValue), new("Missing", null))],
                lostByEtw: 3, droppedByProfiler: 2);
            var markers = Path.Combine(directory, "markers.json");
            File.WriteAllText(markers, """{"version":1,"providers":[],"milestones":[]}""");
            JsonQueries(path, markers);
            EventPaging(path);
            RegionQueries(directory);
            InvalidQueries(path, directory);
        }
        finally
        {
            Directory.Delete(directory, recursive: true);
        }

        Console.WriteLine("Passed: CLI structured queries, event drilldown, object links, pagination and errors.");
    }

    private static void JsonQueries(string path, string markers)
    {
        var objectId = SessionFile.ReadObjects(path).First().ObjectId.ToString(CultureInfo.InvariantCulture);
        string[][] commands =
        [
            ["info", path, "--json"],
            ["processes", path, "--json"],
            ["summary", path, "--json"],
            ["objects", path, "--json"],
            ["timeline", path, "--object", objectId, "--json"],
            ["frames", path, "--json"],
            ["frames", path, "--list", "--json"],
            ["interesting", path, "--json"],
            ["launch", path, "--markers", markers, "--json"]
        ];
        var culture = CultureInfo.CurrentCulture;
        try
        {
            CultureInfo.CurrentCulture = CultureInfo.GetCultureInfo("fr-FR");
            foreach (var command in commands)
            {
                var result = Invoke(command);
                Check(result.Code == 0, $"{command[0]} JSON failed: {result.Error}");
                using var document = JsonDocument.Parse(result.Output);
                Check(document.RootElement.ValueKind is JsonValueKind.Array or JsonValueKind.Object,
                    $"{command[0]} must emit one JSON document, with no prose on stdout.");
            }
        }
        finally
        {
            CultureInfo.CurrentCulture = culture;
        }

        var info = Json(["info", path, "--json"]);
        Check(info.GetProperty("LostByEtw").GetInt32() == 3 && info.GetProperty("DroppedByProfiler").GetInt32() == 2,
            "Session health must retain ETW loss and profiler drops separately.");
        var dump = Json(["dump", path, "--name", "WideFields", "--json"]);
        var fields = dump[0].GetProperty("Fields");
        Check(fields[0].GetProperty("Value").GetUInt64() == ulong.MaxValue &&
            fields[1].GetProperty("Value").GetInt64() == long.MinValue &&
            fields[2].GetProperty("Value").ValueKind == JsonValueKind.Null,
            "Event JSON must retain unsigned/signed 64-bit domains and unavailable values.");
    }

    private static void EventPaging(string path)
    {
        var frame = SessionFile.ReadFrames(path).First();
        var id = frame.FrameId.ToString(CultureInfo.InvariantCulture);
        var expected = new List<string>();
        SessionFile.ReadFrameEvents(path, frame.FrameId, null, null, 0, -1,
            (item, _, _) => expected.Add(item.Name));
        var page = Json(["events", path, "--frame", id, "--offset", "1", "--count", "2", "--json"]);
        Check(page.GetProperty("Total").GetInt64() == expected.Count &&
            page.GetProperty("Events").EnumerateArray().Select(row => row.GetProperty("Event").GetProperty("Name").GetString())
                .SequenceEqual(expected.Skip(1).Take(2)), "Paged events must retain the shared frame query's order and total.");
        Check(page.GetProperty("NextOffset").GetInt32() == 3, "The next offset must advance by the actual returned count.");

        var linked = Json(["events", path, "--frame", id, "--filter", "OkButton", "--count", "0", "--json"]);
        Check(linked.GetProperty("Events").EnumerateArray().Any(row => row.GetProperty("Objects").GetArrayLength() > 0),
            "Filtered event drilldown must include IDs usable by the object timeline command.");
        var oldFrame = Json(["frames", path, "--frame", id, "--offset", "1", "--count", "2", "--json"]);
        Check(oldFrame.GetProperty("Events").GetArrayLength() == 2, "The existing frame command must support paged JSON drilldown.");

        var objectId = SessionFile.ReadObjects(path).First().ObjectId.ToString(CultureInfo.InvariantCulture);
        var timeline = Json(["timeline", path, "--object", objectId, "--offset", "1", "--count", "1", "--json"]);
        Check(timeline[0].GetProperty("Events").GetArrayLength() == 1 &&
            timeline[0].GetProperty("Total").GetInt64() > 1, "Object timelines must page without losing total counts.");
        var dump = Json(["dump", path, "--offset", "1", "--count", "2", "--json"]);
        Check(dump.GetArrayLength() == 2 && dump[0].GetProperty("Name").GetString() == expected[1],
            "The dump's count/offset must select matching events before rendering JSON.");
        var all = Json(["dump", path, "--count", "0", "--json"]);
        Check(all.GetArrayLength() == SessionFile.ReadInfo(path).StoredEvents, "--count 0 must mean all events.");
    }

    private static void RegionQueries(string directory)
    {
        var path = SessionFixtures.Write(Path.Combine(directory, "regions.sqlite"), FrameToolTests.RegionEvents);
        var rows = SessionFile.ReadInterestingRows(path);
        var region = rows.Single(row => row.Type == InterestingRowType.Region);
        var id = region.RegionId!.Value.ToString(CultureInfo.InvariantCulture);
        var data = Json(["events", path, "--region", id, "--count", "0", "--json"]);
        Check(data.GetProperty("Total").GetInt64() ==
            SessionFile.CountSpanEvents(path, region.ThreadId, region.StartSeq, region.StopSeq, null, null),
            "A region's drilldown must use the UI's thread and sequence bounds.");
        var layout = rows.First(row => row.Type == InterestingRowType.UpdateLayout);
        var thread = layout.ThreadId.ToString(CultureInfo.InvariantCulture);
        var start = layout.StartSeq.ToString(CultureInfo.InvariantCulture);
        var stop = layout.StopSeq.ToString(CultureInfo.InvariantCulture);
        var rowEvents = Json(["events", path, "--row", start, "--thread", thread, "--json"]);
        var spanEvents = Json(["events", path, "--thread", thread, "--first-seq", start, "--last-seq", stop, "--json"]);
        Check(rowEvents.GetProperty("Total").GetInt64() > 0 &&
            rowEvents.GetProperty("Events").GetRawText() == spanEvents.GetProperty("Events").GetRawText(),
            "Interesting-row and explicit-span navigation must select identical event data.");
    }

    private static void InvalidQueries(string path, string directory)
    {
        string[][] invalid =
        [
            ["events", path],
            ["events", path, "--frame", "1", "--region", "1"],
            ["events", path, "--row", "2"],
            ["events", path, "--thread", "100", "--first-seq", "9", "--last-seq", "2"],
            ["events", path, "--frame", "1", "--count", "-1"],
            ["events", path, "--frame", "1", "--offset", "2147483648"],
            ["events", path, "--frame", "1", "--bucket", "bogus"],
            ["dump", path, "--count", "1", "--max", "1"],
            ["dump", path, "--count", "1", "--count", "2"],
            ["frames", path, "--count", "0"],
            ["frames", path, "--frame", "1", "--list"],
            ["summary", path, "--pid", "--json"]
        ];
        foreach (var args in invalid)
        {
            var result = Invoke(args);
            Check(result.Code == 2 && result.Error.Length > 0 && result.Output.Length == 0,
                $"Invalid options must fail as usage errors without success output: {string.Join(' ', args)}.");
        }

        foreach (var args in new[]
        {
            new[] { "events", path, "--frame", long.MaxValue.ToString(CultureInfo.InvariantCulture), "--json" },
            new[] { "info", Path.Combine(directory, "missing.sqlite"), "--json" }
        })
        {
            var result = Invoke(args);
            Check(result.Code == 3 && result.Error.Length > 0, "Missing query inputs must report an operational error.");
        }

        var cancelled = Invoke(["events", path, "--frame", "1", "--json"], new CancellationToken(canceled: true));
        Check(cancelled.Code == 4 && cancelled.Output.Length == 0, "Cancelled queries must preserve the CLI cancellation exit code.");
    }

    private static JsonElement Json(string[] args)
    {
        var result = Invoke(args);
        Check(result.Code == 0, $"CLI failed: {result.Error}");
        using var document = JsonDocument.Parse(result.Output);
        return document.RootElement.Clone();
    }

    private static (int Code, string Output, string Error) Invoke(string[] args, CancellationToken token = default)
    {
        using var output = new StringWriter(CultureInfo.InvariantCulture);
        using var error = new StringWriter(CultureInfo.InvariantCulture);
        var code = CliApplication.Run(args, output, error, token);
        return (code, output.ToString(), error.ToString());
    }

    private static void Check(bool condition, string message)
    {
        if (!condition)
        {
            throw new InvalidOperationException(message);
        }
    }
}
