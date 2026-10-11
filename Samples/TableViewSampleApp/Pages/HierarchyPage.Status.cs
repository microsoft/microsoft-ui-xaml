// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using Microsoft.UI.Xaml.Controls;
using TableViewSampleApp.Data;
using TableViewSampleApp.Helpers;

namespace TableViewSampleApp.Pages;

public sealed partial class HierarchyPage
{
    private string FilterText => FilterBox.Text?.Trim() ?? string.Empty;

    private string SortLabel => (SortCombo.SelectedItem as ComboBoxItem)?.Content?.ToString() ?? "Authored order";

    private static bool Matches(Employee employee, string text) =>
        employee.Name.Contains(text, StringComparison.OrdinalIgnoreCase);

    protected override void RefreshReadouts()
    {
        var filter = FilterText;
        Status.Rows = filter.Length == 0
            ? SampleShaping.RowCountText(_employees.Count)
            : string.Format(CultureInfo.CurrentCulture, "{0:N0} ({1:N0} match the filter)", _employees.Count, _employees.Count(e => Matches(e, filter)));

        var parts = new List<string> { "Hierarchy by Manager and by Mentor" };
        if (GroupToggle.IsOn)
        {
            parts.Add("roots grouped by Dept");
        }

        if (SortCombo.SelectedIndex > 0)
        {
            parts.Add("sorted " + SortLabel);
        }

        if (filter.Length > 0)
        {
            parts.Add(string.Format(CultureInfo.CurrentCulture, "filtered \u201C{0}\u201D", filter));
        }

        Status.ShapingText = string.Join(", ", parts);

        SelectedText.Text = SelectedEmployee() is { } selected
            ? string.Format(CultureInfo.CurrentCulture, "{0} (Id {1}, {2} table)", selected.Name, selected.Id, ReferenceEquals(_activeTable, MentorTable) ? "Mentor" : "Manager")
            : "(none)";
        LiveText.Text = LiveToggle.IsOn ? "On" : "Off";
    }
}
