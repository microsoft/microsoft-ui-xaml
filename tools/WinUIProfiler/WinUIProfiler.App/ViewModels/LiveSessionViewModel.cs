using System.ComponentModel;
using System.Diagnostics;
using WinUIProfiler.Engine.Events;
using WinUIProfiler.Engine.Ingestion;
using WinUIProfiler.Engine.Metrics;
using WinUIProfiler.Engine.Storage;
using WinUIProfiler.Engine.Symbols;
using WinUIProfiler.Engine.Target;

namespace WinUIProfiler.App.ViewModels;

// A connection to a running target, or the reading of a recorded trace (State starts with "Trace"); one at a time.
public sealed class LiveSessionViewModel : INotifyPropertyChanged
{
    private volatile TargetConnection? connection;
    private volatile TraceReplay? replay;
    private TraceReplayStatus? finalTrace;
    private CancellationTokenSource? operation;
    private Task? connecting;
    private string state = "Disconnected";
    private string detail = "";
    private string errorMessage = "";
    private EtwStatus? finalEtw;
    private string? finalEtwError;
    private RecordingStatus? finalRecording;
    private string? finalRecordingError;
    private string? finalCpuSamplingError;
    // The symbolizing of the last session's files, which starts once its connection or trace reading has ended and closed
    // them.
    private CancellationTokenSource? symbolizing;
    private IReadOnlyList<string> symbolizedPaths = [];
    private bool symbolsStopped;
    // Every symbolizing started, superseded ones included, which may still be writing to their session files.
    private Task naming = Task.CompletedTask;
    public event PropertyChangedEventHandler? PropertyChanged;
    public MetricHistory? History
    {
        get; private set;
    }
    public bool IsBusy
    {
        get; private set;
    }
    public bool CanConnect => !IsBusy && connection is null && replay is null;
    public bool CanDisconnect => operation is not null || connection is not null || replay?.State == TraceReplayState.Reading;
    public string State => connection?.State ?? (replay is { } reading ? "Trace" + reading.State : state);
    public string Detail => connection?.Failure?.Message ?? connection?.Detail ?? detail;
    public string ErrorMessage => connection?.Failure?.Message ?? errorMessage;
    public TargetConnection? Connection => connection;
    // Like the metric history, the last ETW counts stay visible after the connection ends.
    public EtwStatus? Etw => connection?.Etw ?? finalEtw;
    public string? EtwError => connection?.EtwError ?? finalEtwError;
    public RecordingStatus? Recording => connection?.Recording ?? replay?.Recording ?? finalRecording;
    public string? RecordingError => connection?.RecordingError ?? replay?.RecordingError ?? finalRecordingError;
    // Why a connection records no CPU samples, or only some.
    public string? CpuSamplingError => connection?.CpuSamplingError ?? finalCpuSamplingError;
    // The symbol path that the functions of the next session's CPU call stacks are named from: in memory by the CPU tab
    // while it records, and in its file once it ends; null to not name them. Only this path is searched (never
    // _NT_SYMBOL_PATH).
    public string? SymbolPath
    {
        get; set;
    }
    // What the ended session's file recorded of CPU sampling, once read; null when it did not sample CPU.
    public CpuSamplingInfo? CpuSampling
    {
        get; private set;
    }
    public bool Symbolizing => symbolizing is not null;
    // The session file whose functions are being named, which isn't read meanwhile; null when none is.
    public string? SymbolizingPath
    {
        get; private set;
    }
    // The symbol path of the symbolizing that is running or ended, and how it ended (over all the session's files).
    public string? SymbolsFrom
    {
        get; private set;
    }
    public SymbolizeResult? Symbols
    {
        get; private set;
    }
    public string? SymbolsError
    {
        get; private set;
    }
    // The trace being read, or the last one read, until the next connection.
    public string? TracePath
    {
        get; private set;
    }
    public TraceProcess? TraceTarget
    {
        get; private set;
    }
    public TraceReplayStatus? TraceStatus => replay?.Status ?? finalTrace;

    public Task ConnectAsync(TargetConnectionOptions options, IProgress<ConnectionProgress> progress)
    {
        if (!CanConnect)
        {
            throw new InvalidOperationException("Disconnect the current target before connecting again.");
        }

        connecting = ConnectCoreAsync(options, progress);
        return connecting;
    }

