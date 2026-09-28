# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License. See LICENSE in the project root for license information.
#requires -Version 7.0

$ErrorActionPreference = 'Stop'
$source = Get-Content (Join-Path (Split-Path $PSScriptRoot -Parent) 'Pages\VirtualizationPage.xaml.cs') -Raw
# Compile the actual page handlers without starting WinUI. These doubles provide
# only their dependencies; the state transitions under test are not reimplemented.
$doubles = @'
#nullable enable
using System;
using System.Collections.Generic;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
namespace Microsoft.UI.Xaml
{
    public class RoutedEventArgs : EventArgs { }
    public enum GridUnitType { Auto }
    public struct GridLength { public GridLength(int value, GridUnitType unit) { } }
}
namespace Microsoft.UI.Xaml.Controls
{
    public class Page
    {
        public event Action<object, RoutedEventArgs>? Loaded;
        public event Action<object, RoutedEventArgs>? Unloaded;
        public void Load() => Loaded?.Invoke(this, new());
        public void Unload() => Unloaded?.Invoke(this, new());
    }
    public class SelectionChangedEventArgs : EventArgs { }
    public class ComboBoxItem { public object? Tag { get; set; } }
    public class TestControl
    {
        public object? SelectedItem { get; set; }
        public bool IsEnabled { get; set; }
        public string Text { get; set; } = "";
    }
}
namespace Microsoft.UI.Xaml.Controls.Tabular
{
    public class Column { public GridLength Width { get; set; } }
    public class TableView
    {
        public event Action<TableView, SelectionChangedEventArgs>? SelectionChanged;
        public object? ItemsSource { get; set; }
        public object? SelectedItem { get; set; }
        public List<Column> Columns { get; } = new();
        public void Select(object? item) { SelectedItem = item; SelectionChanged?.Invoke(this, new()); }
    }
}
namespace TableViewSampleApp.Data
{
    public static class PersonData
    {
        public static List<TableViewSampleApp.Pages.NumberedPerson> All { get; } =
            new() { new() { FirstName = "Test", LastName = "Person", Role = "Before" } };
    }
}
namespace TableViewSampleApp.Pages
{
    public sealed partial class VirtualizationPage
    {
        public Microsoft.UI.Xaml.Controls.Tabular.TableView PeopleTable = new();
        public TestControl SizeSelector = new(), MutationStatusText = new(), SourceModeText = new(),
            TotalRowsText = new(), SelectedRowsText = new(), ColumnCountText = new(),
            UpdateRowButton = new(), RemoveRowButton = new();
        private void InitializeComponent() { }
        private static void Check(bool condition, string message)
        { if (!condition) throw new Exception(message); }
        public static void RunRegression()
        {
            var page = new VirtualizationPage();
            page.SizeSelector.SelectedItem = new ComboBoxItem { Tag = "100" };
            page.Load();
            Check(page.People.Count == 100, "First load must honor selected size.");
            for (int i = 0; i < 100; i++)
            {
                page.PeopleTable.Select(page.People[0]);
                page.OnRemoveRowClick(page, new());
            }
            Check(page.People.Count == 0, "Remove all must empty the dataset.");
            page.Unload();
            page.Load();
            page.Load();
            Check(page.People.Count == 0, "Reload must preserve an empty dataset.");
            Check(page.SizeSelector.SelectedItem is ComboBoxItem item && (string?)item.Tag == "100",
                "Reload must preserve the size selection.");
            page.OnInsertRowClick(page, new());
            Check(page.People.Count == 1 && page.People[0].Id == 101, "Reload must preserve next insert ID.");
            var survivor = page.People[0];
            page.Unload();
            page.Load();
            Check(ReferenceEquals(page.People[0], survivor), "Reload must preserve surviving row identity.");
            page.OnInsertRowClick(page, new());
            Check(page.People[0].Id == 102, "Insert IDs must remain monotonic.");
            page.OnResetRowsClick(page, new());
            Check(page.People.Count == 100 && page.People[0].Id == 1 && page.People[99].Id == 100,
                "Explicit reset must repopulate the selected size.");
            page.OnInsertRowClick(page, new());
            Check(page.People[0].Id == 101, "Explicit reset must restart the insert sequence.");

            var early = new VirtualizationPage();
            early.SizeSelector.SelectedItem = new ComboBoxItem { Tag = "100" };
            early.OnSizeChanged(early, new());
            var first = early.People[0];
            early.Load();
            Check(ReferenceEquals(first, early.People[0]), "Selection initialization must not run twice.");
            var fallback = new VirtualizationPage();
            fallback.Load();
            Check(fallback.People.Count == 10000, "First load without selection must use the default.");
        }
    }
}
'@
$source = $source.Replace('namespace TableViewSampleApp.Pages;', 'namespace TableViewSampleApp.Pages {')
$doubles = $doubles -replace '(?m)^using .*;\r?\n', ''
Add-Type -TypeDefinition ("#nullable enable`nusing System;`nusing System.Collections.Generic;`n" + $source + "`n}`n" + $doubles)
[TableViewSampleApp.Pages.VirtualizationPage]::RunRegression()
Write-Output 'PASS: actual page handler initialization/remove-all/revisit/insert/reset regression. Test controls only; real UI runtime remains required.'
