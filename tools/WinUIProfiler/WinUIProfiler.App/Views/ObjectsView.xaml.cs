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

// The Objects tab: the UI elements and composition visuals of a session file and the history of the selected one, the
// events that mention it. Like the Frames tab it reads the file, so it shows a session being recorded and a saved one the
// same way. It reads only while it is shown.
public sealed partial class ObjectsView : UserControl
{
    private const int PageSize = 100;

    private readonly Microsoft.Windows.ApplicationModel.Resources.ResourceLoader strings = new();
    private readonly DispatcherQueueTimer refreshTimer;
    private readonly DispatcherQueueTimer searchTimer;
    private readonly Dictionary<long, ObjectRow> rows = [];
    private List<ObjectRow> all = [];
    private ObservableCollection<ObjectRow> shownRows = [];
    private SessionSource? source;
    private string? path;
    // Counts the files shown, so a read of an earlier one, or of this one before it was reopened, is dropped.
    private int shown;
    // The stored events and where the file was complete from at the last read; new events or trimming change them.
    private (long Events, long? CompleteFromSeq)? readStamp;
    private bool reading;
    private bool readAgain;
    private string? search;
    // The kind the list shows, or null for both.
    private ObjectKind? kindFilter;
    private ObjectRow? selected;
    // An object to select once the list is read.
    private long? pending;
    private int offset;
    // The object and offset of the history page shown.
    private (long ObjectId, int Offset)? shownPage;
    private CancellationTokenSource? historyRead;

    public ObjectsView()
    {
        InitializeComponent();
        refreshTimer = DispatcherQueue.CreateTimer();
        refreshTimer.Interval = TimeSpan.FromSeconds(1);
        refreshTimer.Tick += (_, _) => _ = RefreshAsync();
        searchTimer = DispatcherQueue.CreateTimer();
        searchTimer.Interval = TimeSpan.FromMilliseconds(300);
        searchTimer.IsRepeating = false;
        searchTimer.Tick += (_, _) => ApplyFilter();
        ObjectList.ItemsSource = shownRows;
        // Hooked up after the first selection, which shows both kinds.
        KindFilter.SelectedIndex = 0;
        KindFilter.SelectionChanged += (_, _) => ApplyFilter();
        Loaded += (_, _) =>
        {
            UpdateTimer();
            if (selected is not null)
            {
                ObjectList.ScrollIntoView(selected);
            }

            _ = RefreshAsync();
        };
        Unloaded += (_, _) => refreshTimer.Stop();
        Show(null);
    }

    public ObservableCollection<ObjectEventRow> History { get; } = [];

    // The window that owns the file picker.
    public WindowId Owner { get; set; }

    // The file to show, re-read every second while the connection writes it and the tab is shown.
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

    // Selects an object by ID, reading the file first if the object is newer than the last read.
    public void ShowObject(long objectId)
    {
        NotFound.IsOpen = false;
        pending = objectId;
        if (rows.ContainsKey(objectId))
        {
            ShowPending();
        }
        else
        {
            _ = RefreshAsync();
        }
    }

    // Selects the object a snapshot node was when the snapshot was taken.
    public async void ShowSnapshotObject(SnapshotObject node)
    {
        NotFound.IsOpen = false;
        if (path is not { } file)
        {
            ShowNotFound(Text("FramesNoFile"));
            return;
        }

        try
        {
            var (info, objectId) = await Task.Run(() =>
            {
                var info = SessionFile.ReadInfo(file);
                return (info, info.ProcessId != node.ProcessId ? null
                    : node.Kind == ObjectKind.Visual ? SessionFile.FindSnapshotVisual(file, node.Addresses[0], node.Qpc)
                    : SessionFile.FindSnapshotElement(file, node.Addresses, node.Qpc));
            });
            if (file != path)
            {
                return;
            }

            if (info.ProcessId != node.ProcessId)
            {
                ShowNotFound(Format("SnapshotOtherProcessFormat", node.ProcessId, info.ProcessId));
            }
            else if (objectId is { } id)
            {
                ShowObject(id);
            }
            else
            {
                ShowNotFound(Format(node.Kind == ObjectKind.Visual ? "SnapshotVisualNotFoundFormat" : "SnapshotElementNotFoundFormat",
                    node.Description));
            }
        }
        catch (Exception error) when (error is not OutOfMemoryException)
        {
            ShowError(error);
        }
    }

    private string Text(string key) => strings.GetString(key);
    private string Format(string key, params object?[] values) => string.Format(CultureInfo.CurrentCulture, Text(key), values);

