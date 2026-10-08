// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Globalization;
using System.Linq;
using TableViewSampleApp.Helpers;

namespace TableViewSampleApp.Pages;

// CellTemplatingPage readouts: Active and Rows. The Shaping and Last action rows are written by
// SamplePageBase; this file only computes the page's own values.
public sealed partial class CellTemplatingPage
{
    protected override void RefreshReadouts()
    {
        Status.Rows = SampleShaping.RowCountText(People.Count);
        ActiveCountText.Text = string.Format(CultureInfo.CurrentCulture, "{0:N0} of {1:N0}", People.Count(p => p.IsActive), People.Count);
    }
}
