// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.ObjectModel;
using System.Globalization;
using Microsoft.UI;
using Microsoft.UI.Dispatching;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.Windows.Storage.Pickers;
using WinUIProfiler.App.ViewModels;
using WinUIProfiler.Engine.Storage;

namespace WinUIProfiler.App.Views;

// The Frames tab: a session file's frames and the events of the selected one. It reads the file, not the live
// pipeline, so it shows a session being recorded and a saved one the same way.
public sealed partial class FramesView : UserControl
{
    private const int PageSize = 100;
    private const int LaunchFrames = 10;

    private readonly Microsoft.Windows.ApplicationModel.Resources.ResourceLoader strings = new();
    private readonly DispatcherQueueTimer refreshTimer;
    private readonly DispatcherQueueTimer filterTimer;
    private readonly Dictionary<long, FrameRow> rows = [];
    private SessionSource? source;
    private string? path;
    // Counts the files shown, so a read of an earlier one, or of this one before it was reopened, is dropped.
    private int shown;
    // The latest event the last read saw, and where the file was complete from then: trimming the oldest events changes it.
    private long? readSeq;
    private long? trimmedBefore;
    private long? reselect;
    private bool reading;
    private bool readAgain;
    private FrameRow? selected;
    private int offset;
    // The frame, bucket, filter and offset of the event page shown.
    private (long FrameId, string? Bucket, string? Filter, int Offset)? shownPage;
    private CancellationTokenSource? eventsRead;

    public FramesView()
    {
        InitializeComponent();
        refreshTimer = DispatcherQueue.CreateTimer();
        refreshTimer.Interval = TimeSpan.FromSeconds(1);
        refreshTimer.Tick += (_, _) => _ = RefreshAsync();
        filterTimer = DispatcherQueue.CreateTimer();
        filterTimer.Interval = TimeSpan.FromMilliseconds(300);
        filterTimer.IsRepeating = false;
        filterTimer.Tick += (_, _) => ShowEvents(0);
        Show(null);
    }

    public ObservableCollection<FrameRow> Frames { get; } = [];
    public ObservableCollection<FrameEventRow> Events { get; } = [];

    // The window that owns the file picker.
    public WindowId Owner { get; set; }

    // The file to show, re-read every second while the connection writes it.
    public SessionSource? Source
    {
        get => source;
        set
        {
            if (source is not null)
            {
                source.Changed -= Source_Changed;
            }

            source = value;
            if (source is not null)
            {
                source.Changed += Source_Changed;
                Source_Changed(source, EventArgs.Empty);
            }
        }
    }

    // Asks to show a UI element in the Elements tab.
    public event EventHandler<long>? ElementRequested;

    private void Source_Changed(object? sender, EventArgs args)
    {
        if (source!.Path != path)
        {
            Show(source.Path);
        }

        if (source.Live)
        {
            if (!refreshTimer.IsRunning)
            {
                refreshTimer.Start();
            }
        }
        else if (refreshTimer.IsRunning)
        {
            // One last read for the events saved when the connection ended.
            refreshTimer.Stop();
            _ = RefreshAsync();
        }
    }

    private string Text(string key) => strings.GetString(key);
    private string Format(string key, params object?[] values) => string.Format(CultureInfo.CurrentCulture, Text(key), values);

    private void Show(string? sessionPath)
    {
        path = sessionPath;
        shown++;
        reselect = null;
        rows.Clear();
        Frames.Clear();
        readSeq = null;
        ReadError.IsOpen = false;
        FileSummary.Text = path is null ? Text("FramesNoFile") : Path.GetFileName(path);
        ToolTipService.SetToolTip(FileSummary, path);
        SetLine(LaunchSummary, "");
        SelectFrame(null);
        _ = RefreshAsync();
    }

