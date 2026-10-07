// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Globalization;
using System.Linq;
using Microsoft.UI;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Tabular;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Media;
using TableView = Microsoft.UI.Xaml.Controls.Tabular.TableView;
using TableViewSelectionChangedEventArgs = Microsoft.UI.Xaml.Controls.SelectionChangedEventArgs;
using TableViewSelectionMode = Microsoft.UI.Xaml.Controls.Tabular.TableViewSelectionMode;
using TableViewSampleApp.Data;
using TableViewSampleApp.Models;

namespace TableViewSampleApp.Pages;

/// <summary>
/// Selection surface. Beyond SelectionMode and the SelectedItem / SelectedIndex readouts,
/// this page shows what selection does across a <see cref="TableViewSource"/> reshape:
/// pick a row, switch the shaping mode to Grouped, and watch the per-event
/// AddedItems / RemovedItems delta. It also pins two selection invariants that are easy to
/// regress — inserting a row above the selected row must not raise SelectionChanged, and
/// removing the selected row must clear selection rather than slide to a neighbour.
/// </summary>
public sealed partial class SelectionPage : Page
{
    // One source for the lifetime of the page. Filter / GroupBy / ClearGroupBy mutate and
    // return this same instance (TableViewSource.cpp:49-50), so the projection is reshaped in
    // place and never rebuilt per interaction.
    private TableViewSource? _source;

    // Requested vs applied. Every readout and every enable/disable guard reads the *applied*
    // fields, which are assigned only after the shaping call returns — a readout driven from
    // the requested value lies about the control's state whenever GroupBy throws.
    private string _shapeMode = "flat";
    private string _appliedShapeMode = "flat";
    private string _groupKey = "Department";
    private string _appliedGroupKey = "none";

    private int _changeCount;
    private int _insertedCount;
    private string _lastDelta = "(no SelectionChanged yet)";
    private string _lastAction = "(not exercised yet)";

    public SelectionPage()
    {
        People = PersonData.Take(50);
        InitializeComponent();
        PeopleTable.HeadersVisibility = TableViewHeadersVisibility.Column;

        _source = TableViewSource.From(People);
        PeopleTable.ItemsSource = _source;

        Loaded += (_, _) =>
        {
            UpdateModeDescription((ModeCombo?.SelectedItem as ComboBoxItem)?.Content as string);
            ApplyShaping();
        };
    }

    public ObservableCollection<Person> People { get; }

    private void OnModeChanged(object sender, SelectionChangedEventArgs e)
    {
        if (PeopleTable is null || ModeCombo.SelectedItem is not ComboBoxItem item) return;
        var label = item.Content as string;
        PeopleTable.SelectionMode = label switch
        {
            "None" => TableViewSelectionMode.None,
            "Single" => TableViewSelectionMode.Single,
            _ => TableViewSelectionMode.Single,
        };
        UpdateModeDescription(label);
        RefreshReadout();
    }

    private void UpdateModeDescription(string? mode)
    {
        if (ModeDescriptionText is null) return;
        ModeDescriptionText.Text = mode switch
        {
            "None" => "Rows cannot be selected.",
            "Single" => "Click a row or use the buttons to select exactly one item.",
            _ => "Click a row or use the buttons to select exactly one item.",
        };
    }

    private void OnSelectFirstClick(object sender, Microsoft.UI.Xaml.RoutedEventArgs e)
    {
        if (People.Count == 0) return;
        PeopleTable.Select(0);
        _lastAction = "Select(0) called programmatically.";
        RefreshReadout();
    }

    private void OnSelectLastClick(object sender, Microsoft.UI.Xaml.RoutedEventArgs e)
    {
        if (People.Count == 0) return;
        PeopleTable.Select(People.Count - 1);
        _lastAction = string.Format(CultureInfo.InvariantCulture, "Select({0}) called programmatically.", People.Count - 1);
        RefreshReadout();
    }