    private async Task ConnectCoreAsync(TargetConnectionOptions options, IProgress<ConnectionProgress> progress)
    {
        IsBusy = true;
        state = "Launching";
        detail = options.Target;
        errorMessage = "";
        History = null;
        finalEtw = null;
        finalEtwError = null;
        finalRecording = null;
        finalRecordingError = null;
        finalTrace = null;
        TracePath = null;
        TraceTarget = null;
        ResetSymbols();
        using var cancellation = new CancellationTokenSource();
        operation = cancellation;
        Notify();
        try
        {
            var candidate = await Task.Run(() => TargetConnection.ConnectAsync(options, progress, cancellation.Token));
            if (cancellation.IsCancellationRequested)
            {
                await candidate.DisposeAsync();
                cancellation.Token.ThrowIfCancellationRequested();
            }
            connection = candidate;
            History = candidate.History;
            candidate.Changed += Connection_Changed;
            if (History is not null)
            {
                History.Changed += History_Changed;
            }
            _ = CompleteConnectionAsync(candidate);
        }
        catch (OperationCanceledException) when (cancellation.IsCancellationRequested)
        {
            state = "Disconnected";
            detail = "";
        }
        catch (Exception error)
        {
            state = "Failed";
            ReportError(error);

        }
        finally
        {
            operation = null;
            IsBusy = false;
            Notify();

        }
    }

    public void ReportProgress(ConnectionProgress progress)
    {
        state = progress.State;
        detail = progress.Detail;
        Notify();
    }

    // Reads process's events, those of the marker providers, and those of the launcher providers of the process that
    // created it, from the trace at etlPath into a new session file in sessionDirectory. Completes once the reading has
    // started; State then follows it to TraceFinished, TraceStopped or TraceFailed.
    public Task OpenTraceAsync(string etlPath, TraceProcess process, string sessionDirectory,
        IReadOnlyCollection<MarkerProviderSettings> markers)
    {
        if (!CanConnect)
        {
            throw new InvalidOperationException("Disconnect the current target before reading a trace.");
        }

        connecting = OpenTraceCoreAsync(etlPath, process, sessionDirectory, markers);
        return connecting;
    }

    private async Task OpenTraceCoreAsync(string etlPath, TraceProcess process, string sessionDirectory,
        IReadOnlyCollection<MarkerProviderSettings> markers)
    {
        IsBusy = true;
        state = "TraceOpening";
        detail = "";
        errorMessage = "";
        History = null;
        finalEtw = null;
        finalEtwError = null;
        finalRecording = null;
        finalRecordingError = null;
        finalTrace = null;
        TracePath = etlPath;
        TraceTarget = process;
        ResetSymbols();
        Notify();
        try
        {
            var started = await Task.Run(() => TraceReplay.Start(etlPath, process.ProcessId, process.Name, sessionDirectory,
                SessionWriter.DefaultLimitBytes, markers));
            replay = started;
            started.Changed += Replay_Changed;
            _ = CompleteReplayAsync(started);
        }
        catch (Exception error)
        {
            state = "TraceFailed";
            ReportError(error);
        }
        finally
        {
            IsBusy = false;
            Notify();
        }
    }

    private void Connection_Changed(object? sender, EventArgs args)
    {
        if (ReferenceEquals(sender, connection))
        {
            Notify();
        }
    }

    private async Task CompleteConnectionAsync(TargetConnection owned)
    {
        Exception? failure = null;
        try
        {
            await owned.Completion;
            await owned.DisposeAsync();
        }
        catch (Exception error)
        {
            failure = error;
            Trace.TraceError(error.ToString());
        }

        if (!ReferenceEquals(owned, connection))
        {
            return;
        }

        owned.Changed -= Connection_Changed;
        if (owned.History is not null)
        {
            owned.History.Changed -= History_Changed;
        }

        state = failure is null ? owned.State : "Failed";
        detail = owned.Detail;
        errorMessage = owned.Failure?.Message ?? failure?.Message ?? "";
        finalEtw = owned.Etw;
        finalEtwError = owned.EtwError;
        finalRecording = owned.Recording;
        finalRecordingError = owned.RecordingError;
        finalCpuSamplingError = owned.CpuSamplingError;
        connection = null;
        Notify();
        StartSymbolizing(owned.RecordingPaths);
    }

    private void Replay_Changed(object? sender, EventArgs args)
    {
        if (ReferenceEquals(sender, replay))
        {
            Notify();
        }
    }

    private async Task CompleteReplayAsync(TraceReplay owned)
    {
        await owned.Completion;
        owned.Changed -= Replay_Changed;
        await owned.DisposeAsync();
        if (!ReferenceEquals(owned, replay))
        {
            return;
        }

        if (owned.Failure is { } failure)
        {
            Trace.TraceError(failure.ToString());
        }

        state = "Trace" + owned.State;
        errorMessage = owned.Failure?.Message ?? "";
        finalTrace = owned.Status;
        finalRecording = owned.Recording;
        finalRecordingError = owned.RecordingError;
        replay = null;
        Notify();
        StartSymbolizing(finalRecording?.Path is { } path ? [path] : []);
    }

    private void History_Changed(object? sender, EventArgs args)
    {
        if (ReferenceEquals(sender, History))
        {
            PropertyChanged?.Invoke(this, new(nameof(History)));
        }
    }

    private void Notify() => PropertyChanged?.Invoke(this, new(null));

