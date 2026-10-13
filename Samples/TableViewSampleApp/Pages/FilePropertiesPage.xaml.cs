// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Threading.Tasks;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Tabular;
using TableViewSampleApp.Helpers;
using TableViewSampleApp.Models;
using TableView = Microsoft.UI.Xaml.Controls.Tabular.TableView;

namespace TableViewSampleApp.Pages;

/// <summary>
/// File properties scenario: a properties dialog for the sample's own binaries. Every one of the
/// 22 values is read from the file's version resource (FileVersionInfo) or the file system
/// (FileInfo), so none is blank. The page starts grouped by section; one TableViewSource over one
/// ObservableCollection is refilled per file and filtered, grouped and edited in place.
/// </summary>
public sealed partial class FilePropertiesPage : Page
{
    private static readonly string[] s_sections = { "Description", "Origin", "File" };

    private TableViewSource? _source;          // created ONCE over Entries; never rebuilt
    private string _appliedMode = "flat";      // written only after GroupBy/ClearGroupBy returns
    private string _appliedKey = "Section";
    private FileInfo? _currentFile;

    public FilePropertiesPage()
    {
        _source = TableViewSource.From(Entries);
        InitializeComponent();

        PropTable.HeadersVisibility = TableViewHeadersVisibility.Column;
        PropTable.GridLinesVisibility = TableViewGridLinesVisibility.Horizontal;
        PropTable.Density = TableViewDensity.Compact;

        // This page starts Grouped by section (ShapingModeSelector SelectedIndex="1"). The
        // SelectionChanged that InitializeComponent raised was ignored by the guard, so apply it now.
        ApplyShaping(announce: false);
        Loaded += OnPageLoaded;
    }

    public ObservableCollection<FilePropertyEntry> Entries { get; } = new();

    public TableViewSource? Source => _source;

    private async void OnPageLoaded(object sender, RoutedEventArgs e)
    {
        if (FileList.ItemsSource is not null)
        {
            return;
        }

        // Reading version resources touches every binary in the folder: do it off the UI thread.
        var folder = AppContext.BaseDirectory;
        var files = await Task.Run(() => FindDescribedFiles(folder));
        FileList.ItemsSource = files;
        if (files.Count > 0)
        {
            FileList.SelectedIndex = 0;
        }

        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Listed {0:N0} files from the app folder", files.Count));
    }

    // Binaries whose version resource fills every Description and Origin row.
    private static List<FileInfo> FindDescribedFiles(string folder)
    {
        try
        {
            return new DirectoryInfo(folder)
                .EnumerateFiles()
                .Where(file => file.Extension.Equals(".dll", StringComparison.OrdinalIgnoreCase)
                    || file.Extension.Equals(".exe", StringComparison.OrdinalIgnoreCase))
                .Where(file => IsFullyDescribed(FileVersionInfo.GetVersionInfo(file.FullName)))
                .OrderBy(file => file.Name, StringComparer.CurrentCultureIgnoreCase)
                .Take(40)
                .ToList();
        }
        catch (Exception ex) when (ex is IOException or UnauthorizedAccessException)
        {
            return new List<FileInfo>();
        }
    }

    private static bool IsFullyDescribed(FileVersionInfo version) =>
        new[]
        {
            version.FileDescription, version.FileVersion, version.ProductName, version.ProductVersion,
            version.LegalCopyright, version.Language, version.CompanyName, version.OriginalFilename, version.InternalName,
        }.All(value => !string.IsNullOrWhiteSpace(value));

    private void OnFileSelectionChanged(object sender, SelectionChangedEventArgs e)
    {
        if (FileList.SelectedItem is not FileInfo file)
        {
            return;
        }

        var announce = _currentFile is not null;
        ShowFile(file);
        if (announce)
        {
            SetLastAction(string.Format(CultureInfo.CurrentCulture, "Showing {0}: {1:N0} properties", file.Name, Entries.Count));
        }
    }

    // Refills the one collection; the source re-projects it under the current filter and grouping.
    private void ShowFile(FileInfo file)
    {
        _currentFile = file;
        Entries.Clear();
        foreach (var entry in ReadProperties(file))
        {
            Entries.Add(entry);
        }

        RefreshReadouts();
    }

