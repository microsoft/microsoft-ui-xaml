// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Globalization;
using System.Text.Json;
using Microsoft.Data.Sqlite;
using WinUIProfiler.Engine.Storage;
using WinUIProfiler.Mcp.Sessions;
using WinUIProfiler.Mcp.Tools;
using static WinUIProfiler.Mcp.SelfTest.SessionFixtures;
using static WinUIProfiler.Mcp.SelfTest.ToolResults;

namespace WinUIProfiler.Mcp.SelfTest;

/// <summary>Finding session files, and what the sessions tool's list and summary views say about them.</summary>
internal static class SessionToolSelfTests
{
    private const string Variable = "WINUIPROFILER_MCP_SELFTEST";
    private const string NotReadToEnd = "wasn't read to its end";

    public static IEnumerable<SelfTestCase> Tests =>
    [
        new("sessions.list", List),
        new("sessions.resolve", Resolve),
        new("sessions.summary", Summary),
        new("sessions.notes", Notes),
        new("sessions.times", Times),
        new("sessions.damaged", Damaged),
        new("sessions.recording", Recording),
        new("sessions.newer", Newer),
    ];

    private static void List(SelfTestContext context)
    {
        var folder = Directory.CreateDirectory(Path.Combine(context.TempDirectory, "list")).FullName;
        var now = DateTime.UtcNow;
        var main = Write(Path.Combine(folder, "main.sqlite"), MainEvents);
        var lossy = Write(Path.Combine(folder, "lossy.sqlite"), MainEvents, lostByEtw: 3, droppedByProfiler: 2);
        var foreign = WriteForeign(Path.Combine(folder, "foreign.sqlite"));
        var garbage = WriteGarbage(Path.Combine(folder, "garbage.sqlite"));
        File.WriteAllText(Path.Combine(folder, "notes.txt"), "Not a session file.");
        File.WriteAllText(Path.Combine(folder, "other.sqlite3"), "Not a session file.");
        File.SetLastWriteTimeUtc(main, now.AddMinutes(-1));
        File.SetLastWriteTimeUtc(lossy, now.AddMinutes(-2));
        File.SetLastWriteTimeUtc(foreign, now.AddMinutes(-3));
        File.SetLastWriteTimeUtc(garbage, now.AddMinutes(-4));

        var result = Run(() => SessionTools.Sessions(folder: folder));
        Expect.Equal("list", result.Text("view"), "The view without a session path");
        Expect.Equal(folder, result.Text("folder"), "The folder");
        Expect.Equal(4L, result.Long("total"), "Session files");
        Expect.Equal(4L, result.Long("returned"), "Session files described");
        Expect.Equal(0, result.Strings("notes").Count, "Notes on a list of live sessions");
        var sessions = result.Items("sessions");
        Expect.Equal("main.sqlite,lossy.sqlite,foreign.sqlite,garbage.sqlite",
            string.Join(",", sessions.Select(item => item.Text("file"))), "The files, newest first");

        var first = sessions[0];
        Expect.Equal(main, first.Text("path"), "The path");
        Expect.Equal((long)ProcessId, first.Long("processId"), "The process ID");
        Expect.Equal(ProcessName, first.Text("processName"), "The process name");
        Expect.Equal("Live", first.Text("source"), "The source");
        Expect.Equal("ProcessStart", first.Text("coverage"), "The coverage");
        Expect.Equal(EndReason, first.Text("endReason"), "The end reason");
        Expect.Equal(16L, first.Long("storedEvents"), "Stored events");
        Expect.Equal(3L, first.Long("ticks"), "Ticks");
        Expect.Equal(2L, first.Long("uiElements"), "UI elements");
        Expect.Equal(2L, first.Long("visuals"), "Visuals");
        var written = DateTime.Parse(first.Text("lastWrittenUtc")!, CultureInfo.InvariantCulture, DateTimeStyles.RoundtripKind);
        Expect.True(Math.Abs((written - now.AddMinutes(-1)).TotalSeconds) < 2, $"The last write time is the file's; it was {written:O}.");
        Expect.True(first.IsNull("lostByEtw") && first.IsNull("droppedByProfiler") && first.IsNull("trimmedBeforeSeq") &&
            first.IsNull("etlPath") && first.IsNull("error"), "Nothing was lost, trimmed or read from a trace.");
        Expect.Equal(3L, sessions[1].Long("lostByEtw"), "Events ETW lost");
        Expect.Equal(2L, sessions[1].Long("droppedByProfiler"), "Events the profiler dropped");
        Contains(sessions[2].Text("error"), "Another SQLite file's error", $"'{foreign}' is not a WinUI Profiler session file",
            "found version 7");
        Contains(sessions[3].Text("error"), "A damaged file's error", $"{garbage} couldn't be read as a session file:");
        Expect.True(sessions[3].IsNull("processId"), "A damaged file has no process.");

        // The list reads the session record of each file that opens; the other two fail before a statement runs.
        Expect.Equal("Listed the 4 session files in the folder, newest first, and read the session record of each; 2 files " +
            "couldn't be read, so their rows have an error instead.", result.Text("computed"), "What the list view computed");
        RecordsRead(result, "The list view");

        var two = Run(() => SessionTools.Sessions(view: " LIST ", folder: folder, limit: 2));
        Expect.Equal(4L, two.Long("total"), "Session files when two are described");
        Expect.Equal(2L, two.Long("returned"), "Session files described");
        Expect.Equal("Only the newest 2 of 4 session files are described; raise limit for more.",
            string.Join(" | ", two.Strings("notes")), "The notes");
        Expect.Equal("Listed the 4 session files in the folder, newest first, and read the session record of the newest 2.",
            two.Text("computed"), "What the list view of the newest two computed");
        RecordsRead(two, "The list view of the newest two");
        Expect.Equal("limit must be from 1 to 500; it was 0.", Error(() => SessionTools.Sessions(folder: folder, limit: 0)), "limit 0");
        Expect.Equal("'files' isn't a view of sessions. Use one of: list, summary.",
            Error(() => SessionTools.Sessions(view: "files", folder: folder)), "An unknown view");
        Expect.Equal("The list view of sessions doesn't read sessionPath; only the summary view does.",
            Error(() => SessionTools.Sessions(view: "list", sessionPath: main, folder: folder)), "sessionPath in the list view");
        Expect.Equal("The summary view of sessions doesn't read limit; only the list view does.",
            Error(() => SessionTools.Sessions(sessionPath: main, limit: 5)), "limit in the summary view");

        var empty = Directory.CreateDirectory(Path.Combine(context.TempDirectory, "empty")).FullName;
        var none = Run(() => SessionTools.Sessions(folder: empty));
        Expect.Equal(0L, none.Long("total"), "Session files in an empty folder");
        var emptyNotes = none.Strings("notes");
        Expect.Equal(1, emptyNotes.Count, "An empty folder's notes");
        Contains(emptyNotes[0], "An empty folder's note", $"There are no session files in {empty}.", "record_trace");
        Expect.Equal("Found no session files in the folder, so read none.", none.Text("computed"),
            "What the list view of an empty folder computed");
        Expect.Equal(0, QuerySelfTests.CheckQueries(none, "The list view of an empty folder").Count,
            "Statements the list view of an empty folder ran");
        Expect.Equal(0L, Run(() => SessionTools.Sessions(folder: Path.Combine(folder, "missing"))).Long("total"),
            "Session files in a missing folder");
        Expect.Equal($"{main} is a file, not a folder. Pass it as sessionPath to the summary view.",
            Error(() => SessionTools.Sessions(folder: main)), "A file as the folder");

        // One statement, the session record's, run once on each file that opened. The list gives their paths, so the
        // statement doesn't name them again.
        void RecordsRead(JsonElement listed, string what)
        {
            var queries = QuerySelfTests.CheckQueries(listed, what);
            Expect.Equal(1, queries.Count, $"Statements {what} ran");
            Expect.True(queries[0].Text("sql")!.Contains("FROM SessionInfo", StringComparison.Ordinal),
                $"{what} read the session records: {queries[0].Text("sql")}");
            Expect.Equal(2L, queries[0].Long("runs"), $"Runs of the session record's statement in {what}");
            Expect.True(queries[0].IsNull("files"), $"{what} doesn't name the files it lists again: {queries[0].GetRawText()}");
        }
    }

