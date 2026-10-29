using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Diagnostics;
using System.Globalization;
using System.Text.Json;
using Microsoft.UI.Windowing;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Automation;
using Microsoft.UI.Xaml.Controls;
using Windows.Graphics;
using WinUIProfiler.App.Services;
using WinUIProfiler.App.ViewModels;
using WinUIProfiler.App.Views;
using WinUIProfiler.Contracts;
using WinUIProfiler.Engine.Ingestion;
using WinUIProfiler.Engine.Metrics;
using WinUIProfiler.Engine.Target;

namespace WinUIProfiler.App;

public sealed partial class MainWindow : Window
{
    private const int TraceModeIndex = 3;
    private static readonly TimeSpan TraceScanDelay = TimeSpan.FromMilliseconds(400);
    private readonly LiveSessionViewModel model = new();
    private readonly SessionSource session = new();
    private readonly Microsoft.Windows.ApplicationModel.Resources.ResourceLoader strings = new();
    private MetricHistory? renderedHistory;
    private long renderedSamples = -1;
    private int refreshQueued;
    private TargetCandidate? selectedTarget;
    private long connectionGeneration;
    private volatile bool closing;
    private bool closeAllowed;
    private bool refreshing;
    private bool? settingsFoldedFor;
    private bool initialSnapshotPending;
    private CancellationTokenSource? snapshotOperation;
    // The trace named in the target box, once its processes are listed; the list's items are traceProcesses.
    private CancellationTokenSource? traceScan;
    private string? scannedTrace;
    private IReadOnlyList<TraceProcess> traceProcesses = [];
    private SavedTreeSnapshot? snapshot;
    private readonly Dictionary<Guid, SnapshotViewerWindow> snapshotWindows = [];
    private readonly Dictionary<(Guid Before, Guid After), SnapshotDiffWindow> snapshotDiffWindows = [];
    public ObservableCollection<SnapshotHistoryEntry> Snapshots { get; } = [];
    public MainWindow()
    {
        InitializeComponent();
        Title = Text("AppTitle");
        AppWindow.Resize(new SizeInt32(1280, 1050));
        Root.Loaded += SizeToDisplay;
        SetNames();
        ApplyMode();
        model.PropertyChanged += Model_PropertyChanged;
        AppWindow.Closing += AppWindow_Closing;
        FramesPanel.Owner = AppWindow.Id;
        ElementsPanel.Owner = AppWindow.Id;
        FramesPanel.Source = session;
        ElementsPanel.Source = session;
        FramesPanel.ElementRequested += (_, objectId) =>
        {
            Tabs.SelectedItem = ElementsTab;
            ElementsPanel.ShowElement(objectId);
        };
        UpdateTargetSummary();
        Refresh();
    }

    // Resize takes physical pixels, so at the display's scaling the window is 1280 x 1050, within the work area.
    private void SizeToDisplay(object sender, RoutedEventArgs args)
    {
        Root.Loaded -= SizeToDisplay;
        var scale = Root.XamlRoot.RasterizationScale;
        var area = DisplayArea.GetFromWindowId(AppWindow.Id, DisplayAreaFallback.Nearest).WorkArea;
        var width = Math.Min((int)(1280 * scale), area.Width);
        var height = Math.Min((int)(1050 * scale), area.Height);
        var position = AppWindow.Position;
        AppWindow.MoveAndResize(new RectInt32(Math.Clamp(position.X, area.X, area.X + area.Width - width),
            Math.Clamp(position.Y, area.Y, area.Y + area.Height - height), width, height));
    }

    private async void AppWindow_Closing(Microsoft.UI.Windowing.AppWindow sender, Microsoft.UI.Windowing.AppWindowClosingEventArgs args)
    {
        if (closeAllowed)
        {
            return;
        }

        args.Cancel = true;
        if (closing)
        {
            return;
        }

        closing = true;
        snapshotOperation?.Cancel();
        traceScan?.Cancel();
        connectionGeneration++;
        model.PropertyChanged -= Model_PropertyChanged;
        foreach (var viewer in snapshotWindows.Values.ToArray()) viewer.Close();
        foreach (var viewer in snapshotDiffWindows.Values.ToArray()) viewer.Close();
        await model.DisconnectAsync();
        closeAllowed = true;
        Close();
    }