    private static IEnumerable<FilePropertyEntry> ReadProperties(FileInfo file)
    {
        file.Refresh();
        var version = FileVersionInfo.GetVersionInfo(file.FullName);
        const string vr = FilePropertyEntry.VersionResource;
        const string fs = FilePropertyEntry.FileSystem;

        return new[]
        {
            new FilePropertyEntry("Description", "File description", version.FileDescription ?? string.Empty, vr),
            new FilePropertyEntry("Description", "Type", FileType(file.Extension), fs),
            new FilePropertyEntry("Description", "File version", version.FileVersion ?? string.Empty, vr),
            new FilePropertyEntry("Description", "Product name", version.ProductName ?? string.Empty, vr),
            new FilePropertyEntry("Description", "Product version", version.ProductVersion ?? string.Empty, vr),
            new FilePropertyEntry("Description", "Copyright", version.LegalCopyright ?? string.Empty, vr),
            new FilePropertyEntry("Description", "Language", version.Language ?? string.Empty, vr),

            new FilePropertyEntry("Origin", "Company", version.CompanyName ?? string.Empty, vr),
            new FilePropertyEntry("Origin", "Original filename", version.OriginalFilename ?? string.Empty, vr),
            new FilePropertyEntry("Origin", "Internal name", version.InternalName ?? string.Empty, vr),
            new FilePropertyEntry("Origin", "Build", version.IsDebug ? "Debug" : version.IsPrivateBuild ? "Private" : "Release", vr),
            new FilePropertyEntry("Origin", "Pre-release", YesNo(version.IsPreRelease), vr),
            new FilePropertyEntry("Origin", "Patched", YesNo(version.IsPatched), vr),

            new FilePropertyEntry("File", "Name", file.Name, fs),
            new FilePropertyEntry("File", "Folder path", file.DirectoryName ?? string.Empty, fs),
            new FilePropertyEntry("File", "Size", SizeText(file.Length), fs),
            // Rounded up to the 4 KB allocation unit of an NTFS volume.
            new FilePropertyEntry("File", "Size on disk", SizeText((file.Length + 4095) / 4096 * 4096), fs),
            new FilePropertyEntry("File", "Date created", file.CreationTime.ToString("F", CultureInfo.CurrentCulture), fs, isDate: true),
            new FilePropertyEntry("File", "Date modified", file.LastWriteTime.ToString("F", CultureInfo.CurrentCulture), fs, isDate: true),
            new FilePropertyEntry("File", "Date accessed", file.LastAccessTime.ToString("F", CultureInfo.CurrentCulture), fs, isDate: true),
            new FilePropertyEntry("File", "Attributes", file.Attributes.ToString(), fs),
            new FilePropertyEntry("File", "Read-only", YesNo(file.IsReadOnly), fs),
        };
    }

    private static string YesNo(bool value) => value ? "Yes" : "No";

    private static string FileType(string extension) => extension.ToLowerInvariant() switch
    {
        ".dll" => "Application extension",
        ".exe" => "Application",
        _ => string.Format(CultureInfo.CurrentCulture, "{0} File", extension.TrimStart('.').ToUpperInvariant()),
    };

    // "1.25 MB (1,310,720 bytes)", as the Properties dialog shows it.
    private static string SizeText(long bytes) =>
        string.Format(CultureInfo.CurrentCulture, "{0} ({1:N0} bytes)", FileSystemEntry.FormatSize(bytes), bytes);

    // ---- Filter (TableViewSource.Filter) ---------------------------------------------------

    private void OnFilterChanged(object sender, SelectionChangedEventArgs e)
    {
        if (_source is null || FilterSelector is null)
        {
            return;
        }

        switch (SampleShaping.SelectedTag(FilterSelector, "all"))
        {
            case "dates":
                _source.Filter(item => item is FilePropertyEntry { IsDate: true });
                break;
            case "version":
                _source.Filter(item => item is FilePropertyEntry { Source: FilePropertyEntry.VersionResource });
                break;
            case "none":
                _source.Filter(_ => false);
                break;
            default:
                _source.ClearFilter();
                break;
        }

        // Fires once during InitializeComponent (SelectedIndex="0"); report user changes only.
        if (IsLoaded)
        {
            SetLastAction(string.Format(CultureInfo.CurrentCulture, "Filter -> {0} ({1:N0} rows)", SampleShaping.Label(FilterSelector), VisibleEntries().Count()));
        }
    }

    private IEnumerable<FilePropertyEntry> VisibleEntries() => SampleShaping.SelectedTag(FilterSelector, "all") switch
    {
        "dates" => Entries.Where(entry => entry.IsDate),
        "version" => Entries.Where(entry => entry.Source == FilePropertyEntry.VersionResource),
        "none" => Enumerable.Empty<FilePropertyEntry>(),
        _ => Entries,
    };

    // ---- Actions ------------------------------------------------------------------------

    private void OnMoveSectionClick(object sender, RoutedEventArgs e)
    {
        if (PropTable.SelectedItem is not FilePropertyEntry entry)
        {
            SetLastAction("No row selected.");
            return;
        }

        var from = entry.Section;
        entry.Section = SampleShaping.Next(s_sections, from);

        // GroupBy takes a delegate, not a property path, so re-apply it to re-bucket the row;
        // ApplyShaping re-selects it in its new section.
        ReapplyIfGroupedOn(nameof(FilePropertyEntry.Section));
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Moved {0} from {1} to {2}", entry.Property, from, entry.Section));
    }

    private void OnRemoveSelectedClick(object sender, RoutedEventArgs e)
    {
        if (PropTable.SelectedItem is not FilePropertyEntry entry)
        {
            SetLastAction("No row selected.");
            return;
        }

        // A collection change: the source drops the row (and an emptied section) by itself.
        Entries.Remove(entry);
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Removed {0} from {1}", entry.Property, entry.Section));
    }

    private void OnRestoreClick(object sender, RoutedEventArgs e)
    {
        if (_currentFile is null)
        {
            SetLastAction("No file selected.");
            return;
        }

        ShowFile(_currentFile);
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Re-read all {0:N0} properties of {1}", Entries.Count, _currentFile.Name));
    }