    private static void Resolve(SelfTestContext context)
    {
        var folder = Directory.CreateDirectory(Path.Combine(context.TempDirectory, "resolve")).FullName;
        var older = Write(Path.Combine(folder, "a.sqlite"), MainEvents);
        var newer = Write(Path.Combine(folder, "b.sqlite"), MainEvents);
        File.SetLastWriteTimeUtc(older, DateTime.UtcNow.AddHours(-1));

        Expect.Equal(newer, SessionCatalog.Resolve(null, folder), "No session path");
        Expect.Equal(newer, SessionCatalog.Resolve("  ", folder), "A blank session path");
        Expect.Equal(older, SessionCatalog.Resolve("a", folder), "A file name without its extension");
        Expect.Equal(older, SessionCatalog.Resolve("a.sqlite", folder), "A file name");
        Expect.Equal(older, SessionCatalog.Resolve(older, folder), "A full path");
        Expect.Equal(older, SessionCatalog.Resolve($" \"{older}\" ", folder), "A full path in quotes");
        Expect.Equal(older, SessionCatalog.Resolve($"'{older}'", folder), "A full path in single quotes");
        Environment.SetEnvironmentVariable(Variable, folder);
        try
        {
            Expect.Equal(older, SessionCatalog.Resolve($"%{Variable}%\\a.sqlite"), "A path with an environment variable");
        }
        finally
        {
            Environment.SetEnvironmentVariable(Variable, null);
        }

        var summary = Run(() => SessionTools.Sessions(view: "summary", folder: folder));
        Expect.Equal(newer, summary.Property("session").Text("sessionPath"), "The summary view without a session path");
        Expect.Equal(older, Run(() => SessionTools.Sessions(sessionPath: "a", folder: folder)).Property("session").Text("sessionPath"),
            "A session path found in the folder");

        var trace = Path.Combine(folder, "app.etl");
        File.WriteAllText(trace, "Not a trace.");
        Contains(Error(() => SessionCatalog.Resolve(trace, folder)), "A trace", $"{trace} is a trace, not a session file.",
            "record_trace");
        Contains(Error(() => SessionCatalog.Resolve("app.etl", folder)), "A trace by name", "is a trace, not a session file.");
        var subfolder = Directory.CreateDirectory(Path.Combine(folder, "sub")).FullName;
        Expect.Equal($"{subfolder} is a folder. The sessions tool's list view lists the session files in a folder.",
            Error(() => SessionCatalog.Resolve("sub", folder)), "A folder");
        Expect.Equal($"The session file {Path.Combine(folder, "c")} was not found. The sessions tool's list view lists the " +
            $"session files in {folder}.", Error(() => SessionCatalog.Resolve("c", folder)), "A missing file");
        var empty = Directory.CreateDirectory(Path.Combine(context.TempDirectory, "resolve-empty")).FullName;
        Contains(Error(() => SessionCatalog.Resolve(null, empty)), "An empty folder", $"There are no session files in {empty}.");
    }

