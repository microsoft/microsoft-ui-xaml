// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Globalization;
using TableViewSampleApp.Helpers;

namespace TableViewSampleApp.Pages;

// CellEditingPage readouts: Read-only, Open edit, Edits committed and Rows. The Shaping and Last
// action rows are written by SamplePageBase; this file only computes the page's own values.
public sealed partial class CellEditingPage
{
    private int _editsCommitted;
    private string _openEdit = "(none)";

    protected override void RefreshReadouts()
    {
        Status.Rows = SampleShaping.RowCountText(People.Count);
        ReadOnlyText.Text = PeopleTable.IsReadOnly ? "Yes" : "No";
        OpenEditText.Text = _openEdit;
        EditsCommittedText.Text = _editsCommitted.ToString("N0", CultureInfo.CurrentCulture);
    }
}
