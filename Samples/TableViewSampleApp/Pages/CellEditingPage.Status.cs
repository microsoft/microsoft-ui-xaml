// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Globalization;
using TableViewSampleApp.Helpers;
using TableViewSampleApp.Models;

namespace TableViewSampleApp.Pages;

public sealed partial class CellEditingPage
{
    private int _editsCommitted;
    private int _editsAtBeginEdit;
    private string _openEdit = "(none)";
    private bool _isEditOpen;
    private string? _pendingCommit;
    private Person? _rejectedPerson;
    private string? _rejectedProperty;
    private string? _rejectedError;
    private bool _rollbackPending;

    protected override void RefreshReadouts()
    {
        Status.Rows = SampleShaping.RowCountText(People.Count);
        ReadOnlyText.Text = PeopleTable.IsReadOnly ? "Yes" : "No";
        OpenEditText.Text = _openEdit;
        EditsCommittedText.Text = _editsCommitted.ToString("N0", CultureInfo.CurrentCulture);
    }
}
