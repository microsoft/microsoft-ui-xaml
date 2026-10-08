// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Globalization;
using System.Linq;
using Microsoft.UI.Xaml.Controls.Tabular;
using TableViewSampleApp.Helpers;
using TableViewSortDirection = Microsoft.UI.Xaml.Controls.Tabular.SortDirection;

namespace TableViewSampleApp.Pages;

// RightToLeftPage readouts: FlowDirection, Shown, Sort, Column order and Rows. The Shaping and Last
// action rows are written by SamplePageBase; this file only computes the page's own values.
public sealed partial class RightToLeftPage
{
    protected override void RefreshReadouts()
    {
        Status.Rows = SampleShaping.RowCountText(People.Count);
        var term = FilterBox?.Text.Trim() ?? string.Empty;
        var shown = term.Length == 0 ? People.Count : People.Count(p => Matches(p, term));
        ShownText.Text = string.Format(CultureInfo.CurrentCulture, "{0:N0} of {1:N0}", shown, People.Count);
        FlowDirectionText.Text = PeopleTable.FlowDirection.ToString();
        ColumnOrderText.Text = PeopleTable.Columns.SequenceEqual(_originalColumnOrder)
            ? "Original"
            : string.Join(", ", PeopleTable.Columns.Select(c => c.Header));
    }

    // Raised for header clicks and for SortByColumn alike.
    private void OnTableSorted(TableView sender, TableViewSortedEventArgs e)
    {
        SortText.Text = e.Column is null || e.Direction == TableViewSortDirection.None
            ? "None"
            : string.Format(CultureInfo.CurrentCulture, "{0} {1}", e.Column.Header, e.Direction);
    }
}
