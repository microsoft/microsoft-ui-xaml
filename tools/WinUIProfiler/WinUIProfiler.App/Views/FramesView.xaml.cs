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
// It reads the file, not the live pipeline, so it shows a session being recorded and a saved one the same way. The list,
// the Show filter, a region's rows and the launch summary come from the file's views (FrameList, InterestingRows,
// LaunchSummary), the same rows the CLI and other readers get; this tab only shows them.
public sealed partial class FramesView : UserControl
{
    private const int PageSize = 100;
    // The LaunchSummary view lists at most this many ticks; once it has them, it does not change.
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
    // The rows read, by tick and by interesting row, so each read reuses them and the selection and scroll position stay.
    private Dictionary<long, FrameRow> ticks = [];
    private Dictionary<(InterestingRowType Type, long StartSeq), InterestingRowItem> interestingItems = [];
    // Every row the list can show, in the FrameList view's order, and each row's item in it, for the Show filter.
    private List<object> all = [];
    private Dictionary<object, FrameListItem> listed = [];
    private IReadOnlyList<LaunchFrame> launch = [];
    private ObservableCollection<object> shownRows = [];
    private Filter filter;
    private SessionSource? source;
    private string? path;
    // Counts the files shown, so a read of an earlier one, or of this one before it was reopened, is dropped.
    private int shown;
    // How many events the last read saw, and where the file was complete from then: trimming the oldest events changes it.
    private long? storedEvents;
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
        ticks = [];
        interestingItems = [];
        all = [];
        listed = [];
        launch = [];
        fileRead = false;
        FrameList.ItemsSource = shownRows = [];
        UpdateEmpty();
        storedEvents = null;
        trimmedBefore = null;
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
            var current = shown;
            var known = storedEvents;
            var readLaunch = launch.Count < LaunchFrames;
            var (info, list, launchFrames) = await Task.Run(() =>
            {
                var info = SessionFile.ReadInfo(file);
                // Without new events, the list and the summary are the same as last time.
                return info.StoredEvents == known
                    ? (info, null, null)
                    : (info, SessionFile.ReadFrameList(file), readLaunch ? SessionFile.ReadLaunchSummary(file) : null);
            });
            if (current != shown)
            {
                return;
            }

            if (fileRead && info.CompleteFromSeq != trimmedBefore)
            {
                // The writer deleted the oldest events, and the ticks left without any, so read the file again.
                var keep = (selected as FrameRow)?.Frame.FrameId;
                Show(file);
                reselect = keep;
                return;
            }

            trimmedBefore = info.CompleteFromSeq;
            if (list is not null)
            {
                storedEvents = info.StoredEvents;
                launch = launchFrames ?? launch;
                Update(list);
                if (selected is InterestingRowItem { IsRegion: true } region)
                {
                    ShowRegionRows(region);
                }
            }

