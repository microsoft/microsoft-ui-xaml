// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Controls.Tabular;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Markup;
using MUXControlsTestApp.Utilities;
using WEX.TestExecution;
using WEX.TestExecution.Markup;

using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewTestHelpers;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    [TestClass]
    public class TableViewContextFlyoutTests : TableViewApiTestBase
    {
        [TestMethod]
        public void VerifyContextFlyoutDependencyPropertiesAndInheritance()
        {
            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(typeof(FlyoutBase), typeof(TableViewColumn).GetProperty("CellContextFlyout").PropertyType);
                Verify.AreEqual(typeof(FlyoutBase), typeof(TableViewColumn).GetProperty("HeaderContextFlyout").PropertyType);
                Verify.IsNotNull(TableViewColumn.CellContextFlyoutProperty);
                Verify.IsNotNull(TableViewColumn.HeaderContextFlyoutProperty);
                Verify.AreNotEqual(TableViewColumn.CellContextFlyoutProperty, TableViewColumn.HeaderContextFlyoutProperty);
                Verify.AreEqual(TableViewColumn.CellContextFlyoutProperty, TableViewTextColumn.CellContextFlyoutProperty);
                Verify.AreEqual(TableViewColumn.HeaderContextFlyoutProperty, TableViewTextColumn.HeaderContextFlyoutProperty);
                Verify.AreEqual(TableViewColumn.CellContextFlyoutProperty, TableViewTemplateColumn.CellContextFlyoutProperty);
                Verify.AreEqual(TableViewColumn.HeaderContextFlyoutProperty, TableViewTemplateColumn.HeaderContextFlyoutProperty);

                foreach (var column in new TableViewColumn[]
                {
                    new TableViewColumn(), new TableViewTextColumn(), new TableViewTemplateColumn()
                })
                {
                    Verify.IsNull(column.CellContextFlyout);
                    Verify.IsNull(column.HeaderContextFlyout);
                    Verify.IsNull(column.GetValue(TableViewColumn.CellContextFlyoutProperty));
                    Verify.IsNull(column.GetValue(TableViewColumn.HeaderContextFlyoutProperty));

                    var menu = new MenuFlyout();
                    var flyout = new Flyout();
                    column.CellContextFlyout = menu;
                    column.HeaderContextFlyout = flyout;
                    Verify.AreEqual(menu, column.GetValue(TableViewColumn.CellContextFlyoutProperty));
                    Verify.AreEqual(flyout, column.GetValue(TableViewColumn.HeaderContextFlyoutProperty));

                    column.SetValue(TableViewColumn.CellContextFlyoutProperty, flyout);
                    column.SetValue(TableViewColumn.HeaderContextFlyoutProperty, menu);
                    Verify.AreEqual(flyout, column.CellContextFlyout);
                    Verify.AreEqual(menu, column.HeaderContextFlyout);

                    column.ClearValue(TableViewColumn.CellContextFlyoutProperty);
                    column.ClearValue(TableViewColumn.HeaderContextFlyoutProperty);
                    Verify.IsNull(column.CellContextFlyout);
                    Verify.IsNull(column.HeaderContextFlyout);
                }
            });
        }

        [TestMethod]
        public void VerifyXamlAssignsContextFlyoutsToBothColumnTypes()
        {
            TableView table = null;
            RunOnUIThread.Execute(() =>
            {
                EnsureTabularControlsResources();
                table = (TableView)XamlReader.Load(
                    @"<t:TableView xmlns=""http://schemas.microsoft.com/winfx/2006/xaml/presentation""
                                   xmlns:x=""http://schemas.microsoft.com/winfx/2006/xaml""
                                   xmlns:t=""using:Microsoft.UI.Xaml.Controls.Tabular""
                                   Width=""500"" Height=""300"">
                        <t:TableView.Resources>
                            <MenuFlyout x:Key=""CellMenu""><MenuFlyoutItem Text=""Cell"" /></MenuFlyout>
                            <Flyout x:Key=""HeaderMenu""><TextBlock Text=""Header"" /></Flyout>
                        </t:TableView.Resources>
                        <t:TableViewTextColumn Header=""Text"" Binding=""{Binding Name}""
                            CellContextFlyout=""{StaticResource CellMenu}""
                            HeaderContextFlyout=""{StaticResource HeaderMenu}"" />
                        <t:TableViewTemplateColumn Header=""Template""
                            CellContextFlyout=""{StaticResource CellMenu}""
                            HeaderContextFlyout=""{StaticResource HeaderMenu}"">
                            <t:TableViewTemplateColumn.CellTemplate>
                                <DataTemplate><TextBlock Text=""{Binding Name}"" /></DataTemplate>
                            </t:TableViewTemplateColumn.CellTemplate>
                        </t:TableViewTemplateColumn>
                    </t:TableView>");
                table.ItemsSource = MakeItems();
                LoadContent(table);
            });
            SettleLayout(table);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(table.Columns[0] is TableViewTextColumn);
                Verify.IsTrue(table.Columns[1] is TableViewTemplateColumn);
                foreach (var column in table.Columns)
                {
                    Verify.AreEqual(table.Resources["CellMenu"], column.CellContextFlyout);
                    Verify.AreEqual(table.Resources["HeaderMenu"], column.HeaderContextFlyout);
                }
                VerifyNativeAttachments(table);
            });
        }

        [TestMethod]
        public void VerifyNativeWrappersOwnMenusWithoutChangingCellRoots()
        {
            TableView table = null;
            RunOnUIThread.Execute(() =>
            {
                table = CreateContextTable();
                LoadContent(table);
            });
            SettleLayout(table);

            RunOnUIThread.Execute(() =>
            {
                VerifyNativeAttachments(table);
                foreach (var row in GetProjectedElements(table).Cast<TableViewRow>())
                {
                    var textRoot = (FrameworkElement)GetCellContent(GetRowCell(row, 0));
                    var templateRoot = (FrameworkElement)GetCellContent(GetRowCell(row, 1));
                    Verify.IsTrue(textRoot.ContextFlyout is TextCommandBarFlyout,
                        "The generated text root must retain its native text menu, not receive the column menu.");
                    Verify.IsNull(templateRoot.ContextFlyout, "The template presenter must not receive the column menu.");
                    Verify.AreEqual(DependencyProperty.UnsetValue, textRoot.ReadLocalValue(UIElement.ContextFlyoutProperty));
                    Verify.AreEqual(DependencyProperty.UnsetValue, templateRoot.ReadLocalValue(UIElement.ContextFlyoutProperty));
                    var child = templateRoot.FindVisualChildByName("ChildWithMenu") as Border;
                    Verify.IsNotNull(child);
                    Verify.AreEqual("Child", ((MenuFlyoutItem)((MenuFlyout)child.ContextFlyout).Items[0]).Text);
                    Verify.IsNull(GetRowCell(row, 2).ContextFlyout, "Unconfigured columns retain the native null fallback.");
                }
            });
        }

        [TestMethod]
        public void VerifyLiveReplacementAndClearingPreserveContainersAndOtherColumns()
        {
            TableView table = null;
            Grid cell = null;
            Grid initiallyUnconfiguredCell = null;
            FrameworkElement header = null;
            FrameworkElement initiallyUnconfiguredHeader = null;
            UIElement root = null;
            FlyoutBase otherCellMenu = null;
            FlyoutBase otherHeaderMenu = null;
            RunOnUIThread.Execute(() =>
            {
                table = CreateContextTable();
                LoadContent(table);
            });
            SettleLayout(table);
            RunOnUIThread.Execute(() =>
            {
                cell = GetRowCell(GetProjectedRow(table, 0), 0);
                initiallyUnconfiguredCell = GetRowCell(GetProjectedRow(table, 0), 2);
                header = GetHeaderCell(table, 0);
                initiallyUnconfiguredHeader = GetHeaderCell(table, 2);
                root = GetCellContent(cell);
                otherCellMenu = table.Columns[1].CellContextFlyout;
                otherHeaderMenu = table.Columns[1].HeaderContextFlyout;
                table.Columns[0].CellContextFlyout = new Flyout();
                table.Columns[0].HeaderContextFlyout = new MenuFlyout();
                table.Columns[2].CellContextFlyout = new MenuFlyout();
                table.Columns[2].HeaderContextFlyout = new Flyout();
            });
            SettleLayout(table);
            RunOnUIThread.Execute(() =>
            {
                VerifyNativeAttachments(table);
                Verify.AreEqual(cell, GetRowCell(GetProjectedRow(table, 0), 0), "Replacing a menu must not regenerate cells.");
                Verify.AreEqual(header, GetHeaderCell(table, 0), "Replacing a menu must not rebuild headers.");
                Verify.AreEqual(root, GetCellContent(cell));
                Verify.AreEqual(initiallyUnconfiguredCell, GetRowCell(GetProjectedRow(table, 0), 2),
                    "A menu assigned after realization must attach even if the column started null.");
                Verify.AreEqual(initiallyUnconfiguredHeader, GetHeaderCell(table, 2));
                Verify.AreEqual(otherCellMenu, table.Columns[1].CellContextFlyout);
                Verify.AreEqual(otherHeaderMenu, table.Columns[1].HeaderContextFlyout);
                table.Columns[0].CellContextFlyout = null;
                table.Columns[0].ClearValue(TableViewColumn.HeaderContextFlyoutProperty);
            });
            SettleLayout(table);
            RunOnUIThread.Execute(() =>
            {
                VerifyNativeAttachments(table);
                Verify.AreEqual(cell, GetRowCell(GetProjectedRow(table, 0), 0));
                Verify.AreEqual(header, GetHeaderCell(table, 0));
                Verify.AreEqual(root, GetCellContent(cell));
                Verify.IsNull(cell.ContextFlyout);
                Verify.IsNull(header.ContextFlyout);
                Verify.AreEqual(otherCellMenu, GetRowCell(GetProjectedRow(table, 0), 1).ContextFlyout);
                Verify.AreEqual(otherHeaderMenu, GetHeaderCell(table, 1).ContextFlyout);
            });
        }

        [TestMethod]
        public void VerifyGeneratedContainerLocalOverridesSurviveColumnUpdates()
        {
            TableView table = null;
            Grid cell = null;
            Grid nullCell = null;
            FrameworkElement header = null;
            FrameworkElement nullHeader = null;
            Grid boundCell = null;
            FrameworkElement boundHeader = null;
            Border bindingSource = null;
            BindingExpression cellBinding = null;
            BindingExpression headerBinding = null;
            MenuFlyout appMenu = null;
            RunOnUIThread.Execute(() =>
            {
                table = CreateContextTable();
                table.Columns[2].HeaderContextFlyout = table.Columns[1].CellContextFlyout;
                LoadContent(table);
            });
            SettleLayout(table);
            RunOnUIThread.Execute(() =>
            {
                cell = GetRowCell(GetProjectedRow(table, 0), 0);
                nullCell = GetRowCell(GetProjectedRow(table, 1), 0);
                header = GetHeaderCell(table, 0);
                nullHeader = GetHeaderCell(table, 1);
                appMenu = new MenuFlyout();
                cell.ContextFlyout = appMenu;
                header.ContextFlyout = appMenu;
                nullCell.ContextFlyout = null;
                nullHeader.ContextFlyout = null;

                boundCell = GetRowCell(GetProjectedRow(table, 0), 1);
                boundHeader = GetHeaderCell(table, 2);
                bindingSource = new Border { ContextFlyout = table.Columns[1].CellContextFlyout };
                Verify.AreEqual(bindingSource.ContextFlyout, boundCell.ContextFlyout);
                Verify.AreEqual(bindingSource.ContextFlyout, boundHeader.ContextFlyout);
                boundCell.SetBinding(UIElement.ContextFlyoutProperty, new Binding
                {
                    Source = bindingSource,
                    Path = new PropertyPath("ContextFlyout"),
                    Mode = BindingMode.OneWay,
                });
                boundHeader.SetBinding(UIElement.ContextFlyoutProperty, new Binding
                {
                    Source = bindingSource,
                    Path = new PropertyPath("ContextFlyout"),
                    Mode = BindingMode.OneWay,
                });
                cellBinding = boundCell.GetBindingExpression(UIElement.ContextFlyoutProperty);
                headerBinding = boundHeader.GetBindingExpression(UIElement.ContextFlyoutProperty);
                Verify.IsNotNull(cellBinding);
                Verify.IsNotNull(headerBinding);
            });
            SettleLayout(table);
            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(bindingSource.ContextFlyout, boundCell.ContextFlyout,
                    "The app binding deliberately starts at the same flyout the column previously assigned.");
                Verify.AreEqual(bindingSource.ContextFlyout, boundHeader.ContextFlyout);
            });

            foreach (bool clear in new[] { false, true, false })
            {
                RunOnUIThread.Execute(() =>
                {
                    table.Columns[0].CellContextFlyout = clear ? null : new MenuFlyout();
                    table.Columns[0].HeaderContextFlyout = clear ? null : new Flyout();
                    table.Columns[1].HeaderContextFlyout = clear ? null : new Flyout();
                    table.Columns[1].CellContextFlyout = clear ? null : new MenuFlyout();
                    table.Columns[2].HeaderContextFlyout = clear ? null : new Flyout();
                });
                SettleLayout(table);
                RunOnUIThread.Execute(() =>
                {
                    Verify.AreEqual(cell, GetRowCell(GetProjectedRow(table, 0), 0));
                    Verify.AreEqual(header, GetHeaderCell(table, 0));
                    Verify.AreEqual(boundCell, GetRowCell(GetProjectedRow(table, 0), 1));
                    Verify.AreEqual(boundHeader, GetHeaderCell(table, 2));
                    Verify.AreEqual(appMenu, cell.ContextFlyout, "An app local value must remain app-owned.");
                    Verify.AreEqual(appMenu, header.ContextFlyout);
                    Verify.IsNull(nullCell.ContextFlyout, "An explicit local null is also an app override.");
                    Verify.IsNull(nullHeader.ContextFlyout);
                    Verify.AreEqual(table.Columns[0].CellContextFlyout, GetRowCell(GetProjectedRow(table, 2), 0).ContextFlyout,
                        "A different row without a local override must still follow the column.");
                    Verify.AreEqual(cellBinding, boundCell.GetBindingExpression(UIElement.ContextFlyoutProperty),
                        "Matching the last assigned flyout does not give the column ownership of an app binding.");
                    Verify.AreEqual(headerBinding, boundHeader.GetBindingExpression(UIElement.ContextFlyoutProperty));
                    Verify.AreEqual(bindingSource.ContextFlyout, boundCell.ContextFlyout);
                    Verify.AreEqual(bindingSource.ContextFlyout, boundHeader.ContextFlyout);
                });
            }

            RunOnUIThread.Execute(() => bindingSource.ContextFlyout = new MenuFlyout());
            SettleLayout(table);
            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(cellBinding, boundCell.GetBindingExpression(UIElement.ContextFlyoutProperty));
                Verify.AreEqual(headerBinding, boundHeader.GetBindingExpression(UIElement.ContextFlyoutProperty));
                Verify.AreEqual(bindingSource.ContextFlyout, boundCell.ContextFlyout,
                    "The preserved binding must still react to a later app-source update.");
                Verify.AreEqual(bindingSource.ContextFlyout, boundHeader.ContextFlyout);
                Verify.AreNotEqual(table.Columns[1].CellContextFlyout, boundCell.ContextFlyout);
                Verify.AreNotEqual(table.Columns[2].HeaderContextFlyout, boundHeader.ContextFlyout);
            });
        }

        [TestMethod]
        public void VerifyNullColumnMenusPreserveApplicationImplicitGridStyle()
        {
            TableView table = null;
            Grid nativeGrid = null;
            Grid originalCell = null;
            FrameworkElement originalHeader = null;
            MenuFlyout appMenu = null;
            object previousStyle = null;
            bool hadStyle = false;
            bool installedStyle = false;
            try
            {
                RunOnUIThread.Execute(() =>
                {
                    hadStyle = Application.Current.Resources.Keys.Contains(typeof(Grid));
                    if (hadStyle)
                    {
                        previousStyle = Application.Current.Resources[typeof(Grid)];
                    }
                    appMenu = new MenuFlyout();
                    appMenu.Items.Add(new MenuFlyoutItem { Text = "Application grid menu" });
                    var style = new Style { TargetType = typeof(Grid) };
                    style.Setters.Add(new Setter(UIElement.ContextFlyoutProperty, appMenu));
                    style.Setters.Add(new Setter(UIElement.OpacityProperty, 0.75));
                    Application.Current.Resources[typeof(Grid)] = style;
                    installedStyle = true;

                    table = CreateContextTable();
                    foreach (var column in table.Columns)
                    {
                        column.CellContextFlyout = null;
                        column.HeaderContextFlyout = null;
                    }
                    nativeGrid = new Grid { Width = 100, Height = 10 };
                    LoadContent(new StackPanel { Children = { nativeGrid, table } });
                });
                SettleLayout(table);
                RunOnUIThread.Execute(() =>
                {
                    VerifyAppliedStyle(0.75, columnMenusActive: false);
                    originalCell = GetRowCell(GetProjectedRow(table, 0), 0);
                    originalHeader = GetHeaderCell(table, 0);
                    foreach (var column in table.Columns)
                    {
                        column.CellContextFlyout = new MenuFlyout();
                        column.HeaderContextFlyout = new Flyout();
                    }
                });
                SettleLayout(table);
                RunOnUIThread.Execute(() =>
                {
                    VerifyAppliedStyle(0.75, columnMenusActive: true);
                    appMenu = new MenuFlyout();
                    appMenu.Items.Add(new MenuFlyoutItem { Text = "Replacement application grid menu" });
                    var replacementStyle = new Style { TargetType = typeof(Grid) };
                    replacementStyle.Setters.Add(new Setter(UIElement.ContextFlyoutProperty, appMenu));
                    replacementStyle.Setters.Add(new Setter(UIElement.OpacityProperty, 0.5));
                    Application.Current.Resources[typeof(Grid)] = replacementStyle;
                });
                SettleLayout(table);
                RunOnUIThread.Execute(() =>
                {
                    VerifyAppliedStyle(0.5, columnMenusActive: true);
                    foreach (var column in table.Columns)
                    {
                        column.CellContextFlyout = null;
                        column.HeaderContextFlyout = null;
                    }
                });
                SettleLayout(table);
                RunOnUIThread.Execute(() =>
                {
                    VerifyAppliedStyle(0.5, columnMenusActive: false);
                    Verify.AreEqual(originalCell, GetRowCell(GetProjectedRow(table, 0), 0),
                        "Style restoration must happen on the existing cell, not through regeneration.");
                    Verify.AreEqual(originalHeader, GetHeaderCell(table, 0));
                });
            }
            finally
            {
                RunOnUIThread.Execute(() =>
                {
                    Content = null;
                    if (installedStyle)
                    {
                        if (hadStyle)
                        {
                            Application.Current.Resources[typeof(Grid)] = previousStyle;
                        }
                        else
                        {
                            Application.Current.Resources.Remove(typeof(Grid));
                        }
                    }
                });
            }

            void VerifyAppliedStyle(double opacity, bool columnMenusActive)
            {
                Verify.AreEqual(opacity, nativeGrid.Opacity, "Positive control: Grid must resolve the current application implicit style.");
                Verify.AreEqual(appMenu, nativeGrid.ContextFlyout);
                for (int i = 0; i < table.Columns.Count; i++)
                {
                    var header = GetHeaderCell(table, i);
                    Verify.AreEqual(opacity, header.Opacity,
                        "The unrelated setter proves that the current style reaches generated wrappers even while column menus are active.");
                    Verify.AreEqual(columnMenusActive ? table.Columns[i].HeaderContextFlyout : appMenu, header.ContextFlyout,
                        "Column opt-in wins, but clearing must expose the current style, not a captured original menu.");
                    if (!columnMenusActive)
                    {
                        Verify.AreEqual(DependencyProperty.UnsetValue, header.ReadLocalValue(UIElement.ContextFlyoutProperty));
                    }
                    foreach (var row in GetProjectedElements(table).Cast<TableViewRow>())
                    {
                        var cell = GetRowCell(row, i);
                        Verify.AreEqual(opacity, cell.Opacity);
                        Verify.AreEqual(columnMenusActive ? table.Columns[i].CellContextFlyout : appMenu, cell.ContextFlyout);
                        if (!columnMenusActive)
                        {
                            Verify.AreEqual(DependencyProperty.UnsetValue, cell.ReadLocalValue(UIElement.ContextFlyoutProperty),
                                "Clearing the owned local value must restore style precedence rather than leave a local null.");
                        }
                    }
                }
            }
        }

        [TestMethod]
        public void VerifyNewRowsAndRebuiltHeadersUseCurrentMenus()
        {
            TableView table = null;
            FrameworkElement originalHeader = null;
            TableViewColumn originalColumn = null;
            var items = new ObservableCollection<Person>(MakeItems());
            RunOnUIThread.Execute(() =>
            {
                table = CreateContextTable(items);
                LoadContent(table);
            });
            SettleLayout(table);
            RunOnUIThread.Execute(() =>
            {
                originalHeader = GetHeaderCell(table, 0);
                originalColumn = table.Columns[0];
                table.Columns.RemoveAt(0);
            });
            SettleLayout(table);
            RunOnUIThread.Execute(() =>
            {
                originalColumn.CellContextFlyout = new Flyout();
                originalColumn.HeaderContextFlyout = new MenuFlyout();
                table.Columns.Insert(0, originalColumn);
                items.Add(new Person { Name = "New row", Role = "Added" });
            });
            SettleLayout(table);
            RunOnUIThread.Execute(() =>
            {
                Verify.AreNotEqual(originalHeader, GetHeaderCell(table, 0), "Removing and reinserting a column must exercise a header rebuild.");
                Verify.AreEqual(items[3], GetProjectedRow(table, 3).DataContext);
                VerifyNativeAttachments(table);
            });
        }

        [TestMethod]
        public void VerifyRecycledRowsKeepColumnMenusAndCurrentRowDataContext()
        {
            TableView table = null;
            var seen = new Dictionary<TableViewRow, object>();
            bool recycled = false;
            double extent = 0;
            RunOnUIThread.Execute(() =>
            {
                table = CreateContextTable(Enumerable.Range(0, 300)
                    .Select(i => new Person { Name = "Row " + i, Role = "Role " + i }).ToList());
                table.Height = 200;
                LoadContent(table);
            });
            SettleLayout(table);
            RunOnUIThread.Execute(() =>
            {
                extent = GetBodyScroller(table).ScrollableHeight;
                foreach (var row in CurrentRows(table))
                {
                    seen.Add(row, row.DataContext);
                }
                Verify.IsGreaterThan(seen.Count, 0);
                table.Columns[0].CellContextFlyout = new Flyout();
            });

            foreach (double fraction in new[] { 0.3, 0.6, 0.9, 0.0 })
            {
                ScrollBodyTo(table, ScrollAxis.Vertical, extent * fraction);
                RunOnUIThread.Execute(() =>
                {
                    VerifyNativeAttachments(table, requireAllRows: false);
                    foreach (var row in CurrentRows(table))
                    {
                        if (seen.TryGetValue(row, out var oldItem) && !ReferenceEquals(oldItem, row.DataContext))
                        {
                            recycled = true;
                        }
                        seen[row] = row.DataContext;
                    }
                });
            }
            Verify.IsTrue(recycled, "The test must observe an existing row container prepared for a different item.");
        }

        private static TableView CreateContextTable(object items = null)
        {
            var table = CreateTableViewShell(items ?? MakeItems(), 600, 300);
            var text = MakeTextColumn("Text", "Name", new GridLength(180));
            text.CellContextFlyout = new MenuFlyout();
            text.HeaderContextFlyout = new Flyout();
            table.Columns.Add(text);
            var template = MakeTemplateColumn("Template", (DataTemplate)XamlReader.Load(
                @"<DataTemplate xmlns=""http://schemas.microsoft.com/winfx/2006/xaml/presentation""
                                xmlns:x=""http://schemas.microsoft.com/winfx/2006/xaml"">
                    <Border x:Name=""ChildWithMenu"">
                        <Border.ContextFlyout><MenuFlyout><MenuFlyoutItem Text=""Child"" /></MenuFlyout></Border.ContextFlyout>
                        <TextBlock Text=""{Binding Name}"" />
                    </Border>
                </DataTemplate>"), new GridLength(180));
            template.CellContextFlyout = new Flyout();
            template.HeaderContextFlyout = new MenuFlyout();
            table.Columns.Add(template);
            table.Columns.Add(MakeTextColumn("Default", "Role", new GridLength(180)));
            return table;
        }

        private static List<TableViewRow> CurrentRows(TableView table) =>
            GetProjectedElements(table, requireAllRealized: false).OfType<TableViewRow>().ToList();

        private static void VerifyNativeAttachments(TableView table, bool requireAllRows = true)
        {
            var rows = GetProjectedElements(table, requireAllRows).OfType<TableViewRow>().ToList();
            Verify.IsGreaterThan(rows.Count, 0, "Attachment checks require realized rows.");
            for (int i = 0; i < table.Columns.Count; i++)
            {
                var header = GetHeaderCell(table, i);
                Verify.IsTrue(header is Grid, "The generated header Grid, including its padding, is the native target.");
                Verify.AreEqual(table.Columns[i].HeaderContextFlyout, header.ContextFlyout);
                foreach (var row in rows)
                {
                    var cell = GetRowCell(row, i);
                    Verify.AreEqual(table.Columns[i].CellContextFlyout, cell.ContextFlyout);
                    Verify.AreEqual(DependencyProperty.UnsetValue, cell.ReadLocalValue(FrameworkElement.DataContextProperty),
                        "Attaching a menu must not pin a cell to a particular row.");
                    Verify.AreEqual(row.DataContext, cell.DataContext);
                }
            }
        }
    }
}
