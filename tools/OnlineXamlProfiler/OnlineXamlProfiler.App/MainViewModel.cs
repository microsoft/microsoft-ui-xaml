// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Diagnostics;
using System.Globalization;
using System.Runtime.CompilerServices;
using Microsoft.UI.Dispatching;
using Microsoft.Windows.ApplicationModel.Resources;
using OnlineXamlProfiler.Core;
using OnlineXamlProfiler.Windows;

namespace OnlineXamlProfiler.App;

public sealed record PropertyRow(string Name, string Value);
public sealed record CapturedSnapshot(
    string Label, SnapshotDocument Document,
    IReadOnlyList<SnapshotTreeNode> XamlRoots,
    IReadOnlyList<SnapshotTreeNode> CompRoots,
    IReadOnlyList<SnapshotTreeNode> VisualRoots,
    DateTimeOffset CapturedAtUtc, long CounterTimestamp, ProcessStatisticsSample? Statistics,
    ulong? ObservedGpuMemoryPeakBytes);

public sealed class MainViewModel : INotifyPropertyChanged, IDisposable
{
    private readonly ResourceLoader _resources = new();
    private readonly DispatcherQueue _dispatcher = DispatcherQueue.GetForCurrentThread();
    private ProfilerSession? _session;
    private CancellationTokenSource? _operation;
    private CancellationTokenSource? _statisticsLifetime;
    private bool _busy;
    private string _status = "";
    private string _error = "";
    private CapturedSnapshot? _selectedSnapshot;

    public MainViewModel()
    {
        LaunchExeCommand = new(LaunchExeAsync, () => !_busy && _session is null, ReportError);
        LaunchPackageCommand = new(LaunchPackageAsync, () => !_busy && _session is null, ReportError);
        AttachCommand = new(AttachAsync, () => !_busy && _session is null, ReportError);
        CaptureCommand = new(CaptureAsync, () => !_busy && _session is not null, ReportError);
        DisconnectCommand = new(() => { Disconnect(); return Task.CompletedTask; }, () => _session is not null, ReportError);
        CancelCommand = new(() => { _operation?.Cancel(); return Task.CompletedTask; }, () => _busy, ReportError);
        _status = Text("Ready");
    }

    public event PropertyChangedEventHandler? PropertyChanged;
    public string Executable { get; set; } = "";
    public string Arguments { get; set; } = "";
    public string Aumid { get; set; } = "";
    public string ProcessId { get; set; } = "";
    public string TapPath { get; set; } = Path.Combine(AppContext.BaseDirectory, "OnlineXamlProfilerTap.dll");
    public string Status { get => _status; private set { _status = value; Notify(); } }
    public string Error { get => _error; private set { _error = value; Notify(); Notify(nameof(HasError)); } }
    public bool HasError => Error.Length != 0;
    public bool IsBusy { get => _busy; private set { _busy = value; Notify(); NotifyCommands(); } }
    public AsyncCommand LaunchExeCommand { get; }
    public AsyncCommand LaunchPackageCommand { get; }
    public AsyncCommand AttachCommand { get; }
    public AsyncCommand CaptureCommand { get; }
    public AsyncCommand DisconnectCommand { get; }
    public AsyncCommand CancelCommand { get; }
    public ObservableCollection<CapturedSnapshot> Snapshots { get; } = [];
    public ObservableCollection<PropertyRow> Properties { get; } = [];
    public ProcessStatisticsHistory? StatisticsHistory { get; private set; }
    public string StatisticsStatus { get; private set; } = "";

    public CapturedSnapshot? SelectedSnapshot
    {
        get => _selectedSnapshot;
        set
        {
            if (ReferenceEquals(_selectedSnapshot, value))
            {
                return;
            }
            _selectedSnapshot = value;
            Notify();
            Notify(nameof(XamlRoots));
            Notify(nameof(CompRoots));
            Notify(nameof(VisualRoots));
            Properties.Clear();
            SelectedNode = null;
            if (value is not null)
            {
                Error = string.Join(Environment.NewLine, value.Document.Threads.Where(thread => thread.Status < 0)
                    .Select(thread => $"Thread {thread.ThreadId}: 0x{unchecked((uint)thread.Status):X8} {thread.Error}"));
                Status = $"{Text("Captured")} {value.Document.Threads.Sum(thread => thread.Nodes.Count)} | " +
                    $"{Text("Threads")} {value.Document.Threads.Count}";
            }
        }
    }

    public IReadOnlyList<SnapshotTreeNode> XamlRoots => SelectedSnapshot?.XamlRoots ?? [];
    public IReadOnlyList<SnapshotTreeNode> CompRoots => SelectedSnapshot?.CompRoots ?? [];
    public IReadOnlyList<SnapshotTreeNode> VisualRoots => SelectedSnapshot?.VisualRoots ?? [];
    public SnapshotTreeNode? SelectedNode { get; private set; }

