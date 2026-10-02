// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Private.Controls;
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

using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewTestHelpers;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    // Shared fixtures for the TableView API test suite. 
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

    // Which axis ScrollBodyTo drives; the body scroller can scroll on either.
    internal enum ScrollAxis
    {
        Horizontal,
        Vertical,
    }

    internal static class TableViewTestHelpers
    {
        // Resource init + the column-less TableView shell shared by every Create* variation. Each
        // builder decides how to attach its columns (bound/unbound, widths, template) afterward.
        internal static TableView CreateTableViewShell(
            object itemsSource,
            double width,
            double height,
            DataTemplate emptyTemplate = null)
        {
            EnsureTabularControlsResources();

            return new TableView
            {
                ItemsSource = itemsSource,
                Width = width,
                Height = height,
                EmptyTemplate = emptyTemplate,
            };
        }

        // The single builder behind every fixture TableView. 
        //      itemsSource defaults to MakeItems() for bound tables, null for unbound tables.
        //      headers default to a single "Name" column (pass an empty array for a column-less table).
        //      bound: true binds each text column to the property named by its header
        //                  (a header with no matching property renders empty cells); 
        //      bound: false leaves the columns purely structural.
        internal static TableView CreateTableView(
            object itemsSource = null,
            string[] headers = null,
            bool bound = true,
            DataTemplate emptyTemplate = null,
            double width = 500,
            double height = 300)
        {
            object source = itemsSource ?? (bound ? MakeItems() : null);
            var tableView = CreateTableViewShell(source, width, height, emptyTemplate);

            foreach (var header in headers ?? new[] { "Name" })
            {
                tableView.Columns.Add(MakeTextColumn(header, bound: bound));
            }

            return tableView;
        }

        // A text column bound one-way to bindingPath (defaulting to the header). Width is left at the
        // column default unless supplied. Pass bound: false for a header-only column whose cells stay
        // empty - the unbound structural column CreateTableView(bound: false) uses.
        internal static TableViewTextColumn MakeTextColumn(string header, string bindingPath = null, GridLength? width = null, bool bound = true)
        {
            var column = new TableViewTextColumn { Header = header };

            if (bound)
            {
                column.Binding = new Binding { Path = new PropertyPath(bindingPath ?? header), Mode = BindingMode.OneWay };
            }

            if (width.HasValue)
            {
                column.Width = width.Value;
            }

            return column;
        }

        // A template column whose cells inflate cellTemplate. Width is left at the column default
        // unless supplied.
        internal static TableViewTemplateColumn MakeTemplateColumn(string header, DataTemplate cellTemplate, GridLength? width = null)
        {
            var column = new TableViewTemplateColumn { Header = header, CellTemplate = cellTemplate };

            if (width.HasValue)
            {
                column.Width = width.Value;
            }

            return column;
        }

        internal static List<Person> MakeItems() => new List<Person>
        {
            new Person { Name = "Asha", Role = "Designer" },
            new Person { Name = "Diego", Role = "Engineer" },
            new Person { Name = "Mei", Role = "Architect" },
        };

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

        internal static Panel GetCellsHost(TableViewRow row)
        {
            var host = row.FindVisualChildByName("PART_CellsHost") as Panel;
            Verify.IsNotNull(host, "PART_CellsHost should exist on a realized row.");
            return host;
        }

        internal static Border GetRowCell(TableViewRow row, int index)
        {
            var host = GetCellsHost(row);
            Verify.IsGreaterThan(host.Children.Count, index, "The cells host should have a cell at the requested index.");

            var wrapper = host.Children[index] as Border;
            Verify.IsNotNull(wrapper, "Every cell is hosted in a Border wrapper.");
            return wrapper;
        }

        internal static void ScrollBodyTo(TableView tableView, ScrollAxis axis, double offset)
        {
            RunOnUIThread.Execute(() =>
            {
                var scroller = GetBodyScroller(tableView);
                var scrollableExtent = axis == ScrollAxis.Horizontal ? scroller.ScrollableWidth : scroller.ScrollableHeight;
                Verify.IsGreaterThan(scrollableExtent, offset,
                    "Precondition: the source must be scrollable on the requested axis by more than the offset under test.");

                // disableAnimation so the offset lands synchronously rather than over a composition
                // animation the test would have to poll for.
                scroller.ChangeView(
                    axis == ScrollAxis.Horizontal ? offset : (double?)null,
                    axis == ScrollAxis.Vertical ? offset : (double?)null,
                    null,
                    true);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() => tableView.UpdateLayout());
            IdleSynchronizer.Wait();
        }

        internal static ScrollViewer GetBodyScroller(TableView tableView)
        {
            var scroller = tableView.FindVisualChildByName("PART_BodyScroller") as ScrollViewer;
            Verify.IsNotNull(scroller, "PART_BodyScroller should exist once the template has applied.");
            return scroller;
        }

        internal static FrameworkElement GetHeaderCell(TableView tableView, int index)
        {
            var host = tableView.FindVisualChildByName("PART_HeaderHost") as Panel;
            Verify.IsNotNull(host, "PART_HeaderHost should exist once the template has applied.");
            Verify.IsGreaterThan(host.Children.Count, index, "The header host should have a cell at the requested index.");
            return (FrameworkElement)host.Children[index];
        }

        internal static ResizeGripper FindGripper(DependencyObject headerCell)
            => FindVisualChildrenByType<ResizeGripper>(headerCell).FirstOrDefault();

        internal static ResizeGripper RequireGripper(TableView tableView, int columnIndex)
        {
            var gripper = FindGripper(GetHeaderCell(tableView, columnIndex));
            Verify.IsNotNull(gripper, $"Column {columnIndex} should have a resize gripper in its header cell.");
            return gripper;
        }

        // The general fixture builder: a TableView over items (default sample rows) sized width x
        // height, with text columns each bound to "Name" at the given widths. Defaults to a single
        // 200px "Name" column when none are supplied.
        internal static TableView CreateTableViewWithColumns(
            List<Person> items = null,
            double width = 500,
            double height = 260,
            params (string Header, GridLength Width)[] columns)
        {
            var tableView = CreateTableViewShell(items ?? MakeItems(), width, height);

            if (columns.Length == 0)
            {
                columns = new[] { ("Name", new GridLength(200.0, GridUnitType.Pixel)) };
            }

            foreach (var (header, columnWidth) in columns)
            {
                tableView.Columns.Add(MakeTextColumn(header, "Name", columnWidth));
            }

            return tableView;
        }
    }
    
    // The default row item: two plain, non-notifying string properties. Tests that need change
    // notification, validation, or grouping keys use the richer items defined by their own area.
    internal sealed class Person
    {
        public string Name { get; set; }

        public string Role { get; set; }
    }

    internal static class TableViewRowTestHelpers
    {
        internal const string VeryLongText = "A considerably longer piece of cell text than the column can possibly show";

        internal static TableView CreateTemplateColumnTable(List<Person> items)
        {
            var tableView = CreateTableViewShell(items, 500, 260);

            tableView.Columns.Add(MakeTemplateColumn("Name", CreateBoundTextTemplate(), new GridLength(200.0, GridUnitType.Pixel)));

            return tableView;
        }

        // One text column and one template column over the same property, so a recycle test can check
        // both content routes on the same row.
        internal static TableView CreateMixedColumnTable(List<Person> items)
        {
            var tableView = CreateTableViewWithColumns(items, columns: new[] { ("Text", new GridLength(180.0, GridUnitType.Pixel)) });

            tableView.Columns.Add(MakeTemplateColumn("Template", CreateBoundTextTemplate(), new GridLength(180.0, GridUnitType.Pixel)));

            return tableView;
        }

        internal static DataTemplate CreateBoundTextTemplate() => (DataTemplate)XamlReader.Load(
            @"<DataTemplate xmlns=""http://schemas.microsoft.com/winfx/2006/xaml/presentation"">
                  <TextBlock Text=""{Binding Name}"" />
              </DataTemplate>");

        internal static List<Person> MakeManyItems(int count) => Enumerable
            .Range(0, count)
            .Select(i => new Person { Name = $"Person {i}", Role = $"Role {i}" })
            .ToList();

        internal static TableViewRow RequireFirstRow(TableView tableView)
        {
            var rows = GetRealizedRows(tableView);
            Verify.IsGreaterThan(rows.Count, 0, "At least one row should be realized.");
            return rows[0];
        }

        internal static void VerifyNoLocalDataContext(FrameworkElement element, string what)
        {
            Verify.AreEqual(DependencyProperty.UnsetValue, element.ReadLocalValue(FrameworkElement.DataContextProperty),
                $"{what} must not set a local DataContext, or it shadows inheritance and cells go stale after recycle.");
        }

        // The CommonStates state a row is currently in. Read by name rather than by brush because
        // several states share a brush, which would make a wrong state look correct.
        internal static string GetCommonState(TableViewRow row)
        {
            var rootBorder = row.FindVisualChildByName("PART_RootBorder") as FrameworkElement;
            Verify.IsNotNull(rootBorder, "PART_RootBorder should exist once the row template has applied.");

            var groups = VisualStateManager.GetVisualStateGroups(rootBorder);
            var common = groups.FirstOrDefault(group => group.Name == "CommonStates");
            Verify.IsNotNull(common, "The row template should declare a CommonStates group on its root.");

            return common.CurrentState?.Name;
        }

        internal static void VerifyBanding(
            TableView tableView,
            List<Person> items,
            Brush baseBrush,
            Brush alternateBrush,
            string context)
        {
            var rows = GetRealizedRows(tableView);
            Verify.IsGreaterThan(rows.Count, 0, $"Rows should be realized ({context}).");

            foreach (var row in rows)
            {
                var index = items.IndexOf(row.DataContext as Person);
                Verify.IsGreaterThanOrEqual(index, 0, $"Every realized row should map to a source item ({context}).");

                var expected = (index % 2) == 0 ? baseBrush : alternateBrush;
                Verify.AreEqual(expected, row.Background,
                    $"Row at index {index} should carry the brush for its parity ({context}).");
            }
        }
    }
}
