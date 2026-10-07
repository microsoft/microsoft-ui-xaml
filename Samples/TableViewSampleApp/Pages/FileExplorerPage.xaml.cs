// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Globalization;
using System.IO;
using System.Linq;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Tabular;
using Microsoft.UI.Xaml.Input;
using TableViewSampleApp.Models;
using TableViewSampleApp.Services;
using Windows.System;
using TableView = Microsoft.UI.Xaml.Controls.Tabular.TableView;
using TableViewColumn = Microsoft.UI.Xaml.Controls.Tabular.TableViewColumn;
using TableViewSortedEventArgs = Microsoft.UI.Xaml.Controls.Tabular.TableViewSortedEventArgs;
using TableViewSelectionChangedEventArgs = Microsoft.UI.Xaml.Controls.SelectionChangedEventArgs;
using TableViewDensity = Microsoft.UI.Xaml.Controls.Tabular.TableViewDensity;
using TableViewGridLinesVisibility = Microsoft.UI.Xaml.Controls.Tabular.TableViewGridLinesVisibility;
// #44 enum unification: TableViewSortDirection was removed in favor of the shared Data enum.
using TableViewSortDirection = Microsoft.UI.Xaml.Controls.Tabular.SortDirection;

using TableViewSampleApp.Data;

namespace TableViewSampleApp.Pages;

public sealed partial class FileExplorerPage : Page
{
    private static readonly Dictionary<string, Func<FileSystemEntry, IComparable?>> s_keySelectors =
        new(StringComparer.Ordinal)
        {
            ["Name"] = entry => entry.Name,
            ["TypeDisplay"] = entry => entry.TypeDisplay,
            ["DateModified"] = entry => entry.DateModified,
            ["Size"] = entry => entry.Size,
        };

    private readonly FileSystemBrowser _browser = new();
    private readonly Stack<string> _history = new();
    private string _currentDir = string.Empty;

    // Reshaped IN PLACE: Filter / GroupBy mutate and return the same TableViewSource
    // (TableViewSource.cpp:49-50). Built ONCE over the Entries collection, so a shaping change
    // never re-enters FileSystemBrowser.GetEntries and never touches the file system.
    private TableViewSource? _source;
    private string _mode = "flat";          // requested
    private string _appliedMode = "flat";   // applied
    private string _groupKey = "Type";
    private string _filter = "all";

    public FileExplorerPage()
    {
        InitializeComponent();
        FileTable.HeadersVisibility = TableViewHeadersVisibility.Column;
        FileTable.GridLinesVisibility = TableViewGridLinesVisibility.Horizontal;
        FileTable.Density = TableViewDensity.Compact;
        Loaded += OnPageLoaded;
    }

    public ObservableCollection<FileSystemEntry> Entries { get; } = new();

    private void OnPageLoaded(object sender, RoutedEventArgs e)
    {
        if (string.IsNullOrEmpty(_currentDir))
        {
            Navigate(FileSystemBrowser.GetInitialDirectory(), addToHistory: false);
        }
    }

    private void Navigate(string directoryPath) => Navigate(directoryPath, addToHistory: true);

    private void Navigate(string directoryPath, bool addToHistory)
    {
        if (string.IsNullOrWhiteSpace(directoryPath))
        {
            return;
        }

        string fullPath;
        try
        {
            fullPath = Path.GetFullPath(Environment.ExpandEnvironmentVariables(directoryPath));
        }
        catch
        {
            return;
        }

        if (!Directory.Exists(fullPath))
        {
            return;
        }

        if (addToHistory && !string.IsNullOrEmpty(_currentDir) && !PathsEqual(_currentDir, fullPath))
        {
            _history.Push(_currentDir);
        }

        _currentDir = fullPath;
        RefreshEntries();
    }

    private void RefreshEntries()
    {
        if (FileTable is null)
        {
            return;
        }

        var entries = ApplyActiveSort(_browser.GetEntries(_currentDir)).ToList();
        Entries.Clear();
        foreach (var entry in entries)
        {
            Entries.Add(entry);
        }

        if (_source is null)
        {
            _source = TableViewSource.From(Entries);
            FileTable.ItemsSource = _source;
        }

        ApplyShaping();
        FileTable.DeselectAll();
        AddressBar.Text = _currentDir;
        RefreshStatusText();
        RefreshNavigationButtons();
        RefreshSelectionText();
    }

