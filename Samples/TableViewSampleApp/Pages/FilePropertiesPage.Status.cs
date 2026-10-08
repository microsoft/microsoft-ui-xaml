// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Globalization;
using System.Linq;
using TableViewSampleApp.Helpers;

namespace TableViewSampleApp.Pages;

// FilePropertiesPage readouts: File, Sections and Rows. The Shaping and Last action rows are
// written by SamplePageBase; this file only computes the page's own values.
public sealed partial class FilePropertiesPage
{
    protected override void RefreshReadouts()
    {
        var visible = VisibleEntries().ToList();
        FileText.Text = _currentFile?.Name ?? "(none)";
        SectionsText.Text = visible.Count == 0
            ? "(none)"
            : string.Join(", ", s_sections
                .Select(section => (section, count: visible.Count(entry => entry.Section == section)))
                .Where(pair => pair.count > 0)
                .Select(pair => string.Format(CultureInfo.CurrentCulture, "{0} {1:N0}", pair.section, pair.count)));
        Status.Rows = SampleShaping.RowCountText(visible.Count);
    }
}
