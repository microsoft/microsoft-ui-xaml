// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Linq;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Tabular;
using TableViewSampleApp.Models;
using TableViewGridLinesVisibility = Microsoft.UI.Xaml.Controls.Tabular.TableViewGridLinesVisibility;
using TableViewDensity = Microsoft.UI.Xaml.Controls.Tabular.TableViewDensity;

namespace TableViewSampleApp.Pages;

public sealed partial class FilePropertiesPage : Page
{
    private static readonly string[] s_sections = { "Description", "Origin", "File" };

    // One source per selected file (the items collection genuinely changes); shaping is then
    // applied IN PLACE on it, because Filter / GroupBy mutate and return the same instance
    // (TableViewSource.cpp:49-50). Changing shaping never re-reads the file system.
    private TableViewSource? _source;
    private List<FilePropertyEntry> _entries = new();
    private string _mode = "flat";          // requested
    private string _appliedMode = "flat";   // applied
    private string _groupKey = "Section";
    private string _filter = "all";

    public FilePropertiesPage()
    {
        InitializeComponent();

        if (PropTable is not null)
        {
            PropTable.HeadersVisibility = TableViewHeadersVisibility.Column;
            PropTable.GridLinesVisibility = TableViewGridLinesVisibility.Horizontal;
            PropTable.Density = TableViewDensity.Compact;
        }

        Loaded += OnLoaded;
    }

    private void OnLoaded(object sender, RoutedEventArgs e)
    {
        PopulateFiles();
    }

    private void PopulateFiles()
    {
        if (FileList is null)
        {
            return;
        }

        var files = EnumerateFiles(AppContext.BaseDirectory);
        if (files.Count < 1)
        {
            var userProfile = Environment.GetFolderPath(Environment.SpecialFolder.UserProfile);
            if (!string.IsNullOrWhiteSpace(userProfile))
            {
                files = EnumerateFiles(userProfile);
            }
        }

        FileList.ItemsSource = files;

        if (files.Count > 0)
        {
            FileList.SelectedIndex = 0;
        }
        else
        {
            if (SelectedFileText is not null)
            {
                SelectedFileText.Text = "No readable files found";
            }

            if (StatusText is not null)
            {
                StatusText.Text = "0 properties across 0 sections";
            }
        }
    }

    private static List<FileInfo> EnumerateFiles(string folder)
    {
        try
        {
            if (string.IsNullOrWhiteSpace(folder) || !Directory.Exists(folder))
            {
                return new List<FileInfo>();
            }

            return new DirectoryInfo(folder)
                .EnumerateFiles()
                .OrderBy(file => file.Name, StringComparer.OrdinalIgnoreCase)
                .Take(100)
                .ToList();
        }
        catch
        {
            return new List<FileInfo>();
        }
    }

    private void OnFileSelectionChanged(object sender, SelectionChangedEventArgs e)
    {
        if (FileList?.SelectedItem is FileInfo file)
        {
            ShowFileProperties(file);
        }
    }

    private void ShowFileProperties(FileInfo file)
    {
        if (PropTable is null)
        {
            return;
        }

        _entries = BuildEntries(file);
        _source = TableViewSource.From(_entries);
        PropTable.ItemsSource = _source;
        ApplyShaping();

        if (SelectedFileText is not null)
        {
            SelectedFileText.Text = file.Name;
        }
    }

    private void ApplyShaping()
    {
        if (PropTable is null || _source is null)
        {
            return;
        }

        switch (_filter)
        {
            case "filled":
                _source.Filter(item => item is FilePropertyEntry entry && !string.IsNullOrWhiteSpace(entry.Value));
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
                // the GROUP KEY (TableViewSource.idl:12-16). Keying the identity lambda off the
                // item would yield an empty identity and fail fast with E_INVALIDARG.
                _source.GroupBy(
                    item => (object)GroupValue(item, key),
                    groupKey => groupKey?.ToString() ?? "(none)");
                break;

            // case "hierarchy":
            // case "groupedhierarchy":
            //     Hierarchical rows are not available in this release, and no hierarchy API exists
            //     on TableViewSource or TableView yet, so there is deliberately no call written
            //     here to copy. When the control ships hierarchy support, apply it to this same
            //     source instance alongside the GroupBy stage above so the two compose, and drop
            //     the IsEnabled="False" from the matching options in the XAML.
            //     break;

            default:
                _source.ClearGroupBy();
                break;
        }

        // Set ONLY after the shaping call returns.
        _appliedMode = _mode;

        UpdateShapingUi();
        UpdateStatusText();
    }

