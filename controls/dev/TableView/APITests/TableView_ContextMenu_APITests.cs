// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Automation;
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

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.ContextMenuTestHelpers;
using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewAutomationTestHelpers;
using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewRowTestHelpers;
using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewTestHelpers;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    // Context menu resolution (cell, header, row, content and TableView flyouts). Requests go through
    // an automation peer's ShowContextMenu; real input is covered by the interaction tests.
    [TestClass]
    public class TableViewContextMenuTests : TableViewApiTestBase
    {
        // ---- A. Body precedence -------------------------------------------------------------

        [TestMethod]
        [TestProperty("Description", "A column's CellContextFlyout wins over the row style menu and anchors to the cell.")]
        public void CellMenuWinsOverRowMenu()
        {
            var f = LoadFixture();

            RequestOnCell(f, 3, NameCol);

            VerifyOpened(f, "CellMenu", () => CellOf(f, 3, NameCol), f.P(3), "UIA on row 3 / Name");
            CloseMenus(f);
        }

        [TestMethod]
        [TestProperty("Description", "A cell without a CellContextFlyout falls back to the row style menu, anchored to the row.")]
        public void RowMenuUsedWhenColumnHasNoCellMenu()
        {
            var f = LoadFixture();

            RequestOnCell(f, 3, OfficeCol);

            VerifyOpened(f, "RowMenu", () => RowOf(f, 3), f.P(3), "UIA on row 3 / Office");
            CloseMenus(f);
        }

        [TestMethod]
        [TestProperty("Description", "With no cell or row menu the TableView.ContextFlyout opens at the requested cell.")]
        public void TableMenuUsedWhenNoCellOrRowMenu()
        {
            var f = LoadFixture(configure: x => x.Table.Resources.Remove(typeof(TableViewRow)));

            RequestOnCell(f, 3, OfficeCol);

            VerifyOpened(f, "TableMenu", () => f.Table, expectedItem: null, "UIA on row 3 / Office, no row style menu");
            VerifyMenuPlacedAtTarget(f, () => CellOf(f, 3, OfficeCol));
            CloseMenus(f);
        }

        [TestMethod]
        [TestProperty("Description", "With no TableView candidate the request is left unhandled and an ancestor's ContextFlyout opens.")]
        public void UnhandledWhenNoCandidateBubblesToAncestor()
        {
            var f = LoadFixture(configure: x =>
            {
                x.NameColumn.CellContextFlyout = null;
                x.NameColumn.HeaderContextFlyout = null;
                x.Table.Resources.Remove(typeof(TableViewRow));
                x.Table.ContextFlyout = null;
                x.Root.ContextFlyout = x.Recorder.Create("OuterMenu");
            });

            RequestOnCell(f, 3, NameCol);

            VerifyOpened(f, "OuterMenu", () => f.Root, expectedItem: null, "UIA on row 3 / Name, no TableView menus");
            CloseMenus(f);
        }

        [TestMethod]
        [TestProperty("Description", "CellContextFlyout is read per request; a new value applies and null falls back to the row menu.")]
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
            VerifyOpened(f, "RowMenu", () => RowOf(f, 3), f.P(3), "after clearing CellContextFlyout");
            CloseMenus(f);
        }

        [TestMethod]
        [TestProperty("Description", "A ContextFlyout on the cell template root wins over the column's CellContextFlyout, including one assigned after load.")]
        public void CellTemplateFlyoutWinsOverCellMenu()
        {
            var f = LoadFixture(
                departmentTemplate: ContentTemplate(RootFlyoutContent),
                configure: x => x.DepartmentColumn.CellContextFlyout = x.CellMenu);
            var content = TrackContentMenu(f, 3, DepartmentCol, ContentRootName, "ContentMenu");

            RequestOnCell(f, 3, DepartmentCol);
            VerifyOpened(f, "ContentMenu", () => content, f.P(3), "UIA on row 3 / Department");
            CloseMenus(f);

            RunOnUIThread.Execute(() => f.DepartmentColumn.CellContextFlyout = f.Recorder.Create("CellMenu2"));
            RequestOnCell(f, 3, DepartmentCol);
            VerifyOpened(f, "ContentMenu", () => content, f.P(3), "UIA on row 3 / Department, CellContextFlyout replaced after load");
            CloseMenus(f);
        }

        [TestMethod]
        [TestProperty("Description", "A ContextFlyout on a nested (non-root) element in the cell template is found.")]
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
        [TestProperty("Description", "With nested content flyouts, UIA/keyboard requests pick the outermost one.")]
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
        [TestProperty("Description", "A ContextFlyout on collapsed content is not a candidate.")]
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
        [TestProperty("Description", "A text control's built-in copy/select flyout is not an app menu; the cell menu still opens.")]
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
        [TestProperty("Description", "A ContextFlyout set locally on a text control is an app menu and wins.")]
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
        [TestProperty("Description", "The content search stops at a nested TableView.")]
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
        [TestProperty("Description", "The content search stops at a nested items control, like it does at a nested TableView.")]
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
        [TestProperty("Description", "A request raised on an element inside the cell template opens the template flyout, not CellContextFlyout.")]
        public void RequestFromCellContentWinsOverCellMenu()
        {
            var f = LoadFixture(
                departmentTemplate: ContentTemplate(ButtonFlyoutContent("{Binding Role}")),
                configure: x => x.DepartmentColumn.CellContextFlyout = x.CellMenu);
            var content = TrackContentMenu(f, 3, DepartmentCol, ContentRootName, "ContentMenu");

            RequestOnElement(() => FindNamed(CellOf(f, 3, DepartmentCol), ContentButtonName));

            VerifyOpened(f, "ContentMenu", () => content, expectedItem: null, "UIA on the button in row 3 / Department");
            CloseMenus(f);
        }

        // ---- B. Row targets -----------------------------------------------------------------

        [TestMethod]
        [TestProperty("Description", "A row peer request targets the row even after a cell request made Name the current column.")]
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
        [TestProperty("Description", "A header request opens the column's HeaderContextFlyout anchored to that header; its items see no row item, even after a body request.")]
        public void HeaderMenuOpensOnHeader()
        {
            var f = LoadFixture();
            RequestOnCell(f, 3, OfficeCol);
            CloseMenus(f);

            RequestOnHeader(f, NameCol);

            VerifyOpened(f, "HeaderMenu", () => HeaderOf(f, NameCol), NoItem, "UIA on the Name header");
            CloseMenus(f);
        }

        [TestMethod]
        [TestProperty("Description", "A header without a menu falls back to TableView.ContextFlyout, never to cell or row menus.")]
        public void HeaderWithoutMenuFallsBackToTableMenu()
        {
            var f = LoadFixture(configure: x => x.OfficeColumn.CellContextFlyout = x.CellMenu);

            RequestOnHeader(f, OfficeCol);

            VerifyOpened(f, "TableMenu", () => f.Table, expectedItem: null, "UIA on the Office header");
            VerifyMenuPlacedAtTarget(f, () => HeaderOf(f, OfficeCol));
            CloseMenus(f);
        }

        [TestMethod]
        [TestProperty("Description", "A ContextFlyout in the header template wins over HeaderContextFlyout.")]
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
        [TestProperty("Description", "A header request focuses the header and leaves body selection unchanged; focus returns to the header when the menu closes.")]
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
                VerifyFocusWithin(f, HeaderOf(f, NameCol), "after the header menu closes");
            });
        }

        [TestMethod]
        [TestProperty("Description", "A header peer for a collapsed column must not open any menu.")]
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

        [TestMethod]
        [TestProperty("Description", "A TableView-owned header peer (public constructor) forwards ShowContextMenu to the realized header, so the column's header menu opens rather than the table menu.")]
        public void TableOwnedHeaderPeerOpensHeaderMenu()
        {
            var f = LoadFixture();

            RunOnUIThread.Execute(() => new TableViewColumnHeaderAutomationPeer(f.Table, f.NameColumn).ShowContextMenu());
            IdleSynchronizer.Wait();

            VerifyOpened(f, "HeaderMenu", () => HeaderOf(f, NameCol), expectedItem: null, "UIA on a TableView-owned Name header peer");
            CloseMenus(f);
        }

        [TestMethod]
        [TestProperty("Description", "A header template flyout wins over TableView.ContextFlyout when the column has no HeaderContextFlyout, and cell menus never apply.")]
        public void HeaderTemplateFlyoutWinsOverTableMenu()
        {
            var f = LoadFixture(configure: x =>
            {
                x.OfficeColumn.CellContextFlyout = x.CellMenu;
                x.OfficeColumn.HeaderTemplate = ContentTemplate(
                    "<Grid x:Name='ContentRoot' Background='Transparent'>" +
                    "  <Grid.ContextFlyout><MenuFlyout/></Grid.ContextFlyout>" +
                    "  <TextBlock Text='{Binding}'/>" +
                    "</Grid>");
            });
            FrameworkElement content = null;
            RunOnUIThread.Execute(() => content = TrackNamedMenu(f, HeaderOf(f, OfficeCol), ContentRootName, "ContentMenu"));

            RequestOnHeader(f, OfficeCol);

            VerifyOpened(f, "ContentMenu", () => content, expectedItem: null, "UIA on the Office header");
            CloseMenus(f);
        }

        [TestMethod]
        [TestProperty("Description", "A request raised on an element inside the header template opens the template flyout, not HeaderContextFlyout.")]
        public void RequestFromHeaderContentWinsOverHeaderMenu()
        {
            var f = LoadFixture(configure: x => x.NameColumn.HeaderTemplate = ContentTemplate(ButtonFlyoutContent("{Binding}")));
            FrameworkElement content = null;
            RunOnUIThread.Execute(() => content = TrackNamedMenu(f, HeaderOf(f, NameCol), ContentRootName, "ContentMenu"));

            RequestOnElement(() => FindNamed(HeaderOf(f, NameCol), ContentButtonName));

            VerifyOpened(f, "ContentMenu", () => content, expectedItem: null, "UIA on the button in the Name header");
            CloseMenus(f);
        }

        // ---- D. Group headers ---------------------------------------------------------------

        [TestMethod]
        [TestProperty("Description", "Group headers never use cell or row menus; the table menu opens.")]
        public void GroupHeaderUsesTableMenu()
        {
            var f = LoadFixture(grouped: true);

            RunOnUIThread.Execute(() => GetGroupHeaderPeer(f.Table, 0).ShowContextMenu());
            IdleSynchronizer.Wait();

            VerifyOpened(f, "TableMenu", () => f.Table, expectedItem: null, "UIA on the first group header");
            CloseMenus(f);
        }

        [TestMethod]
        [TestProperty("Description", "A group header ContextFlyout from an implicit style wins over TableView.ContextFlyout, and its items see the TableViewGroupInfo.")]
        public void GroupHeaderStyleMenuWins()
        {
            var f = LoadFixture(grouped: true, configure: x =>
            {
                var style = (Style)XamlReader.Load(
                    "<Style " + XamlNamespaces + " TargetType='tv:TableViewGroupHeader'>" +
                    "  <Setter Property='ContextFlyout'><Setter.Value><MenuFlyout><MenuFlyoutItem Text='{Binding KeyText}'/></MenuFlyout></Setter.Value></Setter>" +
                    "</Style>");
                x.Recorder.Track("GroupHeaderMenu", SetterFlyout(style));
                x.Table.Resources[typeof(TableViewGroupHeader)] = style;
            });

            RunOnUIThread.Execute(() => GetGroupHeaderPeer(f.Table, 0).ShowContextMenu());
            IdleSynchronizer.Wait();

            object groupInfo = null;
            RunOnUIThread.Execute(() => groupInfo = GetGroupHeader(f.Table, 0).Content);
            Verify.IsTrue(groupInfo is TableViewGroupInfo, "Precondition: a group header's Content is its TableViewGroupInfo.");
            VerifyOpened(f, "GroupHeaderMenu", () => GetGroupHeader(f.Table, 0), groupInfo, "UIA on the first group header");
            RunOnUIThread.Execute(() =>
            {
                var item = (MenuFlyoutItem)((MenuFlyout)f.Recorder.Opened[0].Flyout).Items[0];
                Verify.AreEqual(((TableViewGroupInfo)groupInfo).KeyText, item.Text, "{Binding KeyText} on a group header menu item must resolve.");
            });
            CloseMenus(f);
        }

        // ---- E. Side effects ----------------------------------------------------------------

        [TestMethod]
        [TestProperty("Description", "A body request does not change the selection.")]
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
        [TestProperty("Description", "A body request moves focus into the target row, and focus is still there after the menu closes.")]
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
        [TestProperty("Description", "A cell request focuses the exact target cell; a row request focuses the row itself.")]
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

        [TestMethod]
        [TestProperty("Description", "A row-level menu request records its row as the current row even though the menu has taken focus by the time the row's GotFocus runs (GH #12442).")]
        public void RowRequestRecordsRowAsCurrentWhileMenuHasFocus()
        {
            Button after = null;
            var f = LoadFixture(configure: x =>
            {
                // Headers hidden: the body is the table's tab stop.
                x.Table.HeadersVisibility = TableViewHeadersVisibility.None;
                after = new Button { Content = "After" };
                x.Root.Child = null;
                var panel = new StackPanel();
                panel.Children.Add(x.Table);
                panel.Children.Add(after);
                x.Root.Child = panel;
            });
            FocusRow(f, 1);

            RequestOnRow(f, 4);
            VerifyOpened(f, "RowMenu", () => RowOf(f, 4), f.P(4), "UIA on row 4's peer");

            // A menu command that moves focus out of the table, so closing the menu does not refocus row 4.
            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(after.Focus(FocusState.Keyboard), "Precondition: the button takes focus.");
                f.Recorder.HideAll();
                f.Recorder.Clear();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreSame(after, FocusManager.GetFocusedElement(f.Table.XamlRoot), "Precondition: focus stays on the button.");
                Verify.IsTrue(FocusManager.TryMoveFocus(FocusNavigationDirection.Previous,
                    new FindNextElementOptions { SearchRoot = f.Table.XamlRoot.Content }), "Shift+Tab from the button must move focus.");
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() => Verify.AreSame(RowOf(f, 4), FocusManager.GetFocusedElement(f.Table.XamlRoot),
                $"Shift+Tab back in must return to row 4, the row the menu was opened on; was {Describe(FocusManager.GetFocusedElement(f.Table.XamlRoot) as DependencyObject)}."));
        }

        [TestMethod]
        [TestProperty("Description", "A row menu requested in the same turn as a cell focus on that drilled-in row leaves the row at row level, so closing the menu restores focus to the row (GH #12442).")]
        public void RowRequestAfterSameTurnCellFocusKeepsRowLevel()
        {
            var f = LoadFixture(configure: x => x.OfficeColumn.CellContextFlyout = x.CellMenu);
            RunOnUIThread.Execute(() => GetCellPeer(f.Table, f.P(4), NameCol).SetFocus());
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() => Verify.IsFalse(RowOf(f, 4).IsTabStop, "Precondition: row 4 is at cell level."));

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(CellOf(f, 4, OfficeCol).Focus(FocusState.Keyboard), "Precondition: the Office cell takes focus.");
                GetRowPeer(RowOf(f, 4)).ShowContextMenu();
            });
            IdleSynchronizer.Wait();

            VerifyOpened(f, "RowMenu", () => RowOf(f, 4), f.P(4), "UIA on row 4's peer");
            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(RowOf(f, 4).IsTabStop, "Row 4 must be at row level while its row menu is open.");
                Verify.IsFalse(CellOf(f, 4, OfficeCol).IsTabStop, "Row 4's cells must be out of the tab order.");
            });

            CloseMenus(f);
            RunOnUIThread.Execute(() => Verify.AreSame(RowOf(f, 4), FocusManager.GetFocusedElement(f.Table.XamlRoot),
                $"Closing the menu must restore focus to row 4; was {Describe(FocusManager.GetFocusedElement(f.Table.XamlRoot) as DependencyObject)}."));
        }

        [TestMethod]
        [TestProperty("Description", "A row menu opened from a request on an inactive cell focuses that cell's row and leaves the selection unchanged.")]
        public void RowMenuFromInactiveCellRequestFocusesRow()
        {
            var f = LoadFixture();
            SelectRow(f, 1);
            FocusRow(f, 1);

            RequestOnCell(f, 5, OfficeCol);
            VerifyOpened(f, "RowMenu", () => RowOf(f, 5), f.P(5), "UIA on row 5 / Office");

            RunOnUIThread.Execute(() =>
            {
                // A read-only cell with no tooltip, control or menu is not a focus target.
                Verify.AreSame(RowOf(f, 5), f.Recorder.Opened[0].FocusAtOpening,
                    $"Focus must be on row 5 when the row menu opens; was {Describe(f.Recorder.Opened[0].FocusAtOpening)}.");
                Verify.AreSame(f.P(1), f.Table.SelectedItem, "Selection must be unchanged.");
            });
            CloseMenus(f);
        }

        [TestMethod]
        [TestProperty("Description", "While any cell is editing, a request on another row's cell opens no menu (neither its cell, row nor table menu).")]
        public void CellRequestOnOtherRowSuppressedWhileEditing()
        {
            var f = LoadEditableFixture();
            RequestWhileEditing(f, () => GetCellPeer(f.Table, f.P(6), NameCol).ShowContextMenu());
            VerifyNoMenuOpened(f, "UIA on row 6 / Name while row 3 / Name is editing");
        }

        [TestMethod]
        [TestProperty("Description", "While any cell is editing, a row request opens no menu.")]
        public void RowRequestSuppressedWhileEditing()
        {
            var f = LoadEditableFixture();
            RequestWhileEditing(f, () => ((AutomationPeer)FrameworkElementAutomationPeer.CreatePeerForElement(RowOf(f, 6))).ShowContextMenu());
            VerifyNoMenuOpened(f, "UIA on row 6 while row 3 / Name is editing");
        }

        [TestMethod]
        [TestProperty("Description", "While any cell is editing, a column header request opens no menu (neither HeaderContextFlyout nor the table menu).")]
        public void HeaderRequestSuppressedWhileEditing()
        {
            var f = LoadEditableFixture();
            RequestWhileEditing(f, () => new TableViewColumnHeaderAutomationPeer(f.Table, f.NameColumn).ShowContextMenu());
            VerifyNoMenuOpened(f, "UIA on the Name header while row 3 / Name is editing");
        }

        [TestMethod]
        [TestProperty("Description", "While any cell is editing, a group header request opens no menu (neither its style ContextFlyout nor the table menu).")]
        public void GroupHeaderRequestSuppressedWhileEditing()
        {
            var f = LoadEditableFixture(grouped: true, configure: x =>
            {
                var style = (Style)XamlReader.Load(
                    "<Style " + XamlNamespaces + " TargetType='tv:TableViewGroupHeader'>" +
                    "  <Setter Property='ContextFlyout'><Setter.Value><MenuFlyout><MenuFlyoutItem Text='Group'/></MenuFlyout></Setter.Value></Setter>" +
                    "</Style>");
                x.Recorder.Track("GroupHeaderMenu", SetterFlyout(style));
                x.Table.Resources[typeof(TableViewGroupHeader)] = style;
            });
            RequestWhileEditing(f, () => GetGroupHeaderPeer(f.Table, 0).ShowContextMenu());
            VerifyNoMenuOpened(f, "UIA on the first group header while row 3 / Name is editing");
        }

        [TestMethod]
        [TestProperty("Description", "While any cell is editing, a request on the TableView itself opens no menu.")]
        public void TableRequestSuppressedWhileEditing()
        {
            var f = LoadEditableFixture();
            RequestWhileEditing(f, () => ((AutomationPeer)FrameworkElementAutomationPeer.CreatePeerForElement(f.Table)).ShowContextMenu());
            VerifyNoMenuOpened(f, "UIA on the TableView while row 3 / Name is editing");
        }

        [TestMethod]
        public void HeaderDropdownIsOptInAndUpdatesWithoutRebuilding()
        {
            var f = LoadFixture();
            RunOnUIThread.Execute(() =>
            {
                var header = HeaderOf(f, NameCol);
                var button = FindVisualChildrenByType<DropDownButton>(header).Single();
                Verify.IsNull(f.NameColumn.HeaderFlyout);
                Verify.AreEqual(Visibility.Collapsed, button.Visibility);
                var peer = FrameworkElementAutomationPeer.CreatePeerForElement(header);
                Verify.AreEqual(0, peer.GetChildren().Count(p => p.GetPattern(PatternInterface.ExpandCollapse) != null));

                f.NameColumn.HeaderFlyout = f.Recorder.Create("HeaderOptions");
                f.Table.UpdateLayout();
                Verify.AreEqual(header, HeaderOf(f, NameCol), "A live flyout update must not rebuild the header.");
                Verify.AreEqual(Visibility.Visible, button.Visibility);
                Verify.AreEqual(26.0, button.ActualWidth);
                Verify.AreEqual(header.ActualHeight, button.ActualHeight);
                Verify.IsTrue(button.IsTabStop, "The header host owns the band's Tab policy; the dropdown must remain focusable.");
                Verify.AreEqual("Options for Name", AutomationProperties.GetName(button));
                Verify.AreEqual(1, peer.GetChildren().Count(p => p.GetPattern(PatternInterface.ExpandCollapse) != null),
                    "The realized header exposes its dropdown without hiding app-supplied header content.");

                f.NameColumn.HeaderFlyout = null;
                f.Table.UpdateLayout();
                Verify.AreEqual(header, HeaderOf(f, NameCol));
                Verify.AreEqual(Visibility.Collapsed, button.Visibility);
                Verify.IsNull(button.Flyout);
                Verify.AreEqual(0, peer.GetChildren().Count(p => p.GetPattern(PatternInterface.ExpandCollapse) != null));
                f.NameColumn.Header = "";
                f.NameColumn.HeaderFlyout = f.Recorder.Create("UnnamedHeaderOptions");
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                var button = FindVisualChildrenByType<DropDownButton>(HeaderOf(f, NameCol)).Single();
                Verify.AreEqual("Column options", AutomationProperties.GetName(button),
                    "A header without text still needs an actionable accessible name.");
            });
        }

        [TestMethod]
        public void HeaderDropdownCanTakeKeyboardFocusWithoutAddingABandTabStop()
        {
            var f = LoadFixture(configure: x => x.NameColumn.HeaderFlyout = x.Recorder.Create("HeaderOptions"));
            RunOnUIThread.Execute(() =>
            {
                var header = (TableViewColumnHeader)HeaderOf(f, NameCol);
                var button = FindVisualChildrenByType<DropDownButton>(header).Single();
                Verify.IsTrue(button.Focus(FocusState.Keyboard),
                    "The dropdown must be focusable for keyboard opening and native flyout focus restoration.");
                Verify.AreEqual(button, FocusManager.GetFocusedElement(f.Table.XamlRoot));
                Verify.IsTrue(header.Focus(FocusState.Keyboard));
                Verify.IsTrue(FocusManager.TryMoveFocus(FocusNavigationDirection.Next,
                    new FindNextElementOptions { SearchRoot = f.Table.XamlRoot.Content }));
            });
            SettleLayout(f.Table);
            RunOnUIThread.Execute(() =>
            {
                var focused = FocusManager.GetFocusedElement(f.Table.XamlRoot);
                Verify.IsTrue(focused is TableViewRow,
                    "Tab must still leave the header band for the body, not visit its dropdown buttons.");
                Log.Comment($"Focus after leaving the header band: {focused?.GetType().Name}.");
                var button = FindVisualChildrenByType<DropDownButton>(HeaderOf(f, NameCol)).Single();
                Verify.IsTrue(button.Focus(FocusState.Keyboard));
                ((IExpandCollapseProvider)FrameworkElementAutomationPeer.CreatePeerForElement(button)
                    .GetPattern(PatternInterface.ExpandCollapse)).Expand();
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(f.NameColumn.HeaderFlyout.IsOpen);
                f.NameColumn.HeaderFlyout.Hide();
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                var button = FindVisualChildrenByType<DropDownButton>(HeaderOf(f, NameCol)).Single();
                Verify.AreEqual(button, FocusManager.GetFocusedElement(f.Table.XamlRoot),
                    "Closing the menu restores focus to its keyboard-focusable dropdown button.");
            });
        }

        [TestMethod]
        public void HeaderDropdownKeepsItsOuterEdgeAfterFiltering()
        {
            TableViewSource source = null;
            var f = LoadFixture(itemCount: 30, configure: x =>
            {
                source = TableViewSource.From(x.Items);
                x.Table.ItemsSource = source;
                x.Table.Height = 240;
                x.OfficeColumn.Width = new GridLength(1, GridUnitType.Star);
                x.OfficeColumn.HeaderFlyout = x.Recorder.Create("HeaderOptions");
            });

            foreach (var flow in new[] { FlowDirection.LeftToRight, FlowDirection.RightToLeft })
            {
                RunOnUIThread.Execute(() => f.Table.FlowDirection = flow);
                SettleLayout(f.Table);
                foreach (var filteredCount in new[] { 30, 1, 0, 30 })
                {
                    RunOnUIThread.Execute(() =>
                    {
                        var button = FindVisualChildrenByType<DropDownButton>(HeaderOf(f, OfficeCol)).Single();
                        button.Flyout.ShowAt(button);
                        if (filteredCount == 30)
                        {
                            source.ClearFilter();
                        }
                        else
                        {
                            source.Filter(item => filteredCount == 1 && ((Person)item).Name == "Person 0");
                        }
                        button.Flyout.Hide();
                    });
                    SettleLayout(f.Table);
                    RunOnUIThread.Execute(() =>
                    {
                        var header = HeaderOf(f, OfficeCol);
                        var button = FindVisualChildrenByType<DropDownButton>(header).Single();
                        var chrome = (Border)button.FindVisualChildByName("Chrome");
                        var separator = (Border)header.FindVisualChildByName("PART_GridLine");
                        var context = $"{flow}, {filteredCount} filtered rows";
                        var headerBounds = header.TransformToVisual(f.Table).TransformBounds(
                            new global::Windows.Foundation.Rect(0, 0, header.ActualWidth, header.ActualHeight));
                        Log.Comment($"{context}: table width {f.Table.ActualWidth}, border {f.Table.BorderThickness}, header bounds {headerBounds}.");
                        Verify.AreEqual(Visibility.Collapsed, separator.Visibility,
                            $"The last header must not paint an extra stroke against the table border ({context}).");
                        Verify.IsTrue(chrome.CornerRadius.TopRight > 0,
                            $"The dropdown must retain its local trailing corner, mirrored by XAML in RTL ({context}).");
                    });
                }
            }
        }

        [TestMethod]
        public void HeaderDropdownDoesNotSortAndKeepsContextMenuIndependent()
        {
            var f = LoadFixture(configure: x => x.NameColumn.HeaderFlyout = x.Recorder.Create("HeaderOptions"));
            RunOnUIThread.Execute(() =>
            {
                var button = FindVisualChildrenByType<DropDownButton>(HeaderOf(f, NameCol)).Single();
                var peer = FrameworkElementAutomationPeer.CreatePeerForElement(button);
                var provider = (IExpandCollapseProvider)peer.GetPattern(PatternInterface.ExpandCollapse);
                Verify.AreEqual(ExpandCollapseState.Collapsed, provider.ExpandCollapseState);
                provider.Expand();
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(f.NameColumn.HeaderFlyout.IsOpen);
                Verify.IsFalse(f.HeaderMenu.IsOpen);
                Verify.AreEqual(SortDirection.None, f.NameColumn.SortDirection);
                var button = FindVisualChildrenByType<DropDownButton>(HeaderOf(f, NameCol)).Single();
                var peer = FrameworkElementAutomationPeer.CreatePeerForElement(button);
                var provider = (IExpandCollapseProvider)peer.GetPattern(PatternInterface.ExpandCollapse);
                Verify.AreEqual(ExpandCollapseState.Expanded, provider.ExpandCollapseState);
                provider.Collapse();
            });
            IdleSynchronizer.Wait();
            CloseMenus(f);
            RequestOnHeader(f, NameCol);
            VerifyOpened(f, "HeaderMenu", () => HeaderOf(f, NameCol), NoItem, "HeaderContextFlyout is independent of HeaderFlyout");
            CloseMenus(f);

            RunOnUIThread.Execute(() =>
            {
                f.NameColumn.CanSort = false;
                f.Table.UpdateLayout();
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                var button = FindVisualChildrenByType<DropDownButton>(HeaderOf(f, NameCol)).Single();
                Verify.AreEqual(Visibility.Visible, button.Visibility);
                ((IExpandCollapseProvider)FrameworkElementAutomationPeer.CreatePeerForElement(button)
                    .GetPattern(PatternInterface.ExpandCollapse)).Expand();
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() => Verify.IsTrue(f.NameColumn.HeaderFlyout.IsOpen,
                "The dropdown does not require sorting to be enabled."));
            CloseMenus(f);
        }

        [TestMethod]
        public void HeaderDropdownClosesWhenReplacedDisabledOrRemoved()
        {
            var f = LoadFixture(configure: x => x.NameColumn.HeaderFlyout = x.Recorder.Create("HeaderOptions"));
            MenuFlyout oldFlyout = null;
            IExpandCollapseProvider retainedProvider = null;
            RunOnUIThread.Execute(() =>
            {
                oldFlyout = (MenuFlyout)f.NameColumn.HeaderFlyout;
                oldFlyout.Closing += (s, e) => e.Cancel = true;
                var button = FindVisualChildrenByType<DropDownButton>(HeaderOf(f, NameCol)).Single();
                retainedProvider = (IExpandCollapseProvider)FrameworkElementAutomationPeer.CreatePeerForElement(button)
                    .GetPattern(PatternInterface.ExpandCollapse);
                oldFlyout.ShowAt(button);
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                f.NameColumn.HeaderFlyout = f.Recorder.Create("ReplacementOptions");
                f.NameColumn.HeaderFlyout.Closing += (s, e) => e.Cancel = true;
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                Verify.IsFalse(oldFlyout.IsOpen, "Replacement must close the old flyout despite a Closing veto.");
                Verify.AreEqual(ExpandCollapseState.Collapsed, retainedProvider.ExpandCollapseState,
                    "The reused native dropdown must not retain its detached menu's expanded state.");
                var button = FindVisualChildrenByType<DropDownButton>(HeaderOf(f, NameCol)).Single();
                Verify.AreEqual(f.NameColumn.HeaderFlyout, button.Flyout);
                button.Flyout.ShowAt(button);
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() => f.Table.IsEnabled = false);
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                Verify.IsFalse(f.NameColumn.HeaderFlyout.IsOpen, "Disabling must close the flyout despite a Closing veto.");
                var button = FindVisualChildrenByType<DropDownButton>(HeaderOf(f, NameCol)).Single();
                Verify.IsNull(button.Flyout, "A retained UIA provider cannot open a disabled header's flyout.");
                f.Table.IsEnabled = true;
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                var button = FindVisualChildrenByType<DropDownButton>(HeaderOf(f, NameCol)).Single();
                Verify.AreEqual(f.NameColumn.HeaderFlyout, button.Flyout);
                button.Flyout.ShowAt(button);
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                var header = (TableViewColumnHeader)HeaderOf(f, NameCol);
                header.Template = (ControlTemplate)XamlReader.Load(
                    "<ControlTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' TargetType='ContentControl'>" +
                    "<ContentPresenter Content='{TemplateBinding Content}'/></ControlTemplate>");
                header.ApplyTemplate();
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                Verify.IsFalse(f.NameColumn.HeaderFlyout.IsOpen, "Retemplating closes the old button's flyout despite a Closing veto.");
                var header = (TableViewColumnHeader)HeaderOf(f, NameCol);
                Verify.AreEqual(0, FindVisualChildrenByType<DropDownButton>(header).Count,
                    "Custom templates can omit the dropdown part.");
                header.ClearValue(Control.TemplateProperty);
                header.ApplyTemplate();
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                var button = FindVisualChildrenByType<DropDownButton>(HeaderOf(f, NameCol)).Single();
                Verify.AreEqual(f.NameColumn.HeaderFlyout, button.Flyout);
                button.Flyout.ShowAt(button);
            });
            IdleSynchronizer.Wait();
            UIElement rootContent = null;
            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(f.NameColumn.HeaderFlyout.IsOpen, "Precondition: the dropdown is open before unloading.");
                rootContent = f.Root.Child;
                f.Root.Child = null;
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                Verify.IsFalse(f.NameColumn.HeaderFlyout.IsOpen, "Unloading closes the dropdown despite a Closing veto.");
                f.Root.Child = rootContent;
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                var button = FindVisualChildrenByType<DropDownButton>(HeaderOf(f, NameCol)).Single();
                button.Flyout.ShowAt(button);
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() => f.Table.Columns.Remove(f.NameColumn));
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() => Verify.IsFalse(f.NameColumn.HeaderFlyout.IsOpen,
                "Removing a column must close its dropdown despite a Closing veto."));
            CloseMenus(f);
        }

        [TestMethod]
        public void HeaderDropdownCancelsPendingOpenWhenReplacedOrReleased()
        {
            foreach (var release in new[] { false, true })
            {
                var f = LoadFixture(configure: x => x.NameColumn.HeaderFlyout = x.Recorder.Create("HeaderOptions"));
                MenuFlyout oldFlyout = null;
                bool opened = false;
                bool opening = false;
                RunOnUIThread.Execute(() =>
                {
                    oldFlyout = (MenuFlyout)f.NameColumn.HeaderFlyout;
                    oldFlyout.Opened += (s, e) => opened = true;
                    oldFlyout.Closing += (s, e) => e.Cancel = true;
                    oldFlyout.Opening += (s, e) =>
                    {
                        opening = true;
                        Verify.IsFalse(oldFlyout.IsOpen, "The callback must run before the flyout reports open.");
                        if (release)
                        {
                            f.Table.Columns.Remove(f.NameColumn);
                        }
                        else
                        {
                            f.NameColumn.HeaderFlyout = f.Recorder.Create("ReplacementOptions");
                        }
                    };
                    var button = FindVisualChildrenByType<DropDownButton>(HeaderOf(f, NameCol)).Single();
                    oldFlyout.ShowAt(button);
                });
                IdleSynchronizer.Wait();
                RunOnUIThread.Execute(() =>
                {
                    Verify.IsTrue(opening);
                    Verify.IsFalse(opened, "A detached pending menu must never finish opening.");
                    Verify.IsFalse(oldFlyout.IsOpen);
                    if (!release)
                    {
                        var button = FindVisualChildrenByType<DropDownButton>(HeaderOf(f, NameCol)).Single();
                        Verify.AreEqual(f.NameColumn.HeaderFlyout, button.Flyout);
                        button.Flyout.ShowAt(button);
                    }
                });
                IdleSynchronizer.Wait();
                if (!release)
                {
                    RunOnUIThread.Execute(() => Verify.IsTrue(f.NameColumn.HeaderFlyout.IsOpen,
                        "Cancelling the stale open must not prevent the replacement from opening."));
                }
                CloseMenus(f);
            }
        }

        [TestMethod]
        public void HeaderDropdownDoesNotCloseSharedFlyoutOwnedByAnotherHeader()
        {
            var f = LoadFixture(configure: x =>
            {
                x.NameColumn.HeaderFlyout = x.Recorder.Create("SharedOptions");
                x.OfficeColumn.HeaderFlyout = x.NameColumn.HeaderFlyout;
            });
            RunOnUIThread.Execute(() =>
            {
                var button = FindVisualChildrenByType<DropDownButton>(HeaderOf(f, OfficeCol)).Single();
                button.Flyout.ShowAt(button);
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                var inactive = FindVisualChildrenByType<DropDownButton>(HeaderOf(f, NameCol)).Single();
                var active = FindVisualChildrenByType<DropDownButton>(HeaderOf(f, OfficeCol)).Single();
                foreach (var button in new[] { inactive, active })
                {
                    var provider = (IExpandCollapseProvider)FrameworkElementAutomationPeer.CreatePeerForElement(button)
                        .GetPattern(PatternInterface.ExpandCollapse);
                    Verify.AreEqual(button == active ? ExpandCollapseState.Expanded : ExpandCollapseState.Collapsed,
                        provider.ExpandCollapseState, "Only the shared menu's actual target is expanded.");
                }
                f.NameColumn.HeaderFlyout = null;
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() => Verify.IsTrue(f.OfficeColumn.HeaderFlyout.IsOpen,
                "Detaching the same flyout from an inactive header must not dismiss its active owner."));
            CloseMenus(f);
        }

        [TestMethod]
        public void HeaderDropdownIsSuppressedThroughoutEditing()
        {
            var f = LoadEditableFixture(configure: x => x.NameColumn.HeaderFlyout = x.Recorder.Create("HeaderOptions"));
            DropDownButton button = null;
            IExpandCollapseProvider dropdown = null;
            RunOnUIThread.Execute(() =>
            {
                button = FindVisualChildrenByType<DropDownButton>(HeaderOf(f, NameCol)).Single();
                f.NameColumn.HeaderFlyout.Closing += (s, e) => e.Cancel = true;
                f.NameColumn.HeaderFlyout.ShowAt(button);
                dropdown = (IExpandCollapseProvider)FrameworkElementAutomationPeer.CreatePeerForElement(button)
                    .GetPattern(PatternInterface.ExpandCollapse);
                f.Table.CellEditEnding += (s, e) =>
                {
                    Verify.IsTrue(f.Table.IsEditing);
                    Verify.IsFalse(button.IsEnabled);
                    Verify.IsNull(button.Flyout);
                };
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(f.NameColumn.HeaderFlyout.IsOpen, "Precondition: the dropdown is open before editing.");
                f.Recorder.Clear();
            });
            RequestWhileEditing(f, () =>
            {
                Verify.IsFalse(button.IsEnabled);
                Verify.AreEqual(Visibility.Visible, button.Visibility, "Editing must not change header geometry.");
                Verify.IsNull(button.Flyout);
                dropdown.Expand();
                Verify.IsFalse(f.NameColumn.HeaderFlyout.IsOpen);
            });
            RunOnUIThread.Execute(() =>
            {
                Verify.IsFalse(f.Table.IsEditing);
                Verify.IsTrue(button.IsEnabled);
                Verify.AreEqual(f.NameColumn.HeaderFlyout, button.Flyout);
            });
            VerifyNoMenuOpened(f, "BeginningEdit and CellEditEnding");
        }

        [TestMethod]
        public void HeaderDropdownLayoutReservesSpaceAcrossDensityThemeAndRtl()
        {
            var f = LoadFixture(configure: x => x.NameColumn.HeaderFlyout = x.Recorder.Create("HeaderOptions"));
            foreach (var theme in new[] { ElementTheme.Light, ElementTheme.Dark })
            foreach (var density in new[] { TableViewDensity.ExtraCompact, TableViewDensity.Compact, TableViewDensity.Standard })
            foreach (var flow in new[] { FlowDirection.LeftToRight, FlowDirection.RightToLeft })
            {
                RunOnUIThread.Execute(() =>
                {
                    f.Table.RequestedTheme = theme;
                    f.Table.Density = density;
                    f.Table.FlowDirection = flow;
                    f.Table.UpdateLayout();
                });
                IdleSynchronizer.Wait();
                RunOnUIThread.Execute(() =>
                {
                    var header = HeaderOf(f, NameCol);
                    var button = FindVisualChildrenByType<DropDownButton>(header).Single();
                    var presenter = FindVisualChildrenByType<ContentPresenter>(header)
                        .Single(p => p.Name == "PART_ContentPresenter");
                    Verify.AreEqual(26.0, button.ActualWidth);
                    Verify.AreEqual(header.ActualHeight, button.ActualHeight);
                    var headerBounds = header.TransformToVisual(f.Root).TransformBounds(
                        new global::Windows.Foundation.Rect(0, 0, header.ActualWidth, header.ActualHeight));
                    var buttonBounds = button.TransformToVisual(f.Root).TransformBounds(
                        new global::Windows.Foundation.Rect(0, 0, button.ActualWidth, button.ActualHeight));
                    var contentBounds = presenter.TransformToVisual(f.Root).TransformBounds(
                        new global::Windows.Foundation.Rect(0, 0, presenter.ActualWidth, presenter.ActualHeight));
                    if (flow == FlowDirection.LeftToRight)
                    {
                        Verify.IsTrue(contentBounds.Right <= buttonBounds.Left + 0.1, "Content and sort reserve precede the dropdown.");
                        Verify.IsTrue(Math.Abs(buttonBounds.Right - headerBounds.Right) < 0.1);
                    }
                    else
                    {
                        Verify.IsTrue(buttonBounds.Right <= contentBounds.Left + 0.1, "The dropdown mirrors to the leading visual edge in RTL.");
                        Verify.IsTrue(Math.Abs(buttonBounds.Left - headerBounds.Left) < 0.1);
                    }
                });
            }
        }

        [TestMethod]
        public void HeaderDropdownMakesAnOtherwiseInactiveHeaderFocusable()
        {
            var f = LoadFixture(configure: x =>
            {
                x.Table.CanUserSortColumns = false;
                x.Table.CanUserResizeColumns = false;
                x.Table.ContextFlyout = null;
                x.NameColumn.HeaderContextFlyout = null;
                x.NameColumn.HeaderToolTip = null;
            });
            RunOnUIThread.Execute(() =>
            {
                var header = (TableViewColumnHeader)HeaderOf(f, NameCol);
                Verify.IsFalse(header.IsTabStop, "A header with no actions starts outside the tab order.");
                f.NameColumn.HeaderFlyout = f.Recorder.Create("HeaderOptions");
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                var header = (TableViewColumnHeader)HeaderOf(f, NameCol);
                Verify.IsTrue(header.IsTabStop, "A live header dropdown makes its header actionable.");
                Verify.IsTrue(header.Focus(FocusState.Keyboard));
                f.NameColumn.HeaderFlyout = null;
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                Verify.IsFalse(((TableViewColumnHeader)HeaderOf(f, NameCol)).IsTabStop,
                    "Removing the only header action restores its inactive-header stamp.");
            });
        }

        [TestMethod]
        public void NativeRowMenuSuppressedAfterVetoedCommit()
        {
            VerifyNativeOwnerMenuSuppressedAfterVetoedCommit(grouped: false);
        }

        [TestMethod]
        public void NativeGroupHeaderMenuSuppressedAfterVetoedCommit()
        {
            VerifyNativeOwnerMenuSuppressedAfterVetoedCommit(grouped: true);
        }

        [TestMethod]
        public void SharedRowMenuRemainsAvailableOutsideTableWhileEditing()
        {
            Button other = null;
            var f = LoadEditableFixture(configure: x =>
            {
                other = new Button { Content = "Other owner", ContextFlyout = x.RowMenu };
                x.Root.Child = null;
                x.Root.Child = new StackPanel { Children = { x.Table, other } };
            });
            bool veto = true;
            RunOnUIThread.Execute(() =>
            {
                f.Table.CellEditEnding += (s, e) => e.Cancel = veto;
                var provider = GetCellPeer(f.Table, f.P(3), NameCol).GetPattern(PatternInterface.Value) as IValueProvider;
                Verify.IsNotNull(provider);
                Verify.Throws<Exception>(() => provider.SetValue("Uncommitted"));
            });

            RequestOnElement(() => other);

            VerifyOpened(f, "RowMenu", () => other, expectedItem: null, "a shared flyout on an unrelated control");
            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(f.Table.IsEditing, "An unrelated menu must not discard the vetoed edit.");
                veto = false;
                f.Table.CancelEdit();
            });
            CloseMenus(f);
        }

        private void VerifyNativeOwnerMenuSuppressedAfterVetoedCommit(bool grouped)
        {
            var f = LoadEditableFixture(grouped: grouped);
            bool veto = true;
            FlyoutBase menu = null;
            int opened = 0;
            RunOnUIThread.Execute(() =>
            {
                f.Table.CellEditEnding += (s, e) => e.Cancel = veto;
                var provider = GetCellPeer(f.Table, f.P(3), NameCol).GetPattern(PatternInterface.Value) as IValueProvider;
                Verify.IsNotNull(provider);
                Verify.Throws<Exception>(() => provider.SetValue("Uncommitted"));
                Verify.IsTrue(f.Table.IsEditing, "Precondition: the vetoed commit leaves an edit open.");

                FrameworkElement owner = grouped ? (FrameworkElement)GetGroupHeader(f.Table, 0) : RowOf(f, 6);
                menu = new MenuFlyout { Items = { new MenuFlyoutItem { Text = "Native menu" } } };
                menu.Opened += (s, e) => opened++;
                menu.Closing += (s, e) => e.Cancel = true;
                owner.ContextFlyout = menu;
                Verify.IsTrue(owner.Focus(FocusState.Programmatic), "Precondition: focus leaves the editor.");
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                FrameworkElement owner = grouped ? (FrameworkElement)GetGroupHeader(f.Table, 0) : RowOf(f, 6);
                // Bypass the custom peer's guard: keyboard requests originate directly on the owner.
                new FrameworkElementAutomationPeer(owner).ShowContextMenu();
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                Verify.IsFalse(menu.IsOpen, "An owner-originated native request must not open a menu while editing.");
                Verify.AreEqual(0, opened, "Suppression must cancel opening before the popup becomes visible.");
                Verify.IsTrue(f.Table.IsEditing, "Suppressing the menu must preserve the uncommitted edit.");
                veto = false;
                f.Table.CancelEdit();
            });
            CloseMenus(f);
        }

        private ContextMenuFixture LoadEditableFixture(bool grouped = false, Action<ContextMenuFixture> configure = null) => LoadFixture(grouped: grouped, configure: x =>
        {
            x.Table.IsReadOnly = false;
            ((TableViewTextColumn)x.NameColumn).Binding = new Binding { Path = new PropertyPath("Name"), Mode = BindingMode.TwoWay };
            configure?.Invoke(x);
        });

        // Raises the request from BeginningEdit, where IsEditing is already true, so the edit does not
        // have to be held open past the SetValue call.
        internal static void RequestWhileEditing(ContextMenuFixture f, Action request)
        {
            bool? wasEditing = null;
            RunOnUIThread.Execute(() =>
            {
                f.Table.BeginningEdit += (s, e) =>
                {
                    if (wasEditing.HasValue)
                    {
                        return;
                    }

                    wasEditing = f.Table.IsEditing;
                    request();
                };

                var provider = GetCellPeer(f.Table, f.P(3), NameCol).GetPattern(PatternInterface.Value) as IValueProvider;
                Verify.IsNotNull(provider, "Precondition: the Name cell must be editable.");
                provider.SetValue("Edited");
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() => Verify.IsTrue(wasEditing == true, "Precondition: IsEditing must be true inside BeginningEdit."));
        }

        internal static void VerifyNoMenuOpened(ContextMenuFixture f, string when)
        {
            RunOnUIThread.Execute(() => Verify.AreEqual(0, f.Recorder.Opened.Count,
                $"No TableView menu may open while a cell is editing ({when}). Opened: {f.Recorder.Describe()}"));
            CloseMenus(f);
        }

        // ---- F. DataContext, sharing and recycling ------------------------------------------

        [TestMethod]
        [TestProperty("Description", "One shared cell menu sees the requested row's item every time.")]
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
        [TestProperty("Description", "A recycled row container's menu sees its new item.")]
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
        [TestProperty("Description", "A ContextFlyout on the row template root opens with the row's item.")]
        public void RowTemplateMenuSmoke()
        {
            var f = LoadFixture(configure: x =>
            {
                x.Table.Resources[typeof(TableViewRow)] = (Style)XamlReader.Load(RowTemplateStyleXaml);
            });
            FrameworkElement templateRoot = null;
            RunOnUIThread.Execute(() => templateRoot = TrackNamedMenu(f, RowOf(f, 3), "TemplateRoot", "RowTemplateMenu"));

            RequestOnCell(f, 3, OfficeCol);

            VerifyOpened(f, "RowTemplateMenu", () => templateRoot, f.P(3), "UIA on row 3 / Office");
            CloseMenus(f);
        }

        [TestMethod]
        public void RowPeerResolvesTemplateMenusBeforeRowMenu()
        {
            var f = LoadFixture(configure: x =>
                x.Table.Resources[typeof(TableViewRow)] = (Style)XamlReader.Load(RowTemplateStyleXaml));
            FrameworkElement root = null;
            RunOnUIThread.Execute(() =>
            {
                var row = RowOf(f, 3);
                row.ContextFlyout = f.RowMenu;
                root = TrackNamedMenu(f, row, "TemplateRoot", "RowTemplateMenu");
            });

            RequestOnRow(f, 3);

            VerifyOpened(f, "RowTemplateMenu", () => root, f.P(3), "UIA on the row rather than a cell");
            CloseMenus(f);

            FrameworkElement host = null;
            RunOnUIThread.Execute(() =>
            {
                host = FindNamed(RowOf(f, 3), "PART_CellsHost");
                host.ContextFlyout = f.Recorder.Create("CellsHostMenu");
            });
            RequestOnRow(f, 3);
            VerifyOpened(f, "CellsHostMenu", () => host, f.P(3), "cells-host menu before template-root and row menus");
            CloseMenus(f);
        }

        [TestMethod]
        public void OpenGroupMenuKeepsOriginalGroupAfterHeaderRecycling()
        {
            var f = LoadFixture(grouped: true, itemCount: 200);
            TableViewGroupHeader header = null;
            TableViewGroupInfo original = null;
            object originalKey = null;
            MenuFlyout menu = null;
            RunOnUIThread.Execute(() =>
            {
                header = GetGroupHeader(f.Table, 0);
                original = (TableViewGroupInfo)header.Content;
                originalKey = original.Key;
                menu = f.Recorder.Create("GroupMenu");
                header.ContextFlyout = menu;
                GetGroupHeaderPeer(f.Table, 0).ShowContextMenu();
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() => Verify.AreSame(original, menu.Items[0].DataContext,
                "Precondition: the open menu holds the original group projection."));

            bool recycled = false;
            for (double offset = 2000; offset <= 12000 && !recycled; offset += 2000)
            {
                ScrollBodyTo(f.Table, ScrollAxis.Vertical, offset);
                RunOnUIThread.Execute(() =>
                {
                    var current = header.Content as TableViewGroupInfo;
                    recycled = current != null && !Equals(current.Key, originalKey);
                });
            }

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(recycled, "Precondition: the original header must be reused for another group.");
                Verify.AreEqual(originalKey, original.Key, "A retained projection must never change group identity.");
                if (menu.IsOpen)
                {
                    Verify.AreSame(original, menu.Items[0].DataContext,
                        "Commands in an open menu must still receive the originally requested group.");
                }
            });
            CloseMenus(f);
        }

        // ---- G. Robustness ------------------------------------------------------------------

        [TestMethod]
        [TestProperty("Description", "An item removed while focus moves to the target row suppresses the menu.")]
        public void ItemRemovedDuringFocusMoveSuppressesMenu()
        {
            var f = LoadFixture();
            FocusRow(f, 1);
            Person target = null;
            RunOnUIThread.Execute(() => target = f.P(5));
            var focusMoves = OnFirstFocusInto(f, 5, () => f.Items.Remove(target));

            RequestOnCell(f, 5, NameCol);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(1, focusMoves(), "Precondition: the request must move focus into row 5 exactly once.");
                Verify.AreEqual(0, f.Recorder.Opened.Count, $"No menu may open for a removed item. Opened: {f.Recorder.Describe()}");
            });
            CloseMenus(f);
        }

        [TestMethod]
        [TestProperty("Description", "A column removed while focus moves to the target row is not a target.")]
        public void ColumnRemovedDuringFocusMoveSuppressesMenu()
        {
            var f = LoadFixture();
            FocusRow(f, 1);
            var focusMoves = OnFirstFocusInto(f, 5, () => f.Table.Columns.Remove(f.NameColumn));

            RequestOnCell(f, 5, NameCol);

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(1, focusMoves(), "Precondition: the request must move focus into row 5 exactly once.");
                Verify.AreEqual(0, f.Recorder.Opened.Count,
                    $"A request whose column was removed must be suppressed, not passed to another menu. Opened: {f.Recorder.Describe()}");
            });
            CloseMenus(f);
        }

        [TestMethod]
        [TestProperty("Description", "A CellContextFlyout replaced while focus moves to the target row is the menu that opens.")]
        public void CellMenuReplacedDuringFocusMoveShowsNewMenu()
        {
            MenuFlyout replacement = null;
            var f = LoadFixture();
            FocusRow(f, 1);
            RunOnUIThread.Execute(() => replacement = f.Recorder.Create("ReplacementMenu"));
            var focusMoves = OnFirstFocusInto(f, 5, () => f.NameColumn.CellContextFlyout = replacement);

            RequestOnCell(f, 5, NameCol);

            RunOnUIThread.Execute(() => Verify.IsTrue(focusMoves() >= 1, "Precondition: the request must move focus into row 5."));
            VerifyOpened(f, "ReplacementMenu", () => CellOf(f, 5, NameCol), f.P(5), "UIA on row 5 / Name");
            CloseMenus(f);
        }

        [TestMethod]
        [TestProperty("Description", "A row or cell peer whose container went back to the pool raises ElementNotAvailable from ShowContextMenu and opens no menu.")]
        public void PooledRowPeerShowContextMenuOpensNoMenu()
        {
            var f = LoadFixture();
            AutomationPeer rowPeer = null;
            AutomationPeer cellPeer = null;
            RunOnUIThread.Execute(() =>
            {
                rowPeer = GetRowPeer(RowOf(f, 5));
                cellPeer = GetCellPeer(f.Table, f.P(5), OfficeCol);
                f.Items.Clear();
            });
            SettleLayout(f.Table);

            foreach (var (peer, name) in new[] { (rowPeer, "row"), (cellPeer, "cell") })
            {
                RunOnUIThread.Execute(() => Verify.Throws<Exception>(() => peer.ShowContextMenu()));
                IdleSynchronizer.Wait();
                RunOnUIThread.Execute(() => Verify.AreEqual(0, f.Recorder.Opened.Count,
                    $"No menu may open from the pooled {name} peer. Opened: {f.Recorder.Describe()}"));
            }
            CloseMenus(f);
        }

        [TestMethod]
        public void PooledGroupHeaderPeerShowContextMenuOpensNoMenu()
        {
            var f = LoadFixture(grouped: true);
            AutomationPeer peer = null;
            RunOnUIThread.Execute(() =>
            {
                var header = GetGroupHeader(f.Table, 0);
                header.ContextFlyout = f.Recorder.Create("GroupMenu");
                peer = GetGroupHeaderPeer(f.Table, 0);
                f.Items.Clear();
            });
            SettleLayout(f.Table);
            RunOnUIThread.Execute(() => Verify.Throws<Exception>(() => peer.ShowContextMenu()));
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() => Verify.AreEqual(0, f.Recorder.Opened.Count,
                "A recycled group-header peer must not open either its menu or the table fallback."));
            CloseMenus(f);
        }

        [TestMethod]
        [TestProperty("Description", "A TableViewColumn in a content element's Tag does not redirect the request to that column.")]
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

            VerifyOpened(f, "RowMenu", () => RowOf(f, 3), f.P(3), "UIA on Department content tagged with the Name column");
            CloseMenus(f);
        }

        // ---- fixture loading ----------------------------------------------------------------

        private ContextMenuFixture LoadFixture(
            Action<ContextMenuFixture> configure = null,
            DataTemplate nameTemplate = null,
            DataTemplate departmentTemplate = null,
            bool grouped = false,
            int itemCount = 10)
        {
            ContextMenuFixture f = null;
            RunOnUIThread.Execute(() =>
            {
                f = CreateContextMenuFixture(nameTemplate, departmentTemplate, grouped, itemCount);
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
    // Office (text). RowMenu is set on every row by an implicit row style and TableMenu is the TableView.ContextFlyout.
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

        // 1-based: P(1) is "Person 0", the first row.
        public Person P(int n) => Items.First(p => p.Name == $"Person {n - 1}");
    }

    internal static class ContextMenuTestHelpers
    {
        internal const int NameCol = 0;
        internal const int DepartmentCol = 1;
        internal const int OfficeCol = 2;
        internal const string ContentRootName = "ContentRoot";
        internal const string ContentButtonName = "ContentButton";

        internal const string XamlNamespaces =
            "xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' " +
            "xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' " +
            "xmlns:tv='using:Microsoft.UI.Xaml.Controls.Tabular'";

        internal const string RootFlyoutContent =
            "<Grid x:Name='ContentRoot' Background='Transparent'>" +
            "  <Grid.ContextFlyout><MenuFlyout/></Grid.ContextFlyout>" +
            "  <TextBlock Text='{Binding Role}'/>" +
            "</Grid>";

        // A root flyout around a Button, so a request can start on a control inside the template.
        internal static string ButtonFlyoutContent(string contentBinding) =>
            "<Grid x:Name='ContentRoot' Background='Transparent'>" +
            "  <Grid.ContextFlyout><MenuFlyout/></Grid.ContextFlyout>" +
            "  <Button x:Name='ContentButton' Content='" + contentBinding + "'/>" +
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

            f.NameColumn.CellContextFlyout = f.CellMenu;
            f.NameColumn.HeaderContextFlyout = f.HeaderMenu;
            f.Table.ContextFlyout = f.TableMenu;
            f.Table.Resources[typeof(TableViewRow)] = new Style(typeof(TableViewRow))
            {
                Setters = { new Setter(UIElement.ContextFlyoutProperty, f.RowMenu) },
            };

            f.Root = new Border { Child = f.Table, Background = new SolidColorBrush(Microsoft.UI.Colors.Transparent) };
            return f;
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

        // Raises the request on an arbitrary element, e.g. a control inside a cell or header template.
        internal static void RequestOnElement(Func<FrameworkElement> element)
        {
            RunOnUIThread.Execute(() =>
            {
                var peer = FrameworkElementAutomationPeer.CreatePeerForElement(element());
                Verify.IsNotNull(peer, "The element must produce an automation peer.");
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

        // Runs action on the UI thread the first time focus moves into the row. Returns a counter of
        // focus moves into the row, read on the UI thread.
        internal static Func<int> OnFirstFocusInto(ContextMenuFixture f, int row, Action action)
        {
            int count = 0;
            RunOnUIThread.Execute(() => RowOf(f, row).GettingFocus += (s, e) =>
            {
                if (count++ == 0)
                {
                    action();
                }
            });
            return () => count;
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

        // Pass as expectedItem to assert the menu's items see no DataContext; null skips the check.
        internal static readonly object NoItem = new object();

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

                if (expectedItem == NoItem)
                {
                    Verify.IsNull(entry.ItemsSee, $"Menu items must see no DataContext ({context}); saw {entry.ItemsSee}.");
                }
                else if (expectedItem != null)
                {
                    Verify.AreSame(expectedItem, entry.ItemsSee, $"Menu items' DataContext ({context}): saw {entry.ItemsSee}.");
                }
            });
        }

        internal static void VerifyMenuPlacedAtTarget(ContextMenuFixture f, Func<FrameworkElement> getTarget)
        {
            RunOnUIThread.Execute(() =>
            {
                var target = getTarget();
                var presenter = VisualTreeHelper.GetOpenPopupsForXamlRoot(f.Table.XamlRoot)
                    .Select(p => p.Child)
                    .OfType<MenuFlyoutPresenter>()
                    .Single();
                var cellBounds = target.TransformToVisual(null).TransformBounds(
                    new global::Windows.Foundation.Rect(0, 0, target.ActualWidth, target.ActualHeight));
                var menuBounds = presenter.TransformToVisual(null).TransformBounds(
                    new global::Windows.Foundation.Rect(0, 0, presenter.ActualWidth, presenter.ActualHeight));
                const double tolerance = 16;
                Verify.IsTrue(Math.Abs(menuBounds.Left - cellBounds.Left) <= tolerance ||
                    Math.Abs(menuBounds.Right - cellBounds.Right) <= tolerance,
                    "The table fallback must align horizontally with the requested cell.");
                Verify.IsTrue(Math.Abs(menuBounds.Top - cellBounds.Bottom) <= tolerance ||
                    Math.Abs(menuBounds.Bottom - cellBounds.Top) <= tolerance,
                    "The table fallback must open beside the requested cell, not the whole table.");
            });
        }
    }
}
