// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using WinUIProfiler.Engine.Storage;
using WinUIProfiler.Mcp.Tools;
using static WinUIProfiler.Mcp.SelfTest.SessionFixtures;
using static WinUIProfiler.Mcp.SelfTest.ToolResults;

namespace WinUIProfiler.Mcp.SelfTest;

/// <summary>record_trace's argument checks, and its reuse of a session file already read from the same trace.</summary>
internal static class RecordTraceSelfTests
{
    private const string EndOfTrace = "End of trace";
    private const string SessionName = "20261104-120000-FixtureApp-4242.sqlite";

    // The full path of a real trace to read end to end; without it, record_trace.real is skipped.
    private const string TraceVariable = "WINUIPROFILER_SELFTEST_ETL";

    public static IEnumerable<SelfTestCase> Tests =>
    [
        new("record_trace.errors", Errors),
        new("record_trace.reuse", Reuse),
        new("record_trace.real", Real),
    ];

    private static void Errors(SelfTestContext context)
    {
        var folder = Path.GetFullPath(context.TempDirectory);
        var trace = FakeTrace(folder);
        var sessions = Path.Combine(folder, "sessions");

        Expect.Equal("etlPath is required: the full path of a trace (.etl).", Error(() => Record(" ", null, sessions)), "No path");
        Expect.Equal("'app.etl' isn't a full path. Give the trace's full path, such as C:\\traces\\app.etl.",
            Error(() => Record("app.etl", null, sessions)), "A relative path");
        Expect.Equal($"{folder} is a folder. Give the full path of a trace (.etl).",
            Error(() => Record(folder, null, sessions)), "A folder");
        var missing = Path.Combine(folder, "missing.etl");
        Expect.Equal($"The trace {missing} was not found.", Error(() => Record(missing, null, sessions)), "A missing trace");
        var session = WriteMain(context);
        Expect.Equal($"{session} is a session file already. Pass it as sessionPath to the other tools.",
            Error(() => Record(session, null, sessions)), "A session file");
        Expect.Equal("processId can't be negative; it was -1.", Error(() => Record(trace, -1, sessions)), "A negative processId");
        Expect.Equal($"{trace} is a file, not a folder.", Error(() => Record(trace, ProcessId, trace)), "A folder that is a file");

        // The fake trace is text, so reading it fails, with a process named or not.
        Contains(Error(() => Record(trace, null, sessions)), "The error for a file that isn't a trace", trace);
        Contains(Error(() => Record(trace, ProcessId, sessions, reread: true)), "The error for a file that isn't a trace, read again",
            trace);
    }

    private static void Reuse(SelfTestContext context)
    {
        var folder = Path.GetFullPath(context.TempDirectory);
        var trace = FakeTrace(folder);
        var sessions = Directory.CreateDirectory(Path.Combine(folder, "sessions")).FullName;
        var session = WriteTraceSession(sessions, SessionName, trace);

        Expect.Equal(session, SessionTools.FindFinishedSession(sessions, trace, ProcessId), "The session read from the trace");
        Expect.Equal(session, SessionTools.FindFinishedSession(sessions, trace.ToUpperInvariant(), ProcessId),
            "The session, with the trace's path in other case");
        Expect.True(SessionTools.FindFinishedSession(sessions, trace, ProcessId + 1) is null, "No session of another process");

        Expect.True(FindAlone(folder, "stopped", trace, endReason: "Stopped") is null, "No session stopped before the trace's end");
        Expect.True(FindAlone(folder, "unnamed", trace, fileName: "20261104-120000-FixtureApp.sqlite") is null,
            "No session whose name doesn't have the process's ID");
        Expect.True(FindAlone(folder, "misnamed", trace, fileName: "20261104-120000-FixtureApp-4243.sqlite",
            processId: ProcessId + 1) is null, "No session named for one process that holds another");
        Expect.True(FindAlone(folder, "live", trace, coverage: SessionCoverage.ProcessStart) is null,
            "No session recorded live, even with the trace's path");
        Expect.True(FindAlone(folder, "other", trace, etlPath: Path.Combine(folder, "other.etl")) is null,
            "No session read from another trace");

        File.SetLastWriteTimeUtc(trace, DateTime.UtcNow.AddHours(1));
        Expect.True(SessionTools.FindFinishedSession(sessions, trace, ProcessId) is null,
            "No session written before the trace last changed");
        File.SetLastWriteTimeUtc(trace, DateTime.UtcNow.AddMinutes(-1));

        File.WriteAllBytes(session + "-wal", []);
        Expect.True(SessionTools.FindFinishedSession(sessions, trace, ProcessId) is null, "No session with a write-ahead log");
        File.Delete(session + "-wal");

        var result = Run(() => Record(trace, ProcessId, sessions));
        Expect.True(result.Bool("reused"), "The session was reused.");
        Expect.Equal(session, result.Property("session").Text("sessionPath"), "The reused session");
        Expect.Equal("Finished", result.Text("state"), "Its state");
        Expect.Equal((long)MainEvents.Count, result.Long("storedEvents"), "Its events");
        Expect.Equal($"{session} already holds this process's events from {trace}, read to the trace's end, so it was reused. " +
            "Pass reread=true to read the trace again into a new session file.", result.Text("message"), "The message");
    }

