// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using Microsoft.UI;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Tabular;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Media;
// Tabular aliases keep the sample code concise.
using TableView = Microsoft.UI.Xaml.Controls.Tabular.TableView;
using TableViewColumn = Microsoft.UI.Xaml.Controls.Tabular.TableViewColumn;
using TableViewSortedEventArgs = Microsoft.UI.Xaml.Controls.Tabular.TableViewSortedEventArgs;
// #44 enum unification: the per-control TableViewSortDirection was removed; sort direction
// is now the shared Microsoft.UI.Xaml.Controls.Tabular.SortDirection { None=0, Ascending=1, Descending=2 }.
using TableViewSortDirection = Microsoft.UI.Xaml.Controls.Tabular.SortDirection;
using Microsoft.UI.Xaml.Controls.Primitives;
using TableViewSampleApp.Data;
using TableViewSampleApp.Models;

namespace TableViewSampleApp.Pages;

/// <summary>
/// Demonstrates the aligned TableView single-column sort surface: the control
/// owns sort STATE — direction, the header SortIndicator chevron, the public
/// SortByColumn / ToggleSortDirection / ClearSort API, and the Sorted event —
/// and it also reshapes the rows itself, so a plain collection sorts without
/// the app re-ordering anything.
///
/// The page pairs sorting with <c>TableViewSource.GroupBy</c>, because sorting
/// WITHIN groups is the interaction only this page can show: group by Group and
/// click a header, and the rows reorder inside every group rather than the
/// grouping collapsing.
///
/// The dataset is a fictional group-stage standings table where ordering by
/// Points — with Goal Difference as the tiebreaker — is the canonical sort.
/// A new sort always replaces the previous one (single-column), matching the
/// aligned control's scalar sort surface.
/// </summary>
public sealed partial class SortPage : Page
{
    private int _sortedFiredCount;

    // The shaped projection the table is bound to. Reshaped in place — never rebuilt.
    private TableViewSource? _source;

    // Requested shaping, straight off the pickers.
    private string _shapingMode = "flat";
    private string _groupKey = "Group";

    // Mirrors the requested key only once GroupBy has actually succeeded, so no readout
    // and no enable/disable guard can claim a grouping the source never took.
    private string _appliedGroupKey = "none";

    public SortPage()
    {
        Teams = LeagueData.All();
        InitializeComponent();

        _source = TableViewSource.From(Teams);
        TeamsTable.ItemsSource = _source;

        ApplyGrouping();
        Loaded += (_, _) => RefreshReadouts(triggerColumn: null);
    }

    public ObservableCollection<LeagueTeam> Teams { get; }

    // ----- Shaping: Flat / Grouped -----

    private void OnShapingModeChanged(object sender, SelectionChangedEventArgs e)
    {
        if (_source is null || ShapingModeSelector?.SelectedItem is not ComboBoxItem { Tag: string tag })
        {
            return;
        }

        _shapingMode = tag;
        ApplyGrouping();
    }

    private void OnGroupByChanged(object sender, SelectionChangedEventArgs e)
    {
        if (_source is null || GroupBySelector?.SelectedItem is not ComboBoxItem { Tag: string tag })
        {
            return;
        }

        _groupKey = tag;
        ApplyGrouping();
    }

    private void OnExpandAllGroupsClick(object sender, RoutedEventArgs e)
    {
        if (_appliedGroupKey == "none")
        {
            return;
        }

        TeamsTable.ExpandAllGroups();
        RefreshReadouts(triggerColumn: null);
    }

    private void OnCollapseAllGroupsClick(object sender, RoutedEventArgs e)
    {
        if (_appliedGroupKey == "none")
        {
            return;
        }

        TeamsTable.CollapseAllGroups();
        RefreshReadouts(triggerColumn: null);
    }

