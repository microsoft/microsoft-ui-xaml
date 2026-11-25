// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Linq;
using TableViewSampleApp.Helpers;

namespace TableViewSampleApp.Pages;

public sealed partial class ColumnReorderPage
{
    protected override void RefreshReadouts()
    {
        Status.Rows = SampleShaping.RowCountText(People.Count);
        OrderText.Text = string.Join(", ", PeopleTable.Columns.Select(column => column.Header as string));
    }
}
