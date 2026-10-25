using System.ComponentModel;
using System.Diagnostics;
using System.Runtime.Versioning;
using WinUIProfiler.Engine.Target;

namespace WinUIProfiler.App.ViewModels;

[SupportedOSPlatform("windows")]
public sealed class LiveSessionViewModel : INotifyPropertyChanged
{
    private TargetConnection? connection;
    private CancellationTokenSource? cancellation;
    private Task? connecting;
    private Task? stoppingTask;
    private Task? snapshotting;
    private Task? refreshingMetrics;
    private bool stopping;
    public event PropertyChangedEventHandler? PropertyChanged;
    public bool IsBusy { get; private set; }
    public bool CanConnect => !IsBusy && !stopping && !IsTakingSnapshot && (connection is null || connection.Completion.IsCompleted);
    public bool CanDisconnect => !stopping && (IsBusy || connection is not null);
    public bool IsTakingSnapshot { get; private set; }
    public bool CanTakeSnapshot => !IsBusy && !stopping && !IsTakingSnapshot &&
        connection is { State: "Connected" } && !connection.Completion.IsCompleted;
    public string SnapshotStatus { get; private set; } = "";
    public string SnapshotPath { get; private set; } = "";
    public SnapshotHistory Snapshots { get; } = new();
    public LiveMetricGraphs Metrics { get; } = new();
    public string State { get; private set; } = "Disconnected";
    public string Detail { get; private set; } = "Launch an executable, activate a packaged AUMID, or attach using a process ID.";
    public string Statistics { get; private set; } = "";
    public string DatabasePath { get; private set; } = "";
    public string ErrorMessage { get; private set; } = "";
    public bool HasError => ErrorMessage.Length > 0;

    public Task ConnectAsync(TargetConnectionOptions options)
    {
        if (!CanConnect) throw new InvalidOperationException("A target connection is already active.");
        stoppingTask = null;
        cancellation?.Dispose();
        cancellation = new();
        IsBusy = true;
        ErrorMessage = "";
        SnapshotStatus = SnapshotPath = "";
        Metrics.Reset();
        Notify();
        connecting = ConnectCoreAsync(options, cancellation.Token);
        return connecting;
    }

    private async Task ConnectCoreAsync(TargetConnectionOptions options, CancellationToken token)
    {
        try
        {
            if (connection is not null)
            {
                var previous = connection;
                connection = null;
                await previous.DisposeAsync();
            }
            var progress = new Progress<ConnectionProgress>(update =>
            {
                State = update.State;
                Detail = update.Detail;
                Notify();
            });
            connection = await Task.Run(() => TargetConnection.ConnectAsync(options, progress, token));
            DatabasePath = connection.DatabasePath;
            Refresh();
            await RefreshMetricsAsync();
        }
        catch (OperationCanceledException) when (token.IsCancellationRequested)
        {
            State = "Cancelled";
            Detail = "Connection cancelled; the target was not terminated.";
        }
        catch (Exception error)
        {
            State = "Failed";
            ReportError(error);
        }
        finally { IsBusy = false; Notify(); }
    }

    public void Refresh()
    {
        if (connection is null) return;
        State = connection.State;
        var queue = connection.QueueUsage;
        Statistics = $"PID {connection.Scope.Process.ProcessId} | ETW events {connection.ReceivedEvents} | " +
            $"XAML events {connection.ReceivedRuntimeEvents} | " +
            $"Committed {connection.CommittedEvents} | Queue {queue.Items} / {queue.Bytes} bytes | " +
            $"Listener thread {connection.ListenerThreadId} | Processor thread {connection.ProcessingThreadId}";
        DatabasePath = connection.DatabasePath;
        if (connection.Failure is { } error && ErrorMessage.Length == 0) ReportError(error);
        Notify();
    }

    public Task StopAsync() => stoppingTask ??= StopCoreAsync();

    public Task RefreshMetricsAsync()
    {
        if (refreshingMetrics is { IsCompleted: false }) return refreshingMetrics;
        if (connection is null || stopping) return Task.CompletedTask;
        return refreshingMetrics = RefreshMetricsCoreAsync(connection, cancellation!.Token);
    }

    private async Task RefreshMetricsCoreAsync(TargetConnection selected, CancellationToken token)
    {
        try
        {
            var page = await selected.ReadRecentMetricsAsync(token);
            if (!token.IsCancellationRequested && ReferenceEquals(connection, selected))
                Metrics.Apply(page, Stopwatch.Frequency);
        }
        catch (OperationCanceledException) when (token.IsCancellationRequested) { }
        catch (Exception error) { ReportError(error); }
    }

    public Task TakeSnapshotAsync()
    {
        if (!CanTakeSnapshot) throw new InvalidOperationException("Connect to a target before taking a snapshot.");
        IsTakingSnapshot = true;
        ErrorMessage = SnapshotPath = "";
        SnapshotStatus = "Capturing realized XAML and composition trees inside the target TAP...";
        Notify();
        return snapshotting = TakeSnapshotCoreAsync(connection!);
    }

    private async Task TakeSnapshotCoreAsync(TargetConnection selected)
    {
        try
        {
            var saved = await Task.Run(() => selected.TakeSnapshotAsync(cancellation!.Token));
            Snapshots.Add(saved, selected.Scope.Process, DateTimeOffset.UtcNow);
            SnapshotPath = saved.Path;
            SnapshotStatus = $"Saved {saved.Roots} roots, {saved.XamlNodes} XAML objects, " +
                $"{saved.CompositionNodes} composition visuals, {saved.Correlations} object-to-visual links.";
        }
        catch (OperationCanceledException) when (stopping || cancellation?.IsCancellationRequested == true)
        { SnapshotStatus = "Snapshot cancelled; no partial snapshot was published."; }
        catch (Exception error) { SnapshotStatus = "Snapshot failed; no partial snapshot was published."; ReportError(error); }
        finally { IsTakingSnapshot = false; Notify(); }
    }

    private async Task StopCoreAsync()
    {
        stopping = true;
        Notify();
        try
        {
            if (cancellation is not null) await cancellation.CancelAsync();
            if (connecting is not null) await connecting;
            if (snapshotting is not null) await snapshotting;
            if (refreshingMetrics is not null) await refreshingMetrics;
            if (connection is not null)
            {
                var selected = connection;
                try
                {
                    await selected.DisposeAsync();
                    Metrics.Apply(await selected.ReadRecentMetricsAsync(), Stopwatch.Frequency);
                }
                finally
                {
                    Refresh();
                    connection = null;
                }
            }
            if (!HasError) State = "Disconnected";
        }
        catch (Exception error) { ReportError(error); State = "Failed"; }
        finally { stopping = false; Notify(); }
    }

    public void ReportError(Exception error)
    {
        ErrorMessage = error.Message;
        Trace.TraceError(error.ToString());
        Notify();
    }

    private void Notify() => PropertyChanged?.Invoke(this, new(null));
}
