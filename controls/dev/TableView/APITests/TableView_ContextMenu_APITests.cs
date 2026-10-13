// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Automation.Peers;
using Microsoft.UI.Xaml.Automation.Provider;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
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
using System.Windows.Input;

using WEX.TestExecution;
using WEX.TestExecution.Markup;

using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.ContextMenuTestHelpers;
using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewAutomationTestHelpers;
using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewRowTestHelpers;
using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewTestHelpers;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    // Context menus: TableView.RowContextFlyout, TableViewColumn.CellContextFlyout and
    // HeaderContextFlyout, the native TableView.ContextFlyout fallback, and ContextFlyouts set in cell
    // or header content and on rows (implicit style or template).
    //
    // Test IDs match controls\dev\TableView\docs\context-menu-test-design.md. Every request is raised
    // through an automation peer's ShowContextMenu (no position, like a keyboard request). Real mouse
    // and keyboard input, the chevron, and the current cell (internal; observable only through
    // keyboard behavior) are covered by the interaction tests.
    //
    // Red tests encode a known product gap and are expected to fail until it is fixed:
    // A15, A16, C6, E4, E6, E8, G5.
    [TestClass]
    public class TableViewContextMenuTests : TableViewApiTestBase
    {
        // ---- A. Body precedence -------------------------------------------------------------

        [TestMethod]
        [TestProperty("Description", "A1: a column's CellContextFlyout wins over RowContextFlyout and anchors to the cell.")]
        public void CellMenuWinsOverRowMenu()
        {
            var f = LoadFixture();

            RequestOnCell(f, 3, NameCol);

            VerifyOpened(f, "CellMenu", () => CellOf(f, 3, NameCol), f.P(3), "UIA on row 3 / Name");
            CloseMenus(f);
        }

        [TestMethod]
        [TestProperty("Description", "A2: a cell without a CellContextFlyout         falls back to RowContextFlyout, anchored to the requested cell.")]
        public void RowMenuUsedWhenColumnHasNoCellMenu()
        {
            var f = LoadFixture();

            RequestOnCell(f, 3, OfficeCol);

                    VerifyOpened(f, "RowMenu", () => CellOf(f, 3, OfficeCol), f.P(3), "UIA on row 3 / Office");
            CloseMenus(f);
        }

        [TestMethod]
        [TestProperty("Description", "A3: with no cell or row menu the native TableView.ContextFlyout opens.")]
        public void TableMenuUsedWhenNoCellOrRowMenu()
        {
            var f = LoadFixture(configure: x => x.Table.RowContextFlyout = null);

            RequestOnCell(f, 3, OfficeCol);

            VerifyOpened(f, "TableMenu", () => f.Table, expectedItem: null, "UIA on row 3 / Office, no RowContextFlyout");
            CloseMenus(f);
        }

        [TestMethod]
        [TestProperty("Description", "A4: with no TableView candidate the request is left unhandled and an ancestor's ContextFlyout opens.")]
        public void UnhandledWhenNoCandidateBubblesToAncestor()
        {
            var f = LoadFixture(configure: x =>
            {
                ClearTableMenus(x);
                x.Root.ContextFlyout = x.OuterMenu;
            });

            RequestOnCell(f, 3, NameCol);

            VerifyOpened(f, "OuterMenu", () => f.Root, expectedItem: null, "UIA on row 3 / Name, no TableView menus");
            CloseMenus(f);
        }

        [TestMethod]
        [TestProperty("Description", "A5: CellContextFlyout is read per request; a new value applies and null falls back to the row menu.")]
        public void CellMenuChangesApplyOnNextRequest()
        {
            var f = LoadFixture();
            MenuFlyout cellMenu2 = null;
            RunOnUIThread.Execute(() => cellMenu2 = f.Recorder.Create("CellMenu2"));

            RequestOnCell(f, 3, NameCol);
            VerifyOpened(f, "CellMenu", () => CellOf(f, 3, NameCol), f.P(3), "initial CellMenu");
            CloseMenus(f);

            RunOnUIThread.Execute(() => f.NameColumn.CellContextFlyout = cellMenu2);
            RequestOnCell(f, 3, NameCol);
            VerifyOpened(f, "CellMenu2", () => CellOf(f, 3, NameCol), f.P(3), "after replacing CellContextFlyout");
            CloseMenus(f);

            RunOnUIThread.Execute(() => f.NameColumn.CellContextFlyout = null);
            RequestOnCell(f, 3, NameCol);
            VerifyOpened(f, "RowMenu", () => CellOf(f, 3, NameCol), f.P(3), "after clearing CellContextFlyout");
            CloseMenus(f);
        }

        [TestMethod]
        [TestProperty("Description", "A7: a ContextFlyout on the cell template root wins over the column's CellContextFlyout.")]
        public void CellTemplateFlyoutWinsOverCellMenu()
        {
            var f = LoadFixture(
                departmentTemplate: ContentTemplate(RootFlyoutContent),
                configure: x => x.DepartmentColumn.CellContextFlyout = x.CellMenu);
            var content = TrackContentMenu(f, 3, DepartmentCol, ContentRootName, "ContentMenu");

            RequestOnCell(f, 3, DepartmentCol);

            VerifyOpened(f, "ContentMenu", () => content, f.P(3), "UIA on row 3 / Department");
            CloseMenus(f);
        }

        [TestMethod]
        [TestProperty("Description", "A9: a ContextFlyout on a nested (non-root) element in the cell template is found.")]
        public void CellTemplateInnerFlyoutFound()
        {
            var f = LoadFixture(
                departmentTemplate: ContentTemplate(
                    "<Grid>" +
                    "  <Border x:Name='ContentRoot' Background='Transparent'>" +
                    "    <Border.ContextFlyout><MenuFlyout/></Border.ContextFlyout>" +
                    "    <TextBlock Text='{Binding Role}'/>" +
                    "  </Border>" +
                    "</Grid>"),
                configure: x => x.DepartmentColumn.CellContextFlyout = x.CellMenu);
            var content = TrackContentMenu(f, 3, DepartmentCol, ContentRootName, "ContentMenu");

            RequestOnCell(f, 3, DepartmentCol);

            VerifyOpened(f, "ContentMenu", () => content, f.P(3), "UIA on row 3 / Department");
            CloseMenus(f);
        }

        [TestMethod]
        [TestProperty("Description", "A10: with nested content flyouts, UIA/keyboard requests pick the outermost one.")]
        public void OutermostContentFlyoutWins()
        {
            var f = LoadFixture(departmentTemplate: ContentTemplate(
                "<Grid x:Name='ContentRoot' Background='Transparent'>" +
                "  <Grid.ContextFlyout><MenuFlyout/></Grid.ContextFlyout>" +
                "  <Border x:Name='ContentInner' Background='Transparent'>" +
                "    <Border.ContextFlyout><MenuFlyout/></Border.ContextFlyout>" +
                "    <TextBlock Text='{Binding Role}'/>" +
                "  </Border>" +
                "</Grid>"));
            var outer = TrackContentMenu(f, 3, DepartmentCol, ContentRootName, "ContentMenu");
            TrackContentMenu(f, 3, DepartmentCol, "ContentInner", "ContentMenu2");

            RequestOnCell(f, 3, DepartmentCol);

            VerifyOpened(f, "ContentMenu", () => outer, f.P(3), "UIA on row 3 / Department");
            CloseMenus(f);
        }

        [TestMethod]
        [TestProperty("Description", "A11: a ContextFlyout on collapsed content is not a candidate.")]
        public void CollapsedContentFlyoutIgnored()
        {
            var f = LoadFixture(
                departmentTemplate: ContentTemplate(
                    "<Grid>" +
                    "  <Border x:Name='ContentRoot' Visibility='Collapsed'>" +
                    "    <Border.ContextFlyout><MenuFlyout/></Border.ContextFlyout>" +
                    "  </Border>" +
                    "  <TextBlock Text='{Binding Role}'/>" +
                    "</Grid>"),
                configure: x => x.DepartmentColumn.CellContextFlyout = x.CellMenu);
            TrackContentMenu(f, 3, DepartmentCol, ContentRootName, "ContentMenu");

            RequestOnCell(f, 3, DepartmentCol);

            VerifyOpened(f, "CellMenu", () => CellOf(f, 3, DepartmentCol), f.P(3), "UIA on row 3 / Department");
            CloseMenus(f);
        }

        [TestMethod]
        [TestProperty("Description", "A12: a text control's built-in copy/select flyout is not an app menu; the cell menu still opens.")]
        public void TextControlDefaultFlyoutIgnored()
        {
            var cases = new[]
            {
                ("TextBox", "<TextBox Text='{Binding Name}' IsReadOnly='True'/>"),
                ("selectable TextBlock", "<TextBlock Text='{Binding Name}' IsTextSelectionEnabled='True'/>"),
            };

            foreach (var (label, xaml) in cases)
            {
                var f = LoadFixture(nameTemplate: ContentTemplate(xaml));

                RequestOnCell(f, 3, NameCol);

                VerifyOpened(f, "CellMenu", () => CellOf(f, 3, NameCol), f.P(3), $"Name cell is a {label}");
                CloseMenus(f);
            }
        }

        [TestMethod]
        [TestProperty("Description", "A13: a ContextFlyout set locally on a text control is an app menu and wins.")]
        public void TextControlLocalFlyoutCounts()
        {
            var f = LoadFixture(nameTemplate: ContentTemplate(
                "<TextBox x:Name='ContentRoot' Text='{Binding Name}' IsReadOnly='True'>" +
                "  <TextBox.ContextFlyout><MenuFlyout/></TextBox.ContextFlyout>" +
                "</TextBox>"));
            var content = TrackContentMenu(f, 3, NameCol, ContentRootName, "ContentMenu");

            RequestOnCell(f, 3, NameCol);

            VerifyOpened(f, "ContentMenu", () => content, f.P(3), "UIA on row 3 / Name (TextBox)");
            CloseMenus(f);
        }

        [TestMethod]
        [TestProperty("Description", "A14: the content search stops at a nested TableView.")]
        public void NestedTableViewFlyoutsNotPickedByOuter()
        {
            var f = LoadFixture(
                departmentTemplate: ContentTemplate(
                    "<tv:TableView x:Name='ContentRoot' Height='40' Width='120'>" +
                    "  <tv:TableView.ContextFlyout><MenuFlyout/></tv:TableView.ContextFlyout>" +
                    "</tv:TableView>"),
                configure: x => x.DepartmentColumn.CellContextFlyout = x.CellMenu);
            TrackContentMenu(f, 3, DepartmentCol, ContentRootName, "InnerTableMenu");

            RequestOnCell(f, 3, DepartmentCol);

            VerifyOpened(f, "CellMenu", () => CellOf(f, 3, DepartmentCol), f.P(3), "UIA on row 3 / Department");
            CloseMenus(f);
        }

        [TestMethod]
        [TestProperty("Description", "A15 (Red): the content search stops at a nested items control, like it does at a nested TableView.")]
        public void NestedItemsControlFlyoutNotPicked()
        {
            var f = LoadFixture(
                departmentTemplate: ContentTemplate(
                    "<ListView Height='40' Width='120'>" +
                    "  <ListViewItem x:Name='ContentRoot'>" +
                    "    <ListViewItem.ContextFlyout><MenuFlyout/></ListViewItem.ContextFlyout>" +
                    "    <TextBlock Text='item'/>" +
                    "  </ListViewItem>" +
                    "</ListView>"),
                configure: x => x.DepartmentColumn.CellContextFlyout = x.CellMenu);
            TrackContentMenu(f, 3, DepartmentCol, ContentRootName, "ContentMenu");

            RequestOnCell(f, 3, DepartmentCol);

            VerifyOpened(f, "CellMenu", () => CellOf(f, 3, DepartmentCol), f.P(3), "UIA on row 3 / Department (ListView content)");
            CloseMenus(f);
        }

        [TestMethod]
        [TestProperty("Description", "A16 (Red): a cell menu wins over a row ContextFlyout set by an implicit row style; cells without one get the row style menu.")]
        public void CellMenuWinsOverRowStyleMenu()
        {
            var f = LoadFixture(rowStyle: RowStyleWithFlyout());

            RequestOnCell(f, 3, NameCol);
            VerifyOpened(f, "CellMenu", () => CellOf(f, 3, NameCol), f.P(3), "UIA on row 3 / Name with a row style menu");
            CloseMenus(f);

            RequestOnCell(f, 3, OfficeCol);
            VerifyOpened(f, "RowStyleMenu", () => RowOf(f, 3), f.P(3), "UIA on row 3 / Office with a row style menu");
            CloseMenus(f);
        }

        [TestMethod]
        [TestProperty("Description", "A17: a cell content flyout wins over a row style menu.")]
        public void CellTemplateFlyoutWinsOverRowStyleMenu()
        {
            var f = LoadFixture(departmentTemplate: ContentTemplate(RootFlyoutContent), rowStyle: RowStyleWithFlyout());
            var content = TrackContentMenu(f, 3, DepartmentCol, ContentRootName, "ContentMenu");

            RequestOnCell(f, 3, DepartmentCol);

            VerifyOpened(f, "ContentMenu", () => content, f.P(3), "UIA on row 3 / Department");
            CloseMenus(f);
        }

        [TestMethod]
        [TestProperty("Description", "A18: an app-set row ContextFlyout (implicit style) wins over RowContextFlyout.")]
        public void RowStyleMenuWinsOverRowContextFlyout()
        {
            var f = LoadFixture(rowStyle: RowStyleWithFlyout());

            RequestOnCell(f, 3, OfficeCol);

            VerifyOpened(f, "RowStyleMenu", () => RowOf(f, 3), f.P(3), "UIA on row 3 / Office");
            CloseMenus(f);
        }

        // ---- B. Row targets -----------------------------------------------------------------

        [TestMethod]
        [TestProperty("Description", "B6: a row peer request targets the row even after a cell request made Name the current column.")]
        public void RowPeerShowContextMenuOpensRowMenu()
        {
            var f = LoadFixture();

            RequestOnCell(f, 3, NameCol);
            VerifyOpened(f, "CellMenu", () => CellOf(f, 3, NameCol), f.P(3), "precondition: Name is now the current column");
            CloseMenus(f);

            RequestOnRow(f, 3);

            VerifyOpened(f, "RowMenu", () => RowOf(f, 3), f.P(3), "UIA on row 3's peer");
            CloseMenus(f);
        }

        // ---- C. Column headers --------------------------------------------------------------

        [TestMethod]
        [TestProperty("Description", "C1: a header request opens the column's HeaderContextFlyout anchored to that header.")]
        public void HeaderMenuOpensOnHeader()
        {
            var f = LoadFixture();

            RequestOnHeader(f, NameCol);

            VerifyOpened(f, "HeaderMenu", () => HeaderOf(f, NameCol), expectedItem: null, "UIA on the Name header");
            CloseMenus(f);
        }

        [TestMethod]
        [TestProperty("Description", "C2: a header without a menu falls back to TableView.ContextFlyout, never to cell or row menus.")]
        public void HeaderWithoutMenuFallsBackToTableMenu()
        {
            var f = LoadFixture(configure: x => x.OfficeColumn.CellContextFlyout = x.CellMenu);

            RequestOnHeader(f, OfficeCol);

            VerifyOpened(f, "TableMenu", () => f.Table, expectedItem: null, "UIA on the Office header");
            CloseMenus(f);
        }

        [TestMethod]
        [TestProperty("Description", "C3: a ContextFlyout in the header template wins over HeaderContextFlyout.")]
        public void HeaderTemplateFlyoutWinsOverHeaderMenu()
        {
            var f = LoadFixture(configure: x => x.NameColumn.HeaderTemplate = ContentTemplate(
                "<Grid x:Name='ContentRoot' Background='Transparent'>" +
                "  <Grid.ContextFlyout><MenuFlyout/></Grid.ContextFlyout>" +
                "  <TextBlock Text='{Binding}'/>" +
                "</Grid>"));
            FrameworkElement content = null;
            RunOnUIThread.Execute(() => content = TrackNamedMenu(f, HeaderOf(f, NameCol), ContentRootName, "ContentMenu"));

            RequestOnHeader(f, NameCol);

            VerifyOpened(f, "ContentMenu", () => content, expectedItem: null, "UIA on the Name header");
            CloseMenus(f);
        }

        [TestMethod]
        [TestProperty("Description", "C4: a header request leaves body selection and focus unchanged.")]
        public void HeaderRequestDoesNotMoveBodyState()
        {
            var f = LoadFixture();
            SelectRow(f, 2);
            FocusRow(f, 2);

            RequestOnHeader(f, NameCol);
            VerifyOpened(f, "HeaderMenu", () => HeaderOf(f, NameCol), expectedItem: null, "UIA on the Name header");
            CloseMenus(f);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreSame(f.P(2), f.Table.SelectedItem, "A header menu must not change the selection.");
                VerifyFocusWithin(f, RowOf(f, 2), "after the header menu closes");
            });
        }

        [TestMethod]
        [TestProperty("Description", "C6 (Red): a header peer for a collapsed column must not open any menu.")]
        public void CollapsedColumnHeaderPeerOpensNoMenu()
        {
            var f = LoadFixture(configure: x => x.OfficeColumn.HeaderContextFlyout = x.HeaderMenu);
            SelectRow(f, 2);
            FocusRow(f, 2);

            AutomationPeer peer = null;
            RunOnUIThread.Execute(() => f.OfficeColumn.Visibility = Visibility.Collapsed);
            SettleLayout(f.Table);
            RunOnUIThread.Execute(() =>
            {
                peer = new TableViewColumnHeaderAutomationPeer(f.Table, f.OfficeColumn);
                peer.ShowContextMenu();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(0, f.Recorder.Opened.Count,
                    $"No menu may open for a collapsed column's header. Opened: {f.Recorder.Describe()}");
                Verify.AreSame(f.P(2), f.Table.SelectedItem, "Selection must be unchanged.");
            });
            CloseMenus(f);
            RunOnUIThread.Execute(() => VerifyFocusWithin(f, RowOf(f, 2), "after the request"));
        }

        // ---- D. Group headers ---------------------------------------------------------------

        [TestMethod]
        [TestProperty("Description", "D1: group headers never use cell or row menus; the table menu opens.")]
        public void GroupHeaderUsesTableMenu()
        {
            var f = LoadFixture(grouped: true);

            RunOnUIThread.Execute(() => GetGroupHeaderPeer(f.Table, 0).ShowContextMenu());
            IdleSynchronizer.Wait();

            VerifyOpened(f, "TableMenu", () => f.Table, expectedItem: null, "UIA on the first group header");
            CloseMenus(f);
        }

        [TestMethod]
        [TestProperty("Description", "D2: a group header ContextFlyout from an implicit style wins over TableView.ContextFlyout.")]
        public void GroupHeaderStyleMenuWins()
        {
            var f = LoadFixture(grouped: true, configure: x =>
            {
                var style = (Style)XamlReader.Load(
                    "<Style " + XamlNamespaces + " TargetType='tv:TableViewGroupHeader'>" +
                    "  <Setter Property='ContextFlyout'><Setter.Value><MenuFlyout/></Setter.Value></Setter>" +
                    "</Style>");
                x.Recorder.Track("GroupHeaderMenu", SetterFlyout(style));
                x.Table.Resources[typeof(TableViewGroupHeader)] = style;
            });

            RunOnUIThread.Execute(() => GetGroupHeaderPeer(f.Table, 0).ShowContextMenu());
            IdleSynchronizer.Wait();

            VerifyOpened(f, "GroupHeaderMenu", () => GetGroupHeader(f.Table, 0), expectedItem: null, "UIA on the first group header");
            CloseMenus(f);
        }

        // ---- E. Side effects ----------------------------------------------------------------

        [TestMethod]
        [TestProperty("Description", "E2: a body request does not change the selection.")]
        public void BodyRequestDoesNotChangeSelection()
        {
            var f = LoadFixture();
            SelectRow(f, 1);

            RequestOnCell(f, 5, NameCol);
            VerifyOpened(f, "CellMenu", () => CellOf(f, 5, NameCol), f.P(5), "UIA on row 5 / Name");

            RunOnUIThread.Execute(() => Verify.AreSame(f.P(1), f.Table.SelectedItem, "Opening a menu must not select the target row."));
            CloseMenus(f);
        }

        [TestMethod]
        [TestProperty("Description", "E3: a body request moves focus into the target row, and focus is still there after the menu closes.")]
        public void BodyRequestMovesFocusIntoTargetRow()
        {
            var f = LoadFixture();
            FocusRow(f, 1);

            RequestOnCell(f, 5, NameCol);
            VerifyOpened(f, "CellMenu", () => CellOf(f, 5, NameCol), f.P(5), "UIA on row 5 / Name");
            RunOnUIThread.Execute(() => VerifyWithin(f.Recorder.Opened[0].FocusAtOpening, RowOf(f, 5), "focus when the menu opens"));

            CloseMenus(f);
            RunOnUIThread.Execute(() => VerifyFocusWithin(f, RowOf(f, 5), "after the menu closes"));
        }

        [TestMethod]
        [TestProperty("Description", "E4 (Red): a cell request focuses the exact target cell; a row request focuses the row itself.")]
        public void BodyRequestFocusesTargetCell()
        {
            var f = LoadFixture();
            FocusRow(f, 1);

            RequestOnCell(f, 5, NameCol);
            VerifyOpened(f, "CellMenu", () => CellOf(f, 5, NameCol), f.P(5), "UIA on row 5 / Name");
            RunOnUIThread.Execute(() => Verify.AreSame(CellOf(f, 5, NameCol), f.Recorder.Opened[0].FocusAtOpening,
                $"Focus must be on the row 5 Name cell when the menu opens; was {Describe(f.Recorder.Opened[0].FocusAtOpening)}."));
            CloseMenus(f);

            RequestOnRow(f, 5);
            VerifyOpened(f, "RowMenu", () => RowOf(f, 5), f.P(5), "UIA on row 5's peer");
            RunOnUIThread.Execute(() => Verify.AreSame(RowOf(f, 5), f.Recorder.Opened[0].FocusAtOpening,
                $"Focus must be on row 5 itself for a row request; was {Describe(f.Recorder.Opened[0].FocusAtOpening)}."));
            CloseMenus(f);
        }

        // An API test cannot hold an edit open (SetValue opens and commits in one call), so the request
        // is raised from BeginningEdit, a window in which IsEditing is already true.
        [TestMethod]
        [TestProperty("Description", "E6 (Red): an edit on one row must not disable menus on another row.")]
        public void RequestOnOtherRowWhileEditingUsesThatRowsMenu()
        {
            bool? wasEditing = null;
            var f = LoadFixture(configure: x =>
            {
                x.Table.IsReadOnly = false;
                ((TableViewTextColumn)x.NameColumn).Binding = new Binding { Path = new PropertyPath("Name"), Mode = BindingMode.TwoWay };
            });

            RunOnUIThread.Execute(() =>
            {
                f.Table.BeginningEdit += (s, e) =>
                {
                    if (wasEditing.HasValue)
                    {
                        return;
                    }

                    wasEditing = f.Table.IsEditing;
                    GetCellPeer(f.Table, f.P(6), OfficeCol).ShowContextMenu();
                };

                var provider = GetCellPeer(f.Table, f.P(3), NameCol).GetPattern(PatternInterface.Value) as IValueProvider;
                Verify.IsNotNull(provider, "Precondition: the Name cell must be editable.");
                provider.SetValue("Edited");
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() => Verify.IsTrue(wasEditing == true, "Precondition: IsEditing must be true inside BeginningEdit."));
            VerifyOpened(f, "RowMenu", () => RowOf(f, 6), f.P(6), "UIA on row 6 / Office while row 3 / Name is editing");
            CloseMenus(f);
        }

        [TestMethod]
        [TestProperty("Description", "E8 (Red): body side effects (focus to the target cell, selection untouched) apply when a row style menu wins.")]
        public void RowStyleMenuAppliesBodySideEffects()
        {
            var f = LoadFixture(rowStyle: RowStyleWithFlyout());
            SelectRow(f, 1);
            FocusRow(f, 1);

            RequestOnCell(f, 5, OfficeCol);
            VerifyOpened(f, "RowStyleMenu", () => RowOf(f, 5), f.P(5), "UIA on row 5 / Office");

            RunOnUIThread.Execute(() =>
            {
                Verify.AreSame(CellOf(f, 5, OfficeCol), f.Recorder.Opened[0].FocusAtOpening,
                    $"Focus must be on the row 5 Office cell when the row style menu opens; was {Describe(f.Recorder.Opened[0].FocusAtOpening)}.");
                Verify.AreSame(f.P(1), f.Table.SelectedItem, "Selection must be unchanged.");
            });
            CloseMenus(f);
        }

        // ---- F. DataContext, sharing and recycling ------------------------------------------

        [TestMethod]
        [TestProperty("Description", "F1: one shared cell menu sees the requested row's item every time.")]
        public void SharedCellMenuSeesRequestedRowItem()
        {
            var f = LoadFixture();

            RequestOnCell(f, 2, NameCol);
            VerifyOpened(f, "CellMenu", () => CellOf(f, 2, NameCol), f.P(2), "first request, row 2");
            CloseMenus(f);

            RequestOnCell(f, 7, NameCol);
            VerifyOpened(f, "CellMenu", () => CellOf(f, 7, NameCol), f.P(7), "second request, row 7");
            CloseMenus(f);
        }

        [TestMethod]
        [TestProperty("Description", "F2: a row menu command bound with CommandParameter={Binding} acts on the requested item.")]
        public void MenuCommandActsOnRequestedItem()
        {
            MenuFlyoutItem remove = null;
            var f = LoadFixture(configure: x =>
            {
                remove = new MenuFlyoutItem
                {
                    Text = "Remove",
                    Command = new DelegateCommand(p => x.Items.Remove((Person)p)),
                };
                remove.SetBinding(MenuFlyoutItem.CommandParameterProperty, new Binding());
                x.RowMenu.Items.Insert(0, remove);
            });
            Person target = null;
            RunOnUIThread.Execute(() => target = f.P(4));

            RequestOnCell(f, 4, OfficeCol);
            VerifyOpened(f, "RowMenu", () => CellOf(f, 4, OfficeCol), target, "UIA on row 4 / Office");

            RunOnUIThread.Execute(() =>
            {
                var invoke = FrameworkElementAutomationPeer.CreatePeerForElement(remove)?.GetPattern(PatternInterface.Invoke) as IInvokeProvider;
                Verify.IsNotNull(invoke, "The menu item must be invokable through UIA.");
                invoke.Invoke();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsFalse(f.Items.Contains(target), "The requested item must be removed.");
                Verify.AreEqual(9, f.Items.Count, "Only the requested item may be removed.");
            });
            CloseMenus(f);
        }

        [TestMethod]
        [TestProperty("Description", "F3: a recycled row container's menu sees its new item.")]
        public void RecycledRowMenuSeesNewItem()
        {
            var f = LoadFixture(itemCount: 300);
            var original = new Dictionary<TableViewRow, object>();

            RunOnUIThread.Execute(() =>
            {
                foreach (var row in GetRealizedRows(f.Table))
                {
                    original[row] = row.DataContext;
                }
            });
            RequestOnRow(f, 1);
            VerifyOpened(f, "RowMenu", () => RowOf(f, 1), f.P(1), "row container before scrolling");
            CloseMenus(f);

            // Any original container that is now the live container for a different item was recycled.
            TableViewRow container = null;
            object recycledItem = null;
            for (var offset = 2000.0; offset <= 8000.0 && container == null; offset += 2000.0)
            {
                ScrollBodyTo(f.Table, ScrollAxis.Vertical, offset);
                RunOnUIThread.Execute(() =>
                {
                    foreach (var pair in original)
                    {
                        var item = pair.Key.DataContext;
                        if (item != null && !ReferenceEquals(item, pair.Value) &&
                            ReferenceEquals(GetRowForItem(f.Table, item), pair.Key))
                        {
                            container = pair.Key;
                            recycledItem = item;
                            break;
                        }
                    }
                });
            }

            RunOnUIThread.Execute(() =>
            {
                Verify.IsNotNull(container, "Precondition: a row container must be reused for a different item.");
                GetRowPeer(container).ShowContextMenu();
            });
            IdleSynchronizer.Wait();

            VerifyOpened(f, "RowMenu", () => container, recycledItem, "the recycled row container");
            CloseMenus(f);
        }

        [TestMethod]
        [TestProperty("Description", "F4 (smoke): a ContextFlyout on the row template root opens with the row's item.")]
        public void RowTemplateMenuSmoke()
        {
            var f = LoadFixture(configure: x =>
            {
                x.Table.RowContextFlyout = null;
                x.Table.Resources[typeof(TableViewRow)] = (Style)XamlReader.Load(RowTemplateStyleXaml);
            });
            RunOnUIThread.Execute(() => TrackNamedMenu(f, RowOf(f, 3), "TemplateRoot", "RowTemplateMenu"));

            RequestOnCell(f, 3, OfficeCol);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(1, f.Recorder.Opened.Count, $"Exactly one menu must open. Opened: {f.Recorder.Describe()}");
                Verify.AreEqual("RowTemplateMenu", f.Recorder.Opened[0].Name, "The row template menu must open.");
                Verify.AreSame(f.P(3), f.Recorder.Opened[0].ItemsSee, "Its items must see person 3.");
            });
            CloseMenus(f);
        }

        // Header menus see no row item; per-column menus are how an app knows the column (see the sample).
        [TestMethod]
        [TestProperty("Description", "F5: a header menu's items never see a row item.")]
        public void HeaderMenuSeesNoRowItem()
        {
            var f = LoadFixture();

            RequestOnCell(f, 3, OfficeCol);
            CloseMenus(f);
            RequestOnHeader(f, NameCol);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(1, f.Recorder.Opened.Count, $"Exactly one menu must open. Opened: {f.Recorder.Describe()}");
                Verify.AreEqual("HeaderMenu", f.Recorder.Opened[0].Name, "The header menu must open.");
                Verify.IsFalse(f.Recorder.Opened[0].ItemsSee is Person, $"Header menu items must not see a row item; saw {f.Recorder.Opened[0].ItemsSee}.");
            });
            CloseMenus(f);
        }

        // ---- G. Robustness ------------------------------------------------------------------

        [TestMethod]
        [TestProperty("Description", "G1: an item removed while focus moves to the target row suppresses the menu.")]
        public void ItemRemovedDuringFocusMoveSuppressesMenu()
        {
            int ran = 0;
            var f = LoadFixture();
            FocusRow(f, 1);
            RunOnUIThread.Execute(() =>
            {
                var target = f.P(5);
                RowOf(f, 5).GettingFocus += (s, e) =>
                {
                    if (ran++ == 0)
                    {
                        f.Items.Remove(target);
                    }
                };
            });

            RequestOnCell(f, 5, NameCol);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(1, ran, "Precondition: the request must move focus into row 5 exactly once.");
                Verify.AreEqual(0, f.Recorder.Opened.Count, $"No menu may open for a removed item. Opened: {f.Recorder.Describe()}");
            });
            CloseMenus(f);
        }

        [TestMethod]
        [TestProperty("Description", "G2: a column removed while focus moves to the target row is not a target.")]
        public void ColumnRemovedDuringFocusMoveSuppressesMenu()
        {
            int ran = 0;
            var f = LoadFixture();
            FocusRow(f, 1);
            RunOnUIThread.Execute(() => RowOf(f, 5).GettingFocus += (s, e) =>
            {
                if (ran++ == 0)
                {
                    f.Table.Columns.Remove(f.NameColumn);
                }
            });

            RequestOnCell(f, 5, NameCol);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(1, ran, "Precondition: the request must move focus into row 5 exactly once.");
                Verify.IsFalse(f.Recorder.Opened.Any(o => o.Name == "CellMenu"),
                    $"The removed column's cell menu must not open. Opened: {f.Recorder.Describe()}");
            });
            CloseMenus(f);
        }

        [TestMethod]
        [TestProperty("Description", "G3: a request raised from inside Opening is suppressed by the busy guard.")]
        [TestProperty("Ignore", "True")] // A synchronous nested ShowContextMenu fail-fasts in CXcpDispatcher::CheckReentrancy before TableView sees it, so the busy guard cannot be reached from app code.
        public void ReentrantRequestDuringOpeningIsSuppressed()
        {
            var f = LoadFixture();
            bool nested = false;
            RunOnUIThread.Execute(() => f.CellMenu.Opening += (s, e) =>
            {
                if (!nested)
                {
                    nested = true;
                    GetCellPeer(f.Table, f.P(5), NameCol).ShowContextMenu();
                }
            });

            RequestOnCell(f, 2, NameCol);

            RunOnUIThread.Execute(() => Verify.IsTrue(nested, "Precondition: the nested request must have been raised."));
            VerifyOpened(f, "CellMenu", () => CellOf(f, 2, NameCol), f.P(2), "outer request on row 2 / Name");
            CloseMenus(f);
        }

        [TestMethod]
        [TestProperty("Description", "G5 (Red): a TableViewColumn in a content element's Tag does not redirect the request to that column.")]
        public void ColumnTagOnContentDoesNotRedirect()
        {
            var f = LoadFixture(departmentTemplate: ContentTemplate("<TextBlock x:Name='ContentRoot' Text='{Binding Role}'/>"));
            RunOnUIThread.Execute(() =>
            {
                var content = FindNamed(CellOf(f, 3, DepartmentCol), ContentRootName);
                content.Tag = f.NameColumn;
                FrameworkElementAutomationPeer.CreatePeerForElement(content).ShowContextMenu();
            });
            IdleSynchronizer.Wait();

            VerifyOpened(f, "RowMenu", () => CellOf(f, 3, DepartmentCol), f.P(3), "UIA on Department content tagged with the Name column");
            CloseMenus(f);
        }

        // ---- fixture loading ----------------------------------------------------------------

        private ContextMenuFixture LoadFixture(
            Action<ContextMenuFixture> configure = null,
            DataTemplate nameTemplate = null,
            DataTemplate departmentTemplate = null,
            Style rowStyle = null,
            bool grouped = false,
            int itemCount = 10)
        {
            ContextMenuFixture f = null;
            RunOnUIThread.Execute(() =>
            {
                f = CreateContextMenuFixture(nameTemplate, departmentTemplate, rowStyle, grouped, itemCount);
                configure?.Invoke(f);
                LoadContent(f.Root);
            });

            SettleLayout(f.Table);
            return f;
        }
    }

    // Records which tracked flyout opened, its Target, focus at Opening, and what its items see.
    internal sealed class MenuRecorder
    {
        internal sealed class Entry
        {
            public string Name;
            public FlyoutBase Flyout;
            public DependencyObject Target;
            public DependencyObject FocusAtOpening;
            public object ItemsSee;
        }

        private readonly List<Entry> _opened = new List<Entry>();
        private readonly List<FlyoutBase> _tracked = new List<FlyoutBase>();

        public IReadOnlyList<Entry> Opened => _opened;

        public MenuFlyout Create(string name)
        {
            var flyout = new MenuFlyout();
            Track(name, flyout);
            return flyout;
        }

        // Every tracked MenuFlyout gets a probe item so the DataContext its items see is observable.
        public void Track(string name, FlyoutBase flyout)
        {
            Verify.IsNotNull(flyout, $"{name} must exist to be tracked.");
            if (_tracked.Contains(flyout))
            {
                return;
            }

            _tracked.Add(flyout);
            if (flyout is MenuFlyout menu && menu.Items.Count == 0)
            {
                menu.Items.Add(new MenuFlyoutItem { Text = name });
            }

            flyout.Opening += (s, e) =>
            {
                var root = flyout.Target?.XamlRoot;
                _opened.Add(new Entry
                {
                    Name = name,
                    Flyout = flyout,
                    Target = flyout.Target,
                    FocusAtOpening = root == null ? null : FocusManager.GetFocusedElement(root) as DependencyObject,
                });
            };
            flyout.Opened += (s, e) =>
            {
                var entry = _opened.LastOrDefault(o => o.Flyout == flyout);
                if (entry != null && flyout is MenuFlyout menu && menu.Items.Count > 0)
                {
                    entry.ItemsSee = menu.Items[0].DataContext;
                }
            };
        }

        public void HideAll()
        {
            foreach (var flyout in _tracked.Where(fl => fl.IsOpen))
            {
                flyout.Hide();
            }
        }

        public void Clear() => _opened.Clear();

        public string Describe() => _opened.Count == 0 ? "none" : string.Join(", ", _opened.Select(o => o.Name));
    }

    // Three columns over 10 people: Name (text, CellMenu, HeaderMenu), Department (template) and
    // Office (text). RowMenu is the RowContextFlyout and TableMenu the TableView.ContextFlyout.
    // OuterMenu is created but only attached to Root by tests that need it.
    internal sealed class ContextMenuFixture
    {
        public Border Root;
        public TableView Table;
        public ObservableCollection<Person> Items;
        public TableViewColumn NameColumn;
        public TableViewColumn DepartmentColumn;
        public TableViewColumn OfficeColumn;
        public MenuRecorder Recorder = new MenuRecorder();
        public MenuFlyout CellMenu;
        public MenuFlyout RowMenu;
        public MenuFlyout HeaderMenu;
        public MenuFlyout TableMenu;
        public MenuFlyout OuterMenu;

        // 1-based, matching the design doc's "row n" / "person n".
        public Person P(int n) => Items.First(p => p.Name == $"Person {n - 1}");
    }

    internal sealed partial class DelegateCommand : ICommand
    {
        private readonly Action<object> _execute;

        public DelegateCommand(Action<object> execute) { _execute = execute; }

        public event EventHandler CanExecuteChanged { add { } remove { } }

        public bool CanExecute(object parameter) => true;

        public void Execute(object parameter) => _execute(parameter);
    }

    internal static class ContextMenuTestHelpers
    {
        internal const int NameCol = 0;
        internal const int DepartmentCol = 1;
        internal const int OfficeCol = 2;
        internal const string ContentRootName = "ContentRoot";

        internal const string XamlNamespaces =
            "xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' " +
            "xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' " +
            "xmlns:tv='using:Microsoft.UI.Xaml.Controls.Tabular'";

        internal const string RootFlyoutContent =
            "<Grid x:Name='ContentRoot' Background='Transparent'>" +
            "  <Grid.ContextFlyout><MenuFlyout/></Grid.ContextFlyout>" +
            "  <TextBlock Text='{Binding Role}'/>" +
            "</Grid>";

        // The default row template's parts, without visual states, under a root Grid with a ContextFlyout.
        internal const string RowTemplateStyleXaml =
            "<Style " + XamlNamespaces + " TargetType='tv:TableViewRow'>" +
            "  <Setter Property='Template'>" +
            "    <Setter.Value>" +
            "      <ControlTemplate TargetType='tv:TableViewRow'>" +
            "        <Grid x:Name='TemplateRoot' Background='Transparent'>" +
            "          <Grid.ContextFlyout><MenuFlyout/></Grid.ContextFlyout>" +
            "          <Border x:Name='PART_RootBorder' Background='{TemplateBinding Background}'" +
            "                  BorderThickness='{TemplateBinding BorderThickness}' Padding='{TemplateBinding Padding}'>" +
            "            <Grid>" +
            "              <ContentPresenter x:Name='PART_CellForegroundPresenter' Foreground='{TemplateBinding Foreground}'" +
            "                                HorizontalContentAlignment='Stretch' VerticalContentAlignment='Stretch'>" +
            "                <Grid>" +
            "                  <tv:TableViewCellsPanel x:Name='PART_CellsHost' HorizontalAlignment='Left'/>" +
            "                  <Border x:Name='PART_RowExpanderGutter' Width='16' Background='Transparent'" +
            "                          HorizontalAlignment='Left' Visibility='Collapsed'>" +
            "                    <FontIcon x:Name='PART_RowExpanderIcon' FontSize='12' Glyph='&#xE76C;'/>" +
            "                  </Border>" +
            "                </Grid>" +
            "              </ContentPresenter>" +
            "              <Rectangle x:Name='PART_SelectionIndicator' Width='3' Height='16' Fill='Blue'" +
            "                         HorizontalAlignment='Left' IsHitTestVisible='False' Opacity='0'/>" +
            "            </Grid>" +
            "          </Border>" +
            "          <Border x:Name='PART_GridLineBorder' BorderBrush='{TemplateBinding BorderBrush}'" +
            "                  BorderThickness='{TemplateBinding BorderThickness}' IsHitTestVisible='False'/>" +
            "        </Grid>" +
            "      </ControlTemplate>" +
            "    </Setter.Value>" +
            "  </Setter>" +
            "</Style>";

        // Call on the UI thread.
        internal static ContextMenuFixture CreateContextMenuFixture(
            DataTemplate nameTemplate,
            DataTemplate departmentTemplate,
            Style rowStyle,
            bool grouped,
            int itemCount)
        {
            var f = new ContextMenuFixture
            {
                Items = new ObservableCollection<Person>(MakeManyItems(itemCount)),
            };

            object source = f.Items;
            if (grouped)
            {
                var grouping = TableViewSource.From(f.Items);
                grouping.GroupBy(item => (object)((Person)item).Role);
                source = grouping;
            }

            var cellWidth = new GridLength(150.0, GridUnitType.Pixel);
            f.Table = CreateTableViewShell(source, 600, 560);
            f.NameColumn = nameTemplate == null
                ? MakeTextColumn("Name", width: cellWidth)
                : MakeTemplateColumn("Name", nameTemplate, cellWidth);
            f.DepartmentColumn = MakeTemplateColumn("Department", departmentTemplate ?? CreateBoundTextTemplate("Role"), cellWidth);
            f.OfficeColumn = MakeTextColumn("Office", "Role", cellWidth);
            f.Table.Columns.Add(f.NameColumn);
            f.Table.Columns.Add(f.DepartmentColumn);
            f.Table.Columns.Add(f.OfficeColumn);

            f.CellMenu = f.Recorder.Create("CellMenu");
            f.RowMenu = f.Recorder.Create("RowMenu");
            f.HeaderMenu = f.Recorder.Create("HeaderMenu");
            f.TableMenu = f.Recorder.Create("TableMenu");
            f.OuterMenu = f.Recorder.Create("OuterMenu");

            f.NameColumn.CellContextFlyout = f.CellMenu;
            f.NameColumn.HeaderContextFlyout = f.HeaderMenu;
            f.Table.RowContextFlyout = f.RowMenu;
            f.Table.ContextFlyout = f.TableMenu;

            if (rowStyle != null)
            {
                f.Recorder.Track("RowStyleMenu", SetterFlyout(rowStyle));
                f.Table.Resources[typeof(TableViewRow)] = rowStyle;
            }

            f.Root = new Border { Child = f.Table, Background = new SolidColorBrush(Microsoft.UI.Colors.Transparent) };
            return f;
        }

        internal static void ClearTableMenus(ContextMenuFixture f)
        {
            f.NameColumn.CellContextFlyout = null;
            f.NameColumn.HeaderContextFlyout = null;
            f.Table.RowContextFlyout = null;
            f.Table.ContextFlyout = null;
        }

        // XamlReader must run on the UI thread; safe to call from either thread.
        internal static T LoadXaml<T>(string xaml) where T : class
        {
            T result = null;
            RunOnUIThread.Execute(() => result = (T)XamlReader.Load(xaml));
            return result;
        }

        internal static DataTemplate ContentTemplate(string content) => LoadXaml<DataTemplate>(
            "<DataTemplate " + XamlNamespaces + ">" + content + "</DataTemplate>");

        // An implicit TableViewRow style whose ContextFlyout setter shares one MenuFlyout across rows.
        internal static Style RowStyleWithFlyout() => LoadXaml<Style>(
            "<Style " + XamlNamespaces + " TargetType='tv:TableViewRow'>" +
            "  <Setter Property='ContextFlyout'><Setter.Value><MenuFlyout/></Setter.Value></Setter>" +
            "</Style>");

        internal static FlyoutBase SetterFlyout(Style style) => style.Setters
            .OfType<Setter>()
            .Select(s => s.Value as FlyoutBase)
            .FirstOrDefault(v => v != null);

        // ---- requests (call from the test thread) ----

        internal static void RequestOnCell(ContextMenuFixture f, int row, int column)
        {
            RunOnUIThread.Execute(() => GetCellPeer(f.Table, f.P(row), column).ShowContextMenu());
            IdleSynchronizer.Wait();
        }

        internal static void RequestOnRow(ContextMenuFixture f, int row)
        {
            RunOnUIThread.Execute(() => GetRowPeer(RowOf(f, row)).ShowContextMenu());
            IdleSynchronizer.Wait();
        }

        internal static void RequestOnHeader(ContextMenuFixture f, int column)
        {
            RunOnUIThread.Execute(() =>
            {
                var peer = FrameworkElementAutomationPeer.CreatePeerForElement(HeaderOf(f, column));
                Verify.IsNotNull(peer, "The header cell must produce an automation peer.");
                peer.ShowContextMenu();
            });
            IdleSynchronizer.Wait();
        }

        internal static void CloseMenus(ContextMenuFixture f)
        {
            RunOnUIThread.Execute(() =>
            {
                f.Recorder.HideAll();
                f.Recorder.Clear();
            });
            IdleSynchronizer.Wait();
        }

        // ---- state (call on the UI thread) ----

        internal static TableViewRow RowOf(ContextMenuFixture f, int row) => GetRowForItem(f.Table, f.P(row));

        internal static Grid CellOf(ContextMenuFixture f, int row, int column) => GetRowCell(RowOf(f, row), column);

        internal static FrameworkElement HeaderOf(ContextMenuFixture f, int column)
        {
            var headers = GetHeaderCells(f.Table);
            Verify.IsGreaterThan(headers.Count, column, "The header cell must be realized.");
            return headers[column];
        }

        internal static FrameworkElement FindNamed(DependencyObject root, string name)
        {
            var element = FindVisualChildrenByType<FrameworkElement>(root).FirstOrDefault(e => e.Name == name);
            Verify.IsNotNull(element, $"'{name}' must be realized under {root}.");
            return element;
        }

        // Tracks the ContextFlyout of a named element under root. Returns the element.
        internal static FrameworkElement TrackNamedMenu(ContextMenuFixture f, DependencyObject root, string elementName, string menuName)
        {
            var element = FindNamed(root, elementName);
            f.Recorder.Track(menuName, element.ContextFlyout);
            return element;
        }

        // Call from the test thread.
        internal static FrameworkElement TrackContentMenu(ContextMenuFixture f, int row, int column, string elementName, string menuName)
        {
            FrameworkElement element = null;
            RunOnUIThread.Execute(() => element = TrackNamedMenu(f, CellOf(f, row, column), elementName, menuName));
            return element;
        }

        internal static void SelectRow(ContextMenuFixture f, int row)
        {
            RunOnUIThread.Execute(() =>
            {
                var provider = GetRowPeer(RowOf(f, row)).GetPattern(PatternInterface.SelectionItem) as ISelectionItemProvider;
                Verify.IsNotNull(provider, "Rows must advertise SelectionItem.");
                provider.Select();
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() => Verify.AreSame(f.P(row), f.Table.SelectedItem, $"Precondition: row {row} is selected."));
        }

        internal static void FocusRow(ContextMenuFixture f, int row)
        {
            RunOnUIThread.Execute(() => Verify.IsTrue(RowOf(f, row).Focus(FocusState.Programmatic), $"Precondition: row {row} takes focus."));
            IdleSynchronizer.Wait();
        }

        internal static bool IsWithin(DependencyObject element, DependencyObject ancestor)
        {
            for (var node = element; node != null; node = VisualTreeHelper.GetParent(node))
            {
                if (node == ancestor)
                {
                    return true;
                }
            }

            return false;
        }

        internal static void VerifyWithin(DependencyObject element, DependencyObject ancestor, string when)
            => Verify.IsTrue(IsWithin(element, ancestor), $"Focus must be within {Describe(ancestor)} {when}; was {Describe(element)}.");

        internal static void VerifyFocusWithin(ContextMenuFixture f, DependencyObject ancestor, string when)
            => VerifyWithin(FocusManager.GetFocusedElement(f.Table.XamlRoot) as DependencyObject, ancestor, when);

        internal static string Describe(DependencyObject element)
        {
            if (element == null)
            {
                return "null";
            }

            var name = (element as FrameworkElement)?.Name;
            var dc = (element as FrameworkElement)?.DataContext as Person;
            return $"{element.GetType().Name}{(string.IsNullOrEmpty(name) ? "" : $" '{name}'")}{(dc == null ? "" : $" [{dc.Name}]")}";
        }

        // Exactly one tracked menu opened, it is the expected one, anchored to the expected element,
        // and its items see the expected item (when one is given).
        internal static void VerifyOpened(
            ContextMenuFixture f,
            string expectedName,
            Func<DependencyObject> expectedTarget,
            object expectedItem,
            string context)
        {
            RunOnUIThread.Execute(() =>
            {
                var opened = f.Recorder.Opened;
                Verify.AreEqual(1, opened.Count, $"Exactly one menu must open ({context}). Opened: {f.Recorder.Describe()}");
                if (opened.Count != 1)
                {
                    return;
                }

                var entry = opened[0];
                Verify.AreEqual(expectedName, entry.Name, $"Menu ({context}).");

                var target = expectedTarget();
                Verify.AreSame(target, entry.Target, $"Target ({context}): expected {Describe(target)}, was {Describe(entry.Target)}.");

                if (expectedItem != null)
                {
                    Verify.AreSame(expectedItem, entry.ItemsSee, $"Menu items' DataContext ({context}): saw {entry.ItemsSee}.");
                }
            });
        }
    }
}
