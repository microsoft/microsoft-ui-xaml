// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Diagnostics;
using System.Globalization;
using Microsoft.Data.Sqlite;
using WinUIProfiler.Engine.Events;
using WinUIProfiler.Engine.Storage;

namespace WinUIProfiler.Mcp.SelfTest;

/// <summary>
/// An event a fixture writes, at a time in milliseconds after the recording started. Marker is the provider of a marker
/// event (<see cref="XamlProvider.Marker"/>), and null for the others.
/// </summary>
internal sealed record FixtureEvent(double Milliseconds, int ThreadId, XamlProvider Provider, bool TraceLogging, string Name,
    int Opcode, IReadOnlyList<EventField> Fields, MarkerProvider? Marker = null);

/// <summary>
/// Session files for the self-tests. The app's own SessionWriter writes them, so its indexes build the frames and objects
/// as they do while the app records.
/// </summary>
internal static class SessionFixtures
{
    internal const int ProcessId = 4242;
    internal const string ProcessName = "FixtureApp";
    internal const int UiThread = 100;
    internal const int DiagnosticsThread = 200;
    internal const int KernelThread = 300;
    internal const string GridType = "Microsoft.UI.Xaml.Controls.Grid";
    internal const string ButtonType = "Microsoft.UI.Xaml.Controls.Button";
    internal const string EndReason = "Process exited";

    /// <summary>
    /// The main fixture, in Seq order: frame 1 (Seq 1-7) with two elements and a visual; the first UI-thread frame's end
    /// (Seq 8, held until frame 2 starts); a GPU memory row and two Diagnostics events on other threads (Seq 9-11, in no
    /// frame); frame 2 (Seq 12-15) with a layout pass; and frame 3, still open (Seq 16).
    /// </summary>
    internal static IReadOnlyList<FixtureEvent> MainEvents { get; } =
    [
        FrameStart(1.0, 1),
        Enter(1.5, 0x1000, "Root", GridType, 100),
        EnteredTree(1.6, 0x1000, 0, "Root", 0x7000, 300, "", 0, 0),
        Enter(2.0, 0x2000, "OkButton", ButtonType, 101),
        EnteredTree(2.1, 0x2000, 0x1000, "OkButton", 0x7100, 400, "ms-appx:///MainWindow.xaml", 12, 5),
        VisualInserted(3.0, 0xA000, 0xB000),
        FrameEnd(4.0, 1),
        Tl(5.0, UiThread, XamlProvider.MicrosoftUIXaml, "FirstUiThreadFrameEnd"),
        Manifest(6.0, KernelThread, XamlProvider.DxgKrnl, "VidMmProcessUsageChange", 0, F("NewUsage", 1048576L),
            F("OldUsage", 0L), F("pDxgAdapter", 0xD000UL), F("ProcessId", ProcessId), F("PhysicalAdapterIndex", 0u),
            F("MemorySegmentGroup", (byte)0)),
        PropertyChanged(8.0),
        PropertyChanged(9.0),
        FrameStart(16.0, 2),
        Manifest(17.0, UiThread, XamlProvider.Xaml, "Layout/Start", 1),
        Manifest(18.0, UiThread, XamlProvider.Xaml, "Layout/Stop", 2),
        FrameEnd(20.0, 2),
        FrameStart(30.0, 3),
    ];

    internal static EventField F(string name, object? value) => new(name, value);

    /// <summary>A TraceLogging event, which describes its own fields.</summary>
    internal static FixtureEvent Tl(double ms, int thread, XamlProvider provider, string name, params EventField[] fields) =>
        new(ms, thread, provider, true, name, 0, fields);

    /// <summary>An event a provider manifest describes.</summary>
    internal static FixtureEvent Manifest(double ms, int thread, XamlProvider provider, string name, int opcode,
        params EventField[] fields) => new(ms, thread, provider, false, name, opcode, fields);

