// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.Generic;
using System.Globalization;
using TableViewSampleApp.Helpers;

namespace TableViewSampleApp.Pages;

// ToolTipsPage readouts: Cell tooltips, Per column and Rows. The Shaping and Last action rows are
// written by SamplePageBase; this file only computes the page's own values.
public sealed partial class ToolTipsPage
{
    protected override void RefreshReadouts()
    {
        Status.Rows = SampleShaping.RowCountText(People.Count);
        CellToolTipsText.Text = NameColumn.CellToolTipBinding is null ? "Detached" : "Attached";

        // Read back from the columns, so the readout reports what the control actually holds.
        var parts = new List<string>();
        foreach (var column in PeopleTable.Columns)
        {
            var what = new List<string>();
            if (column.HeaderToolTip is not null)
            {
                what.Add(column.HeaderToolTip is string ? "header" : "header (card)");
            }

            if (column.CellToolTipBinding is not null)
            {
                what.Add("cell");
            }

            if (column == EmailColumn)
            {
                what.Add("template-owned");
            }

            parts.Add(string.Format(CultureInfo.CurrentCulture, "{0}: {1}", column.Header, what.Count == 0 ? "none" : string.Join(", ", what)));
        }

        PerColumnText.Text = string.Join("; ", parts);
    }
}