    private void OnClearClick(object sender, Microsoft.UI.Xaml.RoutedEventArgs e)
    {
        PeopleTable.DeselectAll();
        _lastAction = "DeselectAll() called.";
        RefreshReadout();
    }

    // ---- Shaping (Flat / Grouped; Hierarchy modes are present but unavailable) -------------

    private void OnShapingModeChanged(object sender, SelectionChangedEventArgs e)
    {
        if (PeopleTable is null || ShapingModeSelector?.SelectedItem is not ComboBoxItem item) return;
        _shapeMode = item.Tag as string ?? "flat";
        ApplyShaping();
    }

    private void OnGroupKeyChanged(object sender, SelectionChangedEventArgs e)
    {
        if (PeopleTable is null || GroupKeyCombo?.SelectedItem is not ComboBoxItem item) return;
        _groupKey = item.Tag as string ?? "Department";
        ApplyShaping();
    }

    private void ApplyShaping()
    {
        if (_source is null || PeopleTable is null) return;

        // Flat mode pins the key to "none" so the applied key can never claim a grouping the
        // projection does not have.
        var requestedKey = _shapeMode == "grouped" ? _groupKey : "none";
        bool wasGrouped = _appliedGroupKey != "none";

        switch (_shapeMode)
        {
            case "grouped":
            {
                var key = requestedKey;
                // The two delegates receive DIFFERENT things despite both parameters being named
                // `item` (TableViewSource.idl:12-16):
                //   TableViewKeySelector(Object item)      -> receives the ROW ITEM
                //   TableViewIdentitySelector(Object item) -> receives the GROUP KEY
                // An item-typed identity lambda returns the empty string, which is an
                // unresolvable identity: GroupBy fails fast with E_INVALIDARG and grouping
                // silently never applies. Keep the identity selector key-based.
                _source.GroupBy(
                    item => (object)GroupValue(item, key),
                    groupKey => groupKey?.ToString() ?? "(none)");
                break;
            }

            // case "hierarchy":
            // case "groupedhierarchy":
            //     Hierarchical rows are not available in this release, which is why the two
            //     matching ComboBoxItems ship IsEnabled="False" with a tooltip. No call is
            //     written here on purpose: as of this commit TableViewSource.idl has no
            //     hierarchy verb and TableView.idl has no hierarchy property, so any code here
            //     would be naming a member that does not exist. The intended shape, per
            //     TableViewRowInfo.h:27 ("when hierarchical (tree) rows land"), is to apply the
            //     hierarchy stage to THIS SAME _source alongside the GroupBy stage above so the
            //     two compose into one projection rather than two passes. When that ships,
            //     write the real call here and drop IsEnabled="False" from the two items.
            //     break;

            default:
                _source.ClearGroupBy();
                break;
        }

        // Applied state is committed only once the shaping call has returned.
        _appliedGroupKey = requestedKey;
        _appliedShapeMode = requestedKey == "none" ? "flat" : "grouped";

        if (_appliedGroupKey != "none" && !wasGrouped)
        {
            DispatcherQueue.TryEnqueue(() =>
            {
                if (_appliedGroupKey != "none")
                {
                    PeopleTable.ExpandAllGroups();
                }
            });
        }

        RefreshReadout();
    }

    /// <summary>
    /// Group key for a row item. Never returns <see cref="string.Empty"/>: the empty string is
    /// not a usable group identity and makes the projection fail fast.
    /// </summary>
    private static string GroupValue(object item, string key)
    {
        if (item is not Person person)
        {
            return "(none)";
        }

        string? raw = key switch
        {
            "Role" => person.Role,
            "Active" => person.IsActive ? "Active" : "Inactive",
            _ => person.Department,
        };

        return string.IsNullOrWhiteSpace(raw) ? "(none)" : raw;
    }

