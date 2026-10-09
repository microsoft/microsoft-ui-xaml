// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using TableViewSampleApp.Data;
using TableViewSampleApp.Helpers;
using TableViewSampleApp.Models;
using TableViewColumn = Microsoft.UI.Xaml.Controls.Tabular.TableViewColumn;

namespace TableViewSampleApp.Pages;

public sealed partial class SortPage
{
    private int _sortedFiredCount;
    private string _lastSortedColumn = "(none)";

    protected override void RefreshReadouts()
    {
        var column = SampleShaping.ActiveSortColumn(TeamsTable);
        ActiveSortText.Text = column is null
            ? "(none)"
            : string.Format(CultureInfo.CurrentCulture, "{0} {1}", ColumnLabel(column), column.SortDirection);
        SortedFiredText.Text = string.Format(CultureInfo.CurrentCulture, "{0:N0} (last: {1})", _sortedFiredCount, _lastSortedColumn);
        Status.Rows = SampleShaping.RowCountText(Teams.Count);
        ToggleEmptyButton.Content = Teams.Count > 0 ? "Clear all rows" : "Restore rows";
        PostponeButton.IsEnabled = TeamsTable.SelectedItem is LeagueTeam;

        var top = InViewOrder().Take(5).Select((t, i) => string.Format(
            CultureInfo.CurrentCulture, "{0}. {1} ({2} pts, {3:+0;-0;0} GD)", i + 1, t.Team, t.Points, t.GoalDifference)).ToList();
        TopRowsText.Text = top.Count > 0 ? string.Join(Environment.NewLine, top) : "(no rows)";
    }

    private IEnumerable<LeagueTeam> InViewOrder() =>
        SampleShaping.InViewOrder(TeamsTable, Teams, SortKey, IsGrouped ? t => LeagueData.GroupKeyOf(t, AppliedGroupKey) : null, SortOrdersGroups);

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

    private static string ColumnLabel(TableViewColumn column) =>
        column.Header?.ToString() ?? "(unnamed)";
}
