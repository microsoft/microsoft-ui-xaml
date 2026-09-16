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

using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewColumnTestHelpers;
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
    public class TableViewToolTipTests : ApiTestBase
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
                Content = tableView;
                Content.UpdateLayout();
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
                        var wrapper = RequireCellWrapper(row, i);

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
                Content = tableView;
                Content.UpdateLayout();
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
                    var wrapper = RequireCellWrapper(rows[rowIndex], 0);
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
                Content = tableView;
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var rows = GetRealizedRows(tableView);
                Verify.IsGreaterThan(rows.Count, 2, "All three rows must be realized.");

                Verify.AreEqual("Designer", RequireToolTipContent(RequireCellWrapper(rows[0], 0), "row 0"),
                    "A row with a real value must still get its tooltip.");

                // TableView.idl:212-213 - "null or empty means no tooltip". An empty popup is worse
                // than none: it opens over the content it is meant to explain.
                Verify.IsNull(ToolTipService.GetToolTip(RequireCellWrapper(rows[1], 0)),
                    "An empty-string tooltip value must attach no tooltip.");
                Verify.IsNull(ToolTipService.GetToolTip(RequireCellWrapper(rows[2], 0)),
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
                Content = tableView;
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var rows = GetRealizedRows(tableView);
                Verify.IsGreaterThan(rows.Count, 1, "Two rows are needed to show each gets its own element.");

                var first = RequireToolTip(RequireCellWrapper(rows[0], 0), "row 0").Content as Border;
                var second = RequireToolTip(RequireCellWrapper(rows[1], 0), "row 1").Content as Border;

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
                Content = tableView;
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var wrapper = RequireCellWrapper(RequireFirstRow(tableView), 0);

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
                EnsureTabularControlsResources();

                tableView = new TableView { ItemsSource = MakeItems(), Width = 500, Height = 300 };

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

                Content = tableView;
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var wrapper = RequireCellWrapper(RequireFirstRow(tableView), 0);

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
                Content = tableView;
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();

            // ScrollBodyToVerticalOffset does the double settle. A row sampled before the repeater
            // finishes re-assigning DataContext is parented but unbound, and reading it there has twice
            // produced a false product bug in this control.
            ScrollBodyToVerticalOffset(tableView, 2000.0);

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
                    Verify.AreEqual(item.Role, RequireToolTipContent(RequireCellWrapper(row, 0), $"the row for {item.Name}"),
                        "A recycled row's tooltip must re-resolve to its current item.");
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
                Content = tableView;
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                original = RequireToolTip(RequireCellWrapper(RequireFirstRow(tableView), 0), "row 0");
                Verify.AreEqual("Designer", original.Content as string, "Precondition: the tooltip starts at the bound value.");

                items[0].Role = "Principal Designer";
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var current = RequireToolTip(RequireCellWrapper(RequireFirstRow(tableView), 0), "row 0");

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

                Content = tableView;
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual("Designer", RequireToolTipContent(RequireCellWrapper(RequireFirstRow(tableView), 0), "row 0"),
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

                Verify.AreEqual("Designer", RequireToolTipContent(RequireCellWrapper(RequireFirstRow(tableView), 0), "row 0"),
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
                Content = tableView;
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var row = RequireFirstRow(tableView);
                var wrapper = RequireCellWrapper(row, 0);

                // Element and peer are asserted separately on purpose: the element is where the control
                // publishes, the peer is what a client actually reads, and 14.3 shows the two are not
                // always the same.
                Verify.AreEqual("Designer", AutomationProperties.GetHelpText(wrapper),
                    "String tooltip text must be published as the cell's AutomationProperties.HelpText.");

                Verify.AreEqual("Designer", RequireCellPeer(row, 0).GetHelpText(),
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
                Content = tableView;
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var row = RequireFirstRow(tableView);
                var wrapper = RequireCellWrapper(row, 0);

                Verify.IsNotNull(ToolTipService.GetToolTip(wrapper) as ToolTip,
                    "Precondition: the rich tooltip is attached, so an empty help text is a decision and not an accident.");

                // Announcing "Microsoft.UI.Xaml.Controls.Border" is worse than silence: meaningless and
                // confidently spoken.
                Verify.AreEqual(string.Empty, AutomationProperties.GetHelpText(wrapper),
                    "Non-string tooltip content must not be stringified into help text.");

                var peerHelpText = RequireCellPeer(row, 0).GetHelpText();
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
                EnsureTabularControlsResources();

                tableView = new TableView { ItemsSource = MakeItems(), Width = 600, Height = 300 };

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

                Content = tableView;
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var row = RequireFirstRow(tableView);

                // Suppression happens at UIA query time, not at attach: the element keeps the text and
                // only the peer withholds it.
                Verify.AreEqual("Asha", AutomationProperties.GetHelpText(RequireCellWrapper(row, 0)),
                    "The published help text stays on the element; suppression is a query-time decision.");

                var duplicate = RequireCellPeer(row, 0).GetHelpText();
                Verify.IsTrue(string.IsNullOrEmpty(duplicate),
                    $"Help text that merely repeats the cell value must be withheld, or the value is announced twice; saw '{duplicate}'.");

                Verify.AreEqual("Asha", RequireCellPeer(row, 1).GetHelpText(),
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
                Content = tableView;
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var row = RequireFirstRow(tableView);
                var wrapper = RequireCellWrapper(row, 0);

                // Set to exactly the cell's own value, so the suppression rule would fire if it were not
                // gated on the ownership record. Setting it on the wrapper is the only route an app has:
                // the peer reads help text from its owner, which is the wrapper, not the cell content.
                AutomationProperties.SetHelpText(wrapper, "Asha");

                Verify.AreEqual("Asha", RequireCellPeer(row, 0).GetHelpText(),
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
                tableView = CreateTableView("Name", "Role");
                tableView.Columns[0].HeaderToolTip = "The person's display name";
                Content = tableView;
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var headerCell = RequireHeaderCell(tableView, 0);

                // On the cell, not its ContentPresenter: the spec has it "cover the whole header cell,
                // including its padding and sort affordance", which is most of the header's hit area.
                Verify.AreEqual("The person's display name", RequireToolTipContent(headerCell, "header cell 0"),
                    "A string HeaderToolTip must attach a tooltip to the header cell itself.");

                Verify.IsNull(ToolTipService.GetToolTip(GetHeaderPresenter(tableView, 0)),
                    "The tooltip must not sit on the inner presenter, which covers only the header content.");

                Verify.IsNull(ToolTipService.GetToolTip(RequireHeaderCell(tableView, 1)),
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
                tableView = CreateTableView("Name");
                content = new Border { Child = new TextBlock { Text = "Rich header tooltip" } };
                tableView.Columns[0].HeaderToolTip = content;
                Content = tableView;
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                // HeaderToolTip is typed Object (TableView.idl:119) precisely so this works.
                var hosted = RequireToolTip(RequireHeaderCell(tableView, 0), "header cell 0").Content;
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
                tableView = CreateTableView("Name", "Role", "Team");
                // Column 0 unset, column 1 empty, column 2 real - so a blanket "never attaches"
                // implementation cannot pass.
                tableView.Columns[1].HeaderToolTip = string.Empty;
                tableView.Columns[2].HeaderToolTip = "Owning team";
                Content = tableView;
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsNull(ToolTipService.GetToolTip(RequireHeaderCell(tableView, 0)),
                    "An unset HeaderToolTip must attach nothing.");
                Verify.IsNull(ToolTipService.GetToolTip(RequireHeaderCell(tableView, 1)),
                    "An empty-string HeaderToolTip must attach nothing - TableView.idl:116-117 makes empty equivalent to null.");
                Verify.AreEqual("Owning team", RequireToolTipContent(RequireHeaderCell(tableView, 2), "header cell 2"),
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
                tableView = CreateTableView("Name");
                Content = tableView;
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsNull(ToolTipService.GetToolTip(RequireHeaderCell(tableView, 0)),
                    "Precondition: no tooltip before one is set.");

                tableView.Columns[0].HeaderToolTip = "First";
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual("First", RequireToolTipContent(RequireHeaderCell(tableView, 0), "header cell 0"),
                    "Setting HeaderToolTip after load must reach the realized header.");

                tableView.Columns[0].HeaderToolTip = "Second";
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual("Second", RequireToolTipContent(RequireHeaderCell(tableView, 0), "header cell 0"),
                    "Replacing the value must replace the content, not leave the first one in place.");

                tableView.Columns[0].HeaderToolTip = null;
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                // Header cells are rebuilt rather than recycled, so there is no binding here - only the
                // property-changed path, which is what this whole test exercises.
                var remaining = ToolTipService.GetToolTip(RequireHeaderCell(tableView, 0));
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
                tableView = CreateTableView("Name");
                tableView.Columns[0].HeaderToolTip = "The person's display name";
                Content = tableView;
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                // Deliberately the opposite of the cell case in 14.2. The header peer is virtual, so
                // help text on the element would never reach a client - publishing it anyway would look
                // like working accessibility to anyone inspecting the tree.
                Verify.AreEqual(string.Empty, AutomationProperties.GetHelpText(RequireHeaderCell(tableView, 0)),
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
                tableView = CreateTableView("Name");
                tableView.CanUserSortColumns = true;
                tableView.Columns[0].CanSort = true;
                tableView.Columns[0].SortMemberPath = "Name";
                tableView.Columns[0].HeaderToolTip = "The person's display name";
                Content = tableView;
                Content.UpdateLayout();
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
                tableView = CreateTableView("Name", "Role");
                tableView.Columns[0].HeaderToolTip = "Name";          // identical to the header
                tableView.Columns[1].HeaderToolTip = "What they do";   // distinct from it
                Content = tableView;
                Content.UpdateLayout();
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
    }

    internal static class TableViewToolTipTestHelpers
    {
        // One text column showing Name, optionally tooltipped from another path. cellToolTipPath null
        // means the column opts out entirely, which is its own tested behaviour.
        internal static TableView CreateToolTipTable(
            IList items,
            string cellToolTipPath,
            IValueConverter converter = null)
        {
            EnsureTabularControlsResources();

            var tableView = new TableView
            {
                ItemsSource = items,
                Width = 500,
                Height = 300,
            };

            var column = new TableViewTextColumn
            {
                Header = "Name",
                Width = new GridLength(200.0, GridUnitType.Pixel),
                Binding = new Binding { Path = new PropertyPath("Name"), Mode = BindingMode.OneWay },
            };

            if (cellToolTipPath != null)
            {
                column.CellToolTipBinding = new Binding
                {
                    Path = new PropertyPath(cellToolTipPath),
                    Mode = BindingMode.OneWay,
                    Converter = converter,
                };
            }

            tableView.Columns.Add(column);
            return tableView;
        }

        // Header cells are the PART_HeaderHost children; the control tags each with its column, which is
        // the supported way to map a rendered element back to a column.
        internal static FrameworkElement RequireHeaderCell(TableView tableView, int index)
        {
            var host = tableView.FindVisualChildByName("PART_HeaderHost") as Panel;
            Verify.IsNotNull(host, "PART_HeaderHost should exist once the template has applied.");
            Verify.IsGreaterThan(host.Children.Count, index, "The header host should have a cell at the requested index.");

            var cell = host.Children[index] as FrameworkElement;
            Verify.IsNotNull(cell, $"Header cell {index} should be a FrameworkElement.");
            return cell;
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

        internal static AutomationPeer RequireCellPeer(TableViewRow row, int columnIndex)
        {
            var rowPeer = FrameworkElementAutomationPeer.CreatePeerForElement(row) as TableViewRowAutomationPeer;
            Verify.IsNotNull(rowPeer, "A TableViewRow must produce a TableViewRowAutomationPeer.");

            var children = rowPeer.GetChildren();
            Verify.IsTrue(children != null && children.Count > columnIndex,
                $"The row peer must expose a cell peer for column {columnIndex}; saw {children?.Count ?? 0}.");

            var cellPeer = children[columnIndex] as TableViewCellAutomationPeer;
            Verify.IsNotNull(cellPeer, $"Child {columnIndex} of the row peer must be a TableViewCellAutomationPeer.");
            return cellPeer;
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
