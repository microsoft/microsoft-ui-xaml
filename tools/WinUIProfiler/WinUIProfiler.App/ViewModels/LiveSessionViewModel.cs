using System.Diagnostics;
using WinUIProfiler.Engine.Metrics;
using WinUIProfiler.Engine.Target;

namespace WinUIProfiler.App.ViewModels;

public sealed class LiveSessionViewModel
{
    private TargetConnection? connection;
    private CancellationTokenSource? operation;
    private Task? connecting;
    public MetricHistory? History { get; private set; }
    public bool IsBusy { get; private set; }
    public bool CanConnect => !IsBusy && connection is null;
    public bool CanDisconnect => operation is not null || connection is not null;
    public string State { get; private set; } = "Disconnected";
    public string Detail { get; private set; } = "";
    public string ErrorMessage { get; private set; } = "";
    public TargetConnection? Connection => connection;

    public Task ConnectAsync(TargetConnectionOptions options, IProgress<ConnectionProgress> progress)
    {
        if (!CanConnect) throw new InvalidOperationException("Disconnect the current target before connecting again.");
        connecting = ConnectCoreAsync(options, progress);
        return connecting;
    }

    private async Task ConnectCoreAsync(TargetConnectionOptions options, IProgress<ConnectionProgress> progress)
    {
        IsBusy = true;
        ErrorMessage = "";
        History = null;
        using var cancellation = new CancellationTokenSource();
        operation = cancellation;
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
            Refresh();
        }
        catch (OperationCanceledException) when (cancellation.IsCancellationRequested)
        {
            State = "Disconnected";
            Detail = "";
        }
        catch (Exception error) { State = "Failed"; ReportError(error); }
        finally { operation = null; IsBusy = false; }
    }

    public void ReportProgress(ConnectionProgress progress)
    {
        State = progress.State;
        Detail = progress.Detail;
    }

    public void Refresh()
    {
        if (connection is null) return;
        State = connection.State;
        Detail = connection.Detail;
        if (connection.Failure is { } error)
        {
            Detail = error.Message;
            if (ErrorMessage != error.Message) ReportError(error);
        }
    }

    public async Task DisconnectAsync()
    {
        operation?.Cancel();
        if (connecting is { IsCompleted: false }) await connecting;
        var owned = connection;
        if (owned is null) return;
        IsBusy = true;
        try { await owned.DisposeAsync(); State = "Disconnected"; Detail = ""; }
        catch (Exception error) { State = "Failed"; ReportError(error); }
        finally { connection = null; IsBusy = false; }
    }

    public void ReportError(Exception error)
    {
        Trace.TraceError(error.ToString());
        ErrorMessage = error.Message;
    }
}