    /// <summary>Reads a real trace into a session file, then reuses that file, then reads the trace again.</summary>
    private static void Real(SelfTestContext context)
    {
        var variable = Environment.GetEnvironmentVariable(TraceVariable);
        var trace = string.IsNullOrWhiteSpace(variable)
            ? throw Expect.Skip($"Set {TraceVariable} to the full path of a trace with XAML events to read one end to end.")
            : variable;
        var sessions = Path.Combine(context.TempDirectory, "sessions");

        var first = Run(() => Record(trace, null, sessions));
        if (!first.IsNull("needsProcessId"))
        {
            int? listed = (int)first.Items("processes")[0].Long("processId");
            first = Run(() => Record(trace, listed, sessions));
        }

        Expect.True(!first.Bool("reused"), "The first read makes a session file.");
        Expect.Equal("Finished", first.Text("state"), "The first read");
        var session = first.Property("session");
        var path = session.Text("sessionPath");
        Expect.True(File.Exists(path), $"The session file {path} exists.");
        Expect.Equal("Trace", session.Text("coverage"), "Its coverage");
        Expect.Equal(EndOfTrace, session.Text("endReason"), "Its end reason");
        var read = first.Long("xamlEvents") + first.Long("profilerEvents") + first.Long("gpuMemoryEvents");
        Expect.True(read > 0, "Events were read.");
        Expect.Equal(read, first.Long("storedEvents"), "Stored events, which are all the events read");

        int? processId = (int)session.Long("processId");
        var again = Run(() => Record(trace, processId, sessions));
        Expect.True(again.Bool("reused"), "Reading the same process again reuses its session file.");
        Expect.Equal(path, again.Property("session").Text("sessionPath"), "The reused session");

        var reread = Run(() => Record(trace, processId, sessions, reread: true));
        Expect.True(!reread.Bool("reused"), "reread=true reads the trace again.");
        Expect.True(reread.Property("session").Text("sessionPath") != path, "Reading again makes a new session file.");
        Expect.Equal(read, reread.Long("storedEvents"), "The events read again");
    }

    private static Task<object> Record(string etlPath, int? processId, string? folder, bool reread = false) =>
        SessionTools.RecordTraceCore(etlPath, processId, folder, reread, null, null, CancellationToken.None);

    /// <summary>A text file named as a trace, last written a minute ago, so session files written now are newer.</summary>
    private static string FakeTrace(string folder)
    {
        var trace = Path.Combine(folder, "app.etl");
        File.WriteAllText(trace, "This is not a trace.");
        File.SetLastWriteTimeUtc(trace, DateTime.UtcNow.AddMinutes(-1));
        return trace;
    }

    /// <summary>A session file of the fixture process read from a trace to its end, as record_trace and the app write one.</summary>
    private static string WriteTraceSession(string folder, string fileName, string trace, string endReason = EndOfTrace,
        SessionCoverage coverage = SessionCoverage.Trace, string? etlPath = null) =>
        Write(Path.Combine(folder, fileName), MainEvents, endReason, coverage, etlPath: etlPath ?? trace);

    /// <summary>Writes one session file into a folder of its own, then looks there for a finished session of the trace.</summary>
    private static string? FindAlone(string folder, string name, string trace, string endReason = EndOfTrace,
        SessionCoverage coverage = SessionCoverage.Trace, string? etlPath = null, string fileName = SessionName,
        int processId = ProcessId)
    {
        var directory = Directory.CreateDirectory(Path.Combine(folder, name)).FullName;
        WriteTraceSession(directory, fileName, trace, endReason, coverage, etlPath);
        return SessionTools.FindFinishedSession(directory, trace, processId);
    }
}
