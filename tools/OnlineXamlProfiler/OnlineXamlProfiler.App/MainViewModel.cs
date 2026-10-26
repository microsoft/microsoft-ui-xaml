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
    SnapshotMemorySummary Memory,
    GpuTraceSnapshot GpuTrace,
    DateTimeOffset CapturedAtUtc, long CounterTimestamp, ProcessStatisticsSample? Statistics,
    ulong? ObservedGpuMemoryPeakBytes);

public sealed class MainViewModel : INotifyPropertyChanged, IDisposable
{
    private readonly ResourceLoader _resources = new();
    private readonly DispatcherQueue _dispatcher = DispatcherQueue.GetForCurrentThread();
    private ProfilerSession? _session;
    private CancellationTokenSource? _operation;
    private CancellationTokenSource? _statisticsLifetime;
    private GpuObservationSession? _gpuTrace;
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
    public bool EnableGpuTracing { get; set; }
    public bool CanConfigureGpuTracing => !_busy && _session is null;
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
            Notify(nameof(MemoryRows));
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

    public IReadOnlyList<PropertyRow> MemoryRows
    {
        get
        {
            if (SelectedSnapshot is not { } snapshot) { return []; }
            var rows = new List<PropertyRow>
            {
                new(Text("MemoryMeasuredCore"), Bytes(snapshot.Memory.MeasuredCoreBytes)),
                new(Text("MemoryUnmeasuredCore"), snapshot.Memory.UnmeasuredCoreCount.ToString(CultureInfo.CurrentCulture)),
                new(Text("MemorySurfaceEstimate"), Bytes(snapshot.Memory.EstimatedSurfaceBytes)),
                new(Text("MemoryUnknownSurfaces"), snapshot.Memory.UnknownSurfaceReferenceCount.ToString(CultureInfo.CurrentCulture)),
            };
            foreach (var thread in snapshot.Document.Threads.Where(thread => thread.Status >= 0))
            {
                var memory = thread.Memory;
                rows.Add(new($"TID {thread.ThreadId} | {Text("MemoryHeap")}",
                    memory.Status < 0 ? Hresult(memory.Status) : memory.HeapStatus < 0 ? Hresult(memory.HeapStatus) :
                    $"{memory.HeapId} | {Bytes(memory.OutstandingBytes)} | {memory.OutstandingCount:N0} | private={memory.IsPrivateHeap}"));
                rows.Add(new($"TID {thread.ThreadId} | {Text("MemoryCache")}", memory.Status < 0 ? Hresult(memory.Status) :
                    memory.CacheCapacity == 0 ? Hresult(memory.CacheStatus) :
                    $"{Hresult(memory.CacheStatus)} | {memory.CachedSurfaceCount:N0}/{memory.CacheCapacity:N0} | " +
                    $"{Text("MemoryDropped")}: {memory.DroppedRegistrations:N0} | {Bytes(memory.CacheBytes)}"));
            }
            rows.Add(new(Text("MemoryAdapterQuery"), Hresult(snapshot.Document.AdapterMemory.Status)));
            rows.Add(new(Text("MemoryAdapterInterval"),
                $"{snapshot.Document.AdapterMemory.StartQpc} / {snapshot.Document.AdapterMemory.EndQpc} / " +
                $"{snapshot.Document.AdapterMemory.QpcFrequency}"));
            foreach (var adapter in snapshot.Document.AdapterMemory.Adapters)
            {
                rows.Add(new($"{adapter.Description} | {adapter.AdapterLuid} | {(adapter.Segment == 0 ? "Local" : "NonLocal")}",
                    adapter.Status < 0 ? Hresult(adapter.Status) :
                    string.Format(CultureInfo.CurrentCulture, Text("MemoryAdapterValues"),
                        Bytes(adapter.CurrentUsage), Bytes(adapter.Budget), Bytes(adapter.CurrentReservation),
                        Bytes(adapter.AvailableForReservation))));
            }
            foreach (var group in snapshot.Memory.CoreByType)
            {
                rows.Add(new(group.Type, string.Format(CultureInfo.CurrentCulture, Text("MemoryTypeValues"),
                    Bytes(group.MeasuredBytes), group.MeasuredCount, group.ObjectCount)));
            }
            var trace = snapshot.GpuTrace;
            rows.Add(new(Text("GpuTraceState"), Text($"GpuTrace{trace.State}")));
            if (trace.State != "Disabled")
            {
                rows.Add(new(Text("GpuTraceCoverage"), string.Format(CultureInfo.CurrentCulture, Text("GpuTraceCoverageValues"),
                    trace.StartedBeforeLaunch ? Text("GpuTraceBeforeLaunch") : Text("GpuTraceLateAttach"),
                    trace.EventsLost, trace.DroppedObservations, trace.InvalidPayloads)));
                rows.Add(new(Text("GpuTraceDelivery"), $"{trace.StartQpc} / {trace.CaptureEndQpc} / {trace.LastDeliveredQpc}"));
                rows.Add(new(Text("GpuTraceReportedBytes"), trace.DistinctReportedAllocationBytes is { } bytes
                    ? Bytes(bytes) : Text("StatisticsUnavailable")));
                rows.Add(new(Text("GpuTracePresents"), trace.ObservedPresentCount.ToString(CultureInfo.CurrentCulture)));
                if (trace.Diagnostic.Length != 0) { rows.Add(new(Text("GpuTraceDiagnostic"), trace.Diagnostic)); }
                foreach (var evidence in trace.Resources)
                {
                    rows.Add(new($"{evidence.ResourceId} | QPC {evidence.DrawObservedQpc}",
                        evidence.Status == GpuEvidenceStatus.Matched ?
                            $"{evidence.AllocationId} | {Bytes(evidence.AllocationBytes!.Value)} | PID {evidence.OwnerProcessId}" :
                            Text($"GpuEvidence{evidence.Status}")));
                }
            }
            return rows;
        }
    }

