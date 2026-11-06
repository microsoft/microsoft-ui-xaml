// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.Data.Sqlite;
using ModelContextProtocol;
using WinUIProfiler.Engine.Events;
using WinUIProfiler.Engine.Ingestion;
using WinUIProfiler.Engine.Storage;
using WinUIProfiler.Mcp.Sessions;
using WinUIProfiler.Mcp.Tools;
using static WinUIProfiler.Mcp.SelfTest.SessionFixtures;
using static WinUIProfiler.Mcp.SelfTest.ToolResults;

namespace WinUIProfiler.Mcp.SelfTest;

/// <summary>
/// record_trace's argument checks, and its reuse of a session file it already read from the same, unchanged trace.
/// </summary>
internal static class RecordTraceSelfTests
{
    private const string SessionName = "20261104-120000-FixtureApp-4242.sqlite";

    private const string FakeTraceText = "This is not a trace.";

    // The full path of a real trace to read end to end; without it, record_trace.real is skipped.
    private const string TraceVariable = "WINUIPROFILER_SELFTEST_ETL";

    public static IEnumerable<SelfTestCase> Tests =>
    [
        new("record_trace.errors", Errors),
        new("record_trace.reuse", Reuse),
        new("record_trace.real", Real),
        new("record_trace.cancel", Cancel),
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
        Expect.True(FindAlone(folder, "unrecorded", trace, keep: false) is null,
            "No session without a record of the trace's version, as the app writes them");

        File.SetLastWriteTimeUtc(trace, DateTime.UtcNow.AddHours(1));
        Expect.True(SessionTools.FindFinishedSession(sessions, trace, ProcessId) is null,
            "No session written before the trace last changed");
        File.SetLastWriteTimeUtc(trace, DateTime.UtcNow.AddMinutes(-1));

        // A trace copied over the one that was read keeps its own time, which can be older than the session file.
        ReplaceTrace(trace, "This is not a trace!");
        Expect.True(SessionTools.FindFinishedSession(sessions, trace, ProcessId) is null,
            "No session once another trace of the same size, and an older time, replaced the one read");
        ReplaceTrace(trace, "This isn't a trace either, and it's longer.");
        Expect.True(SessionTools.FindFinishedSession(sessions, trace, ProcessId) is null,
            "No session once a trace of another size, and an older time, replaced the one read");
        ReplaceTrace(trace, FakeTraceText);
        Expect.Equal(session, SessionTools.FindFinishedSession(sessions, trace, ProcessId),
            "The session, once the trace it was read from is back");

        var record = session + TraceVersion.RecordSuffix;
        var kept = File.ReadAllText(record);
        File.WriteAllText(record, "{");
        Expect.True(SessionTools.FindFinishedSession(sessions, trace, ProcessId) is null, "No session whose record can't be read");
        File.WriteAllText(record, kept);

        // Deleting a session file alone leaves its record, which a later session file of the same name doesn't take.
        var renamed = Directory.CreateDirectory(Path.Combine(folder, "renamed")).FullName;
        File.Delete(WriteTraceSession(renamed, SessionName, trace));
        var later = WriteTraceSession(renamed, SessionName, trace, keep: false);
        Expect.True(SessionTools.FindFinishedSession(renamed, trace, ProcessId) is null,
            "No session whose record is of an earlier session file of its name");
        TraceVersion.Read(trace)!.Keep(later, SessionFile.ReadInfo(later), trace);
        Expect.Equal(later, SessionTools.FindFinishedSession(renamed, trace, ProcessId),
            "The later session file, once its own record is kept");

        File.WriteAllBytes(session + "-wal", []);
        Expect.True(SessionTools.FindFinishedSession(sessions, trace, ProcessId) is null, "No session with a write-ahead log");
        File.Delete(session + "-wal");

        // A newer session file of the process that can't be read is passed over.
        var damaged = Directory.CreateDirectory(Path.Combine(folder, "damaged")).FullName;
        var good = WriteTraceSession(damaged, "20261104-110000-FixtureApp-4242.sqlite", trace);
        var bad = WriteTraceSession(damaged, SessionName, trace);
        Expect.Equal(1, SessionToolSelfTests.Update(bad, "UPDATE SessionInfo SET StartedUtc = $value", "not a date"),
            "Session records given a start time that isn't a time");
        Expect.True(!File.Exists(bad + "-wal"), "The damaged session file has no write-ahead log.");
        File.SetLastWriteTimeUtc(good, DateTime.UtcNow.AddSeconds(-20));
        File.SetLastWriteTimeUtc(bad, DateTime.UtcNow.AddSeconds(-10));
        Expect.Equal(good, SessionTools.FindFinishedSession(damaged, trace, ProcessId), "The session after a damaged one");

        var result = Run(() => Record(trace, ProcessId, sessions));
        Expect.True(result.Bool("reused"), "The session was reused.");
        Expect.Equal(session, result.Property("session").Text("sessionPath"), "The reused session");
        Expect.Equal("Finished", result.Text("state"), "Its state");
        Expect.Equal((long)MainEvents.Count, result.Long("storedEvents"), "Its events");
        Expect.Equal($"{session} already holds this process's events from {trace}, read to the trace's end, and the trace " +
            "hasn't changed since, so it was reused. Pass reread=true to read the trace again into a new session file.",
            result.Text("message"), "The message");
    }