            fileRead = true;
            UpdateEmpty();
            if (reselect is { } id)
            {
                reselect = null;
                if (ticks.GetValueOrDefault(id) is { } row && shownRows.Contains(row))
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

    // Takes the list the FrameList view gives. A tick or interesting row read before keeps its object, updated in place,
    // so the selection and the scroll position stay. A region's tick gets the region, for its tint.
    private void Update(IReadOnlyList<FrameListItem> list)
    {
        var nextTicks = new Dictionary<long, FrameRow>();
        var nextItems = new Dictionary<(InterestingRowType Type, long StartSeq), InterestingRowItem>();
        var regionItems = new Dictionary<long, InterestingRowItem>();
        all = new(list.Count);
        listed = new(list.Count);
        foreach (var item in list)
        {
            object row;
            if (item.Tick is { } frame)
            {
                if (!ticks.TryGetValue(frame.FrameId, out var tick))
                {
                    tick = new(frame);
                }
                else if (tick.Frame != frame)
                {
                    var grew = tick.Frame.Events != frame.Events;
                    tick.Update(frame);
                    if (tick == selected)
                    {
                        UpdateTitle();
                        if (grew)
                        {
                            ShowEvents(offset, grew: true);
                        }
                    }
                }

                nextTicks[frame.FrameId] = tick;
                row = tick;
            }
            else
            {
                var interesting = item.Row!;
                var key = (interesting.Type, interesting.StartSeq);
                if (!interestingItems.TryGetValue(key, out var rowItem) || rowItem.Row != interesting)
                {
                    rowItem = new(interesting, interesting.Type == InterestingRowType.Region
                        ? Format("RegionTextFormat", interesting.Trigger ?? interesting.Name, interesting.StartMilliseconds,
                            interesting.StopMilliseconds, interesting.DurationMilliseconds)
                        : Format("InterestingTextFormat", interesting.Name, interesting.ThreadId, interesting.StartMilliseconds,
                            interesting.StopMilliseconds, interesting.DurationMilliseconds));
                }

                nextItems[key] = rowItem;
                if (rowItem.IsRegion && item.RegionId is { } regionId)
                {
                    regionItems[regionId] = rowItem;
                }

                row = rowItem;
            }

            all.Add(row);
            listed[row] = item;
        }

        ticks = nextTicks;
        interestingItems = nextItems;
        foreach (var tick in ticks.Values)
        {
            var region = listed[tick].RegionId is { } regionId ? regionItems.GetValueOrDefault(regionId) : null;
            if (tick.Region != region)
            {
                tick.Region = region;
                // The tick may be listed already, without the region's tint.
                if (FrameList.ContainerFromItem(tick) is ListViewItem container)
                {
                    container.Style = Styles.SelectStyle(tick, container);
                }
            }
        }

        Sync();
    }

    private bool Shows(object item) => filter switch
    {
        Filter.Interesting => listed[item].InInteresting,
        Filter.Slow => listed[item].InSlow,
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

    // Shows when the first drawing ticks of the first UI thread ended (the LaunchSummary view): from process start if the
    // recording saw it, otherwise from the start of the recording. The view has none for a trimmed file.
    private void ShowSummary(SessionFileInfo info)
    {
        FileSummary.Text = Format(info.CompleteFromSeq is null ? "FramesFileFormat" : "FramesFileTrimmedFormat",
            Path.GetFileName(path), info.Frames);
        var items = string.Join(" · ", launch.Select(frame => Format("LaunchItemFormat", frame.FrameNumber, frame.Milliseconds)));
        SetLine(LaunchSummary, items.Length == 0 ? ""
            : Format(launch[0].FromProcessStart ? "LaunchFromProcessFormat" : "LaunchFromRecordingFormat", items));
    }

    // A one-line text that may be cut off; its tooltip shows all of it.
    private static void SetLine(TextBlock line, string text)
    {
        line.Text = text;
        ToolTipService.SetToolTip(line, text.Length == 0 ? null : text);
    }

    private void FrameList_SelectionChanged(object sender, SelectionChangedEventArgs args) => SelectRow(FrameList.SelectedItem);

    // The list skips its style selector for a container whose Style the app set, such as a tick whose region arrived after
    // the tick was listed. So the style is applied again each time a container gets a row.
    private void FrameList_ContainerContentChanging(ListViewBase sender, ContainerContentChangingEventArgs args)
    {
        if (!args.InRecycleQueue)
        {
            args.ItemContainer.Style = Styles.SelectStyle(args.Item, args.ItemContainer);
        }
    }

    private StyleSelector Styles => (StyleSelector)Resources["FrameItemStyles"];

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

    // Lists the interesting rows in the selected region: those of the InterestingRows view with its RegionId.
    private async void ShowRegionRows(InterestingRowItem? item)
    {
        if (item is not { IsRegion: true, Row: var region } || path is not { } file)
        {
            RegionPanel.Visibility = Visibility.Collapsed;
            RegionRows.ItemsSource = null;
            return;
        }

        var current = shown;
        try
        {
            var rows = await Task.Run(() => SessionFile.ReadInterestingRows(file, region.RegionId));
            if (current != shown || selected != item)
            {
                return;
            }

            RegionRows.ItemsSource = rows.Where(row => row.Type != InterestingRowType.Region)
                .Select(row => new InterestingRowItem(row, row.Name)).ToList();
            RegionPanel.Visibility = Visibility.Visible;
        }
        catch (Exception error) when (error is not OutOfMemoryException)
        {
            ShowError(error);
        }
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