    private async Task RefreshAsync()
    {
        if (reading)
        {
            readAgain = true;
            return;
        }

        if (path is not { } file)
        {
            return;
        }

        reading = true;
        try
        {
            var after = readSeq;
            var current = shown;
            var (info, (frames, seq)) = await Task.Run(() => (SessionFile.ReadInfo(file), SessionFile.ReadChangedFrames(file, after)));
            if (current != shown)
            {
                return;
            }

            if (after is not null && info.CompleteFromSeq != trimmedBefore)
            {
                // The writer deleted the oldest events, and the frames left without any, so read the file again.
                var keep = selected?.Frame.FrameId;
                Show(file);
                reselect = keep;
                return;
            }

            trimmedBefore = info.CompleteFromSeq;
            readSeq = seq;
            foreach (var frame in frames)
            {
                if (!rows.TryGetValue(frame.FrameId, out var row))
                {
                    rows.Add(frame.FrameId, row = new(frame));
                    Frames.Add(row);
                    continue;
                }

                var grew = row.Frame.Events != frame.Events;
                row.Update(frame);
                if (row == selected)
                {
                    UpdateFrameTitle();
                    if (grew)
                    {
                        ShowEvents(offset, grew: true);
                    }
                }
            }

            if (reselect is { } id)
            {
                reselect = null;
                FrameList.SelectedItem = rows.GetValueOrDefault(id);
            }

            ShowSummary(info);
            ReadError.IsOpen = false;
        }
        catch (Exception error) when (error is not OutOfMemoryException)
        {
            ShowError(error);
        }
        finally
        {
            reading = false;
            if (readAgain)
            {
                readAgain = false;
                _ = RefreshAsync();
            }
        }
    }

    // Shows when the first drawn frames of the first UI thread ended: from process start if the recording saw it,
    // otherwise from the start of the recording. Not shown for a trimmed file, which no longer has them.
    private void ShowSummary(SessionFileInfo info)
    {
        FileSummary.Text = Format(info.CompleteFromSeq is null ? "FramesFileFormat" : "FramesFileTrimmedFormat",
            Path.GetFileName(path), info.Frames);
        if (Frames.Count == 0 || info.CompleteFromSeq is not null)
        {
            SetLine(LaunchSummary, "");
            return;
        }

        var origin = info.Coverage == SessionCoverage.ProcessStart ? info.ProcessStartMilliseconds : null;
        var thread = Frames[0].Frame.ThreadId;
        var drawn = Frames.Select(row => row.Frame).Where(frame => frame.ThreadId == thread && frame.Drawn && frame.EndMilliseconds is not null)
            .Take(LaunchFrames)
            .Select(frame => Format("LaunchItemFormat", frame.FrameNumber, frame.EndMilliseconds - (origin ?? 0)));
        var items = string.Join(" · ", drawn);
        SetLine(LaunchSummary, items.Length == 0 ? "" : Format(origin is null ? "LaunchFromRecordingFormat" : "LaunchFromProcessFormat", items));
    }

    // A one-line text that may be cut off; its tooltip shows all of it.
    private static void SetLine(TextBlock line, string text)
    {
        line.Text = text;
        ToolTipService.SetToolTip(line, text.Length == 0 ? null : text);
    }

    private void FrameList_SelectionChanged(object sender, SelectionChangedEventArgs args) => SelectFrame(FrameList.SelectedItem as FrameRow);

    private void Buckets_SelectionChanged(SelectorBar sender, SelectorBarSelectionChangedEventArgs args) => ShowEvents(0);

    private void SelectFrame(FrameRow? row)
    {
        selected = row;
        UpdateFrameTitle();
        ShowEvents(0);
    }

    private void UpdateFrameTitle()
    {
        var frame = selected?.Frame;
        SetLine(FrameTitle, frame is null ? Text("SelectFramePrompt") : Format("FrameTitleFormat", frame.FrameNumber, frame.ThreadId,
            frame.Ticks, frame.StartMilliseconds, frame.EndMilliseconds, frame.TickMilliseconds));
    }