    private static void Summary(SelfTestContext context)
    {
        var main = WriteMain(context);
        var summary = Run(() => SessionTools.Sessions(sessionPath: main));
        Expect.Equal("summary", summary.Text("view"), "The view with a session path");
        var session = summary.Property("session");
        Expect.Equal(main, session.Text("sessionPath"), "The session path");
        Expect.Equal((long)ProcessId, session.Long("processId"), "The process ID");
        Expect.Equal(ProcessName, session.Text("processName"), "The process name");
        Expect.Equal("ProcessStart", session.Text("coverage"), "The coverage");
        Expect.Equal(EndReason, session.Text("endReason"), "The end reason");
        Contains(session.Text("timeBase"), "The time base", "Milliseconds from the start of the recording");
        Expect.Equal(16L, summary.Long("storedEvents"), "Stored events");
        Expect.Equal(3L, summary.Long("ticks"), "Ticks");
        Expect.Equal(2L, summary.Long("uiElements"), "UI elements");
        Expect.Equal(2L, summary.Long("visuals"), "Visuals");
        Expect.True(summary.IsNull("trimmedBeforeSeq"), "Nothing was trimmed.");
        Expect.True(!summary.IsNull("startedUtc") && !summary.IsNull("endedUtc"), "The session started and ended.");

        var threads = summary.Items("uiThreads");
        Expect.Equal(1, threads.Count, "UI threads");
        Expect.Equal((long)UiThread, threads[0].Long("threadId"), "The UI thread");
        Expect.Equal(3L, threads[0].Long("ticks"), "The UI thread's ticks");
        Expect.Equal(2L, threads[0].Long("drew"), "Ticks that drew");
        Expect.Equal(0L, threads[0].Long("slow"), "Slow ticks");
        Expect.Equal(4.0, threads[0].Double("firstDrawnEndMs"), "When the first tick that drew ended");
        Expect.Equal(1.0, threads[0].Double("firstStartMs"), "When the first tick started");
        Expect.Equal(20.0, threads[0].Double("lastEndMs"), "When the last tick that ended ended; the third is still running");

        Expect.Equal(0L, summary.Long("regions"), "Regions");
        Expect.Equal(0, summary.Property("interestingRows").EnumerateObject().Count(), "Interesting rows");
        var firstFrame = summary.Property("firstFrame");
        Expect.Equal(1L, firstFrame.Long("frameNumber"), "The first frame");
        Expect.Equal("ProcessStart", firstFrame.Text("measuredFrom"), "What the first frame is measured from");
        Expect.True(firstFrame.Double("ms") > 4, $"The first frame is measured from the process's creation; it was {firstFrame.Double("ms")}.");

        var tools = ServerTools();
        foreach (var analysis in summary.Items("analyses"))
        {
            Expect.True(tools.Contains(analysis.Text("tool")!), $"The {analysis.Text("name")} analysis names a registered tool.");
        }

        Expect.True(Analysis(summary, "frames").Bool("available"), "The frames analysis has data.");
        var launch = Analysis(summary, "launch");
        Expect.True(launch.Bool("available"), "The launch analysis has data.");
        Expect.Equal("milestones", string.Join(",", launch.Strings("views")), "The launch analysis's views");
        var launchFrames = Analysis(summary, "launchFrames");
        Expect.True(launchFrames.Bool("available"), "The launch frames analysis has data.");
        Expect.Equal("launch", launchFrames.Text("tool"), "The launch frames analysis's tool");
        Expect.Equal("frames", string.Join(",", launchFrames.Strings("views")), "The launch frames analysis's views");
        Expect.True(Analysis(summary, "objects").Bool("available"), "The objects analysis has data.");
        var regions = Analysis(summary, "regions");
        Expect.True(!regions.Bool("available"), "The regions analysis has no data.");
        Expect.Equal(FrameTools.NoRegions, regions.Text("why"), "Why there are no regions");
        Expect.Equal("frames", regions.Text("tool"), "The regions analysis's tool");

        var empty = Write(Path.Combine(context.TempDirectory, "empty.sqlite"), []);
        var nothing = Run(() => SessionTools.Sessions(sessionPath: empty));
        Expect.Equal(0L, nothing.Long("ticks"), "Ticks in an empty session");
        Expect.True(nothing.IsNull("firstFrame"), "An empty session has no first frame.");
        Expect.Equal(FrameTools.NoTicks, Analysis(nothing, "frames").Text("why"), "Why an empty session has no frames");
        Expect.Equal(ObjectTools.NoObjects, Analysis(nothing, "objects").Text("why"), "Why an empty session has no objects");
        var noLaunch = Analysis(nothing, "launch");
        Expect.True(!noLaunch.Bool("available"), "An empty session has no launch.");
        Expect.Equal(LaunchTools.NoMilestones, noLaunch.Text("why"), "Why an empty session has no launch");
        var noLaunchFrames = Analysis(nothing, "launchFrames");
        Expect.True(!noLaunchFrames.Bool("available"), "An empty session has no launch frames.");
        Expect.Equal(FrameTools.NoTicks, noLaunchFrames.Text("why"), "Why an empty session has no launch frames");
    }

