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
    private long? selectedCreationTime;
    private long connectionGeneration;
    private volatile bool closing;
    private bool closeAllowed;
    private bool refreshing;
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
        connectionGeneration++;
        model.PropertyChanged -= Model_PropertyChanged;
        await model.DisconnectAsync();
        closeAllowed = true;
        Close();
    }

    private string Text(string key) => strings.GetString(key);
    private string Format(string key, params object?[] values) => string.Format(CultureInfo.CurrentCulture, Text(key), values);
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
            (ConnectionStatus, "StatusAutomation"),
            (ConnectionIdentity, "IdentityAutomation")
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
            var stateKey = "State" + model.State.Replace(" ", "", StringComparison.Ordinal);
            ConnectionStatus.Title = Text(stateKey);
            ConnectionStatus.Message = model.Detail;
            ConnectionStatus.Severity = model.State == "Failed" ? InfoBarSeverity.Error : model.State == "Connected" ? InfoBarSeverity.Success : InfoBarSeverity.Informational;
            AutomationProperties.SetItemStatus(ConnectionStatus, model.State);
            var connection = model.Connection;
            var scope = connection?.Scope;
            ConnectionIdentity.Text = scope is null ? "" : Format("IdentityFormat",
scope.Process.ProcessId,
scope.Process.CreationTimeFileTime,
connection!.LastHelloUtc?.ToLocalTime().ToString("T", CultureInfo.CurrentCulture),
connection.RuntimePath);
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
        selectedCreationTime = null;
        Refresh();
    }

    private void Target_TextChanged(object sender, TextChangedEventArgs args) => selectedCreationTime = null;
    private async void Connect_Click(object sender, RoutedEventArgs args)
    {
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
selectedCreationTime);
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
        connectionGeneration++;
        DisconnectButton.IsEnabled = false;
        await model.DisconnectAsync();
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
                TargetText.Text = selected.Value;
                selectedCreationTime = selected.CreationTimeFileTime;
            }
        }
        catch (Exception error)
        {
            model.ReportError(error);
        }
    }
}
