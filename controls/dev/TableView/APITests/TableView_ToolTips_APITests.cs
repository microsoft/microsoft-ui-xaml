// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Automation;
using Microsoft.UI.Xaml.Automation.Peers;
using Microsoft.UI.Xaml.Automation.Provider;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Tabular;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Markup;
using MUXControlsTestApp.Utilities;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;

using WEX.Logging.Interop;
using WEX.TestExecution;
using WEX.TestExecution.Markup;

using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewTestHelpers;
using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewEditingTestHelpers;
using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewRowTestHelpers;
using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewToolTipTestHelpers;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    // Category 14 of the TableView test plan: tooltips.
    //
    // Spec basis: TableView-functional-spec.md:78-88, which is unusually complete for this control - it
    // states the opt-in, author precedence, what content is allowed, the accessibility projection and
    // its duplicate suppression, and the recycling contract. The IDL adds the two property-level
    // contracts: TableView.idl:116-119 (HeaderToolTip, "null or empty means no tooltip", explicitly not
    // a Binding property) and :212-214 (CellToolTipBinding, "bound against the row's data item", a CLR
    // property so XAML passes the Binding through unevaluated).
    //
    // Why these are API tests even though a tooltip is a hover affordance: every assertion here is about
    // the tooltip OBJECT and its automation projection, not about a popup opening.
    // ToolTipService.GetToolTip(element) returns the attached ToolTip and its Content is readable with
    // no pointer involved; the accessibility half is AutomationProperties.GetHelpText on the element and
    // GetHelpText() on the peer. Reworded as "a tooltip appears on hover" every test below would become
    // an interaction test and assert strictly less.
    //
    // NOT IMPLEMENTED HERE:
    //   Defaults and DP identity     - TableViewTests.cs:151/:167/:181 assert HeaderToolTip and
    //                                  CellToolTipBinding default to null, :264 the DP static, :367 the
    //                                  settable roundtrip.
    //   Sort-state-only help text    - TableView_AutomationPeer_APITests.cs owns the header help text
    //                                  case with no tooltip in play. This file owns every case where a
    //                                  tooltip and a help text meet.
    //   Group header tooltips        - deferred by TableView-functional-spec.md:86 until grouping ships.
    //   An app tooltip on the cell   - the wrapper Border is created by the row and is not reachable
    //   WRAPPER itself                 from public API before the control attaches its own tooltip, so
    //                                  the ownership check that guards it is verifiable only by
    //                                  inspection. 14.1 covers the reachable half: an authored tooltip
    //                                  inside the CellTemplate.
    [TestClass]
    public class TableViewToolTipTests : TableViewApiTestBase
    {
        #region 14.1 Cell tooltips

        [TestMethod]
        [TestProperty("Description", "Verifies a column with no CellToolTipBinding creates no ToolTip object on any cell.")]
        public void VerifyNoCellToolTipBindingCreatesNoToolTip()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateToolTipTable(MakeItems(), cellToolTipPath: null);
                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var rows = GetRealizedRows(tableView);
                Verify.IsGreaterThan(rows.Count, 0, "The table must realize rows for this test to mean anything.");

                foreach (var row in rows)
                {
                    for (int i = 0; i < tableView.Columns.Count; i++)
                    {
                        var wrapper = GetRowCell(row, i);

                        // The spec states the cost, not just the absence: "No binding means no tooltip
                        // and no per-cell cost." A ToolTip per cell per row would land on the hot
                        // virtualization path.
                        Verify.IsNull(ToolTipService.GetToolTip(wrapper),
                            "A column with no CellToolTipBinding must not attach a ToolTip to its cells.");
                    }
                }
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a string CellToolTipBinding produces a per-row ToolTip carrying that row's value.")]
        public void VerifyStringCellToolTipProducesToolTipWithThatText()
        {
            TableView tableView = null;
            var items = MakeItems();

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateToolTipTable(items, cellToolTipPath: "Role");
                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var rows = GetRealizedRows(tableView);
                Verify.IsGreaterThan(rows.Count, 1, "Two rows are needed: one row cannot distinguish a per-row value from a constant.");

                // Asserting two rows is the point. The binding is specified as evaluated "against each
                // row's data item", and a single-row assertion passes just as happily against an
                // implementation that evaluates once and shares the result.
                for (int rowIndex = 0; rowIndex < 2; rowIndex++)
                {
                    var wrapper = GetRowCell(rows[rowIndex], 0);
                    Verify.AreEqual(items[rowIndex].Role, RequireToolTipContent(wrapper, $"row {rowIndex}"),
                        $"Row {rowIndex}'s tooltip must carry row {rowIndex}'s bound value.");
                }
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies an empty or null cell tooltip value attaches no tooltip, while real values still do.")]
        public void VerifyEmptyOrNullCellToolTipValueProducesNoToolTip()
        {
            TableView tableView = null;

            // Row 0 has a real value, so a blanket "never attaches" implementation cannot pass.
            var items = new List<Person>
            {
                new Person { Name = "Asha", Role = "Designer" },
                new Person { Name = "Diego", Role = string.Empty },
                new Person { Name = "Mei", Role = null },
            };

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateToolTipTable(items, cellToolTipPath: "Role");
                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var rows = GetRealizedRows(tableView);
                Verify.IsGreaterThan(rows.Count, 2, "All three rows must be realized.");

                Verify.AreEqual("Designer", RequireToolTipContent(GetRowCell(rows[0], 0), "row 0"),
                    "A row with a real value must still get its tooltip.");

                // TableView.idl:212-213 - "null or empty means no tooltip". An empty popup is worse
                // than none: it opens over the content it is meant to explain.
                Verify.IsNull(ToolTipService.GetToolTip(GetRowCell(rows[1], 0)),
                    "An empty-string tooltip value must attach no tooltip.");
                Verify.IsNull(ToolTipService.GetToolTip(GetRowCell(rows[2], 0)),
                    "A null tooltip value must attach no tooltip.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a non-string cell tooltip value is hosted as ToolTip content rather than stringified.")]
        public void VerifyNonStringCellToolTipContentIsHosted()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                // A fresh element per evaluation, per the spec: one UIElement cannot be parented by two
                // ToolTips, so a shared instance would fail for reasons unrelated to this behaviour.
                tableView = CreateToolTipTable(MakeItems(), cellToolTipPath: "Role", converter: new BorderToolTipConverter());
                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var rows = GetRealizedRows(tableView);
                Verify.IsGreaterThan(rows.Count, 1, "Two rows are needed to show each gets its own element.");

                var first = RequireToolTip(GetRowCell(rows[0], 0), "row 0").Content as Border;
                var second = RequireToolTip(GetRowCell(rows[1], 0), "row 1").Content as Border;

                if (first == null || second == null)
                {
                    Verify.Fail("A non-string tooltip value must be hosted as ToolTip content, not stringified.");
                    return;
                }

                Verify.AreEqual("Designer", (first.Child as TextBlock)?.Text,
                    "The hosted element must be the one the converter produced for this row.");
                Verify.IsFalse(ReferenceEquals(first, second),
                    "Each row must host its own element; a shared one could not be parented twice.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a cell tooltip value that is itself a ToolTip is rejected rather than nested.")]
        public void VerifyToolTipValuedCellToolTipIsRejected()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateToolTipTable(MakeItems(), cellToolTipPath: "Role", converter: new ToolTipValuedConverter());
                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var wrapper = GetRowCell(RequireFirstRow(tableView), 0);

                // A ToolTip is not "content a ToolTip can host" (functional spec :81): nesting one
                // inside another puts the inner tooltip's placement in a fight with the placement the
                // control owns. ToolTipService.SetToolTip accepts this shape, so an app reaches it
                // easily and the control has to refuse it.
                Verify.IsNull(ToolTipService.GetToolTip(wrapper),
                    "A ToolTip-valued tooltip must leave the cell with no tooltip at all, not a nested one.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a tooltip authored inside a CellTemplate survives alongside the column's own cell tooltip.")]
        public void VerifyAuthoredToolTipInCellTemplateIsIndependentOfTheColumnToolTip()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateTableViewShell(MakeItems(), 500, 300);

                tableView.Columns.Add(new TableViewTemplateColumn
                {
                    Header = "Name",
                    Width = new GridLength(200.0, GridUnitType.Pixel),
                    CellTemplate = (DataTemplate)XamlReader.Load(
                        @"<DataTemplate xmlns=""http://schemas.microsoft.com/winfx/2006/xaml/presentation"">
                              <Border ToolTipService.ToolTip=""Authored by the app"">
                                  <TextBlock Text=""{Binding Name}"" />
                              </Border>
                          </DataTemplate>"),
                    CellToolTipBinding = new Binding { Path = new PropertyPath("Role"), Mode = BindingMode.OneWay },
                });

                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var wrapper = GetRowCell(RequireFirstRow(tableView), 0);

                // The control's tooltip covers the rest of the cell; the authored one opens over the
                // content it was authored on. Both must survive - the control "never touches a tooltip
                // it did not attach", and app content it walked into and overwrote is unrecoverable.
                Verify.AreEqual("Designer", RequireToolTipContent(wrapper, "the cell wrapper"),
                    "The column's own tooltip must still be attached to the cell wrapper.");

                var authoredHost = FindVisualChildrenByType<Border>(wrapper)
                    .FirstOrDefault(b => !ReferenceEquals(b, wrapper) && ToolTipService.GetToolTip(b) != null);

                if (authoredHost == null)
                {
                    Verify.Fail("The tooltip authored inside the CellTemplate must survive on the template root.");
                    return;
                }

                Verify.AreEqual("Authored by the app", ToolTipService.GetToolTip(authoredHost) as string,
                    "The authored tooltip's content must be untouched by the control.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a recycled row shows the new item's tooltip, never the previous item's.")]
        public void VerifyRecycledRowShowsTheNewItemsToolTip()
        {
            TableView tableView = null;
            var items = MakeManyItems(200);

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateToolTipTable(items, cellToolTipPath: "Role");
                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            // ScrollBodyTo does the double settle. A row sampled before the repeater
            // finishes re-assigning DataContext is parented but unbound, and reading it there has twice
            // produced a false product bug in this control.
            ScrollBodyTo(tableView, ScrollAxis.Vertical, 2000.0);

            RunOnUIThread.Execute(() =>
            {
                var rows = GetRealizedRows(tableView);
                Verify.IsGreaterThan(rows.Count, 0, "Rows must be realized after the scroll.");

                foreach (var row in rows)
                {
                    var item = row.DataContext as Person;
                    if (item == null)
                    {
                        Verify.Fail("Every realized row must carry its item as an inherited DataContext.");
                        continue;
                    }

                    // The cell text being right while the tooltip is stale is the form of this bug most
                    // likely to ship, because only hovering reveals it.
                    Verify.AreEqual(item.Role, RequireToolTipContent(GetRowCell(row, 0), $"the row for {item.Name}"),
                        "A recycled row's tooltip must re-resolve to its current item.");
                }
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies recycled rows keep their tooltip and help text when the bound value does not change between items.")]
        public void VerifyRecycledRowKeepsToolTipWhenItemsShareTheValue()
        {
            TableView tableView = null;
            var items = MakeManyItems(200);

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateToolTipTable(items, cellToolTipPath: null);
                // An explicit Source keeps the value constant across the DataContext reset, so a
                // recycled cell gets no property-changed callback to revive its tooltip.
                tableView.Columns[0].CellToolTipBinding = new Binding { Source = "Shared role", Mode = BindingMode.OneWay };
                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            ScrollBodyTo(tableView, ScrollAxis.Vertical, 2000.0);
            ScrollBodyTo(tableView, ScrollAxis.Vertical, 4000.0);

            RunOnUIThread.Execute(() =>
            {
                var rows = GetRealizedRows(tableView);
                Verify.IsGreaterThan(rows.Count, 0, "Rows must be realized after the scroll.");

                foreach (var row in rows)
                {
                    var item = row.DataContext as Person;
                    var wrapper = GetRowCell(row, 0);
                    var toolTip = RequireToolTip(wrapper, $"the row for {item?.Name}");
                    Verify.AreEqual("Shared role", toolTip.Content as string, "A recycled row must keep its tooltip content.");
                    Verify.IsTrue(toolTip.IsEnabled, "A recycled row's tooltip must be enabled.");
                    Verify.AreEqual("Shared role", AutomationProperties.GetHelpText(wrapper),
                        "A recycled row must re-publish the tooltip as help text.");
                }
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a source PropertyChanged updates a live cell tooltip in place, on the same ToolTip instance.")]
        public void VerifyItemPropertyChangeUpdatesCellToolTipInPlace()
        {
            TableView tableView = null;
            var items = MakeEditableItems("Asha", "Diego", "Mei");
            ToolTip original = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateToolTipTable(items, cellToolTipPath: "Role");
                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                original = RequireToolTip(GetRowCell(RequireFirstRow(tableView), 0), "row 0");
                Verify.AreEqual("Designer", original.Content as string, "Precondition: the tooltip starts at the bound value.");

                items[0].Role = "Principal Designer";
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var current = RequireToolTip(GetRowCell(RequireFirstRow(tableView), 0), "row 0");

                Verify.AreEqual("Principal Designer", current.Content as string,
                    "A source PropertyChanged must reach a live cell tooltip.");

                // "In place" is the load-bearing half. Replacing the ToolTip would drop a live popup's
                // child, which is the shape behind the reentrant CPopup::RemoveChild crash the control
                // comments on - and without this assertion the test only re-proves that the binding works.
                Verify.IsTrue(ReferenceEquals(original, current),
                    "The update must reuse the same ToolTip instance rather than attaching a new one.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a cell tooltip is present again once an edit on that cell closes.")]
        public void VerifyCellToolTipIsRestoredAfterAnEditCloses()
        {
            TableView tableView = null;
            var items = MakeEditableItems("Asha", "Diego", "Mei");

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateEditableTable(items);
                tableView.IsReadOnly = false;
                tableView.Columns[0].CellToolTipBinding =
                    new Binding { Path = new PropertyPath("Role"), Mode = BindingMode.OneWay };

                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual("Designer", RequireToolTipContent(GetRowCell(RequireFirstRow(tableView), 0), "row 0"),
                    "Precondition: the editable cell starts with its tooltip.");

                // SetValue runs BeginEdit -> write -> CommitEdit inside one call, so the mid-edit state
                // is not observable from here. The spec says nothing about tooltips during editing;
                // only the restoration is asserted, which holds under any reading - a display cell must
                // carry its tooltip.
                var error = CaptureSetValue(GetValueProvider(tableView, items[0], columnIndex: 0), "Asha Kapoor");
                Verify.IsTrue(error == null, $"The edit must succeed for this test to say anything. Instead: {error?.Message}");
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsFalse(tableView.IsEditing, "The edit must have closed.");

                Verify.AreEqual("Designer", RequireToolTipContent(GetRowCell(RequireFirstRow(tableView), 0), "row 0"),
                    "A cell must carry its tooltip again once the edit closes; losing it permanently is a state the user cannot escape.");
            });
        }

        #endregion

        #region 14.2 Cell tooltip accessibility

        [TestMethod]
        [TestProperty("Description", "Verifies a string cell tooltip is published as help text on the element and reported by the peer.")]
        public void VerifyStringCellToolTipIsPublishedAsHelpText()
        {
            TableView tableView = null;
            var items = MakeItems();

            RunOnUIThread.Execute(() =>
            {
                // The cell displays Name; the tooltip carries Role, so the two differ and suppression
                // (covered separately below) is not in play here.
                tableView = CreateToolTipTable(items, cellToolTipPath: "Role");
                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var row = RequireFirstRow(tableView);
                var wrapper = GetRowCell(row, 0);

                // Element and peer are asserted separately on purpose: the element is where the control
                // publishes, the peer is what a client actually reads, and 14.3 shows the two are not
                // always the same.
                Verify.AreEqual("Designer", AutomationProperties.GetHelpText(wrapper),
                    "String tooltip text must be published as the cell's AutomationProperties.HelpText.");

                Verify.AreEqual("Designer", GetCellPeer(row, 0).GetHelpText(),
                    "And it must survive to the peer, or a screen reader never sees it.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a non-string cell tooltip publishes no help text rather than a ToString artefact.")]
        public void VerifyNonStringCellToolTipPublishesNoHelpText()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateToolTipTable(MakeItems(), cellToolTipPath: "Role", converter: new BorderToolTipConverter());
                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var row = RequireFirstRow(tableView);
                var wrapper = GetRowCell(row, 0);

                Verify.IsNotNull(ToolTipService.GetToolTip(wrapper) as ToolTip,
                    "Precondition: the rich tooltip is attached, so an empty help text is a decision and not an accident.");

                // Announcing "Microsoft.UI.Xaml.Controls.Border" is worse than silence: meaningless and
                // confidently spoken.
                Verify.AreEqual(string.Empty, AutomationProperties.GetHelpText(wrapper),
                    "Non-string tooltip content must not be stringified into help text.");

                var peerHelpText = GetCellPeer(row, 0).GetHelpText();
                Verify.IsTrue(string.IsNullOrEmpty(peerHelpText),
                    $"The peer must report no help text for non-string content; saw '{peerHelpText}'.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies the cell peer withholds help text that merely repeats the cell's own value, but keeps text that does not.")]
        public void VerifyCellPeerSuppressesHelpTextThatRepeatsTheCellValue()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateTableViewShell(MakeItems(), 600, 300);

                // Column 0: tooltip repeats what the cell shows - the most common authoring shape for
                // this feature, since you tooltip the column you truncate.
                tableView.Columns.Add(new TableViewTextColumn
                {
                    Header = "Name",
                    Binding = new Binding { Path = new PropertyPath("Name"), Mode = BindingMode.OneWay },
                    CellToolTipBinding = new Binding { Path = new PropertyPath("Name"), Mode = BindingMode.OneWay },
                });

                // Column 1: same tooltip text over a different cell value. Without this arm, a peer that
                // returned empty unconditionally would pass.
                tableView.Columns.Add(new TableViewTextColumn
                {
                    Header = "Role",
                    Binding = new Binding { Path = new PropertyPath("Role"), Mode = BindingMode.OneWay },
                    CellToolTipBinding = new Binding { Path = new PropertyPath("Name"), Mode = BindingMode.OneWay },
                });

                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var row = RequireFirstRow(tableView);

                // Suppression happens at UIA query time, not at attach: the element keeps the text and
                // only the peer withholds it.
                Verify.AreEqual("Asha", AutomationProperties.GetHelpText(GetRowCell(row, 0)),
                    "The published help text stays on the element; suppression is a query-time decision.");

                var duplicate = GetCellPeer(row, 0).GetHelpText();
                Verify.IsTrue(string.IsNullOrEmpty(duplicate),
                    $"Help text that merely repeats the cell value must be withheld, or the value is announced twice; saw '{duplicate}'.");

                Verify.AreEqual("Asha", GetCellPeer(row, 1).GetHelpText(),
                    "Help text that differs from the cell value must still be reported.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies help text the app set on a cell is never dropped, even when it repeats the cell value.")]
        public void VerifyCellPeerKeepsAppAuthoredHelpText()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                // No CellToolTipBinding, so the control owns no tooltip on this cell and holds no
                // ownership record for it.
                tableView = CreateToolTipTable(MakeItems(), cellToolTipPath: null);
                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var row = RequireFirstRow(tableView);
                var wrapper = GetRowCell(row, 0);

                // Set to exactly the cell's own value, so the suppression rule would fire if it were not
                // gated on the ownership record. Setting it on the wrapper is the only route an app has:
                // the peer reads help text from its owner, which is the wrapper, not the cell content.
                AutomationProperties.SetHelpText(wrapper, "Asha");

                Verify.AreEqual("Asha", GetCellPeer(row, 0).GetHelpText(),
                    "Suppression must be gated on the control's ownership record; app-authored help text has no other way back.");
            });
        }

        #endregion

        #region 14.3 Header tooltips

        [TestMethod]
        [TestProperty("Description", "Verifies a string HeaderToolTip attaches a ToolTip to the whole header cell.")]
        public void VerifyStringHeaderToolTipProducesToolTipWithThatText()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateTableView(headers: new[] { "Name", "Role" });
                tableView.Columns[0].HeaderToolTip = "The person's display name";
                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var headerCell = GetHeaderCell(tableView, 0);

                // On the cell, not its ContentPresenter: the spec has it "cover the whole header cell,
                // including its padding and sort affordance", which is most of the header's hit area.
                Verify.AreEqual("The person's display name", RequireToolTipContent(headerCell, "header cell 0"),
                    "A string HeaderToolTip must attach a tooltip to the header cell itself.");

                Verify.IsNull(ToolTipService.GetToolTip(GetHeaderPresenter(tableView, 0)),
                    "The tooltip must not sit on the inner presenter, which covers only the header content.");

                Verify.IsNull(ToolTipService.GetToolTip(GetHeaderCell(tableView, 1)),
                    "A column that set no HeaderToolTip must not acquire one.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a non-string HeaderToolTip is hosted as tooltip content.")]
        public void VerifyNonStringHeaderToolTipContentIsHosted()
        {
            TableView tableView = null;
            Border content = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateTableView(headers: new[] { "Name" });
                content = new Border { Child = new TextBlock { Text = "Rich header tooltip" } };
                tableView.Columns[0].HeaderToolTip = content;
                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                // HeaderToolTip is typed Object (TableView.idl:119) precisely so this works.
                var hosted = RequireToolTip(GetHeaderCell(tableView, 0), "header cell 0").Content;
                Verify.IsTrue(ReferenceEquals(content, hosted),
                    "A non-string HeaderToolTip must be hosted as the tooltip's content, not stringified.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies an unset or empty-string HeaderToolTip attaches no header tooltip.")]
        public void VerifyNullOrEmptyHeaderToolTipProducesNoToolTip()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateTableView(headers: new[] { "Name", "Role", "Team" });
                // Column 0 unset, column 1 empty, column 2 real - so a blanket "never attaches"
                // implementation cannot pass.
                tableView.Columns[1].HeaderToolTip = string.Empty;
                tableView.Columns[2].HeaderToolTip = "Owning team";
                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsNull(ToolTipService.GetToolTip(GetHeaderCell(tableView, 0)),
                    "An unset HeaderToolTip must attach nothing.");
                Verify.IsNull(ToolTipService.GetToolTip(GetHeaderCell(tableView, 1)),
                    "An empty-string HeaderToolTip must attach nothing - TableView.idl:116-117 makes empty equivalent to null.");
                Verify.AreEqual("Owning team", RequireToolTipContent(GetHeaderCell(tableView, 2), "header cell 2"),
                    "A column with a real value must still get its tooltip.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies HeaderToolTip changes after load are re-applied to the realized header.")]
        public void VerifyHeaderToolTipChangeIsReappliedInPlace()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateTableView(headers: new[] { "Name" });
                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsNull(ToolTipService.GetToolTip(GetHeaderCell(tableView, 0)),
                    "Precondition: no tooltip before one is set.");

                tableView.Columns[0].HeaderToolTip = "First";
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual("First", RequireToolTipContent(GetHeaderCell(tableView, 0), "header cell 0"),
                    "Setting HeaderToolTip after load must reach the realized header.");

                tableView.Columns[0].HeaderToolTip = "Second";
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual("Second", RequireToolTipContent(GetHeaderCell(tableView, 0), "header cell 0"),
                    "Replacing the value must replace the content, not leave the first one in place.");

                tableView.Columns[0].HeaderToolTip = null;
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                // Header cells are rebuilt rather than recycled, so there is no binding here - only the
                // property-changed path, which is what this whole test exercises.
                var remaining = ToolTipService.GetToolTip(GetHeaderCell(tableView, 0));
                if (remaining is ToolTip toolTip)
                {
                    Verify.IsNull(toolTip.Content, "Clearing HeaderToolTip must leave no tooltip content behind.");
                    Verify.IsFalse(toolTip.IsEnabled, "A neutralized tooltip must not be able to open.");
                }
                else
                {
                    Verify.IsNull(remaining, "Clearing HeaderToolTip must remove the header tooltip.");
                }
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a header tooltip reaches automation through the peer, not through help text on the element.")]
        public void VerifyHeaderToolTipPublishesNoHelpTextOnTheElement()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateTableView(headers: new[] { "Name" });
                // Unsortable, so the peer has no sort state to append: this test is about where the
                // tooltip text is published, and VerifyHeaderPeerHelpTextCombinesToolTipAndSortState
                // owns the tooltip + sort-state join.
                tableView.Columns[0].CanSort = false;
                tableView.Columns[0].HeaderToolTip = "The person's display name";
                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                // Deliberately the opposite of the cell case in 14.2. The header peer is virtual, so
                // help text on the element would never reach a client - publishing it anyway would look
                // like working accessibility to anyone inspecting the tree.
                Verify.AreEqual(string.Empty, AutomationProperties.GetHelpText(GetHeaderCell(tableView, 0)),
                    "Header tooltips must not publish help text onto the element, where no client would read it.");

                var peer = new TableViewColumnHeaderAutomationPeer(tableView, tableView.Columns[0]);
                Verify.AreEqual("The person's display name", peer.GetHelpText(),
                    "The header peer is the only route a client has to the tooltip text.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies header help text carries both the tooltip text and the current sort state.")]
        public void VerifyHeaderPeerHelpTextCombinesToolTipAndSortState()
        {
            TableView tableView = null;
            var helpTexts = new List<string>();

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateTableView(headers: new[] { "Name" });
                tableView.CanUserSortColumns = true;
                tableView.Columns[0].CanSort = true;
                tableView.Columns[0].SortMemberPath = "Name";
                tableView.Columns[0].HeaderToolTip = "The person's display name";
                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var column = tableView.Columns[0];

                foreach (var direction in new[] { SortDirection.None, SortDirection.Ascending, SortDirection.Descending })
                {
                    tableView.SortByColumn(column, direction);

                    var text = new TableViewColumnHeaderAutomationPeer(tableView, column).GetHelpText();
                    Log.Comment($"HelpText at {direction}: '{text}'");
                    helpTexts.Add(text);
                }

                // Two independent pieces of information; neither may silently displace the other.
                foreach (var text in helpTexts)
                {
                    Verify.IsTrue(text != null && text.Contains("The person's display name"),
                        $"Header help text must keep the tooltip text at every sort direction; saw '{text}'.");
                }

                Verify.AreEqual(3, helpTexts.Distinct().Count(),
                    "Header help text must still vary with sort direction once a tooltip is set.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a header tooltip that repeats the header name is dropped from help text, while a distinct one is kept.")]
        public void VerifyHeaderPeerHelpTextDropsAToolTipThatRepeatsTheHeaderName()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateTableView(headers: new[] { "Name", "Role" });
                // Unsortable, so help text is the tooltip alone and the dedup rule is observable in
                // isolation; a sortable column would append its sort state to both arms.
                tableView.Columns[0].CanSort = false;
                tableView.Columns[1].CanSort = false;
                tableView.Columns[0].HeaderToolTip = "Name";          // identical to the header
                tableView.Columns[1].HeaderToolTip = "What they do";   // distinct from it
                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var duplicatePeer = new TableViewColumnHeaderAutomationPeer(tableView, tableView.Columns[0]);
                Verify.AreEqual("Name", duplicatePeer.GetName(), "Precondition: the header name is the string header.");

                var duplicateHelpText = duplicatePeer.GetHelpText();
                Verify.IsTrue(!string.Equals("Name", duplicateHelpText, StringComparison.Ordinal),
                    $"A tooltip repeating the header name must not be restated as help text; saw '{duplicateHelpText}'.");

                // The second arm is what stops a peer that returns empty unconditionally from passing.
                Verify.AreEqual("What they do", new TableViewColumnHeaderAutomationPeer(tableView, tableView.Columns[1]).GetHelpText(),
                    "A tooltip that adds information must still be reported.");
            });
        }

        #endregion

        #region 14.4 Row tooltips

        [TestMethod]
        [TestProperty("Description", "Verifies RowToolTipBinding attaches a ToolTip to each row carrying that row's value, and none to its cells.")]
        public void VerifyRowToolTipBindingProducesPerRowToolTip()
        {
            TableView tableView = null;
            var items = MakeItems();

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateRowToolTipTable(items, rowToolTipPath: "Role");
                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var rows = GetRealizedRows(tableView);
                Verify.IsGreaterThan(rows.Count, 1, "Two rows are needed: one row cannot distinguish a per-row value from a constant.");

                for (int rowIndex = 0; rowIndex < 2; rowIndex++)
                {
                    // On the row itself: keyboard focus lands on the row, and ToolTipService opens a
                    // tooltip on focus only for the focused element or one of its ancestors.
                    Verify.AreEqual(items[rowIndex].Role, RequireToolTipContent(rows[rowIndex], $"row {rowIndex}"),
                        $"Row {rowIndex}'s tooltip must carry row {rowIndex}'s bound value.");
                    Verify.IsNull(ToolTipService.GetToolTip(GetRowCell(rows[rowIndex], 0)),
                        "A row tooltip must not be copied onto the row's cells.");
                }
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies no RowToolTipBinding, or an empty or null bound value, attaches no row tooltip.")]
        public void VerifyEmptyOrNullRowToolTipValueProducesNoToolTip()
        {
            TableView unset = null;
            TableView tableView = null;
            var items = new List<Person>
            {
                new Person { Name = "Asha", Role = "Designer" },
                new Person { Name = "Diego", Role = string.Empty },
                new Person { Name = "Mei", Role = null },
            };

            RunOnUIThread.Execute(() =>
            {
                unset = CreateRowToolTipTable(MakeItems(), rowToolTipPath: null);
                tableView = CreateRowToolTipTable(items, rowToolTipPath: "Role");
                LoadContent(new StackPanel { Children = { unset, tableView } });
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                foreach (var row in GetRealizedRows(unset))
                {
                    Verify.IsNull(ToolTipService.GetToolTip(row), "A table with no RowToolTipBinding must not attach a row tooltip.");
                }

                var rows = GetRealizedRows(tableView);
                Verify.IsGreaterThan(rows.Count, 2, "All three rows must be realized.");

                Verify.AreEqual("Designer", RequireToolTipContent(rows[0], "row 0"),
                    "A row with a real value must still get its tooltip.");
                Verify.IsNull(ToolTipService.GetToolTip(rows[1]), "An empty-string row tooltip value must attach no tooltip.");
                Verify.IsNull(ToolTipService.GetToolTip(rows[2]), "A null row tooltip value must attach no tooltip.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies RowToolTipBinding set in XAML reaches the control as an unevaluated Binding.")]
        public void VerifyRowToolTipBindingFromXaml()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                EnsureTabularControlsResources();

                tableView = (TableView)XamlReader.Load(
                    @"<tabular:TableView
                        xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                        xmlns:tabular='using:Microsoft.UI.Xaml.Controls.Tabular'
                        Width='500' Height='300'
                        RowToolTipBinding='{Binding Role}'>
                        <tabular:TableViewTextColumn Header='Name' Binding='{Binding Name}' />
                    </tabular:TableView>");

                // A DP would evaluate the binding against the TableView's own DataContext instead.
                Verify.IsNotNull(tableView.RowToolTipBinding, "The XAML Binding must be passed through to the CLR property.");
                Verify.AreEqual("Role", tableView.RowToolTipBinding.Path.Path);

                tableView.ItemsSource = MakeItems();
                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual("Designer", RequireToolTipContent(RequireFirstRow(tableView), "row 0"),
                    "A XAML-authored RowToolTipBinding must be evaluated per row.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies changing RowToolTipBinding after load re-applies it to realized rows, and clearing it removes the tooltip.")]
        public void VerifyRowToolTipBindingChangeIsReappliedToRealizedRows()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateRowToolTipTable(MakeItems(), rowToolTipPath: null);
                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsNull(ToolTipService.GetToolTip(RequireFirstRow(tableView)), "Precondition: no row tooltip before one is bound.");
                tableView.RowToolTipBinding = new Binding { Path = new PropertyPath("Role"), Mode = BindingMode.OneWay };
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual("Designer", RequireToolTipContent(RequireFirstRow(tableView), "row 0"),
                    "Setting RowToolTipBinding after load must reach realized rows.");
                tableView.RowToolTipBinding = new Binding { Path = new PropertyPath("Name"), Mode = BindingMode.OneWay };
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual("Asha", RequireToolTipContent(RequireFirstRow(tableView), "row 0"),
                    "Replacing the binding must replace the content.");
                tableView.RowToolTipBinding = null;
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var row = RequireFirstRow(tableView);
                // An on-screen row with no value registers no tooltip, so it cannot block anything.
                Verify.IsNull(ToolTipService.GetToolTip(row), "Clearing RowToolTipBinding must remove the row tooltip.");

                Verify.AreEqual(string.Empty, AutomationProperties.GetHelpText(row),
                    "Clearing RowToolTipBinding must retract the help text it published.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies rows a grouped reshape rebinds carry the tooltip of their new item.")]
        public void VerifyRowToolTipFollowsItemsAcrossGroupedReshape()
        {
            TableView tableView = null;
            TableViewSource source = null;
            var items = Enumerable.Range(0, 400)
                .Select(i => new Person { Name = $"Person {i}", Role = $"Team {i % 7}" })
                .ToList();

            RunOnUIThread.Execute(() =>
            {
                source = TableViewSource.From(items).GroupBy(o => ((Person)o).Role);
                tableView = CreateRowToolTipTable(source, rowToolTipPath: "Name");
                LoadContent(tableView);
            });

            SettleLayout(tableView);
            VerifyRowToolTipsMatchItemsWhileScrolling(tableView, "grouped by Role");

            RunOnUIThread.Execute(() =>
            {
                source.GroupBy(o => ((Person)o).Name.Length.ToString());
                source.Sort("Name", SortDirection.Descending);
            });

            SettleLayout(tableView);
            VerifyRowToolTipsMatchItemsWhileScrolling(tableView, "after regrouping and sorting");

            RunOnUIThread.Execute(() => source.ClearGroupBy());

            SettleLayout(tableView);
            VerifyRowToolTipsMatchItemsWhileScrolling(tableView, "after clearing the grouping");
        }

        // Scrolls far enough that rows come back out of the recycle pool, checking at each stop.
        private static void VerifyRowToolTipsMatchItemsWhileScrolling(TableView tableView, string when)
        {
            foreach (var offset in new[] { 0.0, 6000.0, 3000.0, 0.0 })
            {
                ScrollBodyTo(tableView, ScrollAxis.Vertical, offset);
                RunOnUIThread.Execute(() => VerifyEachRowToolTipMatchesItsItem(tableView, $"{when}, offset {offset}"));
            }
        }

        private static void VerifyEachRowToolTipMatchesItsItem(TableView tableView, string when)
        {
            var repeater = GetRowsRepeater(tableView);
            int checkedRows = 0;
            foreach (var row in GetRealizedRows(tableView))
            {
                if (repeater.GetElementIndex(row) >= 0 && row.DataContext is Person item && row.ActualHeight > 0)
                {
                    Verify.AreEqual(item.Name, RequireToolTipContent(row, $"the row for {item.Name}"),
                        $"A row's tooltip must match its current item {when}.");
                    checkedRows++;
                }
            }

            Verify.IsGreaterThan(checkedRows, 0, $"Rows must be realized {when}.");
        }
        [TestMethod]
        [TestProperty("Description", "Verifies cells and rows whose value is null after scrolling far and back register no tooltip, so they block nothing.")]
        public void VerifyRecycledThenNullValuesRegisterNoToolTip()
        {
            TableView tableView = null;
            var items = MakeManyItems(400);
            for (int i = 0; i < items.Count; i += 3)
            {
                items[i].Role = null;
            }

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateToolTipTable(items, cellToolTipPath: "Role");
                tableView.RowToolTipBinding = new Binding { Path = new PropertyPath("Role"), Mode = BindingMode.OneWay };
                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            foreach (var offset in new[] { 6000.0, 12000.0, 0.0 })
            {
                ScrollBodyTo(tableView, ScrollAxis.Vertical, offset);

                RunOnUIThread.Execute(() =>
                {
                    int nullRows = 0;
                    foreach (var row in GetRealizedRows(tableView))
                    {
                        if (!(row.DataContext is Person item) || row.ActualHeight <= 0)
                        {
                            continue;
                        }

                        var cell = GetRowCell(row, 0);
                        if (item.Role == null)
                        {
                            nullRows++;
                            Verify.IsNull(ToolTipService.GetToolTip(cell), $"The cell for {item.Name} must register no tooltip at offset {offset}.");
                            Verify.IsNull(ToolTipService.GetToolTip(row), $"The row for {item.Name} must register no tooltip at offset {offset}.");
                        }
                        else
                        {
                            Verify.IsTrue(RequireToolTip(cell, $"the cell for {item.Name}").IsEnabled, $"The cell for {item.Name} must keep a live tooltip.");
                            Verify.IsTrue(RequireToolTip(row, $"the row for {item.Name}").IsEnabled, $"The row for {item.Name} must keep a live tooltip.");
                        }
                    }

                    Verify.IsGreaterThan(nullRows, 0, $"Some realized rows must have a null value at offset {offset}.");
                });
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a recycled row shows the new item's row tooltip, never the previous item's.")]
        public void VerifyRecycledRowShowsTheNewItemsRowToolTip()
        {
            TableView tableView = null;
            var items = MakeManyItems(200);

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateRowToolTipTable(items, rowToolTipPath: "Role");
                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            ScrollBodyTo(tableView, ScrollAxis.Vertical, 2000.0);

            RunOnUIThread.Execute(() =>
            {
                var rows = GetRealizedRows(tableView);
                Verify.IsGreaterThan(rows.Count, 0, "Rows must be realized after the scroll.");

                foreach (var row in rows)
                {
                    var item = row.DataContext as Person;
                    if (item == null)
                    {
                        Verify.Fail("Every realized row must carry its item as its DataContext.");
                        continue;
                    }

                    Verify.AreEqual(item.Role, RequireToolTipContent(row, $"the row for {item.Name}"),
                        "A recycled row's tooltip must re-resolve to its current item.");
                }
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies recycled rows keep their row tooltip and help text when the bound value does not change between items.")]
        public void VerifyRecycledRowKeepsRowToolTipWhenItemsShareTheValue()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateRowToolTipTable(MakeManyItems(200), rowToolTipPath: null);
                // A constant across the DataContext reset raises no callback to revive the released tooltip.
                tableView.RowToolTipBinding = new Binding { Source = "Shared row", Mode = BindingMode.OneWay };
                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            ScrollBodyTo(tableView, ScrollAxis.Vertical, 2000.0);
            ScrollBodyTo(tableView, ScrollAxis.Vertical, 4000.0);

            RunOnUIThread.Execute(() =>
            {
                var rows = GetRealizedRows(tableView);
                Verify.IsGreaterThan(rows.Count, 0, "Rows must be realized after the scroll.");

                foreach (var row in rows)
                {
                    var toolTip = RequireToolTip(row, $"the row for {(row.DataContext as Person)?.Name}");
                    Verify.AreEqual("Shared row", toolTip.Content as string, "A recycled row must keep its tooltip content.");
                    Verify.IsTrue(toolTip.IsEnabled, "A recycled row's tooltip must be enabled.");
                    Verify.AreEqual("Shared row", AutomationProperties.GetHelpText(row),
                        "A recycled row must re-publish the tooltip as help text.");
                }
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies the row tooltip stays withdrawn for the whole edit, through a bound value change and a new RowToolTipBinding, and returns when the edit commits.")]
        public void VerifyRowToolTipIsSuppressedForTheWholeEdit()
        {
            TableView tableView = null;
            var items = MakeEditableItems("Asha", "Diego", "Mei");
            bool veto = true;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateEditableTable(items);
                tableView.IsReadOnly = false;
                tableView.RowToolTipBinding = new Binding { Path = new PropertyPath("Role"), Mode = BindingMode.OneWay };
                tableView.CellEditEnding += (s, e) => e.Cancel = veto;
                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual("Designer", RequireToolTipContent(RequireFirstRow(tableView), "row 0"),
                    "Precondition: the row starts with its tooltip.");

                // A vetoed commit holds the edit open.
                CaptureSetValue(GetValueProvider(tableView, items[0], columnIndex: 0), "Asha Kapoor");
                Verify.IsTrue(tableView.IsEditing, "Precondition: the vetoed commit keeps the edit open.");
                VerifyRowToolTipWithdrawn(RequireFirstRow(tableView), "once the edit opens");

                items[0].Role = "Principal Designer";
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                VerifyRowToolTipWithdrawn(RequireFirstRow(tableView), "after the bound value changes mid-edit");
                tableView.RowToolTipBinding = new Binding { Path = new PropertyPath("Name"), Mode = BindingMode.OneWay };
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                VerifyRowToolTipWithdrawn(RequireFirstRow(tableView), "after RowToolTipBinding is replaced mid-edit");

                veto = false;
                Verify.IsTrue(tableView.CommitEdit(), "The commit must now close the edit.");
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var row = RequireFirstRow(tableView);
                var toolTip = RequireToolTip(row, "row 0");
                Verify.AreEqual("Asha Kapoor", toolTip.Content as string, "After the edit the row tooltip must carry the current binding's value.");
                Verify.IsTrue(toolTip.IsEnabled, "The restored row tooltip must be able to open.");
                Verify.AreEqual("Asha Kapoor", AutomationProperties.GetHelpText(row), "The help text must be published again.");
            });
        }

        // Withdrawn, not merely disabled: a registered ToolTip would still claim focus and Escape.
        private static void VerifyRowToolTipWithdrawn(TableViewRow row, string when)
        {
            Verify.IsNull(ToolTipService.GetToolTip(row), $"The row must carry no tooltip {when}.");
            Verify.AreEqual(string.Empty, AutomationProperties.GetHelpText(row), $"The row must publish no help text {when}.");
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a cell whose tooltip value becomes empty after recycling is left with no registered tooltip, so the row tooltip covers it.")]
        public void VerifyClearedCellToolTipIsDetachedSoTheRowToolTipCoversTheCell()
        {
            TableView tableView = null;
            var items = MakeManyItems(200);
            for (int i = 0; i < items.Count; i += 2)
            {
                items[i].Role = string.Empty;
            }

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateToolTipTable(items, cellToolTipPath: "Role");
                tableView.RowToolTipBinding = new Binding { Path = new PropertyPath("Name"), Mode = BindingMode.OneWay };
                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            ScrollBodyTo(tableView, ScrollAxis.Vertical, 2000.0);
            ScrollBodyTo(tableView, ScrollAxis.Vertical, 4000.0);

            RunOnUIThread.Execute(() =>
            {
                int emptyRows = 0;
                foreach (var row in GetRealizedRows(tableView))
                {
                    if (!(row.DataContext is Person item))
                    {
                        continue;
                    }

                    var cell = GetRowCell(row, 0);
                    if (string.IsNullOrEmpty(item.Role))
                    {
                        emptyRows++;
                        // ToolTipService prefers the innermost registered owner, so a disabled-but-registered
                        // cell tooltip would block the row's on hover and focus.
                        Verify.IsNull(ToolTipService.GetToolTip(cell), $"The cell for {item.Name} must not keep a registered tooltip.");
                    }
                    else
                    {
                        Verify.AreEqual(item.Role, RequireToolTipContent(cell, $"the cell for {item.Name}"));
                    }

                    Verify.AreEqual(item.Name, RequireToolTipContent(row, $"the row for {item.Name}"));
                }

                Verify.IsGreaterThan(emptyRows, 0, "Some realized rows must have an empty cell tooltip value.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a row tooltip and a cell tooltip coexist on their own elements without overwriting each other.")]
        public void VerifyRowToolTipCoexistsWithCellToolTip()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateToolTipTable(MakeItems(), cellToolTipPath: "Name");
                tableView.RowToolTipBinding = new Binding { Path = new PropertyPath("Role"), Mode = BindingMode.OneWay };
                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var row = RequireFirstRow(tableView);

                // ToolTipService shows one automatic tooltip at a time, preferring the innermost owner,
                // so a cell's own tooltip wins under the pointer and the row's covers the rest.
                Verify.AreEqual("Designer", RequireToolTipContent(row, "row 0"), "The row must carry the row tooltip.");
                Verify.AreEqual("Asha", RequireToolTipContent(GetRowCell(row, 0), "row 0's cell"), "The cell must keep its own tooltip.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a string row tooltip is published as help text on the row and reported by the row peer.")]
        public void VerifyRowToolTipIsReportedAsRowHelpText()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                // The row shows Name only, so the Role tooltip adds information the row name lacks.
                tableView = CreateRowToolTipTable(MakeItems(), rowToolTipPath: "Role");
                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var row = RequireFirstRow(tableView);
                Verify.AreEqual("Designer", AutomationProperties.GetHelpText(row),
                    "String row tooltip text must be published as the row's AutomationProperties.HelpText.");
                Verify.AreEqual("Designer", GetRowPeer(row).GetHelpText(),
                    "The row peer must report it, or a screen reader never hears the row tooltip.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies the row peer withholds row tooltip help text that repeats the row name or a visible cell, but keeps text that does not.")]
        public void VerifyRowPeerSuppressesHelpTextAlreadyInTheRowName()
        {
            TableView repeatsCell = null;
            TableView adds = null;

            RunOnUIThread.Execute(() =>
            {
                // The common shape: tooltip the column that truncates, which the row name already reads in full.
                repeatsCell = CreateTableViewWithColumns(MakeItems(), height: 150,
                    columns: new[] { ("Name", new GridLength(120.0, GridUnitType.Pixel)) });
                repeatsCell.Columns.Add(MakeTextColumn("Role", "Role", new GridLength(120.0, GridUnitType.Pixel)));
                repeatsCell.RowToolTipBinding = new Binding { Path = new PropertyPath("Name"), Mode = BindingMode.OneWay };

                adds = CreateRowToolTipTable(MakeItems(), rowToolTipPath: "Role");
                adds.Height = 150;

                LoadContent(new StackPanel { Children = { repeatsCell, adds } });
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var row = RequireFirstRow(repeatsCell);
                Verify.AreEqual("Asha", AutomationProperties.GetHelpText(row),
                    "The published help text stays on the element; suppression is a query-time decision.");

                var duplicate = GetRowPeer(row).GetHelpText();
                Verify.IsTrue(string.IsNullOrEmpty(duplicate),
                    $"Help text repeating a visible cell is already in the row name and must be withheld; saw '{duplicate}'.");

                Verify.AreEqual("Designer", GetRowPeer(RequireFirstRow(adds)).GetHelpText(),
                    "Help text the row name does not contain must still be reported.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies a style-set row HelpText is never overwritten or cleared by the row tooltip, before or after recycling.")]
        public void VerifyStyleSetRowHelpTextSurvivesRowToolTipAndRecycling()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateRowToolTipTable(MakeManyItems(200), rowToolTipPath: "Role");
                tableView.Resources[typeof(TableViewRow)] = XamlReader.Load(
                    @"<Style xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                             xmlns:tabular='using:Microsoft.UI.Xaml.Controls.Tabular'
                             TargetType='tabular:TableViewRow'>
                          <Setter Property='AutomationProperties.HelpText' Value='X' />
                      </Style>");
                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() => VerifyStyleHelpText(tableView, "before recycling"));

            ScrollBodyTo(tableView, ScrollAxis.Vertical, 3000.0);
            ScrollBodyTo(tableView, ScrollAxis.Vertical, 0.0);

            RunOnUIThread.Execute(() => VerifyStyleHelpText(tableView, "after recycling"));
        }

        private static void VerifyStyleHelpText(TableView tableView, string when)
        {
            foreach (var row in GetRealizedRows(tableView))
            {
                if (!(row.DataContext is Person item))
                {
                    continue;
                }

                Verify.AreEqual(item.Role, RequireToolTipContent(row, $"the row for {item.Name}"), $"The row tooltip must apply {when}.");
                Verify.AreEqual("X", AutomationProperties.GetHelpText(row), $"The style-set HelpText must survive {when}.");
                Verify.AreEqual("X", GetRowPeer(row).GetHelpText(), $"The peer must report the style-set HelpText {when}.");
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies cell and row tooltips with the same value on every item survive scrolling far and back.")]
        public void VerifyIdenticalCellAndRowToolTipsSurviveScrollingFarAndBack()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateRowToolTipTable(MakeManyItems(400), rowToolTipPath: null);
                tableView.RowToolTipBinding = new Binding { Source = "Same row", Mode = BindingMode.OneWay };
                tableView.Columns[0].CellToolTipBinding = new Binding { Source = "Same cell", Mode = BindingMode.OneWay };
                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            foreach (var offset in new[] { 6000.0, 12000.0, 0.0 })
            {
                ScrollBodyTo(tableView, ScrollAxis.Vertical, offset);

                RunOnUIThread.Execute(() =>
                {
                    var rows = GetRealizedRows(tableView).Where(r => r.DataContext is Person).ToList();
                    Verify.IsGreaterThan(rows.Count, 0, "Rows must be realized.");
                    foreach (var row in rows)
                    {
                        var name = ((Person)row.DataContext).Name;
                        var rowTip = RequireToolTip(row, $"the row for {name}");
                        Verify.IsTrue(rowTip.IsEnabled && (rowTip.Content as string) == "Same row", $"The row for {name} must keep its tooltip at offset {offset}.");
                        var cellTip = RequireToolTip(GetRowCell(row, 0), $"the cell for {name}");
                        Verify.IsTrue(cellTip.IsEnabled && (cellTip.Content as string) == "Same cell", $"The cell for {name} must keep its tooltip at offset {offset}.");
                    }
                });
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies help text the app set on a row is never dropped by the row peer.")]
        public void VerifyRowPeerKeepsAppAuthoredHelpText()
        {
            TableView tableView = null;

            RunOnUIThread.Execute(() =>
            {
                tableView = CreateRowToolTipTable(MakeItems(), rowToolTipPath: null);
                LoadContent(tableView);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var row = RequireFirstRow(tableView);
                // Set before the binding applies, and equal to both the Name cell and the row tooltip, so
                // only the ownership gate keeps the suppression rule from dropping it.
                AutomationProperties.SetHelpText(row, "Asha");
                tableView.RowToolTipBinding = new Binding { Path = new PropertyPath("Name"), Mode = BindingMode.OneWay };
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var row = RequireFirstRow(tableView);
                Verify.AreEqual("Asha", RequireToolTipContent(row, "row 0"), "Precondition: the row tooltip applied.");
                Verify.AreEqual("Asha", GetRowPeer(row).GetHelpText(),
                    "Suppression must be gated on the control's ownership record.");
            });
        }

        #endregion
    }

    internal static class TableViewToolTipTestHelpers
    {
        // One text column showing Name, with the rows optionally tooltipped from another path.
        internal static TableView CreateRowToolTipTable(object items, string rowToolTipPath)
        {
            var tableView = CreateTableViewWithColumns(items, height: 300);
            if (rowToolTipPath != null)
            {
                tableView.RowToolTipBinding = new Binding { Path = new PropertyPath(rowToolTipPath), Mode = BindingMode.OneWay };
            }

            return tableView;
        }

        // One text column showing Name, optionally tooltipped from another path. cellToolTipPath null
        // means the column opts out entirely, which is its own tested behaviour.
        internal static TableView CreateToolTipTable(
            IList items,
            string cellToolTipPath,
            IValueConverter converter = null)
        {
            var tableView = CreateTableViewWithColumns(items, height: 300);
            var column = (TableViewTextColumn)tableView.Columns[0];

            if (cellToolTipPath != null)
            {
                column.CellToolTipBinding = new Binding
                {
                    Path = new PropertyPath(cellToolTipPath),
                    Mode = BindingMode.OneWay,
                    Converter = converter,
                };
            }

            return tableView;
        }

        internal static ToolTip RequireToolTip(FrameworkElement element, string what)
        {
            var toolTip = ToolTipService.GetToolTip(element) as ToolTip;
            if (toolTip == null)
            {
                Verify.Fail($"Expected a ToolTip on {what}.");
                throw new InvalidOperationException($"No ToolTip on {what}.");
            }

            return toolTip;
        }

        internal static string RequireToolTipContent(FrameworkElement element, string what)
        {
            return RequireToolTip(element, what).Content as string;
        }
    }

    // A fresh element per evaluation: one UIElement cannot be parented by two ToolTips, which is why the
    // spec prescribes a converter rather than a shared instance.
    internal sealed class BorderToolTipConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, string language)
        {
            return new Border { Child = new TextBlock { Text = value as string ?? string.Empty } };
        }

        public object ConvertBack(object value, Type targetType, object parameter, string language)
        {
            throw new NotImplementedException();
        }
    }

    // Produces the one shape a tooltip value may not take.
    internal sealed class ToolTipValuedConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, string language)
        {
            return new ToolTip { Content = value as string ?? string.Empty };
        }

        public object ConvertBack(object value, Type targetType, object parameter, string language)
        {
            throw new NotImplementedException();
        }
    }
}