    private static void Notes(SelfTestContext context)
    {
        var lossy = Write(Path.Combine(context.TempDirectory, "lossy.sqlite"), MainEvents, lostByEtw: 3, droppedByProfiler: 2);
        var lossyNotes = Run(() => SessionTools.Sessions(sessionPath: lossy));
        HasNote(lossyNotes, "ETW lost 3 events while recording; results may miss some.");
        HasNote(lossyNotes, "The profiler dropped 2 events because its queue was full; results miss them.");
        Expect.Equal(3L, lossyNotes.Long("lostByEtw"), "Events ETW lost");
        var lostOne = Write(Path.Combine(context.TempDirectory, "lost-one.sqlite"), MainEvents, lostByEtw: 1, droppedByProfiler: 1);
        var lostOneNotes = Run(() => SessionTools.Sessions(sessionPath: lostOne));
        HasNote(lostOneNotes, "ETW lost 1 event while recording; results may miss it.");
        HasNote(lostOneNotes, "The profiler dropped 1 event because its queue was full; results miss it.");
        LacksNote(lossyNotes, NotReadToEnd);

        var trimmed = WriteMain(context, "trimmed.sqlite");
        SetCompleteFrom(trimmed, 5);
        var trimmedSummary = Run(() => SessionTools.Sessions(sessionPath: trimmed));
        Expect.Equal(5L, trimmedSummary.Long("trimmedBeforeSeq"), "Where the trimmed file's events begin");
        HasNote(trimmedSummary, "The oldest events, before Seq 5, were deleted to keep the file within");
        var trimmedLaunch = Analysis(trimmedSummary, "launch");
        Expect.True(!trimmedLaunch.Bool("available"), "A trimmed file's launch was deleted, though its ticks remain.");
        Expect.Equal(LaunchTools.LaunchDeleted, trimmedLaunch.Text("why"), "Why a trimmed file has no launch");
        var trimmedFrames = Analysis(trimmedSummary, "launchFrames");
        Expect.True(!trimmedFrames.Bool("available"), "A trimmed file has no launch frames.");
        Contains(trimmedFrames.Text("why"), "Why a trimmed file has no launch frames", "with the launch's first frames, were deleted");

        var etl = Path.Combine(context.TempDirectory, "app.etl");
        var trace = Write(Path.Combine(context.TempDirectory, "trace.sqlite"), MainEvents, "End of trace", SessionCoverage.Trace,
            lostByEtw: 4, etlPath: etl);
        var traceSummary = Run(() => SessionTools.Sessions(sessionPath: trace));
        Expect.Equal("Trace", traceSummary.Property("session").Text("coverage"), "A trace's coverage");
        Contains(traceSummary.Property("session").Text("timeBase"), "A trace's time base", "the first of the process's events");
        HasNote(traceSummary, $"Read from the trace {etl}.");
        HasNote(traceSummary, "The trace lost 4 events while it was recorded, of any process");
        LacksNote(traceSummary, NotReadToEnd);
        var stopped = Write(Path.Combine(context.TempDirectory, "stopped.sqlite"), MainEvents, "Stopped", SessionCoverage.Trace,
            etlPath: etl);
        HasNote(Run(() => SessionTools.Sessions(sessionPath: stopped)), "The trace wasn't read to its end (the session's end " +
            "reason is \"Stopped\"); results cover only the events read before then.");

        var connection = Write(Path.Combine(context.TempDirectory, "connection.sqlite"), MainEvents,
            coverage: SessionCoverage.Connection);
        HasNote(Run(() => SessionTools.Sessions(sessionPath: connection)),
            "The recording began when the profiler connected to the running process");
    }

