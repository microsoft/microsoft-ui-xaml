// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Globalization;
using System.Linq;
using TableViewSampleApp.Helpers;

namespace TableViewSampleApp.Pages;

// HeadersVisibilityPage readouts: Headers, Resolved and Rows (Sort is written by OnTableSorted).
// The Shaping and Last action rows are written by SamplePageBase; this file only computes the
// page's own values.
public sealed partial class HeadersVisibilityPage
{
    protected override void RefreshReadouts()
    {
        Status.Rows = SampleShaping.RowCountText(Tickets.Count);
        ResolvedCountText.Text = string.Format(CultureInfo.CurrentCulture, "{0:N0} of {1:N0}", Tickets.Count(t => t.IsResolved), Tickets.Count);
        HeadersText.Text = TicketsTable.HeadersVisibility.ToString();
    }
}
