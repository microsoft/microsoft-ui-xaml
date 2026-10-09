// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Globalization;
using System.Linq;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls.Tabular;
using TableViewSampleApp.Data;
using TableViewSampleApp.Helpers;
using TableViewSampleApp.Models;
// Tabular aliases keep the sample code concise. SortDirection is the shared
// Microsoft.UI.Xaml.Controls.Tabular.SortDirection { None, Ascending, Descending }.
using TableView = Microsoft.UI.Xaml.Controls.Tabular.TableView;
using TableViewColumn = Microsoft.UI.Xaml.Controls.Tabular.TableViewColumn;
using TableViewSortDirection = Microsoft.UI.Xaml.Controls.Tabular.SortDirection;

namespace TableViewSampleApp.Pages;

// Template columns sort through SortMemberPath, by the value behind the control. Grouped, a sort
// declared after GroupBy sorts within each group; one declared before it orders the groups too.
public sealed partial class SortPage : SamplePageBase
{
    private bool _isResorting;
    private TableViewColumn? _reportedSortColumn;
    private TableViewSortDirection _reportedSortDirection;

    // Edits made in the same dispatcher turn, re-applied together by one queued callback.
    private readonly HashSet<string> _pendingProperties = new();
    private int _lateEntrantCount;

    // Rows removed by "Clear all rows", held so the empty state is reversible.
    private readonly List<LeagueTeam> _stashedRows = new();

    public SortPage()
    {
        // <snippet>
        Source = TableViewSource.From(Teams);     // created once; reshaped in place, never rebuilt
        InitializeComponent();
        // </snippet>
        InitializeSample(Status, Shaping.Attach(TeamsTable, Source, (row, key) => LeagueData.GroupKeyOf(row as LeagueTeam, key)));
        TrackItems(Teams, OnTeamChanged);

        // A callback queued just before the page unloaded never runs; start clean on the next Loaded.
        TrackLifetime(_pendingProperties.Clear);
    }

    public ObservableCollection<LeagueTeam> Teams { get; } = LeagueData.All();

    public TableViewSource Source { get; }

    // <snippet>
    // ---- Programmatic sort: one column at a time; a new sort replaces the previous one -------

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

        // A re-sort (ResortIfSortedOn) raises Sorted again with the same column and direction,
        // possibly on a later turn: that is not a new sort, so keep the action's own narration.
        var unchanged = ReferenceEquals(args.Column, _reportedSortColumn) && args.Direction == _reportedSortDirection;
        _reportedSortColumn = args.Column;
        _reportedSortDirection = args.Direction;
        if (_isResorting || unchanged)
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

    private bool ResortIfSortedOn(params string[] propertyNames)
    {
        var column = SampleShaping.ActiveSortColumn(TeamsTable);
        if (column is null || !propertyNames.Contains(SampleShaping.SortPathOf(column)))
        {
            return false;
        }

        // SortByColumn ignores a request for the sort already applied; re-declaring the same path on the
        // source re-reads every key. That is a new declaration after any GroupBy, hence OnSortRedeclared.
        _isResorting = true;
        try
        {
            Source.Sort(SampleShaping.SortPathOf(column)!, column.SortDirection);
            OnSortRedeclared();
        }
        finally
        {
            _isResorting = false;
        }

        return true;
    }

    private void OnTeamChanged(object? sender, PropertyChangedEventArgs e)
    {
        if (sender is not LeagueTeam team || IsBulkUpdating || e.PropertyName is null)
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

        // Let the in-cell editor finish its own update first, then re-group and re-sort once for
        // every property edited in this turn.
        if (_pendingProperties.Count == 0)
        {
            EnqueueIfLoaded(ApplyPendingEdits);
        }

        _pendingProperties.Add(e.PropertyName);
    }

    private void ApplyPendingEdits()
    {
        var properties = new List<string>(_pendingProperties);
        _pendingProperties.Clear();
        foreach (var property in properties)
        {
            ReapplyIfGroupedOn(property);
        }

        // The text-date column sorts by NextMatchText, which follows NextMatchDate.
        properties.Add(nameof(LeagueTeam.NextMatchText));
        ResortIfSortedOn(properties.ToArray());
        RefreshReadouts();
    }

    // </snippet>

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

        using (BeginBulkUpdate())
        {
            team.Losses -= 1;
            team.Wins += 1;
            team.GoalsFor += 2;
            rival.Wins -= 1;
            rival.Losses += 1;
            rival.GoalsAgainst += 2;
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
            LeagueData.Band(team.Points),
            resorted ? "; re-sorted, so the row moved" : string.Empty));
    }

    // <snippet>
    private void OnPostponeClick(object sender, RoutedEventArgs e)
    {
        if (TeamsTable.SelectedItem is not LeagueTeam team)
        {
            SetLastAction("No row selected.");
            return;
        }

        var from = team.NextMatchText; // snippet:skip
        using (BeginBulkUpdate())
        {
            team.NextMatchDate = team.NextMatchDate.AddDays(7);
        }

        var resorted = ResortIfSortedOn(nameof(LeagueTeam.NextMatchDate), nameof(LeagueTeam.NextMatchText));
        SetLastAction(string.Format(
            CultureInfo.CurrentCulture,
            "Postponed the {0} match from {1} to {2}{3}",
            team.Team,
            from,
            team.NextMatchText,
            resorted ? "; re-sorted, so the row moved" : string.Empty));
    }
    // </snippet>

    private void OnTeamsSelectionChanged(TableView sender, Microsoft.UI.Xaml.Controls.SelectionChangedEventArgs args) => RefreshReadouts();

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
}