    // A session file read from a trace is stamped with when it was read, so both views say so, and only for traces.
    private static void Times(SelfTestContext context)
    {
        var folder = Directory.CreateDirectory(Path.Combine(context.TempDirectory, "times")).FullName;
        var live = Write(Path.Combine(folder, "live.sqlite"), MainEvents);
        var trace = Write(Path.Combine(folder, "trace.sqlite"), MainEvents, "End of trace", SessionCoverage.Trace,
            etlPath: Path.Combine(context.TempDirectory, "app.etl"));
        var older = Write(Path.Combine(folder, "older.sqlite"), MainEvents);
        File.SetLastWriteTimeUtc(live, DateTime.UtcNow.AddMinutes(-1));
        File.SetLastWriteTimeUtc(trace, DateTime.UtcNow.AddMinutes(-2));
        File.SetLastWriteTimeUtc(older, DateTime.UtcNow.AddMinutes(-3));

        var traceSummary = Run(() => SessionTools.Sessions(sessionPath: trace));
        HasNote(traceSummary, SessionTools.TraceTimes);
        Expect.True(!traceSummary.IsNull("startedUtc") && !traceSummary.IsNull("endedUtc"), "A trace's summary keeps its times.");
        LacksNote(Run(() => SessionTools.Sessions(sessionPath: live)), SessionTools.TraceTimes);

        var all = Run(() => SessionTools.Sessions(folder: folder));
        Expect.Equal("live.sqlite,trace.sqlite,older.sqlite",
            string.Join(",", all.Items("sessions").Select(item => item.Text("file"))), "The files, newest first");
        Expect.Equal(SessionTools.ListTraceTimes, string.Join(" | ", all.Strings("notes")), "The notes on a list with a trace");
        var newest = Run(() => SessionTools.Sessions(folder: folder, limit: 1));
        Expect.Equal("Only the newest 1 of 3 session files are described; raise limit for more.",
            string.Join(" | ", newest.Strings("notes")), "The notes on a list that leaves the trace out");
    }

