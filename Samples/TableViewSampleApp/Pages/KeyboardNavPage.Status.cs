// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Globalization;
using TableViewSampleApp.Helpers;
using TableViewSampleApp.Models;

namespace TableViewSampleApp.Pages;

public sealed partial class KeyboardNavPage
{
    protected override void RefreshReadouts()
    {
        var index = PeopleTable.SelectedIndex;
        SelectedIndexText.Text = index >= 0
            ? index.ToString(CultureInfo.CurrentCulture)
            : string.Format(CultureInfo.CurrentCulture, "{0} (none)", -1);
        ColumnsText.Text = PeopleTable.Columns.Count.ToString(CultureInfo.CurrentCulture);
        Status.Rows = SampleShaping.RowCountText(People.Count);

        FixtureRecordsText.Text = string.Format(CultureInfo.CurrentCulture, "{0:N0} in the source, {1:N0} removed", _data.Rows.Count, _data.RemovedCount);
        FixtureSelectionText.Text = AssessmentTable.SelectedItem is AccessibilityRow selected
            ? string.Format(CultureInfo.CurrentCulture, "Record {0:00}, SelectedIndex {1}", selected.Id, AssessmentTable.SelectedIndex)
            : string.Format(CultureInfo.CurrentCulture, "(none), SelectedIndex {0}", AssessmentTable.SelectedIndex);
        FixtureEditingText.Text = string.Format(
            CultureInfo.CurrentCulture,
            "{0} (IsReadOnly {1}, IsEnabled {2})",
            AssessmentTable.IsEditing,
            AssessmentTable.IsReadOnly,
            AssessmentTable.IsEnabled);
    }
}