    private void OnNextFileClick(object sender, RoutedEventArgs e)
    {
        if (FileList.Items.Count == 0)
        {
            SetLastAction("No files to show.");
            return;
        }

        // Announced by OnFileSelectionChanged.
        FileList.SelectedIndex = (FileList.SelectedIndex + 1) % FileList.Items.Count;
        FileList.ScrollIntoView(FileList.SelectedItem);
    }

    private void OnPropSelectionChanged(TableView sender, SelectionChangedEventArgs args)
    {
        if (SampleShaping.IsReselecting)
        {
            return;
        }

        RefreshReadouts();
    }

    private void RefreshReadouts()
    {
        if (RowsText is null || FileText is null || SectionsText is null)
        {
            return;
        }

        var visible = VisibleEntries().ToList();
        FileText.Text = _currentFile?.Name ?? "(none)";
        SectionsText.Text = visible.Count == 0
            ? "(none)"
            : string.Join(", ", s_sections
                .Select(section => (section, count: visible.Count(entry => entry.Section == section)))
                .Where(pair => pair.count > 0)
                .Select(pair => string.Format(CultureInfo.CurrentCulture, "{0} {1:N0}", pair.section, pair.count)));
        RowsText.Text = SampleShaping.RowCountText(visible.Count);
    }

    // Group key resolution for this page's model (FIX-PLAN §1.6 R2). Never returns a blank key.
    private static object KeyOf(FilePropertyEntry? entry, string key)
    {
        var value = key == nameof(FilePropertyEntry.Source) ? entry?.Source : entry?.Section;
        return string.IsNullOrWhiteSpace(value) ? SampleShaping.NoneKey : value;
    }

    #region Sample scaffolding (generic; see FIX-PLAN §6)

    private void OnShapingModeChanged(object sender, SelectionChangedEventArgs e) => ApplyShaping(announce: true);

    private void OnGroupKeyChanged(object sender, SelectionChangedEventArgs e) => ApplyShaping(announce: true);

    private void ApplyShaping(bool announce)
    {
        // Fires during InitializeComponent (the selectors' SelectedIndex), before the
        // later-declared elements exist. Guard every element this path touches.
        if (_source is null || PropTable is null || ShapingModeSelector is null || GroupKeySelector is null
            || ExpandAllButton is null || CollapseAllButton is null || ShapingModeText is null)
        {
            return;
        }

        var mode = SampleShaping.SelectedTag(ShapingModeSelector, "flat");
        var key = SampleShaping.SelectedTag(GroupKeySelector, "Section");
        var selected = PropTable.SelectedItem;

        switch (mode)
        {
            case "grouped":
                // The key selector receives the ROW; the identity selector receives the KEY.
                _source.GroupBy(item => KeyOf(item as FilePropertyEntry, key), SampleShaping.GroupIdentity);
                break;
            // case "hierarchy":
            // case "groupedHierarchy":
            //     Hierarchical (tree) rows are not available in this release, so the two matching
            //     ComboBoxItems ship disabled. TableViewSource and TableView have no hierarchy
            //     member today. When hierarchy ships, apply it to this same source here, composed
            //     with the GroupBy stage above rather than replacing it, and set _appliedMode only
            //     after the call returns.
            default:
                _source.ClearGroupBy();
                mode = "flat";
                break;
        }

        _appliedMode = mode;
        _appliedKey = key;

        // Re-applying GroupBy can drop the selection when the selected row changed group.
        SampleShaping.Reselect(PropTable, selected, Entries.Count + s_sections.Length + 2, RefreshReadouts);
        UpdateShapingGating();
        if (announce)
        {
            SetLastAction(mode == "grouped"
                ? string.Format(CultureInfo.CurrentCulture, "Shaping -> Grouped by {0}", SampleShaping.Label(GroupKeySelector))
                : "Shaping -> Flat");
        }
    }

    // Call after ANY write to the grouped-on property.
    private void ReapplyIfGroupedOn(string? propertyName)
    {
        if (_appliedMode == "grouped" && propertyName == _appliedKey)
        {
            ApplyShaping(announce: false);
        }
    }

    private void UpdateShapingGating()
    {
        var grouped = _appliedMode == "grouped";
        GroupKeySelector.IsEnabled = grouped;
        ExpandAllButton.IsEnabled = grouped;
        CollapseAllButton.IsEnabled = grouped;
        ShapingModeText.Text = SampleShaping.ShapingText(grouped, GroupKeySelector);
    }

    private void OnExpandAllClick(object sender, RoutedEventArgs e)
    {
        PropTable.ExpandAllGroups();
        SetLastAction("Expanded all groups");
    }

    private void OnCollapseAllClick(object sender, RoutedEventArgs e)
    {
        PropTable.CollapseAllGroups();
        SetLastAction("Collapsed all groups");
    }

    // The only writer of LastActionText.
    private void SetLastAction(string message)
    {
        if (LastActionText is not null)
        {
            LastActionText.Text = message;
        }

        RefreshReadouts();
    }

    #endregion
}
