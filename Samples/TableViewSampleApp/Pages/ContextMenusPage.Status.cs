// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.Generic;
using Microsoft.UI.Xaml;
using TableViewSampleApp.Helpers;

namespace TableViewSampleApp.Pages;

public sealed partial class ContextMenusPage
{
    protected override void RefreshReadouts()
    {
        Status.Rows = SampleShaping.RowCountText(People.Count);

        var hidden = new List<string>();
        foreach (var column in PeopleTable.Columns)
        {
            if (column.Visibility != Visibility.Visible)
            {
                hidden.Add(column.Header?.ToString() ?? string.Empty);
            }
        }

        HiddenColumnsText.Text = hidden.Count == 0 ? "(none)" : string.Join(", ", hidden);
    }
}