    private void OnShapingModeChanged(object sender, SelectionChangedEventArgs e)
    {
        if (ShapingModeSelector?.SelectedItem is not ComboBoxItem { Tag: string tag })
        {
            return;
        }

        _mode = tag;
        ApplyShaping();
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Shaping mode -> {0}", _appliedMode == "grouped" ? "Grouped" : "Flat"));
    }

    private void OnShapingGroupKeyChanged(object sender, SelectionChangedEventArgs e)
    {
        if (ShapingGroupKeyCombo?.SelectedItem is not ComboBoxItem { Tag: string tag })
        {
            return;
        }

        _groupKey = tag;
        ApplyShaping();
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Group key -> {0}", tag));
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
            "filled" => "Filtered to properties that have a value",
            "none" => "Filtered to zero rows",
            _ => "Filter cleared",
        });
    }

    private void OnPropSelectionChanged(object sender, SelectionChangedEventArgs e) => UpdateShapingUi();

    private void OnExpandAllClick(object sender, RoutedEventArgs e)
    {
        if (_appliedMode != "grouped")
        {
            return;
        }

        PropTable.ExpandAllGroups();
        SetLastAction("Expanded all sections");
    }

    private void OnCollapseAllClick(object sender, RoutedEventArgs e)
    {
        if (_appliedMode != "grouped")
        {
            return;
        }

        PropTable.CollapseAllGroups();
        SetLastAction("Collapsed all sections");
    }

    private void OnMoveSelectedSectionClick(object sender, RoutedEventArgs e)
    {
        if (PropTable?.SelectedItem is not FilePropertyEntry selected)
        {
            return;
        }

        var index = Array.IndexOf(s_sections, selected.Section);
        var next = s_sections[(index + 1 + s_sections.Length) % s_sections.Length];
        var previous = string.IsNullOrWhiteSpace(selected.Section) ? "(none)" : selected.Section;
        selected.Section = next;

        // Re-running the shaping stage re-reads every key, so the mutated row re-buckets.
        ApplyShaping();
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "{0}: {1} -> {2}", selected.Property, previous, next));
    }

    private void OnRemoveSelectedClick(object sender, RoutedEventArgs e)
    {
        if (PropTable?.SelectedItem is not FilePropertyEntry selected)
        {
            return;
        }

        // The projection is built over this List, so rebuild the source over the shortened list.
        _entries.Remove(selected);
        _source = TableViewSource.From(_entries);
        PropTable.ItemsSource = _source;
        ApplyShaping();
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Removed \"{0}\" from {1}", selected.Property, selected.Section));
    }

    private void OnReloadClick(object sender, RoutedEventArgs e)
    {
        if (FileList?.SelectedItem is FileInfo file)
        {
            ShowFileProperties(file);
            SetLastAction("Rebuilt the property list");
        }
    }

    private void UpdateShapingUi()
    {
        var grouped = _appliedMode == "grouped";
        var hasSelection = PropTable?.SelectedItem is FilePropertyEntry;

        if (ShapingGroupKeyCombo is not null)
        {
            ShapingGroupKeyCombo.IsEnabled = grouped;
            ToolTipService.SetToolTip(ShapingGroupKeyCombo, grouped
                ? "Change the GroupBy key while grouped - expansion and selection survive."
                : "Available once Grouped mode is selected.");
        }

        SetGroupActionState(ExpandAllButton, grouped, "Expand every section.");
        SetGroupActionState(CollapseAllButton, grouped, "Collapse every section.");

        if (MoveSectionButton is not null)
        {
            MoveSectionButton.IsEnabled = hasSelection;
            ToolTipService.SetToolTip(MoveSectionButton, hasSelection
                ? "Rewrites the row's Section, then re-applies GroupBy so the row moves between groups."
                : "Select a property row first.");
        }

        if (RemoveRowButton is not null)
        {
            RemoveRowButton.IsEnabled = hasSelection;
            ToolTipService.SetToolTip(RemoveRowButton, hasSelection
                ? "Removes the row; its group disappears when it was the last one."
                : "Select a property row first.");
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
                : string.Join(", ", groups.Select(g => string.Format(CultureInfo.CurrentCulture, "{0} ({1})", g.Key, g.Count)));
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

    private IEnumerable<FilePropertyEntry> VisibleEntries() => _filter switch
    {
        "filled" => _entries.Where(entry => !string.IsNullOrWhiteSpace(entry.Value)),
        "none" => Enumerable.Empty<FilePropertyEntry>(),
        _ => _entries,
    };

    private IEnumerable<(string Key, int Count)> GroupCounts()
    {
        var key = _groupKey;
        return VisibleEntries()
            .GroupBy(entry => GroupValue(entry, key), StringComparer.Ordinal)
            .Select(group => (group.Key, group.Count()));
    }

    // Never returns string.Empty: an empty group identity is an E_INVALIDARG fail-fast.
    private static string GroupValue(object item, string key)
    {
        if (item is not FilePropertyEntry entry)
        {
            return "(none)";
        }

        var value = key switch
        {
            "Filled" => string.IsNullOrWhiteSpace(entry.Value) ? "Empty" : "Has a value",
            _ => entry.Section,
        };

        return string.IsNullOrWhiteSpace(value) ? "(none)" : value;
    }

    private static string GroupKeyLabel(string key) => key == "Filled" ? "filled / empty" : "section";

    private void SetLastAction(string text)
    {
        if (LastActionText is not null)
        {
            LastActionText.Text = text;
        }
    }

    private void UpdateStatusText()
    {
        if (StatusText is null)
        {
            return;
        }

        var visible = VisibleEntries().ToList();
        var sectionCount = visible.Select(entry => entry.Section).Distinct(StringComparer.Ordinal).Count();
        StatusText.Text = string.Format(
            CultureInfo.CurrentCulture,
            "{0} properties across {1} sections ({2})",
            visible.Count,
            sectionCount,
            _appliedMode == "grouped" ? "grouped" : "flat");
    }

    private static List<FilePropertyEntry> BuildEntries(FileInfo file)
    {
        return new List<FilePropertyEntry>
        {
            new("Description", "Title"),
            new("Description", "Subject"),
            new("Description", "Tags"),
            new("Description", "Categories"),
            new("Description", "Comments"),

            new("Origin", "Authors"),
            new("Origin", "Last saved by"),
            new("Origin", "Revision number"),
            new("Origin", "Version number"),
            new("Origin", "Program name"),
            new("Origin", "Company"),
            new("Origin", "Manager"),
            new("Origin", "Content created", SafeRead(() => file.CreationTime.ToString("g", CultureInfo.CurrentCulture))),
            new("Origin", "Date last saved", SafeRead(() => file.LastWriteTime.ToString("g", CultureInfo.CurrentCulture))),
            new("Origin", "Last printed"),

            new("File", "Name", SafeRead(() => file.Name)),
            new("File", "Item type", SafeRead(() => GetFileType(file.Extension))),
            new("File", "Folder path", SafeRead(() => file.DirectoryName ?? string.Empty)),
            new("File", "Size", SafeRead(() => FormatSize(file.Length))),
            new("File", "Date created", SafeRead(() => file.CreationTime.ToString("g", CultureInfo.CurrentCulture))),
            new("File", "Date modified", SafeRead(() => file.LastWriteTime.ToString("g", CultureInfo.CurrentCulture))),
            new("File", "Attributes", SafeRead(() => file.Attributes.ToString())),
        };
    }

    private static string SafeRead(Func<string> read)
    {
        try
        {
            return read() ?? string.Empty;
        }
        catch
        {
            return string.Empty;
        }
    }

    private static string GetFileType(string extension)
    {
        if (string.IsNullOrWhiteSpace(extension))
        {
            return "File";
        }

        return extension.ToLowerInvariant() switch
        {
            ".cs" => "C# Source File",
            ".dll" => "Application extension",
            ".exe" => "Application",
            ".json" => "JSON File",
            ".pdb" => "Program Debug Database",
            ".pri" => "PRI File",
            ".txt" => "Text Document",
            ".winmd" => "Windows Metadata File",
            ".xaml" => "XAML File",
            ".xml" => "XML Document",
            _ => string.Format(CultureInfo.InvariantCulture, "{0} File", extension.TrimStart('.').ToUpperInvariant()),
        };
    }

    private static string FormatSize(long bytes)
    {
        string[] units = { "bytes", "KB", "MB", "GB" };
        var size = (double)bytes;
        var unit = 0;

        while (size >= 1024 && unit < units.Length - 1)
        {
            size /= 1024;
            unit++;
        }

        if (unit == 0)
        {
            return string.Format(CultureInfo.CurrentCulture, "{0:N0} {1}", bytes, units[unit]);
        }

        return string.Format(CultureInfo.CurrentCulture, "{0:N1} {1}", size, units[unit]);
    }
}