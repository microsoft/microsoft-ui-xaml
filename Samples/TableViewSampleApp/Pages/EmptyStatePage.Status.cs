// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Globalization;
using TableViewSampleApp.Helpers;

namespace TableViewSampleApp.Pages;

// EmptyStatePage readouts: ItemsSource, Empty template and Rows. The Shaping and Last action rows
// are written by SamplePageBase; this file only computes the page's own values.
public sealed partial class EmptyStatePage
{
    protected override void RefreshReadouts()
    {
        var shown = !_sourceAttached ? 0 : _filteredToZero ? 0 : People.Count;
        ItemsSourceText.Text = !_sourceAttached ? "null" : _filteredToZero ? "TableViewSource (filtered)" : "TableViewSource";
        EmptyTemplateText.Text = shown == 0 ? "Shown" : "Hidden";
        Status.Rows = shown == People.Count
            ? SampleShaping.RowCountText(People.Count)
            : string.Format(CultureInfo.CurrentCulture, "{0:N0} shown of {1:N0}", shown, People.Count);
    }
}