    /// <summary>A TraceLogging event of a marker provider, which the marker configuration names.</summary>
    internal static FixtureEvent MarkerEvent(double ms, int thread, MarkerProvider provider, string name,
        params EventField[] fields) => new(ms, thread, XamlProvider.Marker, true, name, 0, fields, provider);

    internal static FixtureEvent FrameStart(double ms, uint frame) =>
        Tl(ms, UiThread, XamlProvider.Xaml, "CoreServices_Frame", F("IsStart", true), F("FrameNumber", frame));

    internal static FixtureEvent FrameEnd(double ms, uint frame) =>
        Tl(ms, UiThread, XamlProvider.Xaml, "CoreServices_Frame", F("IsStart", false), F("FrameNumber", frame));

    /// <summary>An object entering, with its type and name, as Microsoft-Windows-XAML writes it.</summary>
    internal static FixtureEvent Enter(double ms, long address, string name, string type, int typeIndex) =>
        Tl(ms, UiThread, XamlProvider.Xaml, "DependencyObject_Enter", F("IsStart", true), F("FrameNumber", 1u),
            F("ObjectPointer", address), F("Name", name), F("Type", type), F("TypeIndex", typeIndex));

    /// <summary>An element entering the live tree, as the XAML-Profiler provider writes it.</summary>
    internal static FixtureEvent EnteredTree(double ms, long element, long parent, string label, long peer, long coreSize,
        string sourceUri, int line, int column) =>
        Tl(ms, UiThread, XamlProvider.XamlProfiler, "ElementEnteredTree", F("ElementId", element), F("ParentId", parent),
            F("IsLive", true), F("Label", label), F("IsTemplateChild", false), F("PeerHandle", peer), F("CoreSize", coreSize),
            F("SourceFileUri", sourceUri), F("SourceLine", line), F("SourceColumn", column));

    internal static FixtureEvent LeftTree(double ms, long element, long parent) =>
        Tl(ms, UiThread, XamlProvider.XamlProfiler, "ElementLeftTree", F("ElementId", element), F("ParentId", parent));

    /// <summary>A ContainerVisual inserted into a parent visual: the event gives the child its type, but not the parent.</summary>
    internal static FixtureEvent VisualInserted(double ms, long parent, long child) =>
        Tl(ms, UiThread, XamlProvider.XamlProfiler, "WucVisualChildInserted", F("ParentVisualId", parent),
            F("ChildVisualId", child), F("OwnerCompNodeId", 0xC000L), F("Index", 0), F("VisualType", "ContainerVisual"),
            F("Properties", "Size=0,0"));

    /// <summary>A Diagnostics manifest event that names OkButton and Root, which doesn't make them objects.</summary>
    internal static FixtureEvent PropertyChanged(double ms) =>
        Manifest(ms, DiagnosticsThread, XamlProvider.XamlDiagnostics, "PropertyChanged", 0, F("ElementId", 0x2000L),
            F("ParentId", 0x1000L));

    /// <summary>
    /// Writes a finished session file of the fixture process. For a trace, processStartMs is when the process was created,
    /// in ms from the start of the recording, as the kernel's process start event in a trace gives it; null when the trace
    /// has no such event.
    /// </summary>
    internal static string Write(string path, IEnumerable<FixtureEvent> events, string endReason = EndReason,
        SessionCoverage coverage = SessionCoverage.ProcessStart, int? lostByEtw = null, long? droppedByProfiler = null,
        string? etlPath = null, double? processStartMs = null)
    {
        using var recording = new FixtureRecording(path, coverage, etlPath, processStartMs);
        recording.Add(events);
        recording.Close(endReason, lostByEtw, droppedByProfiler);
        return path;
    }

    /// <summary>The main fixture's session file, in a test's folder.</summary>
    internal static string WriteMain(SelfTestContext context, string name = "main.sqlite") =>
        Write(Path.Combine(context.TempDirectory, name), MainEvents);

