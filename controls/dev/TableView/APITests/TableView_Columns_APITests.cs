// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Automation.Peers;
using Microsoft.UI.Xaml.Automation.Provider;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Tabular;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Markup;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Shapes;
using MUXControlsTestApp.Utilities;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using Windows.Foundation.Collections;
using Windows.Graphics;

using WEX.Logging.Interop;
using WEX.TestExecution;
using WEX.TestExecution.Markup;

using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewTestHelpers;
using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewEditingTestHelpers;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    // Category 3.1 of the TableView test plan: the Columns collection.
    //
    // These tests assert against two structures the control builds from Columns:
    //   - PART_HeaderHost, whose children are TableViewColumnHeader controls, each tagged with its column
    //     (TableView::RebuildHeaders, header.Tag(column)).
    //   - each realized TableViewRow's PART_CellsHost, whose children are Grid cell wrappers,
    //     each tagged with its column (TableViewRow.cpp:762, cellWrapper.Tag(column)).
    // The Tag is the supported way to map a rendered element back to the column that produced it,
    // and it is what the control's own frozen-column and recycling code uses.
    [TestClass]
    public class TableViewColumnsTests : TableViewApiTestBase
    {
        [TestMethod]
        [TestProperty("Description", "Verifies TableView.Columns is observable despite being typed as IVector, and raises VectorChanged.")]
        public void VerifyColumnsCollectionIsObservable()
        {
            RunOnUIThread.Execute(() =>
            {
                var tableView = new TableView();

                // TableView.idl:452 documents that the ABI exposes IVector but the backing vector is
                // observable, because the control relies on VectorChanged for live column updates.
                // If this cast ever stops working, every incremental column update silently becomes a
                // full rebuild or no rebuild at all, so pin it explicitly.
                var observable = tableView.Columns as IObservableVector<TableViewColumn>;
                Verify.IsNotNull(observable, "Columns should be castable to IObservableVector<TableViewColumn>.");

                var changes = new List<(CollectionChange Change, uint Index)>();
                void OnVectorChanged(IObservableVector<TableViewColumn> sender, IVectorChangedEventArgs args)
                    => changes.Add((args.CollectionChange, args.Index));

                observable.VectorChanged += OnVectorChanged;
                try
                {
                    var first = new TableViewTextColumn { Header = "Name" };
                    var second = new TableViewTextColumn { Header = "Role" };

                    tableView.Columns.Add(first);
                    tableView.Columns.Insert(0, second);
                    tableView.Columns.RemoveAt(0);
                    tableView.Columns.Clear();
                }
                finally
                {
                    observable.VectorChanged -= OnVectorChanged;
                }

                Verify.AreEqual(4, changes.Count, "Add, Insert, RemoveAt, and Clear should each raise exactly one notification.");
                Verify.AreEqual(CollectionChange.ItemInserted, changes[0].Change, "Add should report ItemInserted.");
                Verify.AreEqual(0u, changes[0].Index, "Add to an empty collection should report index 0.");
                Verify.AreEqual(CollectionChange.ItemInserted, changes[1].Change, "Insert should report ItemInserted.");
                Verify.AreEqual(0u, changes[1].Index, "Insert at 0 should report index 0.");
                Verify.AreEqual(CollectionChange.ItemRemoved, changes[2].Change, "RemoveAt should report ItemRemoved.");
                Verify.AreEqual(0u, changes[2].Index, "RemoveAt(0) should report index 0.");
                Verify.AreEqual(CollectionChange.Reset, changes[3].Change, "Clear should report Reset.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies adding a column creates its header and a cell in every realized row.")]
        public void VerifyAddColumnAddsHeaderAndCells()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateLoadedTableView("Name");
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                VerifyHeaderColumns(tableView, tableView.Columns.ToArray(), "before the add");
                VerifyEveryRowMatchesHeaders(tableView, "before the add");

                tableView.Columns.Add(new TableViewTextColumn { Header = "Role" });
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(2, tableView.Columns.Count);
                VerifyHeaderColumns(tableView, tableView.Columns.ToArray(), "after the add");
                VerifyEveryRowMatchesHeaders(tableView, "after the add");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies inserting a column places its header and cells at the requested index.")]
        public void VerifyInsertColumnPlacesHeaderAtIndex()
        {
            TableView tableView = null;
            TableViewColumn inserted = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateLoadedTableView("Name", "Role");
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                inserted = new TableViewTextColumn { Header = "City" };
                tableView.Columns.Insert(1, inserted);
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                // Order matters, not just membership: an insert that appends would still produce the
                // right set of headers while rendering the table in the wrong order.
                var headerColumns = GetHeaderColumns(tableView);
                Verify.AreEqual(3, headerColumns.Count);
                Verify.AreEqual(inserted, headerColumns[1], "The inserted column's header should be at index 1.");

                VerifyHeaderColumns(tableView, tableView.Columns.ToArray(), "after the insert");
                VerifyEveryRowMatchesHeaders(tableView, "after the insert");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies removing a column removes its header and its cell from every row.")]
        public void VerifyRemoveColumnRemovesHeaderAndCells()
        {
            TableView tableView = null;
            TableViewColumn removed = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateLoadedTableView("Name", "Role", "City");
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                removed = tableView.Columns[1];
                tableView.Columns.RemoveAt(1);
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var headerColumns = GetHeaderColumns(tableView);
                Verify.AreEqual(2, headerColumns.Count);
                Verify.IsFalse(headerColumns.Contains(removed), "The removed column should have no header.");

                foreach (var row in GetRealizedRows(tableView))
                {
                    Verify.IsFalse(GetRowCellColumns(row).Contains(removed), "The removed column should have no cell in any row.");
                }

                VerifyHeaderColumns(tableView, tableView.Columns.ToArray(), "after the remove");
                VerifyEveryRowMatchesHeaders(tableView, "after the remove");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies Columns.Clear removes every header and leaves rows with no cells.")]
        public void VerifyClearColumnsRemovesAllHeaders()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateLoadedTableView("Name", "Role");
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                tableView.Columns.Clear();
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(0, tableView.Columns.Count);
                Verify.AreEqual(0, GetHeaderColumns(tableView).Count, "Clearing Columns should leave no header cells.");

                foreach (var row in GetRealizedRows(tableView))
                {
                    Verify.AreEqual(0, GetRowCellColumns(row).Count, "Clearing Columns should leave every row with no cells.");
                }
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies replacing a column at an index swaps the header and cells at that position only.")]
        public void VerifyReplaceColumnSwapsHeaderAndCells()
        {
            TableView tableView = null;
            TableViewColumn replaced = null;
            TableViewColumn replacement = null;
            TableViewColumn untouched = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateLoadedTableView("Name", "Role");
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                replaced = tableView.Columns[0];
                untouched = tableView.Columns[1];
                replacement = new TableViewTextColumn { Header = "City" };

                tableView.Columns[0] = replacement;
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var headerColumns = GetHeaderColumns(tableView);
                Verify.AreEqual(2, headerColumns.Count);
                Verify.AreEqual(replacement, headerColumns[0], "The replacement column should own index 0.");
                Verify.AreEqual(untouched, headerColumns[1], "The column at index 1 should be untouched by the replace.");
                Verify.IsFalse(headerColumns.Contains(replaced), "The replaced column should no longer have a header.");

                VerifyEveryRowMatchesHeaders(tableView, "after the replace");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies column edits made before the control loads are reflected on first layout.")]
        public void VerifyColumnsChangedBeforeLoadAppliesOnLoad()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                // Every mutation here happens with no template applied and no visual tree, so the
                // control cannot rebuild incrementally; it must reconcile on first layout instead.
                tableView = CreateTableViewShell(MakeItems(), 500, 300);

                var scratch = new TableViewTextColumn { Header = "Scratch" };
                tableView.Columns.Add(scratch);
                tableView.Columns.Add(new TableViewTextColumn { Header = "Name" });
                tableView.Columns.Insert(0, new TableViewTextColumn { Header = "Role" });
                tableView.Columns.Remove(scratch);

                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(2, tableView.Columns.Count);
                VerifyHeaderColumns(tableView, tableView.Columns.ToArray(), "after loading with pre-load column edits");
                VerifyEveryRowMatchesHeaders(tableView, "after loading with pre-load column edits");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies the header host and every row's cells host stay in sync across add, insert, remove, and clear.")]
        public void VerifyHeaderAndRowCellsStayInSyncAfterColumnMutation()
        {
            // Headers and row cells are rebuilt by two independent reactions to the same
            // Columns.VectorChanged: TableView::RebuildHeaders and TableViewRow::OnColumnsVectorChanged.
            // If one path runs and the other does not, the two hosts desynchronize. That is checked
            // structurally here rather than by comparing pixel offsets: both hosts are the same
            // TableViewCellsPanel type and arrange children from column ActualWidth in Columns order,
            // so identical column lists are what alignment actually reduces to.
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateLoadedTableView("Name", "Role");
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() => VerifyEveryRowMatchesHeaders(tableView, "initial"));

            var mutations = new (string Name, Action<TableView> Apply)[]
            {
                ("add", tv => tv.Columns.Add(new TableViewTextColumn { Header = "City" })),
                ("insert", tv => tv.Columns.Insert(0, new TableViewTextColumn { Header = "Score" })),
                ("remove", tv => tv.Columns.RemoveAt(1)),
                ("replace", tv => tv.Columns[0] = new TableViewTextColumn { Header = "Notes" }),
                ("clear", tv => tv.Columns.Clear()),
            };

            foreach (var mutation in mutations)
            {
                var current = mutation;

                RunOnUIThread.Execute(() =>
                {
                    current.Apply(tableView);
                    Content.UpdateLayout();
                });

                IdleSynchronizer.Wait();

                RunOnUIThread.Execute(() =>
                {
                    VerifyHeaderColumns(tableView, tableView.Columns.ToArray(), $"after {current.Name}");
                    VerifyEveryRowMatchesHeaders(tableView, $"after {current.Name}");
                });
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies the same column instance added twice to one TableView renders twice without corrupting layout.")]
        public void VerifySameColumnInstanceAddedTwiceBehavesPerContract()
        {
            // TableViewColumn::SetOwningTableViewInternal rejects only a re-own by a *different*
            // TableView; re-owning by the same control returns true. So a duplicate entry in one
            // Columns collection is owned, tracked, and rendered like any other entry. This pins that
            // documented-by-code behavior: two headers and two cells per row for one column object.
            TableView tableView = null;
            TableViewColumn shared = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateLoadedTableView("Name");
                shared = tableView.Columns[0];
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                tableView.Columns.Add(shared);
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(2, tableView.Columns.Count, "The same instance should be accepted at two indices.");

                var headerColumns = GetHeaderColumns(tableView);
                Verify.AreEqual(2, headerColumns.Count, "A duplicated column should produce one header per entry.");
                Verify.AreEqual(shared, headerColumns[0]);
                Verify.AreEqual(shared, headerColumns[1]);

                VerifyEveryRowMatchesHeaders(tableView, "after duplicating a column instance");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a focused header keeps focus when the header band is rebuilt.")]
        public void VerifyHeaderFocusSurvivesHeaderRebuild()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateLoadedTableView("Name", "Role", "City");
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(GetHeaderCell(tableView, 1).Focus(FocusState.Keyboard), "Precondition: header 1 takes keyboard focus.");
                tableView.Columns[1].Header = "Renamed";
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                VerifyHeaderFocused(tableView, tableView.Columns[1], "after a header rename");
                tableView.CanUserSortColumns = false;
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                VerifyHeaderFocused(tableView, tableView.Columns[1], "after CanUserSortColumns = false");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a header rebuild leaves focus where the app moved it while the focused header's Content was being cleared.")]
        public void VerifyHeaderRebuildKeepsAppFocusMoveFromFocusedHeader()
        {
            TableView tableView = null;
            TextBox external = null;
            bool moved = false;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateTableView(headers: new[] { "Name", "Role", "City" });
                external = new TextBox();
                var host = new StackPanel();
                host.Children.Add(external);
                host.Children.Add(tableView);
                LoadContent(host);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var header = GetHeaderCell(tableView, 1) as ContentControl;
                Verify.IsNotNull(header, "Precondition: the header cell is a ContentControl.");
                Verify.IsTrue(header.Focus(FocusState.Keyboard), "Precondition: header 1 takes keyboard focus.");
                header.RegisterPropertyChangedCallback(ContentControl.ContentProperty, (sender, property) =>
                {
                    if (!moved && ((ContentControl)sender).Content == null)
                    {
                        moved = true;
                        external.Focus(FocusState.Programmatic);
                    }
                });
                tableView.CanUserSortColumns = false;
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(moved, "Precondition: the rebuild cleared the focused header's Content.");
                Verify.IsTrue(ReferenceEquals(external, FocusManager.GetFocusedElement(tableView.XamlRoot)),
                    "The rebuild must not pull focus back from where the app moved it.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a header rebuild leaves focus where the app moved it, out of the table, while the header's focused Button content is being released.")]
        [TestProperty("data:Move", "{ContentCallback, LosingFocusRedirect}")]
        public void VerifyHeaderRebuildKeepsAppFocusMoveFromFocusedHeaderContent()
        {
            var move = ((string)TestContext.DataRow["Move"]).Trim();
            TableView tableView = null;
            TextBox external = null;
            Button headerButton = null;
            bool moved = false;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateTableView(headers: new[] { "Name", "Role", "City" });
                headerButton = new Button { Content = "Role" };
                tableView.Columns[1].Header = headerButton;
                external = new TextBox();
                var host = new StackPanel();
                host.Children.Add(external);
                host.Children.Add(tableView);
                LoadContent(host);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var header = GetHeaderCell(tableView, 1) as ContentControl;
                Verify.IsNotNull(header, "Precondition: the header cell is a ContentControl.");
                Verify.IsTrue(ReferenceEquals(headerButton, header.Content), "Precondition: the header hosts the app's Button.");
                Verify.IsTrue(headerButton.Focus(FocusState.Keyboard), "Precondition: the header's Button takes keyboard focus.");
                if (move == "ContentCallback")
                {
                    header.RegisterPropertyChangedCallback(ContentControl.ContentProperty, (sender, property) =>
                    {
                        if (!moved && ((ContentControl)sender).Content == null)
                        {
                            moved = external.Focus(FocusState.Programmatic);
                        }
                    });
                }
                else
                {
                    // The app sends focus out of the table when its header content gives it up.
                    headerButton.LosingFocus += (sender, args) =>
                    {
                        if (!moved)
                        {
                            moved = args.TrySetNewFocusedElement(external);
                        }
                    };
                }
                tableView.CanUserSortColumns = false;
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(moved, "Precondition: the app moved focus during the rebuild.");
                var focused = FocusManager.GetFocusedElement(tableView.XamlRoot);
                Log.Comment($"Focused after rebuild: {(focused as FrameworkElement)?.GetType().Name}");
                Verify.IsTrue(ReferenceEquals(external, focused),
                    "The rebuild must not pull focus back from where the app moved it.");
            });
        }
        [TestMethod]
        [TestProperty("Description", "Verifies a focused header keeps focus across the header rebuild a theme change performs.")]
        public void VerifyHeaderFocusSurvivesThemeChange()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateLoadedTableView("Name", "Role", "City");
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(GetHeaderCell(tableView, 1).Focus(FocusState.Keyboard), "Precondition: header 1 takes keyboard focus.");
                // ActualThemeChanged only fires on a real change, so flip away from the current theme.
                tableView.RequestedTheme = tableView.ActualTheme == ElementTheme.Dark ? ElementTheme.Light : ElementTheme.Dark;
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                VerifyHeaderFocused(tableView, tableView.Columns[1], "after a theme change");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies hiding the focused column moves focus to the next visible header, or the previous one when it was last.")]
        public void VerifyHidingFocusedColumnMovesFocusToNextVisibleHeader()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateLoadedTableView("Name", "Role", "City");
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(GetHeaderCell(tableView, 1).Focus(FocusState.Keyboard), "Precondition: header 1 takes keyboard focus.");
                tableView.Columns[1].Visibility = Visibility.Collapsed;
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                VerifyHeaderFocused(tableView, tableView.Columns[2], "after hiding the focused middle column");

                Verify.IsTrue(GetHeaderCell(tableView, 2).Focus(FocusState.Keyboard), "Precondition: the last header takes keyboard focus.");
                tableView.Columns[2].Visibility = Visibility.Collapsed;
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                VerifyHeaderFocused(tableView, tableView.Columns[0], "after hiding the focused last column");
            });
        }

        private static void VerifyHeaderFocused(TableView tableView, TableViewColumn column, string context)
        {
            var focused = FocusManager.GetFocusedElement(tableView.XamlRoot) as FrameworkElement;
            Verify.IsNotNull(focused, $"Focus must stay in the header band {context}.");
            Verify.IsTrue(ReferenceEquals(GetHeaderHost(tableView), VisualTreeHelper.GetParent(focused)),
                $"The focused element must be a header cell {context}, not {focused.GetType().Name}.");
            Verify.IsTrue(ReferenceEquals(column, focused.Tag), $"The header of the expected column must hold focus {context}.");
        }


        // Builds a TableView with the given text column headers, loads it, and lays it out.
        private TableView CreateLoadedTableView(params string[] headers)
        {
            var tableView = CreateTableView(headers: headers);
            LoadContent(tableView);
            return tableView;
        }
    }

    // Category 3.2 of the TableView test plan: column ownership.
    [TestClass]
    public class TableViewColumnOwnershipTests : TableViewApiTestBase
    {
        [TestMethod]
        [TestProperty("Description", "Verifies a column instance removed from Columns can be added back and renders again.")]
        public void VerifyRemovedColumnCanBeReAdded()
        {
            TableView tableView = null;
            TableViewColumn column = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateTableView(headers: new[] { "Name", "Role" });
                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                column = tableView.Columns[0];
                tableView.Columns.RemoveAt(0);
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(1, GetHeaderColumns(tableView).Count, "The removed column should be gone before it is re-added.");

                // The same instance goes back in. Ownership was released on removal and has to be
                // re-acquired here, which is the part that can regress.
                tableView.Columns.Insert(0, column);
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var headerColumns = GetHeaderColumns(tableView);
                Verify.AreEqual(2, headerColumns.Count, "Re-adding the column should restore its header.");
                Verify.AreEqual(column, headerColumns[0], "The re-added column should own index 0 again.");
                VerifyEveryRowMatchesHeaders(tableView, "after re-adding a removed column");
            });
        }
    }

    // Categories 3.3 and 3.4 of the TableView test plan: header content and header visibility.
    //
    // Header content is read back from the ContentPresenter inside each header cell, which is where
    // RebuildHeaders puts Header, HeaderTemplate, and HeaderTemplateSelector.
    [TestClass]
    public class TableViewHeaderTests : TableViewApiTestBase
    {
        [TestMethod]
        [TestProperty("Description", "Verifies string, non-string, and null Header values all render a header cell.")]
        public void VerifyHeaderRendersStringObjectAndNull()
        {
            // Merged from three checklist items that differed only in the value assigned to Header.
            var objectHeader = new Person { Name = "Boxed", Role = "Header" };
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateTableView(headers: new string[0]);
                tableView.Columns.Add(new TableViewTextColumn { Header = "Name" });
                tableView.Columns.Add(new TableViewTextColumn { Header = objectHeader });
                tableView.Columns.Add(new TableViewTextColumn { Header = null });

                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(3, GetHeaderColumns(tableView).Count, "Every column should get a header cell, including the null-header one.");

                Verify.AreEqual("Name", GetRenderedHeaderText(tableView, 0), "A string Header should be presented verbatim.");
                Verify.AreEqual(objectHeader, GetHeaderPresenter(tableView, 1).Content, "A non-string Header should be presented as the object itself, not stringified.");
                Verify.IsNull(GetHeaderPresenter(tableView, 2).Content, "A null Header should present null rather than a placeholder.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies changing Header on a loaded column updates the rendered header.")]
        public void VerifyHeaderChangeAfterLoadUpdatesLive()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateTableView(headers: new[] { "Name", "Role" });
                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual("Name", GetRenderedHeaderText(tableView, 0));

                tableView.Columns[0].Header = "Renamed";
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual("Renamed", GetRenderedHeaderText(tableView, 0),
                    "Changing Header after load should update the rendered header.");
                Verify.AreEqual("Role", GetRenderedHeaderText(tableView, 1),
                    "The other column's header should be untouched.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies HeaderTemplate is applied to the header content presenter.")]
        public void VerifyHeaderTemplateApplies()
        {
            TableView tableView = null;
            DataTemplate template = null;

            RunOnUIThread.Execute(() =>
            {
                template = CreateTextTemplate("Templated");
                tableView = CreateTableView(headers: new[] { "Name" });
                tableView.Columns[0].HeaderTemplate = template;

                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var presenter = GetHeaderPresenter(tableView, 0);
                Verify.AreEqual(template, presenter.ContentTemplate, "HeaderTemplate should be applied to the header presenter.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies HeaderTemplateSelector is applied to the header content presenter.")]
        public void VerifyHeaderTemplateSelectorApplies()
        {
            TableView tableView = null;
            TestHeaderTemplateSelector selector = null;

            RunOnUIThread.Execute(() =>
            {
                selector = new TestHeaderTemplateSelector { Template = CreateTextTemplate("Selected") };
                tableView = CreateTableView(headers: new[] { "Name" });
                tableView.Columns[0].HeaderTemplateSelector = selector;

                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var presenter = GetHeaderPresenter(tableView, 0);
                Verify.AreEqual(selector, presenter.ContentTemplateSelector, "HeaderTemplateSelector should be applied to the header presenter.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies HeaderTemplateSelector wins over HeaderTemplate when both are set, per TableView.idl.")]
        public void VerifyHeaderTemplateSelectorTakesPrecedenceOverHeaderTemplate()
        {
            // TableView.idl:178 states "Optional header template; HeaderTemplateSelector takes
            // precedence." That is the OPPOSITE of the ContentControl convention, where
            // ContentTemplate wins over ContentTemplateSelector. The deviation is deliberate and is
            // pinned here because it is exactly the kind of thing that gets "fixed" by accident.
            TableView tableView = null;
            TestHeaderTemplateSelector selector = null;

            RunOnUIThread.Execute(() =>
            {
                selector = new TestHeaderTemplateSelector { Template = CreateTextTemplate("Selected") };

                tableView = CreateTableView(headers: new[] { "Name" });
                tableView.Columns[0].HeaderTemplate = CreateTextTemplate("Templated");
                tableView.Columns[0].HeaderTemplateSelector = selector;

                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var presenter = GetHeaderPresenter(tableView, 0);
                Verify.AreEqual(selector, presenter.ContentTemplateSelector, "The selector should be applied when both are set.");
                Verify.IsNull(presenter.ContentTemplate, "HeaderTemplate must not be applied when a selector is present.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a header Style's ContentTemplate or ContentTemplateSelector renders a plain string Header, a selector returning no template keeps the trimming TextBlock, and the column's templates still win.")]
        [TestProperty("data:Setter", "{ContentTemplate, ContentTemplateSelector, NullSelector}")]
        public void VerifyHeaderStyleContentTemplateAppliesToStringHeader()
        {
            var setter = ((string)TestContext.DataRow["Setter"]).Trim();
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                var template = CreateTextTemplate("Styled");
                var style = new Style(typeof(TableViewColumnHeader));
                style.Setters.Add(setter == "ContentTemplate"
                    ? new Setter(ContentControl.ContentTemplateProperty, template)
                    : new Setter(ContentControl.ContentTemplateSelectorProperty,
                        new TestHeaderTemplateSelector { Template = setter == "NullSelector" ? null : template }));

                tableView = CreateTableView(headers: new[] { "Name", "Role", "City" });
                tableView.ColumnHeaderStyle = style;
                tableView.Columns[1].HeaderTemplate = CreateTextTemplate("Column");
                tableView.Columns[2].HeaderTemplateSelector = new TestHeaderTemplateSelector { Template = CreateTextTemplate("Selected") };

                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var header = (ContentControl)GetHeaderCell(tableView, 0);
                if (setter == "NullSelector")
                {
                    var block = header.Content as TextBlock;
                    Verify.IsNotNull(block, "A selector returning no template must keep the header's own TextBlock.");
                    Verify.AreEqual(TextTrimming.CharacterEllipsis, block.TextTrimming, "The header text must still ellipsize.");
                }
                else
                {
                    Verify.AreEqual("Name", header.Content as string, $"{setter}: the string Header must reach the style's template as data.");
                    Verify.IsTrue(FindVisualChildrenByType<TextBlock>(header).Any(t => t.Text == "Styled"),
                        $"{setter}: the header Style's template must render the string Header.");
                }
                Verify.IsTrue(FindVisualChildrenByType<TextBlock>(GetHeaderCell(tableView, 1)).Any(t => t.Text == "Column"),
                    $"{setter}: the column's HeaderTemplate must win over the header Style's.");
                Verify.IsTrue(FindVisualChildrenByType<TextBlock>(GetHeaderCell(tableView, 2)).Any(t => t.Text == "Selected"),
                    $"{setter}: the column's HeaderTemplateSelector must win over the header Style's.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a header Style's selector is asked once, with the string Header and a header container tagged with its column; its template is applied, and a null or throwing result keeps the trimming TextBlock.")]
        [TestProperty("data:Result", "{Template, Null, Throw}")]
        public void VerifyHeaderStyleSelectorProbeFallsBackToTrimmingText()
        {
            var result = ((string)TestContext.DataRow["Result"]).Trim();
            TableView tableView = null;
            RecordingHeaderTemplateSelector selector = null;

            RunOnUIThread.Execute(() =>
            {
                selector = new RecordingHeaderTemplateSelector { Throw = result == "Throw", Template = result == "Template" ? CreateTextTemplate("Styled") : null };
                var style = new Style(typeof(TableViewColumnHeader));
                style.Setters.Add(new Setter(ContentControl.ContentTemplateSelectorProperty, selector));
                tableView = CreateTableView(headers: new[] { "Name" });
                tableView.ColumnHeaderStyle = style;
                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Selector calls: " + string.Join("; ", selector.Calls.Select(call => $"{call.Item?.GetType().Name ?? "null"}:{call.Item}")));
                Verify.AreEqual(1, selector.Calls.Count, $"{result}: the selector must be asked exactly once.");
                Verify.AreEqual("Name", selector.Calls[0].Item as string, "The selector must be asked about the string Header.");
                var container = selector.Calls[0].Container as TableViewColumnHeader;
                Verify.IsNotNull(container, "The selector must receive the header as its container.");
                Verify.IsTrue(ReferenceEquals(tableView.Columns[0], container.Tag), "The container must already be tagged with its column.");

                var header = (ContentControl)GetHeaderCell(tableView, 0);
                if (result == "Template")
                {
                    Verify.AreEqual("Name", header.Content as string, "The string Header must reach the selected template as data.");
                    Verify.IsTrue(FindVisualChildrenByType<TextBlock>(header).Any(t => t.Text == "Styled"), "The selected template must render.");
                    return;
                }
                var block = header.Content as TextBlock;
                Verify.IsNotNull(block, $"{result}: the header must keep its own TextBlock.");
                Verify.AreEqual(TextTrimming.CharacterEllipsis, block.TextTrimming, "The header text must still ellipsize.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a header Style's ContentTemplate and ContentTemplateSelector follow WinUI setter precedence: the last setter in a Style wins, a derived Style wins over BasedOn, and a winning {x:Null} keeps the trimming TextBlock.")]
        [TestProperty("data:Setters", "{NullSelectorLast, NullTemplateLast, DerivedNullSelector, SelectorLast}")]
        public void VerifyHeaderStyleContentSettersUseLastMatch()
        {
            var setters = ((string)TestContext.DataRow["Setters"]).Trim();
            TableView tableView = null;
            RecordingHeaderTemplateSelector selector = null;

            RunOnUIThread.Execute(() =>
            {
                selector = new RecordingHeaderTemplateSelector { Template = CreateTextTemplate("Styled") };
                var style = new Style(typeof(TableViewColumnHeader));
                switch (setters)
                {
                    case "NullSelectorLast":
                        style.Setters.Add(new Setter(ContentControl.ContentTemplateSelectorProperty, selector));
                        style.Setters.Add(NullSetter("ContentTemplateSelector"));
                        break;
                    case "NullTemplateLast":
                        style.Setters.Add(new Setter(ContentControl.ContentTemplateProperty, CreateTextTemplate("Styled")));
                        style.Setters.Add(NullSetter("ContentTemplate"));
                        break;
                    case "DerivedNullSelector":
                        var basedOn = new Style(typeof(TableViewColumnHeader));
                        basedOn.Setters.Add(new Setter(ContentControl.ContentTemplateSelectorProperty, selector));
                        style.BasedOn = basedOn;
                        style.Setters.Add(NullSetter("ContentTemplateSelector"));
                        break;
                    case "SelectorLast":
                        style.Setters.Add(NullSetter("ContentTemplateSelector"));
                        style.Setters.Add(new Setter(ContentControl.ContentTemplateSelectorProperty, selector));
                        break;
                }
                tableView = CreateTableView(headers: new[] { "Name" });
                tableView.ColumnHeaderStyle = style;
                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var header = (ContentControl)GetHeaderCell(tableView, 0);
                if (setters == "SelectorLast")
                {
                    Verify.AreEqual(1, selector.Calls.Count, "The last selector setter must be asked once.");
                    Verify.IsTrue(FindVisualChildrenByType<TextBlock>(header).Any(t => t.Text == "Styled"), "The last selector's template must render.");
                    return;
                }
                Verify.AreEqual(0, selector.Calls.Count, $"{setters}: a selector a later setter replaced must not be asked.");
                var block = header.Content as TextBlock;
                Verify.IsNotNull(block, $"{setters}: a winning null setter must keep the header's own TextBlock.");
                Verify.AreEqual((TextTrimming?)TextTrimming.CharacterEllipsis, block?.TextTrimming, "The header text must still ellipsize.");
            });
        }

        // A {x:Null} setter as XAML builds it.
        private static Setter NullSetter(string property)
        {
            var style = (Style)XamlReader.Load(
                "<Style xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' TargetType='ContentControl'>" +
                $"<Setter Property='{property}' Value='{{x:Null}}'/></Style>");
            var setter = (Setter)style.Setters[0];
            style.Setters.RemoveAt(0);
            return setter;
        }

        [TestMethod]
        [TestProperty("Description", "Verifies replacing HeaderTemplate on a loaded column rebuilds the header.")]
        public void VerifyHeaderTemplateChangeAfterLoadUpdatesLive()
        {
            TableView tableView = null;
            DataTemplate replacement = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateTableView(headers: new[] { "Name" });
                tableView.Columns[0].HeaderTemplate = CreateTextTemplate("First");

                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                replacement = CreateTextTemplate("Second");
                tableView.Columns[0].HeaderTemplate = replacement;
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(replacement, GetHeaderPresenter(tableView, 0).ContentTemplate,
                    "Replacing HeaderTemplate after load should rebuild the header with the new template.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies replacing HeaderTemplateSelector on a loaded column rebuilds the header.")]
        public void VerifyHeaderTemplateSelectorChangeAfterLoadUpdatesLive()
        {
            // Kept separate from the HeaderTemplate test above: these are different property-changed
            // callbacks, so a regression can plausibly break one and not the other.
            TableView tableView = null;
            TestHeaderTemplateSelector replacement = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateTableView(headers: new[] { "Name" });
                tableView.Columns[0].HeaderTemplateSelector = new TestHeaderTemplateSelector { Template = CreateTextTemplate("First") };

                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                replacement = new TestHeaderTemplateSelector { Template = CreateTextTemplate("Second") };
                tableView.Columns[0].HeaderTemplateSelector = replacement;
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(replacement, GetHeaderPresenter(tableView, 0).ContentTemplateSelector,
                    "Replacing HeaderTemplateSelector after load should rebuild the header and re-run selection.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies HeadersVisibility.Column renders a visible header row with non-zero height.")]
        public void VerifyHeadersVisibilityColumnShowsHeaderRow()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateTableView(headers: new[] { "Name", "Role" });
                Verify.AreEqual(TableViewHeadersVisibility.Column, tableView.HeadersVisibility, "Column is the documented default.");

                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var headerRow = GetHeaderRow(tableView);
                Verify.AreEqual(Visibility.Visible, headerRow.Visibility);
                Verify.IsGreaterThan(headerRow.ActualHeight, 0.0, "A visible header row should take up height.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies HeadersVisibility.None collapses the header row and reclaims its height.")]
        public void VerifyHeadersVisibilityNoneCollapsesHeaderRow()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateTableView(headers: new[] { "Name", "Role" });
                tableView.HeadersVisibility = TableViewHeadersVisibility.None;

                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var headerRow = GetHeaderRow(tableView);

                // Collapsed rather than Hidden is the whole point of the value: the band must not
                // reserve space behind the rows.
                Verify.AreEqual(Visibility.Collapsed, headerRow.Visibility);
                Verify.AreEqual(0.0, headerRow.ActualHeight, "A collapsed header row must contribute no height.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies toggling HeadersVisibility on a loaded control updates the header row both ways.")]
        public void VerifyHeadersVisibilityChangeAfterLoadUpdatesLive()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateTableView(headers: new[] { "Name", "Role" });
                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                tableView.HeadersVisibility = TableViewHeadersVisibility.None;
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(Visibility.Collapsed, GetHeaderRow(tableView).Visibility, "Switching to None should collapse the header row.");

                tableView.HeadersVisibility = TableViewHeadersVisibility.Column;
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var headerRow = GetHeaderRow(tableView);
                Verify.AreEqual(Visibility.Visible, headerRow.Visibility, "Switching back to Column should show the header row.");
                Verify.IsGreaterThan(headerRow.ActualHeight, 0.0, "The restored header row should take up height again.");
            });
        }

        private static FrameworkElement GetHeaderRow(TableView tableView)
        {
            var headerRow = tableView.FindVisualChildByName("PART_HeaderRow");
            Verify.IsNotNull(headerRow, "PART_HeaderRow should exist once the template has applied.");
            return headerRow;
        }

        private static string GetRenderedHeaderText(TableView tableView, int index)
        {
            var textBlock = GetHeaderPresenter(tableView, index).Content as TextBlock;
            Verify.IsNotNull(textBlock, $"String header {index} should render through a TextBlock.");
            return textBlock.Text;
        }

        private sealed class TestHeaderTemplateSelector : DataTemplateSelector
        {
            public DataTemplate Template { get; set; }

            protected override DataTemplate SelectTemplateCore(object item) => Template;

            protected override DataTemplate SelectTemplateCore(object item, DependencyObject container) => Template;
        }

        private sealed class RecordingHeaderTemplateSelector : DataTemplateSelector
        {
            public bool Throw { get; set; }

            public DataTemplate Template { get; set; }

            public List<(object Item, DependencyObject Container)> Calls { get; } = new List<(object, DependencyObject)>();

            protected override DataTemplate SelectTemplateCore(object item) => SelectTemplateCore(item, null);

            protected override DataTemplate SelectTemplateCore(object item, DependencyObject container)
            {
                Calls.Add((item, container));
                if (Throw)
                {
                    throw new InvalidOperationException("Header selector fault");
                }
                return Template;
            }
        }
    }

    // Category 3.5 of the TableView test plan: column visibility.
    [TestClass]
    public class TableViewColumnVisibilityTests : TableViewApiTestBase
    {
        [TestMethod]
        [TestProperty("Description", "Verifies a collapsed column contributes no width to the header or to rows.")]
        public void VerifyCollapsedColumnRemovesCellsFromLayout()
        {
            TableView tableView = null;
            double headerHostWidthBefore = 0.0;
            double collapsedColumnWidth = 0.0;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateTableView(headers: new[] { "Name", "Role", "City" });
                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsGreaterThan(GetHeaderCellWidth(tableView, 1), 0.0, "The column should occupy width before it is collapsed.");

                headerHostWidthBefore = GetHeaderHost(tableView).ActualWidth;
                collapsedColumnWidth = tableView.Columns[1].ActualWidth;

                tableView.Columns[1].Visibility = Visibility.Collapsed;
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                VerifyCellTakesNoWidth(GetHeaderHost(tableView), 1, headerHostWidthBefore, collapsedColumnWidth, "header");

                foreach (var row in GetRealizedRows(tableView))
                {
                    // Every row's cells host is the same TableViewCellsPanel type as the header host, so
                    // the collapsed column has to drop out of its track the same way. The row's own
                    // pre-collapse width is not recorded because a row is arranged to the same total as
                    // the header; comparing against the header's total keeps one source of truth.
                    VerifyCellTakesNoWidth(GetCellsHost(row), 1, headerHostWidthBefore, collapsedColumnWidth, "row");
                }

                Verify.AreEqual(3, tableView.Columns.Count, "Collapsing must not remove the column from Columns.");
                Verify.IsGreaterThan(GetHeaderCellWidth(tableView, 2), 0.0, "The following column should still occupy width.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a collapsed column keeps its authored state while hidden.")]
        public void VerifyCollapsedColumnPreservesColumnState()
        {
            TableView tableView = null;
            TableViewColumn column = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateTableView(headers: new[] { "Name", "Role" });
                tableView.Columns[1].Width = new GridLength(180.0, GridUnitType.Pixel);
                tableView.Columns[1].MinWidth = 40.0;

                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                column = tableView.Columns[1];
                column.Visibility = Visibility.Collapsed;
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                // Collapsing must be a presentation change, not a disguised remove: the authored
                // intent has to survive so showing the column again restores the user's sizing.
                Verify.AreEqual(new GridLength(180.0, GridUnitType.Pixel), column.Width, "Authored Width should survive collapse.");
                Verify.AreEqual(40.0, column.MinWidth, "MinWidth should survive collapse.");
                Verify.AreEqual("Role", column.Header as string, "Header should survive collapse.");
                Verify.AreEqual(column, tableView.Columns[1], "The column should stay at its index while collapsed.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies restoring Visibility brings a collapsed column's header and cells back.")]
        public void VerifyRestoringColumnVisibilityRestoresCells()
        {
            // Kept separate from the collapse test: hide and show are different transitions and a
            // regression can break only one of them.
            TableView tableView = null;
            double widthBeforeCollapse = 0.0;
            double headerHostWidthBefore = 0.0;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateTableView(headers: new[] { "Name", "Role", "City" });
                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                widthBeforeCollapse = GetHeaderCellWidth(tableView, 1);
                headerHostWidthBefore = GetHeaderHost(tableView).ActualWidth;

                tableView.Columns[1].Visibility = Visibility.Collapsed;
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                VerifyCellTakesNoWidth(GetHeaderHost(tableView), 1, headerHostWidthBefore, widthBeforeCollapse, "header");

                tableView.Columns[1].Visibility = Visibility.Visible;
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(widthBeforeCollapse, GetHeaderCellWidth(tableView, 1),
                    "Restoring Visibility should restore the column's previous width.");
                Verify.AreEqual(headerHostWidthBefore, GetHeaderHost(tableView).ActualWidth,
                    "Restoring Visibility should give the header host its full width back.");

                foreach (var row in GetRealizedRows(tableView))
                {
                    Verify.IsGreaterThan(GetCellWidth(row, 1), 0.0, "Restored cells should occupy width again.");
                }

                VerifyEveryRowMatchesHeaders(tableView, "after restoring column visibility");
            });
        }

        // A collapsed column is asserted through the panel's arranged total and its neighbour's offset,
        // never through the collapsed element's own ActualWidth.
        //
        // ActualWidth is RenderSize, and XAML does not run a layout pass on a Collapsed element, so the
        // element keeps the width it last rendered at forever. Reading it reports the pre-collapse width
        // and looks like the control reserved a blank track, which it does not: TableViewCellsPanel
        // measures a non-visible column's cell at zero width (TableViewCellsPanel.cpp:156) and arranges
        // it at w = 0, accumulating only visible widths into the total it returns
        // (TableViewCellsPanel.cpp:264-272). Those two effects are what this checks.
        private static void VerifyCellTakesNoWidth(Panel host, int index, double hostWidthBefore, double columnWidth, string context)
        {
            var cell = (FrameworkElement)host.Children[index];

            Verify.AreEqual(Visibility.Collapsed, cell.Visibility,
                $"A collapsed column's {context} cell should itself be collapsed.");
            Verify.AreEqual(0.0, cell.DesiredSize.Width,
                $"A collapsed column's {context} cell must desire no width.");

            Verify.AreEqual(hostWidthBefore - columnWidth, host.ActualWidth,
                $"The {context} host must give back exactly the collapsed column's width.");

            var next = (FrameworkElement)host.Children[index + 1];
            Verify.AreEqual(cell.ActualOffset.X, next.ActualOffset.X,
                $"The following {context} cell must close up into the collapsed column's slot, leaving no gap.");
        }

        private static double GetHeaderCellWidth(TableView tableView, int index)
            => ((FrameworkElement)GetHeaderHost(tableView).Children[index]).ActualWidth;

        private static double GetCellWidth(TableViewRow row, int index)
            => ((FrameworkElement)GetCellsHost(row).Children[index]).ActualWidth;
    }

    // Category 3.6 of the TableView test plan: the cell element factory and custom columns.
    //
    // TableView.idl:198-201 states the contract these tests pin: the element returned by
    // GenerateElementCore "must bind reactively to its inherited DataContext (the row data item) and
    // must not set a local DataContext: rows are recycled and their DataContext is updated in place
    // ... baking it in as static content (or a local DataContext) will show stale data after
    // recycle."
    [TestClass]
    public class TableViewCellFactoryTests : TableViewApiTestBase
    {
        [TestMethod]
        [TestProperty("Description", "Verifies TableViewTextColumn.GenerateElement returns a TextBlock.")]
        public void VerifyTextColumnGenerateElementProducesTextBlock()
        {
            RunOnUIThread.Execute(() =>
            {
                var column = new TableViewTextColumn { Header = "Name", Binding = CreateBinding("Name") };
                var element = column.GenerateElement(new Person { Name = "Asha", Role = "Designer" });

                Verify.IsNotNull(element, "GenerateElement should produce a cell element.");
                Verify.IsTrue(element is TextBlock, $"The built-in text cell should be a TextBlock, was {element.GetType().Name}.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies GenerateElement returns a distinct element on every call.")]
        public void VerifyGenerateElementReturnsNewElementPerCall()
        {
            RunOnUIThread.Execute(() =>
            {
                var item = new Person { Name = "Asha", Role = "Designer" };
                var column = new TableViewTextColumn { Header = "Name", Binding = CreateBinding("Name") };

                var first = column.GenerateElement(item);
                var second = column.GenerateElement(item);

                // A FrameworkElement cannot have two parents. If a column cached one element, rows
                // would steal it from each other as they realize.
                Verify.IsFalse(ReferenceEquals(first, second), "Each call must return a new element.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies built-in columns do not set a local DataContext on generated cells.")]
        public void VerifyGenerateElementDoesNotSetLocalDataContext()
        {
            RunOnUIThread.Execute(() =>
            {
                var item = new Person { Name = "Asha", Role = "Designer" };

                var textElement = new TableViewTextColumn { Binding = CreateBinding("Name") }.GenerateElement(item);
                var templateElement = new TableViewTemplateColumn { CellTemplate = CreateTextTemplate("Cell") }.GenerateElement(item);

                // The IDL imposes this rule on third-party columns; the built-ins are its reference
                // implementation and must follow it too.
                Verify.AreEqual(DependencyProperty.UnsetValue, textElement.ReadLocalValue(FrameworkElement.DataContextProperty),
                    "A text cell must not carry a local DataContext.");
                Verify.AreEqual(DependencyProperty.UnsetValue, templateElement.ReadLocalValue(FrameworkElement.DataContextProperty),
                    "A template cell must not carry a local DataContext.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies TableViewTemplateColumn.GenerateElement carries CellTemplate rather than falling back to text.")]
        public void VerifyTemplateColumnGenerateElementUsesCellTemplate()
        {
            RunOnUIThread.Execute(() =>
            {
                var cellTemplate = CreateTextTemplate("FromTemplate");
                var column = new TableViewTemplateColumn { CellTemplate = cellTemplate };
                var element = column.GenerateElement(new Person { Name = "Asha", Role = "Designer" });

                Verify.IsNotNull(element, "A template column should produce a cell element.");

                // Asserted on the returned element's ContentTemplate, not on inflated content.
                // GenerateElement is a factory: it hands back a presenter carrying the template, and the
                // content is wired later by TableViewRow when the cell is attached to a row. A presenter
                // that is not in a tree and has no Content has nothing to inflate, so looking for a
                // TextBlock here asserts a stage of the pipeline this method is not responsible for.
                // TableViewCellContentTests.VerifyTemplateCellRendersRowItem covers actual inflation.
                var presenter = element as ContentPresenter;
                Verify.IsNotNull(presenter, "A template column should produce a ContentPresenter.");
                Verify.AreEqual(cellTemplate, presenter.ContentTemplate,
                    "The presenter should carry the column's CellTemplate.");

                // The regression the test name is really about: falling back to rendering the item's
                // ToString() in a TextBlock instead of honouring CellTemplate.
                Verify.IsNull(element as TextBlock,
                    "A template column must not fall back to a plain text cell when CellTemplate is set.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies GenerateElement with a null data item does not throw into row realization.")]
        public void VerifyGenerateElementWithNullDataItemDoesNotCrash()
        {
            RunOnUIThread.Execute(() =>
            {
                // Null items are legal in a List<T> of reference types, so this has to survive.
                // The IDL does not say whether the return may be null, so nothing is asserted about
                // the returned value -- only that the call does not throw.
                new TableViewTextColumn { Binding = CreateBinding("Name") }.GenerateElement(null);
                new TableViewTemplateColumn { CellTemplate = CreateTextTemplate("Cell") }.GenerateElement(null);
                new ProbeColumn().GenerateElement(null);
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a C#-derived column's GenerateElementCore override is invoked for realized cells.")]
        public void VerifyCustomColumnGenerateElementCoreIsCalled()
        {
            TableView tableView = null;
            ProbeColumn probe = null;

            RunOnUIThread.Execute(() =>
            {
                probe = new ProbeColumn { Header = "Probe" };
                tableView = CreateTableView(headers: new[] { "Name" });
                tableView.Columns.Add(probe);

                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var rowCount = GetRealizedRows(tableView).Count;
                Verify.IsGreaterThan(rowCount, 0, "At least one row should be realized.");
                Verify.IsGreaterThanOrEqual(probe.GenerateCount, rowCount,
                    "GenerateElementCore should be invoked at least once per realized row.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies the element a custom column returns is what appears in the row's cell.")]
        public void VerifyCustomColumnElementRendersInCell()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateTableView(headers: new[] { "Name" });
                tableView.Columns.Add(new ProbeColumn { Header = "Probe" });

                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var rows = GetRealizedRows(tableView);
                Verify.IsGreaterThan(rows.Count, 0, "At least one row should be realized.");

                foreach (var row in rows)
                {
                    var probeCells = FindVisualChildrenByType<TextBlock>(row)
                        .Where(t => t.Name == ProbeColumn.CellName)
                        .ToList();

                    Verify.AreEqual(1, probeCells.Count, "Each row should host exactly one element from the custom column.");
                }
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies custom cells bound to the inherited DataContext show the new row's data after recycling.")]
        public void VerifyCustomColumnElementUpdatesOnRecycle()
        {
            // This is the contract from TableView.idl:198-201. The failure mode it guards is silent
            // and data-corrupting: cells keep showing a previous row's values after a scroll.
            TableView tableView = null;
            ScrollViewer scroller = null;

            RunOnUIThread.Execute(() =>
            {
                var items = Enumerable.Range(0, 200)
                    .Select(i => new Person { Name = $"Person {i:D3}", Role = $"Role {i:D3}" })
                    .ToList();

                tableView = CreateTableViewShell(items, 500, 300);
                tableView.Columns.Add(new ProbeColumn { Header = "Probe" });

                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                VerifyProbeCellsMatchRowData(tableView, "before scrolling");

                scroller = GetBodyScroller(tableView);

                // Far enough that every row realized above has been recycled.
                scroller.ChangeView(null, 2000.0, null, true);
            });

            // A second layout pass and wait. ChangeView returns before the repeater has finished
            // re-assigning DataContext to the containers it recycles, and a row sampled in that window
            // is parented but not yet bound. Without this settle the test reads a transient state and
            // reports it as a product bug.
            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsGreaterThan(scroller.VerticalOffset, 0.0, "The body should have actually scrolled.");
                VerifyProbeCellsMatchRowData(tableView, "after scrolling");
            });
        }

        // Every realized row's custom cell must show that row's current data item.
        private static void VerifyProbeCellsMatchRowData(TableView tableView, string context)
        {
            var rows = GetRealizedRows(tableView);
            Verify.IsGreaterThan(rows.Count, 0, $"At least one row should be realized ({context}).");

            foreach (var row in rows)
            {
                var item = row.DataContext as Person;
                if (item == null)
                {
                    // Report and move on rather than dereferencing. TAEF's Verify logs and continues, so
                    // falling through to item.Name raises a NullReferenceException that crosses the WinRT
                    // boundary as 0xC000027B - a test-host crash with no managed stack, which hides the
                    // assertion that actually failed and looks like a product assert.
                    Verify.Fail($"A realized row should carry its data item as DataContext ({context}).");
                    continue;
                }

                var cell = FindVisualChildrenByType<TextBlock>(row).FirstOrDefault(t => t.Name == ProbeColumn.CellName);
                if (cell == null)
                {
                    Verify.Fail($"The custom column's cell should be present ({context}).");
                    continue;
                }

                Verify.AreEqual(item.Name, cell.Text, $"The cell should show its own row's data ({context}).");
            }
        }

        private static Binding CreateBinding(string path) => new Binding
        {
            Path = new PropertyPath(path),
            Mode = BindingMode.OneWay,
        };
    }

    // User column reorder: CanUserReorderColumns, CanReorder, ColumnReordering/ColumnReordered and the
    // Ctrl+Shift+Left/Right chord. The chord is the only programmatic route to a user reorder, and the
    // product reads the modifiers from the keyboard state, so the tests inject real key input.
    [TestClass]
    public class TableViewColumnReorderTests : TableViewApiTestBase
    {
        [TestMethod]
        [TestProperty("Description", "Verifies reorder is opt-in, a column opts in by default, and enabling it makes an otherwise inactive header focusable.")]
        public void VerifyReorderDefaultsAndHeaderActionability()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateTableView(headers: new[] { "Name", "Role" });
                tableView.CanUserSortColumns = false;
                tableView.CanUserResizeColumns = false;
                Verify.IsFalse(tableView.CanUserReorderColumns, "CanUserReorderColumns must default to false.");
                Verify.IsTrue(tableView.Columns[0].CanReorder, "CanReorder must default to true.");
                Verify.IsNotNull(TableView.CanUserReorderColumnsProperty);
                Verify.IsNotNull(TableViewColumn.CanReorderProperty);
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsFalse(GetHeaderCell(tableView, 0).Focus(FocusState.Keyboard),
                    "Precondition: a header that cannot sort, resize or reorder is not actionable.");
                tableView.CanUserReorderColumns = true;
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(GetHeaderCell(tableView, 0).Focus(FocusState.Keyboard), "A reorderable header must take keyboard focus.");
                tableView.Columns[0].CanReorder = false;
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(GetHeaderCell(tableView, 1).Focus(FocusState.Keyboard), "The other column is still reorderable.");
                Verify.IsFalse(GetHeaderCell(tableView, 0).Focus(FocusState.Keyboard), "CanReorder=false makes the header inactive again.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies Ctrl+Shift+Right/Left on a focused header moves the column one position, raises ColumnReordering then ColumnReordered with Columns indexes, and keeps focus on the moved header.")]
        public void VerifyCtrlShiftArrowMovesFocusedHeader()
        {
            TableView tableView = null;
            TableViewColumn name = null;
            var events = new List<string>();

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateReorderTable("Name", "Role", "City");
                name = tableView.Columns[0];
                tableView.ColumnReordering += (s, e) =>
                {
                    events.Add($"Reordering:{e.Column.Header}:{e.OldIndex}->{e.NewIndex}:{s.Columns.IndexOf(e.Column)}");
                };
                tableView.ColumnReordered += (s, e) =>
                {
                    events.Add($"Reordered:{e.Column.Header}:{e.OldIndex}->{e.NewIndex}:{s.Columns.IndexOf(e.Column)}");
                };
                LoadContent(tableView);
            });

            if (!InputReachesTable(tableView))
            {
                return;
            }
            FocusHeader(tableView, 0);
            ReorderInput.Send(right: true);

            RunOnUIThread.Execute(() =>
            {
                VerifyOrder(tableView, "Role,Name,City", "after Ctrl+Shift+Right");
                Verify.AreEqual("Reordering:Name:0->1:0|Reordered:Name:0->1:1", string.Join("|", events),
                    "ColumnReordering must come before the move and ColumnReordered after it, with Columns indexes.");
                VerifyHeaderFocused(tableView, name, "after the move");
                VerifyHeaderColumns(tableView, tableView.Columns.ToArray(), "after the move");
                VerifyEveryRowMatchesHeaders(tableView, "after the move");
                events.Clear();
            });

            ReorderInput.Send(right: true);
            ReorderInput.Send(right: true);

            RunOnUIThread.Execute(() =>
            {
                VerifyOrder(tableView, "Role,City,Name", "after two more presses (the last at the end)");
                Verify.AreEqual(2, events.Count, "A press at the end moves nothing and raises nothing.");
                VerifyHeaderFocused(tableView, name, "at the end");
                events.Clear();
            });

            ReorderInput.Send(right: false);

            RunOnUIThread.Execute(() =>
            {
                VerifyOrder(tableView, "Role,Name,City", "after Ctrl+Shift+Left");
                Verify.AreEqual("Reordering:Name:2->1:2|Reordered:Name:2->1:1", string.Join("|", events));
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies canceling ColumnReordering keeps the order and raises no ColumnReordered, and that CanUserReorderColumns=false ignores the chord.")]
        public void VerifyCanceledOrDisabledReorderChangesNothing()
        {
            TableView tableView = null;
            int reordered = 0;
            bool cancel = true;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateReorderTable("Name", "Role", "City");
                tableView.ColumnReordering += (s, e) => e.Cancel = cancel;
                tableView.ColumnReordered += (s, e) => reordered++;
                LoadContent(tableView);
            });

            if (!InputReachesTable(tableView))
            {
                return;
            }
            FocusHeader(tableView, 1);
            ReorderInput.Send(right: true);

            RunOnUIThread.Execute(() =>
            {
                VerifyOrder(tableView, "Name,Role,City", "after a canceled reorder");
                Verify.AreEqual(0, reordered, "A canceled reorder must not raise ColumnReordered.");
                cancel = false;
                tableView.CanUserReorderColumns = false;
                tableView.CanUserSortColumns = true;
            });

            FocusHeader(tableView, 1);
            ReorderInput.Send(right: true);

            RunOnUIThread.Execute(() =>
            {
                VerifyOrder(tableView, "Name,Role,City", "with CanUserReorderColumns=false");
                Verify.AreEqual(0, reordered);
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a column with CanReorder=false cannot be moved, while another column can still move past it.")]
        public void VerifyCanReorderFalseColumnStaysButOthersPass()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateReorderTable("Name", "Role", "City");
                tableView.CanUserSortColumns = true;
                tableView.Columns[0].CanReorder = false;
                LoadContent(tableView);
            });

            if (!InputReachesTable(tableView))
            {
                return;
            }
            FocusHeader(tableView, 0);
            ReorderInput.Send(right: true);

            RunOnUIThread.Execute(() => VerifyOrder(tableView, "Name,Role,City", "after the chord on a CanReorder=false column"));

            FocusHeader(tableView, 1);
            ReorderInput.Send(right: false);

            RunOnUIThread.Execute(() => VerifyOrder(tableView, "Role,Name,City", "after moving another column past it"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a column never crosses the frozen boundary: frozen columns reorder only among themselves.")]
        public void VerifyReorderRespectsFrozenBoundary()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateReorderTable("Name", "Role", "City", "Team");
                tableView.Columns[0].FrozenEdge = TableViewFrozenEdge.Leading;
                tableView.Columns[1].FrozenEdge = TableViewFrozenEdge.Leading;
                LoadContent(tableView);
            });

            if (!InputReachesTable(tableView))
            {
                return;
            }
            FocusHeader(tableView, 2);
            ReorderInput.Send(right: false);
            RunOnUIThread.Execute(() => VerifyOrder(tableView, "Name,Role,City,Team", "a scrolling column cannot enter the frozen band"));

            FocusHeader(tableView, 1);
            ReorderInput.Send(right: true);
            RunOnUIThread.Execute(() => VerifyOrder(tableView, "Name,Role,City,Team", "a frozen column cannot leave the frozen band"));

            ReorderInput.Send(right: false);
            RunOnUIThread.Execute(() =>
            {
                VerifyOrder(tableView, "Role,Name,City,Team", "frozen columns reorder among themselves");
                Verify.AreEqual(TableViewFrozenEdge.Leading, tableView.Columns[0].FrozenEdge);
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies collapsed columns keep their place and are skipped: a move lands before the next visible column.")]
        public void VerifyReorderSkipsCollapsedColumns()
        {
            TableView tableView = null;
            var indexes = new List<string>();

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateReorderTable("Name", "Hidden", "Role", "City");
                tableView.Columns[1].Visibility = Visibility.Collapsed;
                tableView.ColumnReordered += (s, e) => indexes.Add($"{e.OldIndex}->{e.NewIndex}");
                LoadContent(tableView);
            });

            if (!InputReachesTable(tableView))
            {
                return;
            }
            FocusHeader(tableView, 0);
            ReorderInput.Send(right: true);

            RunOnUIThread.Execute(() =>
            {
                VerifyOrder(tableView, "Hidden,Role,Name,City", "after moving Name past Role");
                Verify.AreEqual("0->2", string.Join("|", indexes), "Indexes are Columns indexes, collapsed columns included.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies the chord follows reading order under FlowDirection=RightToLeft.")]
        public void VerifyReorderMirrorsUnderRightToLeft()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateReorderTable("Name", "Role", "City");
                tableView.FlowDirection = FlowDirection.RightToLeft;
                LoadContent(tableView);
            });

            if (!InputReachesTable(tableView))
            {
                return;
            }
            FocusHeader(tableView, 0);
            ReorderInput.Send(right: false);
            RunOnUIThread.Execute(() => VerifyOrder(tableView, "Role,Name,City", "Ctrl+Shift+Left moves later under RTL"));

            ReorderInput.Send(right: true);
            RunOnUIThread.Execute(() => VerifyOrder(tableView, "Name,Role,City", "Ctrl+Shift+Right moves earlier under RTL"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a moved column keeps its sort direction and the row order, raises no Sorting, and that Pixel, Auto and Star widths are unchanged.")]
        public void VerifySortAndWidthsFollowReorderedColumn()
        {
            TableView tableView = null;
            TableViewColumn role = null;
            double[] widths = null;
            List<object> rowsBefore = null;
            int sorting = 0;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateReorderTable("Name", "Role", "City");
                tableView.CanUserSortColumns = true;
                tableView.Columns[0].Width = new GridLength(1, GridUnitType.Star);
                tableView.Columns[1].Width = GridLength.Auto;
                tableView.Columns[2].Width = new GridLength(90);
                role = tableView.Columns[1];
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(tableView.SortByColumn(role, SortDirection.Ascending), "Precondition: Role sorts.");
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                widths = tableView.Columns.Select(c => c.ActualWidth).ToArray();
                rowsBefore = GetProjectedItems(tableView);
                tableView.Sorting += (s, e) => sorting++;
            });

            if (!InputReachesTable(tableView))
            {
                return;
            }
            FocusHeader(tableView, 1);
            ReorderInput.Send(right: false);
            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                VerifyOrder(tableView, "Role,Name,City", "after the move");
                Verify.AreEqual(SortDirection.Ascending, role.SortDirection, "The sort must follow the moved column.");
                Verify.AreEqual(0, sorting, "A reorder must not raise Sorting.");
                Verify.IsTrue(rowsBefore.SequenceEqual(GetProjectedItems(tableView)), "The row order must not change.");
                var byHeader = new[] { "Name", "Role", "City" }.Select(h => tableView.Columns.First(c => (string)c.Header == h).ActualWidth).ToArray();
                for (int i = 0; i < widths.Length; i++)
                {
                    Verify.AreEqual(widths[i], byHeader[i], $"Column {i} must keep its ActualWidth.");
                }
                Verify.AreEqual(GridUnitType.Auto, role.Width.GridUnitType, "A reorder must not rewrite Width.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies the UIA column headers and a cell's GridItem.Column follow a user reorder.")]
        public void VerifyAutomationHeaderOrderFollowsReorder()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateReorderTable("Name", "Role", "City");
                LoadContent(tableView);
            });

            if (!InputReachesTable(tableView))
            {
                return;
            }
            FocusHeader(tableView, 2);
            ReorderInput.Send(right: false);
            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                VerifyOrder(tableView, "Name,City,Role", "after the move");
                var peer = FrameworkElementAutomationPeer.CreatePeerForElement(tableView);
                var table = peer.GetPattern(PatternInterface.Table) as ITableProvider;
                Verify.IsNotNull(table, "The TableView peer must advertise the Table pattern.");
                var probe = new ProviderPeerProbe();
                var names = table.GetColumnHeaders().Select(p => probe.ToPeer(p).GetName()).ToArray();
                Verify.AreEqual("Name,City,Role", string.Join(",", names), "The header items must follow Columns.");

                var row = GetProjectedRow(tableView, 0);
                var cell = GetCellPeer(row, 1);
                var gridItem = cell.GetPattern(PatternInterface.GridItem) as IGridItemProvider;
                Verify.AreEqual(1, gridItem.Column, "The moved City cell must report its new column.");
                Verify.AreEqual("City", GetHeaderColumns(tableView)[1].Header);
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies the open edit is committed before the move is computed, so a CellEditEnding handler that inserts a column does not misplace it.")]
        public void VerifyEditCommitsBeforeReorderTargetIsComputed()
        {
            TableView tableView = null;
            List<EditablePerson> items = null;
            bool veto = true;
            var moves = new List<string>();

            RunOnUIThread.Execute(() =>
            {
                items = MakeEditableItems();
                tableView = CreateEditableTable(items);
                tableView.Columns.Add(MakeTextColumn("Team", bound: false));
                tableView.IsReadOnly = false;
                tableView.CanUserReorderColumns = true;
                tableView.CanUserSortColumns = false;
                tableView.CanUserResizeColumns = false;
                tableView.CellEditEnding += (s, e) =>
                {
                    if (veto)
                    {
                        e.Cancel = true;
                    }
                    else if (s.Columns.Count == 3)
                    {
                        s.Columns.Insert(0, MakeTextColumn("Extra", bound: false));
                    }
                };
                tableView.ColumnReordering += (s, e) => moves.Add($"{e.Column.Header}:{e.OldIndex}->{e.NewIndex}");
                LoadContent(tableView);
            });

            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                CaptureSetValue(GetValueProvider(tableView, items[0], columnIndex: 0), "Zed");
                Verify.IsTrue(tableView.IsEditing, "Precondition: the vetoed commit keeps the edit open.");
            });

            if (!InputReachesTable(tableView))
            {
                return;
            }
            FocusHeader(tableView, 1);
            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(tableView.IsEditing, "Precondition: the edit is still open with the Role header focused.");
                veto = false;
            });

            ReorderInput.Send(right: true);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsFalse(tableView.IsEditing, "The chord must commit the open edit.");
                VerifyOrder(tableView, "Extra,Name,Team,Role", "after the commit inserted Extra");
                Verify.AreEqual("Role:2->3", string.Join("|", moves), "The move must be computed after the commit changed Columns.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a ColumnReordering handler that inserts or removes another column refuses the move and raises no ColumnReordered.")]
        public void VerifyReorderingHandlerChangingColumnsRefusesMove()
        {
            TableView tableView = null;
            string mutation = null;
            int reordered = 0;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateReorderTable("Name", "Role", "City");
                tableView.ColumnReordering += (s, e) =>
                {
                    if (mutation == "insert")
                    {
                        s.Columns.Add(MakeTextColumn("Extra", bound: false));
                    }
                    else if (mutation == "remove")
                    {
                        s.Columns.RemoveAt(s.Columns.Count - 1);
                    }
                    mutation = null;
                };
                tableView.ColumnReordered += (s, e) => reordered++;
                LoadContent(tableView);
            });

            mutation = "insert";
            if (!InputReachesTable(tableView))
            {
                return;
            }
            FocusHeader(tableView, 0);
            ReorderInput.Send(right: true);
            RunOnUIThread.Execute(() =>
            {
                VerifyOrder(tableView, "Name,Role,City,Extra", "after a Reordering handler inserted a column");
                Verify.AreEqual(0, reordered, "The refused move must not raise ColumnReordered.");
            });

            mutation = "remove";
            FocusHeader(tableView, 0);
            ReorderInput.Send(right: true);
            RunOnUIThread.Execute(() =>
            {
                VerifyOrder(tableView, "Name,Role,City", "after a Reordering handler removed a column");
                Verify.AreEqual(0, reordered);
            });

            FocusHeader(tableView, 0);
            ReorderInput.Send(right: true);
            RunOnUIThread.Execute(() =>
            {
                VerifyOrder(tableView, "Role,Name,City", "once the handler leaves Columns alone");
                Verify.AreEqual(1, reordered);
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a reorder keeps the width hold a locked Star column takes during a resize.")]
        public void VerifyLockedStarHoldSurvivesReorder()
        {
            TableView tableView = null;
            double held = 0;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateReorderTable("Name", "Role", "City");
                tableView.CanUserResizeColumns = true;
                tableView.Columns[0].Width = new GridLength(1, GridUnitType.Star);
                tableView.Columns[0].CanResize = false;
                tableView.Columns[1].Width = new GridLength(100);
                tableView.Columns[2].Width = new GridLength(1, GridUnitType.Star);
                LoadContent(tableView);
            });

            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                var gripper = RequireGripper(tableView, 1);
                gripper.BeginDrag();
                gripper.TryDrag(40.0);
                gripper.EndDrag(false);
                tableView.UpdateLayout();
                Verify.IsGreaterThan(tableView.Columns[1].ActualWidth, 130.0, "Precondition: the drag widened Role.");
                held = tableView.Columns[0].ActualWidth;
                Log.Comment($"Widths after the resize: {string.Join(", ", tableView.Columns.Select(c => c.ActualWidth))}.");
            });

            if (!InputReachesTable(tableView))
            {
                return;
            }
            FocusHeader(tableView, 2);
            ReorderInput.Send(right: false);
            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                VerifyOrder(tableView, "Name,City,Role", "after the move");
                Log.Comment($"Widths after the move: {string.Join(", ", tableView.Columns.Select(c => c.ActualWidth))}.");
                Verify.AreEqual(held, tableView.Columns[0].ActualWidth, "The locked Star column must keep its held width across a reorder.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a throwing ColumnReordered handler leaves the move applied and the headers consistent.")]
        public void VerifyThrowingColumnReorderedHandlerKeepsMove()
        {
            TableView tableView = null;
            var faults = new List<string>();
            Microsoft.UI.Xaml.UnhandledExceptionEventHandler onUnhandled = (s, e) =>
            {
                faults.Add(e.Message);
                e.Handled = true;
            };

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateReorderTable("Name", "Role", "City");
                tableView.ColumnReordered += (s, e) => throw new InvalidOperationException("ColumnReordered fault");
                Application.Current.UnhandledException += onUnhandled;
                LoadContent(tableView);
            });

            if (!InputReachesTable(tableView))
            {
                return;
            }
            try
            {
                FocusHeader(tableView, 0);
                ReorderInput.Send(right: true);
                RunOnUIThread.Execute(() =>
                {
                    Log.Comment($"Faults reported: {faults.Count} ({string.Join("; ", faults)}).");
                    VerifyOrder(tableView, "Role,Name,City", "after a move whose ColumnReordered handler threw");
                    VerifyHeaderFocused(tableView, tableView.Columns[1], "after the fault");
                });

                ReorderInput.Send(right: true);
                RunOnUIThread.Execute(() => VerifyOrder(tableView, "Role,City,Name", "a later move still applies"));
            }
            finally
            {
                RunOnUIThread.Execute(() => Application.Current.UnhandledException -= onUnhandled);
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies header tab stops with CanUserReorderColumns=false match a table that never enabled it, and that a header with nowhere to move is not made actionable.")]
        public void VerifyHeaderTabStopsUnchangedWhenReorderIsOff()
        {
            TableView tableView = null;
            string baseline = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateTableView(headers: new[] { "Name", "Role", "City" }, width: 600);
                tableView.CanUserSortColumns = false;
                tableView.CanUserResizeColumns = false;
                tableView.Columns[2].HeaderContextFlyout = new MenuFlyout();
                LoadContent(tableView);
            });

            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                baseline = HeaderTabStops(tableView);
                Verify.AreEqual("False,False,True", baseline, "Precondition: only the header with a context menu is actionable.");
                tableView.Columns[0].CanReorder = false;
                tableView.Columns[0].CanReorder = true;
            });

            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(baseline, HeaderTabStops(tableView), "CanReorder changes nothing while the table disallows reorder.");
                tableView.CanUserReorderColumns = true;
            });

            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual("True,True,True", HeaderTabStops(tableView), "Every header that can move is actionable.");
                tableView.CanUserReorderColumns = false;
            });

            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(baseline, HeaderTabStops(tableView), "Turning reorder off must restore the original tab stops.");
                tableView.CanUserReorderColumns = true;
                tableView.Columns[1].Visibility = Visibility.Collapsed;
                tableView.Columns[2].Visibility = Visibility.Collapsed;
            });

            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                Verify.IsFalse(((Control)GetHeaderCell(tableView, 0)).IsTabStop, "A header with no other visible column has nowhere to move and stays inactive.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a mouse drag shows the ghost and indicator, and that a Columns change mid-drag cancels it so a release without moving changes nothing.")]
        public void VerifyColumnsChangeMidDragCancelsDrag()
        {
            TableView tableView = null;
            int reordered = 0;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateReorderTable("Name", "Role", "City");
                tableView.ColumnReordered += (s, e) => reordered++;
                LoadContent(tableView);
            });

            SettleLayout(tableView);
            var from = HeaderPoint(tableView, 0, 0.5);
            var to = HeaderPoint(tableView, 1, 0.75);
            if (!InputReachesTable(tableView))
            {
                return;
            }
            try
            {
                ReorderInput.PressAndDrag(from, to);
                RunOnUIThread.Execute(() =>
                {
                    tableView.UpdateLayout();
                    Verify.IsNotNull(GetReorderGhost(tableView), "Dragging past the threshold must show the ghost.");
                    Verify.AreEqual(Visibility.Visible, GetReorderIndicator(tableView).Visibility, "The drop indicator must show past Role.");
                    tableView.Columns.Add(MakeTextColumn("Team", bound: false));
                    Verify.IsNull(GetReorderGhost(tableView), "A Columns change must cancel the drag.");
                    Verify.AreEqual(Visibility.Collapsed, GetReorderIndicator(tableView).Visibility, "A Columns change must hide the indicator.");
                });
            }
            finally
            {
                ReorderInput.LeftUp();
            }

            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                VerifyOrder(tableView, "Name,Role,City,Team", "after releasing a canceled drag");
                Verify.AreEqual(0, reordered, "A canceled drag must not move anything.");
                Verify.AreEqual(1.0, GetHeaderCell(tableView, 0).Opacity, "The dragged header's opacity must be restored.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies that under RightToLeft the ghost stays under the pointer unmirrored and the indicator marks the reading-order drop edge.")]
        public void VerifyRightToLeftDragPlacesGhostAndIndicator()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateReorderTable("Name", "Role", "City");
                tableView.FlowDirection = FlowDirection.RightToLeft;
                LoadContent(tableView);
            });

            SettleLayout(tableView);
            var from = HeaderPoint(tableView, 0, 0.5);
            var to = HeaderPoint(tableView, 1, 0.75);
            if (!InputReachesTable(tableView))
            {
                return;
            }
            try
            {
                ReorderInput.PressAndDrag(from, to);
                RunOnUIThread.Execute(() =>
                {
                    tableView.UpdateLayout();
                    var ghost = GetReorderGhost(tableView);
                    var indicator = GetReorderIndicator(tableView);
                    Verify.IsNotNull(ghost, "Dragging past the threshold must show the ghost.");
                    Verify.AreEqual(FlowDirection.LeftToRight, ghost.FlowDirection, "The ghost must not mirror the header snapshot again.");
                    Verify.AreEqual(Visibility.Visible, indicator.Visibility);
                    var overlay = (Panel)VisualTreeHelper.GetParent(indicator);
                    Verify.IsLessThan(overlay.Children.IndexOf(ghost), overlay.Children.IndexOf(indicator),
                        "The indicator must draw above the ghost, which is opaque in High Contrast.");

                    var role = GetHeaderCell(tableView, 1);
                    var roleBounds = VisualBounds(role);
                    var cityBounds = VisualBounds(GetHeaderCell(tableView, 2));
                    var pointer = role.TransformToVisual(null).TransformPoint(new global::Windows.Foundation.Point(role.ActualWidth * 0.75, role.ActualHeight / 2));
                    var ghostBounds = VisualBounds(ghost);
                    var indicatorBounds = VisualBounds(indicator);
                    Log.Comment($"Role {roleBounds}, City {cityBounds}, pointer {pointer}, ghost {ghostBounds}, indicator {indicatorBounds}.");

                    Verify.IsLessThan(Math.Abs(cityBounds.Right - roleBounds.Left), 1.0, "Precondition: under RTL City sits visually left of Role.");
                    Verify.IsTrue(ghostBounds.Left <= pointer.X && pointer.X <= ghostBounds.Right, "The ghost must sit under the pointer.");
                    Verify.IsLessThan(Math.Abs(((indicatorBounds.Left + indicatorBounds.Right) / 2) - roleBounds.Left), 2.0,
                        "Past Role in reading order, the indicator must mark the edge Role shares with City.");
                });
            }
            finally
            {
                ReorderInput.LeftUp();
            }

            SettleLayout(tableView);
            RunOnUIThread.Execute(() => VerifyOrder(tableView, "Role,Name,City", "after the RTL drop"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies losing pointer capture or unloading the table mid-drag cancels the drag and restores the header.")]
        public void VerifyCaptureLossAndUnloadCancelDrag()
        {
            TableView tableView = null;
            int reordered = 0;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateReorderTable("Name", "Role", "City");
                tableView.ColumnReordered += (s, e) => reordered++;
                LoadContent(tableView);
            });

            SettleLayout(tableView);
            if (!InputReachesTable(tableView))
            {
                return;
            }
            try
            {
                ReorderInput.PressAndDrag(HeaderPoint(tableView, 0, 0.5), HeaderPoint(tableView, 1, 0.75));
                RunOnUIThread.Execute(() =>
                {
                    Verify.IsNotNull(GetReorderGhost(tableView), "Precondition: the drag is under way.");
                    GetHeaderCell(tableView, 0).ReleasePointerCaptures();
                    Verify.IsNull(GetReorderGhost(tableView), "Losing capture must cancel the drag.");
                });
            }
            finally
            {
                ReorderInput.LeftUp();
            }

            SettleLayout(tableView);
            try
            {
                ReorderInput.PressAndDrag(HeaderPoint(tableView, 0, 0.5), HeaderPoint(tableView, 1, 0.75));
                RunOnUIThread.Execute(() =>
                {
                    Verify.IsNotNull(GetReorderGhost(tableView), "Precondition: the second drag is under way.");
                    Content = null;
                });
                IdleSynchronizer.Wait();
                RunOnUIThread.Execute(() => Verify.IsNull(GetReorderGhost(tableView), "Unloading must cancel the drag."));
            }
            finally
            {
                ReorderInput.LeftUp();
            }

            RunOnUIThread.Execute(() => LoadContent(tableView));
            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                VerifyOrder(tableView, "Name,Role,City", "after both canceled drags");
                Verify.AreEqual(0, reordered);
                Verify.AreEqual(1.0, GetHeaderCell(tableView, 0).Opacity, "The dragged header's opacity must be restored.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies holding the drag near the trailing edge scrolls the body, moving away stops it, and Esc cancels without header focus.")]
        public void VerifyDragAutoScrollStartsAndStops()
        {
            TableView tableView = null;
            PointInt32 edge = default;
            PointInt32 middle = default;
            double scrolled = 0;
            double stopped = 0;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateTableViewWithColumns(width: 300, columns: new[]
                {
                    ("A", new GridLength(120)), ("B", new GridLength(120)), ("C", new GridLength(120)),
                    ("D", new GridLength(120)), ("E", new GridLength(120)),
                });
                tableView.CanUserReorderColumns = true;
                tableView.CanUserSortColumns = false;
                tableView.CanUserResizeColumns = false;
                LoadContent(tableView);
            });

            SettleLayout(tableView);
            if (!InputReachesTable(tableView))
            {
                return;
            }
            RunOnUIThread.Execute(() =>
            {
                var scroller = GetBodyScroller(tableView);
                var y = GetHeaderCell(tableView, 0).ActualHeight / 2;
                edge = ReorderInput.ToScreen(GetHeaderHost(tableView), scroller.ViewportWidth - 6, y);
                middle = ReorderInput.ToScreen(GetHeaderHost(tableView), scroller.ViewportWidth / 2, y);
            });

            try
            {
                ReorderInput.PressAndDrag(HeaderPoint(tableView, 0, 0.5), edge);
                global::System.Threading.Thread.Sleep(600);
                IdleSynchronizer.Wait();
                RunOnUIThread.Execute(() => scrolled = GetBodyScroller(tableView).HorizontalOffset);
                Verify.IsGreaterThan(scrolled, 0.0, "Holding the pointer near the trailing edge must scroll the body.");

                ReorderInput.MoveTo(middle);
                RunOnUIThread.Execute(() => stopped = GetBodyScroller(tableView).HorizontalOffset);
                global::System.Threading.Thread.Sleep(400);
                IdleSynchronizer.Wait();
                RunOnUIThread.Execute(() =>
                {
                    Verify.AreEqual(stopped, GetBodyScroller(tableView).HorizontalOffset, "Moving away from the edge must stop the scroll.");
                    Verify.IsNotNull(GetReorderGhost(tableView), "Precondition: the drag is still under way.");
                });

                ReorderInput.SendChord(ReorderInput.VkEscape);
                RunOnUIThread.Execute(() => Verify.IsNull(GetReorderGhost(tableView), "Esc must cancel the drag though the header has no focus."));
            }
            finally
            {
                ReorderInput.LeftUp();
            }

            SettleLayout(tableView);
            RunOnUIThread.Execute(() => VerifyOrder(tableView, "A,B,C,D,E", "after the Esc-canceled drag"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a resize gripper drag and a gripper double-click never start a column reorder.")]
        public void VerifyGripperGesturesDoNotStartReorder()
        {
            TableView tableView = null;
            int reordering = 0;
            double before = 0;
            PointInt32 grip = default;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateReorderTable("Name", "Role", "City");
                tableView.CanUserResizeColumns = true;
                tableView.ColumnReordering += (s, e) => reordering++;
                LoadContent(tableView);
            });

            SettleLayout(tableView);
            if (!InputReachesTable(tableView))
            {
                return;
            }
            RunOnUIThread.Execute(() =>
            {
                var gripper = RequireGripper(tableView, 0);
                grip = ReorderInput.ToScreen(gripper, gripper.ActualWidth / 2, gripper.ActualHeight / 2);
                before = tableView.Columns[0].ActualWidth;
            });

            try
            {
                ReorderInput.PressAndDrag(grip, new PointInt32(grip.X + 40, grip.Y));
                RunOnUIThread.Execute(() => Verify.IsNull(GetReorderGhost(tableView), "A gripper drag must not start a reorder."));
            }
            finally
            {
                ReorderInput.LeftUp();
            }

            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                Verify.AreNotEqual(before, tableView.Columns[0].ActualWidth, "Precondition: the gripper drag resized Name.");
                var gripper = RequireGripper(tableView, 0);
                grip = ReorderInput.ToScreen(gripper, gripper.ActualWidth / 2, gripper.ActualHeight / 2);
            });

            ReorderInput.MoveTo(grip);
            ReorderInput.LeftDown();
            ReorderInput.LeftUp();
            ReorderInput.LeftDown();
            ReorderInput.LeftUp();
            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                VerifyOrder(tableView, "Name,Role,City", "after the gripper gestures");
                Verify.AreEqual(0, reordering, "No gripper gesture may raise ColumnReordering.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a horizontal touch drag reorders a table inside a vertical ScrollViewer, and a vertical touch pan on the header still scrolls it.")]
        public void VerifyTouchDragInsideVerticalScrollViewer()
        {
            TableView tableView = null;
            ScrollViewer scroller = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateReorderTable("Name", "Role", "City");
                var panel = new StackPanel();
                panel.Children.Add(new Border { Height = 150 });
                panel.Children.Add(tableView);
                panel.Children.Add(new Border { Height = 1200 });
                scroller = new ScrollViewer { Height = 400, VerticalScrollMode = ScrollMode.Enabled, Content = panel };
                LoadContent(scroller);
            });

            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                var host = GetHeaderHost(tableView);
                Verify.AreEqual(ManipulationModes.System | ManipulationModes.TranslateX, host.ManipulationMode);
                tableView.CanUserReorderColumns = false;
                Verify.AreEqual(DependencyProperty.UnsetValue, host.ReadLocalValue(UIElement.ManipulationModeProperty), "Reorder off must leave the header row as on main.");

                host.ManipulationMode = ManipulationModes.System | ManipulationModes.Scale;
                tableView.CanUserReorderColumns = true;
                Verify.AreEqual(ManipulationModes.System | ManipulationModes.TranslateX, host.ManipulationMode);
                foreach (var column in tableView.Columns)
                {
                    column.CanReorder = false;
                }
                tableView.UpdateLayout();
                Verify.AreEqual(ManipulationModes.System | ManipulationModes.Scale, host.ManipulationMode, "With no movable header the host's own value must come back.");
                foreach (var column in tableView.Columns)
                {
                    column.CanReorder = true;
                }
                tableView.CanUserReorderColumns = false;
                Verify.AreEqual(ManipulationModes.System | ManipulationModes.Scale, host.ManipulationMode, "Unhooking must restore the host's own value.");
                host.ClearValue(UIElement.ManipulationModeProperty);
                tableView.CanUserReorderColumns = true;
            });
            SettleLayout(tableView);

            if (!InputReachesTable(tableView))
            {
                return;
            }
            ReorderInput.TouchDrag(HeaderPoint(tableView, 0, 0.5), HeaderPoint(tableView, 1, 0.75));
            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                VerifyOrder(tableView, "Role,Name,City", "after a horizontal touch drag");
                Verify.AreEqual(0.0, scroller.VerticalOffset, "A horizontal drag must not scroll the page.");
            });

            var from = HeaderPoint(tableView, 1, 0.5);
            ReorderInput.TouchDrag(from, new PointInt32(from.X, from.Y - 120));
            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                Log.Comment($"VerticalOffset after a vertical pan: {scroller.VerticalOffset}.");
                Verify.IsGreaterThan(scroller.VerticalOffset, 0.0, "A vertical touch pan on the header must still scroll the ScrollViewer.");
                VerifyOrder(tableView, "Role,Name,City", "after a vertical touch pan");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a vetoed commit refuses both a keyboard and a dragged move, and Esc during the drag leaves the edit open.")]
        public void VerifyRefusedCommitRefusesReorder()
        {
            TableView tableView = null;
            List<EditablePerson> items = null;
            int reordering = 0;

            RunOnUIThread.Execute(() =>
            {
                items = MakeEditableItems();
                tableView = CreateEditableTable(items);
                tableView.Columns.Add(MakeTextColumn("Team", bound: false));
                tableView.IsReadOnly = false;
                tableView.CanUserReorderColumns = true;
                tableView.CanUserSortColumns = false;
                tableView.CanUserResizeColumns = false;
                tableView.CellEditEnding += (s, e) => e.Cancel = true;
                tableView.ColumnReordering += (s, e) => reordering++;
                LoadContent(tableView);
            });

            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                CaptureSetValue(GetValueProvider(tableView, items[0], columnIndex: 0), "Zed");
                Verify.IsTrue(tableView.IsEditing, "Precondition: the vetoed commit keeps the edit open.");
            });

            if (!InputReachesTable(tableView))
            {
                return;
            }
            FocusHeader(tableView, 1);
            ReorderInput.Send(right: true);
            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(tableView.IsEditing, "A refused commit must keep the edit open.");
                VerifyOrder(tableView, "Name,Role,Team", "after a keyboard move with a refused commit");
            });

            try
            {
                ReorderInput.PressAndDrag(HeaderPoint(tableView, 0, 0.5), HeaderPoint(tableView, 1, 0.75));
                RunOnUIThread.Execute(() =>
                {
                    Verify.IsNotNull(GetReorderGhost(tableView), "Precondition: the drag is under way.");
                    Verify.IsTrue(tableView.IsEditing, "Starting a drag must not touch the edit.");
                });
                ReorderInput.SendChord(ReorderInput.VkEscape);
                RunOnUIThread.Execute(() =>
                {
                    Verify.IsNull(GetReorderGhost(tableView), "Esc must cancel the drag.");
                    Verify.IsTrue(tableView.IsEditing, "Esc during a drag must leave the edit open.");
                });
            }
            finally
            {
                ReorderInput.LeftUp();
            }

            try
            {
                ReorderInput.PressAndDrag(HeaderPoint(tableView, 0, 0.5), HeaderPoint(tableView, 1, 0.75));
            }
            finally
            {
                ReorderInput.LeftUp();
            }
            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(tableView.IsEditing, "A refused commit at the drop must keep the edit open.");
                VerifyOrder(tableView, "Name,Role,Team", "after a drop with a refused commit");
                Verify.AreEqual(0, reordering, "A refused commit must not raise ColumnReordering.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies that with reorder on a touch tap still sorts and a touch press-and-hold still opens the header menu.")]
        public void VerifyTouchTapSortsAndHoldOpensHeaderMenu()
        {
            TableView tableView = null;
            int menuOpened = 0;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateReorderTable("Name", "Role", "City");
                tableView.CanUserSortColumns = true;
                var menu = new MenuFlyout();
                menu.Items.Add(new MenuFlyoutItem { Text = "Hide" });
                menu.Opened += (s, e) => menuOpened++;
                tableView.Columns[1].HeaderContextFlyout = menu;
                LoadContent(tableView);
            });

            SettleLayout(tableView);
            if (!InputReachesTable(tableView))
            {
                return;
            }
            ReorderInput.TouchTap(HeaderPoint(tableView, 0, 0.5));
            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                Verify.AreNotEqual(SortDirection.None, tableView.Columns[0].SortDirection, "A touch tap on a header must still sort it.");
                VerifyOrder(tableView, "Name,Role,City", "after a touch tap");
            });

            ReorderInput.TouchHold(HeaderPoint(tableView, 1, 0.5), 1500);
            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(1, menuOpened, "A touch press-and-hold must still open the header menu.");
                Verify.IsNull(GetReorderGhost(tableView), "A press-and-hold must not start a drag.");
                VerifyOrder(tableView, "Name,Role,City", "after a press-and-hold");
                ((MenuFlyout)tableView.Columns[1].HeaderContextFlyout).Hide();
            });
            SettleLayout(tableView);
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a pen drag with the barrel button pressed does not start a reorder, and the same drag without it does.")]
        public void VerifyPenBarrelDragDoesNotReorder()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateReorderTable("Name", "Role", "City");
                LoadContent(tableView);
            });

            SettleLayout(tableView);
            if (!InputReachesTable(tableView))
            {
                return;
            }
            ReorderInput.PenDrag(HeaderPoint(tableView, 0, 0.5), HeaderPoint(tableView, 1, 0.75), barrel: true);
            SettleLayout(tableView);
            RunOnUIThread.Execute(() => VerifyOrder(tableView, "Name,Role,City", "after a pen drag with the barrel button"));

            ReorderInput.PenDrag(HeaderPoint(tableView, 0, 0.5), HeaderPoint(tableView, 1, 0.75), barrel: false);
            SettleLayout(tableView);
            RunOnUIThread.Execute(() => VerifyOrder(tableView, "Role,Name,City", "after a plain pen drag"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a ContextRequested during a pending press cancels it, so a later move does not start a drag.")]
        public void VerifyContextRequestedCancelsPendingPress()
        {
            TableView tableView = null;
            int contextRequested = 0;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateReorderTable("Name", "Role", "City");
                tableView.AddHandler(UIElement.ContextRequestedEvent, new global::Windows.Foundation.TypedEventHandler<UIElement, ContextRequestedEventArgs>((s, e) => contextRequested++), true);
                LoadContent(tableView);
            });

            if (!InputReachesTable(tableView))
            {
                return;
            }
            FocusHeader(tableView, 0);
            var from = HeaderPoint(tableView, 0, 0.5);
            var to = HeaderPoint(tableView, 1, 0.75);
            try
            {
                ReorderInput.MoveTo(from);
                ReorderInput.LeftDown();
                ReorderInput.SendChord(ReorderInput.VkF10, ReorderInput.VkShift);
                RunOnUIThread.Execute(() => Verify.AreEqual(1, contextRequested, "Precondition: Shift+F10 raised ContextRequested."));
                ReorderInput.MoveTo(new PointInt32((from.X + to.X) / 2, from.Y));
                ReorderInput.MoveTo(to);
                RunOnUIThread.Execute(() => Verify.IsNull(GetReorderGhost(tableView), "A move after ContextRequested must not start a drag."));
            }
            finally
            {
                ReorderInput.LeftUp();
            }

            SettleLayout(tableView);
            RunOnUIThread.Execute(() => VerifyOrder(tableView, "Name,Role,City", "after the canceled press"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a FrozenEdge.Trailing column reorders as a scrolling column and still cannot cross the leading frozen boundary.")]
        public void VerifyTrailingColumnReordersAsScrollingColumn()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateReorderTable("Name", "Role", "City");
                tableView.Columns[0].FrozenEdge = TableViewFrozenEdge.Leading;
                tableView.Columns[2].FrozenEdge = TableViewFrozenEdge.Trailing;
                LoadContent(tableView);
            });

            if (!InputReachesTable(tableView))
            {
                return;
            }
            FocusHeader(tableView, 2);
            ReorderInput.Send(right: false);
            RunOnUIThread.Execute(() => VerifyOrder(tableView, "Name,City,Role", "after moving the Trailing column left"));
            ReorderInput.Send(right: false);
            RunOnUIThread.Execute(() => VerifyOrder(tableView, "Name,City,Role", "a Trailing column must not enter the leading frozen band"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies that with body focus a pointer reorder keeps the column cursor and the focused cell on their column.")]
        public void VerifyPointerReorderKeepsBodyColumnCursor()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateReorderTable("Name", "Role", "City");
                tableView.IsReadOnly = false;
                LoadContent(tableView);
            });

            SettleLayout(tableView);
            RunOnUIThread.Execute(() => GetCellPeer(TableViewRowTestHelpers.RequireFirstRow(tableView), 1).SetFocus());
            SettleLayout(tableView);
            RunOnUIThread.Execute(() => Verify.AreEqual("Role", FocusedCellHeader(tableView), "Precondition: the Role cell has focus."));

            if (!InputReachesTable(tableView))
            {
                return;
            }
            DragHeader(tableView, 0, 1);
            RunOnUIThread.Execute(() =>
            {
                VerifyOrder(tableView, "Role,Name,City", "after dragging Name past Role");
                Verify.AreEqual("Role", FocusedCellHeader(tableView), "Focus must stay on the Role cell.");
            });
            ReorderInput.SendChord(ReorderInput.VkDown);
            RunOnUIThread.Execute(() => Verify.AreEqual("Role", FocusedCellHeader(tableView), "The column cursor must still target Role."));

            RunOnUIThread.Execute(() => GetCellPeer(TableViewRowTestHelpers.RequireFirstRow(tableView), 1).SetFocus());
            SettleLayout(tableView);
            RunOnUIThread.Execute(() => Verify.AreEqual("Name", FocusedCellHeader(tableView), "Precondition: a cell of the column to move has focus."));
            DragHeader(tableView, 1, 2);
            RunOnUIThread.Execute(() =>
            {
                VerifyOrder(tableView, "Role,City,Name", "after dragging Name past City");
                Verify.AreEqual("Name", FocusedCellHeader(tableView), "Focus must follow the moved column's cell.");
            });
            ReorderInput.SendChord(ReorderInput.VkDown);
            RunOnUIThread.Execute(() => Verify.AreEqual("Name", FocusedCellHeader(tableView), "The column cursor must follow the moved column."));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a Columns VectorChanged handler that throws mid-move does not interrupt the move: the column stays in Columns and ColumnReordered is raised.")]
        public void VerifyThrowingColumnsHandlerDoesNotInterruptMove()
        {
            TableView tableView = null;
            bool armed = false;
            int reordered = 0;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateReorderTable("Name", "Role", "City");
                ((IObservableVector<TableViewColumn>)tableView.Columns).VectorChanged += (s, e) =>
                {
                    if (armed && e.CollectionChange == CollectionChange.ItemRemoved)
                    {
                        armed = false;
                        throw new InvalidOperationException("Test handler failure.");
                    }
                };
                tableView.ColumnReordered += (s, e) => reordered++;
                LoadContent(tableView);
            });

            if (!InputReachesTable(tableView))
            {
                return;
            }
            FocusHeader(tableView, 0);
            RunOnUIThread.Execute(() => armed = true);
            ReorderInput.Send(right: true);
            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                Verify.IsFalse(armed, "Precondition: the handler threw.");
                VerifyOrder(tableView, "Role,Name,City", "after a move whose VectorChanged handler threw");
                Verify.AreEqual(1, reordered, "The move must complete and raise ColumnReordered.");
            });
        }
        private static void DragHeader(TableView tableView, int from, int past)
        {
            try
            {
                ReorderInput.PressAndDrag(HeaderPoint(tableView, from, 0.5), HeaderPoint(tableView, past, 0.75));
            }
            finally
            {
                ReorderInput.LeftUp();
            }
            SettleLayout(tableView);
        }

        // The header of the column whose body cell holds keyboard focus. Call on the UI thread.
        private static string FocusedCellHeader(TableView tableView)
        {
            for (var node = FocusManager.GetFocusedElement(tableView.XamlRoot) as DependencyObject; node != null; node = VisualTreeHelper.GetParent(node))
            {
                if (node is FrameworkElement element && element.Tag is TableViewColumn column && VisualTreeHelper.GetParent(element) is Panel panel && panel != GetHeaderHost(tableView))
                {
                    return column.Header as string;
                }
            }
            return null;
        }
        // These tests drive real input. Some CI agents never deliver synthetic input to the test window; there the
        // tests report Skipped instead of failing on input that did not arrive.
        private static bool InputReachesTable(TableView tableView)
        {
            int moves = 0;
            var handler = new PointerEventHandler((s, e) => moves++);
            RunOnUIThread.Execute(() => tableView.AddHandler(UIElement.PointerMovedEvent, handler, true));
            var point = HeaderPoint(tableView, 0, 0.5);
            ReorderInput.EnsureForeground(point);
            ReorderInput.MoveTo(new PointInt32(point.X + 4, point.Y));
            ReorderInput.MoveTo(point);
            int seen = 0;
            RunOnUIThread.Execute(() =>
            {
                tableView.RemoveHandler(UIElement.PointerMovedEvent, handler);
                seen = moves;
            });
            if (seen == 0)
            {
                Log.Result(TestResult.Skipped, "Synthetic input does not reach the test window on this machine.");
                return false;
            }
            return true;
        }
        private static PointInt32 HeaderPoint(TableView tableView, int index, double fraction)
        {
            PointInt32 point = default;
            RunOnUIThread.Execute(() =>
            {
                var cell = GetHeaderCell(tableView, index);
                point = ReorderInput.ToScreen(cell, cell.ActualWidth * fraction, cell.ActualHeight / 2);
            });
            return point;
        }

        private static Border GetReorderGhost(TableView tableView)
            => (tableView.FindVisualChildByName("PART_ColumnReorderOverlay") as Canvas)?.Children.OfType<Border>().FirstOrDefault();

        private static Rectangle GetReorderIndicator(TableView tableView)
            => tableView.FindVisualChildByName("PART_ColumnReorderIndicator") as Rectangle;

        private static global::Windows.Foundation.Rect VisualBounds(FrameworkElement element)
            => element.TransformToVisual(null).TransformBounds(new global::Windows.Foundation.Rect(0, 0, element.ActualWidth, element.ActualHeight));

        private static string HeaderTabStops(TableView tableView)
            => string.Join(",", GetHeaderHost(tableView).Children.OfType<Control>().Select(header => header.IsTabStop));

        private sealed class ProviderPeerProbe : FrameworkElementAutomationPeer
        {
            public ProviderPeerProbe() : base(new Border()) { }

            public AutomationPeer ToPeer(IRawElementProviderSimple provider) => PeerFromProvider(provider);
        }

        private static TableView CreateReorderTable(params string[] headers)
        {
            var tableView = CreateTableView(headers: headers, width: 600);
            tableView.CanUserReorderColumns = true;
            tableView.CanUserSortColumns = false;
            tableView.CanUserResizeColumns = false;
            return tableView;
        }

        private static void FocusHeader(TableView tableView, int index)
        {
            SettleLayout(tableView);
            ReorderInput.EnsureForeground(HeaderPoint(tableView, index, 0.5));
            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(GetHeaderCell(tableView, index).Focus(FocusState.Keyboard), $"Precondition: header {index} takes keyboard focus.");
            });
            IdleSynchronizer.Wait();
        }

        private static void VerifyOrder(TableView tableView, string expected, string context)
        {
            Verify.AreEqual(expected, string.Join(",", tableView.Columns.Select(c => (string)c.Header)), $"Columns order {context}.");
            Verify.AreEqual(expected, string.Join(",", GetHeaderColumns(tableView).Select(c => (string)c.Header)), $"Header order {context}.");
        }

        private static void VerifyHeaderFocused(TableView tableView, TableViewColumn column, string context)
        {
            var focused = FocusManager.GetFocusedElement(tableView.XamlRoot) as FrameworkElement;
            Verify.IsNotNull(focused, $"Focus must stay in the header band {context}.");
            Verify.IsTrue(ReferenceEquals(column, focused.Tag) && ReferenceEquals(GetHeaderHost(tableView), VisualTreeHelper.GetParent(focused)),
                $"The moved column's header must hold focus {context}.");
        }
    }

    // Real input through SendInput, so the product's keyboard-state reads and pointer routing see it.
    // The test window has to be in the foreground.
    internal static class ReorderInput
    {
        internal const ushort VkShift = 0x10;
        internal const ushort VkControl = 0x11;
        internal const ushort VkMenu = 0x12;
        internal const ushort VkEscape = 0x1B;
        internal const ushort VkLeft = 0x25;
        internal const ushort VkRight = 0x27;
        internal const ushort VkDown = 0x28;
        internal const ushort VkF10 = 0x79;

        private const uint InputMouse = 0;
        private const uint InputKeyboard = 1;
        private const uint KeyEventExtendedKey = 0x1;
        private const uint KeyEventKeyUp = 0x2;
        private const uint MouseEventMove = 0x1;
        private const uint MouseEventLeftDown = 0x2;
        private const uint MouseEventLeftUp = 0x4;
        private const uint MouseEventVirtualDesk = 0x4000;
        private const uint MouseEventAbsolute = 0x8000;
        private const int SmXVirtualScreen = 76;
        private const int SmYVirtualScreen = 77;
        private const int SmCxVirtualScreen = 78;
        private const int SmCyVirtualScreen = 79;

        [StructLayout(LayoutKind.Sequential)]
        private struct KeyboardInput
        {
            public ushort Vk;
            public ushort Scan;
            public uint Flags;
            public uint Time;
            public IntPtr ExtraInfo;
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct MouseInput
        {
            public int Dx;
            public int Dy;
            public uint MouseData;
            public uint Flags;
            public uint Time;
            public IntPtr ExtraInfo;
        }

        [StructLayout(LayoutKind.Explicit)]
        private struct InputUnion
        {
            [FieldOffset(0)] public MouseInput Mouse;
            [FieldOffset(0)] public KeyboardInput Keyboard;
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct NativeInput
        {
            public uint Type;
            public InputUnion Data;
        }

        [DllImport("user32.dll", SetLastError = true)]
        private static extern uint SendInput(uint count, NativeInput[] inputs, int size);

        [DllImport("user32.dll")]
        private static extern int GetSystemMetrics(int index);

        private static NativeInput Key(ushort vk, bool up) => new NativeInput
        {
            Type = InputKeyboard,
            Data = new InputUnion
            {
                Keyboard = new KeyboardInput
                {
                    Vk = vk,
                    Flags = (up ? KeyEventKeyUp : 0) | (vk == VkLeft || vk == VkRight || vk == VkDown ? KeyEventExtendedKey : 0),
                },
            },
        };

        private static NativeInput Mouse(uint flags, int dx = 0, int dy = 0) => new NativeInput
        {
            Type = InputMouse,
            Data = new InputUnion { Mouse = new MouseInput { Dx = dx, Dy = dy, Flags = flags } },
        };

        private static void SendAll(params NativeInput[] inputs)
        {
            Verify.AreEqual((uint)inputs.Length, SendInput((uint)inputs.Length, inputs, Marshal.SizeOf<NativeInput>()),
                "SendInput must inject all of the input; the test window has to be in the foreground.");
            IdleSynchronizer.Wait();
        }

        [DllImport("user32.dll")]
        private static extern IntPtr WindowFromPoint(PointInt32 point);

        [DllImport("user32.dll")]
        private static extern IntPtr GetAncestor(IntPtr window, uint flags);

        [DllImport("user32.dll")]
        private static extern IntPtr GetForegroundWindow();

        [DllImport("user32.dll")]
        private static extern bool SetForegroundWindow(IntPtr window);

        [DllImport("user32.dll")]
        private static extern uint GetWindowThreadProcessId(IntPtr window, out uint processId);

        // SendInput keys go to the foreground window, which the test window need not be on a CI agent.
        internal static void EnsureForeground(PointInt32 point)
        {
            IntPtr window = GetAncestor(WindowFromPoint(point), 2 /* GA_ROOT */);
            GetWindowThreadProcessId(window, out uint processId);
            if (window == IntPtr.Zero || processId != (uint)global::System.Environment.ProcessId || GetForegroundWindow() == window)
            {
                return;
            }
            // A synthesized key press lets this process take the foreground.
            SendAll(Key(VkMenu, false), Key(VkMenu, true));
            SetForegroundWindow(window);
            IdleSynchronizer.Wait();
            Log.Comment($"Test window brought to the foreground: {GetForegroundWindow() == window}.");
        }

        // Ctrl+Shift+Left/Right.
        internal static void Send(bool right) => SendChord(right ? VkRight : VkLeft, VkControl, VkShift);

        internal static void SendChord(ushort key, params ushort[] modifiers)
        {
            var inputs = new List<NativeInput>();
            inputs.AddRange(modifiers.Select(modifier => Key(modifier, false)));
            inputs.Add(Key(key, false));
            inputs.Add(Key(key, true));
            inputs.AddRange(Enumerable.Reverse(modifiers).Select(modifier => Key(modifier, true)));
            SendAll(inputs.ToArray());
        }

        // A point in element coordinates as a screen pixel. Call on the UI thread.
        internal static PointInt32 ToScreen(UIElement element, double x, double y)
            => element.XamlRoot.CoordinateConverter.ConvertLocalToScreen(
                element.TransformToVisual(null).TransformPoint(new global::Windows.Foundation.Point(x, y)));

        internal static void MoveTo(PointInt32 point)
        {
            int left = GetSystemMetrics(SmXVirtualScreen);
            int top = GetSystemMetrics(SmYVirtualScreen);
            int width = Math.Max(2, GetSystemMetrics(SmCxVirtualScreen));
            int height = Math.Max(2, GetSystemMetrics(SmCyVirtualScreen));
            SendAll(Mouse(MouseEventMove | MouseEventAbsolute | MouseEventVirtualDesk,
                (int)((point.X - left) * 65535L / (width - 1)),
                (int)((point.Y - top) * 65535L / (height - 1))));
        }

        internal static void LeftDown() => SendAll(Mouse(MouseEventLeftDown));

        internal static void LeftUp() => SendAll(Mouse(MouseEventLeftUp));

        // Presses at from and moves to to in steps, leaving the button down.
        internal static void PressAndDrag(PointInt32 from, PointInt32 to, int steps = 4)
        {
            EnsureForeground(from);
            MoveTo(from);
            LeftDown();
            for (int step = 1; step <= steps; step++)
            {
                MoveTo(new PointInt32(from.X + ((to.X - from.X) * step / steps), from.Y + ((to.Y - from.Y) * step / steps)));
            }
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct PointerInfo
        {
            public int Type;
            public uint Id;
            public uint FrameId;
            public uint Flags;
            public IntPtr Device;
            public IntPtr Hwnd;
            public PointInt32 Pixel;
            public PointInt32 Himetric;
            public PointInt32 PixelRaw;
            public PointInt32 HimetricRaw;
            public uint Time;
            public uint HistoryCount;
            public int InputData;
            public uint KeyStates;
            public ulong PerformanceCount;
            public int ButtonChangeType;
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct TouchInfo
        {
            public PointerInfo Pointer;
            public uint TouchFlags;
            public uint Mask;
            public RectInt32Ltrb Contact;
            public RectInt32Ltrb ContactRaw;
            public uint Orientation;
            public uint Pressure;
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct RectInt32Ltrb
        {
            public int Left;
            public int Top;
            public int Right;
            public int Bottom;
        }

        [DllImport("user32.dll", SetLastError = true)]
        private static extern bool InitializeTouchInjection(uint maxCount, uint feedbackMode);

        [DllImport("user32.dll", SetLastError = true)]
        private static extern bool InjectTouchInput(uint count, TouchInfo[] contacts);

        private const uint PointerFlagInRange = 0x2;
        private const uint PointerFlagInContact = 0x4;
        private const uint PointerFlagDown = 0x10000;
        private const uint PointerFlagUpdate = 0x20000;
        private const uint PointerFlagUp = 0x40000;
        private static bool s_touchInitialized;

        private static void Touch(PointInt32 point, uint flags)
        {
            if (!s_touchInitialized)
            {
                Verify.IsTrue(InitializeTouchInjection(1, 1 /* TOUCH_FEEDBACK_DEFAULT */), $"InitializeTouchInjection failed: {Marshal.GetLastWin32Error()}.");
                s_touchInitialized = true;
            }
            var contact = new TouchInfo
            {
                Pointer = new PointerInfo { Type = 2 /* PT_TOUCH */, Flags = flags, Pixel = point },
                Mask = 0x7 /* contact, orientation, pressure */,
                Contact = new RectInt32Ltrb { Left = point.X - 2, Top = point.Y - 2, Right = point.X + 2, Bottom = point.Y + 2 },
                Orientation = 90,
                Pressure = 32000,
            };
            Verify.IsTrue(InjectTouchInput(1, new[] { contact }), $"InjectTouchInput failed: {Marshal.GetLastWin32Error()}.");
        }

        // A touch press at from, a straight drag to to, and a lift. Updates stay frequent so the contact doesn't time out.
        internal static void TouchDrag(PointInt32 from, PointInt32 to, int steps = 12)
        {
            EnsureForeground(from);
            Touch(from, PointerFlagDown | PointerFlagInRange | PointerFlagInContact);
            try
            {
                for (int step = 1; step <= steps; step++)
                {
                    global::System.Threading.Thread.Sleep(16);
                    Touch(new PointInt32(from.X + ((to.X - from.X) * step / steps), from.Y + ((to.Y - from.Y) * step / steps)),
                        PointerFlagUpdate | PointerFlagInRange | PointerFlagInContact);
                }
            }
            finally
            {
                Touch(to, PointerFlagUp);
            }
            IdleSynchronizer.Wait();
        }

        internal static void TouchTap(PointInt32 point)
        {
            EnsureForeground(point);
            Touch(point, PointerFlagDown | PointerFlagInRange | PointerFlagInContact);
            global::System.Threading.Thread.Sleep(50);
            Touch(point, PointerFlagUp);
            IdleSynchronizer.Wait();
        }

        // A still contact for holdMs, refreshed so it doesn't time out, then a lift.
        internal static void TouchHold(PointInt32 point, int holdMs)
        {
            EnsureForeground(point);
            Touch(point, PointerFlagDown | PointerFlagInRange | PointerFlagInContact);
            try
            {
                for (int elapsed = 0; elapsed < holdMs; elapsed += 50)
                {
                    global::System.Threading.Thread.Sleep(50);
                    Touch(point, PointerFlagUpdate | PointerFlagInRange | PointerFlagInContact);
                }
            }
            finally
            {
                Touch(point, PointerFlagUp);
            }
            IdleSynchronizer.Wait();
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct PenInfo
        {
            public PointerInfo Pointer;
            public uint PenFlags;
            public uint PenMask;
            public uint Pressure;
            public uint Rotation;
            public int TiltX;
            public int TiltY;
        }

        [StructLayout(LayoutKind.Explicit)]
        private struct PointerTypeInfo
        {
            [FieldOffset(0)] public int Type;
            [FieldOffset(8)] public PenInfo Pen;
            [FieldOffset(8)] public TouchInfo Touch;
        }

        [DllImport("user32.dll", SetLastError = true)]
        private static extern IntPtr CreateSyntheticPointerDevice(int pointerType, uint maxCount, int feedbackMode);

        [DllImport("user32.dll", SetLastError = true)]
        private static extern bool InjectSyntheticPointerInput(IntPtr device, PointerTypeInfo[] pointerInfo, uint count);

        [DllImport("user32.dll")]
        private static extern void DestroySyntheticPointerDevice(IntPtr device);

        // A pen press at from, a drag to to, and a lift; barrel holds the barrel button down throughout.
        internal static void PenDrag(PointInt32 from, PointInt32 to, bool barrel, int steps = 8)
        {
            EnsureForeground(from);
            IntPtr device = CreateSyntheticPointerDevice(3 /* PT_PEN */, 1, 1 /* POINTER_FEEDBACK_DEFAULT */);
            Verify.AreNotEqual(IntPtr.Zero, device, $"CreateSyntheticPointerDevice failed: {Marshal.GetLastWin32Error()}.");
            try
            {
                void Send(PointInt32 point, uint flags)
                {
                    var info = new PointerTypeInfo
                    {
                        Type = 3,
                        Pen = new PenInfo
                        {
                            Pointer = new PointerInfo { Type = 3, Flags = flags | (barrel && (flags & PointerFlagInContact) != 0 ? 0x20u /* SECONDBUTTON */ : 0u), Pixel = point },
                            PenFlags = barrel ? 0x1u /* PEN_FLAG_BARREL */ : 0u,
                            PenMask = 0x1 /* PEN_MASK_PRESSURE */,
                            Pressure = 512,
                        },
                    };
                    Verify.IsTrue(InjectSyntheticPointerInput(device, new[] { info }, 1), $"InjectSyntheticPointerInput failed: {Marshal.GetLastWin32Error()}.");
                }

                Send(from, PointerFlagInRange | PointerFlagUpdate);
                global::System.Threading.Thread.Sleep(16);
                Send(from, PointerFlagDown | PointerFlagInRange | PointerFlagInContact | 0x10u /* FIRSTBUTTON */);
                for (int step = 1; step <= steps; step++)
                {
                    global::System.Threading.Thread.Sleep(16);
                    Send(new PointInt32(from.X + ((to.X - from.X) * step / steps), from.Y + ((to.Y - from.Y) * step / steps)),
                        PointerFlagUpdate | PointerFlagInRange | PointerFlagInContact | 0x10u);
                }
                global::System.Threading.Thread.Sleep(16);
                Send(to, PointerFlagUp | PointerFlagInRange);
                global::System.Threading.Thread.Sleep(16);
                Send(to, PointerFlagUpdate);
            }
            finally
            {
                DestroySyntheticPointerDevice(device);
            }
            IdleSynchronizer.Wait();
        }
    }
}
