// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Globalization;
using System.Linq;
using Microsoft.UI.Xaml;
using TableViewSampleApp.Helpers;
using TableViewSampleApp.Models;

namespace TableViewSampleApp.Pages;

// ShowcasePage readouts: Selection, Sort, Columns and Rows. The Shaping and Last action rows are
// written by SamplePageBase; this file only computes the page's own values.
public sealed partial class ShowcasePage
{
    protected override void RefreshReadouts()
    {
        Status.Rows = SampleShaping.RowCountText(People.Count);
        SelectionText.Text = PeopleTable.SelectedItem is Person person
            ? string.Format(CultureInfo.CurrentCulture, "{0} ({1})", person.FullName, PeopleTable.SelectionMode)
            : string.Format(CultureInfo.CurrentCulture, "(none) ({0})", PeopleTable.SelectionMode);

        var sortColumn = SampleShaping.ActiveSortColumn(PeopleTable);
        SortText.Text = sortColumn is null
            ? "(none)"
            : string.Format(CultureInfo.CurrentCulture, "{0} ({1})", sortColumn.Header, sortColumn.SortDirection);

        var shown = PeopleTable.Columns.Count(column => column.Visibility == Visibility.Visible);
        ColumnsText.Text = string.Format(CultureInfo.CurrentCulture, "{0} of {1} shown", shown, PeopleTable.Columns.Count);
    }
}