    /// <summary>
    /// Reshapes the existing <see cref="TableViewSource"/> in place: GroupBy /
    /// ClearGroupBy mutate and return the same instance, so the source is never
    /// rebuilt — rebuilding would drop selection, scroll offset, the active sort
    /// and group expansion.
    /// </summary>
    private void ApplyGrouping()
    {
        if (_source is null)
        {
            return;
        }

        var requestedKey = _shapingMode == "grouped" ? _groupKey : "none";

        if (requestedKey == "none")
        {
            _source.ClearGroupBy();
            _appliedGroupKey = "none";
        }
        else
        {
            var key = requestedKey;
            // The two delegates do NOT receive the same thing despite both parameters being
            // named `item`: the key selector is handed the ROW ITEM, while the identity
            // selector is handed the group KEY this selector just returned
            // (TableViewSource.idl). Testing the argument against LeagueTeam in the identity
            // selector would yield an empty identity, which fails fast with E_INVALIDARG —
            // GroupBy throws and grouping silently never applies.
            _source.GroupBy(
                item => (object)GroupValue(item, key),
                groupKey => groupKey?.ToString() ?? "(none)");

            // Only now is grouping genuinely applied; every readout reads this, never _groupKey.
            _appliedGroupKey = key;
        }

        // case "hierarchy":
        // case "groupedhierarchy":
        //     Hierarchical (tree) rows are not available in this release, which is why the two
        //     matching ComboBoxItems ship disabled with a tooltip rather than hidden. No
        //     hierarchy verb exists on TableViewSource or TableView today — the only trace in
        //     the control source is TableViewRowInfo.h, which reserves row metadata "when
        //     hierarchical (tree) rows land" — so this stub stays prose rather than naming a
        //     member that does not exist. When hierarchy ships, apply it to this same source
        //     here, alongside the GroupBy stage above so grouping and hierarchy compose instead
        //     of replacing one another, and set the applied-mode field only after it returns.

        var grouped = _appliedGroupKey != "none";

        if (ExpandAllButton is not null)
        {
            ExpandAllButton.IsEnabled = grouped;
            CollapseAllButton.IsEnabled = grouped;
        }

        if (GroupBySelector is not null)
        {
            GroupBySelector.IsEnabled = grouped;
        }

        if (grouped)
        {
            DispatcherQueue.TryEnqueue(() => TeamsTable?.ExpandAllGroups());
        }

        RefreshReadouts(triggerColumn: null);
    }

    // Never returns the empty string: an empty group identity is an E_INVALIDARG fail-fast.
    private static string GroupValue(object item, string key)
    {
        if (item is not LeagueTeam team)
        {
            return "(none)";
        }

        var value = key switch
        {
            "Standing" => SortChipPalette.Band(team.Points),
            _ => string.IsNullOrWhiteSpace(team.Group) ? "(none)" : $"Group {team.Group}",
        };

        return string.IsNullOrWhiteSpace(value) ? "(none)" : value;
    }

    private static string GroupKeyLabel(string key) => key switch
    {
        "Standing" => "Standing",
        "Group" => "Group",
        _ => "(none)",
    };

    // ----- Actions -----
    //
    // Each of these probes an INTERACTION between sorting and grouping rather than the
    // happy path: a mutation that must re-position a row under the active sort (and move it
    // between groups when grouped on the mutated value), an add and a remove inside a live
    // group, and grouping an empty set.

    // Rows removed by "Clear all rows", held so the empty-set case is reversible.
    private readonly List<LeagueTeam> _stashedRows = new();
    private int _addedTeamCount;

    private void OnAwardWinClick(object sender, RoutedEventArgs e)
    {
        if (Teams.Count == 0)
        {
            SetLastAction("No rows to mutate.");
            return;
        }

        // Lowest points first: the row most likely to have to climb under a Points sort, and
        // to cross a Standing band boundary while grouped on Standing.
        var team = Teams.OrderBy(t => t.Points).ThenBy(t => t.GoalDifference).First();
        team.Wins += 1;
        team.GoalsFor += 2;

        SetLastAction($"{team.Team} won a match — now {team.Points} pts ({SortChipPalette.Band(team.Points)}).");
    }

    private void OnAddTeamClick(object sender, RoutedEventArgs e)
    {
        _addedTeamCount++;
        var team = new LeagueTeam
        {
            Group = "A",
            Team = $"Newcomers {_addedTeamCount}",
            Played = 6,
            Wins = 3,
            Draws = 1,
            Losses = 2,
            GoalsFor = 9,
            GoalsAgainst = 8,
        };

        Teams.Add(team);
        SetLastAction($"Added {team.Team} to Group A ({team.Points} pts).");
    }

