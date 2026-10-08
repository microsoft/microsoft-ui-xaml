// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Threading.Tasks;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Tabular;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Media;
using TableViewSampleApp.Controls;
using TableViewSampleApp.Helpers;
using TableViewSampleApp.Models;
using TableViewSampleApp.Services;
using Windows.System;
using SortDirection = Microsoft.UI.Xaml.Controls.Tabular.SortDirection;
using TableView = Microsoft.UI.Xaml.Controls.Tabular.TableView;

namespace TableViewSampleApp.Pages;

/// <summary>
/// File Explorer scenario: a folder listing that opens in the sample's install folder, reads
/// each folder off the UI thread, keeps folders first through computed SortMemberPath keys, and
/// filters and groups one TableViewSource in place. Enter opens a folder, Backspace goes up.
/// </summary>
public sealed partial class FileExplorerPage : SamplePageBase
{
    private readonly Stack<string> _history = new();
    private string _currentDir = string.Empty;
    private int _navigationVersion;

    public FileExplorerPage()
    {
        // <snippet>
        Source = TableViewSource.From(Entries);    // created once; filtered and grouped in place
        InitializeComponent();

        FileTable.HeadersVisibility = TableViewHeadersVisibility.Column;
        FileTable.GridLinesVisibility = TableViewGridLinesVisibility.Horizontal;
        FileTable.Density = TableViewDensity.Compact;
        // handledEventsToo: the table consumes Enter for its own row navigation.
        FileTable.AddHandler(KeyDownEvent, new KeyEventHandler(OnTableKeyDown), handledEventsToo: true);
        // </snippet>

        Shaping.ProbeLimit = () => Entries.Count + 64;
        InitializeSample(Status, Shaping.Attach(FileTable, Source, (row, key) => KeyOf(row as FileSystemEntry, key)));
        TrackLifetime(OpenInitialFolder);
    }

    // Folder contents replace the items of this one collection; the source is never rebuilt.
    public ObservableCollection<FileSystemEntry> Entries { get; } = new();

    public TableViewSource Source { get; }

    private void OpenInitialFolder()
    {
        if (_currentDir.Length == 0)
        {
            _ = NavigateAsync(FileSystemBrowser.InitialDirectory, addToHistory: false, verb: "Opened the app folder");
        }
    }

    // <snippet>
    // ---- Navigation (the folder is read on a background thread) ---------------------------

    private async Task NavigateAsync(string path, bool addToHistory, string verb)
    {
        var version = ++_navigationVersion;
        var listing = await Task.Run(() => FileSystemBrowser.Read(path));

        // A newer navigation started while this one was reading; drop the stale result.
        if (version != _navigationVersion)
        {
            return;
        }

        if (listing.Error is not null)
        {
            AddressBar.Text = _currentDir;
            SetLastAction(string.Format(CultureInfo.CurrentCulture, "Cannot open {0}: {1}", listing.FullPath, listing.Error));
            return;
        }

        if (addToHistory && _currentDir.Length > 0 && !string.Equals(_currentDir, listing.FullPath, StringComparison.OrdinalIgnoreCase))
        {
            _history.Push(_currentDir);
        }

        _currentDir = listing.FullPath;
        var focusState = TableFocusState();
        Entries.Clear();
        foreach (var entry in listing.Entries)
        {
            Entries.Add(entry);
        }

        AddressBar.Text = _currentDir;
        BackButton.IsEnabled = _history.Count > 0;
        UpButton.IsEnabled = Directory.GetParent(_currentDir) is not null;
        if (focusState != FocusState.Unfocused)
        {
            FocusFirstRowAfterLayout(focusState);
        }

        SetLastAction(string.Format(CultureInfo.CurrentCulture, "{0}: {1} ({2:N0} items)", verb, DisplayName(_currentDir), Entries.Count));
    }
    // </snippet>

    // How focus sits in the table, or Unfocused when it is elsewhere on the page.
    private FocusState TableFocusState()
    {
        var focused = XamlRoot is null ? null : FocusManager.GetFocusedElement(XamlRoot) as DependencyObject;
        for (var node = focused; node is not null; node = VisualTreeHelper.GetParent(node))
        {
            if (node == FileTable)
            {
                return focused is Control { FocusState: not FocusState.Unfocused } control ? control.FocusState : FocusState.Programmatic;
            }
        }

        return FocusState.Unfocused;
    }