    private void Source_Changed(object? sender, EventArgs args)
    {
        if (source!.Path != path)
        {
            Show(source.Path);
        }

        UpdateTimer();
        if (!source.Live)
        {
            // One last read for the events saved when the connection ended.
            _ = RefreshAsync();
        }
    }

    private void UpdateTimer()
    {
        if (source?.Live == true && IsLoaded)
        {
            refreshTimer.Start();
        }
        else
        {
            refreshTimer.Stop();
        }
    }

    private void Show(string? sessionPath)
    {
        path = sessionPath;
        shown++;
        readStamp = null;
        pending = null;
        rows.Clear();
        all = [];
        shownRows.Clear();
        ReadError.IsOpen = false;
        NotFound.IsOpen = false;
        FileSummary.Text = path is null ? Text("FramesNoFile") : Path.GetFileName(path);
        ToolTipService.SetToolTip(FileSummary, path);
        SelectObject(null);
        _ = RefreshAsync();
    }

    private async Task RefreshAsync()
    {
        if (reading)
        {
            readAgain = true;
            return;
        }

        if (path is not { } file || !IsLoaded)
        {
            return;
        }

        reading = true;
        try
        {
            var current = shown;
            var stamp = readStamp;
            var (info, objects) = await Task.Run(() =>
            {
                var info = SessionFile.ReadInfo(file);
                return (info, stamp == (info.StoredEvents, info.CompleteFromSeq) ? null : SessionFile.ReadObjects(file));
            });
            if (current != shown)
            {
                return;
            }

            if (objects is not null)
            {
                readStamp = (info.StoredEvents, info.CompleteFromSeq);
                Merge(objects, trimmed: stamp is { } last && last.CompleteFromSeq != info.CompleteFromSeq);
            }

            var visuals = all.Count(row => row.IsVisual);
            FileSummary.Text = Format(info.CompleteFromSeq is null ? "ObjectsFileFormat" : "ObjectsFileTrimmedFormat",
                Path.GetFileName(file), all.Count - visuals, visuals);
            ReadError.IsOpen = false;
            if (!readAgain && pending is { } id && !rows.ContainsKey(id))
            {
                pending = null;
                ShowNotFound(Text("ObjectNotFound"));
            }

            ShowPending();
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

    // Updates the rows in place, so the list keeps its scroll position and selection. Objects are in the order they were
    // first seen, which only trimming changes.
    private void Merge(IReadOnlyList<SessionObject> objects, bool trimmed)
    {
        var next = new List<ObjectRow>(objects.Count);
        var seen = new HashSet<long>();
        foreach (var item in objects)
        {
            seen.Add(item.ObjectId);
            if (!rows.TryGetValue(item.ObjectId, out var row))
            {
                rows.Add(item.ObjectId, row = new(item));
            }
            else if (row.Item != item)
            {
                var grew = row.Item.Events != item.Events;
                row.Update(item);
                if (row == selected)
                {
                    UpdateTitle();
                    if (grew)
                    {
                        // Trimming removes events before the page, so the page is read again.
                        ShowHistory(offset, grew: !trimmed);
                    }
                }
            }

            next.Add(row);
        }

        foreach (var id in rows.Keys.Where(id => !seen.Contains(id)).ToList())
        {
            rows.Remove(id);
        }

        all = next;
        var keep = selected;
        var target = all.Where(Matches).ToList();
        var wanted = target.ToHashSet();
        for (var i = shownRows.Count - 1; i >= 0; i--)
        {
            if (!wanted.Contains(shownRows[i]))
            {
                shownRows.RemoveAt(i);
            }
        }

        var present = shownRows.ToHashSet();
        for (var i = 0; i < target.Count; i++)
        {
            if (i < shownRows.Count && shownRows[i] == target[i])
            {
                continue;
            }

            if (present.Contains(target[i]))
            {
                shownRows.Remove(target[i]);
            }

            shownRows.Insert(i, target[i]);
        }

        if (keep is not null && selected != keep && wanted.Contains(keep))
        {
            ObjectList.SelectedItem = keep;
        }
    }

    private bool Matches(ObjectRow row) =>
        (kindFilter is not { } kind || row.Item.Kind == kind) && (search is not { } text || row.Matches(text));

    private void ApplyFilter()
    {
        search = string.IsNullOrWhiteSpace(SearchBox.Text) ? null : SearchBox.Text.Trim();
        kindFilter = KindFilter.SelectedIndex switch
        {
            1 => ObjectKind.Element,
            2 => ObjectKind.Visual,
            _ => null,
        };
        var keep = selected;
        ObjectList.ItemsSource = shownRows = [.. all.Where(Matches)];
        if (keep is not null && shownRows.Contains(keep))
        {
            ObjectList.SelectedItem = keep;
            ObjectList.ScrollIntoView(keep);
        }
    }

    // Selects the pending object, clearing the search and the kind filter if they hide it.
    private void ShowPending()
    {
        if (pending is not { } id || !rows.TryGetValue(id, out var row))
        {
            return;
        }

        pending = null;
        if (!shownRows.Contains(row))
        {
            searchTimer.Stop();
            SearchBox.Text = "";
            if (KindFilter.SelectedIndex == 0)
            {
                ApplyFilter();
            }
            else
            {
                // Its SelectionChanged applies the filter.
                KindFilter.SelectedIndex = 0;
            }
        }

        ObjectList.SelectedItem = row;
        ObjectList.ScrollIntoView(row);
    }

    private void ShowNotFound(string message)
    {
        NotFound.Message = message;
        NotFound.IsOpen = true;
    }

    private void ObjectList_SelectionChanged(object sender, SelectionChangedEventArgs args)
    {
        if (ObjectList.SelectedItem != selected)
        {
            SelectObject(ObjectList.SelectedItem as ObjectRow);
        }
    }

    private void SelectObject(ObjectRow? row)
    {
        selected = row;
        UpdateTitle();
        ShowHistory(0);
    }

    private void UpdateTitle()
    {
        var item = selected?.Item;
        ObjectTitle.Text = item is null ? Text("SelectObjectPrompt") : Format("ObjectTitleFormat", selected!.KindText,
            ObjectRow.Describe(item.Kind, item.Type, item.Name), item.ObjectId, selected.Address, item.FirstMilliseconds,
            item.LastMilliseconds);
        ToolTipService.SetToolTip(ObjectTitle, item is null ? null : ObjectTitle.Text);
    }

    // Shows a page of the selected object's history. When it grew, a full page stays as it is, since events are only
    // added after it, and a last page is read again keeping the selection.
    private async void ShowHistory(int start, bool grew = false)
    {
        historyRead?.Cancel();
        offset = start;
        if (selected is not { } row || path is not { } file)
        {
            History.Clear();
            shownPage = null;
            EventDetails.Text = Text("SelectEventPrompt");
            Position.Text = "";
            PreviousButton.IsEnabled = NextButton.IsEnabled = false;
            return;
        }

        using var cancellation = new CancellationTokenSource();
        historyRead = cancellation;
        var objectId = row.Item.ObjectId;
        // Only the page that is shown, not one still being read, can be kept.
        var pageIsFull = grew && shownPage == (objectId, start) && History.Count == PageSize;
        var keep = grew ? HistoryList.SelectedIndex : -1;
        try
        {
            var (total, first, page) = await Task.Run(() =>
            {
                var count = SessionFile.CountTimeline(file, objectId);
                if (pageIsFull)
                {
                    return (count, start, (List<ObjectEventRow>?)null);
                }

                // Past the end, after trimming, the last page is shown.
                var first = start >= count && count > 0 ? (int)((count - 1) / PageSize * PageSize) : start;
                var events = new List<ObjectEventRow>(PageSize);
                SessionFile.ReadTimeline(file, objectId, first, PageSize,
                    (profilerEvent, fields, frame) => events.Add(new(profilerEvent, fields, frame)), cancellation.Token);
                return (count, first, events);
            }, cancellation.Token);
            if (cancellation.IsCancellationRequested)
            {
                return;
            }

            offset = first;
            if (page is not null)
            {
                History.Clear();
                foreach (var item in page)
                {
                    History.Add(item);
                }

                shownPage = (objectId, first);
                if (keep >= 0 && keep < History.Count)
                {
                    HistoryList.SelectedIndex = keep;
                }
            }

            Position.Text = total == 0 ? Text("PositionNoEvents")
                : Format("PositionFormat", first + 1, first + History.Count, total);
            PreviousButton.IsEnabled = first > 0;
            NextButton.IsEnabled = first + History.Count < total;
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
            if (historyRead == cancellation)
            {
                historyRead = null;
            }
        }
    }

    private void Previous_Click(object sender, RoutedEventArgs args) => ShowHistory(Math.Max(0, offset - PageSize));

    private void Next_Click(object sender, RoutedEventArgs args) => ShowHistory(offset + PageSize);

    private void SearchBox_TextChanged(object sender, TextChangedEventArgs args)
    {
        searchTimer.Stop();
        searchTimer.Start();
    }

    private void HistoryList_SelectionChanged(object sender, SelectionChangedEventArgs args) =>
        EventDetails.Text = (HistoryList.SelectedItem as ObjectEventRow)?.Details ?? Text("SelectEventPrompt");

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
