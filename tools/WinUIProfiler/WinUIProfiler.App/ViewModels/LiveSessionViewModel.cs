using System.ComponentModel;
using System.Diagnostics;
using WinUIProfiler.Engine.Metrics;
using WinUIProfiler.Engine.Storage;
using WinUIProfiler.Engine.Target;

namespace WinUIProfiler.App.ViewModels;

public sealed class LiveSessionViewModel : INotifyPropertyChanged
{
    private volatile TargetConnection? connection;
    private CancellationTokenSource? operation;
    private Task? connecting;
    private string state = "Disconnected";
    private string detail = "";
    private string errorMessage = "";
    private EtwStatus? finalEtw;
    private string? finalEtwError;
    private RecordingStatus? finalRecording;
    private string? finalRecordingError;
    public event PropertyChangedEventHandler? PropertyChanged;
    public MetricHistory? History
    {
        get; private set;
    }
    public bool IsBusy
    {
        get; private set;
    }
    public bool CanConnect => !IsBusy && connection is null;
    public bool CanDisconnect => operation is not null || connection is not null;
    public string State => connection?.State ?? state;
    public string Detail => connection?.Failure?.Message ?? connection?.Detail ?? detail;
    public string ErrorMessage => connection?.Failure?.Message ?? errorMessage;
    public TargetConnection? Connection => connection;
    // Like the metric history, the last ETW counts stay visible after the connection ends.
    public EtwStatus? Etw => connection?.Etw ?? finalEtw;
    public string? EtwError => connection?.EtwError ?? finalEtwError;
    public RecordingStatus? Recording => connection?.Recording ?? finalRecording;
    public string? RecordingError => connection?.RecordingError ?? finalRecordingError;

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
        connection = null;
        Notify();
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
            connection = null;
            IsBusy = false;
            Notify();
        }
    }

    public void ReportError(Exception error)
    {
        Trace.TraceError(error.ToString());
        errorMessage = error.Message;
        Notify();
    }
}
