// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Common;
using MUXControlsTestApp;
using MUXControlsTestApp.Utilities;
using Microsoft.UI;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Controls.Tabular;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Markup;
using Microsoft.UI.Xaml.Media;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using Windows.UI;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    [TestClass]
    public class TableViewTests : ApiTestBase
    {
        [TestMethod]
        public void VerifyDefaultPropertyValues()
        {
            RunOnUIThread.Execute(() =>
            {
                TableView tableView = new TableView();
                Verify.IsNotNull(tableView);
                Verify.AreEqual(TableViewGridLinesVisibility.All, tableView.GridLinesVisibility);
                Verify.AreEqual(TableViewHeadersVisibility.Column, tableView.HeadersVisibility);
                Verify.AreEqual(TableViewDensity.Standard, tableView.Density);
                Verify.IsNull(tableView.RowBackground);
                Verify.IsNull(tableView.AlternatingRowBackground);
                Verify.IsTrue(tableView.IsReadOnly);
            });
        }

        [TestMethod]
        public void VerifyAlternatingRowBackgroundBandsOddRows()
        {
            SolidColorBrush rowBackground = null;
            SolidColorBrush alternatingRowBackground = null;
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                rowBackground = new SolidColorBrush(Colors.Red);
                alternatingRowBackground = new SolidColorBrush(Colors.Blue);
                tableView = CreateTableView();
                tableView.RowBackground = rowBackground;
                tableView.AlternatingRowBackground = alternatingRowBackground;
                LoadTableView(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreSame(rowBackground, GetRealizedRow(tableView, 0).Background);
                Verify.AreSame(alternatingRowBackground, GetRealizedRow(tableView, 1).Background);
                Verify.AreSame(rowBackground, GetRealizedRow(tableView, 2).Background);
                Verify.AreSame(alternatingRowBackground, GetRealizedRow(tableView, 3).Background);
            });
        }

        [TestMethod]
        public void VerifyRowBackgroundAloneFillsAllRowsUniformly()
        {
            SolidColorBrush rowBackground = null;
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                rowBackground = new SolidColorBrush(Colors.Green);
                tableView = CreateTableView();
                tableView.RowBackground = rowBackground;
                LoadTableView(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                for (int i = 0; i < 4; i++)
                {
                    Verify.AreSame(rowBackground, GetRealizedRow(tableView, i).Background, $"Row {i} should use RowBackground.");
                }
            });
        }

        [TestMethod]
        public void VerifyGridLinesVisibilityDrivesHeaderBottomRule()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateTableView();
                tableView.GridLinesVisibility = TableViewGridLinesVisibility.All;
                LoadTableView(tableView);

                VerifyHeaderBottomRule(tableView, TableViewGridLinesVisibility.All, 1);
                VerifyHeaderBottomRule(tableView, TableViewGridLinesVisibility.Horizontal, 1);
                VerifyHeaderBottomRule(tableView, TableViewGridLinesVisibility.Vertical, 0);
                VerifyHeaderBottomRule(tableView, TableViewGridLinesVisibility.None, 0);
            });
        }

        [TestMethod]
        public void VerifyGridLinesVisibilityDrivesHeaderVerticalSeparators()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateTableView();
                tableView.GridLinesVisibility = TableViewGridLinesVisibility.All;
                LoadTableView(tableView);

                VerifyHeaderSeparatorVisibility(tableView, TableViewGridLinesVisibility.All, Visibility.Visible);
                VerifyHeaderSeparatorVisibility(tableView, TableViewGridLinesVisibility.Vertical, Visibility.Visible);
                VerifyHeaderSeparatorVisibility(tableView, TableViewGridLinesVisibility.Horizontal, Visibility.Collapsed);
                VerifyHeaderSeparatorVisibility(tableView, TableViewGridLinesVisibility.None, Visibility.Collapsed);
            });
        }

        [TestMethod]
        public void VerifyHeaderSeparatorsSurviveColumnMutation()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateTableView(columnCount: 3);
                LoadTableView(tableView);

                tableView.Columns.Add(CreateTextColumn("Added", nameof(TableViewTestItem.City)));
                Content.UpdateLayout();
                VerifyHeaderSeparatorCountAndVisibility(tableView, tableView.Columns.Count, Visibility.Visible);

                tableView.Columns.RemoveAt(0);
                Content.UpdateLayout();
                VerifyHeaderSeparatorCountAndVisibility(tableView, tableView.Columns.Count, Visibility.Visible);

                tableView.GridLinesVisibility = TableViewGridLinesVisibility.None;
                Content.UpdateLayout();
                VerifyHeaderSeparatorCountAndVisibility(tableView, tableView.Columns.Count, Visibility.Collapsed);

                tableView.GridLinesVisibility = TableViewGridLinesVisibility.All;
                Content.UpdateLayout();
                VerifyHeaderSeparatorCountAndVisibility(tableView, tableView.Columns.Count, Visibility.Visible);
            });
        }

        [TestMethod]
        public void VerifyDensityResolvesHeaderAndRowMinHeights()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateTableView();
                tableView.Density = TableViewDensity.Compact;
                LoadTableView(tableView);

                VerifyDensityMinHeights(tableView, TableViewDensity.Compact, 26, 30);
                VerifyDensityMinHeights(tableView, TableViewDensity.Standard, 32, 40);
                VerifyDensityMinHeights(tableView, TableViewDensity.Comfortable, 40, 48);
                VerifyDensityMinHeights(tableView, TableViewDensity.Compact, 26, 30);
            });
        }

        [TestMethod]
        public void VerifyThemeResourceKeysResolveInAllThemes()
        {
            foreach (ElementTheme theme in new[] { ElementTheme.Light, ElementTheme.Dark })
            {
                RunOnUIThread.Execute(() =>
                {
                    var host = CreateHost(theme);
                    host.Children.Add(CreateTableView(itemCount: 1, columnCount: 1));
                    Content = host;
                    Content.UpdateLayout();

                    foreach (string key in BrushResourceKeys)
                    {
                        var probe = CreateBrushProbe(key);
                        host.Children.Add(probe);
                        Content.UpdateLayout();
                        Verify.IsTrue(probe.Background is Brush, $"{key} should resolve to a Brush in {theme} theme.");
                    }

                    foreach (string key in DoubleResourceKeys)
                    {
                        var probe = CreateDoubleProbe(key);
                        host.Children.Add(probe);
                        Content.UpdateLayout();
                        Verify.IsTrue(probe.MinHeight > 0, $"{key} should resolve to a positive double in {theme} theme.");
                    }
                });
            }
        }

        [TestMethod]
        public void VerifyThemeSwitchReresolvesGridLineBrushes()
        {
            Grid host = null;
            TableView tableView = null;
            Color lightHorizontalGridLineColor = default;
            Color lightVerticalGridLineColor = default;

            RunOnUIThread.Execute(() =>
            {
                host = CreateHost(ElementTheme.Light);
                tableView = CreateTableView();
                tableView.GridLinesVisibility = TableViewGridLinesVisibility.All;
                host.Children.Add(tableView);
                Content = host;
                Content.UpdateLayout();

                lightHorizontalGridLineColor = GetSolidColorBrushColor(GetRealizedRow(tableView, 0).BorderBrush, "light horizontal row grid line");
                lightVerticalGridLineColor = GetSolidColorBrushColor(GetCellWrapper(GetRealizedRow(tableView, 0), 0).BorderBrush, "light vertical cell grid line");

                host.RequestedTheme = ElementTheme.Dark;
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var darkHorizontalGridLineColor = GetSolidColorBrushColor(GetRealizedRow(tableView, 0).BorderBrush, "dark horizontal row grid line");
                var darkVerticalGridLineColor = GetSolidColorBrushColor(GetCellWrapper(GetRealizedRow(tableView, 0), 0).BorderBrush, "dark vertical cell grid line");

                Verify.AreNotEqual(lightHorizontalGridLineColor, darkHorizontalGridLineColor, "Horizontal grid line brush should re-resolve after theme change.");
                Verify.AreNotEqual(lightVerticalGridLineColor, darkVerticalGridLineColor, "Vertical grid line brush should re-resolve after theme change.");
            });
        }

        private static readonly string[] BrushResourceKeys =
        {
            "TabularSurfaceGridLineBrush",
            "TabularSurfaceVerticalGridLineBrush",
            "TabularSurfaceBorderBrush",
            "TabularSurfaceHeaderSeparatorBrush",
            "TabularSurfaceHeaderBackgroundBrush",
            "TabularSurfaceRowBackgroundAlternatingBrush",
        };

        private static readonly string[] DoubleResourceKeys =
        {
            "TableViewHeaderMinHeight",
            "TableViewHeaderMinHeightCompact",
            "TableViewHeaderMinHeightComfortable",
            "TableViewRowMinHeight",
            "TableViewRowMinHeightCompact",
            "TableViewRowMinHeightComfortable",
        };

        private TableView CreateTableView(int itemCount = 4, int columnCount = 2)
        {
            var tableView = new TableView
            {
                Width = 500,
                Height = 300,
                ItemsSource = new ObservableCollection<TableViewTestItem>(
                    Enumerable.Range(0, itemCount).Select(i => new TableViewTestItem
                    {
                        Name = $"Name {i}",
                        City = $"City {i}",
                    })),
            };

            for (int i = 0; i < columnCount; i++)
            {
                tableView.Columns.Add(CreateTextColumn($"Column {i}", (i % 2) == 0 ? nameof(TableViewTestItem.Name) : nameof(TableViewTestItem.City)));
            }

            return tableView;
        }

        private TableViewTextColumn CreateTextColumn(string header, string path)
        {
            return new TableViewTextColumn
            {
                Header = header,
                Binding = new Binding { Path = new PropertyPath(path) },
                Width = new GridLength(120),
            };
        }

        private Grid CreateHost(ElementTheme requestedTheme = ElementTheme.Default)
        {
            return new Grid
            {
                Width = 600,
                Height = 400,
                RequestedTheme = requestedTheme,
            };
        }

        private void LoadTableView(TableView tableView, ElementTheme requestedTheme = ElementTheme.Default)
        {
            var host = CreateHost(requestedTheme);
            host.Children.Add(tableView);
            Content = host;
            Content.UpdateLayout();
        }

        private void VerifyHeaderBottomRule(TableView tableView, TableViewGridLinesVisibility visibility, double expectedBottom)
        {
            tableView.GridLinesVisibility = visibility;
            Content.UpdateLayout();
            Verify.AreEqual(expectedBottom, GetHeaderRow(tableView).BorderThickness.Bottom, $"Header bottom rule should be {expectedBottom} for {visibility}.");
        }

        private void VerifyHeaderSeparatorVisibility(TableView tableView, TableViewGridLinesVisibility visibility, Visibility expectedVisibility)
        {
            tableView.GridLinesVisibility = visibility;
            Content.UpdateLayout();
            VerifyHeaderSeparatorCountAndVisibility(tableView, tableView.Columns.Count, expectedVisibility);
        }

        private void VerifyHeaderSeparatorCountAndVisibility(TableView tableView, int expectedCount, Visibility expectedVisibility)
        {
            var separators = GetHeaderSeparators(tableView);
            Verify.AreEqual(expectedCount, separators.Count, "Header separator count should match column count.");

            foreach (var separator in separators)
            {
                Verify.AreEqual(expectedVisibility, separator.Visibility);
            }
        }

        private void VerifyDensityMinHeights(TableView tableView, TableViewDensity density, double expectedHeaderMinHeight, double expectedRowMinHeight)
        {
            tableView.Density = density;
            Content.UpdateLayout();

            var headerCell = GetHeaderCell(tableView, 0);
            var row = GetRealizedRow(tableView, 0);
            Verify.AreEqual(expectedHeaderMinHeight, headerCell.MinHeight, $"Header MinHeight should match {density} density.");
            Verify.AreEqual(expectedRowMinHeight, row.MinHeight, $"Row MinHeight should match {density} density.");
            Verify.AreNotEqual(headerCell.MinHeight, row.MinHeight, $"Header and row MinHeight should differ for {density} density.");
        }

        private Border GetHeaderRow(TableView tableView)
        {
            var headerRow = tableView.FindVisualChildByName("PART_HeaderRow") as Border;
            Verify.IsNotNull(headerRow, "PART_HeaderRow should be realized.");
            return headerRow;
        }

        private Panel GetHeaderHost(TableView tableView)
        {
            var headerHost = tableView.FindVisualChildByName("PART_HeaderHost") as Panel;
            Verify.IsNotNull(headerHost, "PART_HeaderHost should be realized.");
            return headerHost;
        }

        private Panel GetHeaderCell(TableView tableView, int index)
        {
            var headerHost = GetHeaderHost(tableView);
            Verify.IsTrue(headerHost.Children.Count > index, $"Header cell {index} should be realized.");
            var headerCell = headerHost.Children[index] as Panel;
            Verify.IsNotNull(headerCell, $"Header cell {index} should be a Panel.");
            return headerCell;
        }

        private List<Border> GetHeaderSeparators(TableView tableView)
        {
            var separators = new List<Border>();
            var headerHost = GetHeaderHost(tableView);

            foreach (var child in headerHost.Children)
            {
                if (child is Panel headerCell)
                {
                    foreach (var headerCellChild in headerCell.Children)
                    {
                        if (headerCellChild is Border border &&
                            border.Width == 1 &&
                            !border.IsHitTestVisible)
                        {
                            separators.Add(border);
                        }
                    }
                }
            }

            return separators;
        }

        private ItemsRepeater GetRowsRepeater(TableView tableView)
        {
            var repeater = tableView.FindVisualChildByName("PART_RowsRepeater") as ItemsRepeater;
            Verify.IsNotNull(repeater, "PART_RowsRepeater should be realized.");
            return repeater;
        }

        private TableViewRow GetRealizedRow(TableView tableView, int index)
        {
            var row = GetRowsRepeater(tableView).TryGetElement(index) as TableViewRow;
            Verify.IsNotNull(row, $"TableViewRow at index {index} should be realized.");
            return row;
        }

        private Border GetCellWrapper(TableViewRow row, int index)
        {
            var cellsHost = row.FindVisualChildByName("PART_CellsHost") as Panel;
            Verify.IsNotNull(cellsHost, "PART_CellsHost should be realized.");
            Verify.IsTrue(cellsHost.Children.Count > index, $"Cell wrapper {index} should be realized.");
            var cellWrapper = cellsHost.Children[index] as Border;
            Verify.IsNotNull(cellWrapper, $"Cell wrapper {index} should be a Border.");
            return cellWrapper;
        }

        private Border CreateBrushProbe(string key)
        {
            return (Border)XamlReader.Load(
                "<Border xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' " +
                "Background='{ThemeResource " + key + "}' />");
        }

        private Border CreateDoubleProbe(string key)
        {
            return (Border)XamlReader.Load(
                "<Border xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' " +
                "MinHeight='{ThemeResource " + key + "}' />");
        }

        private Color GetSolidColorBrushColor(Brush brush, string brushDescription)
        {
            var solidColorBrush = brush as SolidColorBrush;
            Verify.IsNotNull(solidColorBrush, $"{brushDescription} should be a SolidColorBrush.");
            return solidColorBrush.Color;
        }

        private class TableViewTestItem
        {
            public string Name { get; set; }
            public string City { get; set; }
        }
    }
}
