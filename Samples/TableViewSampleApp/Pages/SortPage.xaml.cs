// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Collections.Specialized;
using System.ComponentModel;
using System.Globalization;
using System.Linq;
using Microsoft.UI;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Tabular;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Media;
using TableViewSampleApp.Converters;
using TableViewSampleApp.Data;
using TableViewSampleApp.Helpers;
using TableViewSampleApp.Models;
// Tabular aliases keep the sample code concise. SortDirection is the shared
// Microsoft.UI.Xaml.Controls.Tabular.SortDirection { None, Ascending, Descending }.
using TableView = Microsoft.UI.Xaml.Controls.Tabular.TableView;
using TableViewColumn = Microsoft.UI.Xaml.Controls.Tabular.TableViewColumn;
using TableViewSortDirection = Microsoft.UI.Xaml.Controls.Tabular.SortDirection;

namespace TableViewSampleApp.Pages;

/// <summary>
/// Sorting: TableView owns single-column sort STATE (direction, the header arrow, the
/// SortByColumn / ToggleSortDirection / ClearSort API and the Sorted event) and reorders the
/// bound rows itself. Template columns (Seeded CheckBox, Next match date picker, Kickoff time
/// picker, Country and Standing chips) sort through SortMemberPath, by the value behind the
/// control. Grouped shaping plus a sort reorders rows within each group.
/// </summary>
public sealed partial class SortPage : Page
{
    private TableViewSource? _source;          // created ONCE; reshaped in place, never rebuilt
    private string _appliedMode = "flat";      // written only after GroupBy/ClearGroupBy returns
    private string _appliedKey = "Group";
    private bool _isBulkUpdate;
    private bool _isResorting;
    private bool _resortQueued;
    private int _sortedFiredCount;
    private string _lastSortedColumn = "(none)";
    private int _lateEntrantCount;

    // Rows removed by "Clear all rows", held so the empty state is reversible.
    private readonly List<LeagueTeam> _stashedRows = new();

    public SortPage()
    {
        _source = TableViewSource.From(Teams);
        InitializeComponent();
        Loaded += OnPageLoaded;
        Unloaded += OnPageUnloaded;
        RefreshReadouts();
    }

    public ObservableCollection<LeagueTeam> Teams { get; } = LeagueData.All();

    public TableViewSource? Source => _source;

    private void OnPageLoaded(object sender, RoutedEventArgs e)
    {
        Teams.CollectionChanged += OnTeamsCollectionChanged;
        foreach (var team in Teams)
        {
            team.PropertyChanged += OnTeamChanged;
        }

        RefreshReadouts();
    }

    private void OnPageUnloaded(object sender, RoutedEventArgs e)
    {
        Teams.CollectionChanged -= OnTeamsCollectionChanged;
        foreach (var team in Teams)
        {
            team.PropertyChanged -= OnTeamChanged;
        }
    }

    private void OnTeamsCollectionChanged(object? sender, NotifyCollectionChangedEventArgs e)
    {
        foreach (LeagueTeam team in e.OldItems ?? Array.Empty<LeagueTeam>())
        {
            team.PropertyChanged -= OnTeamChanged;
        }

        foreach (LeagueTeam team in e.NewItems ?? Array.Empty<LeagueTeam>())
        {
            team.PropertyChanged += OnTeamChanged;
        }
    }

    // ---- Programmatic sort -------------------------------------------------------------------

    private void OnSortPointsDescClick(object sender, RoutedEventArgs e) => SortBy(PointsColumn, TableViewSortDirection.Descending);

    private void OnSortGroupAscClick(object sender, RoutedEventArgs e) => SortBy(GroupColumn, TableViewSortDirection.Ascending);

    private void OnSortGoalDifferenceDescClick(object sender, RoutedEventArgs e) => SortBy(GoalDifferenceColumn, TableViewSortDirection.Descending);

    private void OnSortNextMatchClick(object sender, RoutedEventArgs e) => SortBy(NextMatchColumn, TableViewSortDirection.Ascending);

    private void OnSortKickoffClick(object sender, RoutedEventArgs e) => SortBy(KickoffColumn, TableViewSortDirection.Ascending);

