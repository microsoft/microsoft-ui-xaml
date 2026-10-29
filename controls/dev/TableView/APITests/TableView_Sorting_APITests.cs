// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Private.Controls;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Automation.Peers;
using Microsoft.UI.Xaml.Automation.Provider;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Tabular;
using Microsoft.UI.Xaml.Data;
using MUXControlsTestApp.Utilities;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Runtime.CompilerServices;

using WEX.Logging.Interop;
using WEX.TestExecution;
using WEX.TestExecution.Markup;

using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewTestHelpers;
using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewShapingTestHelpers;
using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewSortingTestHelpers;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    // Category 7 of the TableView test plan: sorting.
    //
    // SUBJECT. The control-level sort API - the state that lives on TableViewColumn.SortDirection,
    // the three verbs and their documented Boolean returns, the Sorting/Sorted events, the two
    // click-to-sort gates, and the reconciliation rule with TableViewSource.Sort. Category 8 owns
    // TableViewSource.Sort itself. Plan section 7.0 writes the split up in full; the short version
    // is that a test belongs here if it would still be meaningful with TableViewSource never
    // mentioned in the test body.
    //
    // KEY RESOLUTION. Several tests reach GetSortMemberPathCore(), which is `overridable` in the
    // IDL and therefore projects as `protected virtual`. A test class cannot call it, so the probe
    // columns at the bottom of this file expose a public passthrough to base.GetSortMemberPathCore().
    [TestClass]
    public class TableViewSortingTests : TableViewApiTestBase
    {
        #region 7.1 Column sort surface

        [TestMethod]
        [TestProperty("Description", "Verifies a fresh column reports the sort defaults declared in TableView.idl.")]
        public void VerifyColumnSortDefaults()
        {
            RunOnUIThread.Execute(() =>
            {
                var column = new TableViewColumn();

                Verify.IsTrue(column.CanUserSort, "CanUserSort defaults to true (TableView.idl:157).");
                Verify.AreEqual(TableViewSortCycle.AscendingDescending, column.SortCycle,
                    "SortCycle defaults to AscendingDescending (TableView.idl:165).");
                Verify.AreEqual(string.Empty, column.SortMemberPath, "SortMemberPath defaults to empty.");
                Verify.IsNull(column.CustomSortComparer, "CustomSortComparer defaults to null.");
                Verify.AreEqual(SortDirection.None, column.SortDirection,
                    "SortDirection defaults to None (TableView.idl:185).");

                var textColumn = new TableViewTextColumn();

                Verify.IsTrue(textColumn.CanUserSort, "A text column shares the base defaults.");
                Verify.AreEqual(SortDirection.None, textColumn.SortDirection, "A text column starts unsorted.");
                Verify.AreEqual(string.Empty, textColumn.SortMemberPath, "A text column starts with no explicit path.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies the base column returns SortMemberPath verbatim from GetSortMemberPathCore.")]
        public void VerifyGetSortMemberPathReturnsExplicitPath()
        {
            RunOnUIThread.Execute(() =>
            {
                var column = new SortPathProbeColumn { SortMemberPath = "DepartmentName" };

                Verify.AreEqual("DepartmentName", column.ReadSortMemberPath(),
                    "The base column returns SortMemberPath verbatim (TableView.idl:192-193).");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a text column with no explicit SortMemberPath falls back to its Binding path.")]
        public void VerifyGetSortMemberPathFallsBackToTextColumnBindingPath()
        {
            RunOnUIThread.Execute(() =>
            {
                var column = new SortPathProbeTextColumn
                {
                    Binding = new Binding { Path = new PropertyPath("Name"), Mode = BindingMode.OneWay },
                };

                Verify.AreEqual("Name", column.ReadSortMemberPath(),
                    "An empty SortMemberPath falls back to Binding.Path.Path (TableView.idl:175, :192).");

                column.SortMemberPath = "Role";

                Verify.AreEqual("Role", column.ReadSortMemberPath(),
                    "An explicit path takes priority over the binding fallback.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a base column with no path and no comparer reports no sort key.")]
        public void VerifyBaseColumnWithNoPathAndNoComparerHasNoSortKey()
        {
            RunOnUIThread.Execute(() =>
            {
                var column = new SortPathProbeColumn();

                Verify.AreEqual(string.Empty, column.ReadSortMemberPath(),
                    "The base column reports an empty key when nothing was configured (TableView.idl:175-177).");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies the sort pipeline consults a custom column's GetSortMemberPathCore override.")]
        public void VerifyCustomColumnSortMemberPathOverrideDrivesSort()
        {
            TableView tableView = null;
            ProbeColumn column = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateTableViewShell(MakeShapedPeople(), 500, 700);

                // The column never mentions Role in a Binding: the only route to it is the override.
                column = new ProbeColumn { Header = "Custom", ValuePath = "Role" };
                tableView.Columns.Add(column);

                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() => tableView.SortByColumn(column, SortDirection.Ascending));

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                var roles = RowValues(tableView, person => person.Role);

                VerifySequence(
                    new[] { "Architect", "Designer", "Designer", "Engineer", "Engineer", "Engineer" },
                    roles,
                    "an overridden GetSortMemberPathCore must drive the sort key");
            });
        }

        #endregion

        #region 7.2 Programmatic sort state

        [TestMethod]
        [TestProperty("Description", "Verifies SortByColumn sets the requested direction on the column.")]
        [TestProperty("data:Direction", "{Ascending, Descending, None}")]
        public void VerifySortByColumnSetsDirection()
        {
            var directionName = ((string)TestContext.DataRow["Direction"]).Trim();
            var direction = (SortDirection)Enum.Parse(typeof(SortDirection), directionName);
            Log.Comment($"Direction under test: {directionName}.");

            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateSortTable();
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() => tableView.SortByColumn(tableView.Columns[0], direction));

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
                Verify.AreEqual(direction, tableView.Columns[0].SortDirection,
                    $"{directionName}: the column must report the direction that was requested."));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies sorting does not narrow the header content, because its padding already reserves the indicator.")]
        public void VerifySortingDoesNotChangeHeaderContentWidth()
        {
            TableView tableView = null;
            double unsortedWidth = 0.0;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateSortTable();
                tableView.Columns[0].Width = new GridLength(200.0, GridUnitType.Pixel);
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                unsortedWidth = GetHeaderPresenter(tableView, 0).ActualWidth;
                Verify.IsGreaterThan(unsortedWidth, 0.0, "The header content must have been laid out.");
                tableView.SortByColumn(tableView.Columns[0], SortDirection.Ascending);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(unsortedWidth, GetHeaderPresenter(tableView, 0).ActualWidth,
                    "Sorting must not take the indicator's width from the header content a second time.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies SortByColumn returns true only when the sort state actually changed.")]
        public void VerifySortByColumnReturnsWhetherStateChanged()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateSortTable();
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                var column = tableView.Columns[0];

                Verify.IsFalse(tableView.SortByColumn(column, SortDirection.None),
                    "Clearing an already-unsorted column changes nothing (TableView.idl:587).");
                Verify.IsTrue(tableView.SortByColumn(column, SortDirection.Ascending),
                    "The first real sort changes the state.");
                Verify.IsFalse(tableView.SortByColumn(column, SortDirection.Ascending),
                    "Re-applying the same direction to the same column changes nothing.");
                Verify.IsTrue(tableView.SortByColumn(column, SortDirection.Descending),
                    "Flipping the direction changes the state.");
                Verify.IsTrue(tableView.SortByColumn(column, SortDirection.None),
                    "Clearing a sorted column changes the state.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies sorting a second column clears the first column's sort state.")]
        public void VerifySortByColumnReplacesPreviousColumnSort()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateSortTable();
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() => tableView.SortByColumn(tableView.Columns[0], SortDirection.Ascending));

            SettleLayout(tableView);

            RunOnUIThread.Execute(() => tableView.SortByColumn(tableView.Columns[1], SortDirection.Descending));

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(SortDirection.None, tableView.Columns[0].SortDirection,
                    "Sorting is single-column: the previous column must be cleared (TableView.idl:180-181).");
                Verify.AreEqual(SortDirection.Descending, tableView.Columns[1].SortDirection,
                    "The newly sorted column holds the active state.");

                var roles = RowValues(tableView, person => person.Role);

                VerifySequence(
                    new[] { "Engineer", "Engineer", "Engineer", "Designer", "Designer", "Architect" },
                    roles,
                    "the rows must follow the column that currently owns the sort");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies repeated ToggleSortDirection calls walk the sequence named by the column's SortCycle, and wrap.")]
        [TestProperty("data:Cycle", "{AscendingDescending, AscendingDescendingNone, DescendingAscending, DescendingAscendingNone}")]
        public void VerifyToggleSortDirectionCyclesPerSortCycle()
        {
            var cycleName = ((string)TestContext.DataRow["Cycle"]).Trim();
            var cycle = (TableViewSortCycle)Enum.Parse(typeof(TableViewSortCycle), cycleName);
            Log.Comment($"Sort cycle under test: {cycleName}.");

            var expected = cycle switch
            {
                TableViewSortCycle.AscendingDescending => new[]
                {
                    SortDirection.Ascending, SortDirection.Descending, SortDirection.Ascending,
                },
                TableViewSortCycle.AscendingDescendingNone => new[]
                {
                    SortDirection.Ascending, SortDirection.Descending, SortDirection.None, SortDirection.Ascending,
                },
                TableViewSortCycle.DescendingAscending => new[]
                {
                    SortDirection.Descending, SortDirection.Ascending, SortDirection.Descending,
                },
                _ => new[]
                {
                    SortDirection.Descending, SortDirection.Ascending, SortDirection.None, SortDirection.Descending,
                },
            };

            TableView tableView = null;
            var actual = new List<SortDirection>();

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateSortTable();
                tableView.Columns[0].SortCycle = cycle;
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                var column = tableView.Columns[0];

                for (var i = 0; i < expected.Length; i++)
                {
                    tableView.ToggleSortDirection(column);
                    actual.Add(column.SortDirection);
                }
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                var detail = $"Expected [{string.Join(", ", expected)}], saw [{string.Join(", ", actual)}].";

                for (var i = 0; i < expected.Length; i++)
                {
                    // The final step is the wrap: it proves the cycle repeats rather than terminating.
                    Verify.AreEqual(expected[i], actual[i],
                        $"{cycleName}: toggle {i + 1} of {expected.Length}. {detail}");
                }
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies ToggleSortDirection sorts a column whose CanUserSort is false: the flag gates user input only.")]
        public void VerifyToggleSortDirectionIgnoresCanUserSort()
        {
            TableView tableView = null;
            var returned = false;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateSortTable();
                tableView.Columns[0].CanUserSort = false;
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() => returned = tableView.ToggleSortDirection(tableView.Columns[0]));

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(returned, "A programmatic toggle must not be gated by CanUserSort.");
                Verify.AreEqual(SortDirection.Ascending, tableView.Columns[0].SortDirection, "The toggle must sort ascending.");
                VerifySequence(NameAscendingOrder, RowValues(tableView, person => person.Name), "the toggle must sort the rows");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies CanUserSort = false withholds header Invoke but SortByColumn still sorts the column.")]
        public void VerifySortByColumnIgnoresCanUserSort()
        {
            TableView tableView = null;
            var returned = false;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateSortTable();
                tableView.CanUserSortColumns = true;
                tableView.Columns[0].CanUserSort = false;
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsNull(CreateColumnHeaderPeer(tableView, tableView.Columns[0]).GetPattern(PatternInterface.Invoke),
                    "CanUserSort = false must withhold the user's Invoke affordance.");
                returned = tableView.SortByColumn(tableView.Columns[0], SortDirection.Ascending);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(returned, "SortByColumn must not be gated by CanUserSort.");
                Verify.AreEqual(SortDirection.Ascending, tableView.Columns[0].SortDirection, "The column must report its sort.");
                VerifySequence(NameAscendingOrder, RowValues(tableView, person => person.Name), "SortByColumn must sort the rows");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies setting CanUserSort = false on the sorted column keeps its sort and raises no Sorting/Sorted.")]
        public void VerifyCanUserSortFalseKeepsActiveSort()
        {
            TableView tableView = null;
            var events = 0;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateSortTable();
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(tableView.SortByColumn(tableView.Columns[0], SortDirection.Ascending), "Baseline: the column must sort.");
                tableView.Sorting += (sender, args) => events++;
                tableView.Sorted += (sender, args) => events++;
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() => tableView.Columns[0].CanUserSort = false);

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(SortDirection.Ascending, tableView.Columns[0].SortDirection, "Opting out of user sorting must keep the applied sort.");
                VerifySequence(NameAscendingOrder, RowValues(tableView, person => person.Name), "the rows must stay sorted");
                Verify.AreEqual(0, events, "No Sorting or Sorted may be raised.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies setting SortDirection only displays: it moves the arrow and the header help text, clears the arrow on other columns, never reorders rows, and the next header Invoke cycles from it.")]
        public void VerifySettingSortDirectionIsDisplayOnly()
        {
            TableView tableView = null;
            string helpTextBefore = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateSortTable();
                tableView.CanUserSortColumns = true;
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                helpTextBefore = CreateColumnHeaderPeer(tableView, tableView.Columns[0]).GetHelpText();
                tableView.Columns[0].SortDirection = SortDirection.Ascending;
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(SortIndicatorDirection.Ascending, FindSortIndicator(tableView, 0)?.Direction ?? SortIndicatorDirection.None,
                    "The header arrow must show the set direction.");
                Verify.AreNotEqual(helpTextBefore, CreateColumnHeaderPeer(tableView, tableView.Columns[0]).GetHelpText(),
                    "The header's UIA help text must report the set direction.");
                VerifySequence(SourceOrderNames, RowValues(tableView, person => person.Name), "setting SortDirection must not reorder rows");

                tableView.Columns[1].SortDirection = SortDirection.Descending;
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(SortDirection.None, tableView.Columns[0].SortDirection, "Lighting another column must clear this one.");
                Verify.AreEqual(SortIndicatorDirection.None, FindSortIndicator(tableView, 0)?.Direction ?? SortIndicatorDirection.None,
                    "The cleared column's arrow must be off.");
                Verify.AreEqual(SortIndicatorDirection.Descending, FindSortIndicator(tableView, 1)?.Direction ?? SortIndicatorDirection.None,
                    "The newly set column must show its arrow.");
                VerifySequence(SourceOrderNames, RowValues(tableView, person => person.Name), "still no reshape");

                InvokeHeader(tableView, 1);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(SortDirection.Ascending, tableView.Columns[1].SortDirection,
                    "The next Invoke must cycle from the displayed Descending to Ascending.");
                VerifySequence(new[] { "Architect", "Designer", "Designer", "Engineer", "Engineer", "Engineer" },
                    RowValues(tableView, person => person.Role), "the Invoke must sort the rows");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies TableView.ClearSort clears a key-selector sort declared on the TableViewSource and keeps SortGroups.")]
        public void VerifyClearSortClearsSourceKeySelectorSortAndKeepsSortGroups()
        {
            TableView tableView = null;
            var returned = false;

            RunOnUIThread.Execute(() =>
            {
                var source = TableViewSource.From(MakeShapedPeople())
                    .GroupBy(o => Person(o).Role)
                    .SortGroups(SortDirection.Descending)
                    .Sort(o => Person(o).Name, SortDirection.Descending);
                tableView = CreateSortTable(source);
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                VerifySequence(new[] { "#Engineer", "Rafa", "Owen", "Diego", "#Designer", "Ines", "Asha", "#Architect", "Mei" },
                    GetProjectedLabels(tableView, includeCount: false), "precondition: groups descending, names descending");
                returned = tableView.ClearSort();
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(returned, "ClearSort must report that it cleared the source sort.");
                VerifySequence(new[] { "#Engineer", "Diego", "Rafa", "Owen", "#Designer", "Asha", "Ines", "#Architect", "Mei" },
                    GetProjectedLabels(tableView, includeCount: false), "the item sort must be cleared and the group order kept");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies ClearSort resets every column and restores source order.")]
        public void VerifyClearSortClearsEveryColumnAndRestoresSourceOrder()
        {
            TableView tableView = null;
            var returned = false;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateSortTable();
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() => tableView.SortByColumn(tableView.Columns[0], SortDirection.Ascending));

            SettleLayout(tableView);

            RunOnUIThread.Execute(() => returned = tableView.ClearSort());

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(returned, "ClearSort returns true when it cleared something (TableView.idl:592-593).");

                foreach (var column in tableView.Columns)
                {
                    Verify.AreEqual(SortDirection.None, column.SortDirection,
                        $"Column '{column.Header}' must report no sort direction after ClearSort.");
                }

                VerifySequence(SourceOrderNames, RowValues(tableView, person => person.Name),
                    "ClearSort must restore source order");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies ClearSort is a safe no-op that returns false when nothing is sorted.")]
        public void VerifyClearSortReturnsFalseWhenNothingIsSorted()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateSortTable();
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
                Verify.IsFalse(tableView.ClearSort(), "Clearing a never-sorted table changes nothing."));

            SettleLayout(tableView);

            RunOnUIThread.Execute(() => tableView.SortByColumn(tableView.Columns[0], SortDirection.Ascending));

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(tableView.ClearSort(), "The first clear after a real sort changes state.");
                Verify.IsFalse(tableView.ClearSort(), "A second consecutive clear is a no-op.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies CanUserSortColumns gates only the gesture, never the programmatic API.")]
        public void VerifyCanUserSortColumnsFalseStillAllowsProgrammaticSort()
        {
            TableView tableView = null;
            var returned = false;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateSortTable();
                tableView.CanUserSortColumns = false;
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
                returned = tableView.SortByColumn(tableView.Columns[0], SortDirection.Ascending));

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(returned, "SortByColumn must work while CanUserSortColumns is false (TableView.idl:577-579).");
                Verify.AreEqual(SortDirection.Ascending, tableView.Columns[0].SortDirection,
                    "The column must hold the programmatically applied direction.");

                VerifySequence(NameAscendingOrder, RowValues(tableView, person => person.Name),
                    "a programmatic sort must reorder rows even with the gesture gate closed");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies no sort affordance is built while CanUserSortColumns is false.")]
        public void VerifyCanUserSortColumnsFalseBuildsNoSortAffordance()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateSortTable();
                tableView.CanUserSortColumns = false;
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                var indicator = FindSortIndicator(tableView, 0);

                // "No affordance" is not spelled out as absent-versus-collapsed, so either satisfies it.
                var absent = indicator == null || indicator.Visibility == Visibility.Collapsed;
                Verify.IsTrue(absent,
                    "No sort affordance may be offered while CanUserSortColumns is false (TableView.idl:577-579).");

                tableView.CanUserSortColumns = true;
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                var indicator = FindSortIndicator(tableView, 0);
                if (indicator == null)
                {
                    Verify.Fail("Re-opening the gate must build the sort affordance.");
                    return;
                }

                Verify.AreEqual(Visibility.Visible, indicator.Visibility,
                    "The affordance must be visible once the gate is open again.");
            });
        }

        #endregion

        #region 7.3 Sort ordering results

        [TestMethod]
        [TestProperty("Description", "Verifies sorting by a column orders rows by the column's sort member path.")]
        public void VerifySortBySortMemberPathOrdersRows()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateSortTable();
                tableView.Columns[0].SortMemberPath = "Name";
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() => tableView.SortByColumn(tableView.Columns[0], SortDirection.Ascending));

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
                VerifySequence(NameAscendingOrder, RowValues(tableView, person => person.Name),
                    "a column sort must order the rows by its sort member path"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies descending order is the exact reverse of ascending order.")]
        public void VerifySortDescendingReversesOrder()
        {
            TableView tableView = null;
            List<string> ascending = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateSortTable();
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() => tableView.SortByColumn(tableView.Columns[0], SortDirection.Ascending));

            SettleLayout(tableView);

            RunOnUIThread.Execute(() => ascending = RowValues(tableView, person => person.Name));

            RunOnUIThread.Execute(() => tableView.SortByColumn(tableView.Columns[0], SortDirection.Descending));

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                var expected = Enumerable.Reverse(ascending).ToList();
                VerifySequence(expected, RowValues(tableView, person => person.Name),
                    "descending must be the exact reverse of ascending");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a CustomSortComparer drives the row order and is actually called.")]
        public void VerifyCustomSortComparerOrdersRows()
        {
            TableView tableView = null;
            LengthThenOrdinalComparer comparer = null;

            RunOnUIThread.Execute(() =>
            {
                comparer = new LengthThenOrdinalComparer();

                tableView = CreateSortTable();
                tableView.Columns[0].CustomSortComparer = comparer;

                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() => tableView.SortByColumn(tableView.Columns[0], SortDirection.Ascending));

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsGreaterThan(comparer.CompareCount, 0,
                    "A supplied comparer must be the thing doing the comparing.");

                // Deliberately not alphabetical, so a default comparison cannot pass by coincidence.
                VerifySequence(
                    new[] { "Mei", "Asha", "Ines", "Owen", "Rafa", "Diego" },
                    RowValues(tableView, person => person.Name),
                    "the comparer's order must be the row order");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies CustomSortComparer takes precedence over SortMemberPath.")]
        public void VerifyCustomSortComparerTakesPrecedenceOverSortMemberPath()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateSortTable();

                // The path and the comparer produce different orders on this data, so exactly one of
                // them can be the explanation for what the test sees.
                tableView.Columns[0].SortMemberPath = "Name";
                tableView.Columns[0].CustomSortComparer = new LengthThenOrdinalComparer();

                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() => tableView.SortByColumn(tableView.Columns[0], SortDirection.Ascending));

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
                VerifySequence(
                    new[] { "Mei", "Asha", "Ines", "Owen", "Rafa", "Diego" },
                    RowValues(tableView, person => person.Name),
                    "the comparer must win over SortMemberPath (TableView.idl:179-180)"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies items with equal sort keys keep their source-relative order.")]
        public void VerifySortIsStableForEqualKeys()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateSortTable();
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            // Role has three ties: one Architect, two Designers, three Engineers.
            RunOnUIThread.Execute(() => tableView.SortByColumn(tableView.Columns[1], SortDirection.Ascending));

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
                VerifySequence(
                    new[] { "Mei", "Asha", "Ines", "Diego", "Rafa", "Owen" },
                    RowValues(tableView, person => person.Name),
                    "tied items must keep their source-relative order"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies an item added while sorted keeps the sort and lands at its sorted position.")]
        public void VerifySortSurvivesItemAddition()
        {
            TableView tableView = null;
            ObservableCollection<ShapedPerson> items = null;

            RunOnUIThread.Execute(() =>
            {
                items = new ObservableCollection<ShapedPerson>(MakeShapedPeople());
                tableView = CreateSortTable(items);
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() => tableView.SortByColumn(tableView.Columns[0], SortDirection.Ascending));

            SettleLayout(tableView);

            // "Nadia" sorts between "Mei" and "Owen", so appending at the end would be visible.
            RunOnUIThread.Execute(() => items.Add(new ShapedPerson("Nadia", "Engineer", "Platform")));

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(SortDirection.Ascending, tableView.Columns[0].SortDirection,
                    "A collection change must not drop the active sort.");

                VerifySequence(
                    new[] { "Asha", "Diego", "Ines", "Mei", "Nadia", "Owen", "Rafa" },
                    RowValues(tableView, person => person.Name),
                    "an item added while sorted must land at its sorted position");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies sorting a column with neither a path nor a comparer leaves the rows alone.")]
        public void VerifyColumnWithNoSortKeyDoesNotReorderRows()
        {
            TableView tableView = null;
            TableViewColumn keyless = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateSortTable();

                // A base column with no Binding to fall back on, so it has no sort key at all.
                keyless = new TableViewColumn { Header = "Keyless" };
                tableView.Columns.Add(keyless);

                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() => tableView.SortByColumn(keyless, SortDirection.Ascending));

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                VerifySequence(SourceOrderNames, RowValues(tableView, person => person.Name),
                    "a column with no sort key must not reorder anything (TableView.idl:175-177)");

                // What SortDirection should read here is not specified. Logged, not asserted.
                Log.Comment($"SortDirection on a keyless column after SortByColumn: {keyless.SortDirection}.");
            });
        }

        #endregion

        #region 7.4 Sort events

        [TestMethod]
        [TestProperty("Description", "Verifies Sorting is raised before the state changes, carrying the column and the pending direction.")]
        public void VerifySortingFiresBeforeStateChangeWithColumnAndDirection()
        {
            TableView tableView = null;
            TableViewColumn seenColumn = null;
            var seenDirection = SortDirection.None;
            var directionInsideHandler = SortDirection.Descending;
            var raiseCount = 0;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateSortTable();

                tableView.Sorting += (sender, args) =>
                {
                    raiseCount++;
                    seenColumn = args.Column;
                    seenDirection = args.Direction;
                    directionInsideHandler = tableView.Columns[0].SortDirection;
                };

                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() => tableView.SortByColumn(tableView.Columns[0], SortDirection.Ascending));

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(1, raiseCount, "Sorting must be raised exactly once for one sort.");
                Verify.AreEqual(tableView.Columns[0], seenColumn, "The args must carry the trigger column.");
                Verify.AreEqual(SortDirection.Ascending, seenDirection, "The args must carry the pending direction.");
                Verify.AreEqual(SortDirection.None, directionInsideHandler,
                    "Sorting is raised BEFORE the state change, so the handler must still see the old direction (TableView.idl:595-596).");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies cancelling Sorting leaves both the row order and every column's sort state untouched.")]
        public void VerifySortingCancelLeavesOrderAndSortStateUnchanged()
        {
            TableView tableView = null;
            var returned = true;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateSortTable();
                tableView.Sorting += (sender, args) => args.Cancel = true;
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
                returned = tableView.SortByColumn(tableView.Columns[0], SortDirection.Ascending));

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsFalse(returned, "A cancelled sort changed no state, so it must not report that it did.");

                foreach (var column in tableView.Columns)
                {
                    Verify.AreEqual(SortDirection.None, column.SortDirection,
                        $"Column '{column.Header}' must be untouched by a cancelled sort (TableView.idl:251-254).");
                }

                VerifySequence(SourceOrderNames, RowValues(tableView, person => person.Name),
                    "a cancelled sort must leave the row order alone");

                var indicator = FindSortIndicator(tableView, 0);
                if (indicator != null)
                {
                    Verify.AreEqual(SortIndicatorDirection.None, indicator.Direction,
                        "The header glyph is suppressed along with the sort state.");
                }
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies Sorted is raised once after the new order has been applied.")]
        public void VerifySortedFiresAfterSuccessfulSort()
        {
            TableView tableView = null;
            TableViewColumn seenColumn = null;
            var seenDirection = SortDirection.None;
            var directionInsideHandler = SortDirection.None;
            List<string> orderInsideHandler = null;
            var raiseCount = 0;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateSortTable();

                tableView.Sorted += (sender, args) =>
                {
                    raiseCount++;
                    seenColumn = args.Column;
                    seenDirection = args.Direction;
                    directionInsideHandler = tableView.Columns[0].SortDirection;
                    orderInsideHandler = RowValues(tableView, person => person.Name);
                };

                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() => tableView.SortByColumn(tableView.Columns[0], SortDirection.Ascending));

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(1, raiseCount, "Sorted must be raised exactly once per successful sort.");
                Verify.AreEqual(tableView.Columns[0], seenColumn, "The args must carry the column that changed.");
                Verify.AreEqual(SortDirection.Ascending, seenDirection, "The args must carry the applied direction.");
                Verify.AreEqual(SortDirection.Ascending, directionInsideHandler,
                    "Sorted is raised AFTER the change, so the handler must see the new direction (TableView.idl:598-599).");

                VerifySequence(NameAscendingOrder, orderInsideHandler,
                    "the new order must already be in place when Sorted runs");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a cancelled sort raises no Sorted event.")]
        public void VerifySortedDoesNotFireWhenCanceled()
        {
            TableView tableView = null;
            var sortedCount = 0;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateSortTable();
                tableView.Sorting += (sender, args) => args.Cancel = true;
                tableView.Sorted += (sender, args) => sortedCount++;
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                tableView.SortByColumn(tableView.Columns[0], SortDirection.Ascending);
                tableView.ToggleSortDirection(tableView.Columns[1]);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
                Verify.AreEqual(0, sortedCount,
                    "A cancelled sort must not report completion (TableView.idl:252-253)."));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies ClearSort raises both sort events with the null-column clear-all sentinel.")]
        public void VerifySortEventsUseNullColumnSentinelForClearSort()
        {
            TableView tableView = null;
            var sortingRaises = 0;
            var sortedRaises = 0;
            TableViewColumn sortingColumn = null;
            TableViewColumn sortedColumn = null;
            var sortingDirection = SortDirection.Ascending;
            var sortedDirection = SortDirection.Ascending;
            var handlersArmed = false;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateSortTable();

                tableView.Sorting += (sender, args) =>
                {
                    if (!handlersArmed)
                    {
                        return;
                    }

                    sortingRaises++;
                    sortingColumn = args.Column;
                    sortingDirection = args.Direction;
                };

                tableView.Sorted += (sender, args) =>
                {
                    if (!handlersArmed)
                    {
                        return;
                    }

                    sortedRaises++;
                    sortedColumn = args.Column;
                    sortedDirection = args.Direction;
                };

                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() => tableView.SortByColumn(tableView.Columns[0], SortDirection.Ascending));

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                handlersArmed = true;
                tableView.ClearSort();
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(1, sortingRaises, "ClearSort must raise Sorting once.");
                Verify.AreEqual(1, sortedRaises, "ClearSort must raise Sorted once.");
                Verify.IsNull(sortingColumn, "A null Column is the clear-all sentinel on Sorting (TableView.idl:245).");
                Verify.IsNull(sortedColumn, "A null Column is the clear-all sentinel on Sorted (TableView.idl:261).");
                Verify.AreEqual(SortDirection.None, sortingDirection, "A clear-all is a change to None.");
                Verify.AreEqual(SortDirection.None, sortedDirection, "A clear-all is a change to None.");
            });
        }

        #endregion

        #region App-set SortDirection, ThenSort and comparer robustness

        [TestMethod]
        [TestProperty("Description", "Verifies a column added with a preset SortDirection is tracked: invoking another header leaves one arrow, and ClearSort clears it.")]
        public void VerifyPresetSortDirectionColumnIsTrackedOnAttach()
        {
            TableView tableView = null;
            TableViewColumn preset = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateSortTable();
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                preset = MakeTextColumn("Team", "DepartmentName");
                preset.SortDirection = SortDirection.Descending;
                tableView.Columns.Add(preset);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(SortIndicatorDirection.Descending, FindSortIndicator(tableView, 2)?.Direction ?? SortIndicatorDirection.None,
                    "Precondition: the preset direction shows its arrow.");
                InvokeHeader(tableView, 0);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(SortDirection.None, preset.SortDirection, "Sorting another column must clear the preset column.");
                Verify.AreEqual(1, tableView.Columns.Count(c => c.SortDirection != SortDirection.None), "Exactly one column may be lit.");
                Verify.AreEqual(SortIndicatorDirection.None, FindSortIndicator(tableView, 2)?.Direction ?? SortIndicatorDirection.None,
                    "The preset column's arrow must be cleared.");
                preset.SortDirection = SortDirection.Ascending;
                Verify.IsTrue(tableView.ClearSort(), "ClearSort must report clearing the sort.");
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(tableView.Columns.All(c => c.SortDirection == SortDirection.None), "ClearSort must clear every column, including an app-set one.");
                Verify.AreEqual(SortIndicatorDirection.None, FindSortIndicator(tableView, 2)?.Direction ?? SortIndicatorDirection.None,
                    "No arrow may remain.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a SortDirection change callback that writes another column during SortByColumn leaves one lit column whose arrow matches.")]
        public void VerifySortDirectionCallbackWritingAnotherColumnStaysConsistent()
        {
            TableView tableView = null;
            var wrote = false;
            var sorted = new List<TableViewSortedEventArgs>();

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateSortTable();
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                tableView.Columns[0].RegisterPropertyChangedCallback(TableViewColumn.SortDirectionProperty, (s, dp) =>
                {
                    if (!wrote && tableView.Columns[0].SortDirection == SortDirection.Ascending)
                    {
                        wrote = true;
                        tableView.Columns[1].SortDirection = SortDirection.Descending;
                    }
                });
                tableView.Sorted += (s, e) => sorted.Add(e);
                tableView.SortByColumn(tableView.Columns[0], SortDirection.Ascending);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(wrote, "Precondition: the callback wrote the other column.");
                Verify.AreEqual(1, sorted.Count, "SortByColumn must raise Sorted once.");
                Verify.AreSame(tableView.Columns[0], sorted.FirstOrDefault()?.Column, "Sorted must name the requested column.");
                Verify.AreEqual(SortDirection.Ascending, sorted.FirstOrDefault()?.Direction ?? SortDirection.None,
                    "Sorted must report the requested direction, not one an app wrote during the sort.");
                Verify.AreEqual(1, tableView.Columns.Count(c => c.SortDirection != SortDirection.None), "Exactly one column may be lit.");
                for (var i = 0; i < 2; i++)
                {
                    var expected = tableView.Columns[i].SortDirection switch
                    {
                        SortDirection.Ascending => SortIndicatorDirection.Ascending,
                        SortDirection.Descending => SortIndicatorDirection.Descending,
                        _ => SortIndicatorDirection.None,
                    };
                    Verify.AreEqual(expected, FindSortIndicator(tableView, i)?.Direction ?? SortIndicatorDirection.None,
                        $"Column {i}'s arrow must match its SortDirection.");
                }
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies an app ThenSort after a header sort appends below it, so the header sort stays primary and lit.")]
        public void VerifyThenSortAfterHeaderSortKeepsHeaderPrimary()
        {
            TableView tableView = null;
            TableViewSource source = null;

            RunOnUIThread.Execute(() =>
            {
                source = TableViewSource.From(MakeShapedPeople());
                tableView = CreateSortTable(source);
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() => InvokeHeader(tableView, 1));

            SettleLayout(tableView);

            RunOnUIThread.Execute(() => source.ThenSort("Name", SortDirection.Descending));

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(SortDirection.Ascending, tableView.Columns[1].SortDirection, "The header sort must stay lit.");
                VerifySequence(new[] { "Mei", "Ines", "Asha", "Rafa", "Owen", "Diego" }, RowValues(tableView, person => person.Name),
                    "Role ascending must stay primary, with Name descending breaking ties");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies setting CustomSortComparer on a column whose SortDirection the app set does not reshape or raise sort events.")]
        public void VerifyCustomSortComparerOnAppSetDirectionDoesNotReshape()
        {
            TableView tableView = null;
            LengthThenOrdinalComparer comparer = null;
            var events = 0;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateSortTable();
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                tableView.Sorting += (s, e) => events++;
                tableView.Sorted += (s, e) => events++;
                tableView.Columns[0].SortDirection = SortDirection.Ascending;
                comparer = new LengthThenOrdinalComparer();
                tableView.Columns[0].CustomSortComparer = comparer;
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(0, events, "No Sorting or Sorted may be raised.");
                Verify.AreEqual(0, comparer.CompareCount, "The comparer must not be used to reshape.");
                VerifySequence(SourceOrderNames, RowValues(tableView, person => person.Name), "the rows must stay in source order");
                Verify.AreEqual(SortDirection.Ascending, tableView.Columns[0].SortDirection, "The displayed direction must stay.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a sort whose comparer throws while ranking leaves the previous sort, its arrow and its row order in place, raising Sorting but not Sorted.")]
        public void VerifyThrowingComparerKeepsThePreviousSort()
        {
            TableView tableView = null;
            var sortingCount = 0;
            var sortedCount = 0;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateSortTable();
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() => Verify.IsTrue(tableView.SortByColumn(tableView.Columns[0], SortDirection.Ascending), "Precondition: Name sorts."));

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                tableView.Sorting += (s, e) => sortingCount++;
                tableView.Sorted += (s, e) => sortedCount++;
                tableView.Columns[1].CustomSortComparer = new ThrowingSortComparer();
                try
                {
                    tableView.SortByColumn(tableView.Columns[1], SortDirection.Ascending);
                }
                catch (Exception e)
                {
                    Log.Comment($"The comparer fault surfaced: {e.Message}");
                }
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(SortDirection.Ascending, tableView.Columns[0].SortDirection, "The previous sort must stay lit.");
                Verify.AreEqual(SortDirection.None, tableView.Columns[1].SortDirection, "The failed sort must not light its column.");
                VerifySequence(NameAscendingOrder, RowValues(tableView, person => person.Name), "the previous order must survive");
                Verify.AreEqual(1, sortingCount, "Sorting is raised before the comparer runs.");
                Verify.AreEqual(0, sortedCount, "Sorted must not be raised for a sort that was not committed.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies setting CustomSortComparer on a column lit by an app TableViewSource sort raises no events and keeps the app's sort level.")]
        public void VerifyCustomSortComparerOnAppSourceSortedColumnKeepsAppSort()
        {
            TableView tableView = null;
            TableViewSource source = null;
            LengthThenOrdinalComparer comparer = null;
            var events = 0;

            RunOnUIThread.Execute(() =>
            {
                source = TableViewSource.From(MakeShapedPeople());
                tableView = CreateSortTable(source);
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() => source.Sort("Name", SortDirection.Ascending));

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(SortDirection.Ascending, tableView.Columns[0].SortDirection, "Precondition: the app sort lights the Name column.");
                tableView.Sorting += (s, e) => events++;
                tableView.Sorted += (s, e) => events++;
                comparer = new LengthThenOrdinalComparer();
                tableView.Columns[0].CustomSortComparer = comparer;
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(0, events, "No Sorting or Sorted may be raised.");
                Verify.AreEqual(0, comparer.CompareCount, "The comparer must not replace the app's sort level.");
                VerifySequence(NameAscendingOrder, RowValues(tableView, person => person.Name), "the app's Name sort must stay");
                Verify.AreEqual(SortDirection.Ascending, tableView.Columns[0].SortDirection, "The column must stay lit.");
            });
        }

        #endregion

        #region 7.5 Sort edge cases

        [TestMethod]
        [TestProperty("Description", "Verifies two columns naming the same sort member path keep independent sort state.")]
        public void VerifyDuplicateSortMemberPathsTrackColumnIdentity()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateTableViewShell(MakeShapedPeople(), 600, 700);

                // The IDL calls this shape out explicitly: one column formats a value, another ranks
                // the same one. Both name the same path.
                tableView.Columns.Add(new TableViewTextColumn
                {
                    Header = "Name",
                    Binding = new Binding { Path = new PropertyPath("Name"), Mode = BindingMode.OneWay },
                    SortMemberPath = "Name",
                });

                tableView.Columns.Add(new TableViewTextColumn
                {
                    Header = "Name again",
                    Binding = new Binding { Path = new PropertyPath("Name"), Mode = BindingMode.OneWay },
                    SortMemberPath = "Name",
                });

                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() => tableView.SortByColumn(tableView.Columns[1], SortDirection.Ascending));

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(SortDirection.Ascending, tableView.Columns[1].SortDirection,
                    "The sorted column holds the state.");
                Verify.AreEqual(SortDirection.None, tableView.Columns[0].SortDirection,
                    "Sort state is keyed on column identity, not on the path (TableView.idl:177-181).");

                VerifySequence(NameAscendingOrder, RowValues(tableView, person => person.Name),
                    "a duplicate path must still sort correctly");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies removing the sorted column leaves the control coherent and still sortable.")]
        public void VerifyRemovingSortedColumnClearsSortState()
        {
            TableView tableView = null;
            TableViewColumn removed = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateSortTable();
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                removed = tableView.Columns[0];
                tableView.SortByColumn(removed, SortDirection.Ascending);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() => tableView.Columns.Remove(removed));

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                foreach (var column in tableView.Columns)
                {
                    Verify.AreEqual(SortDirection.None, column.SortDirection,
                        $"No remaining column may claim the sort that left with column '{removed.Header}'.");
                }

                // Whether the rows revert to source order or keep the departed column's ordering is
                // not specified anywhere. Logged for the spec question, not asserted.
                Log.Comment($"Row order after removing the sorted column: [{string.Join(", ", RowValues(tableView, person => person.Name))}].");

                // The real risk is a dangling reference to the removed column on the next sort.
                Verify.IsTrue(tableView.SortByColumn(tableView.Columns[0], SortDirection.Descending),
                    "The table must still be sortable after losing the sorted column.");
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
                Verify.AreEqual(SortDirection.Descending, tableView.Columns[0].SortDirection,
                    "The post-removal sort must take effect normally."));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a column sort and a TableViewSource sort reconcile, last writer winning.")]
        public void VerifySortReconcilesWithTableViewSourceSort()
        {
            TableView tableView = null;
            TableViewSource source = null;

            RunOnUIThread.Execute(() =>
            {
                source = TableViewSource.From(MakeShapedPeople());
                tableView = CreateSortTable(source);
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() => tableView.SortByColumn(tableView.Columns[0], SortDirection.Ascending));

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
                VerifySequence(NameAscendingOrder, RowValues(tableView, person => person.Name),
                    "the column sort is the last writer so far"));

            // The source now writes the other axis. Exactly one axis is ever in force.
            RunOnUIThread.Execute(() => source.Sort("Role", SortDirection.Descending));

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                VerifySequence(
                    new[] { "Engineer", "Engineer", "Engineer", "Designer", "Designer", "Architect" },
                    RowValues(tableView, person => person.Role),
                    "the source sort must replace the column sort, not stack with it");

                Verify.AreEqual(SortDirection.None, tableView.Columns[0].SortDirection,
                    "A source-owned sort must clear the column indicator (TableView.idl:571-573).");
            });

            // And back the other way: the column writes last and must win again.
            RunOnUIThread.Execute(() => tableView.SortByColumn(tableView.Columns[0], SortDirection.Descending));

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                VerifySequence(
                    Enumerable.Reverse(NameAscendingOrder).ToList(),
                    RowValues(tableView, person => person.Name),
                    "the column sort must replace the source sort when it writes last");

                Verify.AreEqual(SortDirection.Descending, tableView.Columns[0].SortDirection,
                    "The column that wrote last owns the indicator.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies clearing a column lit by an app-declared TableViewSource sort removes that sort.")]
        [TestProperty("data:Clear", "{SortByColumnNone, HeaderInvoke}")]
        public void VerifyClearingColumnLitBySourceSortRemovesSourceSort()
        {
            var clear = ((string)TestContext.DataRow["Clear"]).Trim();
            TableView tableView = null;
            TableViewSource source = null;

            RunOnUIThread.Execute(() =>
            {
                source = TableViewSource.From(MakeShapedPeople());
                tableView = CreateSortTable(source);
                tableView.Columns[0].SortCycle = TableViewSortCycle.AscendingDescendingNone;
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() => source.Sort("Name", SortDirection.Descending));

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(SortDirection.Descending, tableView.Columns[0].SortDirection,
                    "Precondition: the source sort lights the Name column.");
                VerifySequence(Enumerable.Reverse(NameAscendingOrder).ToList(), RowValues(tableView, person => person.Name),
                    "precondition: the source sort orders the rows");

                switch (clear)
                {
                    case "SortByColumnNone":
                        tableView.SortByColumn(tableView.Columns[0], SortDirection.None);
                        break;
                    default:
                        var provider = CreateColumnHeaderPeer(tableView, tableView.Columns[0])
                            .GetPattern(PatternInterface.Invoke) as IInvokeProvider;
                        provider.Invoke();
                        break;
                }
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(SortDirection.None, tableView.Columns[0].SortDirection, $"{clear}: the column must be cleared.");
                VerifySequence(SourceOrderNames, RowValues(tableView, person => person.Name),
                    $"{clear}: the clear must remove the app-declared sort");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies moving, replacing or ungating the columns keeps an app-declared TableViewSource sort and re-lights the matching column.")]
        [TestProperty("data:Change", "{MoveColumn, ReplaceColumns, CanUserSortColumnsFalse}")]
        public void VerifyImplicitClearKeepsSourceSort()
        {
            var change = ((string)TestContext.DataRow["Change"]).Trim();
            TableView tableView = null;
            TableViewSource source = null;
            var sortedArgs = new List<TableViewSortedEventArgs>();

            RunOnUIThread.Execute(() =>
            {
                source = TableViewSource.From(MakeShapedPeople());
                tableView = CreateSortTable(source);
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() => source.Sort("Name", SortDirection.Descending));

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(SortDirection.Descending, tableView.Columns[0].SortDirection,
                    "Precondition: the source sort lights the Name column.");
                tableView.Sorted += (s, e) => sortedArgs.Add(e);

                switch (change)
                {
                    case "MoveColumn":
                        var column = tableView.Columns[0];
                        tableView.Columns.RemoveAt(0);
                        tableView.Columns.Insert(1, column);
                        break;
                    case "ReplaceColumns":
                        tableView.Columns.Clear();
                        tableView.Columns.Add(MakeTextColumn("Name"));
                        tableView.Columns.Add(MakeTextColumn("Role"));
                        break;
                    default:
                        tableView.CanUserSortColumns = false;
                        break;
                }
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                VerifySequence(Enumerable.Reverse(NameAscendingOrder).ToList(), RowValues(tableView, person => person.Name),
                    $"{change}: the app-declared sort must stay on the source");
                var nameColumn = tableView.Columns.First(c => c.Header as string == "Name");
                Verify.AreEqual(SortDirection.Descending, nameColumn.SortDirection,
                    $"{change}: the Name column must re-light from the source sort");
                Verify.IsFalse(sortedArgs.Any(e => e.Column == null && e.Direction == SortDirection.None),
                    $"{change}: a clear that left the source sort in place must not report that sorting was cleared");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies an app Sort that replaces a header sort lights its own column, and closing CanUserSortColumns in the same turn keeps it and reports no clear.")]
        public void VerifyAppSortReplacingHeaderSortSurvivesUngating()
        {
            TableView tableView = null;
            TableViewSource source = null;
            var sortedArgs = new List<TableViewSortedEventArgs>();

            RunOnUIThread.Execute(() =>
            {
                source = TableViewSource.From(MakeShapedPeople());
                tableView = CreateSortTable(source);
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(tableView.SortByColumn(tableView.Columns[0], SortDirection.Ascending), "Precondition: the Name column sorts.");
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                tableView.Sorted += (s, e) => sortedArgs.Add(e);
                // Sort replaces every item sort level, including the one the header installed.
                source.Sort("Role", SortDirection.Descending);
                tableView.CanUserSortColumns = false;
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsFalse(sortedArgs.Any(e => e.Column == null && e.Direction == SortDirection.None),
                    "Replacing the header sort with the app's sort must not report that sorting was cleared.");
                VerifySequence(new[] { "Engineer", "Engineer", "Engineer", "Designer", "Designer", "Architect" },
                    RowValues(tableView, person => person.Role), "the app's Role sort must order the rows");
                var roleColumn = tableView.Columns.First(c => c.Header as string == "Role");
                var nameColumn = tableView.Columns.First(c => c.Header as string == "Name");
                Verify.AreEqual(SortDirection.Descending, roleColumn.SortDirection, "The app's Role sort must light its column.");
                Verify.AreEqual(SortDirection.None, nameColumn.SortDirection, "The replaced Name sort must be gone.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies filter changes under an app sort no column describes raise no Sorted.")]
        [TestProperty("data:Sort", "{KeySelector, UnmatchedPath}")]
        public void VerifyFilterUnderUnmatchedAppSortRaisesNoSorted()
        {
            var sort = ((string)TestContext.DataRow["Sort"]).Trim();
            TableView tableView = null;
            TableViewSource source = null;
            int sortedCount = 0;

            RunOnUIThread.Execute(() =>
            {
                source = TableViewSource.From(MakeShapedPeople());
                tableView = CreateSortTable(source);
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                if (sort == "KeySelector")
                {
                    source.Sort(item => ((ShapedPerson)item).Name, SortDirection.Ascending);
                }
                else
                {
                    source.Sort("DepartmentName", SortDirection.Ascending);
                }
            });

            SettleLayout(tableView);

            for (int i = 0; i < 3; i++)
            {
                RunOnUIThread.Execute(() =>
                {
                    if (i == 0)
                    {
                        tableView.Sorted += (s, e) => sortedCount++;
                    }
                    source.Filter(_ => true);
                });
                SettleLayout(tableView);
            }

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(0, sortedCount, $"{sort}: a filter change must not report a sort change.");
                Verify.IsTrue(tableView.Columns.All(c => c.SortDirection == SortDirection.None), $"{sort}: no column describes the app's sort.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies turning sorting off for the table or the column leaves a column the app's source sort lit untouched and raises no sort events.")]
        [TestProperty("data:Change", "{CanUserSortColumnsFalse, CanSortFalse}")]
        public void VerifyUngatingKeepsAppLitColumnWithoutEvents()
        {
            var change = ((string)TestContext.DataRow["Change"]).Trim();
            TableView tableView = null;
            TableViewSource source = null;
            int sortingCount = 0;
            int sortedCount = 0;
            int directionChanges = 0;

            RunOnUIThread.Execute(() =>
            {
                source = TableViewSource.From(MakeShapedPeople());
                tableView = CreateSortTable(source);
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() => source.Sort("Name", SortDirection.Ascending));

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(SortDirection.Ascending, tableView.Columns[0].SortDirection, "Precondition: the source sort lights the Name column.");
                tableView.Sorting += (s, e) => sortingCount++;
                tableView.Sorted += (s, e) => sortedCount++;
                tableView.Columns[0].RegisterPropertyChangedCallback(TableViewColumn.SortDirectionProperty, (s, dp) => directionChanges++);
                if (change == "CanSortFalse")
                {
                    tableView.Columns[0].CanUserSort = false;
                }
                else
                {
                    tableView.CanUserSortColumns = false;
                }
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(0, sortingCount, "The table installed no sort, so there is nothing to clear and no Sorting.");
                Verify.AreEqual(0, sortedCount, "No Sorted may be raised.");
                Verify.AreEqual(0, directionChanges, "The column's SortDirection must not flip.");
                Verify.AreEqual(SortDirection.Ascending, tableView.Columns[0].SortDirection, "The column keeps describing the app's sort.");
                VerifySequence(NameAscendingOrder, RowValues(tableView, person => person.Name), "the app's sort stays on the source");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies closing CanUserSortColumns keeps the applied sort and its arrow, raises no Sorting or Sorted, and rebuilds the header without Invoke.")]
        public void VerifySortOptOutKeepsSortAndArrowWithoutEvents()
        {
            TableView tableView = null;
            var sortEvents = 0;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateSortTable();
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(tableView.SortByColumn(tableView.Columns[0], SortDirection.Ascending), "Precondition: the column sorts.");
                tableView.Sorting += (s, e) => sortEvents++;
                tableView.Sorted += (s, e) => { sortEvents++; throw new InvalidOperationException("Sorted handler fault"); };
                tableView.CanUserSortColumns = false;
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(0, sortEvents, "Closing the gate must raise no Sorting or Sorted.");
                Verify.AreEqual(SortDirection.Ascending, tableView.Columns[0].SortDirection, "The applied sort must stay.");
                VerifySequence(NameAscendingOrder, RowValues(tableView, person => person.Name), "the rows must stay sorted");
                Verify.AreEqual(SortIndicatorDirection.Ascending, FindSortIndicator(tableView, 0)?.Direction ?? SortIndicatorDirection.None,
                    "The arrow must keep showing the applied sort.");
                Verify.IsNull(CreateColumnHeaderPeer(tableView, tableView.Columns[0]).GetPattern(PatternInterface.Invoke),
                    "The rebuilt header must not offer Invoke while the gate is closed.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a clear queued by removing the sorted column leaves the app-declared sort of the next ItemsSource alone.")]
        public void VerifyColumnRemovalClearKeepsNewItemsSourceSort()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateSortTable(TableViewSource.From(MakeShapedPeople()));
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() => Verify.IsTrue(tableView.SortByColumn(tableView.Columns[1], SortDirection.Ascending),
                "Precondition: the Role column sorts."));

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                tableView.Columns.RemoveAt(1);
                var next = TableViewSource.From(MakeShapedPeople());
                next.Sort("Name", SortDirection.Descending);
                tableView.ItemsSource = next;
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                VerifySequence(Enumerable.Reverse(NameAscendingOrder).ToList(), RowValues(tableView, person => person.Name),
                    "the new ItemsSource must keep the sort the app declared on it");
                Verify.AreEqual(SortDirection.Descending, tableView.Columns[0].SortDirection,
                    "The Name column must reflect the new source's sort.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies invoking a column header toggles the sort and updates the sort indicator.")]
        public void VerifyHeaderInvokeTogglesSortAndUpdatesIndicator()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateSortTable();
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                var provider = CreateColumnHeaderPeer(tableView, tableView.Columns[0])
                    .GetPattern(PatternInterface.Invoke) as IInvokeProvider;

                if (provider == null)
                {
                    Verify.Fail("A sortable header must offer the Invoke pattern.");
                    return;
                }

                provider.Invoke();
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(SortDirection.Ascending, tableView.Columns[0].SortDirection,
                    "Invoking a header must advance the column's SortCycle by one step.");

                VerifySequence(NameAscendingOrder, RowValues(tableView, person => person.Name),
                    "the gesture must drive the same pipeline the API drives");

                var indicator = FindSortIndicator(tableView, 0);
                if (indicator == null)
                {
                    Verify.Fail("A sortable header must carry a SortIndicator.");
                    return;
                }

                Verify.AreEqual(SortIndicatorDirection.Ascending, indicator.Direction,
                    "The glyph must follow the sort state.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies invoking a header over a list holding one object twice neither throws nor changes sort state.")]
        public void VerifySortOverDuplicateItemFailsSafely()
        {
            TableView tableView = null;
            int sortingCount = 0;
            int sortedCount = 0;

            RunOnUIThread.Execute(() =>
            {
                var dup = new ShapedPerson("Zoe", "Engineer", "Platform");
                tableView = CreateSortTable(new List<ShapedPerson>
                {
                    dup,
                    new ShapedPerson("Bea", "Designer", "Studio"),
                    dup,
                    new ShapedPerson("Cal", "Architect", "Research"),
                });
                tableView.Sorting += (sender, args) => sortingCount++;
                tableView.Sorted += (sender, args) => sortedCount++;
                LoadContent(tableView);
            });

            SettleLayout(tableView);
            RunOnUIThread.Execute(() => InvokeHeader(tableView, 0));
            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(SortDirection.None, tableView.Columns[0].SortDirection,
                    "A refused sort must leave the column unsorted.");
                VerifySequence(new[] { "Zoe", "Bea", "Zoe", "Cal" }, RowValues(tableView, person => person.Name),
                    "a refused sort must keep the source order");
                Verify.AreEqual(0, sortingCount, "Sorting must not be raised for a refused sort.");
                Verify.AreEqual(0, sortedCount, "Sorted must not be raised for a refused sort.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies invoking a header over a list holding a null item neither throws nor changes sort state.")]
        public void VerifySortOverNullItemFailsSafely()
        {
            TableView tableView = null;
            int sortingCount = 0;
            int sortedCount = 0;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateSortTable(new List<ShapedPerson>
                {
                    new ShapedPerson("Zoe", "Engineer", "Platform"),
                    null,
                    new ShapedPerson("Bea", "Designer", "Studio"),
                });
                tableView.Sorting += (sender, args) => sortingCount++;
                tableView.Sorted += (sender, args) => sortedCount++;
                LoadContent(tableView);
            });

            SettleLayout(tableView);
            RunOnUIThread.Execute(() => InvokeHeader(tableView, 0));
            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(SortDirection.None, tableView.Columns[0].SortDirection,
                    "A refused sort must leave the column unsorted.");
                VerifySequence(new[] { "Zoe", "<not a ShapedPerson>", "Bea" }, RowValues(tableView, person => person.Name),
                    "a refused sort must keep the source order");
                Verify.AreEqual(0, sortingCount, "Sorting must not be raised for a refused sort.");
                Verify.AreEqual(0, sortedCount, "Sorted must not be raised for a refused sort.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies SortByColumn returns false over duplicate rows, and sorts once the duplicate is removed.")]
        public void VerifySortByColumnReturnsFalseForDuplicateRows()
        {
            TableView tableView = null;
            ObservableCollection<ShapedPerson> items = null;

            RunOnUIThread.Execute(() =>
            {
                var dup = new ShapedPerson("Zoe", "Engineer", "Platform");
                items = new ObservableCollection<ShapedPerson>
                {
                    dup,
                    new ShapedPerson("Bea", "Designer", "Studio"),
                    dup,
                    new ShapedPerson("Cal", "Architect", "Research"),
                };
                tableView = CreateSortTable(items);
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsFalse(tableView.SortByColumn(tableView.Columns[0], SortDirection.Ascending),
                    "Sorting rows that include one object twice must be refused.");
                Verify.AreEqual(SortDirection.None, tableView.Columns[0].SortDirection,
                    "A refused sort must leave the column unsorted.");

                items.RemoveAt(2);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(tableView.SortByColumn(tableView.Columns[0], SortDirection.Ascending),
                    "With the duplicate gone the sort must apply.");
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(SortDirection.Ascending, tableView.Columns[0].SortDirection);
                VerifySequence(new[] { "Bea", "Cal", "Zoe" }, RowValues(tableView, person => person.Name),
                    "the rows must sort once every row is a distinct object");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a CustomSortComparer's ranks do not keep a removed row alive.")]
        public void VerifyCustomSortReleasesRemovedRow()
        {
            VerifyCustomSortReleasesRemovedRowCore(filtered: false);
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a CustomSortComparer's ranks do not keep a row removed under a filter alive.")]
        public void VerifyCustomSortReleasesRowRemovedUnderFilter()
        {
            VerifyCustomSortReleasesRemovedRowCore(filtered: true);
        }

        private void VerifyCustomSortReleasesRemovedRowCore(bool filtered)
        {
            TableView tableView = null;
            ObservableCollection<ShapedPerson> items = null;
            WeakReference weak = null;

            RunOnUIThread.Execute(() =>
            {
                items = new ObservableCollection<ShapedPerson>();
                for (int i = 0; i < 199; i++)
                {
                    items.Add(new ShapedPerson($"P{i:D3}", "Engineer", "Platform"));
                }
                // Longest name, so the comparer sorts it last, and last in the source: never realized.
                weak = AddUntracked(items, "Zed-last-row");

                object source = filtered ? (object)TableViewSource.From(items).Filter(_ => true) : items;
                tableView = CreateTableView(source, headers: new[] { "Name", "Role" }, width: 600, height: 150);
                tableView.Columns[0].CustomSortComparer = new LengthThenOrdinalComparer();
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
                Verify.IsTrue(tableView.SortByColumn(tableView.Columns[0], SortDirection.Ascending),
                    "Precondition: the comparer sort applies."));

            SettleLayout(tableView);
            RunOnUIThread.Execute(() => items.RemoveAt(items.Count - 1));
            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                GC.Collect();
                GC.WaitForPendingFinalizers();
                GC.Collect();

                Verify.IsFalse(weak.IsAlive, "A removed row must not stay alive in the comparer's ranks.");
                GC.KeepAlive(tableView);
            });
        }

        // Not inlined, so no local here keeps the item alive for the caller's GC.
        [MethodImpl(MethodImplOptions.NoInlining)]
        private static WeakReference AddUntracked(ObservableCollection<ShapedPerson> items, string name)
        {
            var person = new ShapedPerson(name, "Engineer", "Platform");
            items.Add(person);
            return new WeakReference(person);
        }

        #endregion
    }

    // A probe exposing the protected GetSortMemberPathCore so a test can assert path resolution
    // directly rather than inferring it from a resulting row order.
    internal partial class SortPathProbeColumn : TableViewColumn
    {
        public string ReadSortMemberPath() => GetSortMemberPathCore();
    }

    internal partial class SortPathProbeTextColumn : TableViewTextColumn
    {
        public string ReadSortMemberPath() => GetSortMemberPathCore();
    }

    internal sealed partial class ThrowingSortComparer : ITableViewSortComparer
    {
        public int Compare(object left, object right) => throw new InvalidOperationException("Comparer fault");
    }

    // Orders by name length first, then ordinally. Deliberately not alphabetical, so a default
    // comparison cannot produce this order by coincidence.
    internal sealed class LengthThenOrdinalComparer : ITableViewSortComparer
    {
        public int CompareCount { get; private set; }

        public int Compare(object left, object right)
        {
            CompareCount++;

            var leftName = (left as ShapedPerson)?.Name ?? string.Empty;
            var rightName = (right as ShapedPerson)?.Name ?? string.Empty;

            if (leftName.Length != rightName.Length)
            {
                return leftName.Length - rightName.Length;
            }

            return string.CompareOrdinal(leftName, rightName);
        }
    }

    internal static class TableViewSortingTestHelpers
    {
        // The six-person fixture in source order. Named so a test asserting "unchanged" says what
        // unchanged means.
        internal static readonly string[] SourceOrderNames =
            { "Asha", "Diego", "Mei", "Rafa", "Ines", "Owen" };

        internal static readonly string[] NameAscendingOrder =
            { "Asha", "Diego", "Ines", "Mei", "Owen", "Rafa" };

        internal static TableView CreateSortTable() => CreateSortTable(MakeShapedPeople());

        // Two text columns with bindings but no explicit SortMemberPath, so the common authoring
        // shape (sort key resolved through the binding fallback) is what most tests exercise.
        internal static TableView CreateSortTable(object itemsSource)
            => CreateTableView(itemsSource, headers: new[] { "Name", "Role" }, width: 600, height: 700);

        // One projected value per row, in projected order, read from the repeater's view so the
        // result describes the projection rather than whatever happens to be realized.
        internal static List<string> RowValues(TableView tableView, Func<ShapedPerson, string> selector)
            => GetProjectedItems(tableView)
                .Select(item => item is ShapedPerson person ? selector(person) : "<not a ShapedPerson>")
                .ToList();

        // The SortIndicator built into a header, or null when no affordance was built. Named
        // "TableViewSortIndicator" by TableView::AppendSortIndicatorVisual.
        internal static SortIndicator FindSortIndicator(TableView tableView, int columnIndex)
            => FindVisualChildrenByType<SortIndicator>(GetHeaderCell(tableView, columnIndex)).FirstOrDefault();

        // Call on the UI thread.
        internal static void InvokeHeader(TableView tableView, int columnIndex)
        {
            var provider = CreateColumnHeaderPeer(tableView, tableView.Columns[columnIndex])
                .GetPattern(PatternInterface.Invoke) as IInvokeProvider;
            if (provider == null)
            {
                Verify.Fail("A sortable header must offer the Invoke pattern.");
                return;
            }

            provider.Invoke();
        }
    }
}
