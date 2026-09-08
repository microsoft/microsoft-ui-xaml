// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
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

        FileTable.ItemsSource = Entries;
        FileTable.DeselectAll();
        AddressBar.Text = _currentDir;
        RefreshStatusText();
        RefreshNavigationButtons();
        RefreshSelectionText();
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
        => RefreshSelectionText();

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
        StatusText.Text = $"{Entries.Count} items ({folderCount} folders)";
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
