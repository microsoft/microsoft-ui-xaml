using System.Diagnostics;
using System.Globalization;
using System.Text.Json;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Automation;
using Microsoft.UI.Xaml.Controls;
using Windows.Graphics;
using WinUIProfiler.App.Services;
using WinUIProfiler.App.ViewModels;
using WinUIProfiler.Engine.Target;

namespace WinUIProfiler.App;

public sealed partial class MainWindow : Window
{
    private readonly LiveSessionViewModel model = new();
    private readonly Microsoft.Windows.ApplicationModel.Resources.ResourceLoader strings = new();
    private readonly DispatcherTimer refresh = new() { Interval = TimeSpan.FromMilliseconds(500) };
    private long? selectedCreationTime;
    private long connectionGeneration;
    private bool closing, closeAllowed, refreshing;

    public MainWindow()
    {
        InitializeComponent();
        Title = Text("AppTitle");
        AppWindow.Resize(new SizeInt32(1280, 1050));
        SetNames();
        refresh.Tick += (_, _) => Refresh();
        refresh.Start();
        AppWindow.Closing += async (_, args) =>
        {
            if (closeAllowed) return;
            args.Cancel = true;
            if (closing) return;
            closing = true;
            connectionGeneration++;
            refresh.Stop();
            await model.DisconnectAsync();
            closeAllowed = true;
            Close();
        };
        Refresh();
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
            (Mode, "ModeAutomation"), (TargetText, "TargetAutomation"), (Browse, "BrowseAutomation"),
            (ArgumentsText, "ArgumentsAutomation"), (WorkingDirectoryText, "DirectoryAutomation"),
            (Endpoint, "EndpointAutomation"), (Trust, "TrustAutomation"), (ConnectButton, "ConnectAutomation"),
            (DisconnectButton, "DisconnectAutomation"), (ConnectionStatus, "StatusAutomation"),
            (ConnectionIdentity, "IdentityAutomation")
        })
            AutomationProperties.SetName(pair.Element, Text(pair.Key));
    }

    private void Refresh()
    {
        if (closing || refreshing) return;
        refreshing = true;
        try
        {
            model.Refresh();
            var idle = model.CanConnect;
            foreach (var control in new Control[] { Mode, TargetText, Browse, Endpoint, Trust })
                control.IsEnabled = idle;
            ArgumentsText.IsEnabled = idle && Kind != TargetLaunchKind.Attach;
            WorkingDirectoryText.IsEnabled = idle && Kind == TargetLaunchKind.Unpackaged;
            ConnectButton.IsEnabled = idle;
            DisconnectButton.IsEnabled = model.CanDisconnect && !closing;
            DisconnectButton.Content = Text(model.IsBusy ? "CancelConnection" : "DisconnectLabel");
            var stateKey = "State" + model.State.Replace(" ", "", StringComparison.Ordinal);
            ConnectionStatus.Title = Text(stateKey);
            ConnectionStatus.Message = model.Detail;
            ConnectionStatus.Severity = model.State == "Failed" ? InfoBarSeverity.Error :
                model.State == "Connected" ? InfoBarSeverity.Success : InfoBarSeverity.Informational;
            AutomationProperties.SetItemStatus(ConnectionStatus, model.State);
            var connection = model.Connection;
            var scope = connection?.Scope;
            ConnectionIdentity.Text = scope is null ? "" : Format("IdentityFormat", scope.Process.ProcessId,
                scope.Process.CreationTimeFileTime, connection!.LastHelloUtc?.ToLocalTime().ToString("T", CultureInfo.CurrentCulture),
                connection.RuntimePath);
            AutomationProperties.SetItemStatus(ConnectionIdentity, JsonSerializer.Serialize(new
            {
                processId = scope?.Process.ProcessId, creationTimeFileTime = scope?.Process.CreationTimeFileTime,
                sessionId = scope?.SessionId, lastHelloUtc = connection?.LastHelloUtc,
                runtimePath = connection?.RuntimePath, samples = model.History?.TotalSamples ?? 0
            }));
            Error.IsOpen = model.ErrorMessage.Length > 0;
            Error.Message = model.ErrorMessage;
            UtilizationChart.ShowSeries(model.History, true, Stopwatch.Frequency);
            MemoryChart.ShowSeries(model.History, false, Stopwatch.Frequency);
        }
        finally { refreshing = false; }
    }

    private void Mode_SelectionChanged(object sender, SelectionChangedEventArgs args)
    {
        if (TargetText is null) return;
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
                throw new ArgumentException(Text("InvalidEndpoint"));
            var options = new TargetConnectionOptions(Kind, TargetText.Text.Trim(), ArgumentsText.Text,
                WorkingDirectoryText.Text, Path.Combine(AppContext.BaseDirectory, "ConnectionPayloads"),
                Trust.IsChecked == true, checked((ushort)Endpoint.Value), selectedCreationTime);
            var progress = new Progress<ConnectionProgress>(value => DispatcherQueue.TryEnqueue(() =>
            {
                if (closing || generation != connectionGeneration || !model.IsBusy) return;
                model.ReportProgress(value);
                Refresh();
            }));
            var pending = model.ConnectAsync(options, progress);
            Refresh();
            await pending;
        }
        catch (Exception error) { model.ReportError(error); }
        finally { connectionGeneration++; Refresh(); }
    }

    private async void Disconnect_Click(object sender, RoutedEventArgs args)
    {
        connectionGeneration++;
        DisconnectButton.IsEnabled = false;
        await model.DisconnectAsync();
        Refresh();
    }

    private async void Browse_Click(object sender, RoutedEventArgs args)
    {
        try
        {
            if (Kind == TargetLaunchKind.Unpackaged)
            {
                var path = ExecutablePicker.Pick(WinRT.Interop.WindowNative.GetWindowHandle(this));
                if (path is not null) TargetText.Text = path;
                return;
            }
            var selected = await PickTargetAsync(Kind);
            if (selected is not null)
            {
                TargetText.Text = selected.Value;
                selectedCreationTime = selected.CreationTimeFileTime;
            }
        }
        catch (Exception error) { model.ReportError(error); Refresh(); }
    }

    private async Task<TargetCandidate?> PickTargetAsync(TargetLaunchKind kind)
    {
        using var cancellation = new CancellationTokenSource();
        var picker = new TargetSelectionViewModel(kind == TargetLaunchKind.Packaged
            ? TargetDiscovery.PackagedAppsAsync : TargetDiscovery.ProcessesAsync);
        var search = new TextBox { PlaceholderText = Text("SearchTargets") };
        var refreshButton = new Button { Content = Text("RefreshTargets") };
        var error = new TextBlock { TextWrapping = TextWrapping.Wrap };
        var list = new ListView { Height = 350, SelectionMode = ListViewSelectionMode.Single,
            ItemTemplate = (DataTemplate)Root.Resources["TargetTemplate"] };
        AutomationProperties.SetName(search, Text("SearchTargets"));
        AutomationProperties.SetName(list, Text("TargetListAutomation"));
        var panel = new StackPanel { Spacing = 8 };
        foreach (var child in new UIElement[] { search, refreshButton, error, list }) panel.Children.Add(child);
        var dialog = new ContentDialog
        {
            XamlRoot = Root.XamlRoot, Title = Text(kind == TargetLaunchKind.Packaged ? "PickPackage" : "PickProcess"),
            PrimaryButtonText = Text("SelectTarget"), CloseButtonText = Text("CancelConnection"),
            IsPrimaryButtonEnabled = false, Content = panel
        };
        void Update()
        {
            if (!ReferenceEquals(list.ItemsSource, picker.Candidates)) list.ItemsSource = picker.Candidates;
            refreshButton.IsEnabled = picker.CanRefresh;
            dialog.IsPrimaryButtonEnabled = picker.CanSelect;
            error.Text = picker.IsBusy ? Text("LoadingTargets") : picker.HasError ? picker.ErrorMessage :
                picker.IsEmpty ? Text("NoTargets") : "";
        }
        picker.PropertyChanged += (_, _) => Update();
        search.TextChanged += (_, _) => picker.SearchText = search.Text;
        list.SelectionChanged += (_, _) => { picker.Selected = list.SelectedItem as TargetCandidate; Update(); };
        Task loading = picker.RefreshAsync(cancellation.Token);
        refreshButton.Click += async (_, _) => { loading = picker.RefreshAsync(cancellation.Token); await loading; };
        try
        {
            var result = await dialog.ShowAsync();
            return result == ContentDialogResult.Primary && picker.CanSelect ? picker.Selected : null;
        }
        finally { cancellation.Cancel(); await loading; }
    }
}
