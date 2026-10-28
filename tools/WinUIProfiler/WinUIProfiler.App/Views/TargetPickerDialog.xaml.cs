using System.ComponentModel;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using WinUIProfiler.App.Services;
using WinUIProfiler.App.ViewModels;
using WinUIProfiler.Engine.Target;

namespace WinUIProfiler.App.Views;

public sealed partial class TargetPickerDialog : ContentDialog
{
    private readonly Microsoft.Windows.ApplicationModel.Resources.ResourceLoader strings = new();
    private readonly CancellationTokenSource cancellation = new();
    private Task loading = Task.CompletedTask;
    public TargetSelectionViewModel Model
    {
        get;
    }

    public TargetPickerDialog(TargetLaunchKind kind)
    {
        Model = new(kind == TargetLaunchKind.Packaged ? TargetDiscovery.PackagedAppsAsync : TargetDiscovery.ProcessesAsync);
        InitializeComponent();
        Title = Text(kind == TargetLaunchKind.Packaged ? "PickPackage" : "PickProcess");
        PrimaryButtonText = Text("SelectTarget");
        CloseButtonText = Text("CancelConnection");
        Model.PropertyChanged += Model_PropertyChanged;
    }

    private string Text(string key) => strings.GetString(key);
    public async Task<TargetCandidate?> PickAsync()
    {
        loading = Model.RefreshAsync(cancellation.Token);
        try
        {
            var result = await ShowAsync();
            return result == ContentDialogResult.Primary && Model.CanSelect ? Model.Selected : null;
        }
        finally
        {
            cancellation.Cancel();
            await loading;
            Model.PropertyChanged -= Model_PropertyChanged;
            cancellation.Dispose();
        }
    }

    private void Model_PropertyChanged(object? sender, PropertyChangedEventArgs args)
    {
        Status.Text = Model.IsBusy ? Text("LoadingTargets") : Model.HasError ? Model.ErrorMessage : Model.IsEmpty ? Text("NoTargets") : "";
    }

    private void Search_TextChanged(object sender, TextChangedEventArgs args)
    {
        Model.SearchText = Search.Text;
    }

    private async void Refresh_Click(object sender, RoutedEventArgs args)
    {
        if (!Model.CanRefresh)
        {
            return;
        }

        loading = Model.RefreshAsync(cancellation.Token);
        await loading;
    }
}