    /// <summary>An SQLite file that isn't a session file: another user_version, and a table of its own.</summary>
    internal static string WriteForeign(string path)
    {
        var connectionString = new SqliteConnectionStringBuilder
        {
            DataSource = path,
            Mode = SqliteOpenMode.ReadWriteCreate,
            Pooling = false,
        }.ToString();
        using var connection = new SqliteConnection(connectionString);
        connection.Open();
        using var command = connection.CreateCommand();
        command.CommandText = "CREATE TABLE Notes (Text TEXT); PRAGMA user_version = 7;";
        command.ExecuteNonQuery();
        return path;
    }

    /// <summary>A file with a session file's extension that isn't an SQLite database.</summary>
    internal static string WriteGarbage(string path)
    {
        File.WriteAllText(path, string.Concat(Enumerable.Repeat("This is not a database. ", 180)));
        return path;
    }
}

/// <summary>A session file being written, with the clock its events' times count from.</summary>
internal sealed class FixtureRecording : IDisposable
{
    private static readonly TimeSpan StoreTimeout = TimeSpan.FromSeconds(15);

    private readonly long startQpc;
    private long sequence;

    internal FixtureRecording(string path, SessionCoverage coverage = SessionCoverage.ProcessStart, string? etlPath = null,
        double? processStartMs = null)
    {
        // A recording from process start knows when the process was created: here, just before the recording started.
        DateTime? created = coverage == SessionCoverage.Trace ? null : DateTime.UtcNow.AddMilliseconds(-50);
        startQpc = Stopwatch.GetTimestamp();
        Writer = SessionWriter.Create(path, new SessionStart(SessionFixtures.ProcessId, SessionFixtures.ProcessName, created,
            Stopwatch.Frequency, startQpc, coverage, etlPath));
        if (processStartMs is { } ms)
        {
            // As a trace's reader stores it, with the process that created this one.
            Writer.SetProcessStart(new TraceProcessStart(Qpc(ms), SessionFixtures.ProcessName, 4100, "explorer"));
        }
    }

    internal SessionWriter Writer { get; }

    internal long Added => sequence;

    internal void Add(IEnumerable<FixtureEvent> events)
    {
        foreach (var item in events)
        {
            Writer.Process(++sequence, new ProfilerEvent(Qpc(item.Milliseconds), item.Milliseconds, SessionFixtures.ProcessId,
                item.ThreadId, item.Provider, item.TraceLogging, item.Name, item.Opcode, item.Fields, item.Marker));
        }
    }

    /// <summary>Waits until a reader of the file sees every event added so far.</summary>
    internal void WaitUntilStored()
    {
        var waited = Stopwatch.StartNew();
        while (true)
        {
            Expect.True(Writer.Failure is null, $"The fixture {Writer.Path} is being written: {Writer.Failure}");
            try
            {
                if (SessionFile.ReadInfo(Writer.Path).StoredEvents == sequence)
                {
                    return;
                }
            }
            catch (SqliteException) when (waited.Elapsed < StoreTimeout)
            {
                // The writer holds the file for a moment; read it again.
            }

            Expect.True(waited.Elapsed < StoreTimeout, string.Create(CultureInfo.InvariantCulture,
                $"A reader of {Writer.Path} saw all {sequence} events within {StoreTimeout.TotalSeconds} s."));
            Thread.Sleep(25);
        }
    }

    internal void Close(string endReason, int? lostByEtw = null, long? droppedByProfiler = null)
    {
        Writer.Close(endReason, lostByEtw, droppedByProfiler);
        Expect.True(Writer.Failure is null, $"The fixture {Writer.Path} was written: {Writer.Failure}");
    }

    public void Dispose() => Writer.Dispose();

    // On the events' clock: QPC ticks from the start of the recording.
    private long Qpc(double ms) => startQpc + (long)Math.Round(ms * Stopwatch.Frequency / 1000.0);
}