    public void SelectNode(SnapshotTreeNode? selected)
    {
        SelectedNode = selected;
        Properties.Clear();
        if (selected is null)
        {
            return;
        }
        var node = selected.Node;
        if (SelectedSnapshot is { } snapshot)
        {
            var thread = snapshot.Document.Threads.Single(thread => thread.ThreadId == selected.ThreadId);
            Properties.Add(new("ProcessId", snapshot.Document.ProcessId.ToString(CultureInfo.InvariantCulture)));
            Properties.Add(new("ProcessStartTime", snapshot.Document.ProcessStartTime));
            Properties.Add(new("RequestId", snapshot.Document.RequestId));
            Properties.Add(new("CaptureStartQpc", thread.StartQpc));
            Properties.Add(new("CaptureEndQpc", thread.EndQpc));
            Properties.Add(new("QpcFrequency", thread.QpcFrequency));
        }
        Properties.Add(new("ThreadId", selected.ThreadId.ToString(CultureInfo.InvariantCulture)));
        Properties.Add(new("Tree", node.Tree.ToString()));
        Properties.Add(new("Type", node.Type));
        Properties.Add(new("Name", node.Name));
        Properties.Add(new("Id", node.Id));
        Properties.Add(new("ParentId", node.ParentId));
        Properties.Add(new("XamlId", node.XamlId));
        Properties.Add(new("CompId", node.CompId));
        Properties.Add(new("VisualId", node.VisualId));
        foreach (var property in node.Properties)
        {
            Properties.Add(new(property.Name, property.Status >= 0
                ? property.Value! : $"HRESULT 0x{unchecked((uint)property.Status):X8}"));
        }
    }

    public SnapshotTreeNode? FindLinkedNode(TreeKind kind)
    {
        if (SelectedNode is null)
        {
            return null;
        }
        string id = kind switch
        {
            TreeKind.Xaml => SelectedNode.Node.XamlId,
            TreeKind.Composition => SelectedNode.Node.CompId,
            TreeKind.Visual => SelectedNode.Node.VisualId,
            _ => throw new ArgumentOutOfRangeException(nameof(kind)),
        };
        // A XAML node links to its HWCompNode; that node records its backing visual.
        if (id.Length == 0 && kind == TreeKind.Visual && SelectedNode.Node.CompId.Length != 0)
        {
            id = Enumerate(CompRoots).FirstOrDefault(node => node.ThreadId == SelectedNode.ThreadId &&
                node.Node.Id == SelectedNode.Node.CompId)?.Node.VisualId ?? "";
        }
        var roots = kind switch { TreeKind.Xaml => XamlRoots, TreeKind.Composition => CompRoots, _ => VisualRoots };
        var linked = Enumerate(roots).FirstOrDefault(node =>
            node.ThreadId == SelectedNode.ThreadId && node.Node.Id == id);
        if (linked is null)
        {
            Status = Text("NoLink");
        }
        return linked;
    }

    private static IEnumerable<SnapshotTreeNode> Enumerate(IReadOnlyList<SnapshotTreeNode> roots)
    {
        var pending = new Stack<SnapshotTreeNode>(roots);
        while (pending.TryPop(out var node))
        {
            yield return node;
            foreach (var child in node.Children)
            {
                pending.Push(child);
            }
        }
    }

    private Task LaunchExeAsync() => RunAsync(async token =>
    {
        uint pid = await Task.Run(() => TargetLauncher.LaunchExecutable(Executable, Arguments), token);
        ProcessId = pid.ToString(CultureInfo.InvariantCulture);
        Notify(nameof(ProcessId));
        await ConnectAsync(pid, token);
    });

    private Task LaunchPackageAsync() => RunAsync(async token =>
    {
        uint pid = await Task.Run(() => TargetLauncher.LaunchPackaged(Aumid, Arguments), token);
        ProcessId = pid.ToString(CultureInfo.InvariantCulture);
        Notify(nameof(ProcessId));
        await ConnectAsync(pid, token);
    });

    private Task AttachAsync() => RunAsync(async token =>
    {
        if (!uint.TryParse(ProcessId, NumberStyles.None, CultureInfo.InvariantCulture, out uint pid) || pid == 0)
        {
            throw new ArgumentException(Text("InvalidPid"));
        }
        await ConnectAsync(pid, token);
    });

    private async Task ConnectAsync(uint pid, CancellationToken token)
    {
        Status = Text("Connecting");
        _session = await ProfilerSession.ConnectAsync(pid, TapPath, token);
        Status = $"{Text("Connected")} {pid} | {_session.RuntimePath}";
        StartStatistics(_session);
    }

