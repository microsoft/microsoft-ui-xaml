// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Runtime.InteropServices;
using Common;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Automation;
using Microsoft.UI.Xaml.Automation.Peers;
using Microsoft.UI.Xaml.Automation.Provider;
using Microsoft.UI.Xaml.Controls.Tabular;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Media;
using MUXControlsTestApp.Utilities;
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using Windows.System;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    [TestClass]
    public partial class TableViewAutomationTests : ApiTestBase
    {
        [TestMethod]
        public void HeaderProvidersBelongToTheActualVisual()
        {
            var table = CreateLoadedTable();
            RunOnUIThread.Execute(() =>
            {
                var peer = FrameworkElementAutomationPeer.CreatePeerForElement(table);
                var headers = ((ITableProvider)peer.GetPattern(PatternInterface.Table)).GetColumnHeaders();
                Verify.AreEqual(2, headers.Length);

                var visuals = Descendants(table).Where(e => e.Tag is TableViewColumn &&
                    FrameworkElementAutomationPeer.CreatePeerForElement(e) is TableViewColumnHeaderAutomationPeer).ToArray();
                Verify.AreEqual(2, visuals.Length);
                var first = FrameworkElementAutomationPeer.CreatePeerForElement(visuals[0]);
                var treeHeaders = PeerDescendants(peer).OfType<TableViewColumnHeaderAutomationPeer>().ToArray();
                Verify.AreEqual(2, treeHeaders.Length);
                Verify.IsTrue(first == treeHeaders[0]);
                Verify.AreEqual(AutomationControlType.HeaderItem, first.GetAutomationControlType());
                Verify.AreEqual("Name", first.GetName());
                Verify.AreEqual(1, first.GetPositionInSet());
                Verify.AreEqual(2, first.GetSizeOfSet());
                Verify.IsTrue(first == PeerAccess.FromProvider(headers[0]));
                Verify.IsNotNull(first.GetParent());

                var cell = PeerAccess.FromProvider(((IGridProvider)peer.GetPattern(PatternInterface.Grid)).GetItem(0, 0));
                var cellHeaders = ((ITableItemProvider)cell.GetPattern(PatternInterface.TableItem)).GetColumnHeaderItems();
                Verify.AreEqual(1, cellHeaders.Length);
                Verify.IsTrue(first == PeerAccess.FromProvider(cellHeaders[0]));
                Verify.IsTrue(first == PeerAccess.FromProvider(
                    ((ITableProvider)peer.GetPattern(PatternInterface.Table)).GetColumnHeaders()[0]));

                AutomationProperties.SetName(visuals[0], "Override");
                AutomationProperties.SetPositionInSet(visuals[0], 7);
                AutomationProperties.SetSizeOfSet(visuals[0], 9);
                Verify.AreEqual("Override", first.GetName());
                Verify.AreEqual(7, first.GetPositionInSet());
                Verify.AreEqual(9, first.GetSizeOfSet());
                first.SetFocus();
                Verify.IsTrue(first.HasKeyboardFocus());
                Verify.IsTrue(FocusManager.GetFocusedElement(table.XamlRoot) == visuals[0]);
                ((IInvokeProvider)first.GetPattern(PatternInterface.Invoke)).Invoke();
                Verify.AreEqual(SortDirection.Ascending, table.Columns[0].SortDirection);
            });
        }

        [TestMethod]
        public void GridFirstCellHasTheSameParentAndIdentityAsTreeCell()
        {
            var table = CreateLoadedTable();
            RunOnUIThread.Execute(() =>
            {
                var tablePeer = FrameworkElementAutomationPeer.CreatePeerForElement(table);
                var gridCell = PeerAccess.FromProvider(((IGridProvider)tablePeer.GetPattern(PatternInterface.Grid)).GetItem(0, 0));
                Verify.IsNotNull(gridCell);
                var row = Descendants(table).OfType<TableViewRow>().First();
                var rowPeer = FrameworkElementAutomationPeer.CreatePeerForElement(row);
                Verify.IsTrue(gridCell.GetParent() == rowPeer, "Grid-first acquisition must establish the row parent.");
                Verify.IsTrue(rowPeer.GetChildren()[0] == gridCell);
                ((IValueProvider)gridCell.GetPattern(PatternInterface.Value)).SetValue("Alice");
                Verify.AreEqual("Alice", ((IValueProvider)gridCell.GetPattern(PatternInterface.Value)).Value);
                Verify.IsFalse(table.IsEditing);
                Verify.IsTrue(PeerAccess.FromProvider(((IGridProvider)tablePeer.GetPattern(PatternInterface.Grid)).GetItem(0, 0)) == gridCell);
            });
        }

        [TestMethod]
        public void ExplicitTemplateBindingKeepsSourceUnchangedUntilCommit()
        {
            Item item = null;
            Microsoft.UI.Xaml.Controls.TextBox editor = null;
            Microsoft.UI.Xaml.Controls.Button other = null;
            RunOnUIThread.Execute(() =>
            {
                item = new Item { Name = "Before" };
                var template = (DataTemplate)Microsoft.UI.Xaml.Markup.XamlReader.Load(
                    "<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>" +
                    "<TextBox Text='{Binding Name, Mode=TwoWay, UpdateSourceTrigger=Explicit}'/>" +
                    "</DataTemplate>");
                editor = (Microsoft.UI.Xaml.Controls.TextBox)template.LoadContent();
                editor.DataContext = item;
                other = new Microsoft.UI.Xaml.Controls.Button { Content = "Other" };
                var host = new Microsoft.UI.Xaml.Controls.StackPanel();
                host.Children.Add(editor);
                host.Children.Add(other);
                Content = host;
                host.UpdateLayout();
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(editor.Focus(FocusState.Programmatic));
                editor.Text = "Pending";
                Verify.AreEqual("Before", item.Name);
                Verify.IsTrue(other.Focus(FocusState.Programmatic));
                Verify.AreEqual("Before", item.Name, "Focus restoration must not write an uncommitted template value.");
                editor.GetBindingExpression(Microsoft.UI.Xaml.Controls.TextBox.TextProperty).UpdateSource();
                Verify.AreEqual("Pending", item.Name);
            });
        }

        [TestMethod]
        public void RemovedItemIsCollectibleWithPooledRowAndRetainedCellPeer()
        {
            TableView table = null;
            TableViewRow retainedRow = null;
            FrameworkElementAutomationPeer retainedPeer = null;
            ObservableCollection<Item> items = null;
            WeakReference removedItem = null;
            RunOnUIThread.Execute(() =>
            {
                table = CreateTable();
                items = new ObservableCollection<Item> { new Item { Name = "Remove me" } };
                removedItem = new WeakReference(items[0]);
                table.ItemsSource = items;
                Content = table;
                table.UpdateLayout();
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                retainedRow = Descendants(table).OfType<TableViewRow>().First();
                var host = Descendants(retainedRow).OfType<TableViewCellsPanel>().Single();
                retainedPeer = (FrameworkElementAutomationPeer)
                    FrameworkElementAutomationPeer.CreatePeerForElement(host.Children[0]);
                Verify.AreEqual("Name, Remove me", retainedPeer.GetName());
                Verify.IsFalse(table.IsEditing);
                items.Clear();
                table.UpdateLayout();
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                Verify.IsFalse(table.IsEditing);
                Verify.IsNull(retainedRow.DataContext, "The recycled container must release its inherited item.");
            });
            for (int i = 0; i < 5 && removedItem.IsAlive; ++i)
            {
                GC.Collect();
                GC.WaitForPendingFinalizers();
                IdleSynchronizer.Wait();
            }
            Verify.IsFalse(removedItem.IsAlive,
                "A retained cell peer must not pin a removed item through semantic-name state.");
            GC.KeepAlive(retainedPeer);
            GC.KeepAlive(retainedRow);
            GC.KeepAlive(table);
            GC.KeepAlive(items);
        }

        [TestMethod]
        public void ExistingPeerLookupDoesNotCreateBoundCellPeer()
        {
            RunOnUIThread.Execute(() =>
            {
                var table = CreateTable();
                Content = table;
                table.UpdateLayout();
                var row = Descendants(table).OfType<TableViewRow>().First();
                var cell = Descendants(row).OfType<TableViewCellsPanel>().Single().Children[0];
                for (int i = 0; i < 3; ++i)
                {
                    Verify.IsFalse(TableViewPeerTestAccess.HasExistingPeer(cell));
                }
            });
        }

        [TestMethod]
        public void DirectVisualCellPeerSupportsEditWithoutRowCacheAcquisition()
        {
            TableView table = null;
            SnapshotTrackingColumn column = null;
            RunOnUIThread.Execute(() =>
            {
                table = CreateTable();
                column = new SnapshotTrackingColumn
                {
                    Header = "Name", Width = new GridLength(200),
                    Binding = new Binding { Path = new PropertyPath("Name") }
                };
                table.Columns[0] = column;
                Content = table;
                table.UpdateLayout();
            });
            IdleSynchronizer.Wait();
            FrameworkElementAutomationPeer peer = null;
            UIElement cell = null;
            RunOnUIThread.Execute(() =>
            {
                var row = Descendants(table).OfType<TableViewRow>().First();
                var cellsHost = Descendants(row).OfType<TableViewCellsPanel>().Single();
                cell = cellsHost.Children[0];
                Verify.IsFalse(TableViewPeerTestAccess.HasExistingPeer(cell),
                    "No row or grid route may acquire the cell before direct visual discovery.");
                peer = (FrameworkElementAutomationPeer)FrameworkElementAutomationPeer.CreatePeerForElement(cell);
                Verify.IsTrue(peer is TableViewCellAutomationPeer);
                var value = (IValueProvider)peer.GetPattern(PatternInterface.Value);
                Verify.AreEqual("Al", value.Value);
                var originalLabel = (SnapshotLabel)AutomationProperties.GetLabeledBy(VisualTreeHelper.GetChild(cell, 0));
                var readsBefore = originalLabel.Reads;
                value.SetValue("Alice");
                Verify.IsTrue(originalLabel.Reads > readsBefore,
                    "The direct visual peer must capture the old display name before replacing it.");
                Verify.AreEqual("Alice", value.Value);
                Verify.IsFalse(table.IsEditing);
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(FrameworkElementAutomationPeer.FromElement(cell) == peer);
                var value = (IValueProvider)peer.GetPattern(PatternInterface.Value);
                value.SetValue("Alice");
                value.SetValue("Bob");
                Verify.AreEqual("Bob", value.Value);
                Verify.AreEqual("Name, Stable label", peer.GetName());
                Verify.IsFalse(table.IsEditing);
            });
        }

        [TestMethod]
        public void VetoedEditKeepsSemanticNameUntilCancellation()
        {
            var table = CreateLoadedTable();
            AutomationPeer cell = null;
            string name = null;
            RunOnUIThread.Execute(() =>
            {
                table.CellEditEnding += (_, args) =>
                {
                    if (args.EditAction == TableViewEditAction.Commit) args.Cancel = true;
                };
                var peer = FrameworkElementAutomationPeer.CreatePeerForElement(table);
                cell = PeerAccess.FromProvider(((IGridProvider)peer.GetPattern(PatternInterface.Grid)).GetItem(0, 0));
                name = cell.GetName();
                bool rejected = false;
                try { ((IValueProvider)cell.GetPattern(PatternInterface.Value)).SetValue("Rejected"); }
                catch (COMException error) { rejected = error.HResult == unchecked((int)0x80004005); }
                Verify.IsTrue(rejected);
                Verify.IsTrue(table.IsEditing);
                Verify.AreEqual(name, cell.GetName());
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(name, cell.GetName());
                table.CancelEdit();
                Verify.IsFalse(table.IsEditing);
                Verify.AreEqual("Al", ((IValueProvider)cell.GetPattern(PatternInterface.Value)).Value);
                Verify.AreEqual(name, cell.GetName());
            });
        }

        [TestMethod]
        public void ChildParentQueryPreservesRetainedCellOwner()
        {
            var table = CreateLoadedTable();
            FrameworkElementAutomationPeer cell = null;
            Microsoft.UI.Xaml.Controls.Grid visual = null;
            AutomationPeer rowPeer = null;
            RunOnUIThread.Execute(() =>
            {
                var tablePeer = FrameworkElementAutomationPeer.CreatePeerForElement(table);
                cell = (FrameworkElementAutomationPeer)PeerAccess.FromProvider(
                    ((IGridProvider)tablePeer.GetPattern(PatternInterface.Grid)).GetItem(0, 0));
                visual = (Microsoft.UI.Xaml.Controls.Grid)cell.Owner;
                rowPeer = cell.GetParent();
                Verify.IsTrue(cell == FrameworkElementAutomationPeer.FromElement(visual));
                var bounds = cell.GetBoundingRectangle();
                var child = (FrameworkElement)visual.Children[0];
                var childPeer = FrameworkElementAutomationPeer.CreatePeerForElement(child);
                Verify.IsTrue(childPeer.GetParent() == cell);
                Verify.IsTrue(cell.Owner == visual);
                Verify.IsTrue(cell.GetParent() == rowPeer);
                Verify.IsTrue(cell.IsEnabled());
                Verify.IsFalse(cell.IsOffscreen());
                Verify.AreEqual(bounds, cell.GetBoundingRectangle());
                ((IValueProvider)cell.GetPattern(PatternInterface.Value)).SetValue("Alice");
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(cell.Owner == visual);
                Verify.IsTrue(FrameworkElementAutomationPeer.FromElement(visual) == cell);
                Verify.IsTrue(cell.GetParent() == rowPeer);
                Verify.IsTrue(cell.IsEnabled());
                var value = (IValueProvider)cell.GetPattern(PatternInterface.Value);
                Verify.AreEqual("Alice", value.Value);
                value.SetValue("Alice");
                Verify.AreEqual("Alice", value.Value);
                Verify.IsFalse(table.IsEditing);
            });
        }

        [TestMethod]
        public void HeaderVisibilityAndRebuildDoNotReuseStalePeers()
        {
            var table = CreateLoadedTable();
            RunOnUIThread.Execute(() =>
            {
                var provider = (ITableProvider)FrameworkElementAutomationPeer.CreatePeerForElement(table).GetPattern(PatternInterface.Table);
                var original = PeerAccess.FromProvider(provider.GetColumnHeaders()[0]);
                var originalVisual = ((FrameworkElementAutomationPeer)original).Owner;
                table.Columns[1].Visibility = Visibility.Collapsed;
                table.UpdateLayout();
                Verify.AreEqual(1, provider.GetColumnHeaders().Length);
                Verify.AreEqual(1, PeerAccess.FromProvider(provider.GetColumnHeaders()[0]).GetSizeOfSet());
                table.Columns[1].Visibility = Visibility.Visible;
                table.UpdateLayout();
                Verify.AreEqual(2, provider.GetColumnHeaders().Length);

                var column = table.Columns[0];
                table.Columns.RemoveAt(0);
                table.Columns.Add(column);
                table.UpdateLayout();
                var rebuilt = PeerAccess.FromProvider(provider.GetColumnHeaders()[1]);
                Verify.IsFalse(original == rebuilt);
                Verify.AreEqual("Name", rebuilt.GetName());
                Verify.AreEqual(2, rebuilt.GetPositionInSet());
                Verify.IsNull(original.GetPattern(PatternInterface.Invoke));
                bool unavailable = false;
                try { ((IInvokeProvider)original).Invoke(); }
                catch (COMException error) { unavailable = error.HResult == unchecked((int)0x80040201); }
                Verify.IsTrue(unavailable, "A retained header must not sort a replacement visual.");
                GC.KeepAlive(originalVisual);

                table.HeadersVisibility = TableViewHeadersVisibility.None;
                table.UpdateLayout();
                Verify.AreEqual("Name", PeerAccess.FromProvider(provider.GetColumnHeaders()[1]).GetName());
                Content = null;
            });
        }

        [TestMethod]
        public void HeaderOverridesAndDisabledInvokeUseTheVisualOwner()
        {
            var table = CreateLoadedTable();
            RunOnUIThread.Execute(() =>
            {
                var peer = FrameworkElementAutomationPeer.CreatePeerForElement(table);
                var provider = (ITableProvider)peer.GetPattern(PatternInterface.Table);
                var header = (TableViewColumnHeaderAutomationPeer)PeerAccess.FromProvider(provider.GetColumnHeaders()[0]);
                var visual = header.Owner;
                var label = new Microsoft.UI.Xaml.Controls.TextBlock { Text = "Column label" };
                AutomationProperties.SetName(visual, "");
                AutomationProperties.SetLabeledBy(visual, label);
                AutomationProperties.SetAutomationId(visual, "AppColumnId");
                Verify.AreEqual("Column label", header.GetName());
                Verify.AreEqual("AppColumnId", header.GetAutomationId());
                AutomationProperties.SetName(visual, "Explicit name");
                Verify.AreEqual("Explicit name", header.GetName());
                var invoke = (IInvokeProvider)header.GetPattern(PatternInterface.Invoke);
                Verify.IsNotNull(invoke);
                Verify.IsTrue(header.IsEnabled());
                table.IsEnabled = false;
                Verify.IsFalse(header.IsEnabled());
                bool disabled = false;
                try { invoke.Invoke(); }
                catch (COMException error) { disabled = error.HResult == unchecked((int)0x80040200); }
                Verify.IsTrue(disabled);
                Verify.AreEqual(SortDirection.None, table.Columns[0].SortDirection);
                table.IsEnabled = true;
                Verify.IsTrue(header.IsEnabled());
                invoke.Invoke();
                Verify.AreEqual(SortDirection.Ascending, table.Columns[0].SortDirection);
            });
        }

        [TestMethod]
        public void HeaderDisabledStateIncludesDisabledAncestors()
        {
            TableView table = null;
            Microsoft.UI.Xaml.Controls.ContentControl host = null;
            RunOnUIThread.Execute(() =>
            {
                table = CreateTable();
                host = new Microsoft.UI.Xaml.Controls.ContentControl { Content = table };
                Content = host;
                table.UpdateLayout();
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                var tablePeer = FrameworkElementAutomationPeer.CreatePeerForElement(table);
                var header = PeerAccess.FromProvider(
                    ((ITableProvider)tablePeer.GetPattern(PatternInterface.Table)).GetColumnHeaders()[0]);
                var invoke = (IInvokeProvider)header.GetPattern(PatternInterface.Invoke);
                Verify.IsTrue(table.IsEnabled);
                Verify.IsTrue(header.IsEnabled());
                host.IsEnabled = false;
                Verify.IsFalse(table.IsEnabled, "The table's effective state includes its ancestor.");
                Verify.IsFalse(header.IsEnabled());
                bool disabled = false;
                try { invoke.Invoke(); }
                catch (COMException error) { disabled = error.HResult == unchecked((int)0x80040200); }
                Verify.IsTrue(disabled);
                Verify.AreEqual(SortDirection.None, table.Columns[0].SortDirection);
                host.IsEnabled = true;
                Verify.IsTrue(table.IsEnabled);
                Verify.IsTrue(header.IsEnabled());
                invoke.Invoke();
                Verify.AreEqual(SortDirection.Ascending, table.Columns[0].SortDirection);
            });
        }

        [TestMethod]
        public void DisabledHeaderTemplateControlBlocksRetainedInvoke()
        {
            var table = CreateLoadedTable();
            RunOnUIThread.Execute(() =>
            {
                var tablePeer = FrameworkElementAutomationPeer.CreatePeerForElement(table);
                var header = PeerAccess.FromProvider(
                    ((ITableProvider)tablePeer.GetPattern(PatternInterface.Table)).GetColumnHeaders()[0]);
                var invoke = (IInvokeProvider)header.GetPattern(PatternInterface.Invoke);
                var scroller = Descendants(table).OfType<Microsoft.UI.Xaml.Controls.ScrollViewer>()
                    .Single(element => element.Name == "PART_HeaderScroller");
                scroller.IsEnabled = false;
                Verify.IsTrue(table.IsEnabled, "Only the header template subtree is disabled.");
                Verify.IsFalse(header.IsEnabled());
                bool disabled = false;
                try { invoke.Invoke(); }
                catch (COMException error) { disabled = error.HResult == unchecked((int)0x80040200); }
                Verify.IsTrue(disabled);
                Verify.AreEqual(SortDirection.None, table.Columns[0].SortDirection);
                scroller.IsEnabled = true;
                Verify.IsTrue(header.IsEnabled());
                invoke.Invoke();
                Verify.AreEqual(SortDirection.Ascending, table.Columns[0].SortDirection);
            });
        }

        [TestMethod]
        public void RetainedCellHasNoHeaderForCollapsedOrRemovedColumn()
        {
            var table = CreateLoadedTable();
            RunOnUIThread.Execute(() =>
            {
                var tablePeer = FrameworkElementAutomationPeer.CreatePeerForElement(table);
                var cell = (FrameworkElementAutomationPeer)PeerAccess.FromProvider(
                    ((IGridProvider)tablePeer.GetPattern(PatternInterface.Grid)).GetItem(0, 0));
                var visual = cell.Owner;
                var row = Descendants(table).OfType<TableViewRow>().First();
                var provider = (ITableItemProvider)cell.GetPattern(PatternInterface.TableItem);
                Verify.AreEqual(1, provider.GetColumnHeaderItems().Length);
                table.Columns[0].Visibility = Visibility.Collapsed;
                Verify.AreEqual(0, provider.GetColumnHeaderItems().Length);
                table.Columns.RemoveAt(0);
                Verify.AreEqual(0, provider.GetColumnHeaderItems().Length);
                GC.KeepAlive(visual);
                GC.KeepAlive(row);
            });
        }

        [TestMethod]
        public void CustomTablePeerPreservesCanonicalCellHeaderIdentity()
        {
            TableView customTable = null;
            RunOnUIThread.Execute(() => customTable = new CustomPeerTable());
            var table = CreateLoadedTable(customTable);
            RunOnUIThread.Execute(() =>
            {
                Verify.IsFalse(FrameworkElementAutomationPeer.CreatePeerForElement(table) is TableViewAutomationPeer);
                var row = Descendants(table).OfType<TableViewRow>().First();
                var cell = FrameworkElementAutomationPeer.CreatePeerForElement(row).GetChildren()[0];
                var headers = ((ITableItemProvider)cell.GetPattern(PatternInterface.TableItem)).GetColumnHeaderItems();
                Verify.AreEqual(1, headers.Length);
                var header = PeerAccess.FromProvider(headers[0]);
                var visual = Descendants(table).First(e => e.Tag is TableViewColumn column &&
                    column == table.Columns[0] &&
                    FrameworkElementAutomationPeer.CreatePeerForElement(e) is TableViewColumnHeaderAutomationPeer);
                Verify.IsTrue(header == FrameworkElementAutomationPeer.CreatePeerForElement(visual));
                Verify.IsTrue(PeerDescendants(FrameworkElementAutomationPeer.CreatePeerForElement(table))
                    .Any(peer => peer == header));
                AutomationProperties.SetName(visual, "Custom table header");
                Verify.AreEqual("Custom table header", header.GetName());
                Verify.IsTrue(header == PeerAccess.FromProvider(
                    ((ITableItemProvider)cell.GetPattern(PatternInterface.TableItem)).GetColumnHeaderItems()[0]));
            });
        }

        [TestMethod]
        public void HeaderPeerConstructorRequiresRealizedColumnHeader()
        {
            TableView table = null;
            RunOnUIThread.Execute(() =>
            {
                table = CreateTable();
                bool rejected = false;
                try { _ = new TableViewColumnHeaderAutomationPeer(table, table.Columns[0]); }
                catch (ArgumentException error) { rejected = error.HResult == unchecked((int)0x80070057); }
                Verify.IsTrue(rejected, "The preview constructor requires a realized header.");
                Content = table;
                table.UpdateLayout();
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(table.IsLoaded);
                var tablePeer = FrameworkElementAutomationPeer.CreatePeerForElement(table);
                var canonical = (FrameworkElementAutomationPeer)PeerAccess.FromProvider(
                    ((ITableProvider)tablePeer.GetPattern(PatternInterface.Table)).GetColumnHeaders()[0]);
                var constructed = new TableViewColumnHeaderAutomationPeer(table, table.Columns[0]);
                Verify.IsTrue(constructed.Owner == canonical.Owner);
                Verify.AreEqual("Name", constructed.GetName());
                Verify.AreEqual(AutomationControlType.HeaderItem, constructed.GetAutomationControlType());
                Verify.IsTrue(FrameworkElementAutomationPeer.CreatePeerForElement(canonical.Owner) == canonical,
                    "Explicit construction must not replace the visual's canonical peer.");
                bool rejected = false;
                try { _ = new TableViewColumnHeaderAutomationPeer(table, new TableViewTextColumn()); }
                catch (ArgumentException error) { rejected = error.HResult == unchecked((int)0x80070057); }
                Verify.IsTrue(rejected, "A realized table does not make an unrelated column header available.");
            });
        }

        // ----- Cell-level keyboard focus -----

        [TestMethod]
        public void ArrowKeysMoveFocusBetweenCellsOfTheSameRow()
        {
            var table = CreateLoadedNavTable();
            RunOnUIThread.Execute(() =>
            {
                var cells = VisibleCells(table, 0);
                Verify.AreEqual(3, cells.Length);

                Verify.IsTrue(cells[0].Focus(FocusState.Keyboard), "A cell must be able to take focus.");
                Verify.IsTrue(FocusManager.GetFocusedElement(table.XamlRoot) == cells[0]);

                Verify.IsTrue(TableViewPeerTestAccess.MoveCellCursor(cells[0], VirtualKey.Right));
                Verify.IsTrue(FocusManager.GetFocusedElement(table.XamlRoot) == cells[1],
                    "Right moves one cell within the row.");

                Verify.IsTrue(TableViewPeerTestAccess.MoveCellCursor(cells[1], VirtualKey.Right));
                Verify.IsTrue(FocusManager.GetFocusedElement(table.XamlRoot) == cells[2]);

                // Clamped at the last visible column, and the key is still consumed so it cannot
                // fall through and scroll the body sideways.
                Verify.IsTrue(TableViewPeerTestAccess.MoveCellCursor(cells[2], VirtualKey.Right));
                Verify.IsTrue(FocusManager.GetFocusedElement(table.XamlRoot) == cells[2],
                    "Right does not wrap past the last visible column.");

                Verify.IsTrue(TableViewPeerTestAccess.MoveCellCursor(cells[2], VirtualKey.Left));
                Verify.IsTrue(FocusManager.GetFocusedElement(table.XamlRoot) == cells[1]);
                Verify.IsTrue(TableViewPeerTestAccess.MoveCellCursor(cells[1], VirtualKey.Left));
                Verify.IsTrue(FocusManager.GetFocusedElement(table.XamlRoot) == cells[0]);
                Verify.IsTrue(TableViewPeerTestAccess.MoveCellCursor(cells[0], VirtualKey.Left));
                Verify.IsTrue(FocusManager.GetFocusedElement(table.XamlRoot) == cells[0],
                    "Left does not wrap past the first visible column.");
            });
        }

        [TestMethod]
        public void HomeAndEndMoveWithinTheRowAndCtrlMovesToTheTableEnds()
        {
            var table = CreateLoadedNavTable();
            RunOnUIThread.Execute(() =>
            {
                var cells = VisibleCells(table, 0);
                Verify.IsTrue(cells[1].Focus(FocusState.Keyboard));

                Verify.IsTrue(TableViewPeerTestAccess.MoveCellCursor(cells[1], VirtualKey.End));
                Verify.IsTrue(FocusManager.GetFocusedElement(table.XamlRoot) == cells[2],
                    "End moves to the last cell of the CURRENT row.");
                Verify.AreEqual(0, FocusedRowIndex(table), "End must not change rows.");

                Verify.IsTrue(TableViewPeerTestAccess.MoveCellCursor(cells[2], VirtualKey.Home));
                Verify.IsTrue(FocusManager.GetFocusedElement(table.XamlRoot) == cells[0]);
                Verify.AreEqual(0, FocusedRowIndex(table));
            });

            RunOnUIThread.Execute(() =>
            {
                var cells = VisibleCells(table, 0);
                Verify.IsTrue(TableViewPeerTestAccess.MoveCellCursor(cells[0], VirtualKey.End, control: true));
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(NavRowCount - 1, FocusedRowIndex(table), "Ctrl+End goes to the last row.");
                Verify.AreEqual(2, FocusedColumnIndex(table), "Ctrl+End goes to the last cell of that row.");
                Verify.IsTrue(TableViewPeerTestAccess.MoveCellCursor(FocusedCell(table), VirtualKey.Home, control: true));
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(0, FocusedRowIndex(table), "Ctrl+Home goes to the first row.");
                Verify.AreEqual(0, FocusedColumnIndex(table), "Ctrl+Home goes to the first cell of that row.");
            });
        }

        [TestMethod]
        public void VerticalNavigationPreservesTheFocusedColumn()
        {
            var table = CreateLoadedNavTable();
            RunOnUIThread.Execute(() =>
            {
                var cells = VisibleCells(table, 0);
                Verify.IsTrue(cells[2].Focus(FocusState.Keyboard));
                Verify.AreEqual(2, FocusedColumnIndex(table));

                // Moving to another row focuses the ROW container, which is exactly what the row
                // navigation path does; the row redirects onto the remembered column.
                var nextRow = RealizedRow(table, 1);
                Verify.IsTrue(nextRow.Focus(FocusState.Keyboard));
                Verify.AreEqual(1, FocusedRowIndex(table));
                Verify.AreEqual(2, FocusedColumnIndex(table),
                    "Moving between rows must keep the column the cursor was on.");
                Verify.IsTrue(FocusManager.GetFocusedElement(table.XamlRoot) == VisibleCells(table, 1)[2],
                    "Focus must land on a CELL of the new row, never on the row container.");
            });
        }

        [TestMethod]
        public void FocusResolvesToTheCellForAutomation()
        {
            var table = CreateLoadedNavTable();
            RunOnUIThread.Execute(() =>
            {
                var cell = VisibleCells(table, 0)[1];
                var peer = FrameworkElementAutomationPeer.CreatePeerForElement(cell);
                Verify.IsTrue(peer is TableViewCellAutomationPeer);
                Verify.IsTrue(peer.IsKeyboardFocusable(),
                    "A cell must report itself as keyboard focusable, or SetFocus throws.");

                // SetFocus used to throw "Target element cannot receive focus" for every cell.
                peer.SetFocus();
                Verify.IsTrue(peer.HasKeyboardFocus());
                Verify.IsTrue(FocusManager.GetFocusedElement(table.XamlRoot) == cell);

                var rowPeer = FrameworkElementAutomationPeer.CreatePeerForElement(RealizedRow(table, 0));
                Verify.IsFalse(rowPeer.HasKeyboardFocus(),
                    "The row must not also claim keyboard focus; that is the row-level-focus defect.");
                Verify.AreEqual("Second, B0", peer.GetName());
            });
        }

        [TestMethod]
        public void MovingWithinARowRaisesNoSelectionChange()
        {
            var table = CreateLoadedNavTable();
            int selectionChanges = 0;
            RunOnUIThread.Execute(() =>
            {
                table.SelectionMode = TableViewSelectionMode.Single;
                var cells = VisibleCells(table, 0);
                Verify.IsTrue(cells[0].Focus(FocusState.Keyboard));
                table.SelectionChanged += (s, e) => ++selectionChanges;

                Verify.IsTrue(TableViewPeerTestAccess.MoveCellCursor(cells[0], VirtualKey.Right));
                Verify.IsTrue(TableViewPeerTestAccess.MoveCellCursor(cells[1], VirtualKey.Right));
                Verify.IsTrue(TableViewPeerTestAccess.MoveCellCursor(cells[2], VirtualKey.Home));
                Verify.IsTrue(TableViewPeerTestAccess.MoveCellCursor(cells[0], VirtualKey.End));

                Verify.AreEqual(0, selectionChanges,
                    "Selection is row-level: moving the cursor inside one row must not re-raise it.");
            });
        }

        [TestMethod]
        public void CellFocusSurvivesScrollRecycling()
        {
            var table = CreateLoadedNavTable();
            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(VisibleCells(table, 0)[1].Focus(FocusState.Keyboard));
                Verify.AreEqual(1, FocusedColumnIndex(table));
                // Far enough to recycle every realized row out from under the cursor.
                Verify.IsTrue(TableViewPeerTestAccess.MoveCellCursor(FocusedCell(table), VirtualKey.End, control: true));
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() => table.UpdateLayout());
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                var lastRow = NavRowCount - 1;
                Verify.AreEqual(lastRow, FocusedRowIndex(table),
                    "Focus must follow the cursor through the rows that were recycled to reach it.");
                var focused = FocusManager.GetFocusedElement(table.XamlRoot) as UIElement;
                Verify.IsNotNull(focused);
                Verify.IsTrue(focused == VisibleCells(table, lastRow)[2],
                    "The focused element must be a live cell of the realized target row, not a recycled one.");

                // And no recycled row left a stale focus state behind in the pool. The focus
                // rectangle itself is the framework's, drawn by the focus rect manager from
                // UseSystemFocusVisuals + FocusState, so those are what there is to assert.
                foreach (var row in Descendants(table).OfType<TableViewRow>())
                {
                    foreach (var cell in VisibleCellsOf(row))
                    {
                        Verify.IsTrue(cell.UseSystemFocusVisuals,
                            "Every cell opts into the framework focus rectangle.");
                        var expected = cell == focused ? FocusState.Keyboard : FocusState.Unfocused;
                        Verify.AreEqual(expected, cell.FocusState,
                            "Exactly the focused cell may be in keyboard focus.");
                    }
                }
            });
        }

        [TestMethod]
        public void CellUsesTheFrameworkFocusRectangleNotABespokeOne()
        {
            var table = CreateLoadedNavTable();
            RunOnUIThread.Execute(() =>
            {
                var cells = VisibleCells(table, 0);
                foreach (var cell in cells)
                {
                    Verify.IsTrue(cell.IsTabStop, "CUIElement::IsFocusable gates on IsTabStop.");
                    Verify.IsTrue(cell.UseSystemFocusVisuals,
                        "The cell must use the framework focus visual, matching every other control.");

                    // No hand-rolled focus chrome: the cell holds the column's content and nothing
                    // else. A bespoke ring here would diverge from the ListViewItem convention and
                    // would not adapt to High Contrast on its own.
                    Verify.AreEqual(1, VisualTreeHelper.GetChildrenCount(cell),
                        "A cell hosts exactly the column-generated element.");
                }

                // Keyboard focus draws the ring; pointer focus deliberately does not.
                Verify.IsTrue(cells[0].Focus(FocusState.Keyboard));
                Verify.AreEqual(FocusState.Keyboard, cells[0].FocusState);
                Verify.IsTrue(cells[1].Focus(FocusState.Pointer));
                Verify.AreEqual(FocusState.Pointer, cells[1].FocusState);
                Verify.AreEqual(FocusState.Unfocused, cells[0].FocusState);

                // Selection chrome stays entirely separate from focus chrome: focusing a cell must
                // not select its row, so "focused but not selected" is distinguishable.
                Verify.AreEqual(-1, table.SelectedIndex);
            });
        }

        [TestMethod]
        public void TableIsOneTabStopAndDoesNotTrapFocus()
        {
            TableView table = null;
            Microsoft.UI.Xaml.Controls.Button before = null;
            Microsoft.UI.Xaml.Controls.Button after = null;
            RunOnUIThread.Execute(() =>
            {
                table = CreateNavTable();
                before = new Microsoft.UI.Xaml.Controls.Button { Content = "Before" };
                after = new Microsoft.UI.Xaml.Controls.Button { Content = "After" };
                var host = new Microsoft.UI.Xaml.Controls.StackPanel();
                host.Children.Add(before);
                host.Children.Add(table);
                host.Children.Add(after);
                Content = host;
                host.UpdateLayout();
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                var options = new FindNextElementOptions { SearchRoot = table.XamlRoot.Content };

                var cells = VisibleCells(table, 0);
                Verify.IsTrue(cells[1].Focus(FocusState.Keyboard));

                // Forward from a focused cell leaves the table outright - the body is ONE tab stop,
                // not one per cell and not one per row.
                var next = FocusManager.FindNextElement(FocusNavigationDirection.Next, options);
                Verify.IsFalse(IsInside(next as DependencyObject, table),
                    "Tab from a cell must leave the table, not step to the next cell or row.");

                // And backwards, so focus is never trapped in either direction.
                var previous = FocusManager.FindNextElement(FocusNavigationDirection.Previous, options);
                Verify.IsFalse(IsInside(previous as DependencyObject, table),
                    "Shift+Tab from a cell must leave the table.");
            });
        }

        [TestMethod]
        public void FocusReturnsToTheCellAfterAnEdit()
        {
            var table = CreateLoadedNavTable();
            RunOnUIThread.Execute(() =>
            {
                var cells = VisibleCells(table, 0);
                Verify.IsTrue(cells[2].Focus(FocusState.Keyboard));

                // Drives the real edit lifecycle (BeginEdit / write / CommitEdit) on the focused cell.
                var peer = FrameworkElementAutomationPeer.CreatePeerForElement(cells[2]);
                ((IValueProvider)peer.GetPattern(PatternInterface.Value)).SetValue("Edited");
                Verify.IsFalse(table.IsEditing, "The edit must have closed.");

                var focused = FocusManager.GetFocusedElement(table.XamlRoot);
                Verify.IsTrue(focused == cells[2],
                    "Focus must come back to the CELL after a commit - not to the row, and not be lost.");
                Verify.AreEqual(2, FocusedColumnIndex(table));

                // And the cursor still navigates from there.
                Verify.IsTrue(TableViewPeerTestAccess.MoveCellCursor(cells[2], VirtualKey.Home));
                Verify.IsTrue(FocusManager.GetFocusedElement(table.XamlRoot) == cells[0]);
            });
        }

        [TestMethod]
        public void CollapsedColumnsAreSkippedByCellNavigation()
        {
            var table = CreateLoadedNavTable();
            RunOnUIThread.Execute(() =>
            {
                table.Columns[1].Visibility = Visibility.Collapsed;
                table.UpdateLayout();
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                var cells = VisibleCells(table, 0);
                Verify.AreEqual(2, cells.Length, "A collapsed column contributes no navigable cell.");
                Verify.IsTrue(cells[0].Focus(FocusState.Keyboard));
                Verify.IsTrue(TableViewPeerTestAccess.MoveCellCursor(cells[0], VirtualKey.Right));
                Verify.IsTrue(FocusManager.GetFocusedElement(table.XamlRoot) == cells[1],
                    "Right must land on the next VISIBLE column, skipping the collapsed one.");
                Verify.IsTrue(TableViewPeerTestAccess.MoveCellCursor(cells[1], VirtualKey.Right));
                Verify.IsTrue(FocusManager.GetFocusedElement(table.XamlRoot) == cells[1]);
            });
        }

        [TestMethod]
        public void SingleArrowPressMovesExactlyOneColumn()
        {
            var table = CreateLoadedNavTable();
            RunOnUIThread.Execute(() =>
            {
                var cells = VisibleCells(table, 0);
                Verify.IsTrue(cells[0].Focus(FocusState.Keyboard));
                Verify.AreEqual(0, FocusedGridItemColumn(table));

                // The regression: each of these used to advance the cursor by TWO columns, because
                // the move was computed against live focus after built-in navigation had already
                // stepped it once.
                Verify.IsTrue(TableViewPeerTestAccess.MoveCellCursor(cells[0], VirtualKey.Right));
                Verify.AreEqual(1, FocusedGridItemColumn(table), "One Right must land on column 1, not 2.");

                Verify.IsTrue(TableViewPeerTestAccess.MoveCellCursor(cells[1], VirtualKey.Right));
                Verify.AreEqual(2, FocusedGridItemColumn(table), "One Right must land on column 2, not 4.");

                Verify.IsTrue(TableViewPeerTestAccess.MoveCellCursor(cells[2], VirtualKey.Left));
                Verify.AreEqual(1, FocusedGridItemColumn(table), "One Left must land on column 1.");

                Verify.IsTrue(TableViewPeerTestAccess.MoveCellCursor(cells[1], VirtualKey.Left));
                Verify.AreEqual(0, FocusedGridItemColumn(table));

                // The row never changes on a horizontal move.
                Verify.AreEqual(0, FocusedGridItemRow(table));
            });
        }

        [TestMethod]
        public void ArrowStepStaysOneWhenBuiltInNavigationAlreadyMovedFocus()
        {
            var table = CreateLoadedNavTable();
            RunOnUIThread.Execute(() =>
            {
                var cells = VisibleCells(table, 0);

                // Reproduce the live sequence exactly: the cursor is anchored on column 0 by the
                // tunneling PreviewKeyDown pass, then XAML's built-in directional navigation
                // advances focus to column 1 before the bubbling KeyDown handler runs.
                Verify.IsTrue(cells[0].Focus(FocusState.Keyboard));
                Verify.IsTrue(cells[1].Focus(FocusState.Keyboard));   // stand-in for built-in nav
                Verify.AreEqual(1, FocusedGridItemColumn(table));

                Verify.IsTrue(TableViewPeerTestAccess.MoveCellCursorFromAnchor(
                    cells[1], VirtualKey.Right, anchorRow: 0, anchorColumn: 0));
                Verify.AreEqual(1, FocusedGridItemColumn(table),
                    "The handler must honour the pre-key anchor and leave the cursor on column 1.");

                // Same in the other direction.
                Verify.IsTrue(cells[1].Focus(FocusState.Keyboard));
                Verify.IsTrue(cells[0].Focus(FocusState.Keyboard));   // stand-in for built-in nav
                Verify.IsTrue(TableViewPeerTestAccess.MoveCellCursorFromAnchor(
                    cells[0], VirtualKey.Left, anchorRow: 0, anchorColumn: 1));
                Verify.AreEqual(0, FocusedGridItemColumn(table));

                // And at a boundary the cursor is re-pinned, so built-in navigation cannot walk
                // focus out of the row past the first column.
                Verify.IsTrue(cells[1].Focus(FocusState.Keyboard));   // stand-in for a stray move
                Verify.IsTrue(TableViewPeerTestAccess.MoveCellCursorFromAnchor(
                    cells[1], VirtualKey.Left, anchorRow: 0, anchorColumn: 0));
                Verify.AreEqual(0, FocusedGridItemColumn(table),
                    "A clamped arrow must restore the cursor to the cell it started on.");
            });
        }

        [TestMethod]
        public void ArrowStepIsOneColumnAndMirroredInRightToLeft()
        {
            var table = CreateLoadedNavTable();
            RunOnUIThread.Execute(() =>
            {
                table.FlowDirection = FlowDirection.RightToLeft;
                table.UpdateLayout();
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() =>
            {
                var cells = VisibleCells(table, 0);
                Verify.IsTrue(cells[1].Focus(FocusState.Keyboard));
                Verify.AreEqual(1, FocusedGridItemColumn(table));

                // Column 0 renders at the RIGHT edge in RTL, so Right moves towards column 0 -
                // by exactly one column, like every other XAML list.
                Verify.IsTrue(TableViewPeerTestAccess.MoveCellCursor(cells[1], VirtualKey.Right));
                Verify.AreEqual(0, FocusedGridItemColumn(table), "RTL Right moves one column towards 0.");

                Verify.IsTrue(TableViewPeerTestAccess.MoveCellCursor(cells[0], VirtualKey.Left));
                Verify.AreEqual(1, FocusedGridItemColumn(table), "RTL Left moves one column away from 0.");

                Verify.IsTrue(TableViewPeerTestAccess.MoveCellCursor(cells[1], VirtualKey.Left));
                Verify.AreEqual(2, FocusedGridItemColumn(table));

                // Clamping still applies to the logical ends.
                Verify.IsTrue(TableViewPeerTestAccess.MoveCellCursor(cells[2], VirtualKey.Left));
                Verify.AreEqual(2, FocusedGridItemColumn(table), "RTL Left clamps at the last column.");
            });
        }

        private const int NavRowCount = 200;

        private TableView CreateLoadedNavTable()
        {
            TableView table = null;
            RunOnUIThread.Execute(() =>
            {
                table = CreateNavTable();
                Content = table;
                table.UpdateLayout();
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() => Verify.IsTrue(table.IsLoaded));
            return table;
        }

        private static TableView CreateNavTable()
        {
            var table = new TableView { Width = 500, Height = 240, IsReadOnly = false };
            table.Resources.MergedDictionaries.Add(new TabularControlsResources());
            foreach (var header in new[] { "First", "Second", "Third" })
            {
                table.Columns.Add(new TableViewTextColumn
                {
                    Header = header,
                    Width = new GridLength(140),
                    Binding = new Binding { Path = new PropertyPath("Name") }
                });
            }
            var items = new ObservableCollection<Item>();
            for (int i = 0; i < NavRowCount; ++i)
            {
                items.Add(new Item { Name = "B" + i });
            }
            table.ItemsSource = items;
            return table;
        }

        private static TableViewRow RealizedRow(TableView table, int index)
        {
            var repeater = Descendants(table).OfType<Microsoft.UI.Xaml.Controls.ItemsRepeater>().First();
            return repeater.TryGetElement(index) as TableViewRow;
        }

        private static UIElement[] VisibleCellsOf(TableViewRow row)
        {
            var host = Descendants(row).OfType<TableViewCellsPanel>().Single();
            return host.Children
                .Where(child => (child as FrameworkElement)?.Tag is TableViewColumn column &&
                    column.Visibility == Visibility.Visible)
                .ToArray();
        }

        private static UIElement[] VisibleCells(TableView table, int rowIndex) =>
            VisibleCellsOf(RealizedRow(table, rowIndex));

        private static UIElement FocusedCell(TableView table) =>
            FocusManager.GetFocusedElement(table.XamlRoot) as UIElement;

        private static int FocusedRowIndex(TableView table)
        {
            var repeater = Descendants(table).OfType<Microsoft.UI.Xaml.Controls.ItemsRepeater>().First();
            DependencyObject node = FocusManager.GetFocusedElement(table.XamlRoot) as DependencyObject;
            while (node != null)
            {
                if (node is TableViewRow row) return repeater.GetElementIndex(row);
                node = VisualTreeHelper.GetParent(node);
            }
            return -1;
        }

        private static int FocusedColumnIndex(TableView table)
        {
            var focused = FocusManager.GetFocusedElement(table.XamlRoot) as DependencyObject;
            var index = FocusedRowIndex(table);
            if (index < 0) return -1;
            return Array.IndexOf(VisibleCells(table, index), focused as UIElement);
        }

        // Reads the coordinate the way a UIA client does - GridItemPattern on the peer of whatever
        // FocusManager reports as focused - rather than by comparing element identity, so an
        // off-by-one in the cursor cannot pass unnoticed.
        private static IGridItemProvider FocusedGridItem(TableView table)
        {
            var focused = FocusManager.GetFocusedElement(table.XamlRoot) as UIElement;
            Verify.IsNotNull(focused, "Something must have keyboard focus.");
            var peer = FrameworkElementAutomationPeer.CreatePeerForElement(focused);
            Verify.IsNotNull(peer);
            return (IGridItemProvider)peer.GetPattern(PatternInterface.GridItem);
        }

        private static int FocusedGridItemColumn(TableView table) => FocusedGridItem(table).Column;

        private static int FocusedGridItemRow(TableView table) => FocusedGridItem(table).Row;

        private static bool IsInside(DependencyObject node, DependencyObject ancestor)
        {
            while (node != null)
            {
                if (node == ancestor) return true;
                node = VisualTreeHelper.GetParent(node);
            }
            return false;
        }

        private TableView CreateLoadedTable(TableView table = null)
        {
            RunOnUIThread.Execute(() =>
            {
                table = CreateTable(table);
                Content = table;
                table.UpdateLayout();
            });
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() => Verify.IsTrue(table.IsLoaded));
            return table;
        }

        private static TableView CreateTable(TableView table = null)
        {
            table ??= new TableView();
            table.Width = 500;
            table.Height = 240;
            table.IsReadOnly = false;
            table.CanUserSortColumns = true;
            table.Resources.MergedDictionaries.Add(new TabularControlsResources());
            table.Columns.Add(new TableViewTextColumn
            {
                Header = "Name", Width = new GridLength(200),
                Binding = new Binding { Path = new PropertyPath("Name") }
            });
            table.Columns.Add(new TableViewTextColumn
            {
                Header = "Other", Width = new GridLength(200), CanSort = false,
                Binding = new Binding { Path = new PropertyPath("Name") }
            });
            table.ItemsSource = new[] { new Item { Name = "Al" } };
            return table;
        }

        private static IEnumerable<FrameworkElement> Descendants(DependencyObject node)
        {
            for (int i = 0; i < VisualTreeHelper.GetChildrenCount(node); ++i)
            {
                var child = VisualTreeHelper.GetChild(node, i);
                if (child is FrameworkElement element) yield return element;
                foreach (var descendant in Descendants(child)) yield return descendant;
            }
        }

        private static IEnumerable<AutomationPeer> PeerDescendants(AutomationPeer root)
        {
            var pending = new Queue<AutomationPeer>();
            var seen = new HashSet<AutomationPeer> { root };
            pending.Enqueue(root);
            while (pending.Count != 0)
            {
                var children = pending.Dequeue().GetChildren();
                if (children == null) continue;
                foreach (var child in children)
                {
                    Verify.IsTrue(seen.Add(child), "The peer tree must not contain cycles or duplicate peers.");
                    Verify.IsTrue(seen.Count < 200, "Bound this small fixture's peer traversal.");
                    pending.Enqueue(child);
                    yield return child;
                }
            }
        }

        [Microsoft.UI.Xaml.Data.Bindable]
        public partial class Item { public string Name { get; set; } }

        private sealed partial class CustomPeerTable : TableView
        {
            protected override AutomationPeer OnCreateAutomationPeer() => new CustomOwnerPeer(this);
        }

        private sealed partial class SnapshotTrackingColumn : TableViewTextColumn
        {
            protected override FrameworkElement GenerateElementCore(object item)
            {
                var text = new Microsoft.UI.Xaml.Controls.TextBlock();
                text.SetBinding(Microsoft.UI.Xaml.Controls.TextBlock.TextProperty,
                    new Binding { Path = new PropertyPath("Name") });
                var label = new SnapshotLabel();
                AutomationProperties.SetLabeledBy(text, label);
                return text;
            }
        }

        private sealed partial class SnapshotLabel : Microsoft.UI.Xaml.Controls.Control
        {
            public int Reads { get; private set; }
            protected override AutomationPeer OnCreateAutomationPeer() => new SnapshotLabelPeer(this);
            private sealed partial class SnapshotLabelPeer : FrameworkElementAutomationPeer
            {
                private readonly SnapshotLabel _label;
                public SnapshotLabelPeer(SnapshotLabel label) : base(label) => _label = label;
                protected override string GetNameCore()
                {
                    ++_label.Reads;
                    return "Stable label";
                }
            }
        }

        private sealed partial class CustomOwnerPeer : FrameworkElementAutomationPeer
        {
            public CustomOwnerPeer(FrameworkElement owner) : base(owner) { }
        }

        private sealed partial class PeerAccess : FrameworkElementAutomationPeer
        {
            private PeerAccess(FrameworkElement owner) : base(owner) { }
            public static AutomationPeer FromProvider(IRawElementProviderSimple provider) =>
                new PeerAccess(new Microsoft.UI.Xaml.Controls.Border()).PeerFromProvider(provider);
        }
    }
}
