// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Automation;
using Microsoft.UI.Xaml.Automation.Peers;
using Microsoft.UI.Xaml.Automation.Provider;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Tabular;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Markup;
using Microsoft.UI.Xaml.Media;
using MUXControlsTestApp.Utilities;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;

using WEX.Logging.Interop;
using WEX.TestExecution;
using WEX.TestExecution.Markup;

using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewTestHelpers;
using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewShapingTestHelpers;
using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewGroupingTestHelpers;
using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewEditingTestHelpers;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    // Category 9 of the TableView test plan: grouping and group headers.
    //
    // SUBJECT. What the CONTROL does with a grouped source: the TableViewGroupInfo projection the
    // headers bind to, header content templating, the expand/collapse verbs, GroupExpanding/GroupCollapsed, and
    // the header's visual states.
    //
    // NOT THE SUBJECT. §8 already owns the grouping algebra on TableViewSource (one header per key,
    // group order, ClearGroupBy, add-creates-group, key changes), and §12 owns the group header's
    // automation peer (ExpandCollapse, GridItem spanning, name, control type). Plan section 9.0
    // writes the split up.
    //
    // FIXTURE. Six people over three roles, deliberately uneven: Designer x2, Engineer x3 and a
    // single-member Architect group, so group-boundary handling is exercised at both ends. Group
    // order follows first appearance, which §8 already pinned: Designer, Engineer, Architect.
    [TestClass]
    public class TableViewGroupingTests : TableViewApiTestBase
    {
        #region 9.1 TableViewGroupInfo

        [TestMethod]
        [TestProperty("Description", "Verifies TableViewGroupInfo reports the group's key and member count.")]
        public void VerifyGroupInfoReportsKeyAndItemCount()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateGroupedTable();
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                var expected = new[]
                {
                    new { Key = "Designer", Count = 2 },
                    new { Key = "Engineer", Count = 3 },
                    new { Key = "Architect", Count = 1 },
                };

                for (var i = 0; i < expected.Length; i++)
                {
                    var info = GetGroupInfo(tableView, i);
                    if (info == null)
                    {
                        return;
                    }

                    Verify.AreEqual(expected[i].Key, info.Key as string, $"Group {i} must carry its own key.");
                    Verify.AreEqual(expected[i].Count, info.ItemCount, $"Group {i} ('{expected[i].Key}') must report its member count.");
                }
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies groups produced by GroupBy start expandable and expanded.")]
        public void VerifyGroupInfoExpandabilityDefaults()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateGroupedTable();
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                for (var i = 0; i < 3; i++)
                {
                    var info = GetGroupInfo(tableView, i);
                    if (info == null)
                    {
                        return;
                    }

                    Verify.IsTrue(info.IsExpandable, $"Group {i} has members, so it must be expandable (TableView.idl:338).");
                    Verify.IsTrue(info.IsExpanded, $"Group {i} must start expanded so its data is reachable.");
                }
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies KeyText and ItemCountText produce the display strings the built-in header template binds.")]
        public void VerifyGroupInfoKeyTextAndItemCountText()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateGroupedTable();
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                var info = GetGroupInfo(tableView, 1);
                if (info == null)
                {
                    return;
                }

                Verify.AreEqual("Engineer", info.KeyText, "KeyText is the key's display string (TableView.idl:343).");

                // The surrounding wording is a localized resource, so only the count is pinned.
                Log.Comment($"ItemCountText for the Engineer group: '{info.ItemCountText}'.");
                Verify.IsFalse(string.IsNullOrEmpty(info.ItemCountText), "ItemCountText must not be empty.");
                Verify.IsTrue(info.ItemCountText.Contains("3"),
                    $"ItemCountText must carry the member count, was '{info.ItemCountText}'.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies collapsing a group raises PropertyChanged for IsExpanded on its TableViewGroupInfo.")]
        public void VerifyGroupInfoRaisesPropertyChangedOnExpandCollapse()
        {
            TableView tableView = null;
            TableViewGroupInfo info = null;
            var raised = new List<string>();

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateGroupedTable();
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                info = GetGroupInfo(tableView, 0);
                if (info == null)
                {
                    return;
                }

                info.PropertyChanged += (sender, args) => raised.Add(args.PropertyName);

                tableView.CollapseGroup(info.Key);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                if (info == null)
                {
                    return;
                }

                Verify.IsFalse(info.IsExpanded, "The group must read back collapsed.");
                Verify.IsTrue(raised.Contains("IsExpanded"),
                    $"PropertyChanged must be raised for IsExpanded; saw [{string.Join(", ", raised)}].");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies an add to a group updates the same TableViewGroupInfo instance and notifies.")]
        public void VerifyGroupInfoItemCountUpdatesInPlaceWithNotification()
        {
            TableView tableView = null;
            TableViewSource source = null;
            ObservableCollection<ShapedPerson> items = null;
            TableViewGroupInfo captured = null;
            var raised = new List<string>();

            RunOnUIThread.Execute(() =>
            {
                items = new ObservableCollection<ShapedPerson>(MakeShapedPeople());
                source = TableViewSource.From(items);
                source.GroupBy(o => Person(o).Role);

                tableView = CreateGroupedTable(source);
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                captured = GetGroupInfo(tableView, 0);
                if (captured == null)
                {
                    return;
                }

                Verify.AreEqual(2, captured.ItemCount, "Precondition: the Designer group starts with two members.");
                captured.PropertyChanged += (sender, args) => raised.Add(args.PropertyName);

                items.Add(new ShapedPerson("Nadia", "Designer", "Studio"));
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                if (captured == null)
                {
                    return;
                }

                // The info objects are pooled and rewritten rather than rebuilt - that is the point
                // of the in-place contract - so the captured reference is only good for proving a
                // notification fired. Group state itself is read back through the header.
                Verify.IsTrue(raised.Contains("ItemCount"),
                    $"PropertyChanged must be raised for ItemCount; saw [{string.Join(", ", raised)}].");

                var current = GetGroupInfo(tableView, 0);
                if (current == null)
                {
                    return;
                }

                Verify.AreEqual("Designer", current.Key as string, "The first group is still the Designer group.");
                Verify.AreEqual(3, current.ItemCount,
                    "The group the header is bound to must report the new count (TableView.idl:329-330).");
            });
        }

        #endregion

        #region 9.2 Group header rendering

        [TestMethod]
        [TestProperty("Description", "Verifies the default group header renders the group key and member count.")]
        public void VerifyDefaultGroupHeaderShowsKeyAndCount()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateGroupedTable();
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                var text = HeaderText(tableView, 1);
                Log.Comment($"Default group header text: '{text}'.");

                Verify.IsTrue(text.Contains("Engineer"), $"The default header must show the key, was '{text}'.");
                Verify.IsTrue(text.Contains("3"), $"The default header must show the member count, was '{text}'.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies GroupHeaderTemplate replaces the default group header content.")]
        public void VerifyGroupHeaderTemplateApplies()
        {
            TableView tableView = null;
            DataTemplate template = null;

            RunOnUIThread.Execute(() =>
            {
                template = CreateMarkerTemplate("A:");

                tableView = CreateGroupedTable();
                tableView.GroupHeaderTemplate = template;

                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                var header = GetGroupHeader(tableView, 0);

                Verify.AreEqual(template, header.ContentTemplate,
                    "GroupHeaderTemplate must reach the header's ContentTemplate (TableView.idl:380-383).");

                var text = HeaderText(tableView, 0);

                // The marker proves the app template rendered; the key proves Content really is the
                // TableViewGroupInfo the template binds against.
                Verify.AreEqual("A:Designer", text, $"The app template must render, was '{text}'.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies replacing GroupHeaderTemplate on a loaded table rebuilds realized headers.")]
        public void VerifyGroupHeaderTemplateChangeAfterLoadUpdatesLive()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateGroupedTable();
                tableView.GroupHeaderTemplate = CreateMarkerTemplate("A:");

                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual("A:Designer", HeaderText(tableView, 0), "Precondition: the first template rendered.");
                tableView.GroupHeaderTemplate = CreateMarkerTemplate("B:");
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual("B:Designer", HeaderText(tableView, 0),
                    "A realized header must rebuild against the new template.");
                Verify.AreEqual("B:Engineer", HeaderText(tableView, 1),
                    "Every realized header must rebuild, not only the first.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies clearing GroupHeaderTemplate reverts headers to the default content.")]
        public void VerifyClearingGroupHeaderTemplateRevertsToTheStyleDefault()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateGroupedTable();
                tableView.GroupHeaderTemplate = CreateMarkerTemplate("A:");

                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual("A:Designer", HeaderText(tableView, 0), "Precondition: the app template applied.");
                tableView.GroupHeaderTemplate = null;
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                var text = HeaderText(tableView, 0);
                Log.Comment($"Group header text after clearing the template: '{text}'.");

                Verify.IsFalse(text.StartsWith("A:"), "The app template must be gone.");
                Verify.IsTrue(text.Contains("Designer"),
                    $"Clearing the override must revert to the Style's default content (TableView.idl:381-383), was '{text}'.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a group header spans the whole column set rather than a single column.")]
        public void VerifyGroupHeaderSpansAllColumns()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateGroupedTable();
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                var header = GetGroupHeader(tableView, 0);
                if (header == null)
                {
                    return;
                }

                var columnsWidth = tableView.Columns.Sum(column => column.ActualWidth);
                var firstColumnWidth = tableView.Columns[0].ActualWidth;

                Log.Comment($"Group header width {header.ActualWidth}, columns total {columnsWidth}, first column {firstColumnWidth}.");

                Verify.IsGreaterThan(header.ActualWidth, firstColumnWidth,
                    "A group header must be wider than a single column - it is a section break, not a cell.");

                // The band sizes itself from the visible-column sum - the same total the cells panel
                // uses - so a header and a row of cells line up edge to edge. The row's own border
                // stretches past that to the viewport, so the row is not the yardstick.
                Verify.IsLessThan(Math.Abs(header.ActualWidth - columnsWidth), 1.0,
                    $"A group header must span the whole column set; header {header.ActualWidth}, columns {columnsWidth}.");
            });
        }

        #endregion

        #region 9.3 Expand and collapse

        [TestMethod]
        [TestProperty("Description", "Verifies ExpandAllGroups and CollapseAllGroups are safe no-ops on an ungrouped source.")]
        public void VerifyExpandAndCollapseAllOnUngroupedSourceAreNoOps()
        {
            TableView tableView = null;
            List<string> before = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateGroupedTable(MakeShapedPeople());
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() => before = GetProjectedLabels(tableView, includeCount: false));

            RunOnUIThread.Execute(() =>
            {
                tableView.CollapseAllGroups();
                tableView.ExpandAllGroups();
                tableView.CollapseAllGroups();
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
                VerifySequence(before, GetProjectedLabels(tableView, includeCount: false),
                    "expand/collapse must not touch an ungrouped source (TableView.idl:557-559)"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies CollapseAllGroups leaves only group headers projected.")]
        public void VerifyCollapseAllGroupsHidesDataRowsKeepsHeaders()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateGroupedTable();
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() => tableView.CollapseAllGroups());

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                VerifySequence(
                    new[] { "#Designer", "#Engineer", "#Architect" },
                    GetProjectedLabels(tableView, includeCount: false),
                    "a full collapse must leave one header per group and no data rows");

                for (var i = 0; i < 3; i++)
                {
                    var info = GetGroupInfo(tableView, i);
                    if (info == null)
                    {
                        return;
                    }

                    Verify.IsFalse(info.IsExpanded, $"Group {i} must report itself collapsed.");
                }
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies ExpandAllGroups restores the full projection after a collapse.")]
        public void VerifyExpandAllGroupsRestoresDataRows()
        {
            TableView tableView = null;
            List<string> before = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateGroupedTable();
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() => before = GetProjectedLabels(tableView, includeCount: false));

            RunOnUIThread.Execute(() => tableView.CollapseAllGroups());

            SettleLayout(tableView);

            RunOnUIThread.Execute(() => tableView.ExpandAllGroups());

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
                VerifySequence(before, GetProjectedLabels(tableView, includeCount: false),
                    "a collapse/expand round trip must restore the same projection"));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies collapsing one group leaves the other groups untouched.")]
        public void VerifyToggleSingleGroupUpdatesOnlyThatGroup()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateGroupedTable();
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() => GetExpandCollapseProvider(tableView, 0).Collapse());

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                VerifySequence(
                    new[] { "#Designer", "#Engineer", "Diego", "Rafa", "Owen", "#Architect", "Mei" },
                    GetProjectedLabels(tableView, includeCount: false),
                    "only the collapsed group may lose its rows");

                var collapsed = GetGroupInfo(tableView, 0);
                var untouched = GetGroupInfo(tableView, 1);
                if (collapsed == null || untouched == null)
                {
                    return;
                }

                Verify.IsFalse(collapsed.IsExpanded, "The toggled group is collapsed.");
                Verify.IsTrue(untouched.IsExpanded, "Per-group state must be per group, not global.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a collapsed group stays collapsed across an unrelated source update.")]
        public void VerifyCollapsedGroupStatePersistsAcrossSourceUpdate()
        {
            TableView tableView = null;
            ObservableCollection<ShapedPerson> items = null;

            RunOnUIThread.Execute(() =>
            {
                items = new ObservableCollection<ShapedPerson>(MakeShapedPeople());
                var source = TableViewSource.From(items);
                source.GroupBy(o => Person(o).Role);

                tableView = CreateGroupedTable(source);
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() => GetExpandCollapseProvider(tableView, 0).Collapse());

            SettleLayout(tableView);

            // An add to a DIFFERENT group: nothing about this touches the collapsed one.
            RunOnUIThread.Execute(() => items.Add(new ShapedPerson("Nadia", "Engineer", "Platform")));

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                var info = GetGroupInfo(tableView, 0);
                if (info == null)
                {
                    return;
                }

                Verify.IsFalse(info.IsExpanded, "A data update must not re-expand what the user collapsed.");

                var labels = GetProjectedLabels(tableView, includeCount: false);
                Verify.IsFalse(labels.Contains("Asha"),
                    $"The collapsed group must still contribute no data rows; saw [{string.Join(", ", labels)}].");
                Verify.IsTrue(labels.Contains("Nadia"), "The added row must appear in its own group.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a collapse does not pull focus back to its header once focus has left the table.")]
        public void VerifyGroupFocusRestoreSkippedWhenFocusLeftTable()
        {
            TableView tableView = null;
            Button button = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateGroupedTable();
                button = new Button { Content = "Outside" };
                var panel = new StackPanel();
                panel.Children.Add(tableView);
                panel.Children.Add(button);
                LoadContent(panel);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                // The collapse captures the focused header for a restore after the deferred reshape.
                Verify.IsTrue(GetGroupHeader(tableView, 0).Focus(FocusState.Keyboard), "Precondition: the header takes keyboard focus.");
                GetExpandCollapseProvider(tableView, 0).Collapse();
                Verify.IsTrue(button.Focus(FocusState.Programmatic), "Precondition: the button takes focus.");
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(ReferenceEquals(button, FocusManager.GetFocusedElement(tableView.XamlRoot)),
                    "Focus that has left the table must not be pulled back to the collapsed header.");
                Verify.IsFalse(GetGroupInfo(tableView, 0)?.IsExpanded ?? true, "The group must still collapse.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies collapsing the group of the selected row clears selection, and neither re-expanding nor a later reshape restores it.")]
        public void VerifyGroupCollapseDropsSelectionOfHiddenRow()
        {
            TableView tableView = null;
            TableViewSource source = null;
            var changes = 0;

            RunOnUIThread.Execute(() =>
            {
                source = TableViewSource.From(MakeShapedPeople());
                source.GroupBy(o => Person(o).Role);

                tableView = CreateGroupedTable(source);
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                tableView.SelectionChanged += (s, e) => changes++;
                tableView.Select(GetProjectedLabels(tableView, includeCount: false).IndexOf("Asha"));
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual("Asha", Person(tableView.SelectedItem)?.Name, "Asha is selected.");
                Verify.AreEqual(1, changes, "One SelectionChanged for the select.");
                GetExpandCollapseProvider(tableView, 0).Collapse();
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsNull(tableView.SelectedItem, "Collapsing Designer hides Asha and clears selection.");
                Verify.AreEqual(2, changes, "One SelectionChanged for the clear.");
                tableView.ExpandAllGroups();
            });

            SettleLayout(tableView);

            // A Reset-driven reshape must not resurrect the selection the collapse cleared.
            RunOnUIThread.Execute(() => source.Sort(o => Person(o).Name, SortDirection.Ascending));

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsNull(tableView.SelectedItem, "Re-expanding and reshaping must not restore the selection.");
                Verify.AreEqual(2, changes, "No further SelectionChanged.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies toggling a focused group header at the bottom of a long grouped table keeps it in view instead of jumping the viewport to an earlier group.")]
        public void VerifyFocusedBottomGroupToggleKeepsViewport()
        {
            const int groupCount = 40;
            const int perGroup = 3;
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                var people = new List<ShapedPerson>();
                for (var g = 0; g < groupCount; g++)
                {
                    for (var p = 0; p < perGroup; p++)
                    {
                        people.Add(new ShapedPerson($"P{g:D2}-{p}", $"G{g:D2}", "Dept"));
                    }
                }

                var source = TableViewSource.From(people);
                source.GroupBy(o => Person(o).Role);
                tableView = CreateTableView(source, headers: new[] { "Name", "Role" }, width: 600, height: 300);
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            // Virtualized extents are estimates, so re-target the end until it stops moving.
            for (var attempt = 0; attempt < 5; attempt++)
            {
                var atEnd = false;
                RunOnUIThread.Execute(() =>
                {
                    var scroller = GetBodyScroller(tableView);
                    atEnd = scroller.ScrollableHeight > 0 && Math.Abs(scroller.VerticalOffset - scroller.ScrollableHeight) < 0.5;
                    if (!atEnd)
                    {
                        scroller.ChangeView(null, scroller.ScrollableHeight, null, true);
                    }
                });
                SettleLayout(tableView);
                if (atEnd)
                {
                    break;
                }
            }

            foreach (var targetGroup in new[] { groupCount - 1, groupCount - 2 })
            {
                var key = $"G{targetGroup:D2}";
                double groupHeight = 0.0;

                RunOnUIThread.Execute(() =>
                {
                    var header = FindRealizedGroupHeader(tableView, key);
                    Verify.IsNotNull(header, $"Precondition: the header of {key} is realized at the bottom.");
                    groupHeight = header.ActualHeight + (perGroup * GetRealizedRows(tableView).Select(r => r.ActualHeight).DefaultIfEmpty(0.0).Max());
                    Verify.IsTrue(header.Focus(FocusState.Keyboard), $"Precondition: the {key} header takes keyboard focus.");
                    VerifyGroupHeaderInViewport(tableView, key, "before any toggle");
                });

                for (var cycle = 0; cycle < 2; cycle++)
                {
                    foreach (var expand in new[] { false, true })
                    {
                        double offsetBefore = 0.0;
                        var context = $"{key} cycle {cycle} after {(expand ? "expand" : "collapse")}";

                        RunOnUIThread.Execute(() =>
                        {
                            offsetBefore = GetBodyScroller(tableView).VerticalOffset;
                            var header = FindRealizedGroupHeader(tableView, key);
                            Verify.IsNotNull(header, $"The {key} header is realized ({context}).");
                            var provider = FrameworkElementAutomationPeer.CreatePeerForElement(header)
                                .GetPattern(PatternInterface.ExpandCollapse) as IExpandCollapseProvider;
                            if (expand)
                            {
                                provider.Expand();
                            }
                            else
                            {
                                provider.Collapse();
                            }
                        });

                        // The focus restore runs on LayoutUpdated after the deferred reshape.
                        SettleLayout(tableView);
                        SettleLayout(tableView);

                        RunOnUIThread.Execute(() =>
                        {
                            var scroller = GetBodyScroller(tableView);
                            var offsetAfter = scroller.VerticalOffset;
                            Log.Comment($"{context}: offset {offsetBefore:F1} -> {offsetAfter:F1}, scrollable {scroller.ScrollableHeight:F1}.");

                            var focused = FocusManager.GetFocusedElement(tableView.XamlRoot) as TableViewGroupHeader;
                            Verify.AreEqual(key, (focused?.Content as TableViewGroupInfo)?.Key as string,
                                $"Keyboard focus stays on the toggled header ({context}).");
                            Verify.AreEqual(expand, GetGroupInfoByKey(tableView, key)?.IsExpanded ?? !expand,
                                $"The group reports the requested state ({context}).");
                            VerifyGroupHeaderInViewport(tableView, key, context);

                            // Beyond clamping at the end of the extent, scroll anchoring may shift by at most one group.
                            var maxBackward = Math.Max(0.0, offsetBefore - scroller.ScrollableHeight) + groupHeight + 0.5;
                            Verify.IsLessThanOrEqual(offsetBefore - offsetAfter, maxBackward,
                                $"The viewport must not jump to an earlier group ({context}).");
                        });
                    }
                }
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies toggling a focused group header whose realized neighbours are collapsed group headers keeps the viewport, even when ItemsRepeater rescues focus onto a header outside it.")]
        [TestProperty("data:Edge", "{Top, Bottom}")]
        public void VerifyFocusedGroupToggleAmongCollapsedHeadersKeepsViewport()
        {
            var edge = ((string)TestContext.DataRow["Edge"]).Trim();
            const int groupCount = 40;
            const int perGroup = 3;
            const string key = "G20";
            TableView tableView = null;
            var focusLog = new List<string>();
            var rescuedHeaders = new List<string>();
            var toggling = false;

            RunOnUIThread.Execute(() =>
            {
                var people = new List<ShapedPerson>();
                for (var g = 0; g < groupCount; g++)
                {
                    for (var p = 0; p < perGroup; p++)
                    {
                        people.Add(new ShapedPerson($"P{g:D2}-{p}", $"G{g:D2}", "Dept"));
                    }
                }

                var source = TableViewSource.From(people);
                source.GroupBy(o => Person(o).Role);
                tableView = CreateTableView(source, headers: new[] { "Name", "Role" }, width: 600, height: 300);
                tableView.GotFocus += (s, e) =>
                {
                    if (!toggling)
                    {
                        return;
                    }

                    var scroller = GetBodyScroller(tableView);
                    var element = e.OriginalSource as FrameworkElement;
                    var header = element as TableViewGroupHeader;
                    var top = double.NaN;
                    try
                    {
                        top = element?.TransformToVisual(scroller).TransformPoint(new global::Windows.Foundation.Point(0.0, 0.0)).Y ?? double.NaN;
                    }
                    catch (ArgumentException)
                    {
                        // The element left the tree between focus and this handler.
                    }

                    var inView = top >= -0.5 && top + element.ActualHeight <= scroller.ViewportHeight + 0.5;
                    var name = header != null ? $"header {(header.Content as TableViewGroupInfo)?.Key}" : element?.GetType().Name ?? "<null>";
                    focusLog.Add($"{name} top {top:F1} {(inView ? "in view" : "outside the viewport")}");
                    if (header != null && !Equals((header.Content as TableViewGroupInfo)?.Key, key) && !inView)
                    {
                        rescuedHeaders.Add(name);
                    }
                };
                LoadContent(tableView);
            });
            SettleLayout(tableView);

            RunOnUIThread.Execute(() => tableView.CollapseAllGroups());
            SettleLayout(tableView);

            // Bring the target header in, expand it, then align it with the requested viewport edge so its
            // neighbour on that side is a collapsed header realized outside the viewport.
            for (var attempt = 0; attempt < 6; attempt++)
            {
                var done = false;
                RunOnUIThread.Execute(() =>
                {
                    var scroller = GetBodyScroller(tableView);
                    var header = FindRealizedGroupHeader(tableView, key);
                    if (header == null)
                    {
                        var repeater = GetRowsRepeater(tableView);
                        var anyHeight = (VisualTreeHelper.GetChild(repeater, 0) as FrameworkElement)?.ActualHeight ?? 0.0;
                        scroller.ChangeView(null, 20 * Math.Max(anyHeight, 30.0), null, true);
                        return;
                    }

                    var info = header.Content as TableViewGroupInfo;
                    if (!info.IsExpanded)
                    {
                        (FrameworkElementAutomationPeer.CreatePeerForElement(header).GetPattern(PatternInterface.ExpandCollapse) as IExpandCollapseProvider).Expand();
                        return;
                    }

                    var top = header.TransformToVisual(scroller).TransformPoint(new global::Windows.Foundation.Point(0.0, 0.0)).Y;
                    var delta = edge == "Top" ? top : top + header.ActualHeight - scroller.ViewportHeight;
                    done = Math.Abs(delta) < 0.5;
                    if (!done)
                    {
                        scroller.ChangeView(null, scroller.VerticalOffset + delta, null, true);
                    }
                });
                SettleLayout(tableView);
                if (done)
                {
                    break;
                }
            }

            RunOnUIThread.Execute(() =>
            {
                var header = FindRealizedGroupHeader(tableView, key);
                Verify.IsNotNull(header, $"Precondition: the {key} header is realized.");
                Verify.IsTrue((header.Content as TableViewGroupInfo).IsExpanded, $"Precondition: {key} is expanded.");
                Verify.IsTrue(header.Focus(FocusState.Keyboard), $"Precondition: the {key} header takes keyboard focus.");
                var scroller = GetBodyScroller(tableView);
                var headerTop = header.TransformToVisual(scroller).TransformPoint(new global::Windows.Foundation.Point(0.0, 0.0)).Y;
                var edgeDelta = edge == "Top" ? headerTop : headerTop + header.ActualHeight - scroller.ViewportHeight;
                Verify.IsLessThan(Math.Abs(edgeDelta), 1.0,
                    $"Precondition: the {key} header is aligned with the {edge} viewport edge (header top {headerTop:F1}, height {header.ActualHeight:F1}, viewport {scroller.ViewportHeight:F1}).");
                VerifyGroupHeaderInViewport(tableView, key, $"{edge} edge before any toggle");
            });
            SettleLayout(tableView);

            for (var cycle = 0; cycle < 2; cycle++)
            {
                foreach (var expand in new[] { false, true })
                {
                    double offsetBefore = 0.0;
                    double topBefore = 0.0;
                    var context = $"{edge} edge cycle {cycle} after {(expand ? "expand" : "collapse")}";

                    RunOnUIThread.Execute(() =>
                    {
                        var scroller = GetBodyScroller(tableView);
                        offsetBefore = scroller.VerticalOffset;
                        var header = FindRealizedGroupHeader(tableView, key);
                        Verify.IsNotNull(header, $"The {key} header is realized ({context}).");
                        topBefore = header.TransformToVisual(scroller).TransformPoint(new global::Windows.Foundation.Point(0.0, 0.0)).Y;
                        focusLog.Clear();
                        toggling = true;
                        var provider = FrameworkElementAutomationPeer.CreatePeerForElement(header)
                            .GetPattern(PatternInterface.ExpandCollapse) as IExpandCollapseProvider;
                        if (expand)
                        {
                            provider.Expand();
                        }
                        else
                        {
                            provider.Collapse();
                        }
                    });

                    SettleLayout(tableView);
                    SettleLayout(tableView);

                    RunOnUIThread.Execute(() =>
                    {
                        toggling = false;
                        var scroller = GetBodyScroller(tableView);
                        var header = FindRealizedGroupHeader(tableView, key);
                        var topAfter = header?.TransformToVisual(scroller).TransformPoint(new global::Windows.Foundation.Point(0.0, 0.0)).Y ?? double.NaN;
                        Log.Comment($"{context}: offset {offsetBefore:F1} -> {scroller.VerticalOffset:F1}, header top {topBefore:F1} -> {topAfter:F1}, scrollable {scroller.ScrollableHeight:F1}; focus: {(focusLog.Count == 0 ? "<unchanged>" : string.Join(" | ", focusLog))}.");

                        var focused = FocusManager.GetFocusedElement(tableView.XamlRoot) as TableViewGroupHeader;
                        Verify.AreEqual(key, (focused?.Content as TableViewGroupInfo)?.Key as string,
                            $"Keyboard focus stays on the toggled header ({context}).");
                        VerifyGroupHeaderInViewport(tableView, key, context);

                        // Estimated extents can settle by a few pixels; revealing a rescued header jumps far more.
                        Verify.IsLessThan(Math.Abs(topAfter - topBefore), header?.ActualHeight ?? 0.0,
                            $"The toggled header must keep its place in the viewport ({context}).");
                    });
                }
            }

            if (rescuedHeaders.Count == 0)
            {
                Log.Warning($"ItemsRepeater's focus rescue never landed on a group header outside the viewport ({edge} edge), so the rescued-focus path was not exercised; the viewport and focus checks above still passed.");
            }
            else
            {
                Log.Comment($"Focus rescues on group headers outside the viewport: {string.Join(", ", rescuedHeaders)}.");
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a focused group toggle dropped because the open edit would not end does not stop a row from being brought into view afterwards.")]
        [TestProperty("data:Reveal", "{KeyboardFocus, StartBringIntoView}")]
        public void VerifyDroppedGroupToggleKeepsRowBringIntoView()
        {
            var reveal = ((string)TestContext.DataRow["Reveal"]).Trim();
            TableView tableView = null;
            List<EditablePerson> items = null;
            var vetoing = true;

            RunOnUIThread.Execute(() =>
            {
                items = Enumerable.Range(0, 30).Select(i => new EditablePerson { Name = $"P{i:D2}", Role = $"R{i / 10}" }).ToList();
                var source = TableViewSource.From(items);
                source.GroupBy(o => ((EditablePerson)o).Role);
                tableView = CreateTableView(source, headers: new[] { "Name", "Role" }, width: 600, height: 300);
                tableView.IsReadOnly = false;
                tableView.CellEditEnding += (s, e) => e.Cancel = vetoing;
                LoadContent(tableView);
            });
            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsNotNull(CaptureSetValue(GetValueProvider(tableView, items[0], columnIndex: 0), "Vetoed"),
                    "Precondition: the vetoed commit fails.");
                Verify.IsTrue(tableView.IsEditing, "Precondition: the edit stays open.");
                var header = FindRealizedGroupHeader(tableView, "R0");
                Verify.IsTrue(header.Focus(FocusState.Keyboard), "Precondition: the header takes keyboard focus.");
                Verify.IsTrue(tableView.IsEditing, "Precondition: the edit is still open.");
                (FrameworkElementAutomationPeer.CreatePeerForElement(header).GetPattern(PatternInterface.ExpandCollapse) as IExpandCollapseProvider).Collapse();
            });
            SettleLayout(tableView);
            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(GetGroupInfoByKey(tableView, "R0")?.IsExpanded ?? false, "Precondition: the toggle is dropped while the edit cannot end.");
                vetoing = false;
                Verify.IsTrue(tableView.CancelEdit(), "Precondition: the edit closes.");
            });
            SettleLayout(tableView);

            TableViewRow target = null;
            double offsetBefore = 0.0;
            RunOnUIThread.Execute(() =>
            {
                var scroller = GetBodyScroller(tableView);
                offsetBefore = scroller.VerticalOffset;
                target = GetRealizedRows(tableView)
                    .Select(r => (Row: r, Top: r.TransformToVisual(scroller).TransformPoint(new global::Windows.Foundation.Point(0.0, 0.0)).Y))
                    .Where(r => r.Top >= scroller.ViewportHeight)
                    .OrderBy(r => r.Top)
                    .Select(r => r.Row)
                    .FirstOrDefault();
                Verify.IsNotNull(target, "Precondition: a realized row lies below the viewport.");
                if (reveal == "KeyboardFocus")
                {
                    Verify.IsTrue(target.Focus(FocusState.Keyboard), "Precondition: the row takes keyboard focus.");
                }
                else
                {
                    target.StartBringIntoView();
                }
            });
            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                var scroller = GetBodyScroller(tableView);
                var top = target.TransformToVisual(scroller).TransformPoint(new global::Windows.Foundation.Point(0.0, 0.0)).Y;
                Verify.IsTrue(scroller.VerticalOffset > offsetBefore + 0.5 && top < scroller.ViewportHeight,
                    $"{reveal}: the row must be brought into view after the dropped toggle: offset {offsetBefore:F1} -> {scroller.VerticalOffset:F1}, top {top:F1}, viewport {scroller.ViewportHeight:F1}.");
            });
        }

        private static TableViewGroupHeader FindRealizedGroupHeader(TableView tableView, string key)
        {
            var repeater = GetRowsRepeater(tableView);
            return Enumerable.Range(0, VisualTreeHelper.GetChildrenCount(repeater))
                .Select(i => VisualTreeHelper.GetChild(repeater, i) as TableViewGroupHeader)
                .FirstOrDefault(h => h != null && repeater.GetElementIndex(h) >= 0 &&
                    Equals((h.Content as TableViewGroupInfo)?.Key, key));
        }

        private static TableViewGroupInfo GetGroupInfoByKey(TableView tableView, string key)
            => FindRealizedGroupHeader(tableView, key)?.Content as TableViewGroupInfo;

        private static void VerifyGroupHeaderInViewport(TableView tableView, string key, string context)
        {
            var header = FindRealizedGroupHeader(tableView, key);
            Verify.IsNotNull(header, $"The {key} header must stay realized ({context}).");
            if (header == null)
            {
                return;
            }

            var scroller = GetBodyScroller(tableView);
            var top = header.TransformToVisual(scroller).TransformPoint(new global::Windows.Foundation.Point(0.0, 0.0)).Y;
            Verify.IsTrue(top >= -0.5 && top + header.ActualHeight <= scroller.ViewportHeight + 0.5,
                $"The {key} header must stay fully in the viewport ({context}): top {top:F1}, height {header.ActualHeight:F1}, viewport {scroller.ViewportHeight:F1}.");
        }

        #endregion

        #region 9.4 Group edge cases

        [TestMethod]
        [TestProperty("Description", "Verifies a group with exactly one member renders a header and one row.")]
        public void VerifySingleItemGroupRenders()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateGroupedTable();
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                var info = GetGroupInfo(tableView, 2);
                if (info == null)
                {
                    return;
                }

                Verify.AreEqual("Architect", info.Key as string, "The third group is the single-member one.");
                Verify.AreEqual(1, info.ItemCount, "A single-member group reports one member.");

                var labels = GetProjectedLabels(tableView, includeCount: false);
                var headerIndex = labels.IndexOf("#Architect");

                Verify.IsTrue(headerIndex >= 0, "The single-member group must still get a header.");
                Verify.AreEqual(labels.Count - 1, headerIndex + 1, "Its one row must follow the header.");
                Verify.AreEqual("Mei", labels[headerIndex + 1], "The row under the header is the group's only member.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies external SetValue on the read-only IsExpanded, IsExpandable and IsEditing DPs is reverted and changes nothing.")]
        public void VerifyReadOnlyDependencyPropertiesRejectExternalWrites()
        {
            TableView tableView = null;
            List<string> before = null;
            var events = 0;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateGroupedTable();
                tableView.GroupExpanding += (s, e) => events++;
                tableView.GroupCollapsed += (s, e) => events++;
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                before = GetProjectedLabels(tableView, includeCount: false);
                var header = GetGroupHeader(tableView, 0);
                header.SetValue(TableViewGroupHeader.IsExpandedProperty, false);
                header.SetValue(TableViewGroupHeader.IsExpandableProperty, false);
                tableView.SetValue(TableView.IsEditingProperty, true);

                Verify.IsTrue(header.IsExpanded, "An external IsExpanded write must be reverted.");
                Verify.IsTrue(header.IsExpandable, "An external IsExpandable write must be reverted.");
                Verify.IsFalse(tableView.IsEditing, "An external IsEditing write must be reverted.");
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                VerifySequence(before, GetProjectedLabels(tableView, includeCount: false), "rejected writes must not reshape");
                Verify.AreEqual(0, events, "Rejected writes must raise no group events.");
                Verify.AreEqual(ExpandCollapseState.Expanded, GetExpandCollapseProvider(tableView, 0).ExpandCollapseState,
                    "The peer must still report the real state.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies removing a group's last member removes its header.")]
        public void VerifyGroupRemovedWhenLastItemRemoved()
        {
            TableView tableView = null;
            ObservableCollection<ShapedPerson> items = null;

            RunOnUIThread.Execute(() =>
            {
                items = new ObservableCollection<ShapedPerson>(MakeShapedPeople());
                var source = TableViewSource.From(items);
                source.GroupBy(o => Person(o).Role);

                tableView = CreateGroupedTable(source);
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            // Mei is the only Architect.
            RunOnUIThread.Execute(() => items.Remove(items.First(person => person.Role == "Architect")));

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
                VerifySequence(
                    new[] { "#Designer", "Asha", "Ines", "#Engineer", "Diego", "Rafa", "Owen" },
                    GetProjectedLabels(tableView, includeCount: false),
                    "an emptied group must not leave its header behind"));
        }

        #endregion

        #region 9.5 Group header visual states

        [TestMethod]
        [TestProperty("Description", "Verifies the group header's ExpansionStates follow IsExpanded.")]
        public void VerifyGroupHeaderExpansionVisualStates()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateGroupedTable();
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
                Verify.AreEqual("Expanded", GetCurrentVisualState(GetGroupHeader(tableView, 0), "ExpansionStates"),
                    "An expanded group must be in the Expanded state (TableView.idl:372)."));

            RunOnUIThread.Execute(() => tableView.CollapseGroup(GetGroupInfo(tableView, 0).Key));

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
                Verify.AreEqual("Collapsed", GetCurrentVisualState(GetGroupHeader(tableView, 0), "ExpansionStates"),
                    "A collapsed group must be in the Collapsed state."));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a group with members stays in the Expandable state whether expanded or collapsed.")]
        public void VerifyGroupHeaderExpandabilityVisualStates()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateGroupedTable();
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
                Verify.AreEqual("Expandable", GetCurrentVisualState(GetGroupHeader(tableView, 0), "ExpandabilityStates"),
                    "A group with members must be in the Expandable state (TableView.idl:373)."));

            RunOnUIThread.Execute(() => tableView.CollapseGroup(GetGroupInfo(tableView, 0).Key));

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
                Verify.AreEqual("Expandable", GetCurrentVisualState(GetGroupHeader(tableView, 0), "ExpandabilityStates"),
                    "Collapsing a group with members must not change its expandability state."));
        }

        #endregion

        #region ExpandGroup, CollapseGroup and the group events

        [TestMethod]
        [TestProperty("Description", "Verifies CollapseGroup/ExpandGroup change only the named group and raise GroupCollapsed/GroupExpanding once each, with no event for a no-op or an unknown key.")]
        public void VerifyExpandAndCollapseGroupRaiseGroupEvents()
        {
            TableView tableView = null;
            var events = new List<string>();

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateGroupedTable();
                tableView.GroupExpanding += (s, e) => events.Add("E:" + e.GroupKey);
                tableView.GroupCollapsed += (s, e) => events.Add("C:" + e.GroupKey);
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                tableView.CollapseGroup("Engineer");
                tableView.CollapseGroup("Engineer");
                tableView.CollapseGroup("NoSuchGroup");
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                VerifySequence(new[] { "#Designer", "Asha", "Ines", "#Engineer", "#Architect", "Mei" },
                    GetProjectedLabels(tableView, includeCount: false), "only Engineer collapses");
                Verify.AreEqual("C:Engineer", string.Join(",", events), "One GroupCollapsed for the one change; none for the no-op or unknown key.");
                events.Clear();
                tableView.ExpandGroup("Engineer");
                tableView.ExpandGroup("Designer");
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                VerifySequence(new[] { "#Designer", "Asha", "Ines", "#Engineer", "Diego", "Rafa", "Owen", "#Architect", "Mei" },
                    GetProjectedLabels(tableView, includeCount: false), "Engineer expands again");
                Verify.AreEqual("E:Engineer", string.Join(",", events), "One GroupExpanding for Engineer; none for the already-expanded Designer.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies GroupExpanding is raised before the group expands.")]
        public void VerifyGroupExpandingIsRaisedBeforeTheStateChanges()
        {
            TableView tableView = null;
            bool? expandedInHandler = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateGroupedTable();
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() => tableView.CollapseGroup("Designer"));

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                var info = GetGroupInfoByKey(tableView, "Designer");
                tableView.GroupExpanding += (s, e) => expandedInHandler = info.IsExpanded;
                tableView.ExpandGroup("Designer");
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(false, expandedInHandler, "GroupExpanding must be raised while the group is still collapsed.");
                Verify.IsTrue(GetGroupInfoByKey(tableView, "Designer").IsExpanded, "The group must then expand.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies CollapseAllGroups and ExpandAllGroups raise one event per group whose state changes, and none for groups already in that state.")]
        public void VerifyBulkExpandCollapseRaisesOneEventPerChangedGroup()
        {
            TableView tableView = null;
            var events = new List<string>();

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateGroupedTable();
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() => tableView.CollapseGroup("Designer"));

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                tableView.GroupExpanding += (s, e) => events.Add("E:" + e.GroupKey);
                tableView.GroupCollapsed += (s, e) => events.Add("C:" + e.GroupKey);
                tableView.CollapseAllGroups();
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual("C:Architect,C:Engineer", string.Join(",", events.OrderBy(e => e, StringComparer.Ordinal)),
                    "CollapseAllGroups must raise GroupCollapsed once for each group it changed.");
                events.Clear();
                tableView.ExpandAllGroups();
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual("E:Architect,E:Designer,E:Engineer", string.Join(",", events.OrderBy(e => e, StringComparer.Ordinal)),
                    "ExpandAllGroups must raise GroupExpanding once for each group it changed.");
                events.Clear();
                tableView.ExpandAllGroups();
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() => Verify.AreEqual(0, events.Count, "A bulk call that changes nothing must raise nothing."));
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a GroupExpanding handler that lazily adds members to the group still ends with the group expanded and showing them.")]
        public void VerifyGroupExpandingHandlerCanLazyLoadItems()
        {
            TableView tableView = null;
            ObservableCollection<ShapedPerson> items = null;

            RunOnUIThread.Execute(() =>
            {
                items = new ObservableCollection<ShapedPerson>(MakeShapedPeople());
                tableView = CreateGroupedTable(TableViewSource.From(items).GroupBy(o => Person(o).Role));
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() => tableView.CollapseGroup("Designer"));

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                tableView.GroupExpanding += (s, e) => items.Add(new ShapedPerson("Zoe", "Designer", "Studio"));
                tableView.ExpandGroup("Designer");
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                VerifySequence(new[] { "#Designer", "Asha", "Ines", "Zoe", "#Engineer", "Diego", "Rafa", "Owen", "#Architect", "Mei" },
                    GetProjectedLabels(tableView, includeCount: false), "the group must expand and show the lazily added member");
                Verify.IsTrue(GetGroupInfoByKey(tableView, "Designer").IsExpanded, "The group must report expanded.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a throwing GroupExpanding handler is contained: raised once, the group still expands, and focus stays in the table.")]
        [TestProperty("data:Route", "{Peer, ExpandGroup}")]
        public void VerifyThrowingGroupExpandingHandlerIsContained()
        {
            var route = ((string)TestContext.DataRow["Route"]).Trim();
            TableView tableView = null;
            Exception escaped = null;
            var raised = 0;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateGroupedTable();
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() => tableView.CollapseGroup("Designer"));

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(GetGroupHeader(tableView, 0).Focus(FocusState.Keyboard), "Precondition: the header takes focus.");
                tableView.GroupExpanding += (s, e) =>
                {
                    raised++;
                    throw new InvalidOperationException("GroupExpanding handler fault");
                };
                try
                {
                    if (route == "Peer")
                    {
                        GetExpandCollapseProvider(tableView, 0).Expand();
                    }
                    else
                    {
                        tableView.ExpandGroup("Designer");
                    }
                }
                catch (Exception e)
                {
                    escaped = e;
                }

                if (route == "ExpandGroup")
                {
                    Verify.AreEqual(1, raised, "ExpandGroup must raise GroupExpanding synchronously.");
                }
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsNull(escaped, $"{route}: a handler fault must not escape the expand request.");
                Verify.AreEqual(1, raised, $"{route}: GroupExpanding must be raised exactly once.");
                Verify.IsTrue(GetGroupInfoByKey(tableView, "Designer").IsExpanded, $"{route}: the group must still expand.");
                Verify.IsTrue(GetProjectedLabels(tableView, includeCount: false).Contains("Asha"), $"{route}: the expanded group's rows must show.");
                var focused = FocusManager.GetFocusedElement(tableView.XamlRoot) as DependencyObject;
                var inTable = false;
                for (var current = focused; current != null; current = VisualTreeHelper.GetParent(current))
                {
                    inTable |= ReferenceEquals(current, tableView);
                }
                Verify.IsTrue(inTable, $"{route}: focus must stay inside the table.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a GroupExpanding handler that calls ExpandGroup on the same group is a no-op: one event, the group expanded, no recursion.")]
        [TestProperty("data:Route", "{Peer, ExpandGroup}")]
        public void VerifyGroupExpandingHandlerReexpandingSameGroupIsNoOp()
        {
            var route = ((string)TestContext.DataRow["Route"]).Trim();
            TableView tableView = null;
            var raised = 0;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateGroupedTable();
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() => tableView.CollapseGroup("Designer"));

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                tableView.GroupExpanding += (s, e) =>
                {
                    // Bounded so a broken guard fails the test instead of overflowing the stack.
                    if (++raised < 5)
                    {
                        tableView.ExpandGroup(e.GroupKey);
                    }
                };
                if (route == "Peer")
                {
                    GetExpandCollapseProvider(tableView, 0).Expand();
                }
                else
                {
                    tableView.ExpandGroup("Designer");
                }
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(1, raised, $"{route}: GroupExpanding must be raised once.");
                Verify.IsTrue(GetGroupInfoByKey(tableView, "Designer").IsExpanded, $"{route}: the group must be expanded.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a GroupExpanding handler that calls CollapseGroup on the same group cannot reverse the pinned direction: state and events stay consistent.")]
        public void VerifyGroupExpandingHandlerCollapsingSameGroupKeepsStateConsistent()
        {
            TableView tableView = null;
            var events = new List<string>();

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateGroupedTable();
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() => tableView.CollapseGroup("Designer"));

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                tableView.GroupCollapsed += (s, e) => events.Add("C:" + e.GroupKey);
                tableView.GroupExpanding += (s, e) =>
                {
                    events.Add("E:" + e.GroupKey);
                    tableView.CollapseGroup(e.GroupKey);
                };
                GetExpandCollapseProvider(tableView, 0).Expand();
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(1, events.Count(e => e.StartsWith("E:", StringComparison.Ordinal)), "GroupExpanding must be raised once.");
                var expanded = GetGroupInfoByKey(tableView, "Designer").IsExpanded;
                var lastWasCollapse = events.Last().StartsWith("C:", StringComparison.Ordinal);
                Verify.AreEqual(!lastWasCollapse, expanded, $"The final state must match the last event; events [{string.Join(", ", events)}].");
                Verify.AreEqual(expanded, GetProjectedLabels(tableView, includeCount: false).Contains("Asha"), "The projection must match the reported state.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a GroupExpanding handler that swaps ItemsSource to a source with the same keys does not apply the expansion to the new source's groups.")]
        public void VerifyGroupExpandingHandlerSwappingItemsSourceLeavesNewGroupsAlone()
        {
            TableView tableView = null;
            var events = 0;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateGroupedTable();
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() => tableView.CollapseGroup("Designer"));

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                var swapped = false;
                tableView.GroupExpanding += (s, e) =>
                {
                    events++;
                    if (!swapped)
                    {
                        swapped = true;
                        tableView.ItemsSource = TableViewSource.From(MakeShapedPeople()).GroupBy(o => Person(o).Role);
                    }
                };
                tableView.GroupCollapsed += (s, e) => events++;
                tableView.ExpandGroup("Designer");
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(1, events, "Only the original GroupExpanding may be raised.");
                VerifySequence(new[] { "#Designer", "Asha", "Ines", "#Engineer", "Diego", "Rafa", "Owen", "#Architect", "Mei" },
                    GetProjectedLabels(tableView, includeCount: false), "the new source's groups must be in their default state");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies ExpandAllGroups with a GroupExpanding handler that regroups raises no event for groups whose state never changes.")]
        public void VerifyBulkExpandWithRegroupingHandlerRaisesOnlyForChangedGroups()
        {
            TableView tableView = null;
            TableViewSource source = null;
            var events = new List<string>();

            RunOnUIThread.Execute(() =>
            {
                source = TableViewSource.From(MakeShapedPeople()).GroupBy(o => Person(o).Role);
                tableView = CreateGroupedTable(source);
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() => tableView.CollapseGroup("Designer"));

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                tableView.GroupExpanding += (s, e) =>
                {
                    events.Add("E:" + e.GroupKey);
                    source.GroupBy(o => Person(o).Role);
                };
                tableView.GroupCollapsed += (s, e) => events.Add("C:" + e.GroupKey);
                tableView.ExpandAllGroups();
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                Verify.IsFalse(events.Any(e => e != "E:Designer"), $"Only the collapsed group may raise an event; saw [{string.Join(", ", events)}].");
                Verify.IsTrue(events.Count <= 1, "The changed group must raise at most one GroupExpanding.");
                Verify.IsTrue(GetProjectedLabels(tableView, includeCount: false).Contains("Asha"), "Every group must end expanded.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies ExpandGroup/CollapseGroup match keys by group identity: a new key instance under an identity selector, and a boxed enum key.")]
        [TestProperty("data:Key", "{Identity, Enum}")]
        public void VerifyExpandCollapseGroupMatchesByIdentity()
        {
            var keyKind = ((string)TestContext.DataRow["Key"]).Trim();
            TableView tableView = null;
            Func<object> key = null;

            RunOnUIThread.Execute(() =>
            {
                var source = TableViewSource.From(MakeShapedPeople());
                if (keyKind == "Identity")
                {
                    source.GroupBy(o => Person(o).Department, k => ((Department)k).Name);
                    key = () => new Department { Name = "Platform" };
                }
                else
                {
                    source.GroupBy(o => Person(o).Role == "Engineer" ? TableViewDensity.Compact : TableViewDensity.Standard);
                    key = () => TableViewDensity.Compact;
                }

                tableView = CreateGroupedTable(source);
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() => tableView.CollapseGroup(key()));

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                var labels = GetProjectedLabels(tableView, includeCount: false);
                Verify.IsFalse(labels.Contains("Diego"), $"{keyKind}: the matching group must collapse.");
                Verify.IsTrue(labels.Contains("Asha"), $"{keyKind}: other groups must stay expanded.");
                tableView.ExpandGroup(key());
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
                Verify.IsTrue(GetProjectedLabels(tableView, includeCount: false).Contains("Diego"), $"{keyKind}: the matching group must expand again."));
        }

        #endregion

        #region 9.6 Selection across a regroup

        [TestMethod]
        [TestProperty("Description", "Verifies a regroup that moves the selected row into a later group keeps it selected without raising SelectionChanged.")]
        public void VerifySelectionSurvivesRegroupWhenKeyChanges()
        {
            TableView tableView = null;
            TableViewSource source = null;
            ShapedPerson ines = null;
            var changes = 0;

            RunOnUIThread.Execute(() =>
            {
                var people = MakeShapedPeople();
                ines = people.First(person => person.Name == "Ines");
                source = TableViewSource.From(people);
                source.GroupBy(o => Person(o).Role);

                tableView = CreateGroupedTable(source);
                LoadContent(tableView);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                tableView.Select(GetProjectedItems(tableView).IndexOf(ines));
                Verify.AreSame(ines, tableView.SelectedItem, "Precondition: Ines is selected.");

                // Designer is bucketed before Engineer, so the old group is refilled before the new one.
                ines.Role = "Engineer";
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                tableView.SelectionChanged += (s, e) => changes++;
                // GroupBy takes a delegate, so the app re-declares it after a key change.
                source.GroupBy(o => Person(o).Role);
            });

            SettleLayout(tableView);

            RunOnUIThread.Execute(() =>
            {
                VerifySequence(
                    new[] { "#Designer", "Asha", "#Engineer", "Diego", "Rafa", "Ines", "Owen", "#Architect", "Mei" },
                    GetProjectedLabels(tableView, includeCount: false),
                    "the regroup must move Ines into Engineer");
                Verify.AreSame(ines, tableView.SelectedItem, "The moved row must stay selected.");
                Verify.AreEqual(GetProjectedItems(tableView).IndexOf(ines), tableView.SelectedIndex,
                    "SelectedIndex must follow the row to its new position.");
                Verify.AreEqual(0, changes, "The selected item did not change, so no SelectionChanged may be raised.");
            });
        }

        #endregion
    }

    internal static class TableViewGroupingTestHelpers
    {
        internal static TableView CreateGroupedTable()
        {
            var source = TableViewSource.From(MakeShapedPeople());
            source.GroupBy(o => Person(o).Role);
            return CreateGroupedTable(source);
        }

        // Tall enough that every header and row in these fixtures realizes, so TryGetElement never
        // legitimately returns null.
        internal static TableView CreateGroupedTable(object itemsSource)
            => CreateTableView(itemsSource, headers: new[] { "Name", "Role" }, width: 600, height: 800);

        // The text a group header renders for its CONTENT, whichever template produced it. Scoped to
        // the content presenter so the expander glyph - a private-use character in an icon TextBlock
        // - stays out of the comparison.
        internal static string HeaderText(TableView tableView, int index)
        {
            var header = GetGroupHeader(tableView, index);
            if (header == null)
            {
                return string.Empty;
            }

            var presenter = FindVisualChildrenByType<ContentPresenter>(header).FirstOrDefault();
            if (presenter == null)
            {
                Verify.Fail("A group header should host its content in a ContentPresenter.");
                return string.Empty;
            }

            return string.Concat(FindVisualChildrenByType<TextBlock>(presenter).Select(textBlock => textBlock.Text));
        }

        // A marker the test can recognize, plus a binding that proves Content is the group info.
        internal static DataTemplate CreateMarkerTemplate(string marker) => (DataTemplate)XamlReader.Load(
            $@"<DataTemplate xmlns=""http://schemas.microsoft.com/winfx/2006/xaml/presentation"">
                   <StackPanel Orientation=""Horizontal"">
                       <TextBlock Text=""{marker}"" />
                       <TextBlock Text=""{{Binding KeyText}}"" />
                   </StackPanel>
               </DataTemplate>");
    }
}