    private void OnExpandAllClick(object sender, Microsoft.UI.Xaml.RoutedEventArgs e)
    {
        if (_appliedGroupKey == "none") return;
        PeopleTable.ExpandAllGroups();
        _lastAction = "ExpandAllGroups() — every group header is now expanded.";
        RefreshReadout();
    }

    private void OnCollapseAllClick(object sender, Microsoft.UI.Xaml.RoutedEventArgs e)
    {
        if (_appliedGroupKey == "none") return;
        PeopleTable.CollapseAllGroups();
        _lastAction = "CollapseAllGroups() — only group headers remain visible.";
        RefreshReadout();
    }

    /// <summary>
    /// Mutates the selected row's group-key property while grouping is applied, so the row has
    /// to leave one group and join another without the projection being rebuilt.
    /// </summary>
    private void OnMoveSelectedGroupClick(object sender, Microsoft.UI.Xaml.RoutedEventArgs e)
    {
        if (_appliedGroupKey == "none" || PeopleTable.SelectedItem is not Person selected)
        {
            return;
        }

        string from = GroupValue(selected, _appliedGroupKey);

        switch (_appliedGroupKey)
        {
            case "Active":
                selected.IsActive = !selected.IsActive;
                break;
            case "Role":
                selected.Role = from == "Group mover" ? "Principal Engineer" : "Group mover";
                break;
            default:
                var departments = PersonData.Departments;
                int next = (departments.ToList().IndexOf(selected.Department) + 1) % departments.Count;
                selected.Department = departments[next];
                break;
        }

        string to = GroupValue(selected, _appliedGroupKey);

        // GroupBy's key selector is evaluated when the projection is built; it does not observe
        // PropertyChanged on the key property. Without re-applying the grouping stage the row
        // keeps its old bucket and the header text goes stale, so re-run it on the same source.
        ApplyShaping();

        DispatcherQueue.TryEnqueue(() =>
        {
            _lastAction = string.Format(
                CultureInfo.InvariantCulture,
                "Moved {0} from \u201C{1}\u201D to \u201C{2}\u201D; selection is {3}.",
                DescribeItem(selected),
                from,
                to,
                ReferenceEquals(PeopleTable.SelectedItem, selected) ? "still on that row" : "no longer on that row");
            RefreshReadout();
        });
    }

    // ---- Selection invariants --------------------------------------------------------------

    private void OnInsertAboveClick(object sender, Microsoft.UI.Xaml.RoutedEventArgs e)
    {
        if (PeopleTable.SelectedItem is not Person selected)
        {
            _lastAction = "Insert above: select a row first.";
            RefreshReadout();
            return;
        }

        int index = People.IndexOf(selected);
        if (index < 0)
        {
            _lastAction = "Insert above: selected row is not in the collection.";
            RefreshReadout();
            return;
        }

        int before = _changeCount;
        _insertedCount++;
        People.Insert(index, new Person
        {
            FirstName = "Inserted",
            LastName = string.Format(CultureInfo.InvariantCulture, "Row{0}", _insertedCount),
            Email = string.Format(CultureInfo.InvariantCulture, "inserted{0}@contoso.com", _insertedCount),
            Department = selected.Department,
            Role = "Inserted above selection",
            IsActive = true,
        });

        // The event, if any, lands before this continuation runs.
        DispatcherQueue.TryEnqueue(() =>
        {
            bool fired = _changeCount != before;
            _lastAction = fired
                ? "FAIL: insert above the selected row raised SelectionChanged."
                : "OK: insert above the selected row raised no SelectionChanged.";
            RefreshReadout();
        });
    }

    private void OnRemoveSelectedClick(object sender, Microsoft.UI.Xaml.RoutedEventArgs e)
    {
        if (PeopleTable.SelectedItem is not Person selected)
        {
            _lastAction = "Remove selected: select a row first.";
            RefreshReadout();
            return;
        }

        People.Remove(selected);

        DispatcherQueue.TryEnqueue(() =>
        {
            _lastAction = PeopleTable.SelectedItem is null
                ? "OK: removing the selected row cleared selection."
                : string.Format(
                    CultureInfo.InvariantCulture,
                    "FAIL: selection slid to {0}.",
                    DescribeItem(PeopleTable.SelectedItem));
            RefreshReadout();
        });
    }

