// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using TableViewSampleApp.Helpers;

namespace TableViewSampleApp.Pages;

// DensityPage readouts: Density and Rows. The Shaping and Last action rows are written by
// SamplePageBase; this file only computes the page's own values.
public sealed partial class DensityPage
{
    protected override void RefreshReadouts()
    {
        Status.Rows = SampleShaping.RowCountText(People.Count);
        DensityText.Text = PeopleTable.Density.ToString();
    }
}