    private void OnSortSeededClick(object sender, RoutedEventArgs e) => SortBy(SeededColumn, TableViewSortDirection.Descending);

    private void SortBy(TableViewColumn column, TableViewSortDirection direction)
    {
        TeamsTable.SortByColumn(column, direction);
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "SortByColumn({0}, {1})", ColumnLabel(column), direction));
    }

    private void OnTogglePointsClick(object sender, RoutedEventArgs e)
    {
        TeamsTable.ToggleSortDirection(PointsColumn);
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "ToggleSortDirection(Pts) -> {0}", PointsColumn.SortDirection));
    }

    private void OnClearSortClick(object sender, RoutedEventArgs e)
    {
        TeamsTable.ClearSort();
        SetLastAction("ClearSort(): rows are back in source order");
    }

    // Sorted fires after the sort state is applied and the header arrow is published: header
    // clicks and the API calls above alike. Column is null when the sort was cleared.
    private void OnTableSorted(TableView sender, TableViewSortedEventArgs args)
    {
        _sortedFiredCount++;
        _lastSortedColumn = args.Column is null ? "cleared" : ColumnLabel(args.Column);
        if (_isResorting)
        {
            RefreshReadouts();
            return;
        }

        SetLastAction(args.Column is null
            ? "Sort cleared"
            : string.Format(CultureInfo.CurrentCulture, "Sorted by {0} {1}", ColumnLabel(args.Column), args.Direction));
    }

    // ---- Re-sorting after a value change -----------------------------------------------------
    //
    // The control sorts when asked and when the collection changes. A PropertyChanged on the
    // sorted value does NOT re-position the row by itself, so the page applies the active sort
    // again whenever a value it depends on changes.

    private TableViewColumn? ActiveSortColumn() =>
        TeamsTable?.Columns.FirstOrDefault(c => c.SortDirection != TableViewSortDirection.None);

    private bool ResortIfSortedOn(params string[] propertyNames)
    {
        var column = ActiveSortColumn();
        if (column is null || !propertyNames.Contains(SampleShaping.SortPathOf(column)))
        {
            return false;
        }

        // SortByColumn ignores a request for the sort that is already applied, so clear the
        // column's sort and apply it again: the rows are re-ordered by the new values.
        var direction = column.SortDirection;
        _isResorting = true;
        try
        {
            TeamsTable.SortByColumn(column, TableViewSortDirection.None);
            TeamsTable.SortByColumn(column, direction);
        }
        finally
        {
            _isResorting = false;
        }

        return true;
    }

    private void OnTeamChanged(object? sender, PropertyChangedEventArgs e)
    {
        if (sender is not LeagueTeam team || _isBulkUpdate || e.PropertyName is null)
        {
            return;
        }

        switch (e.PropertyName)
        {
            case nameof(LeagueTeam.NextMatchDate):
                SetLastAction(string.Format(CultureInfo.CurrentCulture, "Next match -> {0} for {1}", team.NextMatchText, team.Team));
                break;
            case nameof(LeagueTeam.Kickoff):
                SetLastAction(string.Format(CultureInfo.CurrentCulture, "Kickoff -> {0:hh\\:mm} for {1}", team.Kickoff, team.Team));
                break;
            case nameof(LeagueTeam.IsSeeded):
                SetLastAction(string.Format(CultureInfo.CurrentCulture, "Seeded -> {0} for {1}", team.IsSeeded ? "checked" : "unchecked", team.Team));
                break;
            case nameof(LeagueTeam.Team):
                SetLastAction(string.Format(CultureInfo.CurrentCulture, "Renamed a team to {0}", team.Team));
                break;
            default:
                return;
        }

        // Let the in-cell editor finish its own update first, then re-sort and re-group once.
        var property = e.PropertyName;
        if (_resortQueued)
        {
            return;
        }

        _resortQueued = true;
        DispatcherQueue.TryEnqueue(() =>
        {
            _resortQueued = false;
            if (!IsLoaded)
            {
                return;
            }

            ReapplyIfGroupedOn(property);
            ResortIfSortedOn(property, nameof(LeagueTeam.NextMatchText));
            SampleShaping.Reselect(TeamsTable, team, Teams.Count + 40);
            RefreshReadouts();
        });
    }

    // ---- Actions ----------------------------------------------------------------------------

    private void OnAwardWinClick(object sender, RoutedEventArgs e)
    {
        // The lowest-ranked team that has a loss to overturn, against the strongest rival in its
        // group that has a win to give back. Both keep Played = W + D + L, and the two extra goals
        // count once as goals for and once as goals against, so the group's ΣGF = ΣGA still holds.
        var team = Teams
            .Where(t => t.Losses > 0 && Teams.Any(r => r != t && r.Group == t.Group && r.Wins > 0))
            .OrderBy(t => t.Points).ThenBy(t => t.GoalDifference)
            .FirstOrDefault();
        if (team is null)
        {
            SetLastAction("No team has a loss to overturn.");
            return;
        }

        var rival = Teams
            .Where(r => r != team && r.Group == team.Group && r.Wins > 0)
            .OrderByDescending(r => r.Points).ThenByDescending(r => r.GoalDifference)
            .First();

        _isBulkUpdate = true;
        try
        {
            team.Losses -= 1;
            team.Wins += 1;
            team.GoalsFor += 2;
            rival.Wins -= 1;
            rival.Losses += 1;
            rival.GoalsAgainst += 2;
        }
        finally
        {
            _isBulkUpdate = false;
        }

        ReapplyIfGroupedOn("Standing");
        var resorted = ResortIfSortedOn(
            nameof(LeagueTeam.Points), nameof(LeagueTeam.Wins), nameof(LeagueTeam.Losses),
            nameof(LeagueTeam.GoalsFor), nameof(LeagueTeam.GoalsAgainst), nameof(LeagueTeam.GoalDifference));
        SetLastAction(string.Format(
            CultureInfo.CurrentCulture,
            "Overturned a loss: {0} now beat {1} and have {2} pts ({3}){4}",
            team.Team,
            rival.Team,
            team.Points,
            SortChipPalette.Band(team.Points),
            resorted ? "; re-sorted, so the row moved" : string.Empty));
    }

    private void OnPostponeClick(object sender, RoutedEventArgs e)
    {
        if (TeamsTable.SelectedItem is not LeagueTeam team)
        {
            SetLastAction("No row selected.");
            return;
        }

        var from = team.NextMatchText;
        _isBulkUpdate = true;
        try
        {
            team.NextMatchDate = team.NextMatchDate.AddDays(7);
        }
        finally
        {
            _isBulkUpdate = false;
        }

        var resorted = ResortIfSortedOn(nameof(LeagueTeam.NextMatchDate), nameof(LeagueTeam.NextMatchText));
        SampleShaping.Reselect(TeamsTable, team, Teams.Count + 40);
        SetLastAction(string.Format(
            CultureInfo.CurrentCulture,
            "Postponed the {0} match from {1} to {2}{3}",
            team.Team,
            from,
            team.NextMatchText,
            resorted ? "; re-sorted, so the row moved" : string.Empty));
    }

    private void OnToggleTextDateClick(object sender, RoutedEventArgs e)
    {
        var show = NextMatchTextColumn.Visibility != Visibility.Visible;
        SetTextDateColumnVisible(show);
        SetLastAction(show ? "Showed the Next match (text) column" : "Hid the Next match (text) column");
    }

    private void OnSortAsTextClick(object sender, RoutedEventArgs e)
    {
        SetTextDateColumnVisible(true);
        TeamsTable.SortByColumn(NextMatchTextColumn, TableViewSortDirection.Ascending);

        // Find the first neighbouring pair the alphabetical order gets wrong.
        var byText = Teams.OrderBy(t => t.NextMatchText, StringComparer.CurrentCulture).ToList();
        for (var i = 0; i + 1 < byText.Count; i++)
        {
            if (byText[i].NextMatchDate.Date > byText[i + 1].NextMatchDate.Date)
            {
                SetLastAction(string.Format(
                    CultureInfo.CurrentCulture,
                    "Sorted Next match (text): {0} comes before {1}, although {1} is the earlier date",
                    byText[i].NextMatchText,
                    byText[i + 1].NextMatchText));
                return;
            }
        }

        SetLastAction("Sorted Next match (text)");
    }

    private void SetTextDateColumnVisible(bool visible)
    {
        NextMatchTextColumn.Visibility = visible ? Visibility.Visible : Visibility.Collapsed;
        ToggleTextDateButton.Content = visible ? "Hide the text-date column" : "Show the text-date column";
    }

    private void OnAddTeamClick(object sender, RoutedEventArgs e)
    {
        var team = LeagueData.LateEntrant(_lateEntrantCount++, "A");
        Teams.Add(team);
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Added {0} to Group A ({1} pts, no games played)", team.Team, team.Points));
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
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Removed {0} from Group A", team.Team));
    }

    private void OnToggleEmptyClick(object sender, RoutedEventArgs e)
    {
        if (Teams.Count > 0)
        {
            _stashedRows.Clear();
            _stashedRows.AddRange(Teams);
            Teams.Clear();
            SetLastAction("Cleared every row: the EmptyTemplate shows, flat or grouped");
            return;
        }

        foreach (var team in _stashedRows)
        {
            Teams.Add(team);
        }

        var restored = _stashedRows.Count;
        _stashedRows.Clear();
        SetLastAction(string.Format(CultureInfo.CurrentCulture, "Restored {0:N0} rows; the sort and grouping apply again", restored));
    }

    // ---- Readouts ---------------------------------------------------------------------------

    private void RefreshReadouts()
    {
        if (TeamsTable is null || ActiveSortText is null || SortedFiredText is null || TopRowsText is null
            || RowsText is null || ToggleEmptyButton is null)
        {
            return;
        }

        var column = ActiveSortColumn();
        ActiveSortText.Text = column is null
            ? "(none)"
            : string.Format(CultureInfo.CurrentCulture, "{0} {1}", ColumnLabel(column), column.SortDirection);
        SortedFiredText.Text = string.Format(CultureInfo.CurrentCulture, "{0:N0} (last: {1})", _sortedFiredCount, _lastSortedColumn);
        RowsText.Text = SampleShaping.RowCountText(Teams.Count);
        ToggleEmptyButton.Content = Teams.Count > 0 ? "Clear all rows" : "Restore rows";

        var top = InViewOrder().Take(5).Select((t, i) => string.Format(
            CultureInfo.CurrentCulture, "{0}. {1} ({2} pts, {3:+0;-0;0} GD)", i + 1, t.Team, t.Points, t.GoalDifference)).ToList();
        TopRowsText.Text = top.Count > 0 ? string.Join(Environment.NewLine, top) : "(no rows)";
    }

    // The rows in the order the table shows them (shared rules: SampleShaping.InViewOrder).
    private IEnumerable<LeagueTeam> InViewOrder() =>
        SampleShaping.InViewOrder(TeamsTable, Teams, SortKey, _appliedMode == "grouped" ? t => KeyOf(t, _appliedKey) : null);

    private static IComparable? SortKey(LeagueTeam team, string path) => path switch
    {
        nameof(LeagueTeam.Group) => team.Group,
        nameof(LeagueTeam.Team) => team.Team,
        nameof(LeagueTeam.Country) => team.Country,
        nameof(LeagueTeam.IsSeeded) => team.IsSeeded,
        nameof(LeagueTeam.NextMatchDate) => team.NextMatchDate,
        nameof(LeagueTeam.NextMatchText) => team.NextMatchText,
        nameof(LeagueTeam.Kickoff) => team.Kickoff,
        nameof(LeagueTeam.Wins) => team.Wins,
        nameof(LeagueTeam.Draws) => team.Draws,
        nameof(LeagueTeam.Losses) => team.Losses,
        nameof(LeagueTeam.GoalsFor) => team.GoalsFor,
        nameof(LeagueTeam.GoalsAgainst) => team.GoalsAgainst,
        nameof(LeagueTeam.GoalDifference) => team.GoalDifference,
        nameof(LeagueTeam.Points) => team.Points,
        _ => null,
    };

    // GroupBy key for a standings row. Never blank: an empty group identity fails fast.
    private static object KeyOf(LeagueTeam? team, string key)
    {
        if (team is null)
        {
            return SampleShaping.NoneKey;
        }

        var value = key switch
        {
            "Standing" => SortChipPalette.Band(team.Points),
            nameof(LeagueTeam.Country) => team.Country,
            nameof(LeagueTeam.IsSeeded) => team.IsSeeded ? "Seeded" : "Unseeded",
            _ => string.IsNullOrWhiteSpace(team.Group) ? string.Empty : string.Format(CultureInfo.CurrentCulture, "Group {0}", team.Group),
        };

        return string.IsNullOrWhiteSpace(value) ? SampleShaping.NoneKey : value;
    }

    private static string ColumnLabel(TableViewColumn column) =>
        column.Header?.ToString() ?? "(unnamed)";

    #region Sample scaffolding (generic; see FIX-PLAN §6)

    private void OnShapingModeChanged(object sender, SelectionChangedEventArgs e) => ApplyShaping(announce: true);

    private void OnGroupKeyChanged(object sender, SelectionChangedEventArgs e) => ApplyShaping(announce: true);

    private void ApplyShaping(bool announce)
    {
        // Fires during InitializeComponent (each selector's SelectedIndex="0"), before the
        // later-declared elements exist. Guard EVERY element this path touches.
        if (_source is null || TeamsTable is null || ShapingModeSelector is null || GroupKeySelector is null
            || ExpandAllButton is null || CollapseAllButton is null || ShapingModeText is null)
        {
            return;
        }

        var mode = SampleShaping.SelectedTag(ShapingModeSelector, "flat");
        var key = SampleShaping.SelectedTag(GroupKeySelector, "Group");
        var selected = TeamsTable.SelectedItem;

        switch (mode)
        {
            case "grouped":
                // The key selector receives the ROW; the identity selector receives the KEY.
                _source.GroupBy(item => KeyOf(item as LeagueTeam, key), SampleShaping.GroupIdentity);
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
        SampleShaping.Reselect(TeamsTable, selected, Teams.Count + 40, RefreshReadouts);
        UpdateShapingGating();
        if (announce)
        {
            SetLastAction(mode == "grouped"
                ? string.Format(CultureInfo.CurrentCulture, "Shaping -> Grouped by {0}", SampleShaping.Label(GroupKeySelector))
                : "Shaping -> Flat");
        }
        else
        {
            RefreshReadouts();
        }
    }

    // Call after ANY write to the grouped-on property: from an action or from an in-cell edit.
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
        TeamsTable.ExpandAllGroups();
        SetLastAction("Expanded all groups");
    }

    private void OnCollapseAllClick(object sender, RoutedEventArgs e)
    {
        TeamsTable.CollapseAllGroups();
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

/// <summary>
/// Page-private chip palette for the Standing and Country template columns. Every brush is
/// created once into a static field and shared, so Convert never allocates. Under a Contrast
/// theme the tint drops to transparent and the dot to the theme text brush (the shared
/// <see cref="ChipBrushes"/> rule).
/// </summary>
internal static class SortChipPalette
{
    private const byte TintAlpha = 0x33;

    private static readonly Dictionary<string, SolidColorBrush> s_standingTints = BuildStanding(TintAlpha);
    private static readonly Dictionary<string, SolidColorBrush> s_standingDots = BuildStanding(0xFF);
    private static readonly Dictionary<string, SolidColorBrush> s_countryTints = BuildCountry(TintAlpha);
    private static readonly Dictionary<string, SolidColorBrush> s_countryDots = BuildCountry(0xFF);
    private static readonly SolidColorBrush s_fallbackTint = new(ColorHelper.FromArgb(TintAlpha, 0x64, 0x74, 0x8B));
    private static readonly SolidColorBrush s_fallbackDot = new(ColorHelper.FromArgb(0xFF, 0x64, 0x74, 0x8B));

    /// <summary>Qualification band for a points total. Never returns the empty string,
    /// so it is also safe as a GroupBy key.</summary>
    internal static string Band(object? value) => value switch
    {
        int points when points >= 12 => "Qualified",
        int points when points >= 7 => "Playoff",
        int => "Eliminated",
        _ => SampleShaping.NoneKey,
    };

    internal static Brush StandingTint(string band) => Tint(s_standingTints, band);

    internal static Brush StandingDot(string band) => Dot(s_standingDots, band);

    internal static Brush CountryTint(string? country) => Tint(s_countryTints, country);

    internal static Brush CountryDot(string? country) => Dot(s_countryDots, country);

    private static Brush Tint(Dictionary<string, SolidColorBrush> map, string? key) =>
        ChipBrushes.IsHighContrast ? ChipBrushes.Transparent
        : key is not null && map.TryGetValue(key, out var brush) ? brush : s_fallbackTint;

    private static Brush Dot(Dictionary<string, SolidColorBrush> map, string? key) =>
        ChipBrushes.IsHighContrast ? ChipBrushes.HighContrastForeground
        : key is not null && map.TryGetValue(key, out var brush) ? brush : s_fallbackDot;

    private static Dictionary<string, SolidColorBrush> BuildStanding(byte alpha) => new(StringComparer.Ordinal)
    {
        ["Qualified"] = new SolidColorBrush(ColorHelper.FromArgb(alpha, 0x16, 0xA3, 0x4A)),
        ["Playoff"] = new SolidColorBrush(ColorHelper.FromArgb(alpha, 0xF5, 0x9E, 0x0B)),
        ["Eliminated"] = new SolidColorBrush(ColorHelper.FromArgb(alpha, 0x64, 0x74, 0x8B)),
    };

    private static Dictionary<string, SolidColorBrush> BuildCountry(byte alpha) => new(StringComparer.Ordinal)
    {
        ["England"] = new SolidColorBrush(ColorHelper.FromArgb(alpha, 0xEF, 0x44, 0x44)),
        ["Spain"] = new SolidColorBrush(ColorHelper.FromArgb(alpha, 0xF5, 0x9E, 0x0B)),
        ["Italy"] = new SolidColorBrush(ColorHelper.FromArgb(alpha, 0x0E, 0xA5, 0xE9)),
        ["Germany"] = new SolidColorBrush(ColorHelper.FromArgb(alpha, 0x64, 0x74, 0x8B)),
        ["France"] = new SolidColorBrush(ColorHelper.FromArgb(alpha, 0x63, 0x66, 0xF1)),
        ["Portugal"] = new SolidColorBrush(ColorHelper.FromArgb(alpha, 0x22, 0xC5, 0x5E)),
        ["Netherlands"] = new SolidColorBrush(ColorHelper.FromArgb(alpha, 0xF9, 0x73, 0x16)),
        ["Belgium"] = new SolidColorBrush(ColorHelper.FromArgb(alpha, 0xA8, 0x55, 0xF7)),
    };
}

/// <summary>Chip background tint for the Standing template column. Returns a shared brush.</summary>
public sealed partial class SortStandingTintConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, string language)
        => SortChipPalette.StandingTint(SortChipPalette.Band(value));

    public object ConvertBack(object value, Type targetType, object parameter, string language)
        => throw new NotSupportedException();
}