    // Session records with values WinUI Profiler doesn't write: the list view still lists each file, with its error, and
    // the summary view says what's wrong.
    private static void Damaged(SelfTestContext context)
    {
        var folder = Directory.CreateDirectory(Path.Combine(context.TempDirectory, "damaged")).FullName;
        var coverage = Damage(Path.Combine(folder, "coverage.sqlite"), "Coverage", "Hologram");
        var started = Damage(Path.Combine(folder, "started.sqlite"), "StartedUtc", "not a date");
        var process = Damage(Path.Combine(folder, "process.sqlite"), "ProcessId", 5_000_000_000L);

        var listed = Run(() => SessionTools.Sessions(folder: folder)).Items("sessions");
        Expect.Equal(3, listed.Count, "Damaged session files listed");
        foreach (var session in listed)
        {
            Expect.True(session.IsNull("processId") && !session.IsNull("error"), $"{session.Text("file")} is listed with an error.");
        }

        Contains(Error(() => SessionTools.Sessions(sessionPath: coverage)), "The error for a coverage this version doesn't know",
            $"{coverage} holds a value this version of the WinUI Profiler plugin doesn't know", "'Hologram'");
        Contains(Error(() => SessionTools.Sessions(sessionPath: started)), "The error for a start time that isn't a time",
            $"{started} holds a value that isn't in the form WinUI Profiler writes");
        Contains(Error(() => SessionTools.Sessions(sessionPath: process)), "The error for a process ID too large for one",
            $"{process} holds a value that isn't in the form WinUI Profiler writes");

        // A file whose launch view can't be read still gets a summary. It lists the statements that ran, not the one that
        // failed.
        var views = Write(Path.Combine(context.TempDirectory, "views.sqlite"), MainEvents);
        Update(views, "DROP VIEW LaunchSummary");
        var summary = Run(() => SessionTools.Sessions(sessionPath: views));
        HasNote(summary, "The file's frame views couldn't be read: ");
        Expect.True(summary.Items("uiThreads").Count == 0 && summary.IsNull("firstFrame"),
            "A summary without the frame views gives no UI threads or launch frames.");
        Contains(summary.Text("computed"), "What a summary without the frame views computed", "couldn't all be read");
        var ran = QuerySelfTests.CheckQueries(summary, "The summary of a file whose launch view is gone");
        Expect.Equal(3, ran.Count, "Statements of the session record, the ticks and the interesting rows");
        Expect.True(ran.All(query => !query.Text("sql")!.Contains("LaunchSummary", StringComparison.Ordinal)),
            "The statement that failed isn't listed.");
    }

    private static string Damage(string path, string column, object value)
    {
        Write(path, MainEvents);
        Expect.Equal(1, Update(path, $"UPDATE SessionInfo SET {column} = $value", value), $"Session records given a bad {column}");
        return path;
    }

