// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls.Tabular;
using Microsoft.UI.Xaml.Media;
using TableViewSampleApp.Helpers;

namespace TableViewSampleApp.Pages;

// TextWrapPage readouts: Wrapping, Bio width, Max lines, Row heights and Rows. The Shaping and Last
// action rows are written by SamplePageBase; this file only computes the page's own values.
public sealed partial class TextWrapPage
{
    protected override void RefreshReadouts()
    {
        Status.Rows = SampleShaping.RowCountText(People.Count);
        WrappingText.Text = WrapState.TextWrapping.ToString();
        BioWidthText.Text = string.Format(CultureInfo.CurrentCulture, "{0:N0} px", BioColumn.Width.Value);
        MaxLinesText.Text = WrapState.TextWrapping != TextWrapping.Wrap
            ? "Not applied (NoWrap)"
            : _maxLines == 0 ? "Unlimited" : _maxLines.ToString(CultureInfo.CurrentCulture);
    }

    // Row heights: the distinct heights among the realized rows, measured after layout
    // (QueueRowMeasure), not on every RefreshReadouts.
    private void RefreshRowHeights()
    {
        var heights = new SortedSet<double>();
        CollectRowHeights(PeopleTable, heights);
        RowHeightsText.Text = heights.Count == 0
            ? "(no rows realized)"
            : string.Format(CultureInfo.CurrentCulture, "{0:N0} distinct: {1} px", heights.Count,
                string.Join(", ", heights.Select(h => h.ToString("N0", CultureInfo.CurrentCulture))));
    }

    private static void CollectRowHeights(DependencyObject node, SortedSet<double> heights)
    {
        if (node is TableViewRow row)
        {
            if (row.ActualHeight > 0 && row.Visibility == Visibility.Visible)
            {
                heights.Add(Math.Round(row.ActualHeight));
            }

            return;
        }

        var count = VisualTreeHelper.GetChildrenCount(node);
        for (var i = 0; i < count; i++)
        {
            CollectRowHeights(VisualTreeHelper.GetChild(node, i), heights);
        }
    }
}