    private Task CaptureAsync() => RunAsync(async token =>
    {
        Status = Text("Capturing");
        try
        {
            var session = _session ?? throw new InvalidOperationException(Text("NotConnected"));
            int number = Snapshots.Count + 1;
            var snapshot = await Task.Run(async () =>
            {
                long started = Stopwatch.GetTimestamp();
                var time = DateTimeOffset.UtcNow;
                var document = await session.CaptureAsync(token);
                long midpoint = started + (Stopwatch.GetTimestamp() - started) / 2;
                var capturedAt = time + Stopwatch.GetElapsedTime(started, midpoint);
                return new CapturedSnapshot(
                    $"#{number} | PID {document.ProcessId} | {capturedAt.ToLocalTime():T}",
                    document, SnapshotTree.Build(document, TreeKind.Xaml),
                    SnapshotTree.Build(document, TreeKind.Composition), SnapshotTree.Build(document, TreeKind.Visual),
                    capturedAt, midpoint, null, null);
            }, token);
            if (StatisticsHistory is { } history &&
                history.ProcessId == snapshot.Document.ProcessId &&
                history.ProcessStartTime == snapshot.Document.ProcessStartTime)
            {
                snapshot = snapshot with
                {
                    Statistics = history.FindNearest(snapshot.CounterTimestamp),
                    ObservedGpuMemoryPeakBytes = history.ObservedGpuMemoryPeakBytes,
                };
            }
            Snapshots.Add(snapshot);
            SelectedSnapshot = snapshot;
        }
        catch
        {
            Disconnect();
            throw;
        }
    });

    private async Task RunAsync(Func<CancellationToken, Task> action)
    {
        Error = "";
        IsBusy = true;
        using var operation = new CancellationTokenSource();
        _operation = operation;
        try
        {
            await action(operation.Token);
        }
        finally
        {
            _operation = null;
            IsBusy = false;
        }
    }

    private void Disconnect()
    {
        _operation?.Cancel();
        _statisticsLifetime?.Cancel();
        _statisticsLifetime?.Dispose();
        _statisticsLifetime = null;
        _session?.Dispose();
        _session = null;
        Status = Text("Disconnected");
        StatisticsStatus = Text("StatisticsStopped");
        Notify(nameof(StatisticsHistory));
        NotifyCommands();
    }

    private void StartStatistics(ProfilerSession session)
    {
        var lifetime = new CancellationTokenSource();
        _statisticsLifetime = lifetime;
        var history = new ProcessStatisticsHistory(session.ProcessId, session.ProcessStartTime);
        StatisticsHistory = history;
        StatisticsStatus = Text("StatisticsStarting");
        Notify(nameof(StatisticsHistory));
        _ = SampleStatisticsAsync(session, history, lifetime.Token);
    }

    private async Task SampleStatisticsAsync(
        ProfilerSession session, ProcessStatisticsHistory history, CancellationToken token)
    {
        try
        {
            await Task.Run(async () =>
            {
                using var reader = new ProcessStatisticsReader(session.ProcessId, session.ProcessStartTime);
                using var timer = new PeriodicTimer(TimeSpan.FromSeconds(1));
                do
                {
                    token.ThrowIfCancellationRequested();
                    var sample = reader.Read();
                    if (!_dispatcher.TryEnqueue(() =>
                    {
                        if (token.IsCancellationRequested || !ReferenceEquals(_session, session)) { return; }
                        history.Add(sample);
                        StatisticsStatus = string.Format(CultureInfo.CurrentCulture,
                            Text("StatisticsLive"), session.ProcessId, sample.TimeUtc.ToLocalTime());
                        if (sample.ProcessExited)
                        {
                            Disconnect();
                            StatisticsStatus = Text("StatisticsTargetExited");
                        }
                        Notify(nameof(StatisticsHistory));
                    })) { return; }
                    if (sample.ProcessExited) { return; }
                } while (await timer.WaitForNextTickAsync(token).ConfigureAwait(false));
            }, token);
        }
        catch (OperationCanceledException) when (token.IsCancellationRequested)
        {
            // Disconnect or window close owns the sampler's lifetime.
        }
        catch (Exception exception) when (exception is Win32Exception or InvalidOperationException or IOException or OverflowException)
        {
            if (!token.IsCancellationRequested && ReferenceEquals(_session, session))
            {
                StatisticsStatus = $"{Text("StatisticsFailed")} {exception.Message}";
                Notify(nameof(StatisticsHistory));
            }
        }
    }

    internal void ReportError(Exception exception)
    {
        Error = exception is OperationCanceledException ? Text("Cancelled") : exception.Message;
        Status = Text("Failed");
    }

    private string Text(string key) => _resources.GetString(key);

    private void NotifyCommands()
    {
        LaunchExeCommand.Notify();
        LaunchPackageCommand.Notify();
        AttachCommand.Notify();
        CaptureCommand.Notify();
        DisconnectCommand.Notify();
        CancelCommand.Notify();
    }

    private void Notify([CallerMemberName] string? name = null) => PropertyChanged?.Invoke(this, new(name));

    public void Dispose() => Disconnect();
}
