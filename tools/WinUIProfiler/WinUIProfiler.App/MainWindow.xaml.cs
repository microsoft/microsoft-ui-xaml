using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Diagnostics;
using System.Globalization;
using System.Text.Json;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Automation;
using Microsoft.UI.Xaml.Controls;
using Windows.Graphics;
using WinUIProfiler.App.Services;
using WinUIProfiler.App.ViewModels;
using WinUIProfiler.App.Views;
using WinUIProfiler.Contracts;
using WinUIProfiler.Engine.Metrics;
using WinUIProfiler.Engine.Target;

namespace WinUIProfiler.App;

public sealed partial class MainWindow : Window
{
    private readonly LiveSessionViewModel model = new();
    private readonly Microsoft.Windows.ApplicationModel.Resources.ResourceLoader strings = new();
    private MetricHistory? renderedHistory;
    private long renderedSamples = -1;
    private int refreshQueued;
    private TargetCandidate? selectedTarget;
    private long connectionGeneration;
    private volatile bool closing;
    private bool closeAllowed;
    private bool refreshing;
    private CancellationTokenSource? snapshotOperation;
    private SavedTreeSnapshot? snapshot;
    private readonly Dictionary<Guid, SnapshotViewerWindow> snapshotWindows = [];
    public ObservableCollection<SnapshotHistoryEntry> Snapshots { get; } = [];
    public MainWindow()
    {
        InitializeComponent();
        Title = Text("AppTitle");
        AppWindow.Resize(new SizeInt32(1280, 1050));
        SetNames();
        model.PropertyChanged += Model_PropertyChanged;
        AppWindow.Closing += AppWindow_Closing;
        Refresh();
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
        connectionGeneration++;
        model.PropertyChanged -= Model_PropertyChanged;
        foreach (var viewer in snapshotWindows.Values.ToArray()) viewer.Close();
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
    private TargetLaunchKind Kind => Mode.SelectedIndex switch
    {
        0 => TargetLaunchKind.Unpackaged,
        1 => TargetLaunchKind.Packaged,
        _ => TargetLaunchKind.Attach
    };

    private void SetNames()
    {
        foreach (var pair in new (DependencyObject Element, string Key)[]
        {
            (Mode, "ModeAutomation"),
            (TargetText, "TargetAutomation"),
            (Browse, "BrowseAutomation"),
            (ArgumentsText, "ArgumentsAutomation"),
            (WorkingDirectoryText, "DirectoryAutomation"),
            (Endpoint, "EndpointAutomation"),
            (Trust, "TrustAutomation"),
            (ConnectButton, "ConnectAutomation"),
            (DisconnectButton, "DisconnectAutomation"),
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
            var idle = model.CanConnect;
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
            ConnectButton.IsEnabled = idle;
            DisconnectButton.IsEnabled = model.CanDisconnect && !closing;
            DisconnectButton.Content = Text(model.IsBusy ? "CancelConnection" : "DisconnectLabel");
            SnapshotButton.IsEnabled = model.State == "Connected" && !model.IsBusy && snapshotOperation is null;
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
            ConnectionStatus.Title = Text(stateKey);
            ConnectionStatus.Message = model.Detail;
            ConnectionStatus.Severity = model.State == "Failed" ? InfoBarSeverity.Error : model.State == "Connected" ? InfoBarSeverity.Success : InfoBarSeverity.Informational;
            AutomationProperties.SetItemStatus(ConnectionStatus, model.State);
            var connection = model.Connection;
            var scope = connection?.Scope;
            SetLine(ConnectionIdentity,
                scope is null ? "" : Format("IdentityFormat", scope.Process.ProcessId, Path.GetDirectoryName(connection!.RuntimePath)),
                scope is null ? null : Format("IdentityDetailsFormat",
                    scope.Process.CreationTimeFileTime is { } created ? DateTime.FromFileTime(created).ToString("G", CultureInfo.CurrentCulture) : null,
                    connection!.LastHelloUtc?.ToLocalTime().ToString("T", CultureInfo.CurrentCulture),
                    connection.RuntimePath));
            AutomationProperties.SetItemStatus(ConnectionIdentity,
JsonSerializer.Serialize(new
{
    processId = scope?.Process.ProcessId,
    creationTimeFileTime = scope?.Process.CreationTimeFileTime,
    sessionId = scope?.SessionId,
    lastHelloUtc = connection?.LastHelloUtc,
    runtimePath = connection?.RuntimePath,
    samples = model.History?.TotalSamples ?? 0
}));
            Error.IsOpen = model.ErrorMessage.Length > 0;
            Error.Message = model.ErrorMessage;
            var etw = model.Etw;
            if (model.EtwError is { } etwError)
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
        Refresh();
    }

    private void Target_TextChanged(object sender, TextChangedEventArgs args)
    {
        if (TargetText.Text.Trim() != selectedTarget?.Value)
        {
            selectedTarget = null;
        }
    }

    private async void Connect_Click(object sender, RoutedEventArgs args)
    {
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
Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "WinUIProfiler", "Sessions"));
            var progress = new Progress<ConnectionProgress>(value => DispatcherQueue.TryEnqueue(() =>
            {
                if (closing || generation != connectionGeneration || !model.IsBusy)
                {
                    return;
                }

                model.ReportProgress(value);
            }));
            await model.ConnectAsync(options, progress);
        }
        catch (Exception error)
        {
            model.ReportError(error);
        }
        finally
        {
            connectionGeneration++;
        }
    }

    private async void Disconnect_Click(object sender, RoutedEventArgs args)
    {
        snapshotOperation?.Cancel();
        connectionGeneration++;
        DisconnectButton.IsEnabled = false;
        await model.DisconnectAsync();
    }

    private async void Snapshot_Click(object sender, RoutedEventArgs args)
    {
        if (snapshotOperation is not null) return;
        using var cancellation = new CancellationTokenSource();
        snapshotOperation = cancellation;
        Refresh();
        try
        {
            var connection = model.Connection ?? throw new InvalidOperationException(Text("SnapshotRequiresConnection"));
            var process = connection.Scope?.Process ?? throw new InvalidOperationException(Text("SnapshotRequiresConnection"));
            var directory = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
                "WinUIProfiler", "Snapshots");
            snapshot = await connection.TakeSnapshotAsync(directory, cancellation.Token);
            var savedAt = DateTimeOffset.UtcNow;
            Snapshots.Insert(0, new(snapshot, process, savedAt,
                Format("SnapshotHistoryTitleFormat", savedAt.ToLocalTime().ToString("G", CultureInfo.CurrentCulture),
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
            }
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