    private void ApplyShaping()
    {
        if (FileTable is null || _source is null)
        {
            return;
        }

        switch (_filter)
        {
            case "folders":
                _source.Filter(item => item is FileSystemEntry { IsFolder: true });
                break;
            case "files":
                _source.Filter(item => item is FileSystemEntry { IsFolder: false });
                break;
            case "none":
                _source.Filter(_ => false);
                break;
            default:
                _source.ClearFilter();
                break;
        }

        switch (_mode)
        {
            case "grouped":
                var key = _groupKey;
                // TableViewKeySelector receives the ROW ITEM; TableViewIdentitySelector receives
                // the GROUP KEY (TableViewSource.idl:12-16). An item-typed identity lambda returns
                // an empty identity and fails fast with E_INVALIDARG.
                _source.GroupBy(
                    item => (object)GroupValue(item, key),
                    groupKey => groupKey?.ToString() ?? "(none)");
                break;

            // case "hierarchy":
            // case "groupedhierarchy":
            //     Hierarchical rows are not available in this release, and no hierarchy API exists
            //     on TableViewSource or TableView yet, so there is deliberately no call written
            //     here to copy. When the control ships hierarchy support, apply it to this same
            //     source instance alongside the GroupBy stage above so the two compose - folder
            //     rows parenting their children is the natural shape for this page - and drop the
            //     IsEnabled="False" from the matching options in the XAML.
            //     break;

            default:
                _source.ClearGroupBy();
                break;
        }

        // Set ONLY after the shaping call returns.
        _appliedMode = _mode;

        UpdateShapingUi();
        RefreshStatusText();
    }

    private void OnShapingModeChanged(object sender, SelectionChangedEventArgs e)
    {
        if (ShapingModeSelector?.SelectedItem is not ComboBoxItem { Tag: string tag })
        {
            return;
        }

        _mode = tag;
        ApplyShaping();
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Shaping mode -> {0} (no re-enumeration)", _appliedMode == "grouped" ? "Grouped" : "Flat"));
    }

    private void OnShapingGroupKeyChanged(object sender, SelectionChangedEventArgs e)
    {
        if (ShapingGroupKeyCombo?.SelectedItem is not ComboBoxItem { Tag: string tag })
        {
            return;
        }

        _groupKey = tag;
        ApplyShaping();
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Group key -> {0}", GroupKeyLabel(tag)));
    }

    private void OnFilterChanged(object sender, SelectionChangedEventArgs e)
    {
        if (FilterCombo?.SelectedItem is not ComboBoxItem { Tag: string tag })
        {
            return;
        }

        _filter = tag;
        ApplyShaping();
        SetLastAction(tag switch
        {
            "folders" => "Filtered to folders only",
            "files" => "Filtered to files only",
            "none" => "Filtered to zero rows",
            _ => "Filter cleared",
        });
    }

    private void OnExpandAllClick(object sender, RoutedEventArgs e)
    {
        if (_appliedMode != "grouped")
        {
            return;
        }

        FileTable.ExpandAllGroups();
        SetLastAction("Expanded all groups");
    }

    private void OnCollapseAllClick(object sender, RoutedEventArgs e)
    {
        if (_appliedMode != "grouped")
        {
            return;
        }

        FileTable.CollapseAllGroups();
        SetLastAction("Collapsed all groups");
    }

