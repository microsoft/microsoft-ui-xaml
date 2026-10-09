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
public sealed partial class FilePropertiesPage : SamplePageBase
{
    private static readonly string[] s_sections = { "Description", "Origin", "File" };

    private FileInfo? _currentFile;

    public FilePropertiesPage()
    {
        // <snippet>
        Source = TableViewSource.From(Entries);    // created once; never rebuilt
        InitializeComponent();

        PropTable.HeadersVisibility = TableViewHeadersVisibility.Column;
        PropTable.GridLinesVisibility = TableViewGridLinesVisibility.Horizontal;
        PropTable.Density = TableViewDensity.Compact;
        // </snippet>

        // Starts Grouped by section (ShapingOptions InitialMode="grouped"): InitializeSample applies it.
        InitializeSample(Status, Shaping.Attach(PropTable, Source, (row, key) => KeyOf(row as FilePropertyEntry, key)));
        TrackLifetime(ListFilesOnce);
    }

    public ObservableCollection<FilePropertyEntry> Entries { get; } = new();

    public TableViewSource Source { get; }

    private async void ListFilesOnce()
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

    // <snippet>
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
    // </snippet>

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

    // <snippet>
    // ---- Filter (TableViewSource.Filter) ---------------------------------------------------

    private void OnFilterChanged(object sender, SelectionChangedEventArgs e)
    {
        if (FilterSelector is null)
        {
            return;
        }

        switch (SampleShaping.SelectedTag(FilterSelector, "all"))
        {
            case "dates":
                Source.Filter(item => item is FilePropertyEntry { IsDate: true });
                break;
            case "version":
                Source.Filter(item => item is FilePropertyEntry { Source: FilePropertyEntry.VersionResource });
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
            SetLastAction(string.Format(CultureInfo.CurrentCulture, "Filter -> {0} ({1:N0} rows)", SampleShaping.Label(FilterSelector), VisibleEntries().Count()));
        }
    }
    // </snippet>

    private IEnumerable<FilePropertyEntry> VisibleEntries() => SampleShaping.SelectedTag(FilterSelector, "all") switch
    {
        "dates" => Entries.Where(entry => entry.IsDate),
        "version" => Entries.Where(entry => entry.Source == FilePropertyEntry.VersionResource),
        "none" => Enumerable.Empty<FilePropertyEntry>(),
        _ => Entries,
    };

    // ---- Actions ------------------------------------------------------------------------

    // <snippet>
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
        // the shaping restores the selection in its new section.
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
    // </snippet>

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
        RefreshReadouts();
    }

    // <snippet>
    // GroupBy key for this page's model. Never returns a blank key: TableViewSource fails fast on an empty group identity.
    private static object KeyOf(FilePropertyEntry? entry, string key)
    {
        var value = key == nameof(FilePropertyEntry.Source) ? entry?.Source : entry?.Section;
        return string.IsNullOrWhiteSpace(value) ? SampleShaping.NoneKey : value;
    }
    // </snippet>
}
