// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
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
