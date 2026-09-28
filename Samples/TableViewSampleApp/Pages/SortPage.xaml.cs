// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Tabular;
// tabular-namespace TableView aliases: disambiguate from the (stale-mock) base Microsoft.UI.Xaml.Controls.TableView projection
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
/// Demonstrates the aligned TableView single-column sort surface and the
/// consumer-owned re-shape model:
///
///   * The control owns sort STATE — direction, the header SortIndicator
///     chevron, the public SortByColumn / ToggleSortDirection / ClearSort API,
///     and the Sorted event.
///   * The consumer owns the DATA — when Sorted fires, read the active sorted
///     column and re-order the bound items source however you like (a LINQ
///     OrderBy here; could equally be a comparer or a server-side query).
///
/// The dataset is a fictional group-stage standings table where ordering by
/// Points — with Goal Difference as the tiebreaker — is the canonical sort.
/// A new sort always replaces the previous one (single-column), matching the
/// aligned control's scalar sort surface.
/// </summary>
public sealed partial class SortPage : Page
{
    private int _sortedFiredCount;

    // Master snapshot of the original (unsorted) row order. When Sorted fires
    // we rebuild Teams from this list; ClearSort fires with no sorted columns,
    // which naturally restores the original order.
    private readonly List<LeagueTeam> _master;

    private static readonly Dictionary<string, Func<LeagueTeam, IComparable?>> s_keySelectors =
        new(StringComparer.Ordinal)
        {
            ["Group"]          = t => t.Group,
            ["Team"]           = t => t.Team,
            ["Wins"]           = t => t.Wins,
            ["Draws"]          = t => t.Draws,
            ["Losses"]         = t => t.Losses,
            ["GoalsFor"]       = t => t.GoalsFor,
            ["GoalsAgainst"]   = t => t.GoalsAgainst,
            ["GoalDifference"] = t => t.GoalDifference,
            ["Points"]         = t => t.Points,
        };

    public SortPage()
    {
        Teams = LeagueData.All();
        _master = Teams.ToList();
        InitializeComponent();
        TeamsTable.ItemsSource = Teams;
        Loaded += (_, _) => RefreshReadouts(triggerColumn: null);
    }

    public ObservableCollection<LeagueTeam> Teams { get; }

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

        var preview = Teams.Take(5).Select((t, i) =>
            $"{i + 1}. {t.Group} {t.Team} ({t.Points}pts, {t.GoalDifference:+0;-0;0} GD)");
        TopRowsPreviewText.Text = preview.Any() ? string.Join(" · ", preview) : "(no rows)";
    }

    private static string ColumnLabel(TableViewColumn column)
        => column.Header?.ToString() ?? "(unnamed)";
}
