// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Globalization;
using System.Linq;
using TableViewSampleApp.Helpers;
using TableViewSampleApp.Models;

namespace TableViewSampleApp.Pages;

// FilterPage readouts: Filter, Selected and Rows. The Shaping and Last action rows are written by
// SamplePageBase; this file only computes the page's own values.
public sealed partial class FilterPage
{
    protected override void RefreshReadouts()
    {
        var query = Query;
        var activeOnly = ActiveOnly;
        FilterStateText.Text = !HasFilter
            ? "(none)"
            : query.Length == 0
                ? "Active only"
                : string.Format(CultureInfo.CurrentCulture, "Query \u201C{0}\u201D{1}", query, activeOnly ? " + Active only" : string.Empty);

        // The projection's row count is not exposed, so count the matches app-side.
        var matched = People.Count(p => Matches(p, query, activeOnly));
        Status.Rows = HasFilter
            ? string.Format(CultureInfo.CurrentCulture, "Showing {0:N0} of {1:N0}", matched, People.Count)
            : SampleShaping.RowCountText(People.Count);

        SelectedItemText.Text = FilterTable.SelectedItem is Person person ? person.FullName : "(none)";
    }
}
