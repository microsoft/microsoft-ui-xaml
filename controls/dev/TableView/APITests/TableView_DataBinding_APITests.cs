// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Tabular;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Markup;
using MUXControlsTestApp.Utilities;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using Windows.Foundation;

using WEX.Logging.Interop;
using WEX.TestExecution;
using WEX.TestExecution.Markup;

using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewColumnTestHelpers;
using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewShapingTestHelpers;
using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewBindingTestHelpers;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    // Category 2 of the TableView test plan: data binding and data source projection.
    //
    // SUBJECT. Category 2 is the binding pipeline - an UNSHAPED source reaching rendered cells.
    // Category 8 is the shaping algebra. The split is written up as plan section 2.0; the rule of
    // thumb is that a test which would still make sense with no shaping applied belongs here.
    //
    // OBSERVATION. Row order is read from PART_RowsRepeater through the shared helpers
    // (ProjectedCount / ProjectedLabels), the same route Category 8 uses, so the two categories
    // report order the same way. Cell values are read from the realized cell wrapper, because the
    // rendered TextBlock is the thing an app actually sees.
    //
    // DATA MODEL. ShapedPerson (declared alongside the Category 8 tests) is reused deliberately: it
    // raises INotifyPropertyChanged and carries a nested Department, which are exactly the two
    // things this category needs. Declaring a second near-identical item type would make the two
    // files drift.
    [TestClass]
    public class TableViewDataBindingTests : ApiTestBase
    {
        #region 2.1 ItemsSource shapes

        [TestMethod]
        [TestProperty("Description", "Verifies every supported ItemsSource shape reads back by reference and renders one row per item in source order.")]
        [TestProperty("data:SourceKind", "{List, ObservableCollection, TableViewSource}")]
        public void VerifyItemsSourceShapesRenderOneRowPerItem()
        {
            var sourceKind = ((string)TestContext.DataRow["SourceKind"]).Trim();
            Log.Comment($"Source shape under test: {sourceKind}.");

            TableView tableView = null;
            List<ShapedPerson> items = null;
            object assigned = null;

            RunOnUIThread.Execute(() =>
            {
                items = MakeShapedPeople();

                switch (sourceKind)
                {
                    case "List":
                        assigned = items;
                        break;
                    case "ObservableCollection":
                        assigned = new ObservableCollection<ShapedPerson>(items);
                        break;
                    case "TableViewSource":
                        assigned = TableViewSource.From(items);
                        break;
                    default:
                        Verify.Fail($"Unhandled source kind '{sourceKind}'.");
                        return;
                }

                tableView = CreateBindingTable(assigned);
                Content = tableView;
                Content.UpdateLayout();
            });

            Settle(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(ReferenceEquals(assigned, tableView.ItemsSource),
                    $"{sourceKind}: ItemsSource must read back the same instance it was given.");

                VerifyRowItems(tableView, items, $"{sourceKind} source");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies assigning a different ItemsSource rebuilds every row against the new collection.")]
        public void VerifyItemsSourceSwapReplacesAllRows()
        {
            TableView tableView = null;
            List<ShapedPerson> first = null;
            List<ShapedPerson> second = null;

            RunOnUIThread.Execute(() =>
            {
                first = MakeShapedPeople();
                second = new List<ShapedPerson>
                {
                    new ShapedPerson("Nadia", "Engineer", "Platform"),
                    new ShapedPerson("Owen", "Engineer", "Platform"),
                    new ShapedPerson("Pia", "Designer", "Studio"),
                    new ShapedPerson("Quin", "Architect", "Research"),
                };

                tableView = CreateBindingTable(first);
                Content = tableView;
                Content.UpdateLayout();
            });

            Settle(tableView);

            RunOnUIThread.Execute(() =>
            {
                VerifyRowItems(tableView, first, "before the swap");
                tableView.ItemsSource = second;
            });

            Settle(tableView);

            RunOnUIThread.Execute(() =>
            {
                VerifyRowItems(tableView, second, "after the swap");

                foreach (var row in GetRealizedRows(tableView))
                {
                    Verify.IsFalse(first.Contains(row.DataContext as ShapedPerson),
                        "No row may still carry an item from the discarded collection.");
                }
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies setting a populated ItemsSource back to null clears every row.")]
        public void VerifyItemsSourceSetToNullClearsRows()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateBindingTable(MakeShapedPeople());
                Content = tableView;
                Content.UpdateLayout();
            });

            Settle(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(6, ProjectedCount(tableView), "Precondition: six rows before clearing.");
                tableView.ItemsSource = null;
            });

            Settle(tableView);

            RunOnUIThread.Execute(() =>
            {
                var repeater = RequireRowsRepeater(tableView);
                var view = repeater?.ItemsSourceView;

                Verify.IsTrue(view == null || view.Count == 0,
                    $"A null ItemsSource must project no rows at all, projected {view?.Count}.");

                // Rows are not expected to leave the visual tree: ItemsRepeater parks cleared
                // containers at ClearedElementsArrangePosition (-10000, -10000) and keeps them
                // parented for reuse. What must be true is that none of them is still rendered
                // inside the control.
                foreach (var row in GetRealizedRows(tableView))
                {
                    var origin = row.TransformToVisual(tableView).TransformPoint(new Point(0, 0));
                    var offscreen =
                        origin.X + row.ActualWidth <= 0 || origin.X >= tableView.ActualWidth ||
                        origin.Y + row.ActualHeight <= 0 || origin.Y >= tableView.ActualHeight;

                    Verify.IsTrue(offscreen,
                        $"A row left over after clearing ItemsSource is still arranged inside the control at ({origin.X}, {origin.Y}).");
                }
            });
        }

        #endregion

        #region 2.2 Text column binding

        [TestMethod]
        [TestProperty("Description", "Verifies the generated text cell shows the value at the column's binding path, and shows nothing when the path cannot resolve.")]
        [TestProperty("data:BindingCase", "{Simple, Dotted, Invalid, None}")]
        public void VerifyTextCellTextFollowsTheBindingPath()
        {
            var bindingCase = ((string)TestContext.DataRow["BindingCase"]).Trim();
            Log.Comment($"Binding case under test: {bindingCase}.");

            TableView tableView = null;
            string expected = null;

            RunOnUIThread.Execute(() =>
            {
                Binding binding = null;

                switch (bindingCase)
                {
                    case "Simple":
                        binding = new Binding { Path = new PropertyPath("Name"), Mode = BindingMode.OneWay };
                        expected = "Asha";
                        break;
                    case "Dotted":
                        binding = new Binding { Path = new PropertyPath("Department.Name"), Mode = BindingMode.OneWay };
                        expected = "Studio";
                        break;
                    case "Invalid":
                        binding = new Binding { Path = new PropertyPath("NoSuchProperty"), Mode = BindingMode.OneWay };
                        expected = string.Empty;
                        break;
                    case "None":
                        binding = null;
                        expected = string.Empty;
                        break;
                    default:
                        Verify.Fail($"Unhandled binding case '{bindingCase}'.");
                        return;
                }

                EnsureTabularControlsResources();

                tableView = new TableView
                {
                    ItemsSource = MakeShapedPeople(),
                    Width = 500,
                    Height = 700,
                };

                tableView.Columns.Add(new TableViewTextColumn { Header = "Under test", Binding = binding });

                Content = tableView;
                Content.UpdateLayout();
            });

            Settle(tableView);

            RunOnUIThread.Execute(() =>
            {
                var text = RequireCellText(tableView, rowIndex: 0, columnIndex: 0);

                Verify.AreEqual(expected, text,
                    $"{bindingCase}: the first cell should read '{expected}', was '{text}'.");

                // The failure this is really guarding: an unresolved binding that falls back to the
                // item's ToString() looks like data and is far worse than an empty cell.
                Verify.IsFalse(text.Contains("ShapedPerson"),
                    $"{bindingCase}: a cell must never render the data item's type name.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies replacing Binding on a loaded text column re-projects its realized cells.")]
        public void VerifyTextColumnBindingChangeAfterLoadUpdatesRealizedCells()
        {
            TableView tableView = null;
            TableViewTextColumn column = null;

            RunOnUIThread.Execute(() =>
            {
                EnsureTabularControlsResources();

                tableView = new TableView
                {
                    ItemsSource = MakeShapedPeople(),
                    Width = 500,
                    Height = 700,
                };

                column = new TableViewTextColumn
                {
                    Header = "Under test",
                    Binding = new Binding { Path = new PropertyPath("Name"), Mode = BindingMode.OneWay },
                };

                tableView.Columns.Add(column);
                tableView.Columns.Add(new TableViewTextColumn
                {
                    Header = "Role",
                    Binding = new Binding { Path = new PropertyPath("Role"), Mode = BindingMode.OneWay },
                });

                Content = tableView;
                Content.UpdateLayout();
            });

            Settle(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual("Asha", RequireCellText(tableView, 0, 0), "Precondition: the column starts bound to Name.");

                column.Binding = new Binding { Path = new PropertyPath("Role"), Mode = BindingMode.OneWay };
            });

            Settle(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual("Designer", RequireCellText(tableView, 0, 0),
                    "Re-pointing Binding must re-project the realized cells of that column.");
                Verify.AreEqual("Engineer", RequireCellText(tableView, 1, 0),
                    "Every realized row in the column must be re-projected, not only the first.");
                Verify.AreEqual("Designer", RequireCellText(tableView, 0, 1),
                    "The untouched column must be left alone.");
            });
        }

        #endregion

        #region 2.3 Template column binding

        [TestMethod]
        [TestProperty("Description", "Verifies a template column with no CellTemplate renders an empty cell rather than the item's ToString().")]
        public void VerifyTemplateColumnWithNoCellTemplateRendersEmptyCell()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                EnsureTabularControlsResources();

                tableView = new TableView
                {
                    ItemsSource = MakeShapedPeople(),
                    Width = 500,
                    Height = 700,
                };

                tableView.Columns.Add(new TableViewTemplateColumn { Header = "Under test" });

                Content = tableView;
                Content.UpdateLayout();
            });

            Settle(tableView);

            RunOnUIThread.Execute(() =>
            {
                var wrapper = RequireCellWrapper(tableView, rowIndex: 0, columnIndex: 0);
                if (wrapper == null)
                {
                    return;
                }

                var presenter = wrapper.Child as ContentPresenter;
                if (presenter == null)
                {
                    Verify.Fail("A template column should still generate a ContentPresenter when CellTemplate is null.");
                    return;
                }

                Verify.IsNull(presenter.ContentTemplate,
                    "With no CellTemplate there is nothing to template the cell with.");

                var texts = TableViewColumnTestHelpers.FindVisualChildrenByType<TextBlock>(presenter)
                    .Select(textBlock => textBlock.Text)
                    .Where(text => !string.IsNullOrEmpty(text))
                    .ToList();

                if (texts.Count > 0)
                {
                    Verify.Fail($"An unconfigured template column rendered text: '{string.Join("', '", texts)}'.");
                }
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies assigning a new CellTemplate to a loaded template column rebuilds its realized cells.")]
        public void VerifyCellTemplateChangeAfterLoadRebuildsRealizedCells()
        {
            TableView tableView = null;
            TableViewTemplateColumn column = null;

            RunOnUIThread.Execute(() =>
            {
                EnsureTabularControlsResources();

                tableView = new TableView
                {
                    ItemsSource = MakeShapedPeople(),
                    Width = 500,
                    Height = 700,
                };

                column = new TableViewTemplateColumn
                {
                    Header = "Under test",
                    CellTemplate = CreateBoundTemplate("Name"),
                };

                tableView.Columns.Add(column);

                Content = tableView;
                Content.UpdateLayout();
            });

            Settle(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual("Asha", RequireCellText(tableView, 0, 0),
                    "Precondition: the first template cell renders through the first template.");

                column.CellTemplate = CreateBoundTemplate("Role");
            });

            Settle(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual("Designer", RequireCellText(tableView, 0, 0),
                    "A CellTemplate change must rebuild the realized cell content.");

                var row = RequireRow(tableView, 0);
                if (row == null)
                {
                    return;
                }

                Verify.AreEqual(ProjectedItem(tableView, 0), row.DataContext,
                    "Rebuilding cell content must not disturb the row's data item.");
            });
        }

        #endregion

        #region 2.4 Live source updates

        [TestMethod]
        [TestProperty("Description", "Verifies an insert into an observable source places the new row at that index.")]
        [TestProperty("data:Position", "{Start, Middle, End}")]
        public void VerifyObservableInsertPlacesRowAtThatPosition()
        {
            var position = ((string)TestContext.DataRow["Position"]).Trim();
            Log.Comment($"Insert position under test: {position}.");

            TableView tableView = null;
            ObservableCollection<ShapedPerson> items = null;
            int index = 0;

            RunOnUIThread.Execute(() =>
            {
                items = new ObservableCollection<ShapedPerson>(MakeShapedPeople());
                tableView = CreateBindingTable(items);
                Content = tableView;
                Content.UpdateLayout();
            });

            Settle(tableView);

            RunOnUIThread.Execute(() =>
            {
                switch (position)
                {
                    case "Start": index = 0; break;
                    case "Middle": index = 3; break;
                    case "End": index = items.Count; break;
                    default:
                        Verify.Fail($"Unhandled position '{position}'.");
                        return;
                }

                items.Insert(index, new ShapedPerson("Nadia", "Engineer", "Platform"));
            });

            Settle(tableView);

            RunOnUIThread.Execute(() =>
            {
                VerifyRowItems(tableView, items, $"after inserting at {position} (index {index})");

                var item = ProjectedItem(tableView, index) as ShapedPerson;
                Verify.AreEqual("Nadia", item?.Name,
                    $"The inserted row must be at index {index}, was '{item?.Name}'.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies removing an item removes exactly its row and leaves the others in order.")]
        public void VerifyObservableRemoveRemovesOnlyThatRow()
        {
            TableView tableView = null;
            ObservableCollection<ShapedPerson> items = null;
            ShapedPerson removed = null;

            RunOnUIThread.Execute(() =>
            {
                items = new ObservableCollection<ShapedPerson>(MakeShapedPeople());
                tableView = CreateBindingTable(items);
                Content = tableView;
                Content.UpdateLayout();
            });

            Settle(tableView);

            RunOnUIThread.Execute(() =>
            {
                removed = items[2];
                items.RemoveAt(2);
            });

            Settle(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(5, ProjectedCount(tableView), "Exactly one row should have gone.");
                VerifyRowItems(tableView, items, "after removing the third item");

                for (var i = 0; i < ProjectedCount(tableView); i++)
                {
                    Verify.IsFalse(ReferenceEquals(removed, ProjectedItem(tableView, i)),
                        "The removed item must not still hold a row.");
                }
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies replacing an item updates that row and leaves its neighbours untouched.")]
        public void VerifyObservableReplaceUpdatesRowInPlace()
        {
            TableView tableView = null;
            ObservableCollection<ShapedPerson> items = null;
            ShapedPerson before = null;
            ShapedPerson after = null;

            RunOnUIThread.Execute(() =>
            {
                items = new ObservableCollection<ShapedPerson>(MakeShapedPeople());
                tableView = CreateBindingTable(items);
                Content = tableView;
                Content.UpdateLayout();
            });

            Settle(tableView);

            RunOnUIThread.Execute(() =>
            {
                before = items[1];
                after = new ShapedPerson("Nadia", "Engineer", "Platform");
                items[1] = after;
            });

            Settle(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(6, ProjectedCount(tableView), "A replace must not change the row count.");
                Verify.AreEqual(after, ProjectedItem(tableView, 1), "Row 1 must carry the replacement item.");
                Verify.IsFalse(ReferenceEquals(before, ProjectedItem(tableView, 1)),
                    "Row 1 must no longer carry the replaced item.");
                Verify.AreEqual("Nadia", RequireCellText(tableView, 1, 0),
                    "The replaced row's cells must re-project against the new item.");

                VerifyRowItems(tableView, items, "after replacing the second item");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a move notification reorders the projected rows.")]
        public void VerifyObservableMoveReordersRows()
        {
            TableView tableView = null;
            ObservableCollection<ShapedPerson> items = null;

            RunOnUIThread.Execute(() =>
            {
                items = new ObservableCollection<ShapedPerson>(MakeShapedPeople());
                tableView = CreateBindingTable(items);
                Content = tableView;
                Content.UpdateLayout();
            });

            Settle(tableView);

            RunOnUIThread.Execute(() => items.Move(0, 4));

            Settle(tableView);

            RunOnUIThread.Execute(() =>
            {
                VerifyRowItems(tableView, items, "after moving the first item to index 4");

                var moved = ProjectedItem(tableView, 4) as ShapedPerson;
                Verify.AreEqual("Asha", moved?.Name,
                    $"The moved item should now be at index 4, saw '{moved?.Name}'.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a reset notification discards every stale row and rebuilds from the current contents.")]
        public void VerifyObservableResetRebuildsAllRows()
        {
            TableView tableView = null;
            ObservableCollection<ShapedPerson> items = null;
            List<ShapedPerson> discarded = null;

            RunOnUIThread.Execute(() =>
            {
                items = new ObservableCollection<ShapedPerson>(MakeShapedPeople());
                tableView = CreateBindingTable(items);
                Content = tableView;
                Content.UpdateLayout();
            });

            Settle(tableView);

            RunOnUIThread.Execute(() =>
            {
                discarded = items.ToList();

                // Clear() raises Reset, which carries no per-item information - the control has to
                // rebuild from the collection rather than from the notification.
                items.Clear();
                items.Add(new ShapedPerson("Nadia", "Engineer", "Platform"));
                items.Add(new ShapedPerson("Owen", "Engineer", "Platform"));
            });

            Settle(tableView);

            RunOnUIThread.Execute(() =>
            {
                VerifyRowItems(tableView, items, "after a reset and repopulate");

                for (var i = 0; i < ProjectedCount(tableView); i++)
                {
                    Verify.IsFalse(discarded.Contains(ProjectedItem(tableView, i) as ShapedPerson),
                        "No row may survive the reset carrying a discarded item.");
                }
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a burst of mutations in one UI-thread turn settles to the final collection contents.")]
        public void VerifyRapidObservableMutationsSettleToExpectedRows()
        {
            TableView tableView = null;
            ObservableCollection<ShapedPerson> items = null;

            RunOnUIThread.Execute(() =>
            {
                items = new ObservableCollection<ShapedPerson>(MakeShapedPeople());
                tableView = CreateBindingTable(items);
                Content = tableView;
                Content.UpdateLayout();
            });

            Settle(tableView);

            RunOnUIThread.Execute(() =>
            {
                items.Insert(0, new ShapedPerson("Ada", "Engineer", "Platform"));
                items.RemoveAt(3);
                items[1] = new ShapedPerson("Bea", "Designer", "Studio");
                items.Add(new ShapedPerson("Cy", "Architect", "Research"));
                items.Move(0, 2);
                items.RemoveAt(items.Count - 1);
                items.Insert(2, new ShapedPerson("Dev", "Engineer", "Platform"));
                items[0] = new ShapedPerson("Eve", "Designer", "Studio");
                items.Add(new ShapedPerson("Fay", "Engineer", "Platform"));
                items.RemoveAt(1);
                items.Insert(1, new ShapedPerson("Gus", "Architect", "Research"));
                items.Move(items.Count - 1, 0);
            });

            Settle(tableView);

            RunOnUIThread.Execute(() =>
                VerifyRowItems(tableView, items, "after a burst of twelve mutations"));
        }

        #endregion

        #region 2.5 Item property change notification

        [TestMethod]
        [TestProperty("Description", "Verifies an INotifyPropertyChanged raise on a realized item updates its bound text cell.")]
        public void VerifyItemPropertyChangeUpdatesItsBoundTextCell()
        {
            TableView tableView = null;
            List<ShapedPerson> items = null;

            RunOnUIThread.Execute(() =>
            {
                items = MakeShapedPeople();
                tableView = CreateBindingTable(items);
                Content = tableView;
                Content.UpdateLayout();
            });

            Settle(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual("Diego", RequireCellText(tableView, 1, 0), "Precondition: row 1 shows its current name.");
                items[1].Name = "Diego Renamed";
            });

            Settle(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual("Diego Renamed", RequireCellText(tableView, 1, 0),
                    "A property change on a realized item must reach its bound cell.");
                Verify.AreEqual("Asha", RequireCellText(tableView, 0, 0),
                    "Only the changed item's row may change.");
                Verify.AreEqual("Engineer", RequireCellText(tableView, 1, 1),
                    "Only the bound column may change.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a property change on an item with no realized row is ignored, and the new value appears once that row realizes.")]
        public void VerifyItemPropertyChangeOnAnUnrealizedItemIsIgnored()
        {
            TableView tableView = null;
            List<ShapedPerson> items = null;

            RunOnUIThread.Execute(() =>
            {
                items = MakeManyShapedPeople(160);
                tableView = CreateBindingTable(items);
                tableView.Height = 300;
                Content = tableView;
                Content.UpdateLayout();
            });

            Settle(tableView);

            RunOnUIThread.Execute(() =>
            {
                var realized = GetRealizedRows(tableView);
                Verify.IsGreaterThan(realized.Count, 0, "Rows should be realized before scrolling.");
                Verify.IsLessThan(realized.Count, items.Count,
                    "Precondition: the source must be long enough that item 140 is not realized.");

                // No row exists for this item yet. The raise must be harmless.
                items[140].Name = "Renamed While Offscreen";
            });

            Settle(tableView);

            TableViewRowTestHelpers.ScrollBodyToVerticalOffset(tableView, 140 * 40.0);

            RunOnUIThread.Execute(() =>
            {
                var match = GetRealizedRows(tableView)
                    .Select(row => row.DataContext as ShapedPerson)
                    .FirstOrDefault(person => ReferenceEquals(person, items[140]));

                if (match == null)
                {
                    Verify.Fail("The scrolled-to item should have a realized row.");
                    return;
                }

                Verify.AreEqual("Renamed While Offscreen", match.Name,
                    "The item itself should hold the new value.");

                var row = GetRealizedRows(tableView).First(r => ReferenceEquals(r.DataContext, items[140]));
                Verify.AreEqual("Renamed While Offscreen", CellTextOfRow(row, 0),
                    "A row realized after the change must show the current value, not a stale one.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies property changes on items of a discarded source do not touch the rows of the current source.")]
        public void VerifyItemPropertyChangeAfterSourceSwapDoesNotTouchNewRows()
        {
            TableView tableView = null;
            List<ShapedPerson> discarded = null;
            List<ShapedPerson> current = null;

            RunOnUIThread.Execute(() =>
            {
                discarded = MakeShapedPeople();
                current = new List<ShapedPerson>
                {
                    new ShapedPerson("Nadia", "Engineer", "Platform"),
                    new ShapedPerson("Owen", "Engineer", "Platform"),
                };

                tableView = CreateBindingTable(discarded);
                Content = tableView;
                Content.UpdateLayout();
            });

            Settle(tableView);

            RunOnUIThread.Execute(() => tableView.ItemsSource = current);

            Settle(tableView);

            RunOnUIThread.Execute(() =>
            {
                foreach (var person in discarded)
                {
                    person.Name = "Should Not Appear";
                }
            });

            Settle(tableView);

            RunOnUIThread.Execute(() =>
            {
                VerifyRowItems(tableView, current, "after mutating the discarded items");

                Verify.AreEqual("Nadia", RequireCellText(tableView, 0, 0),
                    "A discarded item's property change must not write into a current row.");
                Verify.AreEqual("Owen", RequireCellText(tableView, 1, 0),
                    "A discarded item's property change must not write into a current row.");
            });
        }

        #endregion

        #region 2.6 Error and edge inputs

        [TestMethod]
        [TestProperty("Description", "Verifies an unsupported ItemsSource fails loudly and leaves the control able to accept a valid source.")]
        public void VerifyUnsupportedItemsSourceThrowsAndLeavesTheControlUsable()
        {
            TableView tableView = null;
            List<ShapedPerson> items = null;

            RunOnUIThread.Execute(() =>
            {
                EnsureTabularControlsResources();

                tableView = new TableView { Width = 500, Height = 700 };
                tableView.Columns.Add(new TableViewTextColumn
                {
                    Header = "Name",
                    Binding = new Binding { Path = new PropertyPath("Name"), Mode = BindingMode.OneWay },
                });

                Content = tableView;
                Content.UpdateLayout();
            });

            Settle(tableView);

            RunOnUIThread.Execute(() =>
            {
                var threw = false;

                try
                {
                    // Not a collection of any kind. TableViewSource::From is documented to refuse it,
                    // and TableView adopts that refusal deliberately rather than degrading.
                    tableView.ItemsSource = 42;
                }
                catch (Exception e)
                {
                    threw = true;
                    Log.Comment($"Assigning an unsupported ItemsSource threw {e.GetType().Name}: {e.Message}");
                }

                Verify.IsTrue(threw, "An unsupported ItemsSource must fail loudly rather than silently showing nothing.");
            });

            Settle(tableView);

            RunOnUIThread.Execute(() =>
            {
                items = MakeShapedPeople();
                tableView.ItemsSource = items;
            });

            Settle(tableView);

            RunOnUIThread.Execute(() =>
                VerifyRowItems(tableView, items, "after recovering from an unsupported source"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a source whose enumerator throws does not wedge the control for the next valid source.")]
        public void VerifyThrowingEnumeratorLeavesControlUsable()
        {
            TableView tableView = null;
            List<ShapedPerson> items = null;

            RunOnUIThread.Execute(() =>
            {
                EnsureTabularControlsResources();

                tableView = new TableView { Width = 500, Height = 700 };
                tableView.Columns.Add(new TableViewTextColumn
                {
                    Header = "Name",
                    Binding = new Binding { Path = new PropertyPath("Name"), Mode = BindingMode.OneWay },
                });

                Content = tableView;
                Content.UpdateLayout();
            });

            Settle(tableView);

            RunOnUIThread.Execute(() =>
            {
                try
                {
                    tableView.ItemsSource = new ThrowingSource();
                }
                catch (Exception e)
                {
                    Log.Comment($"Assigning a throwing source surfaced {e.GetType().Name}: {e.Message}");
                }
            });

            Settle(tableView);

            RunOnUIThread.Execute(() =>
            {
                items = MakeShapedPeople();
                tableView.ItemsSource = items;
            });

            Settle(tableView);

            RunOnUIThread.Execute(() =>
                VerifyRowItems(tableView, items, "after a throwing source was rejected"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies the same item instance appearing twice in an unshaped source produces two independent rows.")]
        public void VerifyDuplicateItemInstancesProduceIndependentRows()
        {
            TableView tableView = null;
            ShapedPerson duplicated = null;

            RunOnUIThread.Execute(() =>
            {
                duplicated = new ShapedPerson("Asha", "Designer", "Studio");

                var items = new List<ShapedPerson>
                {
                    duplicated,
                    new ShapedPerson("Diego", "Engineer", "Platform"),
                    duplicated,
                };

                tableView = CreateBindingTable(items);
                Content = tableView;
                Content.UpdateLayout();
            });

            Settle(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(3, ProjectedCount(tableView),
                    "An unshaped source must project one row per entry, duplicates included.");

                Verify.AreEqual(duplicated, ProjectedItem(tableView, 0), "Row 0 carries the duplicated instance.");
                Verify.AreEqual(duplicated, ProjectedItem(tableView, 2), "Row 2 carries the same instance again.");

                var firstRow = RequireRow(tableView, 0);
                var thirdRow = RequireRow(tableView, 2);
                if (firstRow == null || thirdRow == null)
                {
                    return;
                }

                Verify.IsFalse(ReferenceEquals(firstRow, thirdRow),
                    "The two rows must be independent containers, not one container shared by identity.");

                Verify.AreEqual("Asha", RequireCellText(tableView, 0, 0), "Both rows render the item's values.");
                Verify.AreEqual("Asha", RequireCellText(tableView, 2, 0), "Both rows render the item's values.");
            });
        }

        #endregion

        // Two settles: the first lets the source notification reach the repeater, the layout pass
        // rebuilds containers, and the second lets that layout settle before anything is sampled.
        private void Settle(TableView tableView)
        {
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() => tableView.UpdateLayout());
            IdleSynchronizer.Wait();
        }
    }

    internal static class TableViewBindingTestHelpers
    {
        // Two text columns over Name and Role, tall enough that the six-item fixtures fully realize.
        internal static TableView CreateBindingTable(object itemsSource)
        {
            EnsureTabularControlsResources();

            var tableView = new TableView
            {
                ItemsSource = itemsSource,
                Width = 500,
                Height = 700,
            };

            tableView.Columns.Add(new TableViewTextColumn
            {
                Header = "Name",
                Binding = new Binding { Path = new PropertyPath("Name"), Mode = BindingMode.OneWay },
            });

            tableView.Columns.Add(new TableViewTextColumn
            {
                Header = "Role",
                Binding = new Binding { Path = new PropertyPath("Role"), Mode = BindingMode.OneWay },
            });

            return tableView;
        }

        internal static List<ShapedPerson> MakeManyShapedPeople(int count) => Enumerable
            .Range(0, count)
            .Select(i => new ShapedPerson($"Person {i:D3}", i % 2 == 0 ? "Designer" : "Engineer", "Studio"))
            .ToList();

        internal static DataTemplate CreateBoundTemplate(string path) => (DataTemplate)XamlReader.Load(
            $@"<DataTemplate xmlns=""http://schemas.microsoft.com/winfx/2006/xaml/presentation"">
                   <TextBlock Text=""{{Binding {path}}}"" />
               </DataTemplate>");

        // The item at a projected row index, read from the repeater's view so it describes the
        // projection rather than whatever happens to be realized.
        internal static object ProjectedItem(TableView tableView, int index)
        {
            var repeater = RequireRowsRepeater(tableView);
            var view = repeater?.ItemsSourceView;

            if (view == null || index >= view.Count)
            {
                Verify.Fail($"No projected row at index {index}.");
                return null;
            }

            return view.GetAt(index);
        }

        internal static TableViewRow RequireRow(TableView tableView, int index)
        {
            var repeater = RequireRowsRepeater(tableView);
            if (repeater == null)
            {
                return null;
            }

            var row = repeater.TryGetElement(index) as TableViewRow;
            if (row == null)
            {
                Verify.Fail($"Row {index} should be realized in these fixtures.");
            }

            return row;
        }

        internal static Border RequireCellWrapper(TableView tableView, int rowIndex, int columnIndex)
        {
            var row = RequireRow(tableView, rowIndex);
            if (row == null)
            {
                return null;
            }

            var host = row.FindVisualChildByName("PART_CellsHost") as Panel;
            if (host == null)
            {
                Verify.Fail("PART_CellsHost should exist on a realized row.");
                return null;
            }

            var wrappers = host.Children.OfType<Border>().ToList();
            if (columnIndex >= wrappers.Count)
            {
                Verify.Fail($"Row {rowIndex} has {wrappers.Count} cells, no cell at column {columnIndex}.");
                return null;
            }

            return wrappers[columnIndex];
        }

        // The rendered text of a cell, whichever column type produced it: a text column generates a
        // TextBlock directly, a template column generates a ContentPresenter that inflates one.
        internal static string RequireCellText(TableView tableView, int rowIndex, int columnIndex)
        {
            var wrapper = RequireCellWrapper(tableView, rowIndex, columnIndex);
            if (wrapper == null)
            {
                return null;
            }

            return TextOf(wrapper);
        }

        internal static string CellTextOfRow(TableViewRow row, int columnIndex)
        {
            var host = row.FindVisualChildByName("PART_CellsHost") as Panel;
            if (host == null)
            {
                Verify.Fail("PART_CellsHost should exist on a realized row.");
                return null;
            }

            var wrappers = host.Children.OfType<Border>().ToList();
            if (columnIndex >= wrappers.Count)
            {
                Verify.Fail($"No cell at column {columnIndex}.");
                return null;
            }

            return TextOf(wrappers[columnIndex]);
        }

        private static string TextOf(Border wrapper)
        {
            if (wrapper.Child is TextBlock direct)
            {
                return direct.Text;
            }

            var textBlock = TableViewColumnTestHelpers.FindVisualChildrenByType<TextBlock>(wrapper).FirstOrDefault();
            if (textBlock == null)
            {
                Verify.Fail("The cell should host a TextBlock.");
                return null;
            }

            return textBlock.Text;
        }

        // Asserts the projected rows are exactly these items, in this order. Logs both sequences on
        // failure so an ordering bug is readable without re-running under a debugger.
        internal static void VerifyRowItems(TableView tableView, IList<ShapedPerson> expected, string context)
        {
            var actual = new List<string>();
            var repeater = RequireRowsRepeater(tableView);
            var view = repeater?.ItemsSourceView;

            if (view == null)
            {
                Verify.Fail($"PART_RowsRepeater should have an ItemsSourceView ({context}).");
                return;
            }

            for (var i = 0; i < view.Count; i++)
            {
                actual.Add((view.GetAt(i) as ShapedPerson)?.Name ?? "<not a ShapedPerson>");
            }

            var expectedNames = expected.Select(person => person.Name).ToList();
            var detail = $"Expected [{string.Join(", ", expectedNames)}], saw [{string.Join(", ", actual)}].";

            Verify.AreEqual(expectedNames.Count, actual.Count, $"Projected row count ({context}). {detail}");

            for (var i = 0; i < Math.Min(expectedNames.Count, actual.Count); i++)
            {
                Verify.AreEqual(expectedNames[i], actual[i], $"Projected row {i} ({context}). {detail}");
            }
        }
    }

    // A source that enumerates two items and then fails. Nothing in the IDL or the design notes says
    // what a TableView should do with one; the test only asserts the control survives it.
    internal sealed class ThrowingSource : IEnumerable<ShapedPerson>
    {
        public IEnumerator<ShapedPerson> GetEnumerator()
        {
            yield return new ShapedPerson("Asha", "Designer", "Studio");
            yield return new ShapedPerson("Diego", "Engineer", "Platform");
            throw new InvalidOperationException("This source fails partway through enumeration.");
        }

        IEnumerator IEnumerable.GetEnumerator() => GetEnumerator();
    }
}