    /// <summary>Reads a real trace into a session file, then reuses that file, then reads the trace again.</summary>
    private static void Real(SelfTestContext context)
    {
        var trace = RealTrace();
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
        Expect.Equal(SessionCatalog.EndOfTrace, session.Text("endReason"), "Its end reason");
        var read = first.Long("xamlEvents") + first.Long("profilerEvents") + first.Long("gpuMemoryEvents");
        Expect.True(read > 0, "Events were read.");
        Expect.Equal(read + FocusEvents(path!), first.Long("storedEvents"),
            "Stored events: those read, and the process's window focus events, which aren't counted as read");
        Expect.True(File.Exists(path + TraceVersion.RecordSuffix), "The trace's version is recorded beside the session file.");

        int? processId = (int)session.Long("processId");
        var again = Run(() => Record(trace, processId, sessions));
        Expect.True(again.Bool("reused"), "Reading the same process again reuses its session file.");
        Expect.Equal(path, again.Property("session").Text("sessionPath"), "The reused session");

        var reread = Run(() => Record(trace, processId, sessions, reread: true));
        Expect.True(!reread.Bool("reused"), "reread=true reads the trace again.");
        Expect.True(reread.Property("session").Text("sessionPath") != path, "Reading again makes a new session file.");
        Expect.Equal(first.Long("storedEvents"), reread.Long("storedEvents"), "The events stored again");
    }

    /// <summary>The window focus events (Win32k) a session file holds.</summary>
    private static long FocusEvents(string path)
    {
        using var connection = new SqliteConnection(new SqliteConnectionStringBuilder
        {
            DataSource = path,
            Mode = SqliteOpenMode.ReadOnly,
            Pooling = false,
        }.ToString());
        connection.Open();
        using var command = connection.CreateCommand();
        command.CommandText = "SELECT COUNT(*) FROM Events JOIN EventTypes USING (TypeId) WHERE Provider = $provider";
        command.Parameters.AddWithValue("$provider", nameof(XamlProvider.Win32k));
        return (long)command.ExecuteScalar()!;
    }

