// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Tabular;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Markup;
using Microsoft.UI.Xaml.Media;
using MUXControlsTestApp.Utilities;
using System.Collections.Generic;
using System.Linq;

using WEX.TestExecution;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    // Shared fixtures for the TableView API test suite. Types here are used by more than one test
    // file, so they live outside any single area's file to keep those files independently reviewable.
    // A custom column written the way TableView.idl:198-201 requires: it binds reactively against
    // the inherited DataContext and never assigns a local DataContext or bakes dataItem in as
    // static content. Mirrors Samples\TableViewSampleApp\ScoreBarColumn.cs.
    internal partial class ProbeColumn : TableViewColumn
    {
        internal const string CellName = "ProbeCell";

        public int GenerateCount { get; private set; }

        public string ValuePath { get; set; } = "Name";

        protected override FrameworkElement GenerateElementCore(object dataItem)
        {
            GenerateCount++;

            var text = new TextBlock { Name = CellName };
            text.SetBinding(TextBlock.TextProperty, new Binding
            {
                Path = new PropertyPath(ValuePath),
                Mode = BindingMode.OneWay,
            });

            return text;
        }

        protected override string GetSortMemberPathCore() => ValuePath;
    }

    internal static class TableViewColumnTestHelpers
    {
        // Builds an unloaded TableView with the given text column headers.
        internal static TableView CreateTableView(params string[] headers)
        {
            EnsureTabularControlsResources();

            var tableView = new TableView
            {
                ItemsSource = MakeItems(),
                Width = 500,
                Height = 300,
            };

            foreach (var header in headers)
            {
                tableView.Columns.Add(new TableViewTextColumn
                {
                    Header = header,
                    Binding = new Binding { Path = new PropertyPath("Name"), Mode = BindingMode.OneWay },
                });
            }

            return tableView;
        }

        internal static List<Person> MakeItems() => new List<Person>
        {
            new Person { Name = "Asha", Role = "Designer" },
            new Person { Name = "Diego", Role = "Engineer" },
            new Person { Name = "Mei", Role = "Architect" },
        };

        // A minimal two-column, two-row TableView with unbound columns. Distinct from
        // CreateTableView: tests that assert on row counts or selection indices rely on the
        // smaller item set, and the columns are deliberately left unbound.
        internal static TableView CreateBasicTableView() =>
            CreateTableViewWithItems(
                new List<Person>
                {
                    new Person { Name = "Asha", Role = "Designer" },
                    new Person { Name = "Diego", Role = "Engineer" },
                },
                headers: new[] { "Name", "Role" });

        // Builds a sized, hosted-ready TableView over a caller-supplied items source.
        // Headers default to a single unbound "Name" column. Deliberately not an overload of
        // CreateTableView(params string[]): a first parameter of type object would win overload
        // resolution against the params form and silently capture CreateTableView("Name") calls.
        internal static TableView CreateTableViewWithItems(
            object itemsSource,
            DataTemplate emptyTemplate = null,
            double width = 400,
            double height = 300,
            string[] headers = null)
        {
            EnsureTabularControlsResources();

            var tableView = new TableView
            {
                ItemsSource = itemsSource,
                Width = width,
                Height = height,
                EmptyTemplate = emptyTemplate,
            };

            foreach (var header in headers ?? new[] { "Name" })
            {
                tableView.Columns.Add(new TableViewTextColumn { Header = header });
            }

            return tableView;
        }

        internal static void EnsureTabularControlsResources()
        {
            if (!Application.Current.Resources.MergedDictionaries.OfType<TabularControlsResources>().Any())
            {
                Application.Current.Resources.MergedDictionaries.Add(new TabularControlsResources());
            }
        }

        internal static DataTemplate CreateTextTemplate(string text) => (DataTemplate)XamlReader.Load(
            $@"<DataTemplate xmlns=""http://schemas.microsoft.com/winfx/2006/xaml/presentation"">
                   <TextBlock Text=""{text}"" />
               </DataTemplate>");

        // The columns backing the header cells in PART_HeaderHost, in rendered order.
        internal static List<TableViewColumn> GetHeaderColumns(TableView tableView)
        {
            var host = tableView.FindVisualChildByName("PART_HeaderHost") as Panel;
            Verify.IsNotNull(host, "PART_HeaderHost should exist once the template has applied.");

            return host.Children
                .OfType<FrameworkElement>()
                .Select(child => child.Tag as TableViewColumn)
                .Where(column => column != null)
                .ToList();
        }

        // The ContentPresenter RebuildHeaders puts Header / HeaderTemplate / HeaderTemplateSelector on.
        internal static ContentPresenter GetHeaderPresenter(TableView tableView, int index)
        {
            var host = tableView.FindVisualChildByName("PART_HeaderHost") as Panel;
            Verify.IsNotNull(host, "PART_HeaderHost should exist once the template has applied.");
            Verify.IsGreaterThan(host.Children.Count, index, "The header host should have a cell at the requested index.");

            var presenter = ((DependencyObject)host.Children[index]).FindVisualChildByType<ContentPresenter>();
            Verify.IsNotNull(presenter, $"Header cell {index} should host a ContentPresenter.");
            return presenter;
        }

        // The columns backing the cell wrappers in a row's PART_CellsHost, in rendered order.
        internal static List<TableViewColumn> GetRowCellColumns(TableViewRow row)
        {
            var host = row.FindVisualChildByName("PART_CellsHost") as Panel;
            Verify.IsNotNull(host, "PART_CellsHost should exist on a realized row.");

            return host.Children
                .OfType<FrameworkElement>()
                .Select(child => child.Tag as TableViewColumn)
                .Where(column => column != null)
                .ToList();
        }

        internal static List<TableViewRow> GetRealizedRows(TableView tableView)
            => FindVisualChildrenByType<TableViewRow>(tableView);

        internal static void VerifyHeaderColumns(TableView tableView, IList<TableViewColumn> expected, string context)
        {
            var actual = GetHeaderColumns(tableView);
            Verify.AreEqual(expected.Count, actual.Count, $"Header cell count should match the column count ({context}).");

            for (int i = 0; i < expected.Count; i++)
            {
                Verify.AreEqual(expected[i], actual[i], $"Header at index {i} should belong to the column at that index ({context}).");
            }
        }

        internal static void VerifyEveryRowMatchesHeaders(TableView tableView, string context)
        {
            var headerColumns = GetHeaderColumns(tableView);
            var rows = GetRealizedRows(tableView);
            Verify.IsGreaterThan(rows.Count, 0, $"At least one row should be realized ({context}).");

            foreach (var row in rows)
            {
                var cellColumns = GetRowCellColumns(row);
                Verify.AreEqual(headerColumns.Count, cellColumns.Count,
                    $"Row cell count should match header cell count ({context}).");

                for (int i = 0; i < headerColumns.Count; i++)
                {
                    Verify.AreEqual(headerColumns[i], cellColumns[i],
                        $"Row cell at index {i} should belong to the same column as the header at that index ({context}).");
                }
            }
        }

        internal static List<T> FindVisualChildrenByType<T>(DependencyObject root) where T : DependencyObject
        {
            var results = new List<T>();
            Collect(root);
            return results;

            void Collect(DependencyObject element)
            {
                int count = VisualTreeHelper.GetChildrenCount(element);
                for (int i = 0; i < count; i++)
                {
                    var child = VisualTreeHelper.GetChild(element, i);
                    if (child is T match)
                    {
                        results.Add(match);
                    }

                    Collect(child);
                }
            }
        }
    }
    // The default row item: two plain, non-notifying string properties. Tests that need change
    // notification, validation, or grouping keys use the richer items defined by their own area.
    internal sealed class Person
    {
        public string Name { get; set; }

        public string Role { get; set; }
    }
}