    private static void Recording(SelfTestContext context)
    {
        var folder = Directory.CreateDirectory(Path.Combine(context.TempDirectory, "recording")).FullName;
        var path = Path.Combine(folder, "live.sqlite");
        using var recording = new FixtureRecording(path);
        recording.Add(MainEvents);
        recording.WaitUntilStored();

        var summary = Run(() => SessionTools.Sessions(sessionPath: path));
        Expect.Equal(SessionCatalog.NoEndReason, summary.Property("session").Text("endReason"), "A recording's end reason");
        Expect.True(summary.IsNull("endedUtc"), "A recording hasn't ended.");
        Expect.Equal(16L, summary.Long("storedEvents"), "Stored events while recording");
        HasNote(summary, "The session has no end reason: it's still being recorded");
        var listed = Run(() => SessionTools.Sessions(folder: folder)).Items("sessions");
        Expect.Equal(1, listed.Count, "Session files while recording");
        Expect.Equal(SessionCatalog.NoEndReason, listed[0].Text("endReason"), "The listed end reason while recording");
        Expect.Equal(path, SessionCatalog.Resolve(null, folder), "The newest session while recording");
        Expect.True(Run(() => FrameTools.Frames(sessionPath: path)).Items("uiThreads").Count == 1,
            "The frames tool reads a session that's still being recorded.");

        recording.Close(EndReason);
        Expect.Equal(EndReason, Run(() => SessionTools.Sessions(sessionPath: path)).Property("session").Text("endReason"),
            "The end reason once the recording ended");
    }

    // A newer version can store a provider or object kind this version doesn't have. The tools that read it say so, and
    // the list view still lists the file.
    private static void Newer(SelfTestContext context)
    {
        const string Unknown = "holds a value this version of the WinUI Profiler plugin doesn't know";
        var folder = Directory.CreateDirectory(Path.Combine(context.TempDirectory, "newer")).FullName;
        var provider = Write(Path.Combine(folder, "provider.sqlite"), MainEvents);
        Expect.True(Update(provider, "UPDATE EventTypes SET Provider = $value WHERE Name = 'DependencyObject_Enter'", "Gizmo") > 0,
            "Event types given a newer provider");
        var tick = Run(() => FrameTools.Frames(view: "slowest", sessionPath: provider)).Items("ticks")
            .First(item => item.Long("frameNumber") == 1).Long("tickId");
        Contains(Error(() => FrameTools.Frames(tickId: tick, sessionPath: provider)), "The tick view on a newer provider", Unknown);
        Contains(Error(() => ObjectTools.Objects(address: "0x1000", sessionPath: provider)),
            "The history view on a newer provider", Unknown);

        var kind = Write(Path.Combine(folder, "kind.sqlite"), MainEvents);
        Expect.Equal(1, Update(kind, "UPDATE Objects SET Kind = $value WHERE ObjectId = (SELECT MIN(ObjectId) FROM Objects)", "Gadget"),
            "Objects given a newer kind");
        Contains(Error(() => ObjectTools.Objects(sessionPath: kind)), "The types view on a newer object kind", Unknown);

        var listed = Run(() => SessionTools.Sessions(folder: folder)).Items("sessions");
        Expect.Equal(2, listed.Count, "Newer versions' session files listed");
        Expect.True(listed.All(session => session.Long("storedEvents") == 16), "Newer versions' session files are listed with their events.");
    }

    private static HashSet<string> ServerTools() =>
    [
        .. typeof(SessionTools).Assembly.GetTypes()
            .SelectMany(type => type.GetMethods())
            .Select(method => method.GetCustomAttributes(typeof(ModelContextProtocol.Server.McpServerToolAttribute), false)
                .Cast<ModelContextProtocol.Server.McpServerToolAttribute>().FirstOrDefault()?.Name)
            .OfType<string>(),
    ];

    internal static JsonElement Analysis(JsonElement summary, string name) =>
        summary.Items("analyses").FirstOrDefault(item => item.Text("name") == name) is { ValueKind: JsonValueKind.Object } analysis
            ? analysis
            : throw new SelfTestFailedException($"The summary lists the {name} analysis.");

    /// <summary>Marks a session file as trimmed, as SessionWriter does when it deletes the oldest events.</summary>
    private static void SetCompleteFrom(string path, long seq) =>
        Expect.Equal(1, Update(path, "UPDATE SessionInfo SET CompleteFromSeq = $value", seq), "Session records marked as trimmed");

    internal static int Update(string path, string sql, object? value = null)
    {
        using var connection = new SqliteConnection(new SqliteConnectionStringBuilder
        {
            DataSource = path,
            Mode = SqliteOpenMode.ReadWrite,
            Pooling = false,
        }.ToString());
        connection.Open();
        using var command = connection.CreateCommand();
        command.CommandText = sql;
        if (value is not null)
        {
            command.Parameters.AddWithValue("$value", value);
        }

        return command.ExecuteNonQuery();
    }
}
