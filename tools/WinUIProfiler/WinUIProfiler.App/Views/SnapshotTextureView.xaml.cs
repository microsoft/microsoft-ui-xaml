using System.Diagnostics;
using System.Globalization;
using System.Text.Json;
using Microsoft.Data.Sqlite;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Automation;
using Microsoft.UI.Xaml.Controls;
using WinUIProfiler.App.ViewModels;
using WinUIProfiler.Contracts;
using WinUIProfiler.Engine.Processing;
using WinUIProfiler.Engine.Snapshots;
using WinUIProfiler.Engine.Storage;

namespace WinUIProfiler.App.Views;

public sealed record SnapshotTextureDisplayRow(string Label, string Change, string Before, string After, string Delta,
    string BeforeDetails, string AfterDetails, string DetailsHeading, bool Comparison,
    SnapshotTextureDifference? Difference = null)
{
    public GridLength ComparisonWidth => new(Comparison ? 1 : 0, GridUnitType.Star);
    public Visibility ComparisonVisibility => Comparison ? Visibility.Visible : Visibility.Collapsed;
    public string Description => Comparison ? $"{Label}; {Change}; {Before}; {After}; {Delta}" : $"{Label}; {After}";
    public override string ToString() => Description;
}

public sealed partial class SnapshotTextureView : UserControl
{
    private sealed record Source(string? Path, TreeSnapshotDocument Document);
    private static readonly JsonSerializerOptions DetailOptions = new() { WriteIndented = true };
    private readonly Microsoft.Windows.ApplicationModel.Resources.ResourceLoader strings = new();
    private Source? before;
    private Source after = null!;
    private CancellationToken cancellation;
    private bool loading;
    public SnapshotTextureComparison? Data { get; private set; }
    public event EventHandler? Changed;

    public SnapshotTextureView() => InitializeComponent();

    public Task ShowAsync(SnapshotHistoryEntry? baseline, SnapshotViewerData? baselineData,
        SnapshotHistoryEntry current, SnapshotViewerData currentData, CancellationToken token)
    {
        before = baseline is null ? null : new(baseline.Saved.SessionPath, baselineData!.Document);
        after = new(current.Saved.SessionPath, currentData.Document);
        cancellation = token;
        VisualStateManager.GoToState(this, before is null ? "Single" : "Comparison", false);
        Columns.Content = new SnapshotTextureDisplayRow(Text("SnapshotTextureIdentity"), Text("SnapshotTextureChange"),
            Text("SnapshotTextureBeforeBytes"), Text(before is null ? "SnapshotTextureLinkedBytes" : "SnapshotTextureAfterBytes"),
            Text("SnapshotTextureDeltaBytes"), "", "", "", before is not null);
        SourcePath.Text = before is null || before.Path == after.Path ? after.Path ?? Text("SnapshotTextureNoRecording")
            : Format("SnapshotTexturePathsFormat", before.Path ?? Text("SnapshotTextureNoRecording"),
                after.Path ?? Text("SnapshotTextureNoRecording"));
        return ReloadAsync();
    }

    private async void Refresh_Click(object sender, RoutedEventArgs args) => await ReloadAsync();

    private async Task ReloadAsync()
    {
        if (loading || cancellation.IsCancellationRequested) return;
        loading = true;
        Refresh.IsEnabled = false;
        Status.Text = Text("SnapshotTextureLoading");
        try
        {
            Data = await Task.Run(() =>
            {
                if (before is not null && string.Equals(before.Path, after.Path, StringComparison.OrdinalIgnoreCase))
                {
                    var captures = Read(after.Path, [before.Document, after.Document]);
                    return new SnapshotTextureComparison(captures[0], captures[1]);
                }
                return new(before is null ? null : Read(before.Path, [before.Document])[0],
                    Read(after.Path, [after.Document])[0]);
            }, cancellation);
            if (cancellation.IsCancellationRequested) return;
            Status.Text = before is null ? CaptureStatus(Data.After) : Format("SnapshotResourceStatusFormat",
                CaptureStatus(Data.Before!), CaptureStatus(Data.After));
            Summary.Text = before is null ? CaptureSummary(Data.After) : Format("SnapshotTextureComparisonSummary",
                CaptureSummary(Data.Before!), CaptureSummary(Data.After), Data.Comparable
                    ? Format("SnapshotTextureDeltaSummary", Data.CountDelta, Signed(Data.KnownBytesDelta))
                    : Text("SnapshotTextureComparisonUnavailable"));
            AutomationProperties.SetItemStatus(Status, Status.Text);
            ShowRows();
            Changed?.Invoke(this, EventArgs.Empty);
        }
        catch (OperationCanceledException) when (cancellation.IsCancellationRequested) { }
        finally
        {
            loading = false;
            if (!cancellation.IsCancellationRequested) Refresh.IsEnabled = true;
        }
    }

