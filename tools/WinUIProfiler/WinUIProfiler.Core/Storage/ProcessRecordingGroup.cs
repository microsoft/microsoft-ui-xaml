using System.Diagnostics;
using WinUIProfiler.Engine.Events;
using WinUIProfiler.Engine.Ingestion;
using WinUIProfiler.Engine.Processing;

namespace WinUIProfiler.Engine.Storage;

internal sealed record ProcessRecordingStatus(SessionStart Source, bool IsPrimary, RecordingStatus? Recording,
    long XamlEvents, long ProfilerEvents, long GpuMemoryEvents, long DroppedEvents, string? Error);

// Each writer indexes process-local addresses and frames. Never feed another target process into that writer,
// where it would otherwise be mistaken for an external launcher.
internal sealed class ProcessRecordingGroup : IDisposable
{
    private sealed class Recording(SessionStart source, SessionWriter? writer, EventCounter counter,
        EventPipeline pipeline, string? error)
    {
        internal SessionStart Source { get; } = source;
        internal SessionWriter? Writer { get; } = writer;
        internal EventCounter Counter { get; } = counter;
        internal EventPipeline Pipeline { get; } = pipeline;
        internal string? Error => error ?? Writer?.Failure?.Message ?? Pipeline.Failure?.Message;
    }

    private readonly Dictionary<int, Recording> recordings = [];
    private readonly int primaryProcessId;
    private readonly int? launcherProcessId;
    private int closed;

    internal ProcessRecordingGroup(IReadOnlyCollection<SessionStart> sources, int primaryProcessId,
        int? launcherProcessId, string? directory)
    {
        if (sources.Count == 0 || sources.Select(source => source.ProcessId).Distinct().Count() != sources.Count ||
            !sources.Any(source => source.ProcessId == primaryProcessId))
        {
            throw new ArgumentException("A recording group needs distinct process IDs and its primary process.", nameof(sources));
        }
        this.primaryProcessId = primaryProcessId;
        this.launcherProcessId = launcherProcessId;
        try
        {
            foreach (var source in sources)
            {
                SessionWriter? writer = null;
                string? failure = null;
                if (directory is not null)
                {
                    try
                    {
                        var path = Path.Combine(directory,
                            $"{DateTime.Now:yyyyMMdd-HHmmss}-{source.ProcessName}-{source.ProcessId}.sqlite");
                        writer = SessionWriter.Create(path, source);
                    }
                    catch (Exception error) when (error is not OutOfMemoryException)
                    {
                        failure = $"The session file could not be created: {error.Message}";
                        Trace.TraceWarning(error.ToString());
                    }
                }
                var counter = new EventCounter();
                try
                {
                    recordings.Add(source.ProcessId, new(source, writer, counter,
                        new EventPipeline(writer is null ? [counter] : [counter, writer]), failure));
                }
                catch
                {
                    writer?.Dispose();
                    throw;
                }
            }
        }
        catch
        {
            Dispose();
            throw;
        }
    }

    internal ProcessRecordingStatus Primary => Status(recordings[primaryProcessId]);
    internal IReadOnlyList<ProcessRecordingStatus> Processes => recordings.Values.Select(Status).ToArray();
    // Every session file of the group, the primary process's first.
    internal IReadOnlyList<string> RecordingPaths => recordings.Values
        .OrderBy(recording => recording.Source.ProcessId != primaryProcessId)
        .Select(recording => recording.Writer?.Path)
        .OfType<string>()
        .ToArray();
    internal Exception? Failure => recordings.Values.Select(recording => recording.Pipeline.Failure).FirstOrDefault(error => error is not null);
    internal string? RecordingError => recordings.Values.FirstOrDefault(recording => recording.Error is not null) is { } failed
        ? $"PID {failed.Source.ProcessId}: {failed.Error}" : null;

    private ProcessRecordingStatus Status(Recording recording) => new(recording.Source,
        recording.Source.ProcessId == primaryProcessId, recording.Writer?.Status,
        recording.Counter.XamlEvents, recording.Counter.ProfilerEvents, recording.Counter.GpuMemoryEvents,
        recording.Pipeline.DroppedEvents, recording.Error);

    internal void Post(ProfilerEvent profilerEvent)
    {
        if (recordings.TryGetValue(profilerEvent.ProcessId, out var recording))
        {
            recording.Pipeline.Post(profilerEvent);
        }
        else if (profilerEvent.ProcessId == launcherProcessId && profilerEvent.Provider == XamlProvider.Marker)
        {
            recordings[primaryProcessId].Pipeline.Post(profilerEvent);
        }
    }

    // Whether a process's samples are stored: it's one of the target processes, has a session file, and the event is from
    // after the process's creation, so an earlier process that had its ID isn't. Safe on any thread: the group doesn't
    // change.
    internal bool IsRecorded(int processId, long eventFileTime) =>
        recordings.TryGetValue(processId, out var recording) && recording.Writer is not null &&
        (recording.Source.ProcessCreatedUtc is not { } created || eventFileTime >= created.ToFileTimeUtc());
    internal bool IsWriting => recordings.Values.Any(recording => recording.Writer is not null);

    // Stores a live CPU sample in its process's session file, on any thread. It doesn't wait for the writer, which
    // counts the sample as dropped when its queue is full.
    internal void PostSample(CompletedSample sample)
    {
        if (recordings.TryGetValue(sample.ProcessId, out var recording))
        {
            recording.Writer?.PostSample(sample, wait: false);
        }
    }

    // What the CPU sampling recorded, or why there is none (summary null), for every session file. Call before Close.
    internal void SetCpuSampling(CpuSamplingSummary? summary, string? note)
    {
        foreach (var recording in recordings.Values)
        {
            recording.Writer?.SetCpuSampling(summary, note);
        }
    }

    internal void Close(string reason, int? lostEvents = null)
    {
        if (Interlocked.Exchange(ref closed, 1) != 0) return;
        var failures = new List<Exception>();
        foreach (var recording in recordings.Values)
        {
            try { recording.Pipeline.Dispose(); }
            catch (Exception error) { failures.Add(error); }
            try { recording.Writer?.Close(reason, lostEvents, recording.Pipeline.DroppedEvents); }
            catch (Exception error) { failures.Add(error); }
        }
        if (failures.Count > 0) throw new AggregateException("Closing process recordings failed.", failures);
    }

    public void Dispose() => Close("Disconnected");
}