    private void OnRemoveTeamClick(object sender, RoutedEventArgs e)
    {
        var team = Teams.LastOrDefault(t => t.Group == "A");
        if (team is null)
        {
            SetLastAction("Group A is already empty.");
            return;
        }

        Teams.Remove(team);
        SetLastAction($"Removed {team.Team} from Group A.");
    }

    private void OnToggleEmptyClick(object sender, RoutedEventArgs e)
    {
        if (Teams.Count > 0)
        {
            _stashedRows.Clear();
            _stashedRows.AddRange(Teams);
            Teams.Clear();
            SetLastAction("Cleared every row — a grouped empty set projects no groups at all.");
        }
        else
        {
            foreach (var team in _stashedRows)
            {
                Teams.Add(team);
            }

            _stashedRows.Clear();
            SetLastAction("Restored the dataset; the grouping is re-applied to the refilled source.");
        }
    }

    private void SetLastAction(string message)
    {
        if (LastActionText is not null)
        {
            LastActionText.Text = message;
        }

        RefreshReadouts(triggerColumn: null);
    }

    // ----- Programmatic single-column sort affordances -----

    private void OnSortPointsDescClick(object sender, RoutedEventArgs e)
        => TeamsTable.SortByColumn(PointsColumn, TableViewSortDirection.Descending);

    private void OnSortGroupAscClick(object sender, RoutedEventArgs e)
        => TeamsTable.SortByColumn(GroupColumn, TableViewSortDirection.Ascending);

    private void OnSortGoalDifferenceDescClick(object sender, RoutedEventArgs e)
        => TeamsTable.SortByColumn(GoalDifferenceColumn, TableViewSortDirection.Descending);

    private void OnTogglePointsClick(object sender, RoutedEventArgs e)
        => TeamsTable.ToggleSortDirection(PointsColumn);

    private void OnClearSortClick(object sender, RoutedEventArgs e)
        => TeamsTable.ClearSort();

    // ----- Sorted -----
    //
    // The control reshapes the rows itself: an ItemsSource that is not already a reshaping view is
    // projected through one internally, so a plain ObservableCollection sorts without the app
    // re-ordering anything. Sorting is raised first and can be cancelled to keep the ordering the
    // app's own; this page lets the control do it, so only Sorted is handled.
    //
    // Sorted fires AFTER the sort state has been applied and the header chevron published, so this
    // only refreshes the readouts.
    private void OnTableSorted(TableView sender, TableViewSortedEventArgs args)
    {
        _sortedFiredCount++;
        RefreshReadouts(args.Column);
    }

    // ----- Live readouts -----

    private void RefreshReadouts(TableViewColumn? triggerColumn)
    {
        if (TeamsTable is null || SortedFiredCountText is null)
        {
            return;
        }

        SortedFiredCountText.Text = _sortedFiredCount.ToString();
        TriggerColumnText.Text = triggerColumn is null ? "(cleared)" : ColumnLabel(triggerColumn);

        // Sort state lives on the COLUMN now - TableViewColumn.SortDirection - rather than on a
        // pair of scalar DPs on the table, so the active sort is whichever column is not None.
        TableViewColumn? column = null;
        foreach (var candidate in TeamsTable.Columns)
        {
            if (candidate.SortDirection != TableViewSortDirection.None)
            {
                column = candidate;
                break;
            }
        }

        ActiveSortText.Text = column is null
            ? "(none)"
            : $"{ColumnLabel(column)} {column.SortDirection}";

        VisibleRowsText.Text = Teams.Count.ToString();

        if (RemoveTeamButton is not null)
        {
            RemoveTeamButton.IsEnabled = Teams.Any(t => t.Group == "A");
            AwardWinButton.IsEnabled = Teams.Count > 0;
            ToggleEmptyButton.Content = Teams.Count > 0 ? "Clear all rows" : "Restore rows";
        }

        ShapingModeText.Text = _appliedGroupKey == "none"
            ? "Flat"
            : $"Grouped by {GroupKeyLabel(_appliedGroupKey)}";

        var preview = Teams.Take(5).Select((t, i) =>
            $"{i + 1}. {t.Group} {t.Team} ({t.Points}pts, {t.GoalDifference:+0;-0;0} GD)");
        TopRowsPreviewText.Text = preview.Any() ? string.Join(" · ", preview) : "(no rows)";
    }