    /// <summary>A call cancelled while it reads a real trace deletes the session file it was writing.</summary>
    private static void Cancel(SelfTestContext context)
    {
        var trace = RealTrace();
        var processId = OfflineEtlSource.ListProcesses(trace).Processes.MaxBy(process => process.Events)?.ProcessId
            ?? throw new SelfTestFailedException($"No process in {trace} has XAML events.");
        var folder = Directory.CreateDirectory(Path.Combine(context.TempDirectory, "cancelled")).FullName;
        using var cancellation = new CancellationTokenSource();
        var progress = new CancelOnReport(cancellation);
        try
        {
            SessionTools.RecordTraceCore(trace, processId, folder, true, null, progress, cancellation.Token)
                .GetAwaiter().GetResult();
        }
        catch (OperationCanceledException)
        {
            Expect.Equal("", string.Join(", ", Directory.GetFiles(folder).Select(Path.GetFileName)),
                "Files left in the folder after the call was cancelled");
            return;
        }
        finally
        {
            progress.Cancelling.GetAwaiter().GetResult();
        }

        throw Expect.Skip($"{trace} was read before the first progress report, so the read couldn't be cancelled part way.");
    }

    /// <summary>
    /// Cancels a call the first time it reports progress. It cancels on another thread: the call reports from the reader's
    /// timer, and stopping the read waits for that timer.
    /// </summary>
    private sealed class CancelOnReport(CancellationTokenSource cancellation) : IProgress<ProgressNotificationValue>
    {
        private Task? cancelling;

        public Task Cancelling => Volatile.Read(ref cancelling) ?? Task.CompletedTask;

        public void Report(ProgressNotificationValue value)
        {
            var task = new Task(cancellation.Cancel);
            if (Interlocked.CompareExchange(ref cancelling, task, null) is null)
            {
                task.Start(TaskScheduler.Default);
            }
        }
    }

    /// <summary>The trace the variable names; skips the test without one.</summary>
    private static string RealTrace()
    {
        var variable = Environment.GetEnvironmentVariable(TraceVariable);
        return string.IsNullOrWhiteSpace(variable)
            ? throw Expect.Skip($"Set {TraceVariable} to the full path of a trace with XAML events to read one end to end.")
            : variable;
    }

    private static Task<object> Record(string etlPath, int? processId, string? folder, bool reread = false) =>
        SessionTools.RecordTraceCore(etlPath, processId, folder, reread, null, null, CancellationToken.None);

    /// <summary>A text file named as a trace, last written a minute ago, so session files written now are newer.</summary>
    private static string FakeTrace(string folder)
    {
        var trace = Path.Combine(folder, "app.etl");
        ReplaceTrace(trace, FakeTraceText);
        return trace;
    }

    /// <summary>Writes the fake trace again, last written a minute ago, as a trace copied over it keeps its older time.</summary>
    private static void ReplaceTrace(string trace, string text)
    {
        File.WriteAllText(trace, text);
        File.SetLastWriteTimeUtc(trace, DateTime.UtcNow.AddMinutes(-1));
    }

    /// <summary>
    /// A session file of the fixture process read from a trace to its end, as record_trace and the app write one. Unless
    /// keep is false, it has record_trace's record of the trace's version beside it; the app writes none.
    /// </summary>
    private static string WriteTraceSession(string folder, string fileName, string trace, string endReason = SessionCatalog.EndOfTrace,
        SessionCoverage coverage = SessionCoverage.Trace, string? etlPath = null, bool keep = true)
    {
        var path = Write(Path.Combine(folder, fileName), MainEvents, endReason, coverage, etlPath: etlPath ?? trace);
        if (keep)
        {
            var version = TraceVersion.Read(trace) ?? throw new SelfTestFailedException($"{trace} couldn't be read.");
            version.Keep(path, SessionFile.ReadInfo(path), etlPath ?? trace);
        }

        return path;
    }

    /// <summary>Writes one session file into a folder of its own, then looks there for a finished session of the trace.</summary>
    private static string? FindAlone(string folder, string name, string trace, string endReason = SessionCatalog.EndOfTrace,
        SessionCoverage coverage = SessionCoverage.Trace, string? etlPath = null, string fileName = SessionName,
        int processId = ProcessId, bool keep = true)
    {
        var directory = Directory.CreateDirectory(Path.Combine(folder, name)).FullName;
        WriteTraceSession(directory, fileName, trace, endReason, coverage, etlPath, keep);
        return SessionTools.FindFinishedSession(directory, trace, processId);
    }
}
