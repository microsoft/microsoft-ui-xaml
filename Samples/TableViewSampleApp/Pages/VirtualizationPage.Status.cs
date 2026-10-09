// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Globalization;
using TableViewSampleApp.Helpers;

namespace TableViewSampleApp.Pages;

public sealed partial class VirtualizationPage
{
    private int _peakPool;
    private double _rowHeight;                 // last measured; rows are uniform in this table

    protected override void RefreshReadouts()
    {
        var (visible, pool, rowHeight) = IsLoaded ? CountRows() : (0, 0, 0d);
        _rowHeight = rowHeight > 0 ? rowHeight : _rowHeight;
        _peakPool = Math.Max(_peakPool, pool);
        var total = _people.Count;

        Status.Rows = SampleShaping.RowCountText(total);
        RealizedRowsText.Text = visible.ToString("N0", CultureInfo.CurrentCulture);
        RowPoolText.Text = pool.ToString("N0", CultureInfo.CurrentCulture);
        PeakPoolText.Text = _peakPool.ToString("N0", CultureInfo.CurrentCulture);
        RealizedShareText.Text = total > 0
            ? string.Format(CultureInfo.CurrentCulture, "{0:P2} of {1:N0}", (double)visible / total, total)
            : "—";
        RowHeightText.Text = rowHeight > 0 ? string.Format(CultureInfo.CurrentCulture, "{0:F0} px", rowHeight) : "—";
        ColumnsText.Text = PeopleTable.Columns.Count.ToString("N0", CultureInfo.CurrentCulture);
    }
}