/// <summary>Solid dot fill for the Standing chip. Returns a shared brush.</summary>
public sealed partial class SortStandingDotConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, string language)
        => SortChipPalette.StandingDot(SortChipPalette.Band(value));

    public object ConvertBack(object value, Type targetType, object parameter, string language)
        => throw new NotSupportedException();
}

/// <summary>Chip label: "Qualified", "Playoff" or "Eliminated".</summary>
public sealed partial class SortStandingTextConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, string language)
        => SortChipPalette.Band(value);

    public object ConvertBack(object value, Type targetType, object parameter, string language)
        => throw new NotSupportedException();
}

/// <summary>Chip background tint for the Country template column. Returns a shared brush.</summary>
public sealed partial class SortCountryTintConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, string language)
        => SortChipPalette.CountryTint(value as string);

    public object ConvertBack(object value, Type targetType, object parameter, string language)
        => throw new NotSupportedException();
}

/// <summary>Solid dot fill for the Country chip. Returns a shared brush.</summary>
public sealed partial class SortCountryDotConverter : IValueConverter
{
    public object Convert(object value, Type targetType, object parameter, string language)
        => SortChipPalette.CountryDot(value as string);

    public object ConvertBack(object value, Type targetType, object parameter, string language)
        => throw new NotSupportedException();
}
