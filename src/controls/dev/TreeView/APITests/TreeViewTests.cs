﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using MUXControlsTestApp.Utilities;
using System;
using System.Linq;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
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
    class SpecialTreeView : TreeView { }
    class SpecialTreeViewItem : TreeViewItem { }
    class SpecialTreeViewList : TreeViewList
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

        [TestMethod]// Disabled with issue number #1775
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

        [TestMethod]
        public void TreeViewItemTemplateTest()
        {
            RunOnUIThread.Execute(() =>
            {
                TreeView treeView = new TreeView();
                treeView.Loaded += (object sender, RoutedEventArgs e) =>
                {
                    var dataTemplate = (DataTemplate)XamlReader.Load(
                   @"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'> 
                          <TextBlock Text='TreeViewItemTemplate'/>
                      </DataTemplate>");
                    treeView.ItemTemplate = dataTemplate;
                    var node = new TreeViewNode();
                    treeView.RootNodes.Add(node);
                    var listControl = VisualTreeUtils.FindVisualChildByName(treeView, "ListControl") as TreeViewList;
                    var treeViewItem = listControl.ContainerFromItem(node) as TreeViewItem;
                    Verify.AreEqual(treeViewItem.ContentTemplate, dataTemplate);
                };
            });
        }

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

        [TestMethod]
        public void TreeViewItemContainerStyleTest()
        {
            RunOnUIThread.Execute(() =>
            {
                TreeView treeView = new TreeView();
                treeView.Loaded += (object sender, RoutedEventArgs e) =>
                {
                    var style = (Style)XamlReader.Load(
                   @"<Style xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'> 
                          <Setter Property='Background' Value='Green'/>
                      </Style>");
                    treeView.ItemContainerStyle = style;
                    var node = new TreeViewNode();
                    treeView.RootNodes.Add(node);
                    var listControl = VisualTreeUtils.FindVisualChildByName(treeView, "ListControl") as TreeViewList;
                    var treeViewItem = listControl.ContainerFromItem(node) as TreeViewItem;
                    Verify.AreEqual(treeViewItem.Style, style);
                };
            });
        }

        [TestMethod]
        public void TreeViewItemContainerTransitionTest()
        {
            RunOnUIThread.Execute(() =>
            {
                TreeView treeView = new TreeView();
                treeView.Loaded += (object sender, RoutedEventArgs e) =>
                {
                    var transition = (TransitionCollection)XamlReader.Load(
                   @"<TransitionCollection xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'> 
                            <ContentThemeTransition />
                      </TransitionCollection>");
                    treeView.ItemContainerTransitions = transition;
                    var node = new TreeViewNode();
                    treeView.RootNodes.Add(node);
                    var listControl = VisualTreeUtils.FindVisualChildByName(treeView, "ListControl") as TreeViewList;
                    var treeViewItem = listControl.ContainerFromItem(node) as TreeViewItem;
                    Verify.AreEqual(treeViewItem.ContentTransitions, transition);
                };
            });
        }

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

        //[TestMethod] 24022985
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

        [TestMethod]
        [TestProperty("Ignore", "True")] // TODO 29556921: Disabled after converting MUXControlsTestApp to a desktop .NET 5 app.  Re-enable when fixed.
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

        [TestMethod]
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
