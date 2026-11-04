// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using MUXControlsTestApp.Utilities;
using System;
using System.Linq;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Automation;
using Microsoft.UI.Xaml.Automation.Peers;
using Microsoft.UI.Xaml.Automation.Provider;
using System.Threading;
using System.Collections.Generic;
using Windows.Foundation.Collections;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Markup;
using Microsoft.UI.Xaml.Media.Animation;
using System.Collections.ObjectModel;
using MUXControlsTestApp;
using Common;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;
using MUXControlsTestApp.Samples;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    partial class SpecialTreeView : TreeView { }
    partial class SpecialTreeViewItem : TreeViewItem { }
    partial class SpecialTreeViewList : TreeViewList
    {
        protected override DependencyObject GetContainerForItemOverride()
        {
            return new SpecialTreeViewItem();
        }
    }
    class SpecialTreeViewNode : TreeViewNode { }

    [TestClass]
    public class TreeViewTests : ApiTestBase
    {
        // Scenario: Detached TreeViewNodes (no TreeView): Children.Add/Insert/RemoveAt on a parent node.
        // Expected: HasChildren (+DP) false at first; Parent/Count/order/Depth update (child 0, removed -1);
        //   parent IsExpanded stays false.
        // Failure means: TreeViewNodeVector Append/InsertAt/RemoveAt or TreeViewNode::put_ParentImpl/UpdateDepth
        //   no longer maintain Parent/Depth bookkeeping.
        [TestMethod]
        public void TreeViewNodeTest()
        {
            RunOnUIThread.Execute(() =>
            {
                TreeViewNode treeViewNode1 = new TreeViewNode();
                TreeViewNode treeViewNode2 = new TreeViewNode();
                TreeViewNode treeViewNode3 = new TreeViewNode();

                Verify.AreEqual(false, treeViewNode1.HasChildren);
                Verify.AreEqual(false, (bool)treeViewNode1.GetValue(TreeViewNode.HasChildrenProperty));

                //Test adding a single TreeViewNode
                treeViewNode1.Children.Add(treeViewNode2);

                Verify.AreEqual(treeViewNode2.Parent, treeViewNode1);
                Verify.AreEqual(treeViewNode1.Children.Count, 1);
                Verify.AreEqual(treeViewNode1.Children[0], treeViewNode2);
                Verify.AreEqual(treeViewNode1.IsExpanded, false);
                Verify.AreEqual(treeViewNode2.Depth, 0);

                //Test removing a single TreeViweNode
                treeViewNode1.Children.RemoveAt(0);

                Verify.IsNull(treeViewNode2.Parent);
                Verify.AreEqual(treeViewNode1.Children.Count, 0);
                Verify.AreEqual(treeViewNode2.Depth, -1);

                //Test insert multiple TreeViewNodes
                treeViewNode1.Children.Insert(0, treeViewNode2);
                treeViewNode1.Children.Insert(0, treeViewNode3);

                Verify.AreEqual(treeViewNode1.Children.Count, 2);

                Verify.AreEqual(treeViewNode1.Children[0], treeViewNode3);
                Verify.AreEqual(treeViewNode3.Depth, 0);
                Verify.AreEqual(treeViewNode3.Parent, treeViewNode1);

                Verify.AreEqual(treeViewNode1.Children[1], treeViewNode2);
                Verify.AreEqual(treeViewNode2.Depth, 0);
                Verify.AreEqual(treeViewNode2.Parent, treeViewNode1);

                //Test remove multiple TreeViewNodes
                treeViewNode1.Children.RemoveAt(0);

                Verify.AreEqual(treeViewNode1.Children[0], treeViewNode2);
                Verify.AreEqual(treeViewNode1.Children.Count, 1);
                Verify.IsNull(treeViewNode3.Parent);
                Verify.AreEqual(treeViewNode3.Depth, -1);

                treeViewNode1.Children.RemoveAt(0);

                Verify.AreEqual(treeViewNode1.Children.Count, 0);
                Verify.IsNull(treeViewNode2.Parent);
                Verify.AreEqual(treeViewNode2.Depth, -1);
            });
        }

        // Scenario: Node mode (RootNodes); child1 expanded; SetAt under collapsed child2, SetAt on RootNodes[0],
        //   expand child2, then RootNodes.Clear() twice.
        // Expected: ListControl.Items count 3 -> 3 (collapsed SetAt hidden) -> 4 -> 0 -> 0 (empty Clear is safe).
        // Failure means: ViewModel::TreeViewNodeVectorChanged/SetAt/Clear mishandle replace or reset of nodes in
        //   the flattened view (e.g. a replaced expanded root's descendants are not removed).
        [TestMethod]
        public void TreeViewClearAndSetAtTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var treeView = new TreeView();

                Content = treeView;
                Content.UpdateLayout();
                var listControl = VisualTreeUtils.FindVisualChildByName(treeView, "ListControl") as TreeViewList;
                // Verify TreeViewNode::SetAt
                TreeViewNode setAtChildCheckNode = new TreeViewNode() { Content = "Set At Child" };
                TreeViewNode setAtRootCheckNode = new TreeViewNode() { Content = "Set At Root" };

                TreeViewNode child1 = new TreeViewNode() { Content = "Child 1" };
                child1.Children.Add(new TreeViewNode() { Content = "Child 1:1" });

                TreeViewNode child2 = new TreeViewNode() { Content = "Child 2" };
                child2.Children.Add(new TreeViewNode() { Content = "Child 2:1" });
                child2.Children.Add(new TreeViewNode() { Content = "Child 2:2" });

                treeView.RootNodes.Add(child1);
                child1.IsExpanded = true;
                treeView.RootNodes.Add(child2);
                Verify.AreEqual(listControl.Items.Count, 3);

                // SetAt node under child node which is not expanded
                child2.Children[1] = setAtChildCheckNode;
                Verify.AreEqual(listControl.Items.Count, 3);

                // SetAt node under root node and child2 is expanded
                treeView.RootNodes[0] = setAtRootCheckNode;
                child2.IsExpanded = true;
                Verify.AreEqual(listControl.Items.Count, 4);

                // Verify RootNode.Clear
                treeView.RootNodes.Clear();
                Verify.AreEqual(listControl.Items.Count, 0);

                // test clear without any child node
                treeView.RootNodes.Clear();
                Verify.AreEqual(listControl.Items.Count, 0);
            });
        }

        // Scenario: Content mode (ItemsSource = ExtendedObservableCollection of 3 items), then ReplaceAll with 2
        //   new items (raises a collection Reset).
        // Expected: RootNodes count 3 then 2; RootNodes[0].Content is items[0] before and after the reset.
        // Failure means: TreeViewNode::OnItemsSourceChanged/SyncChildrenNodesWithItemsSource no longer rebuilds
        //   the root nodes on a collection Reset.
        [TestMethod]
        public void TreeViewItemSourceResetRecreateItems()
        {
            RunOnUIThread.Execute(() =>
            {

                ExtendedObservableCollection<TreeViewItemSource> items
                    = new ExtendedObservableCollection<TreeViewItemSource>();
                TreeViewItemSource item1 = new TreeViewItemSource() { Content = "item1" };
                TreeViewItemSource item2 = new TreeViewItemSource() { Content = "item2" };
                TreeViewItemSource item3 = new TreeViewItemSource() { Content = "item3" };
                items.Add(item1);
                items.Add(item2);
                items.Add(item3);

                var treeView = new TreeView();
                treeView.ItemsSource = items;

                Verify.AreEqual(treeView.RootNodes.Count, 3);
                Verify.AreEqual(treeView.RootNodes[0].Content as TreeViewItemSource, items[0]);

                List<TreeViewItemSource> newItems = new List<TreeViewItemSource>();
                TreeViewItemSource item4 = new TreeViewItemSource() { Content = "item4" };
                TreeViewItemSource item5 = new TreeViewItemSource() { Content = "item5" };

                newItems.Add(item4);
                newItems.Add(item5);

                items.ReplaceAll(newItems);

                Verify.AreEqual(treeView.RootNodes.Count, 2);
                Verify.AreEqual(treeView.RootNodes[0].Content as TreeViewItemSource, items[0]);


            });
        }

        // Scenario: Node mode; root with 4 children added to RootNodes, Expand(root), then RemoveAt(2),
        //   Insert(1, node4) and Children.Clear() on the expanded root.
        // Expected: ListControl.Items count 1 -> 5 -> 4 -> 5 -> 1 with the exact flattened order after each edit.
        // Failure means: ViewModel::TreeViewNodeVectorChanged/ExpandNode no longer keep the flattened view in sync
        //   with child edits under an expanded node.
        [TestMethod]
        public void TreeViewUpdateTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var treeViewNode1 = new TreeViewNode();
                var treeViewNode2 = new TreeViewNode();
                var treeViewNode3 = new TreeViewNode();
                var treeViewNode4 = new TreeViewNode();
                var treeViewNode5 = new TreeViewNode();

                treeViewNode1.Children.Add(treeViewNode2);
                treeViewNode1.Children.Add(treeViewNode3);
                treeViewNode1.Children.Add(treeViewNode4);
                treeViewNode1.Children.Add(treeViewNode5);

                var treeView = new TreeView();
                Content = treeView;
                Content.UpdateLayout();
                var listControl = VisualTreeUtils.FindVisualChildByName(treeView, "ListControl") as TreeViewList;
                treeView.RootNodes.Add(treeViewNode1);
                Verify.AreEqual(listControl.Items.Count, 1);

                treeView.Expand(treeViewNode1);
                Verify.AreEqual(listControl.Items.Count, 5);

                treeViewNode1.Children.RemoveAt(2);
                Verify.AreEqual(listControl.Items.Count, 4);
                Verify.AreEqual(listControl.Items[0], treeViewNode1);
                Verify.AreEqual(listControl.Items[1], treeViewNode2);
                Verify.AreEqual(listControl.Items[2], treeViewNode3);
                Verify.AreEqual(listControl.Items[3], treeViewNode5);

                treeViewNode1.Children.Insert(1, treeViewNode4);
                Verify.AreEqual(listControl.Items.Count, 5);
                Verify.AreEqual(listControl.Items[0], treeViewNode1);
                Verify.AreEqual(listControl.Items[1], treeViewNode2);
                Verify.AreEqual(listControl.Items[2], treeViewNode4);
                Verify.AreEqual(listControl.Items[3], treeViewNode3);
                Verify.AreEqual(listControl.Items[4], treeViewNode5);

                treeViewNode1.Children.Clear();
                Verify.AreEqual(listControl.Items.Count, 1);
                Verify.AreEqual(listControl.Items[0], treeViewNode1);
            });
        }

        // Scenario: Lays out code-only managed subclasses SpecialTreeView and SpecialTreeViewList (ItemsSource of
        //   5 strings) in a StackPanel; also constructs SpecialTreeViewItem and SpecialTreeViewNode.
        // Expected: No exception or crash while the subclassed controls are laid out.
        // Failure means: Ignored (PC-2, WinUI platform issue, GitHub #1775): laying out code-only managed
        //   subclasses of TreeView (also Expander/InfoBadge) throws 0x800F1000 and crashes the test host;
        //   a pass would mean such subclasses now lay out and the test can be re-enabled.
        [TestMethod]
        // Not a TreeView bug (GitHub #1775 was closed as stale). Observed in this API-test host: laying out code-only
        // managed subclasses of TreeView, Expander or InfoBadge (MUXC templated controls deriving directly from Control;
        // internal or public; defined in this file or in the TreeView TestUI sources) throws COMException 0x800F1000
        // "No installed components were detected" and then crashes TE.ProcessHost with 0xC000027B in Microsoft.UI.Xaml.dll,
        // aborting the rest of the run. Code-only subclasses of TreeViewList (derives from the core ListView), of the core
        // Button and of TreeViewNode lay out fine, and a missing IXamlMetadataProvider entry is not the discriminator (the
        // Button subclass has none either). WinUI platform issue; re-enable once such subclasses can be laid out here.
        [TestProperty("Ignore", "True")]
        public void TreeViewInheritanceTest()
        {
            RunOnUIThread.Execute(() =>
            {
                StackPanel stackPanel = new StackPanel();
                SpecialTreeView inheritedTreeView = new SpecialTreeView();
                SpecialTreeViewList inheritedTreeViewList = new SpecialTreeViewList();
                SpecialTreeViewItem inheritedTreeViewItem = new SpecialTreeViewItem();
                SpecialTreeViewNode inheritedTreeViewNode = new SpecialTreeViewNode();
                IList<string> data = Enumerable.Range(0, 5).Select(x => "Item " + x).ToList();

                Verify.IsNotNull(stackPanel);
                stackPanel.Children.Add(inheritedTreeView);
                stackPanel.Children.Add(inheritedTreeViewList);
                inheritedTreeViewList.ItemsSource = data;
                Content = stackPanel;
                Content.UpdateLayout();
            });
        }

        // Scenario: Default TreeView placed in the visual tree and laid out.
        // Expected: TreeView.IsTabStop is false (focus goes to the inner list/items, not the TreeView itself).
        // Failure means: The TreeView constructor/default style regressed and made the TreeView itself a tab stop.
        [TestMethod]
        public void VerifyTreeViewIsNotTabStop()
        {
            RunOnUIThread.Execute(() =>
            {
                var treeView = new TreeView();
                Content = treeView;
                Content.UpdateLayout();
                Verify.IsFalse(treeView.IsTabStop);
            });
        }

        // Scenario: Node mode; childless root node in a laid-out TreeView; Children.Clear() on it while a
        //   VectorChanged handler throws on Reset.
        // Expected: One list item; clearing an empty Children vector is a no-op (no Reset raised, no crash).
        // Failure means: TreeViewNodeVector::Clear raises a Reset for an already-empty vector, triggering needless
        //   ViewModel/handler work (here an exception).
        [TestMethod]
        public void VerifyClearingNodeWithNoChildren()
        {
            RunOnUIThread.Execute(() =>
            {
                var treeViewNode1 = new TreeViewNode();
                var treeView = new TreeView();

                Content = treeView;
                Content.UpdateLayout();
                var listControl = VisualTreeUtils.FindVisualChildByName(treeView, "ListControl") as TreeViewList;
                treeView.RootNodes.Add(treeViewNode1);
                var children = (treeViewNode1.Children as IObservableVector<TreeViewNode>);
                children.VectorChanged += (vector, args) =>
                {
                    if (((IVectorChangedEventArgs)args).CollectionChange == CollectionChange.Reset)
                    {
                        // should not reset if there are not children items
                        throw new InvalidOperationException();
                    }
                };
                Verify.AreEqual(listControl.Items.Count, 1);

                // this should no-op and not crash
                treeViewNode1.Children.Clear();
            });
        }

        // Scenario: Detached expanded root node with one child; reads values through GetValue on the DPs.
        // Expected: ContentProperty, DepthProperty (-1 root / 0 child), IsExpandedProperty and HasChildrenProperty
        //   report the expected per-node values.
        // Failure means: TreeViewNode DP registration (TreeViewNode.properties.cpp) or the Depth/HasChildren
        //   updates no longer keep the DP values current.
        [TestMethod]
        public void TreeViewNodeDPTest()
        {
            RunOnUIThread.Execute(() =>
            {
                TreeViewNode rootNode = new TreeViewNode() { Content = "Root" };
                TreeViewNode childNode = new TreeViewNode() { Content = "Child" };
                rootNode.Children.Add(childNode);
                rootNode.IsExpanded = true;

                Verify.AreEqual((string)rootNode.GetValue(TreeViewNode.ContentProperty), "Root");
                Verify.AreEqual((string)childNode.GetValue(TreeViewNode.ContentProperty), "Child");

                Verify.AreEqual((int)rootNode.GetValue(TreeViewNode.DepthProperty), -1);
                Verify.AreEqual((int)childNode.GetValue(TreeViewNode.DepthProperty), 0);

                Verify.AreEqual((bool)rootNode.GetValue(TreeViewNode.IsExpandedProperty), true);
                Verify.AreEqual((bool)childNode.GetValue(TreeViewNode.IsExpandedProperty), false);

                Verify.AreEqual((bool)rootNode.GetValue(TreeViewNode.HasChildrenProperty), true);
                Verify.AreEqual((bool)childNode.GetValue(TreeViewNode.HasChildrenProperty), false);
            });
        }

        // Scenario: Node mode; laid-out TreeView, set ItemTemplate (XAML DataTemplate), add a node, lay out.
        // Expected: The realized TreeViewItem's ContentTemplate is the TreeView.ItemTemplate instance.
        // Failure means: TreeView.ItemTemplate no longer flows (via the default template binding) to the inner
        //   TreeViewList and its containers.
        [TestMethod]
        public void TreeViewItemTemplateTest()
        {
            RunOnUIThread.Execute(() =>
            {
                // The assertions previously lived in a Loaded handler on a TreeView that was never
                // added to the tree, so they never ran. Load the control and assert directly.
                TreeView treeView = new TreeView() { Width = 400, Height = 400 };
                Content = treeView;
                Content.UpdateLayout();

                var dataTemplate = (DataTemplate)XamlReader.Load(
               @"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'> 
                      <TextBlock Text='TreeViewItemTemplate'/>
                  </DataTemplate>");
                treeView.ItemTemplate = dataTemplate;
                var node = new TreeViewNode();
                treeView.RootNodes.Add(node);
                Content.UpdateLayout();
                var listControl = VisualTreeUtils.FindVisualChildByName(treeView, "ListControl") as TreeViewList;
                var treeViewItem = listControl.ContainerFromItem(node) as TreeViewItem;
                Verify.IsNotNull(treeViewItem);
                Verify.AreEqual(treeViewItem.ContentTemplate, dataTemplate);
            });
        }

        // Scenario: Content mode (ItemsSource = [5]); then sets the realized TreeViewItem's own ItemsSource.
        // Expected: GlyphOpacity is 0.0 for the leaf and 1.0 once the container has a non-empty ItemsSource.
        // Failure means: TreeViewItem::OnPropertyChanged(ItemsSource)/SetItemsSource no longer updates the node's
        //   children/HasChildren, so the chevron stays hidden.
        [TestMethod]
        public void ValidateTreeViewItemSourceChangeUpdatesChevronOpacity()
        {
            RunOnUIThread.Execute(() =>
            {
                var treeView = new TreeView();
                var collection = new ObservableCollection<int>();
                collection.Add(5);
                treeView.ItemsSource = collection;
                Content = treeView;
                Content.UpdateLayout();
                var tvi = (TreeViewItem)treeView.ContainerFromItem(5);
                Verify.AreEqual(tvi.GlyphOpacity, 0.0);
                tvi.ItemsSource = collection;
                Content.UpdateLayout();
                Verify.AreEqual(tvi.GlyphOpacity, 1.0);
            });
        }

        // Scenario: Node mode; set ItemContainerStyle (Background=Green) on a laid-out TreeView, add a node.
        // Expected: The realized TreeViewItem's Style is the same Style instance.
        // Failure means: TreeView.ItemContainerStyle no longer flows to the inner TreeViewList and its containers.
        [TestMethod]
        public void TreeViewItemContainerStyleTest()
        {
            RunOnUIThread.Execute(() =>
            {
                // Assertions previously lived in a Loaded handler that never fired; assert directly.
                TreeView treeView = new TreeView() { Width = 400, Height = 400 };
                Content = treeView;
                Content.UpdateLayout();

                var style = (Style)XamlReader.Load(
               @"<Style xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' TargetType='TreeViewItem'> 
                      <Setter Property='Background' Value='Green'/>
                  </Style>");
                treeView.ItemContainerStyle = style;
                var node = new TreeViewNode();
                treeView.RootNodes.Add(node);
                Content.UpdateLayout();
                var listControl = VisualTreeUtils.FindVisualChildByName(treeView, "ListControl") as TreeViewList;
                var treeViewItem = listControl.ContainerFromItem(node) as TreeViewItem;
                Verify.IsNotNull(treeViewItem);
                Verify.AreEqual(treeViewItem.Style, style);
            });
        }

        // Scenario: Node mode; set ItemContainerTransitions (one ContentThemeTransition), add a node, lay out.
        // Expected: Container realized; TreeView and inner ListControl share the same TransitionCollection
        //   holding one ContentThemeTransition.
        // Failure means: The TreeView default template no longer binds ItemContainerTransitions to TreeViewList.
        [TestMethod]
        public void TreeViewItemContainerTransitionTest()
        {
            RunOnUIThread.Execute(() =>
            {
                // Assertions previously lived in a Loaded handler that never fired; assert directly.
                TreeView treeView = new TreeView() { Width = 400, Height = 400 };
                Content = treeView;
                Content.UpdateLayout();

                var transition = (TransitionCollection)XamlReader.Load(
               @"<TransitionCollection xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'> 
                        <ContentThemeTransition />
                  </TransitionCollection>");
                treeView.ItemContainerTransitions = transition;
                var node = new TreeViewNode();
                treeView.RootNodes.Add(node);
                Content.UpdateLayout();
                var listControl = VisualTreeUtils.FindVisualChildByName(treeView, "ListControl") as TreeViewList;
                var treeViewItem = listControl.ContainerFromItem(node) as TreeViewItem;
                Verify.IsNotNull(treeViewItem);
                // ItemContainerTransitions is template-bound onto the list control (it is not copied to
                // each container's ContentTransitions).
                Verify.AreEqual(transition, treeView.ItemContainerTransitions);
                Verify.AreEqual(transition, listControl.ItemContainerTransitions);
                Verify.AreEqual(1, listControl.ItemContainerTransitions.Count);
                Verify.IsTrue(listControl.ItemContainerTransitions[0] is ContentThemeTransition);
            });
        }

        // Scenario: Content mode; ItemsSource = CreateTreeViewItemsSource() (2 root items), no layout.
        // Expected: 2 RootNodes are created and RootNodes[0].Content is items[0].
        // Failure means: TreeViewNode::ItemsSource/SyncChildrenNodesWithItemsSource no longer creates root nodes
        //   from the data.
        [TestMethod]
        public void TreeViewItemsSourceTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var treeView = new TreeView();
                var items = CreateTreeViewItemsSource();
                treeView.ItemsSource = items;

                Verify.AreEqual(treeView.RootNodes.Count, 2);
                Verify.AreEqual(treeView.RootNodes[0].Content as TreeViewItemSource, items[0]);
            });
        }

        // Scenario: Content mode; Add, Remove, Replace (items[1] = ...) and Clear on the ObservableCollection.
        // Expected: RootNodes count 3 -> 2; replaced node carries the new content; Clear leaves 0 RootNodes.
        // Failure means: TreeViewNode::OnItemsSourceChanged no longer mirrors Add/Remove/Replace/Reset collection
        //   changes into RootNodes.
        [TestMethod]
        public void TreeViewItemsSourceUpdateTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var treeView = new TreeView();
                var items = CreateTreeViewItemsSource();
                treeView.ItemsSource = items;

                // Insert
                var newItem = new TreeViewItemSource() { Content = "newItem" };
                items.Add(newItem);
                Verify.AreEqual(treeView.RootNodes.Count, 3);
                var itemFromNode = treeView.RootNodes[2].Content as TreeViewItemSource;
                Verify.AreEqual(newItem.Content, itemFromNode.Content);

                // Remove
                items.Remove(newItem);
                Verify.AreEqual(treeView.RootNodes.Count, 2);

                // Replace
                var item3 = new TreeViewItemSource() { Content = "3" };
                items[1] = item3;
                itemFromNode = treeView.RootNodes[1].Content as TreeViewItemSource;
                Verify.AreEqual(item3.Content, itemFromNode.Content);

                // Clear
                items.Clear();
                Verify.AreEqual(treeView.RootNodes.Count, 0);

            });
        }

        // Scenario: object.ToString() on a TreeViewNode (Content "Node") and on a managed subclass TreeViewNode2.
        // Expected: ToString() returns the node's Content for both.
        // Failure means: Ignored (PC-1): C#/WinRT object.ToString() returns the class name because TreeView.idl
        //   does not declare IStringable on TreeViewNode; a pass would mean the IDL now declares IStringable
        //   (re-enable). The native IStringable contract is covered by
        //   TreeViewNodeNativeIStringableReturnsContent.
        [TestMethod]
        // TODO 29556921: Since MUXControlsTestApp became a desktop C#/WinRT app, object.ToString() on a TreeViewNode returns
        // the runtime class name ("Microsoft.UI.Xaml.Controls.TreeViewNode", "MUXControlsTestApp.TreeViewNode2") instead of
        // Content: TreeView.idl does not declare Windows.Foundation.IStringable on TreeViewNode (it is only implemented
        // natively), and C#/WinRT routes ToString() only to declared IStringable. The native IStringable contract is
        // covered by TreeViewNodeNativeIStringableReturnsContent. Re-enable if TreeViewNode declares IStringable.
        [TestProperty("Ignore", "True")]
        public void TreeViewNodeStringableTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var node = new TreeViewNode() { Content = "Node" };
                Verify.AreEqual(node.Content, node.ToString());

                // Test inherited type
                var node2 = new TreeViewNode2() { Content = "Inherited from TreeViewNode" };
                Verify.AreEqual(node2.Content, node2.ToString());
            });
        }

        // Scenario: Casts TreeViewNodes (string Content, then int 42; managed subclass TreeViewNode2) to
        //   Windows.Foundation.IStringable and calls ToString().
        // Expected: "Node", then "42", and the subclass returns its string Content.
        // Failure means: TreeViewNode::ToString (native IStringable) no longer returns the stringified Content.
        [TestMethod]
        public void TreeViewNodeNativeIStringableReturnsContent()
        {
            RunOnUIThread.Execute(() =>
            {
                var node = new TreeViewNode() { Content = "Node" };
                var stringable = (global::Windows.Foundation.IStringable)node;
                Verify.AreEqual("Node", stringable.ToString());

                node.Content = 42;
                Verify.AreEqual("42", ((global::Windows.Foundation.IStringable)node).ToString());

                // A managed subclass reaches the same native IStringable implementation.
                var inherited = new TreeViewNode2() { Content = "Inherited from TreeViewNode" };
                Verify.AreEqual("Inherited from TreeViewNode", ((global::Windows.Foundation.IStringable)inherited).ToString());
            });
        }

        // Scenario: Node mode, Multiple selection; node1 added to SelectedNodes before the TreeView is templated.
        // Expected: After layout node1's multi-select check box is checked and node2's is not.
        // Failure means: SelectedNodes set before TreeView::OnApplyTemplate are no longer applied to the ViewModel
        //   and the realized containers' check boxes.
        [TestMethod]
        public void TreeViewPendingSelectedNodesTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var treeView = new TreeView();
                treeView.SelectionMode = TreeViewSelectionMode.Multiple;

                var node1 = new TreeViewNode() { Content = "Node1" };
                var node2 = new TreeViewNode() { Content = "Node2" };
                treeView.RootNodes.Add(node1);
                treeView.RootNodes.Add(node2);
                treeView.SelectedNodes.Add(node1);

                Content = treeView;
                Content.UpdateLayout();
                Verify.AreEqual(true, IsMultiSelectCheckBoxChecked(treeView, node1));
                Verify.AreEqual(false, IsMultiSelectCheckBoxChecked(treeView, node2));
            });
        }

        // Scenario: Node mode TreeView (400x400, one node) set as visual-tree root and compared against the stored
        //   "TreeView" visual-tree master; gated by TestPass:MaxOSVer 22H2.
        // Expected: The dumped visual tree (elements and properties) matches the master file.
        // Failure means: The TreeView default template/visual tree (or theme resources) changed vs the master.
        [TestMethod]
        [TestProperty("TestPass:MaxOSVer", WindowsOSVersion._22H2)]    // This test is currently failing on 23h2.
        public void VerifyVisualTree()
        {
            TreeView treeView = null;
            RunOnUIThread.Execute(() =>
            {
                treeView = new TreeView() { Width = 400, Height = 400 };
                var node1 = new TreeViewNode() { Content = "Node1" };
                treeView.RootNodes.Add(node1);
            });
            TestUtilities.SetAsVisualTreeRoot(treeView);

            VisualTreeTestHelper.VerifyVisualTree(root: treeView, verificationFileNamePrefix: "TreeView");
        }

        // Scenario: Content mode (ItemsSource [1,2,3]), Single selection; toggles TreeViewItem.IsSelected on
        //   items 1/2, then sets ItemsSource = null.
        // Expected: SelectionChanged fires each time with the exact Added/RemovedItems (data items), incl.
        //   RemovedItems = [1] when ItemsSource is cleared.
        // Failure means: TreeView::OnListControlSelectionChanged/RaiseSelectionChanged report wrong Added/Removed
        //   items or stop raising SelectionChanged when the source is cleared.
        [TestMethod]
        public void TreeViewSelectionChangedSingleMode()
        {
            RunOnUIThread.Execute(() =>
            {
                // input data:
                // - 1
                // - 2
                // - 3
                TreeViewSelectionChangedEventArgs selectionChangedEventArgs = null;

                var treeView = new TreeView { SelectionMode = TreeViewSelectionMode.Single };
                treeView.SelectionChanged += (s, e1) => selectionChangedEventArgs = e1;

                var collection = new ObservableCollection<int> { 1, 2, 3 };
                treeView.ItemsSource = collection;
                Content = treeView;
                Content.UpdateLayout();
                var tvi1 = (TreeViewItem)treeView.ContainerFromItem(1);
                var tvi2 = (TreeViewItem)treeView.ContainerFromItem(2);

                tvi1.IsSelected = true;

                Verify.IsNotNull(selectionChangedEventArgs);
                Verify.AreEqual(1, selectionChangedEventArgs.AddedItems.Count);
                Verify.IsTrue(selectionChangedEventArgs.AddedItems.Contains(1));
                Verify.AreEqual(0, selectionChangedEventArgs.RemovedItems.Count);
                selectionChangedEventArgs = null;

                tvi2.IsSelected = true;

                Verify.IsNotNull(selectionChangedEventArgs);
                Verify.AreEqual(1, selectionChangedEventArgs.AddedItems.Count);
                Verify.IsTrue(selectionChangedEventArgs.AddedItems.Contains(2));
                Verify.AreEqual(1, selectionChangedEventArgs.RemovedItems.Count);
                Verify.IsTrue(selectionChangedEventArgs.RemovedItems.Contains(1));
                selectionChangedEventArgs = null;

                tvi2.IsSelected = false;

                Verify.IsNotNull(selectionChangedEventArgs);
                Verify.AreEqual(0, selectionChangedEventArgs.AddedItems.Count);
                Verify.AreEqual(1, selectionChangedEventArgs.RemovedItems.Count);
                Verify.IsTrue(selectionChangedEventArgs.RemovedItems.Contains(2));
                selectionChangedEventArgs = null;

                tvi1.IsSelected = true;

                Verify.IsNotNull(selectionChangedEventArgs);
                Verify.AreEqual(1, selectionChangedEventArgs.AddedItems.Count);
                Verify.IsTrue(selectionChangedEventArgs.AddedItems.Contains(1));
                Verify.AreEqual(0, selectionChangedEventArgs.RemovedItems.Count);
                selectionChangedEventArgs = null;

                treeView.ItemsSource = null;

                Verify.IsNotNull(selectionChangedEventArgs);
                Verify.AreEqual(0, selectionChangedEventArgs.AddedItems.Count);
                Verify.AreEqual(1, selectionChangedEventArgs.RemovedItems.Count);
                Verify.IsTrue(selectionChangedEventArgs.RemovedItems.Contains(1));
            });
        }

        // Scenario: Node mode, Multiple selection, pre-expanded 1(11,12,13), 2(21), 3; select 1, set 11 selected
        //   then unselected, then SelectAll().
        // Expected: AddedItems = 1 + its 3 children; then RemovedItems = [11] only; SelectAll adds all 7 nodes.
        // Failure means: ViewModel::UpdateSelection/SelectAll no longer propagate selection to descendants or
        //   batch Added/RemovedItems into a single TreeView.SelectionChanged.
        [TestMethod]
        public void TreeViewSelectionChangedMultipleMode()
        {
            RunOnUIThread.Execute(() =>
            {
                // input data:
                // - 1
                //   - 11
                //   - 12
                //   - 13
                // - 2
                //   - 21
                // - 3
                TreeViewSelectionChangedEventArgs selectionChangedEventArgs = null;

                var treeView = new TreeView { SelectionMode = TreeViewSelectionMode.Multiple };
                treeView.SelectionChanged += (s, e) =>
                {
                    selectionChangedEventArgs = e;
                };

                var node1 = new TreeViewNode { Content = "1", IsExpanded = true };
                var node11 = new TreeViewNode { Content = "11" };
                var node12 = new TreeViewNode { Content = "12" };
                var node13 = new TreeViewNode { Content = "13" };
                node1.Children.Add(node11);
                node1.Children.Add(node12);
                node1.Children.Add(node13);

                var node2 = new TreeViewNode { Content = "2", IsExpanded = true };
                var node21 = new TreeViewNode { Content = "21" };
                node2.Children.Add(node21);

                var node3 = new TreeViewNode { Content = "3" };

                treeView.RootNodes.Add(node1);
                treeView.RootNodes.Add(node2);
                treeView.RootNodes.Add(node3);
                Content = treeView;
                Content.UpdateLayout();

                var tvi1 = (TreeViewItem)treeView.ContainerFromItem(node1);
                var tvi11 = (TreeViewItem)treeView.ContainerFromItem(node11);
                var tvi12 = (TreeViewItem)treeView.ContainerFromItem(node12);
                var tvi13 = (TreeViewItem)treeView.ContainerFromItem(node13);
                var tvi2 = (TreeViewItem)treeView.ContainerFromItem(node2);
                var tvi21 = (TreeViewItem)treeView.ContainerFromItem(node21);
                var tvi3 = (TreeViewItem)treeView.ContainerFromItem(node3);

                // - 1         selected
                //   - 11      selected
                //   - 12      selected
                //   - 13      selected
                // - 2
                //   - 21
                // - 3
                tvi1.IsSelected = true;

                Verify.IsNotNull(selectionChangedEventArgs);
                Verify.AreEqual(4, selectionChangedEventArgs.AddedItems.Count);
                Verify.IsTrue(selectionChangedEventArgs.AddedItems.Contains(node1));
                Verify.IsTrue(selectionChangedEventArgs.AddedItems.Contains(node11));
                Verify.IsTrue(selectionChangedEventArgs.AddedItems.Contains(node12));
                Verify.IsTrue(selectionChangedEventArgs.AddedItems.Contains(node13));
                Verify.AreEqual(0, selectionChangedEventArgs.RemovedItems.Count);
                selectionChangedEventArgs = null;

                // - 1         selected
                //   - 11
                //   - 12      selected
                //   - 13      selected
                // - 2
                //   - 21
                // - 3
                tvi11.IsSelected = true;
                tvi11.IsSelected = false;

                Verify.IsNotNull(selectionChangedEventArgs);
                Verify.AreEqual(0, selectionChangedEventArgs.AddedItems.Count);
                Verify.AreEqual(1, selectionChangedEventArgs.RemovedItems.Count);
                Verify.IsTrue(selectionChangedEventArgs.RemovedItems.Contains(node11));
                selectionChangedEventArgs = null;

                // - 1         selected
                //   - 11      selected
                //   - 12      selected
                //   - 13      selected
                // - 2         selected
                //   - 21      selected
                // - 3         selected
                treeView.SelectAll();
                Verify.IsNotNull(selectionChangedEventArgs);
                var items = selectionChangedEventArgs.AddedItems.ToList();
                Verify.AreEqual(7, selectionChangedEventArgs.AddedItems.Count);
                Verify.IsTrue(selectionChangedEventArgs.AddedItems.Contains(node1));
                Verify.IsTrue(selectionChangedEventArgs.AddedItems.Contains(node11));
                Verify.IsTrue(selectionChangedEventArgs.AddedItems.Contains(node12));
                Verify.IsTrue(selectionChangedEventArgs.AddedItems.Contains(node13));
                Verify.IsTrue(selectionChangedEventArgs.AddedItems.Contains(node2));
                Verify.IsTrue(selectionChangedEventArgs.AddedItems.Contains(node21));
                Verify.IsTrue(selectionChangedEventArgs.AddedItems.Contains(node3));
                Verify.AreEqual(0, selectionChangedEventArgs.RemovedItems.Count);
            });
        }

        // Scenario: Detached node mode: expanded non-root node with children; Children.Clear(), Remove(last child)
        //   and RemoveAt(0) of the last child, re-expanding between steps.
        // Expected: IsExpanded becomes false each time the node loses its last child.
        // Failure means: TreeViewNodeVector::Clear/RemoveAt (updateIsExpanded) no longer auto-collapse a non-root
        //   parent whose last child is removed.
        [TestMethod]
        public void RemovingLastChildrenSetsIsExpandedToFalse()
        {
            RunOnUIThread.Execute(() =>
            {
                var treeViewRoot = new TreeViewNode();
                var treeViewNode1 = new TreeViewNode();
                var treeViewNode2 = new TreeViewNode();
                var treeViewNode3 = new TreeViewNode();
                var treeViewNode4 = new TreeViewNode();
                var treeViewNode5 = new TreeViewNode();

                // Need root since in TreeView we otherwise 
                // could collapse whole tree and hide it forever
                treeViewRoot.Children.Add(treeViewNode1);

                treeViewNode1.Children.Add(treeViewNode2);
                treeViewNode1.Children.Add(treeViewNode3);
                treeViewNode1.Children.Add(treeViewNode4);
                treeViewNode1.Children.Add(treeViewNode5);

                treeViewNode1.IsExpanded = true;
                Verify.IsTrue(treeViewNode1.IsExpanded);

                treeViewNode1.Children.Clear();
                Verify.IsFalse(treeViewNode1.IsExpanded);

                treeViewNode1.Children.Add(treeViewNode2);
                treeViewNode1.IsExpanded = true;
                Verify.IsTrue(treeViewNode1.IsExpanded);

                treeViewNode1.Children.Remove(treeViewNode2);
                Verify.IsFalse(treeViewNode1.IsExpanded);

                treeViewNode1.Children.Add(treeViewNode2);
                treeViewNode1.IsExpanded = true;
                Verify.IsTrue(treeViewNode1.IsExpanded);

                treeViewNode1.Children.RemoveAt(0);
                Verify.IsFalse(treeViewNode1.IsExpanded);
            });
        }

        // Scenario: Content mode; root container ItemsSource [1,2], expand, RemoveAt both via the data source
        //   (Remove notifications), then Add one back. Regression for GitHub issue #10251.
        // Expected: 0 children with IsExpanded still true and HasChildren false; after Add: 1 child, still
        //   expanded, HasChildren true.
        // Failure means: ItemsSource-driven removals (TreeViewNode::OnItemsSourceChanged/RemoveFromChildrenNodes,
        //   RemoveAt without updateIsExpanded) collapse the parent again (#10251 regression).
        [TestMethod]
        public void RemovingLastChildFromItemsSourcePreservesIsExpanded()
        {
            RunOnUIThread.Execute(() =>
            {
                var treeView = new TreeView();
                var rootData = new ObservableCollection<int> { 0 };
                treeView.ItemsSource = rootData;
                Content = treeView;
                Content.UpdateLayout();

                var tvi = (TreeViewItem)treeView.ContainerFromItem(0);
                var childrenData = new ObservableCollection<int> { 1, 2 };
                tvi.ItemsSource = childrenData;
                Content.UpdateLayout();

                var parentNode = treeView.RootNodes[0];
                Verify.AreEqual(2, (int)parentNode.Children.Count);

                parentNode.IsExpanded = true;
                Verify.IsTrue(parentNode.IsExpanded);
                Verify.IsTrue(parentNode.HasChildren);

                // Remove children one at a time until empty using RemoveAt.
                // This emits Remove (not Reset) notifications on the data source.
                childrenData.RemoveAt(1);
                childrenData.RemoveAt(0);
                Content.UpdateLayout();

                Verify.AreEqual(0, (int)parentNode.Children.Count);
                // Regression: an ItemsSource-driven removal of the last child must
                // not collapse the parent (see GitHub issue #10251).
                Verify.IsTrue(parentNode.IsExpanded);
                // HasChildren (glyph) and IsExpanded (state) are independent: the
                // chevron hides while empty even though the expansion state persists.
                Verify.IsFalse(parentNode.HasChildren);

                // Adding a child back keeps the parent expanded and re-shows the glyph.
                childrenData.Add(3);
                Content.UpdateLayout();
                Verify.AreEqual(1, (int)parentNode.Children.Count);
                Verify.IsTrue(parentNode.IsExpanded);
                Verify.IsTrue(parentNode.HasChildren);
            });
        }

        // Scenario: Default laid-out TreeView; sets TreeView.Background to a green SolidColorBrush.
        // Expected: Inner ListControl background is not green by default and becomes green after the set.
        // Failure means: The default TreeView template no longer TemplateBinds Background onto the TreeViewList.
        [TestMethod]
        public void TreeViewBackgroundTest()
        {
            RunOnUIThread.Execute(() =>
            {
                var treeView = new TreeView();

                Content = treeView;
                Content.UpdateLayout();

                var treeViewList = VisualTreeUtils.FindVisualChildByName(treeView, "ListControl") as TreeViewList;

                // Make sure the TreeViewList does not already have the background color we are using for this test.
                bool testCondition = treeViewList.Background is SolidColorBrush brush && brush.Color == Colors.Green;
                Verify.IsFalse(testCondition, "The default TreeView background color should not match the test color used.");

                // Check if the Background API affects the TreeView control.
                treeView.Background = new SolidColorBrush(Colors.Green);
                Content.UpdateLayout();

                testCondition = treeViewList.Background is SolidColorBrush brush2 && brush2.Color == Colors.Green;
                Verify.IsTrue(testCondition, "The TreeView background UI should have matched the specified test color.");
            });
        }

        // Scenario: Sets SelectedItem on an untemplated TreeView before and after assigning ItemsSource (strings).
        // Expected: No exception; the SelectedItem getter returns the value just set each time.
        // Failure means: TreeView SelectedItem handling (TreeView::OnPropertyChanged) crashes or drops the value
        //   when there is no list control yet.
        [TestMethod]
        public void VerifySettingSelectionDoesNotCrash()
        {
            RunOnUIThread.Execute(() =>
            {
                var treeview = new TreeView();

                var items = new ObservableCollection<string>() { 
                    "Item0", "Item1", "Item2"
                };

                treeview.SelectedItem = items[0];
                Verify.AreEqual(treeview.SelectedItem, items[0]);

                treeview.ItemsSource = items;
                treeview.SelectedItem = items[1];
                Verify.AreEqual(treeview.SelectedItem, items[1]);
            });
        }

        // Scenario: Content mode via XAML (Multiple; ItemTemplate TreeViewItem binding IsExpanded/ItemsSource;
        //   3x3x3 data); adds/removes items via both SelectedItems and SelectedNodes.
        // Expected: After each edit SelectedNodes and SelectedItems match the expected set, incl. whole-subtree
        //   selection of a parent and the parent dropping out when one of its children is deselected.
        // Failure means: SelectedItemsVector/SelectedTreeNodeVector (ViewModel.cpp) no longer keep SelectedItems
        //   and SelectedNodes in sync or propagate selection to descendants/ancestors.
        [TestMethod]
        public void SelectedItemsTest()
        {
            ObservableCollection<TreeViewItemSource> data = null;
            TreeView treeview = null;

            RunOnUIThread.Execute(() =>
            {
                data = CreateItems();

                string treeViewXamlStr = @"
                <TreeView 
                        xmlns=""http://schemas.microsoft.com/winfx/2006/xaml/presentation""
                        xmlns:x=""http://schemas.microsoft.com/winfx/2006/xaml""
                        Name=""DessertTree""
                        SelectionMode=""Multiple""
                        ItemsSource=""{Binding}"">
                    <TreeView.ItemTemplate>
                        <DataTemplate>
                            <TreeViewItem
                                IsExpanded=""True""
                                ItemsSource=""{Binding Children}""
                                Content=""{Binding Content}""/>
                        </DataTemplate>
                    </TreeView.ItemTemplate>
                </TreeView>";

                treeview = XamlReader.Load(treeViewXamlStr) as TreeView;
                treeview.DataContext = data;

                Content = treeview;
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Add 1.1 to SelectedItems");
                treeview.SelectedItems.Add(data[1].Children[1]);
                VerifySelectedNodesAndSelectedItems(treeview, new[] { "1.1", "1.1.0", "1.1.1", "1.1.2" });

                Log.Comment("Add 1.2.1 to SelectedItems");
                treeview.SelectedItems.Add(data[1].Children[2].Children[1]);
                VerifySelectedNodesAndSelectedItems(treeview, new[] { "1.1", "1.1.0", "1.1.1", "1.1.2", "1.2.1" });

                Log.Comment("Add 0.2 to SelectedNodes");
                treeview.SelectedNodes.Add(treeview.RootNodes[0].Children[2]);
                VerifySelectedNodesAndSelectedItems(treeview, new[] { "1.1", "1.1.0", "1.1.1", "1.1.2", "1.2.1", "0.2", "0.2.0", "0.2.1", "0.2.2" });

                Log.Comment("Remove 0.2.1 from SelectedItems");
                treeview.SelectedItems.Remove(data[0].Children[2].Children[1]);
                VerifySelectedNodesAndSelectedItems(treeview, new[] { "1.1", "1.1.0", "1.1.1", "1.1.2", "1.2.1", "0.2.0", "0.2.2" });

                Log.Comment("Remove 1.1 from SelectedNodes");
                treeview.SelectedNodes.Remove(treeview.RootNodes[1].Children[1]);
                VerifySelectedNodesAndSelectedItems(treeview, new[] { "1.2.1", "0.2.0", "0.2.2" });
            });
        }

        // Builds A(A0, A1), B in a sized, laid out TreeView. Must be called on the UI thread.
        private TreeView CreateLaidOutTreeView(
            out TreeViewNode nodeA, out TreeViewNode nodeA0, out TreeViewNode nodeA1, out TreeViewNode nodeB)
        {
            var treeView = new TreeView() { Width = 400, Height = 400 };
            nodeA = new TreeViewNode() { Content = "A" };
            nodeA0 = new TreeViewNode() { Content = "A0" };
            nodeA1 = new TreeViewNode() { Content = "A1" };
            nodeB = new TreeViewNode() { Content = "B" };
            nodeA.Children.Add(nodeA0);
            nodeA.Children.Add(nodeA1);
            treeView.RootNodes.Add(nodeA);
            treeView.RootNodes.Add(nodeB);
            Content = treeView;
            Content.UpdateLayout();
            return treeView;
        }

        private TreeViewItemAutomationPeer GetItemPeer(TreeView treeView, TreeViewNode node)
        {
            var container = treeView.ContainerFromNode(node) as TreeViewItem;
            Verify.IsNotNull(container, "Container should be realized");
            var peer = FrameworkElementAutomationPeer.CreatePeerForElement(container) as TreeViewItemAutomationPeer;
            Verify.IsNotNull(peer, "TreeViewItem should create a TreeViewItemAutomationPeer");
            return peer;
        }

        // Scenario: Node mode A(A0,A1),B laid out; Expand(A); TreeViewItemAutomationPeer for each container.
        // Expected: PositionInSet/SizeOfSet/Level: A 1/2/1, B 2/2/1, A0 1/2/2, A1 2/2/2.
        // Failure means: TreeViewItemAutomationPeer::GetPositionInSetCore/GetSizeOfSetCore/GetLevelCore compute
        //   wrong UIA set/level values.
        [TestMethod]
        public void TreeViewItemPeerReportsPositionSizeAndLevel()
        {
            RunOnUIThread.Execute(() =>
            {
                var treeView = CreateLaidOutTreeView(out var nodeA, out var nodeA0, out var nodeA1, out var nodeB);
                treeView.Expand(nodeA);
                Content.UpdateLayout();

                var peerA = GetItemPeer(treeView, nodeA);
                Verify.AreEqual(1, peerA.GetPositionInSet());
                Verify.AreEqual(2, peerA.GetSizeOfSet());
                Verify.AreEqual(1, peerA.GetLevel());

                var peerB = GetItemPeer(treeView, nodeB);
                Verify.AreEqual(2, peerB.GetPositionInSet());
                Verify.AreEqual(2, peerB.GetSizeOfSet());
                Verify.AreEqual(1, peerB.GetLevel());

                var peerA0 = GetItemPeer(treeView, nodeA0);
                Verify.AreEqual(1, peerA0.GetPositionInSet());
                Verify.AreEqual(2, peerA0.GetSizeOfSet());
                Verify.AreEqual(2, peerA0.GetLevel());

                var peerA1 = GetItemPeer(treeView, nodeA1);
                Verify.AreEqual(2, peerA1.GetPositionInSet());
                Verify.AreEqual(2, peerA1.GetSizeOfSet());
                Verify.AreEqual(2, peerA1.GetLevel());
            });
        }

        // Scenario: Node mode A(A0,A1),B; automation peer for A's container.
        // Expected: ControlType TreeItem, ClassName "Microsoft.UI.Xaml.Controls.TreeViewItem"; the ExpandCollapse
        //   pattern is returned and is the peer itself.
        // Failure means: TreeViewItemAutomationPeer::GetAutomationControlTypeCore/GetClassNameCore/GetPatternCore
        //   regressed.
        [TestMethod]
        public void TreeViewItemPeerIdentity()
        {
            RunOnUIThread.Execute(() =>
            {
                var treeView = CreateLaidOutTreeView(out var nodeA, out var nodeA0, out var nodeA1, out var nodeB);
                var peerA = GetItemPeer(treeView, nodeA);

                Verify.AreEqual(AutomationControlType.TreeItem, peerA.GetAutomationControlType());
                Verify.AreEqual("Microsoft.UI.Xaml.Controls.TreeViewItem", peerA.GetClassName());
                Verify.IsNotNull(peerA.GetPattern(PatternInterface.ExpandCollapse));
                Verify.AreEqual(peerA, peerA.GetPattern(PatternInterface.ExpandCollapse));
            });
        }

        // Scenario: Node mode A(A0,A1),B; AutomationProperties.Name set on B's container.
        // Expected: B's peer GetName() returns "Explicit name".
        // Failure means: TreeViewItemAutomationPeer::GetNameCore no longer prefers an explicit
        //   AutomationProperties.Name over the node content.
        [TestMethod]
        public void TreeViewItemPeerNameUsesExplicitAutomationName()
        {
            RunOnUIThread.Execute(() =>
            {
                var treeView = CreateLaidOutTreeView(out var nodeA, out var nodeA0, out var nodeA1, out var nodeB);
                var containerB = treeView.ContainerFromNode(nodeB) as TreeViewItem;
                AutomationProperties.SetName(containerB, "Explicit name");

                var peerB = GetItemPeer(treeView, nodeB);
                Verify.AreEqual("Explicit name", peerB.GetName());
            });
        }

        // Scenario: Node mode; peers for node "A", a node with null Content and a node with "" Content.
        // Expected: Names "A", "Microsoft.UI.Xaml.Controls.TreeViewNode" (null) and "TreeViewNode" (empty string).
        // Failure means: TreeViewItemAutomationPeer::GetNameCore fallbacks (node string representation, then
        //   "TreeViewNode" for an empty name) regressed.
        [TestMethod]
        public void TreeViewItemPeerNameFallsBackToNodeContent()
        {
            RunOnUIThread.Execute(() =>
            {
                var treeView = CreateLaidOutTreeView(out var nodeA, out var nodeA0, out var nodeA1, out var nodeB);
                var peerA = GetItemPeer(treeView, nodeA);
                Log.Comment("Name for node with string content: '" + peerA.GetName() + "'");
                Verify.AreEqual("A", peerA.GetName());

                // A node without content is presented through its own string representation,
                // which falls back to the node's runtime class name.
                var nullContentNode = new TreeViewNode();
                treeView.RootNodes.Add(nullContentNode);
                // A node whose content is an empty string yields an empty base name, so the peer
                // must use its "TreeViewNode" fallback rather than reporting an empty name.
                var emptyContentNode = new TreeViewNode() { Content = "" };
                treeView.RootNodes.Add(emptyContentNode);
                Content.UpdateLayout();

                var nullContentPeer = GetItemPeer(treeView, nullContentNode);
                Log.Comment("Name for node without content: '" + nullContentPeer.GetName() + "'");
                Verify.AreEqual("Microsoft.UI.Xaml.Controls.TreeViewNode", nullContentPeer.GetName());

                var emptyContentPeer = GetItemPeer(treeView, emptyContentNode);
                Log.Comment("Name for node with empty content: '" + emptyContentPeer.GetName() + "'");
                Verify.AreEqual("TreeViewNode", emptyContentPeer.GetName());
            });
        }

        // Scenario: Node mode A(A0,A1),B; UIA Expand/Collapse on A's peer, then Expand on leaf B's peer.
        // Expected: Collapsed/LeafNode initially; Expand/Collapse toggle A.IsExpanded and the state, raising
        //   Expanding/Collapsed once each with node A; leaf B stays LeafNode.
        // Failure means: TreeViewItemAutomationPeer::Expand/Collapse/ExpandCollapseState no longer drive the node
        //   or raise TreeView Expanding/Collapsed.
        [TestMethod]
        public void TreeViewItemPeerExpandCollapseProvider()
        {
            RunOnUIThread.Execute(() =>
            {
                var treeView = CreateLaidOutTreeView(out var nodeA, out var nodeA0, out var nodeA1, out var nodeB);
                var expandingNodes = new List<TreeViewNode>();
                var collapsedNodes = new List<TreeViewNode>();
                treeView.Expanding += (sender, args) => expandingNodes.Add(args.Node);
                treeView.Collapsed += (sender, args) => collapsedNodes.Add(args.Node);

                var peerA = GetItemPeer(treeView, nodeA);
                var peerB = GetItemPeer(treeView, nodeB);

                Verify.AreEqual(ExpandCollapseState.Collapsed, peerA.ExpandCollapseState);
                Verify.AreEqual(ExpandCollapseState.LeafNode, peerB.ExpandCollapseState);

                peerA.Expand();
                Verify.IsTrue(nodeA.IsExpanded);
                Verify.AreEqual(ExpandCollapseState.Expanded, peerA.ExpandCollapseState);
                Verify.AreEqual(1, expandingNodes.Count);
                Verify.AreEqual(nodeA, expandingNodes[0]);

                peerA.Collapse();
                Verify.IsFalse(nodeA.IsExpanded);
                Verify.AreEqual(ExpandCollapseState.Collapsed, peerA.ExpandCollapseState);
                Verify.AreEqual(1, collapsedNodes.Count);
                Verify.AreEqual(nodeA, collapsedNodes[0]);

                // Expanding a leaf must not change its state.
                peerB.Expand();
                Verify.AreEqual(ExpandCollapseState.LeafNode, peerB.ExpandCollapseState);
            });
        }

        // Scenario: Peer for a standalone TreeViewItem with no owning TreeView; calls Expand and Collapse.
        // Expected: LeafNode, Level -1, PositionInSet 0, SizeOfSet 0; Expand/Collapse are harmless no-ops.
        // Failure means: TreeViewItemAutomationPeer null handling of GetParentTreeView/GetParentListView regressed
        //   (crash or bogus values for orphaned items).
        [TestMethod]
        public void TreeViewItemPeerWithoutTreeViewReportsDefaults()
        {
            RunOnUIThread.Execute(() =>
            {
                var item = new TreeViewItem();
                var peer = FrameworkElementAutomationPeer.CreatePeerForElement(item) as TreeViewItemAutomationPeer;
                Verify.IsNotNull(peer);

                Verify.AreEqual(ExpandCollapseState.LeafNode, peer.ExpandCollapseState);
                Verify.AreEqual(-1, peer.GetLevel());
                Verify.AreEqual(0, peer.GetPositionInSet());
                Verify.AreEqual(0, peer.GetSizeOfSet());

                // These must be harmless no-ops without an owning TreeView.
                peer.Expand();
                peer.Collapse();
                Verify.AreEqual(ExpandCollapseState.LeafNode, peer.ExpandCollapseState);
            });
        }

        // Scenario: Node mode A(A0,A1),B, Single selection; SelectionItem pattern on B's peer: Select, then
        //   RemoveFromSelection.
        // Expected: Pattern/SelectionContainer non-null; IsSelected toggles; SelectedNode = B, SelectedItem set;
        //   both null after removal.
        // Failure means: TreeViewItemAutomationPeer::Select/RemoveFromSelection/IsSelected or
        //   TreeView::OnListControlSelectionChanged no longer update SelectedNode/SelectedItem.
        [TestMethod]
        public void TreeViewItemPeerSelectionItemPatternSingleMode()
        {
            RunOnUIThread.Execute(() =>
            {
                var treeView = CreateLaidOutTreeView(out var nodeA, out var nodeA0, out var nodeA1, out var nodeB);
                treeView.SelectionMode = TreeViewSelectionMode.Single;
                Content.UpdateLayout();

                var peerB = GetItemPeer(treeView, nodeB);
                var selectionItem = peerB.GetPattern(PatternInterface.SelectionItem) as ISelectionItemProvider;
                Verify.IsNotNull(selectionItem);
                Verify.IsNotNull(selectionItem.SelectionContainer);
                Verify.IsFalse(selectionItem.IsSelected);

                selectionItem.Select();
                Verify.IsTrue(selectionItem.IsSelected);
                Verify.AreEqual(nodeB, treeView.SelectedNode);
                Verify.IsNotNull(treeView.SelectedItem, "SelectedItem should be updated by OnListControlSelectionChanged");
                Verify.IsTrue(treeView.SelectedItem == nodeB || (treeView.SelectedItem as string) == "B",
                    "SelectedItem should reflect the selected node or its content");

                selectionItem.RemoveFromSelection();
                Verify.IsFalse(selectionItem.IsSelected);
                Verify.IsNull(treeView.SelectedNode);
                Verify.IsNull(treeView.SelectedItem, "SelectedItem should be cleared after RemoveFromSelection");
            });
        }

        // Scenario: Node mode A(A0,A1),B, Multiple selection; AddToSelection on A and B, RemoveFromSelection on A.
        // Expected: SelectedNodes contains A and B, both IsSelected; after removal only B remains selected.
        // Failure means: TreeViewItemAutomationPeer::AddToSelection/RemoveFromSelection (UpdateSelection) no
        //   longer edit TreeView.SelectedNodes in multi-select mode.
        [TestMethod]
        public void TreeViewItemPeerSelectionItemPatternMultipleMode()
        {
            RunOnUIThread.Execute(() =>
            {
                var treeView = CreateLaidOutTreeView(out var nodeA, out var nodeA0, out var nodeA1, out var nodeB);
                treeView.SelectionMode = TreeViewSelectionMode.Multiple;
                Content.UpdateLayout();

                var selectionA = GetItemPeer(treeView, nodeA).GetPattern(PatternInterface.SelectionItem) as ISelectionItemProvider;
                var selectionB = GetItemPeer(treeView, nodeB).GetPattern(PatternInterface.SelectionItem) as ISelectionItemProvider;
                Verify.IsNotNull(selectionA);
                Verify.IsNotNull(selectionB);

                selectionA.AddToSelection();
                selectionB.AddToSelection();
                Verify.IsTrue(treeView.SelectedNodes.Contains(nodeA));
                Verify.IsTrue(treeView.SelectedNodes.Contains(nodeB));
                Verify.IsTrue(selectionA.IsSelected);
                Verify.IsTrue(selectionB.IsSelected);

                selectionA.RemoveFromSelection();
                Verify.IsFalse(treeView.SelectedNodes.Contains(nodeA));
                Verify.IsTrue(treeView.SelectedNodes.Contains(nodeB));
                Verify.IsFalse(selectionA.IsSelected);
            });
        }

        // Scenario: Node mode A(A0,A1),B with SelectionMode None; queries the SelectionItem pattern on A's peer.
        // Expected: GetPattern(SelectionItem) returns null.
        // Failure means: TreeViewItemAutomationPeer::GetPatternCore exposes SelectionItem while selection is
        //   disabled.
        [TestMethod]
        public void TreeViewItemPeerHasNoSelectionItemPatternWhenSelectionModeIsNone()
        {
            RunOnUIThread.Execute(() =>
            {
                var treeView = CreateLaidOutTreeView(out var nodeA, out var nodeA0, out var nodeA1, out var nodeB);
                treeView.SelectionMode = TreeViewSelectionMode.None;
                Content.UpdateLayout();

                var peerA = GetItemPeer(treeView, nodeA);
                var pattern = peerA.GetPattern(PatternInterface.SelectionItem);
                Log.Comment("SelectionItem pattern with SelectionMode=None: " + (pattern == null ? "null" : pattern.GetType().Name));
                Verify.IsNull(pattern);
            });
        }

        // Scenario: Node mode A(A0,A1),B; TreeViewListAutomationPeer for the inner ListControl.
        // Expected: ControlType is Tree and a DropTarget pattern is exposed.
        // Failure means: TreeViewListAutomationPeer::GetAutomationControlTypeCore/GetPatternCore regressed.
        [TestMethod]
        public void TreeViewListPeerReportsTreeControlType()
        {
            RunOnUIThread.Execute(() =>
            {
                var treeView = CreateLaidOutTreeView(out var nodeA, out var nodeA0, out var nodeA1, out var nodeB);
                var listControl = VisualTreeUtils.FindVisualChildByName(treeView, "ListControl") as TreeViewList;
                var listPeer = FrameworkElementAutomationPeer.CreatePeerForElement(listControl) as TreeViewListAutomationPeer;
                Verify.IsNotNull(listPeer);
                Verify.AreEqual(AutomationControlType.Tree, listPeer.GetAutomationControlType());
                Verify.IsNotNull(listPeer.GetPattern(PatternInterface.DropTarget));
            });
        }

        // Scenario: Node mode A(A0,A1),B, Multiple selection; Selection pattern on the list peer; B added to
        //   SelectedNodes.
        // Expected: CanSelectMultiple true, IsSelectionRequired false, GetSelection() returns no elements.
        // Failure means: TreeViewListAutomationPeer::CanSelectMultiple/IsMultiselect/IsSelectionRequired/
        //   GetSelection regressed.
        [TestMethod]
        public void TreeViewListPeerSelectionPatternMultipleMode()
        {
            RunOnUIThread.Execute(() =>
            {
                var treeView = CreateLaidOutTreeView(out var nodeA, out var nodeA0, out var nodeA1, out var nodeB);
                treeView.SelectionMode = TreeViewSelectionMode.Multiple;
                Content.UpdateLayout();

                var listControl = VisualTreeUtils.FindVisualChildByName(treeView, "ListControl") as TreeViewList;
                var listPeer = FrameworkElementAutomationPeer.CreatePeerForElement(listControl);
                var selection = listPeer.GetPattern(PatternInterface.Selection) as ISelectionProvider;
                Verify.IsNotNull(selection);
                Verify.IsTrue(selection.CanSelectMultiple);
                Verify.IsFalse(selection.IsSelectionRequired);

                // The selected items may be collapsed or virtualized, so the peer reports none.
                treeView.SelectedNodes.Add(nodeB);
                var selected = selection.GetSelection();
                Verify.AreEqual(0, selected == null ? 0 : selected.Length);
            });
        }

        // Scenario: Node mode A(A0,A1),B, Single selection; Selection pattern on the list peer.
        // Expected: CanSelectMultiple false and IsSelectionRequired false.
        // Failure means: TreeViewListAutomationPeer::CanSelectMultiple/IsMultiselect misreport the selection mode.
        [TestMethod]
        public void TreeViewListPeerCannotSelectMultipleInSingleMode()
        {
            RunOnUIThread.Execute(() =>
            {
                var treeView = CreateLaidOutTreeView(out var nodeA, out var nodeA0, out var nodeA1, out var nodeB);
                treeView.SelectionMode = TreeViewSelectionMode.Single;
                Content.UpdateLayout();

                var listControl = VisualTreeUtils.FindVisualChildByName(treeView, "ListControl") as TreeViewList;
                var listPeer = FrameworkElementAutomationPeer.CreatePeerForElement(listControl);
                var selection = listPeer.GetPattern(PatternInterface.Selection) as ISelectionProvider;
                Verify.IsNotNull(selection);
                Verify.IsFalse(selection.CanSelectMultiple);
                Verify.IsFalse(selection.IsSelectionRequired);
            });
        }

        // Scenario: Node mode A(A0,A1),B; TreeViewItemDataAutomationPeers for A and B on the list peer;
        //   Expand/Collapse through A's data peer.
        // Expected: ExpandCollapse pattern exposed; A Collapsed / B LeafNode; Expand/Collapse update A.IsExpanded
        //   and the reported state.
        // Failure means: TreeViewItemDataAutomationPeer::Expand/Collapse/ExpandCollapseState no longer forward to
        //   the realized item's TreeViewItemAutomationPeer.
        [TestMethod]
        public void TreeViewListPeerCreatesDataPeersThatForwardExpandCollapse()
        {
            RunOnUIThread.Execute(() =>
            {
                var treeView = CreateLaidOutTreeView(out var nodeA, out var nodeA0, out var nodeA1, out var nodeB);
                var listControl = VisualTreeUtils.FindVisualChildByName(treeView, "ListControl") as TreeViewList;
                var listPeer = FrameworkElementAutomationPeer.CreatePeerForElement(listControl) as TreeViewListAutomationPeer;

                var dataPeerA = new TreeViewItemDataAutomationPeer(nodeA, listPeer);
                var dataPeerB = new TreeViewItemDataAutomationPeer(nodeB, listPeer);
                Verify.IsNotNull(dataPeerA.GetPattern(PatternInterface.ExpandCollapse));

                Verify.AreEqual(ExpandCollapseState.Collapsed, dataPeerA.ExpandCollapseState);
                Verify.AreEqual(ExpandCollapseState.LeafNode, dataPeerB.ExpandCollapseState);

                dataPeerA.Expand();
                Verify.IsTrue(nodeA.IsExpanded);
                Verify.AreEqual(ExpandCollapseState.Expanded, dataPeerA.ExpandCollapseState);

                dataPeerA.Collapse();
                Verify.IsFalse(nodeA.IsExpanded);
                Verify.AreEqual(ExpandCollapseState.Collapsed, dataPeerA.ExpandCollapseState);
            });
        }

        // Scenario: Node mode, A collapsed so A0 has no container; data peer for A0; reads ExpandCollapseState,
        //   calls Expand and Collapse.
        // Expected: Each throws UIA_E_ELEMENTNOTENABLED (0x80040200) and A0 stays collapsed.
        // Failure means: TreeViewItemDataAutomationPeer::GetTreeViewItemAutomationPeer no longer throws for
        //   unrealized items (null deref or silent success instead).
        [TestMethod]
        public void TreeViewDataPeerForUnrealizedItemThrowsElementNotEnabled()
        {
            RunOnUIThread.Execute(() =>
            {
                var treeView = CreateLaidOutTreeView(out var nodeA, out var nodeA0, out var nodeA1, out var nodeB);
                var listControl = VisualTreeUtils.FindVisualChildByName(treeView, "ListControl") as TreeViewList;
                var listPeer = FrameworkElementAutomationPeer.CreatePeerForElement(listControl) as TreeViewListAutomationPeer;

                // A0 is not realized because A is collapsed.
                Verify.IsNull(treeView.ContainerFromNode(nodeA0));
                var dataPeer = new TreeViewItemDataAutomationPeer(nodeA0, listPeer);

                const int UIA_E_ELEMENTNOTENABLED = unchecked((int)0x80040200);
                int hr = 0;
                try
                {
                    var state = dataPeer.ExpandCollapseState;
                }
                catch (Exception e)
                {
                    hr = e.HResult;
                }
                Verify.AreEqual(UIA_E_ELEMENTNOTENABLED, hr);

                hr = 0;
                try
                {
                    dataPeer.Expand();
                }
                catch (Exception e)
                {
                    hr = e.HResult;
                }
                Verify.AreEqual(UIA_E_ELEMENTNOTENABLED, hr);
                Verify.IsFalse(nodeA0.IsExpanded);

                hr = 0;
                try
                {
                    dataPeer.Collapse();
                }
                catch (Exception e)
                {
                    hr = e.HResult;
                }
                Verify.AreEqual(UIA_E_ELEMENTNOTENABLED, hr);
                Verify.IsFalse(nodeA0.IsExpanded);
            });
        }

        // Scenario: Node mode A(A0,A1),B; TreeView.Expand(A) then Collapse(A).
        // Expected: Expanding and Collapsed fire exactly once each; args.Node is A and args.Item is "A" (Content).
        // Failure means: TreeView::OnNodeExpanding/OnNodeCollapsed or the event args' Node/Item regressed in
        //   node mode.
        [TestMethod]
        public void TreeViewExpandingAndCollapsedEventArgsExposeNodeAndItemInNodeMode()
        {
            RunOnUIThread.Execute(() =>
            {
                var treeView = CreateLaidOutTreeView(out var nodeA, out var nodeA0, out var nodeA1, out var nodeB);
                TreeViewExpandingEventArgs expandingArgs = null;
                TreeViewCollapsedEventArgs collapsedArgs = null;
                int expandingCount = 0;
                int collapsedCount = 0;
                treeView.Expanding += (sender, args) => { expandingArgs = args; expandingCount++; };
                treeView.Collapsed += (sender, args) => { collapsedArgs = args; collapsedCount++; };

                treeView.Expand(nodeA);
                Verify.AreEqual(1, expandingCount);
                Verify.AreEqual(0, collapsedCount);
                Verify.AreEqual(nodeA, expandingArgs.Node);
                Verify.AreEqual("A", expandingArgs.Item as string);

                treeView.Collapse(nodeA);
                Verify.AreEqual(1, expandingCount);
                Verify.AreEqual(1, collapsedCount);
                Verify.AreEqual(nodeA, collapsedArgs.Node);
                Verify.AreEqual("A", collapsedArgs.Item as string);
            });
        }

        // Scenario: Content mode (ItemsSource = CreateTreeViewItemsSource()); Expand then Collapse RootNodes[0].
        // Expected: Expanding/Collapsed args.Node is the root node and args.Item is the data item items[0].
        // Failure means: TreeViewExpandingEventArgs::Item/TreeViewCollapsedEventArgs::Item no longer return the
        //   data item in content mode.
        [TestMethod]
        public void TreeViewExpandingAndCollapsedEventArgsExposeDataItemInContentMode()
        {
            RunOnUIThread.Execute(() =>
            {
                var items = CreateTreeViewItemsSource();
                var treeView = new TreeView() { Width = 400, Height = 400, ItemsSource = items };
                Content = treeView;
                Content.UpdateLayout();

                TreeViewExpandingEventArgs expandingArgs = null;
                TreeViewCollapsedEventArgs collapsedArgs = null;
                treeView.Expanding += (sender, args) => expandingArgs = args;
                treeView.Collapsed += (sender, args) => collapsedArgs = args;

                var node = treeView.RootNodes[0];
                treeView.Expand(node);
                Verify.IsNotNull(expandingArgs);
                Verify.AreEqual(node, expandingArgs.Node);
                Verify.AreEqual(items[0], expandingArgs.Item);

                treeView.Collapse(node);
                Verify.IsNotNull(collapsedArgs);
                Verify.AreEqual(node, collapsedArgs.Node);
                Verify.AreEqual(items[0], collapsedArgs.Item);
            });
        }

        // Scenario: Node mode A(A0,A1),B; TreeView.Expand(A) then Collapse(A).
        // Expected: Node and container IsExpanded track; ExpandedGlyphVisibility/CollapsedGlyphVisibility in
        //   TreeViewItemTemplateSettings swap Visible/Collapsed.
        // Failure means: TreeView::OnNodeExpanding/OnNodeCollapsed or TreeViewItem::OnPropertyChanged(IsExpanded)
        //   no longer sync container state and glyph visibility.
        [TestMethod]
        public void TreeViewExpandCollapseUpdatesContainerAndGlyphVisibility()
        {
            RunOnUIThread.Execute(() =>
            {
                var treeView = CreateLaidOutTreeView(out var nodeA, out var nodeA0, out var nodeA1, out var nodeB);
                var containerA = treeView.ContainerFromNode(nodeA) as TreeViewItem;
                Verify.IsFalse(containerA.IsExpanded);

                treeView.Expand(nodeA);
                Verify.IsTrue(nodeA.IsExpanded);
                Verify.IsTrue(containerA.IsExpanded);
                Verify.AreEqual(Visibility.Visible, containerA.TreeViewItemTemplateSettings.ExpandedGlyphVisibility);
                Verify.AreEqual(Visibility.Collapsed, containerA.TreeViewItemTemplateSettings.CollapsedGlyphVisibility);

                treeView.Collapse(nodeA);
                Verify.IsFalse(nodeA.IsExpanded);
                Verify.IsFalse(containerA.IsExpanded);
                Verify.AreEqual(Visibility.Collapsed, containerA.TreeViewItemTemplateSettings.ExpandedGlyphVisibility);
                Verify.AreEqual(Visibility.Visible, containerA.TreeViewItemTemplateSettings.CollapsedGlyphVisibility);
            });
        }

        // Scenario: Node mode A(A0,A1),B; cycles TreeView.SelectionMode Single -> Multiple -> None -> Single.
        // Expected: ListControl.SelectionMode is Single, None, None, Single (multi-select is done by TreeView).
        // Failure means: TreeView::UpdateItemsSelectionMode maps TreeViewSelectionMode to the wrong
        //   ListViewSelectionMode.
        [TestMethod]
        public void TreeViewSelectionModeChangeUpdatesListControlSelectionMode()
        {
            RunOnUIThread.Execute(() =>
            {
                var treeView = CreateLaidOutTreeView(out var nodeA, out var nodeA0, out var nodeA1, out var nodeB);
                var listControl = VisualTreeUtils.FindVisualChildByName(treeView, "ListControl") as TreeViewList;

                treeView.SelectionMode = TreeViewSelectionMode.Single;
                Verify.AreEqual(ListViewSelectionMode.Single, listControl.SelectionMode);

                // Multi-select is implemented by TreeView itself, so the list stays unselectable.
                treeView.SelectionMode = TreeViewSelectionMode.Multiple;
                Verify.AreEqual(ListViewSelectionMode.None, listControl.SelectionMode);

                treeView.SelectionMode = TreeViewSelectionMode.None;
                Verify.AreEqual(ListViewSelectionMode.None, listControl.SelectionMode);

                treeView.SelectionMode = TreeViewSelectionMode.Single;
                Verify.AreEqual(ListViewSelectionMode.Single, listControl.SelectionMode);
            });
        }

        // Scenario: Content mode, Single selection; sets SelectedItem = items[1], then items[0].
        // Expected: SelectedItem round-trips; SelectedNode is the matching root node; SelectedItems has 1 entry.
        // Failure means: TreeView::OnPropertyChanged(SelectedItem)/UpdateSelection no longer map the data item to
        //   its node and select it.
        [TestMethod]
        public void TreeViewSelectedItemPropertySelectsMatchingNode()
        {
            RunOnUIThread.Execute(() =>
            {
                var items = CreateTreeViewItemsSource();
                var treeView = new TreeView() { Width = 400, Height = 400, ItemsSource = items };
                treeView.SelectionMode = TreeViewSelectionMode.Single;
                Content = treeView;
                Content.UpdateLayout();

                treeView.SelectedItem = items[1];
                Verify.AreEqual(items[1], treeView.SelectedItem);
                Verify.AreEqual(treeView.RootNodes[1], treeView.SelectedNode);
                Verify.AreEqual(1, treeView.SelectedItems.Count);
                Verify.AreEqual(items[1], treeView.SelectedItems[0]);

                treeView.SelectedItem = items[0];
                Verify.AreEqual(treeView.RootNodes[0], treeView.SelectedNode);
                Verify.AreEqual(1, treeView.SelectedItems.Count);
            });
        }

        // Scenario: ICustomPropertyProvider (and IStringable) on nodes with string, null, int, nested-node, Uri
        //   and SolidColorBrush Content.
        // Expected: Content string/"42"/inner node's string/Uri text; null and brush fall back to the full type
        //   name; Type.Name "TreeViewNode"; GetCustomProperty/GetIndexedProperty return null.
        // Failure means: TreeViewNode::GetStringRepresentation/GetContentAsString/ToString/Type/GetCustomProperty
        //   regressed.
        [TestMethod]
        public void TreeViewNodeStringRepresentationUsesContent()
        {
            RunOnUIThread.Execute(() =>
            {
                var textNode = new TreeViewNode() { Content = "Node" };
                var provider = textNode as Microsoft.UI.Xaml.Data.ICustomPropertyProvider;
                Verify.IsNotNull(provider);
                Verify.AreEqual("Node", provider.GetStringRepresentation());
                Verify.AreEqual("TreeViewNode", provider.Type.Name);

                // Without content the node falls back to its full type name; non-string content is stringified.
                var emptyNode = new TreeViewNode();
                Verify.AreEqual("Microsoft.UI.Xaml.Controls.TreeViewNode",
                    (emptyNode as Microsoft.UI.Xaml.Data.ICustomPropertyProvider).GetStringRepresentation());

                var numberNode = new TreeViewNode() { Content = 42 };
                Verify.AreEqual("42",
                    (numberNode as Microsoft.UI.Xaml.Data.ICustomPropertyProvider).GetStringRepresentation());

                Verify.IsNull(provider.GetCustomProperty("Content"));
                Verify.IsNull(provider.GetIndexedProperty("Item", typeof(int)));

                // Content that is itself an ICustomPropertyProvider (another node) supplies its own representation.
                var outerNode = new TreeViewNode() { Content = new TreeViewNode() { Content = "Inner" } };
                Verify.AreEqual("Inner", (outerNode as Microsoft.UI.Xaml.Data.ICustomPropertyProvider).GetStringRepresentation());
                Verify.AreEqual("Inner", ((global::Windows.Foundation.IStringable)outerNode).ToString());

                // A System.Uri is marshaled as a native Windows.Foundation.Uri, which is IStringable.
                const string uriText = "https://www.example.com/path";
                var uriNode = new TreeViewNode() { Content = new Uri(uriText) };
                Verify.AreEqual(uriText, (uriNode as Microsoft.UI.Xaml.Data.ICustomPropertyProvider).GetStringRepresentation());
                Verify.AreEqual(uriText, ((global::Windows.Foundation.IStringable)uriNode).ToString());

                // Native content that is neither a provider, IStringable nor a string falls back to the node's type name.
                var brushNode = new TreeViewNode() { Content = new SolidColorBrush(Colors.Red) };
                Verify.AreEqual("Microsoft.UI.Xaml.Controls.TreeViewNode",
                    (brushNode as Microsoft.UI.Xaml.Data.ICustomPropertyProvider).GetStringRepresentation());
                Verify.AreEqual("Microsoft.UI.Xaml.Controls.TreeViewNode", ((global::Windows.Foundation.IStringable)brushNode).ToString());
            });
        }

        // Scenario: Detached parent with one child; Children.RemoveAt(1) (past the end).
        // Expected: Throws ArgumentOutOfRangeException; Count stays 1; child's Parent and Depth (0) unchanged.
        // Failure means: TreeViewNodeVector::RemoveAt no longer rejects an out-of-range index before detaching
        //   or mutating children.
        [TestMethod]
        public void TreeViewNodeChildrenRemoveAtOutOfRangeThrowsAndLeavesChildrenIntact()
        {
            RunOnUIThread.Execute(() =>
            {
                var parent = new TreeViewNode();
                var child = new TreeViewNode();
                parent.Children.Add(child);

                Exception thrown = null;
                try
                {
                    parent.Children.RemoveAt(1);
                }
                catch (Exception e)
                {
                    thrown = e;
                }

                Verify.IsNotNull(thrown, "RemoveAt past the end must throw");
                Log.Comment("RemoveAt(1) threw " + thrown.GetType().FullName + " HResult=0x" + thrown.HResult.ToString("X8"));
                Verify.IsTrue(thrown is ArgumentOutOfRangeException, "RemoveAt past the end must report an out-of-range index");
                Verify.AreEqual(1, parent.Children.Count);
                Verify.AreEqual(parent, child.Parent);
                Verify.AreEqual(0, child.Depth);
            });
        }

        // Scenario: Detached parent with two children; Children[0] = replacement.
        // Expected: Count 2, order [replacement, second]; replacement Parent = parent, Depth 0; old child Parent
        //   null, Depth -1.
        // Failure means: TreeViewNodeVector::SetAt no longer detaches the old child and reparents the new one.
        [TestMethod]
        public void TreeViewNodeChildrenSetAtReplacesChildAndReparents()
        {
            RunOnUIThread.Execute(() =>
            {
                var parent = new TreeViewNode();
                var first = new TreeViewNode() { Content = "first" };
                var second = new TreeViewNode() { Content = "second" };
                var replacement = new TreeViewNode() { Content = "replacement" };
                parent.Children.Add(first);
                parent.Children.Add(second);

                parent.Children[0] = replacement;

                Verify.AreEqual(2, parent.Children.Count);
                Verify.AreEqual(replacement, parent.Children[0]);
                Verify.AreEqual(second, parent.Children[1]);
                Verify.AreEqual(parent, replacement.Parent);
                Verify.AreEqual(0, replacement.Depth);
                Verify.IsNull(first.Parent);
                Verify.AreEqual(-1, first.Depth);
            });
        }

        // Scenario: Detached root > expanded parent with two children; parent.Children.Clear().
        // Expected: Count 0; both children Parent null; child1 Depth -1; parent HasChildren and IsExpanded false.
        // Failure means: TreeViewNodeVector::Clear no longer detaches children (put_ParentImpl/UpdateDepth) or
        //   collapses the non-root parent, or UpdateHasChildren is stale.
        [TestMethod]
        public void TreeViewNodeChildrenClearDetachesAllChildren()
        {
            RunOnUIThread.Execute(() =>
            {
                var root = new TreeViewNode();
                var parent = new TreeViewNode();
                var child1 = new TreeViewNode();
                var child2 = new TreeViewNode();
                root.Children.Add(parent);
                parent.Children.Add(child1);
                parent.Children.Add(child2);
                parent.IsExpanded = true;

                parent.Children.Clear();

                Verify.AreEqual(0, parent.Children.Count);
                Verify.IsNull(child1.Parent);
                Verify.IsNull(child2.Parent);
                Verify.AreEqual(-1, child1.Depth);
                Verify.IsFalse(parent.HasChildren);
                Verify.IsFalse(parent.IsExpanded);
            });
        }

        // Scenario: Detached nodes: mid(leaf) is added under root, then removed from root.
        // Expected: leaf Depth 0 -> 1 when mid is attached and back to 0 when detached; mid Depth -1 and Parent
        //   null; leaf.Parent stays mid.
        // Failure means: TreeViewNode::UpdateDepth no longer recurses into descendants when a subtree is
        //   reparented.
        [TestMethod]
        public void TreeViewNodeDepthFollowsReparenting()
        {
            RunOnUIThread.Execute(() =>
            {
                var root = new TreeViewNode();
                var mid = new TreeViewNode();
                var leaf = new TreeViewNode();
                mid.Children.Add(leaf);
                Verify.AreEqual(0, leaf.Depth);

                root.Children.Add(mid);
                Verify.AreEqual(0, mid.Depth);
                Verify.AreEqual(1, leaf.Depth, "Grandchild depth must follow its parent's new depth");

                root.Children.Remove(mid);
                Verify.AreEqual(-1, mid.Depth);
                Verify.AreEqual(0, leaf.Depth, "Grandchild depth must follow its parent's detached depth");
                Verify.IsNull(mid.Parent);
                Verify.AreEqual(mid, leaf.Parent);
            });
        }

        // Scenario: Node mode (int content); checks DP defaults, then sets CanDragItems/CanReorderItems, item
        //   template/style selectors and ItemTemplate/ItemContainerStyle before and after templating.
        // Expected: Defaults true/true/null; getters and GetValue agree and flow to ListControl; selectors pick
        //   T1/S1 for 1, T2/S2 for 2; drag/reorder flags are independent; clearing selectors clears the list's.
        // Failure means: TreeView.properties.cpp getters/setters (e.g. swapped CanDrag/CanReorder) or the
        //   default template bindings to the TreeViewList regressed.
        [TestMethod]
        public void TreeViewPropertiesRoundTripAndFlowToListControl()
        {
            RunOnUIThread.Execute(() =>
            {
                var treeView = new TreeView() { Width = 400, Height = 400 };

                // Defaults (DP metadata) before any template is applied.
                Verify.IsTrue(treeView.CanDragItems);
                Verify.IsTrue(treeView.CanReorderItems);
                Verify.IsNull(treeView.ItemContainerStyle);
                Verify.IsNull(treeView.ItemContainerStyleSelector);
                Verify.IsNull(treeView.ItemTemplate);
                Verify.IsNull(treeView.ItemTemplateSelector);

                var template1 = (DataTemplate)XamlReader.Load(
                    @"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'><TextBlock Text='T1'/></DataTemplate>");
                var template2 = (DataTemplate)XamlReader.Load(
                    @"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'><TextBlock Text='T2'/></DataTemplate>");
                var style1 = (Style)XamlReader.Load(
                    @"<Style xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' TargetType='TreeViewItem'><Setter Property='Tag' Value='S1'/></Style>");
                var style2 = (Style)XamlReader.Load(
                    @"<Style xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' TargetType='TreeViewItem'><Setter Property='Tag' Value='S2'/></Style>");
                // Reuse the TestUI selectors: odd int content -> Template1/Style1, even -> Template2/Style2.
                var templateSelector = new TreeViewItemTemplateSelector() { Template1 = template1, Template2 = template2 };
                var styleSelector = new TreeViewItemStyleSelector() { Style1 = style1, Style2 = style2 };

                treeView.CanDragItems = false;
                treeView.CanReorderItems = false;
                treeView.ItemTemplateSelector = templateSelector;
                treeView.ItemContainerStyleSelector = styleSelector;

                Verify.IsFalse(treeView.CanDragItems);
                Verify.IsFalse((bool)treeView.GetValue(TreeView.CanDragItemsProperty));
                Verify.IsFalse(treeView.CanReorderItems);
                Verify.IsFalse((bool)treeView.GetValue(TreeView.CanReorderItemsProperty));
                Verify.AreSame(templateSelector, treeView.ItemTemplateSelector);
                Verify.AreSame(templateSelector, treeView.GetValue(TreeView.ItemTemplateSelectorProperty));
                Verify.AreSame(styleSelector, treeView.ItemContainerStyleSelector);
                Verify.AreSame(styleSelector, treeView.GetValue(TreeView.ItemContainerStyleSelectorProperty));

                var node1 = new TreeViewNode() { Content = 1 };
                var node2 = new TreeViewNode() { Content = 2 };
                treeView.RootNodes.Add(node1);
                treeView.RootNodes.Add(node2);
                Content = treeView;
                Content.UpdateLayout();

                // The template binds these properties onto the inner list.
                var listControl = VisualTreeUtils.FindVisualChildByName(treeView, "ListControl") as TreeViewList;
                Verify.IsFalse(listControl.CanDragItems);
                Verify.IsFalse(listControl.CanReorderItems);
                Verify.AreSame(templateSelector, listControl.ItemTemplateSelector);
                Verify.AreSame(styleSelector, listControl.ItemContainerStyleSelector);

                // The selectors are applied per item.
                var container1 = treeView.ContainerFromNode(node1) as TreeViewItem;
                var container2 = treeView.ContainerFromNode(node2) as TreeViewItem;
                Verify.AreEqual("S1", container1.Tag as string);
                Verify.AreEqual("S2", container2.Tag as string);
                Verify.AreEqual("T1", (container1.ContentTemplateRoot as TextBlock)?.Text);
                Verify.AreEqual("T2", (container2.ContentTemplateRoot as TextBlock)?.Text);

                // Flipping the drag/reorder flags back flows through the template binding.
                treeView.CanDragItems = true;
                treeView.CanReorderItems = true;
                Verify.IsTrue(listControl.CanDragItems);
                Verify.IsTrue(listControl.CanReorderItems);

                // The two flags are independent: each getter must read its own property.
                treeView.CanDragItems = false;
                treeView.CanReorderItems = true;
                Verify.IsFalse(treeView.CanDragItems);
                Verify.IsFalse((bool)treeView.GetValue(TreeView.CanDragItemsProperty));
                Verify.IsTrue(treeView.CanReorderItems);
                Verify.IsTrue((bool)treeView.GetValue(TreeView.CanReorderItemsProperty));
                Verify.IsFalse(listControl.CanDragItems);
                Verify.IsTrue(listControl.CanReorderItems);

                treeView.CanDragItems = true;
                treeView.CanReorderItems = false;
                Verify.IsTrue(treeView.CanDragItems);
                Verify.IsTrue((bool)treeView.GetValue(TreeView.CanDragItemsProperty));
                Verify.IsFalse(treeView.CanReorderItems);
                Verify.IsFalse((bool)treeView.GetValue(TreeView.CanReorderItemsProperty));
                Verify.IsTrue(listControl.CanDragItems);
                Verify.IsFalse(listControl.CanReorderItems);

                // Explicit ItemTemplate / ItemContainerStyle getters and their DP values agree.
                treeView.ItemTemplate = template2;
                treeView.ItemContainerStyle = style2;
                Verify.AreSame(template2, treeView.ItemTemplate);
                Verify.AreSame(template2, treeView.GetValue(TreeView.ItemTemplateProperty));
                Verify.AreSame(style2, treeView.ItemContainerStyle);
                Verify.AreSame(style2, treeView.GetValue(TreeView.ItemContainerStyleProperty));
                Verify.AreSame(template2, listControl.ItemTemplate);
                Verify.AreSame(style2, listControl.ItemContainerStyle);

                // Clearing the selectors clears them on the list as well.
                treeView.ItemTemplateSelector = null;
                treeView.ItemContainerStyleSelector = null;
                Verify.IsNull(treeView.ItemTemplateSelector);
                Verify.IsNull(treeView.ItemContainerStyleSelector);
                Verify.IsNull(listControl.ItemTemplateSelector);
                Verify.IsNull(listControl.ItemContainerStyleSelector);
            });
        }

        // Scenario: Node mode A(A0,A1),B, Multiple; subscribe Expanding/Collapsed/SelectionChanged, Expand/
        //   Collapse/SelectAll, unsubscribe, add new handlers and repeat.
        // Expected: Original handlers fire once and never after removal; the new handlers fire once each.
        // Failure means: TreeView event remove accessors (event token bookkeeping) no longer detach handlers.
        [TestMethod]
        public void TreeViewEventHandlersStopFiringAfterUnsubscribe()
        {
            RunOnUIThread.Execute(() =>
            {
                var treeView = CreateLaidOutTreeView(out var nodeA, out var nodeA0, out var nodeA1, out var nodeB);
                treeView.SelectionMode = TreeViewSelectionMode.Multiple;
                Content.UpdateLayout();

                // ItemInvoked, DragItemsStarting and DragItemsCompleted are only raised by real input, so their unsubscription
                // can't be observed in-process; the interaction tests cover those events. Only observable events are checked here.
                int expanding = 0, collapsed = 0, selectionChanged = 0;
                global::Windows.Foundation.TypedEventHandler<TreeView, TreeViewExpandingEventArgs> onExpanding = (s, e) => expanding++;
                global::Windows.Foundation.TypedEventHandler<TreeView, TreeViewCollapsedEventArgs> onCollapsed = (s, e) => collapsed++;
                global::Windows.Foundation.TypedEventHandler<TreeView, TreeViewSelectionChangedEventArgs> onSelectionChanged = (s, e) => selectionChanged++;

                treeView.Expanding += onExpanding;
                treeView.Collapsed += onCollapsed;
                treeView.SelectionChanged += onSelectionChanged;

                treeView.Expand(nodeA);
                treeView.Collapse(nodeA);
                treeView.SelectAll();
                Verify.AreEqual(1, expanding);
                Verify.AreEqual(1, collapsed);
                Verify.AreEqual(1, selectionChanged);

                treeView.Expanding -= onExpanding;
                treeView.Collapsed -= onCollapsed;
                treeView.SelectionChanged -= onSelectionChanged;

                // A second, still-subscribed handler proves the operations still raise the events.
                int stillExpanding = 0, stillCollapsed = 0, stillSelectionChanged = 0;
                treeView.Expanding += (s, e) => stillExpanding++;
                treeView.Collapsed += (s, e) => stillCollapsed++;
                treeView.SelectionChanged += (s, e) => stillSelectionChanged++;

                treeView.SelectedNodes.Clear();
                treeView.Expand(nodeA);
                treeView.Collapse(nodeA);
                treeView.SelectAll();

                Verify.AreEqual(1, stillExpanding);
                Verify.AreEqual(1, stillCollapsed);
                Verify.AreEqual(1, stillSelectionChanged);
                Verify.AreEqual(1, expanding, "Removed Expanding handler must not run");
                Verify.AreEqual(1, collapsed, "Removed Collapsed handler must not run");
                Verify.AreEqual(1, selectionChanged, "Removed SelectionChanged handler must not run");
            });
        }

        // Scenario: Standalone TreeViewItem/TemplateSettings defaults; node mode A(A0(A0a),A1),B expanded; sets
        //   glyph props and HasUnrealizedChildren on B's container, then on a detached node.
        // Expected: Default glyphs/size 8/null brush; indentation 0/16/32 by depth; glyph props reach the glyph
        //   template parts (TextBlock or FontIcon); HasUnrealizedChildren drives node HasChildren and GlyphOpacity 0/1.
        // Failure means: TreeViewItem glyph DP defaults/UpdateIndentation/OnPropertyChanged(HasUnrealizedChildren)
        //   or TreeViewNode::HasUnrealizedChildren/UpdateHasChildren regressed.
        [TestMethod]
        public void TreeViewItemGlyphUnrealizedChildrenAndTemplateSettings()
        {
            RunOnUIThread.Execute(() =>
            {
                // Defaults on an unstyled container and on standalone template settings.
                var standalone = new TreeViewItem();
                Verify.AreEqual("\uE76C", standalone.CollapsedGlyph);
                Verify.AreEqual("\uE70D", standalone.ExpandedGlyph);
                Verify.AreEqual(8.0, standalone.GlyphSize);
                Verify.IsNull(standalone.GlyphBrush);
                Verify.IsFalse(standalone.HasUnrealizedChildren);

                var settings = new TreeViewItemTemplateSettings();
                Verify.AreEqual(0, settings.DragItemsCount);
                Verify.AreEqual(0, (int)settings.GetValue(TreeViewItemTemplateSettings.DragItemsCountProperty));
                Verify.AreEqual(new Thickness(0), settings.Indentation);
                Verify.AreEqual(Visibility.Collapsed, settings.ExpandedGlyphVisibility);
                Verify.AreEqual(Visibility.Collapsed, settings.CollapsedGlyphVisibility);

                // A(A0(A0a), A1), B
                var treeView = CreateLaidOutTreeView(out var nodeA, out var nodeA0, out var nodeA1, out var nodeB);
                var nodeA0a = new TreeViewNode() { Content = "A0a" };
                nodeA0.Children.Add(nodeA0a);
                treeView.Expand(nodeA);
                treeView.Expand(nodeA0);
                Content.UpdateLayout();

                var containerA = treeView.ContainerFromNode(nodeA) as TreeViewItem;
                var containerA0 = treeView.ContainerFromNode(nodeA0) as TreeViewItem;
                var containerA0a = treeView.ContainerFromNode(nodeA0a) as TreeViewItem;
                var containerB = treeView.ContainerFromNode(nodeB) as TreeViewItem;

                // Indentation grows by 16px per depth level.
                Verify.AreEqual(0.0, containerA.TreeViewItemTemplateSettings.Indentation.Left);
                Verify.AreEqual(16.0, containerA0.TreeViewItemTemplateSettings.Indentation.Left);
                Verify.AreEqual(32.0, containerA0a.TreeViewItemTemplateSettings.Indentation.Left);
                Verify.AreEqual(0, containerA0a.TreeViewItemTemplateSettings.DragItemsCount);

                // Glyph properties round-trip and flow into the chevron glyph template parts.
                // The glyph parts are looked up as FrameworkElement so the test works whether the template uses
                // TextBlock or FontIcon for them (see ReadGlyphPart).
                var collapsedGlyph = VisualTreeUtils.FindVisualChildByName(containerB, "CollapsedGlyph") as FrameworkElement;
                var expandedGlyph = VisualTreeUtils.FindVisualChildByName(containerB, "ExpandedGlyph") as FrameworkElement;
                Verify.IsNotNull(collapsedGlyph);
                Verify.IsNotNull(expandedGlyph);
                Verify.IsNotNull(containerB.GlyphBrush, "Default style supplies a glyph brush");

                var brush = new SolidColorBrush(Colors.Red);
                containerB.CollapsedGlyph = "+";
                containerB.ExpandedGlyph = "-";
                containerB.GlyphSize = 14.0;
                containerB.GlyphBrush = brush;
                Content.UpdateLayout();

                Verify.AreEqual("+", containerB.CollapsedGlyph);
                Verify.AreEqual("+", (string)containerB.GetValue(TreeViewItem.CollapsedGlyphProperty));
                Verify.AreEqual("-", containerB.ExpandedGlyph);
                Verify.AreEqual("-", (string)containerB.GetValue(TreeViewItem.ExpandedGlyphProperty));
                Verify.AreEqual(14.0, containerB.GlyphSize);
                Verify.AreEqual(14.0, (double)containerB.GetValue(TreeViewItem.GlyphSizeProperty));
                Verify.AreSame(brush, containerB.GlyphBrush);
                Verify.AreSame(brush, containerB.GetValue(TreeViewItem.GlyphBrushProperty));
                var collapsedPart = ReadGlyphPart(collapsedGlyph, "CollapsedGlyph");
                var expandedPart = ReadGlyphPart(expandedGlyph, "ExpandedGlyph");
                Verify.AreEqual("+", collapsedPart.Glyph);
                Verify.AreEqual("-", expandedPart.Glyph);
                Verify.AreEqual(14.0, collapsedPart.FontSize);
                Verify.AreEqual(14.0, expandedPart.FontSize);
                Verify.AreSame(brush, collapsedPart.Foreground);
                Verify.AreSame(brush, expandedPart.Foreground);

                // HasUnrealizedChildren on a realized container is pushed to its node, which then
                // reports children and shows the chevron even though it has no child nodes.
                Verify.IsFalse(nodeB.HasUnrealizedChildren);
                Verify.IsFalse(nodeB.HasChildren);
                Verify.AreEqual(0.0, containerB.GlyphOpacity);

                containerB.HasUnrealizedChildren = true;
                Verify.IsTrue((bool)containerB.GetValue(TreeViewItem.HasUnrealizedChildrenProperty));
                Verify.IsTrue(nodeB.HasUnrealizedChildren);
                Verify.IsTrue(nodeB.HasChildren);
                Verify.AreEqual(0, nodeB.Children.Count);
                Verify.AreEqual(1.0, containerB.GlyphOpacity);

                containerB.HasUnrealizedChildren = false;
                Verify.IsFalse(nodeB.HasUnrealizedChildren);
                Verify.IsFalse(nodeB.HasChildren);
                Verify.AreEqual(0.0, containerB.GlyphOpacity);

                // Setting it directly on a detached node behaves the same way.
                var detached = new TreeViewNode();
                detached.HasUnrealizedChildren = true;
                Verify.IsTrue(detached.HasUnrealizedChildren);
                Verify.IsTrue(detached.HasChildren);
            });
        }

        // Scenario: Node mode TreeView queried before templating (pending SelectedNode), then laid out; then a
        //   content-mode TreeView for container/item lookups.
        // Expected: Pre-template lookups and SelectedItems are null, SelectedNode is pending; after layout it is
        //   applied; container/node/item lookups agree in both modes; SelectedNode set/null updates containers.
        // Failure means: TreeView ContainerFromNode/ContainerFromItem/NodeFromContainer/ItemFromContainer/
        //   SelectedNode null handling or pending-selection application in OnApplyTemplate regressed.
        [TestMethod]
        public void TreeViewContainerAndSelectionApisBeforeAndAfterTemplate()
        {
            RunOnUIThread.Execute(() =>
            {
                var treeView = new TreeView() { Width = 400, Height = 400 };
                var nodeA = new TreeViewNode() { Content = "A" };
                var nodeB = new TreeViewNode() { Content = "B" };
                treeView.RootNodes.Add(nodeA);
                treeView.RootNodes.Add(nodeB);
                var strayContainer = new TreeViewItem();

                // Without a template there is no list control: lookups return null and
                // SelectedItems is unavailable, while SelectedNodes is a pending collection.
                Verify.IsNull(treeView.ContainerFromNode(nodeA));
                Verify.IsNull(treeView.ContainerFromItem(nodeA));
                Verify.IsNull(treeView.NodeFromContainer(strayContainer));
                Verify.IsNull(treeView.ItemFromContainer(strayContainer));
                Verify.IsNull(treeView.SelectedItems);
                Verify.AreEqual(0, treeView.SelectedNodes.Count);
                Verify.IsNull(treeView.SelectedNode);

                treeView.SelectedNode = nodeB;
                Verify.AreEqual(nodeB, treeView.SelectedNode);
                Verify.AreEqual(1, treeView.SelectedNodes.Count);

                treeView.SelectedNode = nodeA;
                Verify.AreEqual(nodeA, treeView.SelectedNode);
                Verify.AreEqual(1, treeView.SelectedNodes.Count, "Setting SelectedNode replaces the pending selection");

                Content = treeView;
                Content.UpdateLayout();

                // The pending selection is applied when the template is applied.
                var containerA = treeView.ContainerFromNode(nodeA) as TreeViewItem;
                var containerB = treeView.ContainerFromNode(nodeB) as TreeViewItem;
                Verify.IsNotNull(containerA);
                Verify.IsNotNull(containerB);
                Verify.AreEqual(nodeA, treeView.SelectedNode);
                Verify.AreEqual(1, treeView.SelectedItems.Count);
                Verify.AreEqual(nodeA, treeView.SelectedItems[0]);
                Verify.IsTrue(containerA.IsSelected);

                // Container <-> node/item lookups agree in node mode.
                Verify.AreEqual(nodeA, treeView.NodeFromContainer(containerA));
                Verify.AreEqual(nodeA, treeView.ItemFromContainer(containerA));
                Verify.AreEqual(containerA, treeView.ContainerFromItem(nodeA));
                Verify.IsNull(treeView.ItemFromContainer(strayContainer));
                Verify.IsNull(treeView.NodeFromContainer(strayContainer));
                Verify.IsNull(treeView.ContainerFromNode(null));

                // The SelectedNode setter replaces the selection and updates the containers.
                treeView.SelectedNode = nodeB;
                Verify.AreEqual(nodeB, treeView.SelectedNode);
                Verify.AreEqual(1, treeView.SelectedNodes.Count);
                Verify.AreEqual(nodeB, treeView.SelectedItems[0]);
                Verify.IsTrue(containerB.IsSelected);
                Verify.IsFalse(containerA.IsSelected);

                treeView.SelectedNode = null;
                Verify.IsNull(treeView.SelectedNode);
                Verify.AreEqual(0, treeView.SelectedNodes.Count);
                Verify.AreEqual(0, treeView.SelectedItems.Count);
                Verify.IsFalse(containerB.IsSelected);

                // Content mode: ItemFromContainer returns the data item and NodeFromContainer its node.
                var items = CreateTreeViewItemsSource();
                var contentTreeView = new TreeView() { Width = 400, Height = 400, ItemsSource = items };
                Content = contentTreeView;
                Content.UpdateLayout();
                var contentContainer = contentTreeView.ContainerFromItem(items[1]) as TreeViewItem;
                Verify.IsNotNull(contentContainer);
                Verify.AreEqual(items[1], contentTreeView.ItemFromContainer(contentContainer));
                Verify.AreEqual(contentTreeView.RootNodes[1], contentTreeView.NodeFromContainer(contentContainer));
                Verify.AreEqual(contentContainer, contentTreeView.ContainerFromNode(contentTreeView.RootNodes[1]));
                Verify.IsNull(contentTreeView.ContainerFromNode(null));
            });
        }

        // Scenario: Node mode A(A0,A1),B in Multiple with B selected; switch to None, then back to Multiple.
        // Expected: EnabledUnselected/EnabledSelected -> Disabled (check boxes Collapsed, B still selected)
        //   -> states restored with check boxes Visible.
        // Failure means: TreeViewList::EnableMultiselect/TreeViewItem::UpdateTreeViewItemVisualState no longer
        //   refresh realized containers on a SelectionMode change.
        [TestMethod]
        public void TreeViewSelectionModeSwitchUpdatesMultiSelectVisualStates()
        {
            RunOnUIThread.Execute(() =>
            {
                var treeView = CreateLaidOutTreeView(out var nodeA, out var nodeA0, out var nodeA1, out var nodeB);
                treeView.SelectionMode = TreeViewSelectionMode.Multiple;
                Content.UpdateLayout();
                treeView.SelectedNodes.Add(nodeB);

                var containerA = treeView.ContainerFromNode(nodeA) as TreeViewItem;
                var containerB = treeView.ContainerFromNode(nodeB) as TreeViewItem;
                Verify.AreEqual("TreeViewMultiSelectEnabledUnselected", GetMultiSelectState(containerA));
                Verify.AreEqual("TreeViewMultiSelectEnabledSelected", GetMultiSelectState(containerB));
                Verify.AreEqual(Visibility.Visible, GetMultiSelectCheckBox(containerA).Visibility);

                // Leaving multi-select disables the check boxes but keeps the node selection.
                treeView.SelectionMode = TreeViewSelectionMode.None;
                Verify.AreEqual("TreeViewMultiSelectDisabled", GetMultiSelectState(containerA));
                Verify.AreEqual("TreeViewMultiSelectDisabled", GetMultiSelectState(containerB));
                Verify.AreEqual(Visibility.Collapsed, GetMultiSelectCheckBox(containerA).Visibility);
                Verify.AreEqual(Visibility.Collapsed, GetMultiSelectCheckBox(containerB).Visibility);
                Verify.IsTrue(treeView.SelectedNodes.Contains(nodeB));

                // Re-entering multi-select restores each realized item's state from the node selection.
                treeView.SelectionMode = TreeViewSelectionMode.Multiple;
                Verify.AreEqual("TreeViewMultiSelectEnabledUnselected", GetMultiSelectState(containerA));
                Verify.AreEqual("TreeViewMultiSelectEnabledSelected", GetMultiSelectState(containerB));
                Verify.AreEqual(Visibility.Visible, GetMultiSelectCheckBox(containerA).Visibility);
                Verify.AreEqual(Visibility.Visible, GetMultiSelectCheckBox(containerB).Visibility);
            });
        }

        // Scenario: Node mode nested pre-expanded tree; remove/add root, insert expanded subtree, remove/replace/
        //   clear expanded children, collapse/expand root, clear last root's children, then re-template.
        // Expected: ListControl flattened content matches after every edit; depths, indentation, parent links and
        //   auto-collapse are correct; a new template rebuilds the same view on a new ListControl.
        // Failure means: ViewModel::TreeViewNodeVectorChanged/IndexOfNextSibling no longer removes the removed
        //   child's expanded descendants or miscomputes flat indexes (or PrepareView fails on re-template).
        [TestMethod]
        public void TreeViewFlattenedViewTracksNestedEditsUnderExpandedParents()
        {
            RunOnUIThread.Execute(() =>
            {
                // R0 (expanded): C0 (expanded): G0, G1 | C1 (expanded): G2 | C2 (expanded): G3
                // R1
                var r0 = new TreeViewNode() { Content = "R0", IsExpanded = true };
                var c0 = new TreeViewNode() { Content = "C0", IsExpanded = true };
                var c1 = new TreeViewNode() { Content = "C1", IsExpanded = true };
                var c2 = new TreeViewNode() { Content = "C2", IsExpanded = true };
                var g0 = new TreeViewNode() { Content = "G0" };
                var g1 = new TreeViewNode() { Content = "G1" };
                var g2 = new TreeViewNode() { Content = "G2" };
                var g3 = new TreeViewNode() { Content = "G3" };
                var r1 = new TreeViewNode() { Content = "R1" };
                c0.Children.Add(g0);
                c0.Children.Add(g1);
                c1.Children.Add(g2);
                c2.Children.Add(g3);
                r0.Children.Add(c0);
                r0.Children.Add(c1);
                r0.Children.Add(c2);

                var treeView = new TreeView() { Width = 400, Height = 800 };
                treeView.RootNodes.Add(r0);
                treeView.RootNodes.Add(r1);
                Content = treeView;
                Content.UpdateLayout();
                var listControl = VisualTreeUtils.FindVisualChildByName(treeView, "ListControl") as TreeViewList;

                // The initial view includes every pre-expanded descendant.
                Verify.AreEqual("R0,C0,G0,G1,C1,G2,C2,G3,R1", GetFlattenedContent(listControl));

                // Remove a root whose preceding sibling has expanded nested descendants: the removed node's flat index
                // must account for every visible descendant of R0 (7), not only R0's direct children.
                treeView.RootNodes.Remove(r1);
                Verify.AreEqual("R0,C0,G0,G1,C1,G2,C2,G3", GetFlattenedContent(listControl));
                treeView.RootNodes.Add(r1);
                Verify.AreEqual("R0,C0,G0,G1,C1,G2,C2,G3,R1", GetFlattenedContent(listControl));

                // Insert a pre-expanded subtree after an expanded sibling.
                var n = new TreeViewNode() { Content = "N", IsExpanded = true };
                var n0 = new TreeViewNode() { Content = "N0" };
                n.Children.Add(n0);
                r0.Children.Insert(1, n);
                Verify.AreEqual("R0,C0,G0,G1,N,N0,C1,G2,C2,G3,R1", GetFlattenedContent(listControl));
                Verify.AreEqual(1, n.Depth);
                Verify.AreEqual(2, n0.Depth);
                Content.UpdateLayout();
                var containerN0 = treeView.ContainerFromNode(n0) as TreeViewItem;
                Verify.IsNotNull(containerN0);
                Verify.AreEqual(32.0, containerN0.TreeViewItemTemplateSettings.Indentation.Left);

                // Remove an expanded child that follows expanded siblings: its descendants leave the view too.
                r0.Children.Remove(c1);
                Verify.AreEqual("R0,C0,G0,G1,N,N0,C2,G3,R1", GetFlattenedContent(listControl));
                Verify.IsNull(c1.Parent);
                Verify.AreEqual(0, g2.Depth);
                Content.UpdateLayout();
                Verify.IsNull(treeView.ContainerFromNode(c1));
                Verify.IsNull(treeView.ContainerFromNode(g2));

                // Replace an expanded child: the replaced subtree is removed and the new node takes its slot.
                var x = new TreeViewNode() { Content = "X" };
                r0.Children[0] = x;
                Verify.AreEqual("R0,X,N,N0,C2,G3,R1", GetFlattenedContent(listControl));
                Verify.IsNull(c0.Parent);
                Verify.AreEqual(r0, x.Parent);

                // Clearing an expanded child that has a next sibling removes only its descendants and collapses it.
                n.Children.Clear();
                Verify.AreEqual("R0,X,N,C2,G3,R1", GetFlattenedContent(listControl));
                Verify.IsFalse(n.IsExpanded);
                Verify.IsFalse(n.HasChildren);

                // Clearing the last child of a non-last root walks up to the next root sibling.
                c2.Children.Clear();
                Verify.AreEqual("R0,X,N,C2,R1", GetFlattenedContent(listControl));
                Verify.IsFalse(c2.IsExpanded);

                // Collapse/expand of the root removes and restores the remaining visible descendants.
                treeView.Collapse(r0);
                Verify.AreEqual("R0,R1", GetFlattenedContent(listControl));
                treeView.Expand(r0);
                Verify.AreEqual("R0,X,N,C2,R1", GetFlattenedContent(listControl));

                // Clearing the children of the expanded LAST root removes everything up to the end of the view.
                r1.Children.Add(new TreeViewNode() { Content = "Z0" });
                r1.Children.Add(new TreeViewNode() { Content = "Z1" });
                treeView.Expand(r1);
                Verify.AreEqual("R0,X,N,C2,R1,Z0,Z1", GetFlattenedContent(listControl));
                r1.Children.Clear();
                Verify.AreEqual("R0,X,N,C2,R1", GetFlattenedContent(listControl));
                Verify.IsFalse(r1.IsExpanded);

                // Re-templating rebuilds the view from the existing root nodes and their expansion state.
                var newTemplate = (ControlTemplate)XamlReader.Load(
                    @"<ControlTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                                       xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
                                       TargetType='TreeView'>
                          <TreeViewList x:Name='ListControl' />
                      </ControlTemplate>");
                treeView.Template = newTemplate;
                Content.UpdateLayout();
                var newListControl = VisualTreeUtils.FindVisualChildByName(treeView, "ListControl") as TreeViewList;
                Verify.IsNotNull(newListControl);
                Verify.AreNotSame(listControl, newListControl);
                Verify.AreEqual("R0,X,N,C2,R1", GetFlattenedContent(newListControl));
                var containerX = treeView.ContainerFromNode(x) as TreeViewItem;
                Verify.IsNotNull(containerX);
                Verify.AreEqual(newListControl, ItemsControl.ItemsControlFromItemContainer(containerX));
            });
        }

        // Scenario: Node mode Multiple, P(K0,K1) collapsed, Q; select unrealized K0, expand P, select P, add/
        //   remove/replace P's children, SelectedNodes[0] = Q, then SelectedNodes.Clear().
        // Expected: P check box indeterminate (not UIA-selected) then checked; selection follows child add/remove/
        //   replace; SetAt/Clear keep SelectedNodes, SelectedItems and check boxes consistent.
        // Failure means: ViewModel::SelectedNodeChildrenChanged/UpdateSelectionStateOfAncestors or
        //   SelectedTreeNodeVector::SetAt/Clear no longer keep multi-select state consistent.
        [TestMethod]
        public void TreeViewMultiSelectStateTracksChildEditsAndSelectedNodesEdits()
        {
            RunOnUIThread.Execute(() =>
            {
                // P (collapsed): K0, K1
                // Q
                var p = new TreeViewNode() { Content = "P" };
                var k0 = new TreeViewNode() { Content = "K0" };
                var k1 = new TreeViewNode() { Content = "K1" };
                var q = new TreeViewNode() { Content = "Q" };
                p.Children.Add(k0);
                p.Children.Add(k1);

                var treeView = new TreeView() { Width = 400, Height = 600, SelectionMode = TreeViewSelectionMode.Multiple };
                treeView.RootNodes.Add(p);
                treeView.RootNodes.Add(q);
                Content = treeView;
                Content.UpdateLayout();
                var containerP = treeView.ContainerFromNode(p) as TreeViewItem;
                var containerQ = treeView.ContainerFromNode(q) as TreeViewItem;
                Verify.IsNull(treeView.ContainerFromNode(k0), "K0 is not realized while P is collapsed");

                // Selecting one of two unrealized children makes the realized parent partially selected.
                treeView.SelectedNodes.Add(k0);
                Verify.AreEqual(1, treeView.SelectedNodes.Count);
                Verify.IsNull(GetMultiSelectCheckBox(containerP).IsChecked, "Parent check box is indeterminate");
                var selectionP = GetItemPeer(treeView, p).GetPattern(PatternInterface.SelectionItem) as ISelectionItemProvider;
                Verify.IsFalse(selectionP.IsSelected, "A partially selected parent is not reported as selected");
                Verify.IsFalse(treeView.SelectedNodes.Contains(p));

                // Realizing the children syncs their check boxes with the node selection.
                treeView.Expand(p);
                Content.UpdateLayout();
                var containerK0 = treeView.ContainerFromNode(k0) as TreeViewItem;
                var containerK1 = treeView.ContainerFromNode(k1) as TreeViewItem;
                Verify.AreEqual(true, GetMultiSelectCheckBox(containerK0).IsChecked);
                Verify.AreEqual(false, GetMultiSelectCheckBox(containerK1).IsChecked);
                Verify.IsNull(GetMultiSelectCheckBox(containerP).IsChecked);

                // Selecting the parent selects every child.
                treeView.SelectedNodes.Add(p);
                VerifySelectedNodeSet(treeView, k0, p, k1);
                Verify.AreEqual(true, GetMultiSelectCheckBox(containerP).IsChecked);
                Verify.IsTrue(selectionP.IsSelected);

                // A child added under a selected parent inherits the selection.
                var k2 = new TreeViewNode() { Content = "K2" };
                p.Children.Add(k2);
                VerifySelectedNodeSet(treeView, k0, p, k1, k2);
                Content.UpdateLayout();
                Verify.AreEqual(true, GetMultiSelectCheckBox(treeView.ContainerFromNode(k2) as TreeViewItem).IsChecked);

                // A child removed from the tree leaves the selection.
                p.Children.Remove(k1);
                VerifySelectedNodeSet(treeView, k0, p, k2);
                Verify.AreEqual(3, treeView.SelectedItems.Count);
                Verify.IsFalse(treeView.SelectedItems.Contains(k1));

                // Replacing a selected child drops the old node and selects the replacement.
                var k3 = new TreeViewNode() { Content = "K3" };
                p.Children[0] = k3;
                VerifySelectedNodeSet(treeView, p, k2, k3);
                Verify.IsFalse(treeView.SelectedItems.Contains(k0));

                // SelectedNodes[i] = node unselects the old node (and its subtree) and selects the new one.
                Verify.AreEqual(p, treeView.SelectedNodes[0]);
                treeView.SelectedNodes[0] = q;
                VerifySelectedNodeSet(treeView, q);
                Verify.AreEqual(q, treeView.SelectedItems[0]);
                Verify.AreEqual(false, GetMultiSelectCheckBox(containerP).IsChecked);
                Verify.AreEqual(true, GetMultiSelectCheckBox(containerQ).IsChecked);

                // Clear() removes from the end until empty and recomputes ancestor state.
                treeView.SelectedNodes.Add(k2);
                VerifySelectedNodeSet(treeView, q, k2);
                Verify.IsNull(GetMultiSelectCheckBox(containerP).IsChecked);
                treeView.SelectedNodes.Clear();
                Verify.AreEqual(0, treeView.SelectedNodes.Count);
                Verify.AreEqual(0, treeView.SelectedItems.Count);
                Verify.AreEqual(false, GetMultiSelectCheckBox(containerP).IsChecked);
                Verify.AreEqual(false, GetMultiSelectCheckBox(containerQ).IsChecked);
            });
        }

        // Scenario: Content mode, Multiple (ItemsSource a,b,c); SelectedItems[0] = c, items[1] = b2, then
        //   RootNodes Insert/RemoveAt/Clear.
        // Expected: SelectedNodes/check boxes follow SelectedItems SetAt; source Replace swaps node and container;
        //   node edits write back to ItemsSource without duplicate nodes.
        // Failure means: SelectedItemsVector::SetAt, TreeViewNode::OnItemsSourceChanged or the
        //   TreeViewNodeVector::GetWritableParentItemsSource write-back regressed.
        [TestMethod]
        public void TreeViewContentModeEditsSyncItemsSourceAndSelectedItems()
        {
            RunOnUIThread.Execute(() =>
            {
                var a = new TreeViewItemSource() { Content = "a" };
                var b = new TreeViewItemSource() { Content = "b" };
                var c = new TreeViewItemSource() { Content = "c" };
                var items = new ObservableCollection<TreeViewItemSource>() { a, b, c };
                var treeView = new TreeView() { Width = 400, Height = 400, SelectionMode = TreeViewSelectionMode.Multiple, ItemsSource = items };
                Content = treeView;
                Content.UpdateLayout();

                // SelectedItems[i] = item replaces the selected item and keeps SelectedNodes in sync.
                treeView.SelectedItems.Add(a);
                Verify.AreEqual(1, treeView.SelectedNodes.Count);
                Verify.AreEqual(treeView.RootNodes[0], treeView.SelectedNodes[0]);
                treeView.SelectedItems[0] = c;
                Verify.AreEqual(1, treeView.SelectedItems.Count);
                Verify.AreEqual(c, treeView.SelectedItems[0]);
                Verify.AreEqual(1, treeView.SelectedNodes.Count);
                Verify.AreEqual(treeView.RootNodes[2], treeView.SelectedNodes[0]);
                Verify.AreEqual(false, GetMultiSelectCheckBox(treeView.ContainerFromItem(a) as TreeViewItem).IsChecked);
                Verify.AreEqual(true, GetMultiSelectCheckBox(treeView.ContainerFromItem(c) as TreeViewItem).IsChecked);

                // Replacing an item in the source replaces the corresponding node.
                var oldNodeB = treeView.RootNodes[1];
                var b2 = new TreeViewItemSource() { Content = "b2" };
                items[1] = b2;
                Verify.AreEqual(3, treeView.RootNodes.Count);
                Verify.AreEqual(b2, treeView.RootNodes[1].Content);
                Verify.AreNotSame(oldNodeB, treeView.RootNodes[1]);
                Verify.IsNull(oldNodeB.Parent);
                Content.UpdateLayout();
                Verify.IsNotNull(treeView.ContainerFromItem(b2));
                Verify.IsNull(treeView.ContainerFromItem(b));

                // Node edits write through to a writable ItemsSource.
                var z = new TreeViewItemSource() { Content = "z" };
                treeView.RootNodes.Insert(0, new TreeViewNode() { Content = z });
                Verify.AreEqual(4, items.Count);
                Verify.AreEqual(z, items[0]);
                Verify.AreEqual(4, treeView.RootNodes.Count, "The write-back must not add a duplicate node");
                Verify.AreEqual(z, treeView.RootNodes[0].Content);

                treeView.RootNodes.RemoveAt(0);
                Verify.AreEqual(3, items.Count);
                Verify.AreEqual(a, items[0]);
                Verify.AreEqual(3, treeView.RootNodes.Count);
                Verify.AreEqual(a, treeView.RootNodes[0].Content);

                treeView.RootNodes.Clear();
                Verify.AreEqual(0, items.Count);
                Verify.AreEqual(0, treeView.RootNodes.Count);
            });
        }

        // Scenario: Node mode: set container IsExpanded true/false; then content mode XAML with TwoWay
        //   IsExpanded binding, with data-side and node-side changes.
        // Expected: Node updates only after a dispatcher pass; bound state realizes children (4, then 5 items);
        //   TreeView.Collapse updates the container and writes back to the data (3 items).
        // Failure means: TreeViewItem::UpdateNodeIsExpandedAsync or TreeView::OnNodeCollapsed container sync
        //   regressed (lost, or synchronous during layout, IsExpanded propagation).
        [TestMethod]
        public void TreeViewContainerIsExpandedDrivesNodeAfterDispatch()
        {
            TreeView treeView = null;
            TreeViewNode nodeA = null, nodeA0 = null, nodeA1 = null, nodeB = null;
            TreeViewItem containerA = null;

            RunOnUIThread.Execute(() =>
            {
                treeView = CreateLaidOutTreeView(out nodeA, out nodeA0, out nodeA1, out nodeB);
                containerA = treeView.ContainerFromNode(nodeA) as TreeViewItem;

                // Setting IsExpanded on the container is deferred to the dispatcher (it cannot change the
                // list's items during layout), so the node is unchanged on return.
                containerA.IsExpanded = true;
                Verify.IsFalse(nodeA.IsExpanded);
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(nodeA.IsExpanded);
                Content.UpdateLayout();
                Verify.IsNotNull(treeView.ContainerFromNode(nodeA0));
                containerA.IsExpanded = false;
                Verify.IsTrue(nodeA.IsExpanded);
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsFalse(nodeA.IsExpanded);
            });

            // Content mode with a TwoWay IsExpanded binding on the item container.
            ObservableCollection<TreeViewItemSource> data = null;
            RunOnUIThread.Execute(() =>
            {
                var item0 = new TreeViewItemSource() { Content = "0", IsExpanded = true };
                item0.Children.Add(new TreeViewItemSource() { Content = "0.0" });
                item0.Children.Add(new TreeViewItemSource() { Content = "0.1" });
                var item1 = new TreeViewItemSource() { Content = "1" };
                item1.Children.Add(new TreeViewItemSource() { Content = "1.0" });
                data = new ObservableCollection<TreeViewItemSource>() { item0, item1 };

                treeView = XamlReader.Load(@"
                    <TreeView xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                              Width='400' Height='400' ItemsSource='{Binding}'>
                        <TreeView.ItemTemplate>
                            <DataTemplate>
                                <TreeViewItem ItemsSource='{Binding Children}'
                                              Content='{Binding Content}'
                                              IsExpanded='{Binding IsExpanded, Mode=TwoWay}'/>
                            </DataTemplate>
                        </TreeView.ItemTemplate>
                    </TreeView>") as TreeView;
                treeView.DataContext = data;
                Content = treeView;
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var listControl = VisualTreeUtils.FindVisualChildByName(treeView, "ListControl") as TreeViewList;
                // The bound container state was pushed to the node, which then realized its children.
                Verify.IsTrue(treeView.RootNodes[0].IsExpanded);
                Verify.IsFalse(treeView.RootNodes[1].IsExpanded);
                Verify.AreEqual(4, listControl.Items.Count);

                // A data-side change flows through the binding to the container and then to the node.
                data[1].IsExpanded = true;
                Verify.IsTrue((treeView.ContainerFromItem(data[1]) as TreeViewItem).IsExpanded);
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var listControl = VisualTreeUtils.FindVisualChildByName(treeView, "ListControl") as TreeViewList;
                Verify.IsTrue(treeView.RootNodes[1].IsExpanded);
                Verify.AreEqual(5, listControl.Items.Count);

                // A node-side collapse updates the container and writes back through the TwoWay binding.
                treeView.Collapse(treeView.RootNodes[0]);
                Verify.IsFalse((treeView.ContainerFromItem(data[0]) as TreeViewItem).IsExpanded);
                Verify.IsFalse(data[0].IsExpanded);
                Verify.AreEqual(3, listControl.Items.Count);
            });
        }

        // Scenario: Node mode A(A0,A1),B; DropTarget pattern from the list peer with no drag in progress.
        // Expected: DropEffect is ""; DropEffects throws E_NOTIMPL (0x80004001), a documented product gap.
        // Failure means: TreeViewListAutomationPeer::DropEffect/DropEffects changed; if DropEffects is now
        //   implemented, update the expectation.
        [TestMethod]
        public void TreeViewListPeerDropTargetOutsideDrag()
        {
            RunOnUIThread.Execute(() =>
            {
                var treeView = CreateLaidOutTreeView(out var nodeA, out var nodeA0, out var nodeA1, out var nodeB);
                var listControl = VisualTreeUtils.FindVisualChildByName(treeView, "ListControl") as TreeViewList;
                var listPeer = FrameworkElementAutomationPeer.CreatePeerForElement(listControl);
                var dropTarget = listPeer.GetPattern(PatternInterface.DropTarget) as IDropTargetProvider;
                Verify.IsNotNull(dropTarget);

                // With nothing being dragged there is no drop effect to describe.
                Verify.AreEqual(string.Empty, dropTarget.DropEffect);

                // Known product gap: DropEffects is not implemented and throws E_NOTIMPL. This documents the
                // current behavior; update the expectation when the product implements it.
                Exception thrown = null;
                try
                {
                    var effects = dropTarget.DropEffects;
                }
                catch (Exception e)
                {
                    thrown = e;
                }
                Verify.IsNotNull(thrown);
                Log.Comment("DropEffects threw " + thrown.GetType().FullName + " HResult=0x" + thrown.HResult.ToString("X8"));
                Verify.AreEqual(unchecked((int)0x80004001), thrown.HResult);
            });
        }

        // Reads the glyph text, font size and foreground from a TreeViewItem chevron glyph template part, which is a
        // TextBlock (Text) today and may be a FontIcon (Glyph); fails clearly for any other part type.
        private static (string Glyph, double FontSize, Brush Foreground) ReadGlyphPart(FrameworkElement part, string partName)
        {
            if (part is TextBlock textBlock)
            {
                return (textBlock.Text, textBlock.FontSize, textBlock.Foreground);
            }
            if (part is FontIcon fontIcon)
            {
                return (fontIcon.Glyph, fontIcon.FontSize, fontIcon.Foreground);
            }
            Verify.Fail(partName + " is a " + (part == null ? "null" : part.GetType().FullName) + ", expected TextBlock or FontIcon");
            throw new InvalidOperationException(partName + " has an unsupported type");
        }

        // Flattened view of the inner list (node mode) as comma-separated node content.
        private static string GetFlattenedContent(TreeViewList listControl)
        {
            return String.Join(",", listControl.Items.Select(item => (item as TreeViewNode)?.Content as string));
        }

        private static string GetMultiSelectState(TreeViewItem container)
        {
            var templateRoot = VisualTreeHelper.GetChild(container, 0) as FrameworkElement;
            var group = VisualStateManager.GetVisualStateGroups(templateRoot).First(g => g.Name == "TreeViewMultiSelectStates");
            return group.CurrentState?.Name;
        }

        private static CheckBox GetMultiSelectCheckBox(TreeViewItem container)
        {
            var checkBox = VisualTreeUtils.FindVisualChildByName(container, "MultiSelectCheckBox") as CheckBox;
            Verify.IsNotNull(checkBox, "Realized TreeViewItem must have a MultiSelectCheckBox");
            return checkBox;
        }

        private static void VerifySelectedNodeSet(TreeView treeView, params TreeViewNode[] expected)
        {
            var actual = String.Join(",", treeView.SelectedNodes.Select(n => n.Content as string));
            Log.Comment("SelectedNodes: " + actual);
            Verify.AreEqual(expected.Length, treeView.SelectedNodes.Count, "SelectedNodes count (actual: " + actual + ")");
            Verify.AreEqual(expected.Length, treeView.SelectedItems.Count, "SelectedItems count");
            foreach (var node in expected)
            {
                Verify.IsTrue(treeView.SelectedNodes.Contains(node), "SelectedNodes should contain " + node.Content);
                Verify.IsTrue(treeView.SelectedItems.Contains(node), "SelectedItems should contain " + node.Content);
            }
        }

        private void VerifySelectedNodesAndSelectedItems(TreeView treeView, string[] expected)
        {
            var expectedStr = String.Join(", ", expected);
            var actualNodesStr = String.Join(", ", treeView.SelectedNodes.Select(node => (node.Content as TreeViewItemSource)?.Content));
            var actualItemsStr = String.Join(", ", treeView.SelectedItems.Select(item => (item as TreeViewItemSource)?.Content));

            Log.Comment($"Verify SelectedItems and SelectedNodes of TreeView. Expected: [{expectedStr}], Actual Nodes: [{actualNodesStr}], ActualItems: [{actualItemsStr}]");

            Log.Comment($"Verify TreeView.SelectedNodes");
            Verify.AreEqual(expected.Length, treeView.SelectedNodes.Count, "Verify TreeView SelectedNodes count");
            for(int i=0; i<expected.Length; i++)
            {
                var nodeContent = treeView.SelectedNodes[i].Content as TreeViewItemSource;
                Verify.IsNotNull(nodeContent);
                Verify.AreEqual(expected[i], nodeContent.Content, $"Verify TreeView SelectedNodes[{i}]");
            }

            Log.Comment($"Verify TreeView.SelectedItems");
            Verify.AreEqual(expected.Length, treeView.SelectedItems.Count, "Verify TreeView SelectedItems count");
            for (int i = 0; i < expected.Length; i++)
            {
                var selectedItem = treeView.SelectedItems[i] as TreeViewItemSource;
                Verify.IsNotNull(selectedItem);
                Verify.AreEqual(expected[i], selectedItem.Content, $"Verify TreeView SelectedItems[{i}]");
            }
        }

        private ObservableCollection<TreeViewItemSource> CreateItems(int itemsPerLevel = 3, int depth = 3)
        {
            ObservableCollection<TreeViewItemSource> items = new ObservableCollection<TreeViewItemSource>();
            foreach (var item in CreateItemsImpl(itemsPerLevel, depth))
            {
                items.Add(item);
            }
            return items;
        }

        private IEnumerable<TreeViewItemSource> CreateItemsImpl(int numItems, int depth, string prefix = "")
        {
            List<TreeViewItemSource> items = new List<TreeViewItemSource>();
            for (int i = 0; i < numItems; i++)
            {
                string name;
                if (string.IsNullOrEmpty(prefix))
                {
                    name = $"{i}";
                }
                else
                {
                    name = $"{prefix}.{i}";
                }
                TreeViewItemSource item = new TreeViewItemSource();
                item.Content = name;
                if (depth > 1)
                {
                    var children = CreateItemsImpl(numItems, depth - 1, name);
                    foreach (var child in children)
                    {
                        item.Children.Add(child);
                    }
                }
                items.Add(item);
            }
            return items;
        }

        private bool IsMultiSelectCheckBoxChecked(TreeView tree, TreeViewNode node)
        {
            var treeViewItem = tree.ContainerFromNode(node) as TreeViewItem;
            var checkBox = VisualTreeUtils.FindVisualChildByName(treeViewItem, "MultiSelectCheckBox") as CheckBox;
            return checkBox.IsChecked == true;
        }

        public ObservableCollection<TreeViewItemSource> CreateTreeViewItemsSource()
        {
            var items = new ObservableCollection<TreeViewItemSource>();
            var item1 = new TreeViewItemSource() { Content = "1" };
            var item1_1 = new TreeViewItemSource() { Content = "1.1" };
            var item2 = new TreeViewItemSource() { Content = "2" };

            item1.Children.Add(item1_1);
            items.Add(item1);
            items.Add(item2);

            return items;
        }
    }
}
