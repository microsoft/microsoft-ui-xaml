// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using TableViewSampleApp.Helpers;
using TableViewSampleApp.Models;

namespace TableViewSampleApp.Pages;

public sealed partial class SelectionPage
{
    private int _changeCount;
    private string _lastDelta = "(no SelectionChanged yet)";

    protected override void RefreshReadouts()
    {
        var item = PeopleTable.SelectedItem;
        var index = PeopleTable.SelectedIndex;
        SelectedItemText.Text = Describe(item);
        SelectedIndexText.Text = index >= 0
            ? index.ToString(CultureInfo.CurrentCulture)
            : item is null
                ? string.Format(CultureInfo.CurrentCulture, "{0} (none)", -1)
                : string.Format(CultureInfo.CurrentCulture, "{0} (the row is in a collapsed group)", -1);
        ChangeCountText.Text = _changeCount.ToString("N0", CultureInfo.CurrentCulture);
        SelectionDeltaText.Text = _lastDelta;
        Status.Rows = SampleShaping.RowCountText(People.Count);

        var single = IsSingleMode;
        SelectFirstButton.IsEnabled = single;
        SelectLastButton.IsEnabled = single;
    }

    private static string DescribeAll(IList<object>? items) =>
        items is null || items.Count == 0 ? "none" : string.Join(", ", items.Select(Describe));

    private static string Describe(object? item) => item is Person person ? person.FullName : "(none)";
}