    private void OnTableSelectionChanged(TableView sender, TableViewSelectionChangedEventArgs args)
    {
        _changeCount++;
        // Count alone cannot tell a clean reset-absorb from a transient clear-and-restore, so
        // log the per-event delta the control actually reported.
        _lastDelta = string.Format(
            CultureInfo.InvariantCulture,
            "+[{0}] -[{1}]",
            DescribeItems(args.AddedItems),
            DescribeItems(args.RemovedItems));
        RefreshReadout();
    }

    private static string DescribeItems(IList<object>? items) =>
        items is null || items.Count == 0 ? "—" : string.Join(", ", items.Select(DescribeItem));

    private static string DescribeItem(object? item) =>
        item is Person person ? $"{person.FirstName} {person.LastName}" : item?.ToString() ?? "(null)";

    private void RefreshReadout()
    {
        if (PeopleTable is null || SelectedCountText is null) return;

        SelectedCountText.Text = (PeopleTable.SelectedItem is null ? 0 : 1).ToString(CultureInfo.InvariantCulture);
        SelectedIndexText.Text = PeopleTable.SelectedIndex.ToString(CultureInfo.InvariantCulture);
        ChangeCountText.Text = _changeCount.ToString(CultureInfo.InvariantCulture);
        SelectedItemText.Text = PeopleTable.SelectedItem is Person person
            ? $"{person.FirstName} {person.LastName}"
            : "(none)";

        if (SelectionDeltaText is not null)
        {
            SelectionDeltaText.Text = _lastDelta;
        }

        if (InvariantText is not null)
        {
            InvariantText.Text = _lastAction;
        }

        bool grouped = _appliedGroupKey != "none";

        if (ShapeModeText is not null)
        {
            ShapeModeText.Text = grouped
                ? string.Format(CultureInfo.InvariantCulture, "Grouped by {0}", GroupLabel(_appliedGroupKey))
                : "Flat";
        }

        if (GroupedByText is not null)
        {
            GroupedByText.Text = grouped
                ? string.Format(
                    CultureInfo.InvariantCulture,
                    "{0} — {1} groups",
                    GroupLabel(_appliedGroupKey),
                    GroupCount(_appliedGroupKey))
                : "(not grouped)";
        }

        if (GroupKeyCombo is not null)
        {
            GroupKeyCombo.IsEnabled = _appliedShapeMode == "grouped";
        }

        bool hasSelection = PeopleTable.SelectedItem is not null;

        if (ExpandAllButton is not null)
        {
            ExpandAllButton.IsEnabled = grouped;
            CollapseAllButton.IsEnabled = grouped;
            ClearButton.IsEnabled = hasSelection;
            InsertAboveButton.IsEnabled = hasSelection;
            RemoveSelectedButton.IsEnabled = hasSelection;
            MoveGroupButton.IsEnabled = hasSelection && grouped;
            SelectFirstButton.IsEnabled = People.Count > 0;
            SelectLastButton.IsEnabled = People.Count > 0;
        }
    }

    private int GroupCount(string key) =>
        People.Select(p => GroupValue(p, key)).Distinct(StringComparer.Ordinal).Count();

    private static string GroupLabel(string key) => key switch
    {
        "Role" => "Role",
        "Active" => "Active status",
        "Department" => "Department",
        _ => "(none)",
    };
}

