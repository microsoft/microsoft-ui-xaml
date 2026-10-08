// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Globalization;
using Microsoft.UI.Xaml;
using TableViewSampleApp.Helpers;

namespace TableViewSampleApp.Pages;

// ColumnLayoutPage readouts: Column, Width / ActualWidth, MinWidth / MaxWidth, Resize and Rows. The
// page has no Shaping section, so the Shaping row keeps StatusPanel's "Flat"; Last action is written
// by SamplePageBase. This file only computes the page's own values.
public sealed partial class ColumnLayoutPage
{
    protected override void RefreshReadouts()
    {
        Status.Rows = SampleShaping.RowCountText(People.Count);

        if (_activeColumn is { } column)
        {
            SelectedColumnText.Text = column.Header?.ToString() ?? "(unnamed)";
            var width = column.Width.GridUnitType switch
            {
                GridUnitType.Auto => "Auto",
                GridUnitType.Star => string.Format(CultureInfo.CurrentCulture, "{0:0.##}* (Star)", column.Width.Value),
                _ => string.Format(CultureInfo.CurrentCulture, "{0:N0} px", column.Width.Value),
            };
            WidthReadoutText.Text = string.Format(CultureInfo.CurrentCulture, "{0} / {1:N0}", width, column.ActualWidth);
            ClampText.Text = string.Format(CultureInfo.CurrentCulture, "{0:N0} / {1}",
                column.MinWidth, double.IsInfinity(column.MaxWidth) ? "none" : column.MaxWidth.ToString("N0", CultureInfo.CurrentCulture));
        }
        else
        {
            SelectedColumnText.Text = "(none)";
            WidthReadoutText.Text = "-";
            ClampText.Text = "-";
        }

        var tableGate = WidthsTable.CanUserResizeColumns ? "CanUserResizeColumns on" : "CanUserResizeColumns off (every gripper)";
        var columnGate = _activeColumn is null ? string.Empty : _activeColumn.CanResize ? "; CanResize on" : "; CanResize off";
        ResizeStateText.Text = tableGate + columnGate;
    }
}
