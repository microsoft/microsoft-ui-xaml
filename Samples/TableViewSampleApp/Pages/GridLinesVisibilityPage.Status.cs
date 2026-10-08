// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using TableViewSampleApp.Helpers;

namespace TableViewSampleApp.Pages;

// GridLinesVisibilityPage readouts: Grid lines, Row banding and Rows. The Shaping and Last action
// rows are written by SamplePageBase; this file only computes the page's own values.
public sealed partial class GridLinesVisibilityPage
{
    protected override void RefreshReadouts()
    {
        Status.Rows = SampleShaping.RowCountText(People.Count);
        GridLinesText.Text = PeopleTable.GridLinesVisibility.ToString();
        RowBandingText.Text = SampleShaping.Label(RowBandingSelector);
    }
}
