// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Globalization;
using System.Linq;
using Microsoft.UI.Xaml.Controls;
using TableViewSampleApp.Helpers;
using TableViewSampleApp.Models;

namespace TableViewSampleApp.Pages;

public sealed partial class HierarchyPage
{
    private string FilterValue => FilterBox.Text?.Trim() ?? string.Empty;

    private string SortLabel => (SortCombo.SelectedItem as ComboBoxItem)?.Content?.ToString() ?? "Source order";

    private static bool Matches(Person person, string text) =>
        person.FullName.Contains(text, StringComparison.CurrentCultureIgnoreCase);

    protected override void RefreshReadouts()
    {
        var filter = FilterValue;
        Status.Rows = filter.Length == 0
            ? SampleShaping.RowCountText(People.Count)
            : string.Format(CultureInfo.CurrentCulture, "{0:N0} ({1:N0} match the filter)", People.Count, People.Count(p => Matches(p, filter)));

        SelectedText.Text = SelectedPerson is { } selected
            ? string.Format(CultureInfo.CurrentCulture, "{0} (#{1}, {2} #{3})", selected.FullName, selected.EmployeeId, Relation.Label.ToLower(CultureInfo.CurrentCulture),
                Relation.ParentKeyOf(selected)?.ToString() ?? "none")
            : "(none)";
        FilterText.Text = filter.Length == 0 ? "(none)" : string.Format(CultureInfo.CurrentCulture, "\u201C{0}\u201D", filter);
        SortText.Text = SortLabel;
        LiveText.Text = Source.IsLiveShaping ? "On" : "Off";
    }
}