    public async Task DisconnectAsync()
    {
        operation?.Cancel();
        if (connecting is { IsCompleted: false })
        {
            await connecting;
        }

        if (replay is { } reading)
        {
            // Completes once the session file is closed; CompleteReplayAsync shows how the reading ended.
            await reading.StopAsync();
            return;
        }

        var owned = connection;
        if (owned is null)
        {
            return;
        }

        IsBusy = true;
        Notify();
        try
        {
            await owned.DisposeAsync();
            state = "Disconnected";
            detail = "";

        }
        catch (Exception error)
        {
            state = "Failed";
            ReportError(error);

        }
        finally
        {
            owned.Changed -= Connection_Changed;
            if (History is not null)
            {
                History.Changed -= History_Changed;
            }

            finalEtw = owned.Etw;
            finalEtwError = owned.EtwError;
            finalRecording = owned.Recording;
            finalRecordingError = owned.RecordingError;
            finalCpuSamplingError = owned.CpuSamplingError;
            connection = null;
            IsBusy = false;
            Notify();
            StartSymbolizing(owned.RecordingPaths);
        }
    }

    // Stops symbolizing for good, as the window closes, and waits until no session file is being written: the module being
    // named is rolled back, and the modules already done stay named. A file left mid-write couldn't be opened read-only.
    public async Task CancelSymbolsAsync()
    {
        symbolsStopped = true;
        symbolizing?.Cancel();
        await naming;
    }

    private void StartSymbolizing(IReadOnlyList<string> paths)
    {
        var run = SymbolizeAsync(paths);
        naming = naming.IsCompleted ? run : Task.WhenAll(naming, run);
    }

    private void ResetSymbols()
    {
        symbolizing?.Cancel();
        symbolizing = null;
        symbolizedPaths = [];
        SymbolizingPath = null;
        finalCpuSamplingError = null;
        CpuSampling = null;
        SymbolsFrom = null;
        Symbols = null;
        SymbolsError = null;
    }

    // Reads what an ended session's file recorded of CPU sampling and, given a symbol path, names the functions of the
    // samples in each of the session's files (the primary process's first; a private-runtime launch hands off to more than
    // one), one at a time on a worker thread. Both ways a connection ends call it, so each session is done once. Symbols
    // describes the primary file, which the tabs show, and SymbolsError the first file that failed.
    private async Task SymbolizeAsync(IReadOnlyList<string> paths)
    {
        if (symbolsStopped || paths.Count == 0 || paths.SequenceEqual(symbolizedPaths, StringComparer.OrdinalIgnoreCase))
        {
            return;
        }

        symbolizedPaths = paths;
        symbolizing?.Cancel();
        using var cancellation = new CancellationTokenSource();
        symbolizing = cancellation;
        Symbols = null;
        SymbolsError = null;
        var symbolPath = string.IsNullOrWhiteSpace(SymbolPath) ? null : SymbolPath;
        var primary = finalRecording?.Path;
        try
        {
            foreach (var path in paths)
            {
                var isPrimary = string.Equals(path, primary, StringComparison.OrdinalIgnoreCase);
                if (!isPrimary && symbolPath is null)
                {
                    continue;
                }

                try
                {
                    var sampling = await Task.Run(() => File.Exists(path) ? CpuProfileReader.ReadSampling(path) : null, cancellation.Token);
                    if (!ReferenceEquals(symbolizing, cancellation))
                    {
                        return;
                    }

                    if (isPrimary)
                    {
                        CpuSampling = sampling;
                    }

                    if (sampling is not { Samples: > 0 } || symbolPath is null)
                    {
                        continue;
                    }

                    SymbolsFrom = SymbolPaths.Redact(symbolPath);
                    SymbolizingPath = path;
                    Notify();
                    var symbols = await Task.Run(
                        () => SessionSymbolizer.Symbolize(path, symbolPath, cancellationToken: cancellation.Token), cancellation.Token);
                    if (!ReferenceEquals(symbolizing, cancellation))
                    {
                        return;
                    }

                    if (isPrimary)
                    {
                        Symbols = symbols;
                    }
                }
                catch (Exception error) when (error is not OutOfMemoryException)
                {
                    if (cancellation.IsCancellationRequested)
                    {
                        return;
                    }

                    Trace.TraceWarning("Naming the functions of the CPU samples in {0} failed: {1}", path,
                        SymbolPaths.Redact(error.ToString()));
                    if (ReferenceEquals(symbolizing, cancellation))
                    {
                        var message = SymbolPaths.Redact(error.Message);
                        SymbolsError ??= isPrimary ? message : $"{Path.GetFileName(path)}: {message}";
                    }
                }
                finally
                {
                    if (ReferenceEquals(symbolizing, cancellation) && SymbolizingPath is not null)
                    {
                        SymbolizingPath = null;
                        Notify();
                    }
                }
            }
        }
        finally
        {
            if (ReferenceEquals(symbolizing, cancellation))
            {
                symbolizing = null;
                Notify();
            }
        }
    }

    public void ReportError(Exception error)
    {
        Trace.TraceError(error.ToString());
        errorMessage = error.Message;
        Notify();
    }
}
