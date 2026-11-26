// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Diagnostics;
using System.Globalization;
using System.Text.Json;
using Microsoft.Data.Sqlite;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Automation;
using Microsoft.UI.Xaml.Controls;
using WinUIProfiler.App.ViewModels;
using WinUIProfiler.Contracts;
using WinUIProfiler.Engine.Snapshots;
using WinUIProfiler.Engine.Storage;

namespace WinUIProfiler.App.Views;

public sealed record SnapshotCompositionDisplayRow(string Label, string NativeId, string Created, string Released,
    string Evidence, NativeCompositionLifetime? Native, WinUICompositionLifetime? WinUI)
{
    public string Description => $"{Label}; {NativeId}; {Created}; {Released}; {Evidence}";
}

public sealed partial class SnapshotCompositionView : UserControl
{
    private static readonly JsonSerializerOptions DetailOptions = new() { WriteIndented = true };
    private readonly Microsoft.Windows.ApplicationModel.Resources.ResourceLoader strings = new();
    private string? path;
    private TreeSnapshotDocument? snapshot;
    private CancellationToken cancellation;
    private bool loading;
    public SnapshotCompositionCapture? Data { get; private set; }

    public SnapshotCompositionView() => InitializeComponent();

    public Task ShowAsync(SnapshotHistoryEntry current, SnapshotViewerData currentData, CancellationToken token)
    {
        path = current.Saved.SessionPath;
        snapshot = currentData.Document;
        cancellation = token;
        Scope.Text = Format("SnapshotCompositionScopeFormat", snapshot.ProcessId, snapshot.EndQpc);
        SourcePath.Text = path ?? Text("SnapshotTextureNoRecording");
        return ReloadAsync();
    }

    private async void Refresh_Click(object sender, RoutedEventArgs args) => await ReloadAsync();

    private async Task ReloadAsync()
    {
        if (loading || snapshot is null || cancellation.IsCancellationRequested) return;
        loading = true;
        Refresh.IsEnabled = false;
        Status.Text = Text("SnapshotCompositionLoading");
        try
        {
            Data = await Task.Run(() => Read(snapshot), cancellation);
            if (cancellation.IsCancellationRequested) return;
            Status.Text = Text("SnapshotComposition" + Data.Status);
            if (Data.Error is { } error) Status.Text += " " + error;
            if (Data.Recording is { } recording)
            {
                if (recording.Session.CompleteFromSeq is not null)
                    Status.Text += " " + Text("SnapshotCompositionTrimmed");
                if (recording.Session.LostByEtw is > 0 || recording.Session.DroppedByProfiler is > 0)
                    Status.Text += " " + Format("SnapshotTextureLossFormat",
                        Number(recording.Session.LostByEtw), Number(recording.Session.DroppedByProfiler));
                NativeSummary.Text = Format("SnapshotCompositionNativeSummaryFormat", recording.NativeCreationObservations,
                    recording.NativeReleaseObservations, recording.MatchedReleaseObservations,
                    recording.NativeLifetimes.Count(row => row.ReleaseSeq is null));
                WinUISummary.Text = Format("SnapshotCompositionWinUISummaryFormat", recording.WinUICreations.Count,
                    recording.WinUICreations.Count(row => row.Correlation == "InferredSequence"),
                    recording.WinUICreations.Count(row => row.Correlation == "Ambiguous"),
                    recording.WinUICreations.Count(row => row.Correlation == "Unattributed"));
            }
            else NativeSummary.Text = WinUISummary.Text = "";
            AutomationProperties.SetItemStatus(Status, Status.Text);
            ShowRows();
        }
        catch (OperationCanceledException) when (cancellation.IsCancellationRequested) { }
        finally
        {
            loading = false;
            if (!cancellation.IsCancellationRequested) Refresh.IsEnabled = true;
        }
    }

    private SnapshotCompositionCapture Read(TreeSnapshotDocument document)
    {
        try { return SessionFile.ReadCompositionSnapshot(path, document, cancellation); }
        catch (Exception error) when (error is IOException or UnauthorizedAccessException or SqliteException or
            InvalidDataException or FormatException)
        {
            Trace.TraceWarning(error.ToString());
            return new(SnapshotCompositionStatus.Unavailable, document.EndQpc) { Error = error.Message };
        }
    }

