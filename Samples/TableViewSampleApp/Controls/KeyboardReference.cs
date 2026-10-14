// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Automation;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Media;

namespace TableViewSampleApp.Controls;

// The keyboard reference shared by every page's Info tab.
public sealed partial class KeyboardReference : RailSection
{
    private static readonly (string Keys, string Action)[] CommonKeys =
    {
        ("Up / Down", "Previous / next row"),
        ("Left / Right", "Previous / next cell in the focused row"),
        ("Home / End", "First / last cell of the focused row"),
        ("Ctrl+Home / Ctrl+End", "First / last row. Known issue: Ctrl+End can stop near the bottom of the visible rows instead of on the last row."),
        ("Page Up / Page Down", "One viewport up / down"),
        ("Tab / Shift+Tab", "The column headers and the table body are one tab stop each: Tab goes to the headers, then the body, then out. Shift+Tab back into the body resumes the row or cell you left, or lands on the last visible row without scrolling"),
        ("Enter / Esc", "On a template cell: into its control / back to the cell"),
        ("Space", "Selects the focused row; toggles a check box once focus is inside it"),
        ("F2", "Edits the focused text cell where editing is allowed; Enter commits, Esc cancels"),
        ("Enter / Space", "On a focused column header: sorts by that column"),
        ("Right / Left", "On a group header: expands / collapses the group"),
        ("Space / Enter", "On a group header: toggles the group"),
    };

    private static readonly (string Keys, string Action)[] HierarchyKeys =
    {
        ("Right / Left", "On a parent row: expands / collapses it; Left on a collapsed row moves to its parent"),
        ("*", "Expands the whole subtree of the focused row"),
    };

    private readonly Grid _grid;

    public KeyboardReference()
    {
        Header = "Keyboard reference";
        Description = "Keys every TableView in this gallery supports. A key that does not apply to this table (editing, group headers) does nothing.";
        AutomationProperties.SetAutomationId(this, "KeyboardReference");

        _grid = new Grid();
        if (Application.Current.Resources.TryGetValue("StatusGridStyle", out var style))
        {
            _grid.Style = (Style)style;
        }

        _grid.ColumnDefinitions.Add(new ColumnDefinition { Width = GridLength.Auto });
        _grid.ColumnDefinitions.Add(new ColumnDefinition { Width = new GridLength(1, GridUnitType.Star) });
        Children.Add(_grid);
        BuildRows();
    }

    public bool IncludeHierarchy
    {
        get => (bool)GetValue(IncludeHierarchyProperty);
        set => SetValue(IncludeHierarchyProperty, value);
    }

    public static readonly DependencyProperty IncludeHierarchyProperty =
        DependencyProperty.Register(nameof(IncludeHierarchy), typeof(bool), typeof(KeyboardReference),
            new PropertyMetadata(false, (d, e) => ((KeyboardReference)d).BuildRows()));

    private void BuildRows()
    {
        _grid.Children.Clear();
        _grid.RowDefinitions.Clear();
        foreach (var (keys, action) in CommonKeys)
        {
            AddRow(keys, action);
        }

        if (IncludeHierarchy)
        {
            foreach (var (keys, action) in HierarchyKeys)
            {
                AddRow(keys, action);
            }
        }
    }

    private void AddRow(string keys, string action)
    {
        var row = _grid.RowDefinitions.Count;
        _grid.RowDefinitions.Add(new RowDefinition { Height = GridLength.Auto });

        var key = new TextBlock
        {
            Text = keys,
            FontFamily = new FontFamily("Consolas"),
            Style = (Style)Application.Current.Resources["StatusReadoutValueStyle"],
        };
        Grid.SetRow(key, row);

        var description = new TextBlock
        {
            Text = action,
            TextWrapping = TextWrapping.Wrap,
            Style = (Style)Application.Current.Resources["StatusReadoutLabelStyle"],
        };
        Grid.SetRow(description, row);
        Grid.SetColumn(description, 1);

        _grid.Children.Add(key);
        _grid.Children.Add(description);
    }
}