// Page-private chip converters. Deliberately NOT the shared Pages/ShowcaseConverters.cs set:
// those allocate a new SolidColorBrush on every Convert call, which under virtualization is a
// per-cell allocation on every realization, and they have no HighContrast path. Everything
// here is cached in static readonly state and built exactly once.
internal static class SelectionChipPalette
{
    // Read once. In HighContrast the chip drops its tint entirely and relies on the theme
    // foreground plus the chip's border for shape.
    internal static readonly bool HighContrast = new Windows.UI.ViewManagement.AccessibilitySettings().HighContrast;

    internal static readonly SolidColorBrush TransparentBrush = new(Colors.Transparent);

    private static readonly IReadOnlyDictionary<string, SolidColorBrush> s_tints = Build(0x40);
    private static readonly IReadOnlyDictionary<string, SolidColorBrush> s_dots = Build(0xFF);
    private static readonly SolidColorBrush s_fallbackTint = new(ColorHelper.FromArgb(0x40, 0x64, 0x74, 0x8B));
    private static readonly SolidColorBrush s_fallbackDot = new(ColorHelper.FromArgb(0xFF, 0x64, 0x74, 0x8B));

    private static IReadOnlyDictionary<string, SolidColorBrush> Build(byte alpha) =>
        new Dictionary<string, SolidColorBrush>(StringComparer.Ordinal)
        {
            ["Engineering"] = new(ColorHelper.FromArgb(alpha, 0x00, 0x78, 0xD4)),
            ["Sales"] = new(ColorHelper.FromArgb(alpha, 0x14, 0xB8, 0xA6)),
            ["Marketing"] = new(ColorHelper.FromArgb(alpha, 0xA8, 0x55, 0xF7)),
            ["HR"] = new(ColorHelper.FromArgb(alpha, 0xF5, 0x9E, 0x0B)),
            ["Operations"] = new(ColorHelper.FromArgb(alpha, 0xEF, 0x44, 0x44)),
            ["Design"] = new(ColorHelper.FromArgb(alpha, 0xEC, 0x48, 0x99)),
            ["Product"] = new(ColorHelper.FromArgb(alpha, 0x0E, 0xA5, 0xE9)),
            ["Finance"] = new(ColorHelper.FromArgb(alpha, 0x22, 0xC5, 0x5E)),
        };

    internal static SolidColorBrush Tint(object? value) =>
        HighContrast ? TransparentBrush : Lookup(s_tints, value, s_fallbackTint);

    internal static SolidColorBrush Dot(object? value) =>
        HighContrast ? TransparentBrush : Lookup(s_dots, value, s_fallbackDot);

    private static SolidColorBrush Lookup(IReadOnlyDictionary<string, SolidColorBrush> map, object? value, SolidColorBrush fallback) =>
        value is string key && map.TryGetValue(key, out var brush) ? brush : fallback;
}

/// <summary>Chip background for the templated Department cell. Never allocates.</summary>
public sealed partial class SelectionChipTintConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, string language) =>
        SelectionChipPalette.Tint(value);

    public object ConvertBack(object value, Type targetType, object parameter, string language) =>
        throw new NotSupportedException();
}

/// <summary>Decorative chip dot for the templated Department cell. Never allocates.</summary>
public sealed partial class SelectionChipDotConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, string language) =>
        SelectionChipPalette.Dot(value);

    public object ConvertBack(object value, Type targetType, object parameter, string language) =>
        throw new NotSupportedException();
}

/// <summary>
/// Builds a per-row accessible name for an interactive cell from the row's identity, so a
/// column of fifty checkboxes does not announce the same string fifty times. The format comes
/// from ConverterParameter, e.g. "Active employee: {0}".
/// </summary>
public sealed partial class SelectionRowCellNameConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, string language)
    {
        var rowName = value?.ToString();
        if (string.IsNullOrWhiteSpace(rowName))
        {
            rowName = "row";
        }

        return parameter is string format && format.Length > 0
            ? string.Format(CultureInfo.CurrentCulture, format, rowName)
            : rowName;
    }

    public object ConvertBack(object value, Type targetType, object parameter, string language) =>
        throw new NotSupportedException();
}
