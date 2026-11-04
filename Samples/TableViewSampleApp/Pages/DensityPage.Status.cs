// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using TableViewSampleApp.Helpers;

namespace TableViewSampleApp.Pages;

public sealed partial class DensityPage
{
    protected override void RefreshReadouts()
    {
        Status.Rows = SampleShaping.RowCountText(People.Count);
        DensityText.Text = PeopleTable.Density.ToString();
    }
}
