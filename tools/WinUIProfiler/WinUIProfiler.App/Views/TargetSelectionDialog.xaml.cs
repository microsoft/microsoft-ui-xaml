using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using WinUIProfiler.App.ViewModels;

namespace WinUIProfiler.App.Views;

public sealed partial class TargetSelectionDialog : ContentDialog
{
    private readonly CancellationTokenSource lifetime = new();
    private Task loading = Task.CompletedTask;
    public TargetSelectionViewModel ViewModel { get; }

    public TargetSelectionDialog(Func<CancellationToken, Task<IReadOnlyList<TargetCandidate>>> discover)
    {
        ViewModel = new(discover);
        InitializeComponent();
    }

    private async void Dialog_Loaded(object sender, RoutedEventArgs args)
    {
        Search.Focus(FocusState.Programmatic);
        await RefreshAsync();
    }

    private async void Refresh_Click(object sender, RoutedEventArgs args) => await RefreshAsync();
    private Task RefreshAsync() => loading = ViewModel.RefreshAsync(lifetime.Token);
    private void Dialog_Closed(ContentDialog sender, ContentDialogClosedEventArgs args) => lifetime.Cancel();

    public async Task FinishAsync()
    {
        await lifetime.CancelAsync();
        await loading;
        lifetime.Dispose();
    }
}
