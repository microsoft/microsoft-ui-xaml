// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Private.Controls;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Automation.Peers;
using Microsoft.UI.Xaml.Automation.Provider;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Tabular;
using MUXControlsTestApp.Utilities;
using System;
using System.Collections.Generic;
using System.Linq;

using WEX.Logging.Interop;
using WEX.TestExecution;
using WEX.TestExecution.Markup;

using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewTestHelpers;
using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewToolTipTestHelpers;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    // Header drags driven through TableViewTestHooks, which runs the drag state machine the header row's
    // manipulation drives, without a pointer. They need no synthetic input, so they run on every agent.
    public partial class TableViewColumnReorderTests
    {
        [TestMethod]
        [TestProperty("Description", "Verifies in LTR and RTL that a header dropped past its neighbour moves its column, raises ColumnReordered once with Columns indexes that count collapsed columns, and updates the header peer's position; a drop where the drag started raises nothing.")]
        public void VerifyHookDragMovesColumnAndRaisesColumnReordered()
        {
            foreach (var flowDirection in new[] { FlowDirection.LeftToRight, FlowDirection.RightToLeft })
            {
                Log.Comment($"FlowDirection {flowDirection}.");
                TableView tableView = null;
                TableViewColumn name = null;
                var events = new List<string>();
                RunOnUIThread.Execute(() =>
                {
                    tableView = CreateReorderTable("Name", "Hidden", "Role", "City");
                    tableView.FlowDirection = flowDirection;
                    tableView.Columns[1].Visibility = Visibility.Collapsed;
                    name = tableView.Columns[0];
                    tableView.ColumnReordered += (s, e) => events.Add($"{e.Column.Header}:{e.OldIndex}->{e.NewIndex}:{ReferenceEquals(s.Columns[e.NewIndex], e.Column)}");
                    LoadContent(tableView);
                });
                SettleLayout(tableView);

                RunOnUIThread.Execute(() =>
                {
                    SnapshotHeaders(tableView);
                    var nameHeader = GetColumnHeader(tableView, name);
                    var roleHeader = GetColumnHeader(tableView, tableView.Columns[2]);
                    var travel = TravelInto(tableView, name, tableView.Columns[2], 0.25);
                    Verify.IsTrue(TableViewTestHooks.BeginColumnReorderDrag(tableView, name), "The drag starts.");
                    Verify.IsTrue(TableViewTestHooks.UpdateColumnReorderDrag(tableView, Physical(flowDirection, travel)));
                    tableView.UpdateLayout();
                    Verify.IsLessThan(Math.Abs(roleHeader.Translation.X + nameHeader.ActualWidth), 0.5, "Role slides into Name's slot once the pointer enters it.");
                    Verify.IsLessThan(Math.Abs(nameHeader.Translation.X - travel), 1.0, "Name follows the pointer.");
                    Verify.AreEqual("Pressed", CurrentState(nameHeader, "CommonStates"), "The dragged header stays Pressed.");
                    Verify.AreEqual(0, events.Count, "Nothing moves before the drop.");
                    Verify.IsTrue(TableViewTestHooks.CompleteColumnReorderDrag(tableView, Physical(flowDirection, travel)), "The drop is taken.");
                    Verify.IsFalse(TableViewTestHooks.IsColumnReorderDragging(tableView), "The drop ends the drag.");
                });
                SettleLayout(tableView);

                RunOnUIThread.Execute(() =>
                {
                    VerifyOrder(tableView, "Hidden,Role,Name,City", "after the drop");
                    Verify.AreEqual("Name:0->2:True", string.Join("|", events), "ColumnReordered is raised once with Columns indexes, collapsed columns included.");
                    Verify.IsFalse(PreviewShown(tableView), "After the drop every header is back at Translation 0.");
                    // The move announcement names the header and its visible position.
                    var peer = FrameworkElementAutomationPeer.CreatePeerForElement(GetColumnHeader(tableView, name));
                    Verify.AreEqual("Name 2/3", $"{peer.GetName()} {peer.GetPositionInSet()}/{peer.GetSizeOfSet()}", "The header peer reports the moved column's visible position.");

                    Verify.IsTrue(TableViewTestHooks.BeginColumnReorderDrag(tableView, name), "A second drag starts.");
                    Verify.IsTrue(TableViewTestHooks.CompleteColumnReorderDrag(tableView, 0.0));
                });
                SettleLayout(tableView);
                RunOnUIThread.Execute(() =>
                {
                    VerifyOrder(tableView, "Hidden,Role,Name,City", "after a drop where the drag started");
                    Verify.AreEqual(1, events.Count, "A drop where the drag started raises nothing.");
                });
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a dragged column keeps an Auto width it has grown to, the resize hold of a locked Star column, its sort and the row order, with no Sorting, and that an app's RemoveAt + Insert of the same column keeps the sort but loses the grown width and the hold.")]
        public void VerifyHookDragKeepsWidthsAndSortUnlikeAnAppMove()
        {
            TableView tableView = null;
            List<NotifyingPerson> items = null;
            TableViewColumn name = null;
            TableViewColumn role = null;
            double grownRole = 0;
            Dictionary<TableViewColumn, double> widths = null;
            List<object> rowsBefore = null;
            int sorting = 0;

            RunOnUIThread.Execute(() =>
            {
                items = MakeNotifyingItems();
                items[0].Role = "A role title long enough to widen its Auto column";
                tableView = CreateTableView(items, headers: new[] { "Name", "Role" }, width: 600, height: 300);
                tableView.Columns.Add(MakeTextTemplateColumn("Team", bound: false));
                tableView.CanUserReorderColumns = true;
                tableView.CanUserSortColumns = true;
                tableView.CanUserResizeColumns = true;
                name = tableView.Columns[0];
                role = tableView.Columns[1];
                name.Width = new GridLength(1, GridUnitType.Star);
                name.CanUserResize = false;
                role.Width = GridLength.Auto;
                tableView.Columns[2].Width = new GridLength(90);
                LoadContent(tableView);
            });
            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                grownRole = role.ActualWidth;
                items[0].Role = "QA";
            });
            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(grownRole, role.ActualWidth, "Precondition: the Auto column keeps the width it grew to.");
                Verify.IsTrue(tableView.SortByColumn(role, SortDirection.Ascending), "Precondition: Role sorts.");
                // Resizing a column after the locked Star column gives the Star column a resize hold.
                var gripper = RequireGripper(tableView, 2);
                gripper.BeginDrag();
                gripper.TryDrag(40.0);
                gripper.EndDrag(false);
            });
            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                widths = tableView.Columns.ToDictionary(c => c, c => c.ActualWidth);
                rowsBefore = GetProjectedItems(tableView);
                tableView.Sorting += (s, e) => sorting++;
                var travel = TravelInto(tableView, role, name, 0.75);
                Verify.IsTrue(TableViewTestHooks.BeginColumnReorderDrag(tableView, role), "The drag starts.");
                Verify.IsTrue(TableViewTestHooks.CompleteColumnReorderDrag(tableView, travel), "The drop is taken.");
            });
            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                VerifyOrder(tableView, "Role,Name,Team", "after the drag");
                foreach (var column in tableView.Columns)
                {
                    Verify.IsLessThan(Math.Abs(column.ActualWidth - widths[column]), 0.5, $"{column.Header} keeps its ActualWidth after the drag.");
                }
                Verify.AreEqual(GridUnitType.Auto, role.Width.GridUnitType, "A user move does not rewrite Width.");
                Verify.AreEqual(SortDirection.Ascending, role.SortDirection, "The sort follows the moved column.");
                Verify.AreEqual(0, sorting, "A user move raises no Sorting.");
                Verify.IsTrue(rowsBefore.SequenceEqual(GetProjectedItems(tableView)), "The row order does not change.");

                // The same move made by the app.
                tableView.Columns.RemoveAt(0);
                tableView.Columns.Insert(1, role);
            });
            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                VerifyOrder(tableView, "Name,Role,Team", "after the app's move");
                Verify.AreEqual(SortDirection.Ascending, role.SortDirection, "An app move keeps the sort too.");
                Verify.IsLessThan(role.ActualWidth, grownRole - 0.5, "An app move measures the Auto column again.");
                Verify.IsGreaterThan(Math.Abs(name.ActualWidth - widths[name]), 0.5, "An app move ends the locked Star column's resize hold.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a drag of a sortable header raises no Sorting and sorts nothing, and the moved header still sorts afterwards.")]
        public void VerifyHookDragWithSortingOnDoesNotSort()
        {
            TableView tableView = null;
            TableViewColumn name = null;
            int sorting = 0;
            RunOnUIThread.Execute(() =>
            {
                tableView = CreateReorderTable("Name", "Role", "City");
                tableView.CanUserSortColumns = true;
                name = tableView.Columns[0];
                tableView.Sorting += (s, e) => sorting++;
                LoadContent(tableView);
            });
            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                var travel = TravelInto(tableView, name, tableView.Columns[1], 0.75);
                Verify.IsTrue(TableViewTestHooks.BeginColumnReorderDrag(tableView, name), "The drag starts.");
                Verify.IsTrue(TableViewTestHooks.CompleteColumnReorderDrag(tableView, travel), "The drop is taken.");
            });
            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                VerifyOrder(tableView, "Role,Name,City", "after the drag");
                Verify.AreEqual(0, sorting, "A drag raises no Sorting.");
                Verify.IsTrue(tableView.Columns.All(c => c.SortDirection == SortDirection.None), "A drag sorts nothing.");
                var invoke = (IInvokeProvider)FrameworkElementAutomationPeer.CreatePeerForElement(GetColumnHeader(tableView, name)).GetPattern(PatternInterface.Invoke);
                Verify.IsNotNull(invoke, "The moved header is still sortable.");
                invoke.Invoke();
            });
            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(1, sorting, "The moved header sorts after the drag.");
                Verify.AreEqual(SortDirection.Ascending, name.SortDirection);
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies each change the API spec lists under Canceling ends a header drag without moving anything: Visibility, FrozenEdge, CanUserReorderColumns off, a Columns change, a Width set in code, a gripper resize, Density, a body or header scroll, a new template, unloading and replacing Columns.")]
        public void VerifyHookDragCancelPaths()
        {
            TableView tableView = null;
            int reordered = 0;
            RunOnUIThread.Execute(() =>
            {
                tableView = CreateReorderTable("Name", "Role", "City", "Team");
                tableView.CanUserResizeColumns = true;
                foreach (var column in tableView.Columns)
                {
                    column.Width = new GridLength(250);
                }
                tableView.ColumnReordered += (s, e) => reordered++;
                LoadContent(tableView);
            });
            SettleLayout(tableView);

            TableViewColumn team = null;
            var cancels = new (string what, Action cancel, Action restore)[]
            {
                ("a column's Visibility", () => RunOnUIThread.Execute(() => tableView.Columns[2].Visibility = Visibility.Collapsed),
                    () => RunOnUIThread.Execute(() => tableView.Columns[2].Visibility = Visibility.Visible)),
                ("a column's FrozenEdge", () => RunOnUIThread.Execute(() => tableView.Columns[2].FrozenEdge = TableViewFrozenEdge.Leading),
                    () => RunOnUIThread.Execute(() => tableView.Columns[2].FrozenEdge = TableViewFrozenEdge.None)),
                ("turning CanUserReorderColumns off", () => RunOnUIThread.Execute(() => tableView.CanUserReorderColumns = false),
                    () => RunOnUIThread.Execute(() => tableView.CanUserReorderColumns = true)),
                ("a Columns change", () => RunOnUIThread.Execute(() => tableView.Columns.Add(team = MakeTextTemplateColumn("Extra", bound: false, width: new GridLength(250)))),
                    () => RunOnUIThread.Execute(() => tableView.Columns.Remove(team))),
                ("a Width set in code", () => RunOnUIThread.Execute(() => tableView.Columns[2].Width = new GridLength(270)), null),
                ("a gripper resize", () => RunOnUIThread.Execute(() =>
                    {
                        var gripper = RequireGripper(tableView, 2);
                        gripper.BeginDrag();
                        gripper.TryDrag(30.0);
                        gripper.EndDrag(false);
                    }), null),
                ("a Density change", () => RunOnUIThread.Execute(() => tableView.Density = tableView.Density == TableViewDensity.Compact ? TableViewDensity.Standard : TableViewDensity.Compact), null),
                ("a body scroll", () => ScrollBodyTo(tableView, ScrollAxis.Horizontal, 100), null),
                ("a header scroll", () => RunOnUIThread.Execute(() =>
                    {
                        var headerScroller = tableView.FindVisualChildByName("PART_HeaderScroller") as ScrollViewer;
                        Verify.IsNotNull(headerScroller, "PART_HeaderScroller should exist once the template has applied.");
                        headerScroller.ChangeView(headerScroller.HorizontalOffset + 40, null, null, true);
                    }),
                    // Scrolling the body brings the header row back to its offset.
                    () => ScrollBodyTo(tableView, ScrollAxis.Horizontal, 0)),
                ("a new template", () => RunOnUIThread.Execute(() =>
                    {
                        var template = tableView.Template;
                        tableView.Template = null;
                        tableView.UpdateLayout();
                        tableView.Template = template;
                    }), null),
                ("unloading", () => RunOnUIThread.Execute(() => Content = null), () => RunOnUIThread.Execute(() => LoadContent(tableView))),
            };
            foreach (var (what, cancel, restore) in cancels)
            {
                VerifyHookDragCancels(tableView, what, cancel, restore);
            }

            Verify.AreEqual(0, reordered, "A canceled drag moves nothing.");
            RunOnUIThread.Execute(() =>
            {
                var name = tableView.Columns.First(c => (string)c.Header == "Name");
                var next = tableView.Columns[tableView.Columns.IndexOf(name) + 1];
                Verify.IsTrue(TableViewTestHooks.BeginColumnReorderDrag(tableView, name), "A drag starts after the canceled ones.");
                Verify.IsTrue(TableViewTestHooks.CompleteColumnReorderDrag(tableView, Physical(tableView.FlowDirection, TravelInto(tableView, name, next, 0.75))));
            });
            SettleLayout(tableView);
            Verify.AreEqual(1, reordered, "A drop after the canceled drags still moves the column.");

            VerifyHookDragCancels(tableView, "replacing Columns",
                () => RunOnUIThread.Execute(() => tableView.SetValue(TableView.ColumnsProperty, new List<TableViewColumn>(tableView.Columns))), null);
            Verify.AreEqual(1, reordered);
        }

        // Starts a drag of the first column into the second, runs cancel and checks the drag ended with every header back.
        private void VerifyHookDragCancels(TableView tableView, string what, Action cancel, Action restore)
        {
            Log.Comment($"Canceling by {what}.");
            SettleLayout(tableView);
            string order = null;
            double travel = 0;
            RunOnUIThread.Execute(() =>
            {
                order = string.Join(",", tableView.Columns.Select(c => (string)c.Header));
                SnapshotHeaders(tableView);
                travel = Physical(tableView.FlowDirection, TravelInto(tableView, tableView.Columns[0], tableView.Columns[1], 0.25));
                Verify.IsTrue(TableViewTestHooks.BeginColumnReorderDrag(tableView, tableView.Columns[0]), $"Precondition: a drag starts before {what}.");
                TableViewTestHooks.UpdateColumnReorderDrag(tableView, travel);
                tableView.UpdateLayout();
                Verify.IsTrue(PreviewShown(tableView), $"Precondition: dragging before {what}.");
            });

            cancel();
            // A change seen during layout ends the drag on the next tick.
            RunOnUIThread.Execute(() => tableView.UpdateLayout());
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                Verify.IsFalse(TableViewTestHooks.IsColumnReorderDragging(tableView), $"{what} must end the drag.");
                Verify.IsFalse(TableViewTestHooks.CompleteColumnReorderDrag(tableView, travel), $"After {what} the drop is ignored.");
                Verify.IsTrue(GetHeaderHost(tableView).Children.All(cell => Canvas.GetZIndex(cell) != 2), $"After {what} no header paints as dragged.");
            });

            restore?.Invoke();
            SettleLayout(tableView);
            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(order, string.Join(",", tableView.Columns.Select(c => (string)c.Header)), $"{what} moves nothing.");
                Verify.IsFalse(PreviewShown(tableView), $"After {what} every header is back where it was.");
            });
        }

        // The logical travel, in reading order, from the middle of column's header to fraction of the way into target's.
        private static double TravelInto(TableView tableView, TableViewColumn column, TableViewColumn target, double fraction)
        {
            var source = GetColumnHeader(tableView, column);
            var to = GetColumnHeader(tableView, target);
            var sourceX = source.TransformToVisual(tableView).TransformPoint(default).X;
            var targetX = to.TransformToVisual(tableView).TransformPoint(default).X;
            return (targetX + (to.ActualWidth * fraction)) - (sourceX + (source.ActualWidth / 2));
        }

        // The manipulation reports physical translation, mirrored under RightToLeft.
        private static double Physical(FlowDirection flowDirection, double logicalTravel)
            => flowDirection == FlowDirection.RightToLeft ? -logicalTravel : logicalTravel;
    }
}
