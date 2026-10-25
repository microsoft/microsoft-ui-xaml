using System.Text.Json;
using Microsoft.Data.Sqlite;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using WinUIProfiler.App.ViewModels;
using WinUIProfiler.Contracts;

namespace WinUIProfiler.App.Views;

public sealed partial class SessionPage : Page
{
    private readonly CancellationTokenSource lifetime = new();
    public SavedSessionViewModel ViewModel { get; } = new();
    public Task StopLiveConnectionAsync() => LiveConnection.StopAsync();
    public void SetOwnerWindow(Window owner) => LiveConnection.SetOwnerWindow(owner);

    public SessionPage()
    {
        InitializeComponent();
        LiveConnection.ViewModel.Metrics.Changed += (_, _) => RefreshCharts();
        RefreshCharts();
        Loaded += OnLoaded;
        Unloaded += (_, _) => lifetime.Cancel();
    }

    private async void OnLoaded(object sender, RoutedEventArgs args)
    {
        Loaded -= OnLoaded;
        var arguments = Environment.GetCommandLineArgs();
        if (arguments.Length == 3 && arguments[1] == "--session")
        {
            Workspace.SelectedIndex = 1;
            SessionPath.Text = arguments[2];
            await ExecuteAsync(() => ViewModel.OpenAsync(SessionPath.Text, lifetime.Token));
        }

        else if (arguments.Length > 1)
            ViewModel.ReportError(new ArgumentException("Supported arguments: --session <saved-session.sqlite>"));
    }

    private void RefreshCharts()
    {
        var graphs = LiveConnection.ViewModel.Metrics;
        CpuChart.ShowSeries(graphs.Series.Single(series => series.Metric == MetricKind.CpuUtilization), graphs);
        GpuChart.ShowSeries(graphs.Series.Single(series => series.Metric == MetricKind.GpuUtilization), graphs);
        WorkingSetChart.ShowSeries(graphs.Series.Single(series => series.Metric == MetricKind.ProcessWorkingSet), graphs);
        PrivateWorkingSetChart.ShowSeries(graphs.Series.Single(series => series.Metric == MetricKind.ProcessPrivateWorkingSet), graphs);
        CommitChart.ShowSeries(graphs.Series.Single(series => series.Metric == MetricKind.ProcessPrivateCommit), graphs);
    }

    private void Workspace_SelectionChanged(object sender, SelectionChangedEventArgs args) =>
        LiveConnection.Visibility = ((TabView)sender).SelectedIndex == 0 ? Visibility.Visible : Visibility.Collapsed;

    private async void Open_Click(object sender, RoutedEventArgs args) =>
        await ExecuteAsync(() => ViewModel.OpenAsync(SessionPath.Text, lifetime.Token));
    private async void Refresh_Click(object sender, RoutedEventArgs args) =>
        await ExecuteAsync(() => ViewModel.RefreshAsync(lifetime.Token));
    private async void Previous_Click(object sender, RoutedEventArgs args) =>
        await ExecuteAsync(() => ViewModel.PreviousAsync(lifetime.Token));
    private async void Next_Click(object sender, RoutedEventArgs args) =>
        await ExecuteAsync(() => ViewModel.NextAsync(lifetime.Token));

    private async Task ExecuteAsync(Func<Task> operation)
    {
        try { await operation(); }
        catch (OperationCanceledException) when (lifetime.IsCancellationRequested) { }
        catch (Exception error) when (error is IOException or UnauthorizedAccessException or ArgumentException
            or InvalidOperationException or NotSupportedException or SqliteException or JsonException)
        { ViewModel.ReportError(error); }
    }
}
