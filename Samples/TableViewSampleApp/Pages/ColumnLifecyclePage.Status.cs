// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Globalization;
using System.Linq;
using Microsoft.UI.Xaml;
using TableViewSampleApp.Helpers;

namespace TableViewSampleApp.Pages;

// ColumnLifecyclePage readouts: Columns, Hidden, Groups and Rows. The Shaping and Last action rows
// are written by SamplePageBase; this file only computes the page's own values.
public sealed partial class ColumnLifecyclePage
{
    protected override void RefreshReadouts()
    {
        Status.Rows = SampleShaping.RowCountText(People.Count);
        ColumnOrderText.Text = string.Join(", ", PeopleTable.Columns.Select(c => c.Header?.ToString()));
        var hidden = PeopleTable.Columns.Where(c => c.Visibility != Visibility.Visible).Select(c => c.Header?.ToString()).ToList();
        HiddenColumnsText.Text = hidden.Count == 0 ? "(none)" : string.Join(", ", hidden);

        var key = AppliedGroupKey;
        GroupCountText.Text = IsGrouped
            ? People.Select(p => SampleShaping.KeyOf(p, key)).Distinct().Count().ToString("N0", CultureInfo.CurrentCulture)
            : "(flat)";
    }
}