    private void OnRemoveSelectedClick(object sender, RoutedEventArgs e)
    {
        if (FileTable?.SelectedItem is not FileSystemEntry selected)
        {
            return;
        }

        // Listing-only: nothing is deleted on disk. Refresh re-reads the folder.
        Entries.Remove(selected);
        RefreshStatusText();
        RefreshSelectionText();
        UpdateShapingUi();
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Removed \"{0}\" from the listing", selected.Name));
    }

    private void UpdateShapingUi()
    {
        var grouped = _appliedMode == "grouped";
        var hasSelection = FileTable?.SelectedItem is FileSystemEntry;

        if (ShapingGroupKeyCombo is not null)
        {
            ShapingGroupKeyCombo.IsEnabled = grouped;
            ToolTipService.SetToolTip(ShapingGroupKeyCombo, grouped
                ? "Change the GroupBy key while grouped - expansion and selection survive."
                : "Available once Grouped mode is selected.");
        }

        SetGroupActionState(ExpandAllButton, grouped, "Expand every group.");
        SetGroupActionState(CollapseAllButton, grouped, "Collapse every group.");

        if (RemoveRowButton is not null)
        {
            RemoveRowButton.IsEnabled = hasSelection;
            ToolTipService.SetToolTip(RemoveRowButton, hasSelection
                ? "Removes the row from this listing only; its group disappears when it was the last one."
                : "Select a row first.");
        }

        if (AppliedModeText is null)
        {
            return;
        }

        AppliedModeText.Text = grouped
            ? string.Format(CultureInfo.CurrentCulture, "Grouped by {0}", GroupKeyLabel(_groupKey))
            : "Flat (no shaping)";

        var groups = GroupCounts().ToList();
        GroupCountText.Text = !grouped
            ? "(n/a - flat)"
            : groups.Count == 0
                ? "0 (empty result)"
                : string.Format(CultureInfo.CurrentCulture, "{0}: {1}", groups.Count, string.Join(", ", groups.Take(6).Select(g => string.Format(CultureInfo.CurrentCulture, "{0} ({1})", g.Key, g.Count))));
    }

    private static void SetGroupActionState(Button? button, bool grouped, string enabledTip)
    {
        if (button is null)
        {
            return;
        }

        button.IsEnabled = grouped;
        ToolTipService.SetToolTip(button, grouped ? enabledTip : "Available once Grouped mode is selected.");
    }

    private IEnumerable<FileSystemEntry> VisibleEntries() => _filter switch
    {
        "folders" => Entries.Where(entry => entry.IsFolder),
        "files" => Entries.Where(entry => !entry.IsFolder),
        "none" => Enumerable.Empty<FileSystemEntry>(),
        _ => Entries,
    };

    private IEnumerable<(string Key, int Count)> GroupCounts()
    {
        var key = _groupKey;
        return VisibleEntries()
            .GroupBy(entry => GroupValue(entry, key), StringComparer.Ordinal)
            .OrderBy(group => group.Key, StringComparer.Ordinal)
            .Select(group => (group.Key, group.Count()));
    }

    // Never returns string.Empty: an empty group identity is an E_INVALIDARG fail-fast.
    private static string GroupValue(object item, string key)
    {
        if (item is not FileSystemEntry entry)
        {
            return "(none)";
        }

        var value = key switch
        {
            "Kind" => entry.IsFolder ? "Folders" : "Files",
            "Modified" => ModifiedBucket(entry.DateModified),
            _ => entry.IsFolder ? "File folder" : entry.TypeDisplay,
        };

        return string.IsNullOrWhiteSpace(value) ? "(none)" : value;
    }

    private static string ModifiedBucket(DateTime modified)
    {
        var age = DateTime.Now.Date - modified.Date;
        return age.TotalDays switch
        {
            < 1 => "Today",
            < 2 => "Yesterday",
            < 8 => "Earlier this week",
            < 31 => "Earlier this month",
            < 366 => "Earlier this year",
            _ => "A long time ago",
        };
    }

    private static string GroupKeyLabel(string key) => key switch
    {
        "Kind" => "folders / files",
        "Modified" => "date modified",
        _ => "item type",
    };

    private void SetLastAction(string text)
    {
        if (LastActionText is not null)
        {
            LastActionText.Text = text;
        }
    }

    private void OnBackClick(object sender, RoutedEventArgs e)
    {
        if (_history.Count == 0)
        {
            return;
        }

        Navigate(_history.Pop(), addToHistory: false);
    }

    private void OnUpClick(object sender, RoutedEventArgs e)
    {
        var parent = string.IsNullOrEmpty(_currentDir) ? null : Directory.GetParent(_currentDir);
        if (parent is not null)
        {
            Navigate(parent.FullName);
        }
    }

    private void OnRefreshClick(object sender, RoutedEventArgs e)
    {
        if (!string.IsNullOrEmpty(_currentDir))
        {
            RefreshEntries();
        }
    }

    private void OnAddressBarKeyDown(object sender, KeyRoutedEventArgs e)
    {
        if (e.Key != VirtualKey.Enter)
        {
            return;
        }

        e.Handled = true;
        var path = AddressBar.Text?.Trim();
        if (!string.IsNullOrEmpty(path) && Directory.Exists(Environment.ExpandEnvironmentVariables(path)))
        {
            Navigate(path);
        }
    }

    private void OnRowDoubleTapped(object sender, DoubleTappedRoutedEventArgs e)
    {
        if (FileTable.SelectedItem is FileSystemEntry entry && entry.IsFolder)
        {
            Navigate(entry.FullPath);
        }
    }

    private void OnTableSorted(TableView sender, TableViewSortedEventArgs args)
    {
        if (FileTable is null || Entries is null)
        {
            return;
        }

        var snapshot = ApplyActiveSort(Entries).ToList();
        Entries.Clear();
        foreach (var entry in snapshot)
        {
            Entries.Add(entry);
        }
    }

    private void OnSelectionChanged(TableView sender, TableViewSelectionChangedEventArgs args)
    {
        RefreshSelectionText();
        UpdateShapingUi();
    }

    private IEnumerable<FileSystemEntry> ApplyActiveSort(IEnumerable<FileSystemEntry> source)
    {
        IOrderedEnumerable<FileSystemEntry> ordered = source.OrderByDescending(entry => entry.IsFolder);
        var column = SampleShape.ActiveSortColumn(FileTable);
        if (FileTable is null ||
            column is null ||
            SampleShape.ActiveSortDirection(FileTable) == TableViewSortDirection.None ||
            string.IsNullOrEmpty(column.SortMemberPath) ||
            !s_keySelectors.TryGetValue(column.SortMemberPath, out var keySelector))
        {
            return ordered;
        }

        return SampleShape.ActiveSortDirection(FileTable) == TableViewSortDirection.Descending
            ? ordered.ThenByDescending(keySelector)
            : ordered.ThenBy(keySelector);
    }

    private void RefreshStatusText()
    {
        if (StatusText is null)
        {
            return;
        }

        var folderCount = Entries.Count(entry => entry.IsFolder);
        var visible = VisibleEntries().ToList();
        StatusText.Text = visible.Count == Entries.Count
            ? $"{Entries.Count} items ({folderCount} folders)"
            : $"{visible.Count} of {Entries.Count} items shown ({folderCount} folders)";
    }

    private void RefreshSelectionText()
    {
        if (FileTable is null || SelectionText is null)
        {
            return;
        }

        var selectedItems = FileTable.SelectedItem is FileSystemEntry selected ? new List<FileSystemEntry> { selected } : new List<FileSystemEntry>();
        var totalSize = selectedItems.Where(entry => !entry.IsFolder).Sum(entry => entry.Size);
        SelectionText.Text = $"{selectedItems.Count} selected ({FileSystemEntry.FormatSize(totalSize)})";
    }

    private void RefreshNavigationButtons()
    {
        if (BackButton is not null)
        {
            BackButton.IsEnabled = _history.Count > 0;
        }

        if (UpButton is not null)
        {
            UpButton.IsEnabled = !string.IsNullOrEmpty(_currentDir) && Directory.GetParent(_currentDir) is not null;
        }
    }

    private static bool PathsEqual(string left, string right)
        => string.Equals(NormalizePath(left), NormalizePath(right), StringComparison.OrdinalIgnoreCase);

    private static string NormalizePath(string path)
        => Path.GetFullPath(path).TrimEnd(Path.DirectorySeparatorChar, Path.AltDirectorySeparatorChar);
}