    private IReadOnlyList<SnapshotTextureCapture> Read(string? path, TreeSnapshotDocument[] snapshots)
    {
        try { return SessionFile.ReadTextureSnapshots(path, snapshots, cancellation); }
        catch (Exception error) when (error is IOException or UnauthorizedAccessException or SqliteException or
            InvalidDataException or FormatException)
        {
            Trace.TraceWarning(error.ToString());
            return snapshots.Select(_ => new SnapshotTextureCapture(SnapshotTextureStatus.Unavailable, null, [])
                { Error = error.Message }).ToArray();
        }
    }

    private string CaptureStatus(SnapshotTextureCapture capture)
    {
        var status = Text("SnapshotTexture" + capture.Status);
        if (capture.Error is { } error) status += " " + error;
        if (capture.Available)
        {
            if (capture.Trimmed) status += " " + Text("SnapshotTextureTrimmed");
            if (capture.LostByEtw is > 0 || capture.DroppedByProfiler is > 0)
                status += " " + Format("SnapshotTextureLossFormat", Value(capture.LostByEtw), Value(capture.DroppedByProfiler));
            if (capture.RecordingActive) status += " " + Text("SnapshotTextureRecording");
        }
        return status;
    }

    private string CaptureSummary(SnapshotTextureCapture capture) => !capture.Available
        ? Text("SnapshotResourceUnavailable")
        : Format("SnapshotTextureSummaryFormat", capture.Textures.Count, Value(capture.KnownLinkedBytes),
            capture.Textures.Count(texture => texture.LinkedBytes is null),
            (capture.AtQpc - capture.StartQpc) * 1000.0 / capture.QpcFrequency);

    private void ChangesOnly_Click(object sender, RoutedEventArgs args) => ShowRows();

    private void ShowRows()
    {
        if (Data is null) return;
        var rows = Data.Rows.Where(row => ChangesOnly.IsChecked != true ||
            row.Change != SnapshotTextureChange.Unchanged).Select(row =>
        {
            var texture = row.After ?? row.Before!;
            return new SnapshotTextureDisplayRow(
                Format("SnapshotTextureIdentityFormat", texture.Kind, texture.Handle, texture.Id),
                Text("SnapshotTexture" + row.Change), Bytes(row.Before), Bytes(row.After), Signed(row.Delta),
                "", "", Text(before is null ? "SnapshotTextureDetails" : "SnapshotTextureAfterDetails"),
                before is not null, row);
        }).ToArray();
        Textures.ItemsSource = rows;
        Textures.SelectedIndex = rows.Length > 0 ? 0 : -1;
        UpdateDetails();
        Empty.Visibility = rows.Length == 0 ? Visibility.Visible : Visibility.Collapsed;
        AutomationProperties.SetItemStatus(Textures, rows.Length.ToString(CultureInfo.InvariantCulture));
    }

    private string DetailsOf(SnapshotTexture? texture, SnapshotTextureCapture? capture)
    {
        if (texture is null) return capture?.Available == true
            ? Text("SnapshotTextureNotObserved") : Text("SnapshotResourceUnavailable");
        using var descriptor = JsonDocument.Parse(texture.Descriptor);
        return JsonSerializer.Serialize(new
        {
            GraphicsObjectId = texture.Id, texture.Kind, texture.Handle, texture.DeviceHandle,
            SnapshotQpc = capture?.AtQpc, texture.StartQpc, texture.EndQpc, texture.EndReason, texture.Trimmed,
            LinkedAllocationBytes = texture.LinkedBytes, Descriptor = descriptor.RootElement,
            texture.Allocations
        }, DetailOptions);
    }

    private void Textures_SelectionChanged(object sender, SelectionChangedEventArgs args) => UpdateDetails();

    private void UpdateDetails() =>
        Details.Content = Textures.SelectedItem is SnapshotTextureDisplayRow { Difference: { } difference } row
            ? row with { BeforeDetails = DetailsOf(difference.Before, Data?.Before),
                AfterDetails = DetailsOf(difference.After, Data?.After) } : null;

    private string Bytes(SnapshotTexture? texture) => texture is null
        ? Text("SnapshotTextureNotObserved") : Value(texture.LinkedBytes);
    private string Value(decimal? value) => value?.ToString("N0", CultureInfo.CurrentCulture)
        ?? Text("SnapshotResourceUnavailable");
    private string Signed(decimal? value) => value?.ToString("+#,0;-#,0;0", CultureInfo.CurrentCulture)
        ?? Text("SnapshotResourceUnavailable");
    private string Text(string key) => strings.GetString(key);
    private string Format(string key, params object?[] values) =>
        string.Format(CultureInfo.CurrentCulture, Text(key), values);
}
