// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using TableViewSampleApp.Helpers;

namespace TableViewSampleApp.Pages;

public sealed partial class GridLinesVisibilityPage
{
    protected override void RefreshReadouts()
    {
        Status.Rows = SampleShaping.RowCountText(People.Count);
        GridLinesText.Text = PeopleTable.GridLinesVisibility.ToString();
        RowBandingText.Text = SampleShaping.Label(RowBandingSelector);
    }
}
