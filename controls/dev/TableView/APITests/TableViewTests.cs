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
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = new TableView();
                Verify.IsNotNull(tableView);
                // Before the default style is applied, these are the DP metadata defaults.
                Verify.AreEqual(TableViewGridLinesVisibility.All, tableView.GridLinesVisibility);
                Verify.AreEqual(TableViewHeadersVisibility.Column, tableView.HeadersVisibility);
                Verify.AreEqual(TableViewDensity.Standard, tableView.Density);
                Verify.IsNull(tableView.RowBackground);
                Verify.IsNull(tableView.AlternatingRowBackground);
                Verify.IsTrue(tableView.IsReadOnly);

                LoadTableView(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                // Loaded, so the default style has been applied. The style must NOT introduce
                // banding or override the gridline default: banding is opt-in, and ClearValue on
                // AlternatingRowBackground must resolve to null rather than to a style brush.
                Verify.AreEqual(TableViewGridLinesVisibility.All, tableView.GridLinesVisibility,
                    "The default style must not set GridLinesVisibility.");
                Verify.IsNull(tableView.AlternatingRowBackground,
                    "The default style must not set AlternatingRowBackground; banding is opt-in.");
                Verify.IsNull(tableView.RowBackground,
                    "The default style must not set RowBackground.");

                tableView.AlternatingRowBackground = new SolidColorBrush(Colors.Blue);
                tableView.ClearValue(TableView.AlternatingRowBackgroundProperty);
                Verify.IsNull(tableView.AlternatingRowBackground,
                    "ClearValue must resolve to null, not to a default-style brush.");
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
                // Load with the rule OFF so the first assertion cannot be satisfied by the
                // template's own local value. The None -> All transition below is the one that
                // actually proves ApplyGridLinesToHeader restores the rule; without it this test
                // would still pass if the horizontal branch were deleted again.
                tableView = CreateTableView();
                tableView.GridLinesVisibility = TableViewGridLinesVisibility.None;
                LoadTableView(tableView);

                VerifyHeaderBottomRule(tableView, TableViewGridLinesVisibility.None, 0);
                VerifyHeaderBottomRule(tableView, TableViewGridLinesVisibility.All, 1);
                VerifyHeaderBottomRule(tableView, TableViewGridLinesVisibility.Vertical, 0);
                VerifyHeaderBottomRule(tableView, TableViewGridLinesVisibility.Horizontal, 1);
                VerifyHeaderBottomRule(tableView, TableViewGridLinesVisibility.None, 0);
                VerifyHeaderBottomRule(tableView, TableViewGridLinesVisibility.All, 1);
            });
        }

        [TestMethod]
        public void VerifyGridLinesVisibilityDrivesBodyRules()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateTableView();
                tableView.GridLinesVisibility = TableViewGridLinesVisibility.None;
                LoadTableView(tableView);

                // Body rules live in TableViewRow, separately from the header logic above.
                VerifyBodyRules(tableView, TableViewGridLinesVisibility.None, 0, 0);
                VerifyBodyRules(tableView, TableViewGridLinesVisibility.All, 1, 1);
                VerifyBodyRules(tableView, TableViewGridLinesVisibility.Horizontal, 1, 0);
                VerifyBodyRules(tableView, TableViewGridLinesVisibility.Vertical, 0, 1);
                VerifyBodyRules(tableView, TableViewGridLinesVisibility.All, 1, 1);
            });
        }

        [TestMethod]
        public void VerifyGridLinesVisibilityDrivesHeaderVerticalSeparators()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateTableView();
                tableView.GridLinesVisibility = TableViewGridLinesVisibility.None;
                LoadTableView(tableView);

                VerifyHeaderSeparatorVisibility(tableView, TableViewGridLinesVisibility.None, Visibility.Collapsed);
                VerifyHeaderSeparatorVisibility(tableView, TableViewGridLinesVisibility.All, Visibility.Visible);
                VerifyHeaderSeparatorVisibility(tableView, TableViewGridLinesVisibility.Horizontal, Visibility.Collapsed);
                VerifyHeaderSeparatorVisibility(tableView, TableViewGridLinesVisibility.Vertical, Visibility.Visible);
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
            });

            IdleSynchronizer.Wait();

            // Columns mutations route through QueueRebuildHeaders, which posts to the dispatcher
            // queue. UpdateLayout does not drain that queue, so each mutation needs its own
            // RunOnUIThread block followed by an idle wait before the assertion.
            RunOnUIThread.Execute(() =>
            {
                tableView.Columns.Add(CreateTextColumn("Added", nameof(TableViewTestItem.City)));
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                VerifyHeaderSeparatorCountAndVisibility(tableView, tableView.Columns.Count, Visibility.Visible);
                tableView.Columns.RemoveAt(0);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                VerifyHeaderSeparatorCountAndVisibility(tableView, tableView.Columns.Count, Visibility.Visible);

                // GridLinesVisibility applies synchronously, so no further wait is needed.
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
            // A key that resolves in Default/Dark but is missing from Light would otherwise fall
            // back silently, so each key is also required to carry a per-theme colour, and Light
            // must differ from Dark. Skipped under High Contrast, where every token is
            // deliberately mapped to the same system colour.
            bool highContrast = false;
            RunOnUIThread.Execute(() =>
            {
                highContrast = new global::Windows.UI.ViewManagement.AccessibilitySettings().HighContrast;
            });

            var colorsByTheme = new Dictionary<ElementTheme, Dictionary<string, Color>>();

            foreach (ElementTheme theme in new[] { ElementTheme.Light, ElementTheme.Dark })
            {
                RunOnUIThread.Execute(() =>
                {
                    var host = CreateHost(theme);
                    host.Children.Add(CreateTableView(itemCount: 1, columnCount: 1));
                    Content = host;
                    Content.UpdateLayout();

                    var colors = new Dictionary<string, Color>();
                    foreach (string key in BrushResourceKeys)
                    {
                        var probe = CreateBrushProbe(key);
                        host.Children.Add(probe);
                        Content.UpdateLayout();
                        Verify.IsTrue(probe.Background is Brush, $"{key} should resolve to a Brush in {theme} theme.");
                        colors[key] = GetSolidColorBrushColor(probe.Background, $"{key} in {theme}");
                    }
                    colorsByTheme[theme] = colors;

                    foreach (string key in DoubleResourceKeys)
                    {
                        var probe = CreateDoubleProbe(key);
                        host.Children.Add(probe);
                        Content.UpdateLayout();
                        Verify.IsTrue(probe.MinHeight > 0, $"{key} should resolve to a positive double in {theme} theme.");
                    }
                });
            }

            if (highContrast)
            {
                Log.Comment("High Contrast is active; skipping the per-theme colour divergence check.");
                return;
            }

            foreach (string key in BrushResourceKeys)
            {
                Verify.AreNotEqual(
                    colorsByTheme[ElementTheme.Light][key],
                    colorsByTheme[ElementTheme.Dark][key],
                    $"{key} must be defined separately in the Light and Default dictionaries.");
            }
        }

        [TestMethod]
        public void VerifyThemeSwitchReresolvesGridLineBrushes()
        {
            // Under High Contrast every grid-line token maps to SystemColorWindowTextColor, so a
            // Light/Dark divergence assertion would be wrong rather than informative.
            bool highContrast = false;
            RunOnUIThread.Execute(() =>
            {
                highContrast = new global::Windows.UI.ViewManagement.AccessibilitySettings().HighContrast;
            });

            if (highContrast)
            {
                Log.Comment("High Contrast is active; skipping the theme-divergence assertions.");
                return;
            }

            Grid host = null;
            TableView tableView = null;
            Color lightHorizontalGridLineColor = default;
            Color lightVerticalGridLineColor = default;
            Color lightHeaderSeparatorColor = default;
            Color lightBorderColor = default;

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
                lightHeaderSeparatorColor = GetSolidColorBrushColor(GetHeaderRow(tableView).BorderBrush, "light header separator");
                lightBorderColor = GetSolidColorBrushColor(tableView.BorderBrush, "light control frame");

                host.RequestedTheme = ElementTheme.Dark;
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var darkHorizontalGridLineColor = GetSolidColorBrushColor(GetRealizedRow(tableView, 0).BorderBrush, "dark horizontal row grid line");
                var darkVerticalGridLineColor = GetSolidColorBrushColor(GetCellWrapper(GetRealizedRow(tableView, 0), 0).BorderBrush, "dark vertical cell grid line");
                var darkHeaderSeparatorColor = GetSolidColorBrushColor(GetHeaderRow(tableView).BorderBrush, "dark header separator");
                var darkBorderColor = GetSolidColorBrushColor(tableView.BorderBrush, "dark control frame");

                Verify.AreNotEqual(lightHorizontalGridLineColor, darkHorizontalGridLineColor, "Horizontal grid line brush should re-resolve after theme change.");
                Verify.AreNotEqual(lightVerticalGridLineColor, darkVerticalGridLineColor, "Vertical grid line brush should re-resolve after theme change.");
                Verify.AreNotEqual(lightHeaderSeparatorColor, darkHeaderSeparatorColor, "Header separator brush should re-resolve after theme change.");
                Verify.AreNotEqual(lightBorderColor, darkBorderColor, "Control frame brush should re-resolve after theme change.");

                // The frame must not be the column-separator token: they are independent concerns.
                Verify.AreNotEqual(darkBorderColor, darkVerticalGridLineColor, "The control frame must not reuse the column-separator brush.");
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

        private void VerifyBodyRules(TableView tableView, TableViewGridLinesVisibility visibility, double expectedRowBottom, double expectedCellRight)
        {
            tableView.GridLinesVisibility = visibility;
            Content.UpdateLayout();

            var row = GetRealizedRow(tableView, 0);
            Verify.AreEqual(expectedRowBottom, row.BorderThickness.Bottom, $"Row bottom rule should be {expectedRowBottom} for {visibility}.");

            var cellWrapper = GetCellWrapper(row, 0);
            Verify.AreEqual(expectedCellRight, cellWrapper.BorderThickness.Right, $"Cell vertical rule should be {expectedCellRight} for {visibility}.");
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