    private static string Hresult(int status) => $"HRESULT 0x{unchecked((uint)status):X8}";
    private string Bytes(ulong value) => string.Format(CultureInfo.CurrentCulture, Text("MemoryBytes"), value);

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
        Properties.Add(new("CoreAllocationBytes",
            node.CoreAllocationStatus >= 0 ? Bytes(node.CoreAllocationBytes) : Hresult(node.CoreAllocationStatus)));
        if (SelectedSnapshot is { } memorySnapshot)
        {
            var subtree = Enumerate([selected]).ToArray();
            var measured = subtree.Where(item => item.Node.CoreAllocationStatus >= 0).ToArray();
            Properties.Add(new("SubtreeCoreAllocationBytes",
                Bytes(measured.Aggregate(0UL, (total, item) => checked(total + item.Node.CoreAllocationBytes)))));
            Properties.Add(new("SubtreeUnmeasuredCoreCount",
                subtree.Count(item => item.Node.Tree == TreeKind.Xaml && item.Node.CoreAllocationStatus < 0)
                    .ToString(CultureInfo.CurrentCulture)));
            var thread = memorySnapshot.Document.Threads.Single(thread => thread.ThreadId == selected.ThreadId);
            var ids = subtree.Select(item => item.Node.Id).ToHashSet(StringComparer.Ordinal);
            var visuals = thread.Nodes.Where(item => item.Tree == TreeKind.Visual &&
                (node.Tree == TreeKind.Visual ? ids.Contains(item.Id) :
                    node.Tree == TreeKind.Xaml ? ids.Contains(item.XamlId) : ids.Contains(item.CompId)))
                .Select(item => item.Id).ToHashSet(StringComparer.Ordinal);
            int index = 0;
            foreach (var surface in thread.Surfaces.Where(surface => visuals.Contains(surface.VisualId)))
            {
                string prefix = $"Surface[{index++}]";
                Properties.Add(new(prefix, $"{surface.VisualId} | {surface.Role} | {surface.CompositionSurfaceId}"));
                Properties.Add(new($"{prefix}.Status", Hresult(surface.Status)));
                if (surface.Status < 0) { continue; }
                Properties.Add(new($"{prefix}.Owner/Generation", $"{surface.SurfaceId} / {surface.Generation}"));
                Properties.Add(new($"{prefix}.Size/Format/Virtual", $"{surface.Width} x {surface.Height} / {surface.Format} / {surface.IsVirtual}"));
                Properties.Add(new($"{prefix}.EstimatedBytes", Bytes(surface.EstimatedBytes)));
                Properties.Add(new($"{prefix}.TextureStatus", Hresult(surface.TextureStatus)));
                Properties.Add(new($"{prefix}.LastObservedQpc", surface.ObservedQpc.ToString(CultureInfo.InvariantCulture)));
                if (surface.TextureStatus < 0) { continue; }
                Properties.Add(new($"{prefix}.ID3D11Resource", surface.ResourceId));
                Properties.Add(new($"{prefix}.TextureSize/Format", $"{surface.TextureWidth} x {surface.TextureHeight} / {surface.TextureFormat}"));
                Properties.Add(new($"{prefix}.Mips/Array/Samples", $"{surface.MipLevels} / {surface.ArraySize} / {surface.SampleCount}"));
                Properties.Add(new($"{prefix}.Offset", $"{surface.OffsetX}, {surface.OffsetY}"));
            }
        }
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
        await StartGpuTraceAsync(null, beforeLaunch: true, token);
        uint pid = await Task.Run(() => TargetLauncher.LaunchExecutable(Executable, Arguments), token);
        ProcessId = pid.ToString(CultureInfo.InvariantCulture);
        Notify(nameof(ProcessId));
        await ConnectAsync(pid, token);
    });

    private Task LaunchPackageAsync() => RunAsync(async token =>
    {
        await StartGpuTraceAsync(null, beforeLaunch: true, token);
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
        await StartGpuTraceAsync(pid, beforeLaunch: false, token);
        await ConnectAsync(pid, token);
    });

    private async Task StartGpuTraceAsync(uint? pid, bool beforeLaunch, CancellationToken token)
    {
        if (EnableGpuTracing)
        {
            _gpuTrace = await GpuObservationSession.StartAsync(pid, beforeLaunch, token);
        }
    }

    private async Task ConnectAsync(uint pid, CancellationToken token)
    {
        Status = Text("Connecting");
        _session = await ProfilerSession.ConnectAsync(pid, TapPath, token);
        _gpuTrace?.SelectTarget(pid);
        Status = $"{Text("Connected")} {pid} | {_session.RuntimePath}";
        StartStatistics(_session);
    }

    private Task CaptureAsync() => RunAsync(async token =>
    {
        Status = Text("Capturing");
        try
        {
            var session = _session ?? throw new InvalidOperationException(Text("NotConnected"));
            var gpuTrace = _gpuTrace;
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
                    SnapshotMemorySummary.Build(document),
                    gpuTrace?.Freeze(document) ?? GpuTraceSnapshot.Disabled,
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
        catch
        {
            if (_session is null)
            {
                _gpuTrace?.Dispose();
                _gpuTrace = null;
            }
            throw;
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
        _gpuTrace?.Dispose();
        _gpuTrace = null;
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
        Notify(nameof(CanConfigureGpuTracing));
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
