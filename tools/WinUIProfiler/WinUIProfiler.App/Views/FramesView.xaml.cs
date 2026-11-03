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

// The Frames tab: a session file's UI-thread ticks, the regions of interest in them, and the events of the selected row.
// It reads the file, not the live pipeline, so it shows a session being recorded and a saved one the same way.
public sealed partial class FramesView : UserControl
{
    private const int PageSize = 100;
    private const int LaunchFrames = 10;

    // The Show filter's choices, in order.
    private enum Filter
    {
        All,
        Interesting,
        Slow
    }

    private readonly Microsoft.Windows.ApplicationModel.Resources.ResourceLoader strings = new();
    private readonly DispatcherQueueTimer refreshTimer;
    private readonly DispatcherQueueTimer filterTimer;
    private readonly Dictionary<long, FrameRow> rows = [];
    // The ticks in the order they were first seen, and each by its thread and latest event: a region ends with a tick's
    // stop.
    private readonly List<FrameRow> ticks = [];
    private readonly Dictionary<(int Thread, long Seq), FrameRow> ticksByLastEvent = [];
    // Every row the list can show, by when it starts: the ticks, each region of interest right above the tick it ends
    // with, and the interesting rows outside the regions. The list shows the ones the Show filter keeps.
    private readonly List<object> all = [];
    // Every interesting row read, and the regions among them, for the rows of a selected region.
    private readonly List<InterestingRow> interestingRows = [];
    private readonly List<InterestingRow> regions = [];
    private ObservableCollection<object> shownRows = [];
    private InterestingRows interesting = new();
    private Filter filter;
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
    private bool fileRead;
    // A FrameRow or an InterestingRowItem.
    private object? selected;
    private int offset;
    // The scope (a tick, or an interesting row's span), bucket, filter and offset of the event page shown.
    private (string Scope, string? Bucket, string? Filter, int Offset)? shownPage;
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
        FrameList.ItemsSource = shownRows;
        // Hooked up after the first selection, which shows all ticks.
        ShowFilter.SelectedIndex = 0;
        ShowFilter.SelectionChanged += ShowFilter_SelectionChanged;
        Show(null);
    }

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

    // Asks to show a UI element or visual in the Objects tab.
    public event EventHandler<ObjectMention>? ObjectRequested;

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
        ticks.Clear();
        ticksByLastEvent.Clear();
        all.Clear();
        interestingRows.Clear();
        regions.Clear();
        interesting = new();
        fileRead = false;
        FrameList.ItemsSource = shownRows = [];
        UpdateEmpty();
        readSeq = null;
        ReadError.IsOpen = false;
        FileSummary.Text = path is null ? Text("FramesNoFile") : Path.GetFileName(path);
        ToolTipService.SetToolTip(FileSummary, path);
        SetLine(LaunchSummary, "");
        SelectRow(null);
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
            var reader = interesting;
            var live = source?.Live == true;
            var (info, (frames, seq), newRows) = await Task.Run(() =>
            {
                var info = SessionFile.ReadInfo(file);
                var changed = SessionFile.ReadChangedFrames(file, after);
                // While the file grows, only up to the ticks just read, so a region's tick is listed when the region is.
                return (info, changed, reader.Read(file, live ? changed.LastSeq : null));
            });
            if (current != shown)
            {
                return;
            }

            if (after is not null && info.CompleteFromSeq != trimmedBefore)
            {
                // The writer deleted the oldest events, and the ticks left without any, so read the file again.
                var keep = (selected as FrameRow)?.Frame.FrameId;
                Show(file);
                reselect = keep;
                return;
            }

            trimmedBefore = info.CompleteFromSeq;
            readSeq = seq;
            var changedRows = newRows.Count > 0;
            foreach (var frame in frames)
            {
                if (!rows.TryGetValue(frame.FrameId, out var row))
                {
                    rows.Add(frame.FrameId, row = new(frame));
                    ticks.Add(row);
                    Insert(row);
                    changedRows = true;
                }
                else
                {
                    var grew = row.Frame.Events != frame.Events;
                    var wasSlow = row.IsSlow;
                    ticksByLastEvent.Remove((row.Frame.ThreadId, row.Frame.LastSeq));
                    row.Update(frame);
                    changedRows |= row.IsSlow != wasSlow;
                    if (row == selected)
                    {
                        UpdateTitle();
                        if (grew)
                        {
                            ShowEvents(offset, grew: true);
                        }
                    }
                }

                ticksByLastEvent[(frame.ThreadId, frame.LastSeq)] = row;
            }

            foreach (var row in newRows)
            {
                AddInteresting(row);
            }

            if (changedRows)
            {
                Sync();
            }

            if (newRows.Count > 0 && selected is InterestingRowItem { IsRegion: true } region)
            {
                ShowRegionRows(region);
            }

            fileRead = true;
            UpdateEmpty();
            if (reselect is { } id)
            {
                reselect = null;
                if (rows.GetValueOrDefault(id) is { } row && shownRows.Contains(row))
                {
                    FrameList.SelectedItem = row;
                }
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

    // A region goes right above the tick it ends with, and takes the interesting rows that start in it out of the list;
    // they show when the region is selected. A tick's interesting row is not listed: the tick is.
    private void AddInteresting(InterestingRow row)
    {
        interestingRows.Add(row);
        if (row.Type == InterestingRowType.Frame)
        {
            return;
        }

        if (row.Type != InterestingRowType.Region)
        {
            if (!regions.Any(region => Contains(region, row)))
            {
                Insert(new InterestingRowItem(row, Format("InterestingTextFormat", row.Name, row.ThreadId, row.StartMilliseconds,
                    row.StopMilliseconds, row.DurationMilliseconds)));
            }

            return;
        }

        regions.Add(row);
        all.RemoveAll(item => item is InterestingRowItem { IsRegion: false } other && Contains(row, other.Row));
        var item = new InterestingRowItem(row, Format("RegionTextFormat", row.Trigger ?? row.Name, row.StartMilliseconds,
            row.StopMilliseconds, row.DurationMilliseconds));
        if (!ticksByLastEvent.TryGetValue((row.ThreadId, row.StopSeq), out var tick))
        {
            Insert(item);
            return;
        }

        item.Tick = tick;
        tick.Region = item;
        all.Insert(all.LastIndexOf(tick), item);
        // The tick may be listed already, without the region's tint.
        if (FrameList.ContainerFromItem(tick) is ListViewItem container)
        {
            container.Style = ((StyleSelector)Resources["FrameItemStyles"]).SelectStyle(tick, container);
        }
    }

    private static bool Contains(InterestingRow region, InterestingRow row) =>
        row.ThreadId == region.ThreadId && row.StartSeq >= region.StartSeq && row.StartSeq <= region.StopSeq;

    // Inserts a row after the rows that start no later. Rows mostly arrive in that order.
    private void Insert(object item)
    {
        var key = Key(item);
        var index = all.Count;
        while (index > 0 && Key(all[index - 1]) > key)
        {
            index--;
        }

        all.Insert(index, item);
    }

    // When a row starts. A region above its tick sorts with the tick; a tick whose start is not stored sorts by its end.
    private static double Key(object item) => item switch
    {
        FrameRow tick => tick.Frame.StartMilliseconds ?? tick.Frame.EndMilliseconds ?? double.MaxValue,
        InterestingRowItem { Tick: { } tick } => Key(tick),
        InterestingRowItem other => other.Row.StartMilliseconds,
        _ => double.MaxValue
    };

    private bool Shows(object item) => filter switch
    {
        Filter.Interesting => item is not FrameRow tick || tick.Region is not null,
        Filter.Slow => item is FrameRow { IsSlow: true },
        _ => true
    };

    // Makes the list show the rows the Show filter keeps. It adds and removes rows in place, so the scroll position and
    // the selection stay.
    private void Sync()
    {
        var keep = all.Where(Shows).ToList();
        if (shownRows.Count == 0)
        {
            FrameList.ItemsSource = shownRows = [.. keep];
            return;
        }

        var kept = keep.ToHashSet();
        for (var index = shownRows.Count - 1; index >= 0; index--)
        {
            if (!kept.Contains(shownRows[index]))
            {
                shownRows.RemoveAt(index);
            }
        }

        for (var index = 0; index < keep.Count; index++)
        {
            if (index == shownRows.Count || !ReferenceEquals(shownRows[index], keep[index]))
            {
                shownRows.Insert(index, keep[index]);
            }
        }
    }

    private void ShowFilter_SelectionChanged(object sender, SelectionChangedEventArgs args)
    {
        filter = (Filter)ShowFilter.SelectedIndex;
        var keep = selected;
        FrameList.ItemsSource = shownRows = [.. all.Where(Shows)];
        UpdateEmpty();
        if (keep is not null && shownRows.Contains(keep))
        {
            FrameList.SelectedItem = keep;
            FrameList.ScrollIntoView(keep);
        }
    }

    private void UpdateEmpty()
    {
        EmptyText.Text = Text(filter == Filter.Slow ? "SlowEmpty" : "InterestingEmpty");
        EmptyText.Visibility = fileRead && filter != Filter.All && shownRows.Count == 0 ? Visibility.Visible : Visibility.Collapsed;
    }

    // Shows when the first drawing ticks of the first UI thread ended: from process start if the recording saw it,
    // otherwise from the start of the recording. Not shown for a trimmed file, which no longer has them.
    private void ShowSummary(SessionFileInfo info)
    {
        FileSummary.Text = Format(info.CompleteFromSeq is null ? "FramesFileFormat" : "FramesFileTrimmedFormat",
            Path.GetFileName(path), info.Frames);
        if (ticks.Count == 0 || info.CompleteFromSeq is not null)
        {
            SetLine(LaunchSummary, "");
            return;
        }

        var origin = info.Coverage == SessionCoverage.ProcessStart ? info.ProcessStartMilliseconds : null;
        var thread = ticks[0].Frame.ThreadId;
        var drawn = ticks.Select(row => row.Frame)
            .Where(frame => frame.ThreadId == thread && frame.Drawn == true && frame.EndMilliseconds is not null)
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

    private void FrameList_SelectionChanged(object sender, SelectionChangedEventArgs args) => SelectRow(FrameList.SelectedItem);

    private void Buckets_SelectionChanged(SelectorBar sender, SelectorBarSelectionChangedEventArgs args) => ShowEvents(0);

    private void SelectRow(object? row)
    {
        selected = row;
        UpdateTitle();
        ShowRegionRows(row as InterestingRowItem);
        ShowEvents(0);
    }

    private void UpdateTitle()
    {
        SetLine(FrameTitle, selected switch
        {
            FrameRow tick => Format("FrameTitleFormat", tick.Frame.FrameNumber, tick.Frame.ThreadId, tick.Start, tick.End, tick.Tick,
                Text(tick.Frame.Drawn switch { true => "TickDrew", false => "TickDidNotDraw", null => "TickLatest" })),
            InterestingRowItem { Row: var row } => Format("InterestingTitleFormat", row.Name, row.ThreadId, row.StartMilliseconds,
                row.StopMilliseconds, row.DurationMilliseconds),
            _ => Text("SelectFramePrompt")
        });
    }

    // Lists the interesting rows in the selected region: the rows on its thread that start in it.
    private void ShowRegionRows(InterestingRowItem? item)
    {
        if (item is not { IsRegion: true, Row: var region })
        {
            RegionPanel.Visibility = Visibility.Collapsed;
            RegionRows.ItemsSource = null;
            return;
        }

        RegionRows.ItemsSource = interestingRows.Where(row => row.Type != InterestingRowType.Region && Contains(region, row))
            .OrderBy(row => row.StartSeq).Select(row => new InterestingRowItem(row, row.Name)).ToList();
        RegionPanel.Visibility = Visibility.Visible;
    }

    // Shows a page of the selected tick's events, or of the selected interesting row's thread in its span, in the
    // selected bucket that match the filter. When the tick grew, a full page stays as it is, since events are only added
    // after it, and a last page is read again keeping the selection.
    private async void ShowEvents(int start, bool grew = false)
    {
        eventsRead?.Cancel();
        offset = start;
        var span = (selected as InterestingRowItem)?.Row;
        var frame = (selected as FrameRow)?.Frame;
        if ((span is null && frame is null) || path is not { } file)
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
        var scope = span is not null ? $"span {span.ThreadId} {span.StartSeq} {span.StopSeq}" : $"frame {frame!.FrameId}";
        var bucket = Buckets.SelectedItem?.Tag as string;
        var filter = string.IsNullOrWhiteSpace(FilterBox.Text) ? null : FilterBox.Text;
        // Only the page that is shown, not one still being read, can be kept.
        var pageIsFull = grew && shownPage == (scope, bucket, filter, start) && Events.Count == PageSize;
        var keep = grew ? EventList.SelectedIndex : -1;
        try
        {
            var (total, first, page) = await Task.Run(() =>
            {
                var count = span is { } row
                    ? SessionFile.CountSpanEvents(file, row.ThreadId, row.StartSeq, row.StopSeq, bucket, filter)
                    : SessionFile.CountFrameEvents(file, frame!.FrameId, bucket, filter);
                if (pageIsFull)
                {
                    return (count, start, (List<FrameEventRow>?)null);
                }

                // Past the end, for example in a bucket with fewer events, the last page is shown.
                var first = start >= count && count > 0 ? (int)((count - 1) / PageSize * PageSize) : start;
                var events = new List<FrameEventRow>(PageSize);
                if (span is { } spanRow)
                {
                    SessionFile.ReadSpanEvents(file, spanRow.ThreadId, spanRow.StartSeq, spanRow.StopSeq, bucket, filter, first, PageSize,
                        (profilerEvent, eventBucket, mentions) => events.Add(new(profilerEvent, eventBucket, mentions)), cancellation.Token);
                }
                else
                {
                    SessionFile.ReadFrameEvents(file, frame!.FrameId, bucket, filter, first, PageSize,
                        (profilerEvent, eventBucket, mentions) => events.Add(new(profilerEvent, eventBucket, mentions)), cancellation.Token);
                }

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

                shownPage = (scope, bucket, filter, first);
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
        ObjectLinks.ItemsSource = row?.Objects;
        ObjectLinksPanel.Visibility = row?.Objects.Count > 0 ? Visibility.Visible : Visibility.Collapsed;
    }

    private void ObjectLink_Click(object sender, RoutedEventArgs args) =>
        ObjectRequested?.Invoke(this, (ObjectMention)((FrameworkElement)sender).Tag);

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