    private static string ColumnLabel(TableViewColumn column)
        => column.Header?.ToString() ?? "(unnamed)";
}

/// <summary>
/// Page-private chip palette for the <c>Standing</c> template column.
///
/// Deliberately NOT reusing Pages/ShowcaseConverters.cs: those allocate a new
/// SolidColorBrush on every Convert call — per-cell churn under virtualization —
/// and have no HighContrast path. Every brush here is created once into a
/// static readonly field and shared; Convert never allocates.
/// </summary>
internal static class SortChipPalette
{
    private static readonly bool s_highContrast =
        new Windows.UI.ViewManagement.AccessibilitySettings().HighContrast;

    private static readonly SolidColorBrush s_transparent = new(Colors.Transparent);

    // HighContrast fallback: the user's guaranteed contrast pair, resolved once. Paired with a
    // transparent tint so the chip never fights the HighContrast theme's own colours.
    private static readonly SolidColorBrush s_ink =
        Application.Current.Resources["TextFillColorPrimaryBrush"] as SolidColorBrush
        ?? new SolidColorBrush(Colors.Gray);

    private static readonly Dictionary<string, SolidColorBrush> s_tints = new(StringComparer.Ordinal)
    {
        ["Qualified"] = new SolidColorBrush(ColorHelper.FromArgb(0x4D, 0x16, 0xA3, 0x4A)),
        ["Playoff"] = new SolidColorBrush(ColorHelper.FromArgb(0x4D, 0xF5, 0x9E, 0x0B)),
        ["Eliminated"] = new SolidColorBrush(ColorHelper.FromArgb(0x4D, 0x64, 0x74, 0x8B)),
    };

    private static readonly Dictionary<string, SolidColorBrush> s_dots = new(StringComparer.Ordinal)
    {
        ["Qualified"] = new SolidColorBrush(ColorHelper.FromArgb(0xFF, 0x16, 0xA3, 0x4A)),
        ["Playoff"] = new SolidColorBrush(ColorHelper.FromArgb(0xFF, 0xF5, 0x9E, 0x0B)),
        ["Eliminated"] = new SolidColorBrush(ColorHelper.FromArgb(0xFF, 0x64, 0x74, 0x8B)),
    };

    /// <summary>Qualification band for a points total. Never returns the empty string,
    /// so it is also safe as a GroupBy key.</summary>
    internal static string Band(object? value) => value switch
    {
        int points when points >= 12 => "Qualified",
        int points when points >= 7 => "Playoff",
        int => "Eliminated",
        _ => "(none)",
    };

    internal static Brush Tint(string band) => s_highContrast
        ? s_transparent
        : s_tints.TryGetValue(band, out var brush) ? brush : s_transparent;

    internal static Brush Dot(string band) => s_highContrast
        ? s_ink
        : s_dots.TryGetValue(band, out var brush) ? brush : s_ink;
}

/// <summary>Chip background tint for the Standing template column. Returns a shared brush.</summary>
public sealed partial class SortStandingTintConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, string language)
        => SortChipPalette.Tint(SortChipPalette.Band(value));

    public object ConvertBack(object value, Type targetType, object parameter, string language)
        => throw new NotSupportedException();
}

/// <summary>Solid dot fill for the Standing chip. Returns a shared brush.</summary>
public sealed partial class SortStandingDotConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, string language)
        => SortChipPalette.Dot(SortChipPalette.Band(value));

    public object ConvertBack(object value, Type targetType, object parameter, string language)
        => throw new NotSupportedException();
}

/// <summary>Chip label — "Qualified" / "Playoff" / "Eliminated".</summary>
public sealed partial class SortStandingTextConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, string language)
        => SortChipPalette.Band(value);

    public object ConvertBack(object value, Type targetType, object parameter, string language)
        => throw new NotSupportedException();
}