    // Refilling the collection recycles every row, so the focused row container is left showing
    // nothing. Like File Explorer, move focus and selection to the first row of the new folder.
    private void FocusFirstRowAfterLayout(FocusState focusState)
    {
        void OnLayoutUpdated(object? sender, object e)
        {
            FileTable.LayoutUpdated -= OnLayoutUpdated;
            if (FindFirstRealizedRow(FileTable) is { } row)
            {
                SampleShaping.Reselect(FileTable, row.DataContext, Entries.Count + 8);
                row.Focus(focusState);
            }
        }

        FileTable.LayoutUpdated += OnLayoutUpdated;
    }

    private static TableViewRow? FindFirstRealizedRow(DependencyObject parent)
    {
        TableViewRow? first = null;
        var firstTop = double.MaxValue;
        Collect(parent);
        return first;

        // Realized containers are not in visual order, so pick the top-most visible one.
        void Collect(DependencyObject node)
        {
            var count = VisualTreeHelper.GetChildrenCount(node);
            for (var i = 0; i < count; i++)
            {
                var child = VisualTreeHelper.GetChild(node, i);
                if (child is TableViewRow { Visibility: Visibility.Visible, DataContext: FileSystemEntry } row)
                {
                    var top = row.TransformToVisual(parent as UIElement).TransformPoint(default).Y;
                    if (top >= 0 && top < firstTop)
                    {
                        firstTop = top;
                        first = row;
                    }
                }
                else
                {
                    Collect(child);
                }
            }
        }
    }

    private static string DisplayName(string path) =>
        Path.GetFileName(path) is { Length: > 0 } name ? name : path;

    private void OpenSelectedFolder()
    {
        switch (FileTable.SelectedItem)
        {
            case FileSystemEntry { IsFolder: true } folder:
                _ = NavigateAsync(folder.FullPath, addToHistory: true, verb: "Opened");
                break;
            case FileSystemEntry file:
                SetLastAction(string.Format(CultureInfo.CurrentCulture, "{0} is a file; only folders open here.", file.Name));
                break;
            default:
                SetLastAction("No row selected.");
                break;
        }
    }

    private void GoUp()
    {
        if (_currentDir.Length > 0 && Directory.GetParent(_currentDir) is { } parent)
        {
            _ = NavigateAsync(parent.FullName, addToHistory: true, verb: "Up to");
        }
        else
        {
            SetLastAction("Already at the top of the drive.");
        }
    }

    private void OnTableKeyDown(object sender, KeyRoutedEventArgs e)
    {
        if (e.Key == VirtualKey.Enter)
        {
            e.Handled = true;
            OpenSelectedFolder();
        }
        else if (e.Key == VirtualKey.Back)
        {
            e.Handled = true;
            GoUp();
        }
    }

    private void OnRowDoubleTapped(object sender, DoubleTappedRoutedEventArgs e) => OpenSelectedFolder();

    private void OnBackClick(object sender, RoutedEventArgs e)
    {
        if (_history.Count > 0)
        {
            _ = NavigateAsync(_history.Pop(), addToHistory: false, verb: "Back to");
        }
    }

    private void OnUpClick(object sender, RoutedEventArgs e) => GoUp();

    private void OnRefreshClick(object sender, RoutedEventArgs e) =>
        _ = NavigateAsync(_currentDir, addToHistory: false, verb: "Re-read");

    private void OnAddressBarKeyDown(object sender, KeyRoutedEventArgs e)
    {
        if (e.Key != VirtualKey.Enter)
        {
            return;
        }

        e.Handled = true;
        var path = AddressBar.Text?.Trim();
        if (string.IsNullOrEmpty(path))
        {
            SetLastAction("Type a folder path first.");
            return;
        }

        // An invalid or unreadable path is reported in Last action by NavigateAsync.
        _ = NavigateAsync(path, addToHistory: true, verb: "Opened");
    }

    // <snippet>
    // ---- Filter (TableViewSource.Filter) ---------------------------------------------------

