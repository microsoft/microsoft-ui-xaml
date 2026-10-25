using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using WinUIProfiler.App.Services;
using WinUIProfiler.App.ViewModels;
using WinUIProfiler.Engine.Target;

namespace WinUIProfiler.App.Views;

public sealed partial class LiveConnectionPanel : UserControl
{
    private readonly DispatcherTimer refresh = new() { Interval = TimeSpan.FromSeconds(1) };
    private IntPtr ownerHandle;
    private bool selecting;
    private bool stopped;
    private TargetCandidate? selectedProcess;
    private TargetSelectionDialog? selector;
    private readonly Dictionary<Guid, SnapshotViewerWindow> snapshotWindows = [];
    public LiveSessionViewModel ViewModel { get; } = new();
    public void SetOwnerWindow(Window owner) => ownerHandle = WinRT.Interop.WindowNative.GetWindowHandle(owner);

    public LiveConnectionPanel()
    {
        InitializeComponent();
        refresh.Tick += async (_, _) =>
        {
            ViewModel.Refresh();
            await ViewModel.RefreshMetricsAsync();
        };
        Loaded += (_, _) => refresh.Start();
        Unloaded += (_, _) => refresh.Stop();
    }

    private async void Browse_Click(object sender, RoutedEventArgs args)
    {
        if (selecting || stopped || !ViewModel.CanConnect) return;
        selecting = true;
        var kind = (TargetLaunchKind)Mode.SelectedIndex;
        // Lock the form across the native modal picker as well as the XAML dialog.
        Mode.IsEnabled = Target.IsEnabled = Arguments.IsEnabled = WorkingDirectory.IsEnabled = false;
        ((Button)sender).IsEnabled = false;
        try
        {
            string? value;
            TargetCandidate? choice = null;
            if (kind == TargetLaunchKind.Unpackaged)
            {
                value = ExecutablePicker.Pick(ownerHandle);
            }
            else
            {
                selector = new(kind == TargetLaunchKind.Packaged ? TargetDiscovery.PackagedAppsAsync : TargetDiscovery.ProcessesAsync)
                { XamlRoot = XamlRoot };
                try
                {
                    var result = await selector.ShowAsync();
                    if (result == ContentDialogResult.Primary && selector.ViewModel.CanSelect)
                        choice = selector.ViewModel.Selected;
                    value = choice?.Value;
                }
                finally { await selector.FinishAsync(); selector = null; }
            }
            if (!stopped && value is not null)
            {
                Target.Text = value;
                selectedProcess = choice?.Kind == TargetLaunchKind.Attach ? choice : null;
            }
        }
        catch (Exception error) { ViewModel.ReportError(error); }
        finally
        {
            selecting = false;
            Mode.IsEnabled = Target.IsEnabled = Arguments.IsEnabled = WorkingDirectory.IsEnabled =
                ((Button)sender).IsEnabled = !stopped && ViewModel.CanConnect;
        }
    }

    private void Target_TextChanged(object sender, TextChangedEventArgs args) => selectedProcess = null;
    private void Mode_SelectionChanged(object sender, SelectionChangedEventArgs args) => selectedProcess = null;

    private async void Connect_Click(object sender, RoutedEventArgs args)
    {
        if (selecting || stopped) return;
        try
        {
            if (!double.IsFinite(Endpoint.Value) || Endpoint.Value != Math.Truncate(Endpoint.Value))
                throw new ArgumentException("Diagnostics endpoint must be an integer between 1 and 65535.");
            var directory = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
                "WinUIProfiler", "Sessions");
            await ViewModel.ConnectAsync(new((TargetLaunchKind)Mode.SelectedIndex, Target.Text, Arguments.Text,
                WorkingDirectory.Text, directory, Path.Combine(AppContext.BaseDirectory, "ConnectionPayloads"),
                Trust.IsChecked == true, checked((ushort)Endpoint.Value), Detailed.IsChecked == true,
                selectedProcess?.CreationTimeFileTime));
        }
        catch (Exception error) { ViewModel.ReportError(error); }
    }

    private async void Disconnect_Click(object sender, RoutedEventArgs args) => await ViewModel.StopAsync();

    private async void Snapshot_Click(object sender, RoutedEventArgs args)
    {
        try { await ViewModel.TakeSnapshotAsync(); }
        catch (Exception error) { ViewModel.ReportError(error); }
    }

    private void SnapshotEntry_Click(object sender, ItemClickEventArgs args)
    {
        if (stopped || args.ClickedItem is not SnapshotHistoryEntry entry) return;
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
        catch (Exception error) { ViewModel.ReportError(error); }
    }

    public Task StopAsync()
    {
        stopped = true;
        refresh.Stop();
        selector?.Hide();
        foreach (var viewer in snapshotWindows.Values.ToArray()) viewer.Close();
        return ViewModel.StopAsync();
    }
}