    private void Filter_Changed(object sender, SelectionChangedEventArgs args) => ShowRows();
    private void Search_Changed(object sender, TextChangedEventArgs args) => ShowRows();
    private void ReleasedOnly_Click(object sender, RoutedEventArgs args) => ShowRows();

    private void ShowRows()
    {
        if (Data is null) return;
        var selected = Lifetimes.SelectedItem as SnapshotCompositionDisplayRow;
        IEnumerable<SnapshotCompositionDisplayRow> rows = [];
        if (Data.Recording is { } recording)
        {
            rows = Mode.SelectedIndex == 0
                ? recording.NativeLifetimes.Select(row => new SnapshotCompositionDisplayRow(
                    row.ResourceType is { } type ? Format("SnapshotCompositionTypeFormat", type)
                        : Text("SnapshotCompositionUnknownType"),
                    Identifier(row.NativeResourceId), Time(row.CreationMilliseconds), Time(row.ReleaseMilliseconds),
                    Text("SnapshotComposition" + row.Status), row, null))
                : recording.WinUICreations.Select(row => new SnapshotCompositionDisplayRow(row.ResourceName,
                    Identifier(row.NativeResourceId), Time(row.CreationMilliseconds), Time(row.ReleaseMilliseconds),
                    Text("SnapshotComposition" + row.Correlation) + (row.NativeStatus is { } status
                        ? "; " + Text("SnapshotComposition" + status) : ""), null, row));
        }
        var filter = Search.Text.Trim();
        rows = rows.Where(row => (ReleasedOnly.IsChecked != true ||
                row.Native?.ReleaseSeq is not null || row.WinUI?.ReleaseSeq is not null) &&
            (filter.Length == 0 || row.Description.Contains(filter, StringComparison.OrdinalIgnoreCase) ||
                (row.Native?.ResourcePointer ?? row.WinUI?.ResourcePointer)?.Contains(filter,
                    StringComparison.OrdinalIgnoreCase) == true ||
                (row.Native?.ResourceHandle ?? row.WinUI?.ResourceHandle)?.Contains(filter,
                    StringComparison.OrdinalIgnoreCase) == true));
        var values = rows.ToArray();
        Lifetimes.ItemsSource = values;
        Lifetimes.SelectedItem = values.FirstOrDefault(row => selected is not null &&
            (selected.Native is not null && row.Native?.NativeResourceId == selected.Native.NativeResourceId ||
                selected.WinUI is not null && row.WinUI?.WinUISeq == selected.WinUI.WinUISeq))
            ?? values.FirstOrDefault();
        Empty.Visibility = values.Length == 0 ? Visibility.Visible : Visibility.Collapsed;
        AutomationProperties.SetItemStatus(Lifetimes, values.Length.ToString(CultureInfo.InvariantCulture));
        UpdateDetails();
    }

    private void Lifetimes_SelectionChanged(object sender, SelectionChangedEventArgs args) => UpdateDetails();

    private void UpdateDetails() =>
        Details.Text = Lifetimes.SelectedItem is SnapshotCompositionDisplayRow row
            ? JsonSerializer.Serialize(new
            {
                snapshot?.CaptureId,
                SnapshotEndQpc = snapshot?.EndQpc,
                ProcessId = Data?.Recording?.Session.ProcessId,
                RecordingPath = path,
                NativeLifetime = row.Native,
                WinUIObservation = row.WinUI
            }, DetailOptions)
            : Text("SnapshotCompositionSelectLifetime");

    private string Identifier(long? value) => value?.ToString(CultureInfo.InvariantCulture)
        ?? Text("SnapshotResourceUnavailable");
    private string Number(long? value) => value?.ToString("N0", CultureInfo.CurrentCulture)
        ?? Text("SnapshotResourceUnavailable");
    private string Time(double? value) => value?.ToString("N3", CultureInfo.CurrentCulture)
        ?? Text("SnapshotResourceUnavailable");
    private string Text(string key) => strings.GetString(key);
    private string Format(string key, params object?[] values) =>
        string.Format(CultureInfo.CurrentCulture, Text(key), values);
}