    private void OnFilterChanged(object sender, SelectionChangedEventArgs e)
    {
        if (FilterSelector is null)
        {
            return;
        }

        var filter = SampleShaping.SelectedTag(FilterSelector, "all");
        switch (filter)
        {
            case "folders":
                Source.Filter(item => item is FileSystemEntry { IsFolder: true });
                break;
            case "files":
                Source.Filter(item => item is FileSystemEntry { IsFolder: false });
                break;
            case "none":
                Source.Filter(_ => false);
                break;
            default:
                Source.ClearFilter();
                break;
        }

        // Fires once during InitializeComponent (SelectedIndex="0"); report user changes only.
        if (IsLoaded)
        {
            SetLastAction(string.Format(CultureInfo.CurrentCulture, "Filter -> {0} ({1:N0} rows)", SampleShaping.Label(FilterSelector), VisibleCount()));
        }
    }

    private int VisibleCount() => SampleShaping.SelectedTag(FilterSelector, "all") switch
    {
        "folders" => Entries.Count(entry => entry.IsFolder),
        "files" => Entries.Count(entry => !entry.IsFolder),
        "none" => 0,
        _ => Entries.Count,
    };
    // </snippet>

    // ---- Actions ------------------------------------------------------------------------

    private void OnOpenSelectedClick(object sender, RoutedEventArgs e) => OpenSelectedFolder();

    private void OnRemoveSelectedClick(object sender, RoutedEventArgs e)
    {
        if (FileTable.SelectedItem is not FileSystemEntry entry)
        {
            SetLastAction("No row selected.");
            return;
        }

        // Listing only: nothing is deleted on disk, and Refresh brings the row back.
        Entries.Remove(entry);
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Removed {0} from the listing (not from disk)", entry.Name));
    }

    private void OnGoToAppFolderClick(object sender, RoutedEventArgs e) =>
        _ = NavigateAsync(FileSystemBrowser.InitialDirectory, addToHistory: true, verb: "Opened the app folder");

    private void OnTableSorted(TableView sender, TableViewSortedEventArgs args)
    {
        SetLastAction(args.Column is null || args.Direction == SortDirection.None
            ? "Sort cleared"
            : string.Format(
                CultureInfo.CurrentCulture,
                "Sorted by {0}, {1}{2}",
                args.Column.Header,
                args.Direction == SortDirection.Descending ? "descending" : "ascending",
                args.Column == DateModifiedColumn ? string.Empty : ", folders kept together"));
    }

    private void OnSelectionChanged(TableView sender, SelectionChangedEventArgs args)
    {
        if (SampleShaping.IsReselecting)
        {
            return;
        }

        RefreshReadouts();
    }

    // <snippet>
    // Groups appear in the order of their first row, and a sort declared BEFORE GroupBy orders
    // them. Sorting newest first makes the Date modified buckets read Today, Yesterday, ... and
    // the Date modified header shows that sort.
    protected override void OnShapingApplying(ShapingApplyingEventArgs e)
    {
        if (e.Mode == "grouped" && e.Key == nameof(FileSystemEntry.DateModified))
        {
            FileTable.SortByColumn(DateModifiedColumn, SortDirection.Descending);
        }
    }
    // Group key resolution for this page's model (FIX-PLAN §1.6 R2). Never returns a blank key.
    private static object KeyOf(FileSystemEntry? entry, string key)
    {
        if (entry is null)
        {
            return SampleShaping.NoneKey;
        }

        var value = key switch
        {
            nameof(FileSystemEntry.IsFolder) => entry.IsFolder ? "Folders" : "Files",
            nameof(FileSystemEntry.DateModified) => ModifiedBucket(entry.DateModified),
            _ => entry.TypeDisplay,
        };

        return string.IsNullOrWhiteSpace(value) ? SampleShaping.NoneKey : value;
    }

    private static string ModifiedBucket(DateTimeOffset modified)
    {
        var days = (DateTime.Now.Date - modified.LocalDateTime.Date).TotalDays;
        return days switch
        {
            < 1 => "Today",
            < 2 => "Yesterday",
            < 8 => "Earlier this week",
            < 31 => "Earlier this month",
            < 366 => "Earlier this year",
            _ => "A long time ago",
        };
    }
    // </snippet>
}