    // Shows a page of the selected frame's events in the selected bucket that match the filter. When the frame grew, a
    // full page stays as it is, since events are only added after it, and a last page is read again keeping the selection.
    private async void ShowEvents(int start, bool grew = false)
    {
        eventsRead?.Cancel();
        offset = start;
        if (selected is not { } row || path is not { } file)
        {
            Events.Clear();
            shownPage = null;
            EventDetails.Text = Text("SelectEventPrompt");
            Position.Text = "";
            PreviousButton.IsEnabled = NextButton.IsEnabled = false;
            return;
        }

        using var cancellation = new CancellationTokenSource();
        eventsRead = cancellation;
        var frameId = row.Frame.FrameId;
        var bucket = Buckets.SelectedItem?.Tag as string;
        var filter = string.IsNullOrWhiteSpace(FilterBox.Text) ? null : FilterBox.Text;
        // Only the page that is shown, not one still being read, can be kept.
        var pageIsFull = grew && shownPage == (frameId, bucket, filter, start) && Events.Count == PageSize;
        var keep = grew ? EventList.SelectedIndex : -1;
        try
        {
            var (total, first, page) = await Task.Run(() =>
            {
                var count = SessionFile.CountFrameEvents(file, frameId, bucket, filter);
                if (pageIsFull)
                {
                    return (count, start, (List<FrameEventRow>?)null);
                }

                // Past the end, for example in a bucket with fewer events, the last page is shown.
                var first = start >= count && count > 0 ? (int)((count - 1) / PageSize * PageSize) : start;
                var events = new List<FrameEventRow>(PageSize);
                SessionFile.ReadFrameEvents(file, frameId, bucket, filter, first, PageSize,
                    (profilerEvent, eventBucket, mentions) => events.Add(new(profilerEvent, eventBucket, mentions)), cancellation.Token);
                return (count, first, events);
            }, cancellation.Token);
            if (cancellation.IsCancellationRequested)
            {
                return;
            }

            offset = first;
            if (page is not null)
            {
                Events.Clear();
                foreach (var item in page)
                {
                    Events.Add(item);
                }

                shownPage = (frameId, bucket, filter, first);
                if (keep >= 0 && keep < Events.Count)
                {
                    EventList.SelectedIndex = keep;
                }
            }

            Position.Text = total == 0 ? Text(filter is null ? "PositionNoEvents" : "PositionNoMatches")
                : Format(filter is null ? "PositionFormat" : "PositionFilteredFormat", first + 1, first + Events.Count, total);
            PreviousButton.IsEnabled = first > 0;
            NextButton.IsEnabled = first + Events.Count < total;
        }
        catch (OperationCanceledException) when (cancellation.IsCancellationRequested)
        {
        }
        catch (Exception error) when (error is not OutOfMemoryException)
        {
            ShowError(error);
        }
        finally
        {
            if (eventsRead == cancellation)
            {
                eventsRead = null;
            }
        }
    }

    private void Previous_Click(object sender, RoutedEventArgs args) => ShowEvents(Math.Max(0, offset - PageSize));

    private void Next_Click(object sender, RoutedEventArgs args) => ShowEvents(offset + PageSize);

    private void FilterBox_TextChanged(object sender, TextChangedEventArgs args)
    {
        filterTimer.Stop();
        filterTimer.Start();
    }

    private void EventList_SelectionChanged(object sender, SelectionChangedEventArgs args)
    {
        var row = EventList.SelectedItem as FrameEventRow;
        EventDetails.Text = row?.Details ?? Text("SelectEventPrompt");
        ElementLinks.ItemsSource = row?.Elements;
        ElementLinksPanel.Visibility = row?.Elements.Count > 0 ? Visibility.Visible : Visibility.Collapsed;
    }

    private void ElementLink_Click(object sender, RoutedEventArgs args) =>
        ElementRequested?.Invoke(this, (long)((FrameworkElement)sender).Tag);

    private async void Open_Click(object sender, RoutedEventArgs args)
    {
        try
        {
            var picker = new FileOpenPicker(Owner);
            picker.FileTypeFilter.Add(".sqlite");
            if (await picker.PickSingleFileAsync() is not { } file)
            {
                return;
            }

            source?.Open(file.Path);
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
