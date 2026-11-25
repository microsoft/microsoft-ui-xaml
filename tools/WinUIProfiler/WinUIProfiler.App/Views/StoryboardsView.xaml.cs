// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Globalization;
using Microsoft.UI;
using Microsoft.UI.Dispatching;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.Windows.Storage.Pickers;
using WinUIProfiler.App.ViewModels;
using WinUIProfiler.Engine.Storage;

namespace WinUIProfiler.App.Views;

public sealed partial class StoryboardsView : UserControl
{
    private readonly Microsoft.Windows.ApplicationModel.Resources.ResourceLoader strings = new();
    private readonly DispatcherQueueTimer refreshTimer;
    private SessionSource? source;
    private string? path;
    private int generation;
    private bool reading;
    private bool readAgain;
    private CancellationTokenSource? activeRead;
    private StoryboardSnapshot? snapshot;

    public StoryboardsView()
    {
        InitializeComponent();
        refreshTimer = DispatcherQueue.CreateTimer();
        refreshTimer.Interval = TimeSpan.FromSeconds(1);
        refreshTimer.Tick += (_, _) => _ = RefreshAsync();
        AtTime.ValueChanged += AtTime_ValueChanged;
        ActiveOnly.Checked += Filter_Changed;
        ActiveOnly.Unchecked += Filter_Changed;
        Loaded += (_, _) => { UpdateTimer(); _ = RefreshAsync(); };
        Unloaded += (_, _) => { refreshTimer.Stop(); activeRead?.Cancel(); };
        ShowRows();
    }

    public WindowId Owner { get; set; }

    public SessionSource? Source
    {
        get => source;
        set
        {
            if (source is not null) source.Changed -= Source_Changed;
            source = value;
            if (source is not null)
            {
                source.Changed += Source_Changed;
                Source_Changed(source, EventArgs.Empty);
            }
        }
    }

    private string Text(string key) => strings.GetString(key);
    private string Format(string key, params object?[] values) =>
        string.Format(CultureInfo.CurrentCulture, Text(key), values);

    private void Source_Changed(object? sender, EventArgs args)
    {
        if (path != source?.Path)
        {
            path = source?.Path;
            snapshot = null;
            generation++;
            activeRead?.Cancel();
            AtTime.Value = double.NaN;
            ReadError.IsOpen = false;
            ShowRows();
        }
        UpdateTimer();
        _ = RefreshAsync();
    }

    private void UpdateTimer()
    {
        if (IsLoaded && source?.Live == true) refreshTimer.Start();
        else refreshTimer.Stop();
    }

    private async Task RefreshAsync()
    {
        if (reading)
        {
            readAgain = true;
            return;
        }
        if (!IsLoaded || path is not { } file) return;

        reading = true;
        var current = generation;
        var at = double.IsNaN(AtTime.Value) ? (double?)null : AtTime.Value;
        using var cancellation = new CancellationTokenSource();
        activeRead = cancellation;
        try
        {
            var result = await Task.Run(() => SessionFile.ReadStoryboards(file, at,
                cancellationToken: cancellation.Token), cancellation.Token);
            if (current != generation || !IsLoaded) return;
            snapshot = result;
            ReadError.IsOpen = false;
            ShowRows();
        }
        catch (OperationCanceledException) when (cancellation.IsCancellationRequested)
        {
        }
        catch (Exception error) when (error is not OutOfMemoryException)
        {
            if (current == generation)
            {
                snapshot = null;
                ShowRows();
                ShowError(error);
            }
        }
        finally
        {
            activeRead = null;
            reading = false;
            if (readAgain)
            {
                readAgain = false;
                _ = RefreshAsync();
            }
        }
    }

    private void ShowRows()
    {
        FileSummary.Text = path is null ? Text("StoryboardsNoFile") : Path.GetFileName(path);
        ToolTipService.SetToolTip(FileSummary, path);
        RefreshButton.IsEnabled = path is not null;
        var rows = snapshot?.Storyboards ?? [];
        var filtered = rows.Where(row => ActiveOnly.IsChecked != true || row.State is "Active" or "Paused").ToArray();
        StoryboardList.ItemsSource = filtered.Select(row => new StoryboardRow(
            row.Name.Length == 0 ? Text("StoryboardsUnnamed") : row.Name,
            Format("StoryboardsIdentityFormat", row.StoryboardId, row.RunId, row.ObjectAddress, row.ThreadId),
            Format("StoryboardsObservedFormat", row.FirstMilliseconds, row.LastMilliseconds),
            Text("StoryboardsState" + row.State),
            Format("StoryboardsFlagsFormat", Text(row.HasIndependentAnimation ? "StoryboardsYes" : "StoryboardsNo"),
                Text(row.IsInTimeManager ? "StoryboardsYes" : "StoryboardsNo"),
                row.ChildCount is { } count ? count.ToString("N0", CultureInfo.CurrentCulture) : Text("StoryboardsUnknown")),
            Format("StoryboardsOperationFormat", Text("StoryboardsOperation" + row.Operation),
                Text(row.BeginObserved ? "StoryboardsBeginObserved" :
                    row.BeginFailed ? "StoryboardsOperationBeginFailed" : "StoryboardsBeginMissing")))).ToArray();
        Empty.Visibility = filtered.Length == 0 ? Visibility.Visible : Visibility.Collapsed;
        Empty.Text = Text(snapshot is { Observations: > 0 } ? "StoryboardsFilteredEmpty" : "StoryboardsEmpty");
        Summary.Text = snapshot is null ? "" : Format("StoryboardsSummaryFormat", filtered.Length, snapshot.Observations);
        Notice.Message = Text("StoryboardsCaveat");
        if (snapshot?.HistoryIncomplete == true) Notice.Message += " " + Text("StoryboardsPartial");
        if (snapshot?.Limited == true) Notice.Message += " " + Text("StoryboardsLimited");
        Notice.Severity = snapshot?.HistoryIncomplete == true ? InfoBarSeverity.Warning : InfoBarSeverity.Informational;
    }

    private void Filter_Changed(object sender, RoutedEventArgs args) => ShowRows();

    private void AtTime_ValueChanged(NumberBox sender, NumberBoxValueChangedEventArgs args)
    {
        generation++;
        activeRead?.Cancel();
        snapshot = null;
        ShowRows();
        _ = RefreshAsync();
    }

    private void Refresh_Click(object sender, RoutedEventArgs args) => _ = RefreshAsync();

    private async void Open_Click(object sender, RoutedEventArgs args)
    {
        try
        {
            var picker = new FileOpenPicker(Owner);
            picker.FileTypeFilter.Add(".sqlite");
            if (await picker.PickSingleFileAsync() is { } file) source?.Open(file.Path);
        }
        catch (Exception error) when (error is not OutOfMemoryException)
        {
            ShowError(error);
        }
    }

    private void ShowError(Exception error)
    {
        ReadError.Message = error.Message;
        ReadError.IsOpen = true;
    }
}

public sealed record StoryboardRow(string Name, string Identity, string Observation, string State, string Flags, string Operation)
{
    public string AccessibleName => $"{Name}. {Identity}. {Observation}. {State}. {Flags}. {Operation}";
}
