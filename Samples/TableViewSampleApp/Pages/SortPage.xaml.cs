// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.ObjectModel;
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
using TableViewSampleApp.Data;
using TableViewSampleApp.Models;

namespace TableViewSampleApp.Pages;

/// <summary>
/// Demonstrates TableView's single-column sort. The control reshapes the view;
/// the bound collection remains in source order. Readouts describe public
/// control state and source counts, never UIA or displayed order.
/// </summary>
public sealed partial class SortPage : Page
{
    private int _sortedFiredCount;

    public SortPage()
    {
        Teams = LeagueData.All();
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

    }

    private static string ColumnLabel(TableViewColumn column)
        => column.Header?.ToString() ?? "(unnamed)";
}