    private string Text(string key) => strings.GetString(key);
    private string Format(string key, params object?[] values) => string.Format(CultureInfo.CurrentCulture, Text(key), values);

    // Status lines stay on one line; the tooltip holds the full details.
    private static void SetLine(TextBlock line, string text, string? details)
    {
        if (line.Text != text)
        {
            line.Text = text;
        }

        if (!Equals(ToolTipService.GetToolTip(line), details))
        {
            ToolTipService.SetToolTip(line, details);
        }
    }
    // Not used in trace mode.
    private TargetLaunchKind Kind => Mode.SelectedIndex switch
    {
        0 => TargetLaunchKind.Unpackaged,
        1 => TargetLaunchKind.Packaged,
        _ => TargetLaunchKind.Attach
    };

    // In trace mode the target box holds a recorded trace's path, and the process to read is chosen from its list.
    private bool TraceMode => Mode.SelectedIndex == TraceModeIndex;

    private static string SessionsDirectory =>
        Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "WinUIProfiler", "Sessions");

    private TraceProcess? SelectedTraceProcess
    {
        get
        {
            var index = TraceProcessList.SelectedIndex;
            return scannedTrace is not null && index >= 0 && index < traceProcesses.Count ? traceProcesses[index] : null;
        }
    }

    private void SetNames()
    {
        foreach (var pair in new (DependencyObject Element, string Key)[]
        {
            (Mode, "ModeAutomation"),
            (ArgumentsText, "ArgumentsAutomation"),
            (WorkingDirectoryText, "DirectoryAutomation"),
            (Endpoint, "EndpointAutomation"),
            (Trust, "TrustAutomation"),
            (TraceProcessList, "TraceProcessAutomation"),
            (TraceProgress, "TraceProgressAutomation"),
            (SnapshotButton, "SnapshotAutomation"),
            (SnapshotStatus, "SnapshotStatusAutomation"),
            (ConnectionStatus, "StatusAutomation"),
            (ConnectionIdentity, "IdentityAutomation"),
            (EtwSummary, "EtwAutomation"),
            (RecordingSummary, "RecordingAutomation")
        })
        {
            AutomationProperties.SetName(pair.Element, Text(pair.Key));
        }
    }

    // Shows the settings, labels and names of the selected mode.
    private void ApplyMode()
    {
        var trace = TraceMode;
        TargetText.Header = Text(trace ? "TraceTargetHeader" : "TargetHeader");
        ConnectButton.Content = Text(trace ? "OpenTraceLabel" : "ConnectLabel");
        foreach (var pair in new (DependencyObject Element, string Key, string TraceKey)[]
        {
            (TargetText, "TargetAutomation", "TraceTargetAutomation"),
            (Browse, "BrowseAutomation", "TraceBrowseAutomation"),
            (ConnectButton, "ConnectAutomation", "OpenTraceAutomation"),
            (DisconnectButton, "DisconnectAutomation", "StopTraceAutomation")
        })
        {
            AutomationProperties.SetName(pair.Element, Text(trace ? pair.TraceKey : pair.Key));
        }

        LaunchOptions.Visibility = trace ? Visibility.Collapsed : Visibility.Visible;
        Trust.Visibility = trace ? Visibility.Collapsed : Visibility.Visible;
        TraceOptions.Visibility = trace ? Visibility.Visible : Visibility.Collapsed;
    }

    private void Model_PropertyChanged(object? sender, PropertyChangedEventArgs args)
    {
        if (closing || Interlocked.Exchange(ref refreshQueued, 1) != 0)
        {
            return;
        }

        if (!DispatcherQueue.TryEnqueue(RefreshQueued))
        {
            Interlocked.Exchange(ref refreshQueued, 0);
            if (!closing)
            {
                Trace.TraceError("The UI dispatcher rejected a profiler state-change notification.");
            }
        }
    }

    private void RefreshQueued()
    {
        // Updates arriving during rendering must be able to schedule another pass.
        Interlocked.Exchange(ref refreshQueued, 0);
        Refresh();
    }

    private void Refresh()
    {
        if (closing || refreshing)
        {
            return;
        }

        refreshing = true;
        try
        {
            var idle = model.CanConnect && !initialSnapshotPending && snapshotOperation is null;
            if (settingsFoldedFor != model.CanConnect)
            {
                // The settings can't change while connecting or connected, so they fold away to give the tabs room.
                TargetSettings.IsExpanded = model.CanConnect;
                settingsFoldedFor = model.CanConnect;
            }

            foreach (var control in new Control[]
            {
                Mode,
                TargetText,
                Browse,
                Endpoint,
                Trust
            })
            {
                control.IsEnabled = idle;
            }

            ArgumentsText.IsEnabled = idle && Kind != TargetLaunchKind.Attach;
            WorkingDirectoryText.IsEnabled = idle && Kind == TargetLaunchKind.Unpackaged;
            TraceProcessList.IsEnabled = idle && traceProcesses.Count > 0;
            ConnectButton.IsEnabled = idle && (!TraceMode || SelectedTraceProcess is not null);
            DisconnectButton.IsEnabled = model.CanDisconnect && !closing;
            DisconnectButton.Content = Text(TraceMode ? "StopTraceLabel" : model.IsBusy ? "CancelConnection" : "DisconnectLabel");
            SnapshotButton.IsEnabled = model.State == "Connected" && !model.IsBusy &&
                !initialSnapshotPending && snapshotOperation is null;
            CompareSnapshotsButton.IsEnabled = Snapshots.Count >= 2;
            SnapshotStatus.Text = snapshotOperation is not null ? Text("SnapshotCapturing") :
                snapshot is null ? "" : Format("SnapshotSavedFormat", snapshot.XamlNodes, snapshot.CompositionNodes, snapshot.Path);
            AutomationProperties.SetItemStatus(SnapshotStatus, JsonSerializer.Serialize(new
            {
                busy = snapshotOperation is not null,
                captureId = snapshot?.CaptureId,
                path = snapshot?.Path,
                roots = snapshot?.Roots,
                xamlNodes = snapshot?.XamlNodes,
                compositionNodes = snapshot?.CompositionNodes
            }));
            var stateKey = "State" + model.State.Replace(" ", "", StringComparison.Ordinal);
            var trace = model.TracePath is null ? null : model.TraceStatus;
            ConnectionStatus.Title = Text(stateKey);
            ConnectionStatus.Message = model.TracePath is null ? model.Detail : TraceMessage(trace, model.Recording?.CompleteFromSeconds);
            ConnectionStatus.Severity = model.State is "Failed" or "TraceFailed" ? InfoBarSeverity.Error :
                model.State is "Connected" or "TraceFinished" ? InfoBarSeverity.Success : InfoBarSeverity.Informational;
            AutomationProperties.SetItemStatus(ConnectionStatus, model.State);
            var reading = model.State is "TraceOpening" or "TraceReading" or "TraceStopping";
            TraceProgress.Visibility = reading ? Visibility.Visible : Visibility.Collapsed;
            TraceProgress.IsIndeterminate = reading && (model.State != "TraceReading" || trace?.Progress is null);
            TraceProgress.Value = trace?.Progress ?? 0;
            if (model.TracePath is { } tracePath && model.TraceTarget is { } target)
            {
                SetLine(ConnectionIdentity, target.Name is { } name
                        ? Format("TraceIdentityFormat", name, target.ProcessId, Path.GetFileName(tracePath))
                        : Format("TraceUnnamedIdentityFormat", target.ProcessId, Path.GetFileName(tracePath)),
                    tracePath);
                AutomationProperties.SetItemStatus(ConnectionIdentity, JsonSerializer.Serialize(new
                {
                    processId = target.ProcessId,
                    processName = target.Name,
                    etlPath = tracePath
                }));
            }
            else
            {
                var connection = model.Connection;
                var scope = connection?.Scope;
                SetLine(ConnectionIdentity,
                    scope is null ? "" : Format("IdentityFormat", scope.Process.ProcessId, Path.GetDirectoryName(connection!.RuntimePath)),
                    scope is null ? null : Format("IdentityDetailsFormat",
                        scope.Process.CreationTimeFileTime is { } created ? DateTime.FromFileTime(created).ToString("G", CultureInfo.CurrentCulture) : null,
                        connection!.LastHelloUtc?.ToLocalTime().ToString("T", CultureInfo.CurrentCulture),
                        connection.RuntimePath));
                AutomationProperties.SetItemStatus(ConnectionIdentity, JsonSerializer.Serialize(new
                {
                    processId = scope?.Process.ProcessId,
                    creationTimeFileTime = scope?.Process.CreationTimeFileTime,
                    sessionId = scope?.SessionId,
                    lastHelloUtc = connection?.LastHelloUtc,
                    runtimePath = connection?.RuntimePath,
                    samples = model.History?.TotalSamples ?? 0
                }));
            }

            Error.IsOpen = model.ErrorMessage.Length > 0;
            Error.Message = model.ErrorMessage;
            var etw = model.Etw;
            if (trace is not null)
            {
                // Nothing read from a trace is dropped: the reader waits while the pipeline is full.
                var counts = Format("EtwFormat", Text("EtwFromTrace"), trace.XamlEvents, trace.ProfilerEvents,
                    trace.GpuMemoryEvents, trace.LostEvents, 0);
                SetLine(EtwSummary,
                    trace.OutOfOrderEvents == 0 ? counts : counts + Format("TraceOutOfOrderFormat", trace.OutOfOrderEvents),
                    Format("EtwDetailsFormat", Text("EtwCoverageTrace")));
            }
            else if (model.EtwError is { } etwError)
            {
                var message = Format("EtwUnavailable", etwError);
                SetLine(EtwSummary, message, message);
            }
            else
            {
                SetLine(EtwSummary,
                    etw is null ? "" : Format("EtwFormat", Text(etw.FromProcessStart ? "EtwFromProcessStart" : "EtwFromConnection"),
                        etw.XamlEvents, etw.ProfilerEvents, etw.GpuMemoryEvents, etw.LostEvents, etw.DroppedEvents),
                    etw is null ? null : Format("EtwDetailsFormat", Text(etw.FromProcessStart ? "EtwCoverageProcessStart" : "EtwCoverageConnection")));
            }

            var recording = model.Recording;
            if (model.RecordingError is { } recordingError)
            {
                SetLine(RecordingSummary, recordingError, recordingError);
            }
            else
            {
                SetLine(RecordingSummary,
                    recording is null ? "" : Format(recording.CompleteFromSeconds is null ? "RecordingFormat" : "RecordingTrimmedFormat",
                        Path.GetFileName(recording.Path), recording.StoredEvents, recording.FileBytes / 1024.0 / 1024,
                        recording.LimitBytes / 1024 / 1024, recording.CompleteFromSeconds ?? 0),
                    recording is null ? null : Format("RecordingDetailsFormat", recording.Path, recording.LimitBytes / 1024 / 1024));
            }

            // A trace's session file grows while it is read, so the tabs follow it as they follow a connection's.
            session.ShowRecording(recording?.Path, model.State is "Connected" or "TraceReading" or "TraceStopping");
            TraceMetricsNote.Visibility = model.TracePath is null ? Visibility.Collapsed : Visibility.Visible;
            var history = model.History;
            var samples = history?.TotalSamples ?? 0;
            if (!ReferenceEquals(history, renderedHistory) || samples != renderedSamples)
            {
                UtilizationChart.ShowSeries(history, true, Stopwatch.Frequency);
                MemoryChart.ShowSeries(history, false, Stopwatch.Frequency);
                renderedHistory = history;
                renderedSamples = samples;
            }
        }
        finally
        {
            refreshing = false;
        }
    }

    private void Mode_SelectionChanged(object sender, SelectionChangedEventArgs args)
    {
        if (TargetText is null)
        {
            return;
        }

        TargetText.Text = "";
        selectedTarget = null;
        ApplyMode();
        // TextChanged is raised later, so the previous trace's processes are cleared now.
        ScanTrace();
    }

    private void Target_TextChanged(object sender, TextChangedEventArgs args)
    {
        if (TargetText.Text.Trim() != selectedTarget?.Value)
        {
            selectedTarget = null;
        }

        if (TraceMode)
        {
            ScanTrace();
            return;
        }

        UpdateTargetSummary();
    }

    private void Arguments_TextChanged(object sender, TextChangedEventArgs args) => UpdateTargetSummary();

    // Lists the processes with XAML events in the trace named in the target box, a moment after its last edit. Another
    // edit, a mode change or closing the window cancels the listing.
    private async void ScanTrace()
    {
        traceScan?.Cancel();
        traceScan = null;
        scannedTrace = null;
        traceProcesses = [];
        TraceProcessList.ItemsSource = null;
        TraceScanProgress.IsActive = false;
        TraceScanStatus.Text = "";
        var path = TargetText.Text.Trim().Trim('"');
        if (closing || !TraceMode || path.Length == 0)
        {
            UpdateTargetSummary();
            Refresh();
            return;
        }

        using var cancellation = new CancellationTokenSource();
        traceScan = cancellation;
        TraceScanProgress.IsActive = true;
        TraceScanStatus.Text = Text("TraceScanning");
        UpdateTargetSummary();
        Refresh();
        try
        {
            await Task.Delay(TraceScanDelay, cancellation.Token);
            var fullPath = Path.GetFullPath(path);
            var contents = await Task.Run(() => OfflineEtlSource.ListProcesses(fullPath, cancellation.Token), cancellation.Token);
            cancellation.Token.ThrowIfCancellationRequested();
            scannedTrace = fullPath;
            traceProcesses = contents.Processes;
            TraceProcessList.ItemsSource = traceProcesses.Select(ProcessLabel).ToList();
            TraceProcessList.SelectedIndex = traceProcesses.Count == 1 ? 0 : -1;
            TraceScanStatus.Text = traceProcesses.Count == 0 ? Text("TraceScanEmpty") :
                Format("TraceContentsFormat", traceProcesses.Count, contents.Duration.TotalSeconds, contents.EventsLost);
        }
        catch (Exception) when (cancellation.IsCancellationRequested)
        {
        }
        catch (Exception error)
        {
            Trace.TraceError(error.ToString());
            TraceScanStatus.Text = Format("TraceScanFailedFormat", ScanFailure(error));
        }
        finally
        {
            if (ReferenceEquals(traceScan, cancellation))
            {
                traceScan = null;
                TraceScanProgress.IsActive = false;
                UpdateTargetSummary();
                Refresh();
            }
        }
    }

    private string ProcessLabel(TraceProcess process) => process.Name is { } name
        ? Format("TraceProcessFormat", name, process.ProcessId, process.Events, process.FirstMilliseconds / 1000,
            process.LastMilliseconds / 1000)
        : Format("TraceUnnamedProcessFormat", process.ProcessId, process.Events, process.FirstMilliseconds / 1000,
            process.LastMilliseconds / 1000);

    // ETW reports a file that is not a trace, or is damaged, as ERROR_FILE_CORRUPT.
    private string ScanFailure(Exception error) => error switch
    {
        FileNotFoundException or DirectoryNotFoundException => Text("TraceNotFound"),
        BadImageFormatException => Text("TraceUnreadable"),
        _ when error.HResult == unchecked((int)0x80070570) => Text("TraceUnreadable"),
        _ => error.Message
    };

    private void TraceProcess_SelectionChanged(object sender, SelectionChangedEventArgs args)
    {
        UpdateTargetSummary();
        Refresh();
    }

    // trimmedBefore is set once the session file's size limit removed the events before that time.
    private string TraceMessage(TraceReplayStatus? status, double? trimmedBefore) => status switch
    {
        { State: TraceReplayState.Reading, Progress: { } read } => Format("TraceReadingFormat", read),
        { State: TraceReplayState.Reading } => Text("TraceReading"),
        { State: TraceReplayState.Stopping } => Text("TraceStopping"),
        { State: TraceReplayState.Finished } => trimmedBefore is { } seconds
            ? Format("TraceFinishedTrimmedFormat", seconds) : Text("TraceFinished"),
        { State: TraceReplayState.Stopped, Progress: { } read } => trimmedBefore is { } seconds
            ? Format("TraceStoppedTrimmedFormat", read, seconds) : Format("TraceStoppedFormat", read),
        { State: TraceReplayState.Stopped } => trimmedBefore is { } seconds
            ? Format("TraceStoppedTrimmed", seconds) : Text("TraceStopped"),
        { State: TraceReplayState.Failed, Progress: { } read } => Format("TraceFailedFormat", read),
        { State: TraceReplayState.Failed } => Text("TraceFailed"),
        // Opening, or opening failed; the error bar says why.
        _ => ""
    };

    // The settings' header names the target, so it is still shown when they are folded away.
    private void UpdateTargetSummary()
    {
        var target = TargetText.Text.Trim();
        TargetSummary.Text = target.Length == 0 ? Text("TargetSettingsHeader")
            : !TraceMode ? Format("TargetSummaryFormat", Path.GetFileName(target), ArgumentsText.Text.Trim())
            : SelectedTraceProcess is { } process ? Format("TraceSummaryProcessFormat", Path.GetFileName(target.Trim('"')), process.ProcessId)
            : Format("TraceSummaryFormat", Path.GetFileName(target.Trim('"')));
    }

    private async void Connect_Click(object sender, RoutedEventArgs args)
    {
        if (!model.CanConnect || initialSnapshotPending || snapshotOperation is not null) return;
        if (TraceMode)
        {
            await OpenTraceAsync();
            return;
        }

        initialSnapshotPending = true;
        snapshot = null;
        var generation = ++connectionGeneration;
        try
        {
            if (!double.IsFinite(Endpoint.Value) || Endpoint.Value != Math.Truncate(Endpoint.Value))
            {
                throw new ArgumentException(Text("InvalidEndpoint"));
            }

            var options = new TargetConnectionOptions(Kind,
TargetText.Text.Trim(),
ArgumentsText.Text,
WorkingDirectoryText.Text,
Path.Combine(AppContext.BaseDirectory, "ConnectionPayloads"),
Trust.IsChecked == true,
checked((ushort)Endpoint.Value),
selectedTarget?.CreationTimeFileTime,
SessionsDirectory);
            var progress = new Progress<ConnectionProgress>(value => DispatcherQueue.TryEnqueue(() =>
            {
                if (closing || generation != connectionGeneration || !model.IsBusy)
                {
                    return;
                }

                model.ReportProgress(value);
            }));
            await model.ConnectAsync(options, progress);
            if (!closing && generation == connectionGeneration && model.Connection?.State == "Connected")
            {
                await TakeSnapshotAsync(isInitial: true);
            }
        }
        catch (Exception error)
        {
            model.ReportError(error);
        }
        finally
        {
            initialSnapshotPending = false;
            connectionGeneration++;
            Refresh();
        }
    }

    // Reads the chosen process's events from the listed trace into a new session file. There is no running process, so
    // there are no snapshots or live metrics; the Frames tab shows the events as they are stored.
    private async Task OpenTraceAsync()
    {
        if (scannedTrace is not { } path || SelectedTraceProcess is not { } process) return;
        snapshot = null;
        Tabs.SelectedItem = FramesTab;
        try
        {
            await model.OpenTraceAsync(path, process, SessionsDirectory);
        }
        catch (Exception error)
        {
            model.ReportError(error);
        }
        finally
        {
            Refresh();
        }
    }

    private async void Disconnect_Click(object sender, RoutedEventArgs args)
    {
        snapshotOperation?.Cancel();
        connectionGeneration++;
        DisconnectButton.IsEnabled = false;
        await model.DisconnectAsync();
    }

    private async void Snapshot_Click(object sender, RoutedEventArgs args) => await TakeSnapshotAsync();

    private async Task TakeSnapshotAsync(bool isInitial = false)
    {
        if (closing || snapshotOperation is not null || !isInitial && initialSnapshotPending) return;
        using var cancellation = new CancellationTokenSource();
        snapshotOperation = cancellation;
        Refresh();
        try
        {
            var connection = model.Connection ?? throw new InvalidOperationException(Text("SnapshotRequiresConnection"));
            var process = connection.Scope?.Process ?? throw new InvalidOperationException(Text("SnapshotRequiresConnection"));
            var directory = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
                "WinUIProfiler", "Snapshots");
            var saved = await connection.TakeSnapshotAsync(directory, cancellation.Token);
            cancellation.Token.ThrowIfCancellationRequested();
            if (closing || !ReferenceEquals(connection, model.Connection)) return;
            snapshot = saved;
            var savedAt = DateTimeOffset.UtcNow;
            Snapshots.Insert(0, new(snapshot, process, savedAt,
                Format(isInitial ? "InitialSnapshotHistoryTitleFormat" : "SnapshotHistoryTitleFormat",
                    savedAt.ToLocalTime().ToString("G", CultureInfo.CurrentCulture),
                    process.ProcessId),
                Format("SnapshotHistorySummaryFormat", snapshot.XamlNodes, snapshot.CompositionNodes, snapshot.Correlations)));
        }
        catch (OperationCanceledException) when (cancellation.IsCancellationRequested) { }
        catch (Exception error)
        {
            model.ReportError(error);
        }
        finally
        {
            snapshotOperation = null;
            Refresh();
        }
    }

    private void SnapshotEntry_Click(object sender, ItemClickEventArgs args)
    {
        if (closing || args.ClickedItem is not SnapshotHistoryEntry entry) return;
        try
        {
            if (!snapshotWindows.TryGetValue(entry.Saved.CaptureId, out var viewer))
            {
                viewer = new(entry);
                snapshotWindows.Add(entry.Saved.CaptureId, viewer);
                viewer.Closed += (_, _) => snapshotWindows.Remove(entry.Saved.CaptureId);
                viewer.ElementRequested += ShowSnapshotElement;
            }
            viewer.Activate();
        }
        catch (Exception error)
        {
            model.ReportError(error);
        }
    }

    // A snapshot window asked for a XAML node's element: show its history in the Elements tab.
    private void ShowSnapshotElement(object? sender, SnapshotElement node)
    {
        if (closing) return;
        Tabs.SelectedItem = ElementsTab;
        ElementsPanel.ShowSnapshotElement(node);
        Activate();
    }

    private void SnapshotDiffPicker_Opening(object sender, object args)
    {
        SnapshotDiffBefore.SelectedIndex = Snapshots.Count >= 2 ? 1 : -1;
        SnapshotDiffAfter.SelectedIndex = Snapshots.Count > 0 ? 0 : -1;
    }

    private void SnapshotDiffSelection_Changed(object sender, SelectionChangedEventArgs args)
    {
        if (CompareSelectedSnapshots is null) return;
        CompareSelectedSnapshots.IsEnabled = SnapshotDiffBefore.SelectedItem is SnapshotHistoryEntry before &&
            SnapshotDiffAfter.SelectedItem is SnapshotHistoryEntry after && before.CaptureId != after.CaptureId &&
            SnapshotDiffData.CanCompare(before, after);
    }

    private void CompareSnapshots_Click(object sender, RoutedEventArgs args)
    {
        if (closing || SnapshotDiffBefore.SelectedItem is not SnapshotHistoryEntry before ||
            SnapshotDiffAfter.SelectedItem is not SnapshotHistoryEntry after) return;
        try
        {
            var key = (before.Saved.CaptureId, after.Saved.CaptureId);
            if (!snapshotDiffWindows.TryGetValue(key, out var viewer))
            {
                viewer = new(before, after);
                snapshotDiffWindows.Add(key, viewer);
                viewer.Closed += (_, _) => snapshotDiffWindows.Remove(key);
                viewer.ElementRequested += ShowSnapshotElement;
            }
            SnapshotDiffPicker.Hide();
            viewer.Activate();
        }
        catch (Exception error)
        {
            model.ReportError(error);
        }
    }

    private async void Browse_Click(object sender, RoutedEventArgs args)
    {
        try
        {
            if (TraceMode)
            {
                if (await TracePicker.PickAsync(AppWindow.Id) is { } trace)
                {
                    TargetText.Text = trace;
                }

                return;
            }

            if (Kind == TargetLaunchKind.Unpackaged)
            {
                var path = await ExecutablePicker.PickAsync(AppWindow.Id);
                if (path is not null)
                {
                    TargetText.Text = path;
                }

                return;
            }

            var dialog = new TargetPickerDialog(Kind)
            {
                XamlRoot = Root.XamlRoot
            };
            var selected = await dialog.PickAsync();
            if (selected is not null)
            {
                selectedTarget = selected;
                TargetText.Text = selected.Value;
            }
        }
        catch (Exception error)
        {
            model.ReportError(error);
        }
    }
}
