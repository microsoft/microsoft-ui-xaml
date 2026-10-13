// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Tabular;
using MUXControlsTestApp.Utilities;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;

using WEX.TestExecution;
using WEX.TestExecution.Markup;

using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewAutomationTestHelpers;
using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewRowTestHelpers;
using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewTestHelpers;
using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewTreeTestHelpers;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    // TableView.RowStyleSelector: a Style per realized container, chosen by kind (leaf row,
    // expandable row, group header) and kept in sync as rows recycle and hierarchy state changes.
    //
    // Fixture: MakeTree with ParentBy(Id, ManagerId):
    //   Ada > { Ben > { Dan }, Cy }, Eve > { Fay }, Gus.
    // Expandable: Ada, Ben, Eve. Leaf: Cy, Dan, Fay, Gus.
    [TestClass]
    public class TableViewRowStyleSelectorTests : TableViewApiTestBase
    {
        [TestMethod]
        [TestProperty("Description", "RS1: leaf and expandable rows get their own style, and item is the data item.")]
        public void LeafAndExpandableRowsGetTheirStyles()
        {
            var f = Place(grouped: false);
            Run(f, t => t.ExpandAllRows());

            RunOnUIThread.Execute(() =>
            {
                VerifyKinds(f, "after ExpandAllRows");
                var ada = FindTreeRow(f.Table, "Ada");
                Verify.AreSame(ada.DataContext, f.Selector.LastItem[ada], "A row's item is its data item.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "RS2: group headers in a grouped hierarchy get the header style, and item is the TableViewGroupInfo.")]
        public void GroupHeadersGetHeaderStyle()
        {
            var f = Place(grouped: true);

            RunOnUIThread.Execute(() =>
            {
                var header = GetGroupHeader(f.Table, 0);
                Verify.AreSame(f.Selector.GroupHeader, header.Style, "Group header style.");
                Verify.IsTrue(f.Selector.LastItem[header] is TableViewGroupInfo, "A header's item is its TableViewGroupInfo.");
                VerifyKinds(f, "grouped roots");
            });
        }

        [TestMethod]
        [TestProperty("Description", "RS3: with no selector, TableView leaves the container Style unset.")]
        public void NoSelectorLeavesStyleUnset()
        {
            var f = Place(grouped: true, withSelector: false);

            RunOnUIThread.Execute(() =>
            {
                foreach (var element in GetProjectedElements(f.Table).OfType<Control>())
                {
                    Verify.AreEqual(DependencyProperty.UnsetValue, element.ReadLocalValue(FrameworkElement.StyleProperty),
                        $"{element.GetType().Name} Style must stay unset without a selector.");
                }
            });
        }

        [TestMethod]
        [TestProperty("Description", "RS4: a realized leaf row that gains a child switches to the expandable style.")]
        public void LeafGainingChildSwitchesToExpandableStyle()
        {
            var f = Place(grouped: false);
            Run(f, t => t.ExpandAllRows());
            RunOnUIThread.Execute(() => Verify.AreSame(f.Selector.Leaf, FindTreeRow(f.Table, "Cy").Style, "Precondition: Cy is a leaf."));

            Run(f, t => f.Items.Add(new TreeEmployee { Id = 8, ManagerId = 3, Name = "Hal", Dept = "Eng" }));

            RunOnUIThread.Execute(() =>
            {
                var cy = FindTreeRow(f.Table, "Cy");
                Verify.IsTrue(cy.IsExpandable, "Cy has a child now.");
                Verify.AreSame(f.Selector.Expandable, cy.Style, "Cy's realized row is restyled as expandable.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "RS5: after collapse and re-expand (rows recycled across kinds), every realized row's style matches its kind.")]
        public void RecycledRowsKeepStyleInSyncWithKind()
        {
            var f = Place(grouped: false);
            Run(f, t => t.ExpandAllRows());
            Run(f, t => t.CollapseAllRows());
            RunOnUIThread.Execute(() => ToggleTreeRow(f.Table, "Ada", expand: true));
            SettleLayout(f.Table);
            RunOnUIThread.Execute(() => ToggleTreeRow(f.Table, "Eve", expand: true));
            SettleLayout(f.Table);

            RunOnUIThread.Execute(() => VerifyKinds(f, "after collapse and partial re-expand"));
        }

        [TestMethod]
        [TestProperty("Description", "RS6: removing the selector restores the app's implicit row style.")]
        public void RemovingSelectorRestoresImplicitStyle()
        {
            Style implicitStyle = null;
            MenuFlyout implicitMenu = null;
            var f = Place(grouped: false, configure: t =>
            {
                implicitMenu = new MenuFlyout();
                implicitStyle = new Style(typeof(TableViewRow)) { Setters = { new Setter(UIElement.ContextFlyoutProperty, implicitMenu) } };
                t.Resources[typeof(TableViewRow)] = implicitStyle;
            });
            RunOnUIThread.Execute(() => Verify.AreSame(f.Selector.Expandable, FindTreeRow(f.Table, "Ada").Style, "Precondition: the selector style wins."));

            Run(f, t => t.RowStyleSelector = null);

            RunOnUIThread.Execute(() =>
            {
                var ada = FindTreeRow(f.Table, "Ada");
                Verify.AreEqual(DependencyProperty.UnsetValue, ada.ReadLocalValue(FrameworkElement.StyleProperty), "Selector style cleared.");
                Verify.AreSame(implicitMenu, ada.ContextFlyout, "The implicit style applies again.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "RS7: swapping the selector restyles realized rows and headers at once.")]
        public void SwappingSelectorRestylesRealizedContainers()
        {
            var f = Place(grouped: true);
            KindStyleSelector second = null;
            Run(f, t =>
            {
                second = KindStyleSelector.Create();
                t.RowStyleSelector = second;
            });

            RunOnUIThread.Execute(() =>
            {
                f.Selector = second;
                VerifyKinds(f, "after swapping the selector");
                Verify.AreSame(second.GroupHeader, GetGroupHeader(f.Table, 0).Style, "Header restyled by the new selector.");
            });
        }

        [TestMethod]
        [TestProperty("Description", "RS8: a context request on an expandable row opens the expandable menu; on a leaf, the leaf menu.")]
        public void RowKindPicksRowMenu()
        {
            var f = Place(grouped: false);
            Run(f, t => t.ExpandAllRows());

            RunOnUIThread.Execute(() => GetRowPeer(FindTreeRow(f.Table, "Ada")).ShowContextMenu());
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() => Verify.AreEqual("ExpandableMenu", f.Recorder.Describe(), "Ada is expandable."));
            CloseMenus(f);

            RunOnUIThread.Execute(() => GetRowPeer(FindTreeRow(f.Table, "Cy")).ShowContextMenu());
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() => Verify.AreEqual("LeafMenu", f.Recorder.Describe(), "Cy is a leaf."));
            CloseMenus(f);
        }

        [TestMethod]
        [TestProperty("Description", "RS9: a context request on a group header opens the group menu.")]
        public void GroupHeaderPicksGroupMenu()
        {
            var f = Place(grouped: true);

            RunOnUIThread.Execute(() => GetGroupHeaderPeer(f.Table, 0).ShowContextMenu());
            IdleSynchronizer.Wait();
            RunOnUIThread.Execute(() => Verify.AreEqual("GroupMenu", f.Recorder.Describe(), "Group header menu."));
            CloseMenus(f);
        }

        // ---- fixture ----

        private sealed class Fixture
        {
            public TableView Table;
            public ObservableCollection<TreeEmployee> Items;
            public KindStyleSelector Selector;
            public MenuRecorder Recorder = new MenuRecorder();
        }

        // Tree table over MakeTree (grouped: roots grouped by Dept), loaded and settled. Call off the UI thread.
        private Fixture Place(bool grouped, bool withSelector = true, Action<TableView> configure = null)
        {
            var f = new Fixture();
            RunOnUIThread.Execute(() =>
            {
                EnsureTabularControlsResources();
                f.Items = MakeTree();
                var source = TableViewSource.From(f.Items).ParentBy(ById, ByManager);
                if (grouped)
                {
                    source.GroupBy(EmpKey(e => e.Dept));
                }

                f.Table = CreateTreeTable(source);
                configure?.Invoke(f.Table);
                if (withSelector)
                {
                    f.Selector = KindStyleSelector.Create();
                    f.Recorder.Track("LeafMenu", f.Selector.LeafMenu);
                    f.Recorder.Track("ExpandableMenu", f.Selector.ExpandableMenu);
                    f.Recorder.Track("GroupMenu", f.Selector.GroupMenu);
                    f.Table.RowStyleSelector = f.Selector;
                }

                LoadContent(f.Table);
            });
            SettleLayout(f.Table);
            return f;
        }

        private void Run(Fixture f, Action<TableView> action)
        {
            RunOnUIThread.Execute(() => action(f.Table));
            SettleLayout(f.Table);
        }

        private static void CloseMenus(Fixture f)
        {
            RunOnUIThread.Execute(() =>
            {
                f.Recorder.HideAll();
                f.Recorder.Clear();
            });
            IdleSynchronizer.Wait();
        }

        // Every realized container's Style matches its kind. Call on the UI thread.
        private static void VerifyKinds(Fixture f, string context)
        {
            foreach (var element in GetProjectedElements(f.Table))
            {
                switch (element)
                {
                    case TableViewGroupHeader header:
                        Verify.AreSame(f.Selector.GroupHeader, header.Style, $"Group header style ({context}).");
                        break;
                    case TableViewRow row:
                        var expected = row.IsExpandable ? f.Selector.Expandable : f.Selector.Leaf;
                        Verify.AreSame(expected, row.Style, $"{NameOf(row.DataContext)} style, IsExpandable={row.IsExpandable} ({context}).");
                        break;
                }
            }
        }
    }

    // Picks by container kind and records the item it was given per container. Create on the UI thread.
    internal sealed partial class KindStyleSelector : StyleSelector
    {
        public Style Leaf;
        public Style Expandable;
        public Style GroupHeader;
        public MenuFlyout LeafMenu;
        public MenuFlyout ExpandableMenu;
        public MenuFlyout GroupMenu;
        public readonly Dictionary<DependencyObject, object> LastItem = new Dictionary<DependencyObject, object>();

        public static KindStyleSelector Create()
        {
            var s = new KindStyleSelector
            {
                LeafMenu = new MenuFlyout(),
                ExpandableMenu = new MenuFlyout(),
                GroupMenu = new MenuFlyout(),
            };
            s.Leaf = new Style(typeof(TableViewRow)) { Setters = { new Setter(UIElement.ContextFlyoutProperty, s.LeafMenu) } };
            s.Expandable = new Style(typeof(TableViewRow)) { Setters = { new Setter(UIElement.ContextFlyoutProperty, s.ExpandableMenu) } };
            s.GroupHeader = new Style(typeof(TableViewGroupHeader)) { Setters = { new Setter(UIElement.ContextFlyoutProperty, s.GroupMenu) } };
            return s;
        }

        protected override Style SelectStyleCore(object item, DependencyObject container)
        {
            LastItem[container] = item;
            if (container is TableViewGroupHeader)
            {
                return GroupHeader;
            }

            if (container is TableViewRow row)
            {
                return row.IsExpandable ? Expandable : Leaf;
            }

            return null;
        }
    }
}
