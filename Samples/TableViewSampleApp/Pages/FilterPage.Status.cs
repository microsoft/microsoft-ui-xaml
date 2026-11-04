// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Globalization;
using System.Linq;
using TableViewSampleApp.Helpers;
using TableViewSampleApp.Models;

namespace TableViewSampleApp.Pages;

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
        var shown = CountShown(People, p => Matches(p, query, activeOnly));
        Status.Rows = !HasFilter
            ? SampleShaping.RowCountText(People.Count)
            : shown == matched
                ? string.Format(CultureInfo.CurrentCulture, "Showing {0:N0} of {1:N0}", matched, People.Count)
                : string.Format(CultureInfo.CurrentCulture, "Showing {0:N0} of {1:N0} ({2:N0} matches plus their managers)", shown, People.Count, matched);

        SelectedItemText.Text = FilterTable.SelectedItem is Person person ? person.FullName : "(none)";
    }

    // Appended to a Last action so the outcome of a filter change is announced, not only the call.
    private string ShownSummary()
    {
        var query = Query;
        var activeOnly = ActiveOnly;
        var shown = HasFilter ? CountShown(People, p => Matches(p, query, activeOnly)) : People.Count;
        return string.Format(CultureInfo.CurrentCulture, "showing {0:N0} of {1:N0} rows", shown, People.Count);
    }
}
