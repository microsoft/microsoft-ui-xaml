// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Markup;
using Windows.UI;
using System.Windows.Input;

using TwoPaneView = Microsoft.UI.Xaml.Controls.TwoPaneView;
using Breadcrumb = Microsoft.UI.Xaml.Controls.Breadcrumb;
using Breadcrumb_TestUI;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using Windows.Security.Authentication.OnlineId;
using Microsoft.UI.Xaml.Controls;
using System.Linq;

namespace MUXControlsTestApp
{
    [TopLevelTestPage(Name = "Breadcrumb")]
    public sealed partial class BreadcrumbPage : TestPage
    {
        private TreeStructure Tree = new TreeStructure();
        public ObservableCollection<object> breadCrumbList { get; } = new ObservableCollection<object>();

        public ObservableCollection<object> currentNodeChildrenList { get; } = new ObservableCollection<object>();

        private const int maxDepth = 7;

        public BreadcrumbPage()
        {
            this.InitializeComponent();
            InitializeBreadcrumbAndChildren();
        }

        private void InitializeBreadcrumbAndChildren()
        {
            // GenerateTree();
            GenerateTreeDynamic();

            breadCrumbList.Add(Tree.Root);
            UpdateChildrenList(Tree.Root);
        }

        private void GenerateTree()
        {
            Tree.Root = new TreeNode() {
                Name = "Root",
                Children = new List<object>()
                {
                    new TreeNode()
                    {
                        Name = "A1",
                        Children = new List<object>()
                        {
                            new TreeNode()
                            {
                                Name = "Nested A1"
                            },
                            new TreeNode()
                            {
                                Name = "Nested A2"
                            },
                            new TreeNode()
                            {
                                Name = "Nested A3"
                            },
                        }
                    },
                    new TreeNode()
                    {
                        Name = "B1",
                        Children = new List<object>()
                        {
                            new TreeNode()
                            {
                                Name = "Nested B1"
                            },
                            new TreeNode()
                            {
                                Name = "Nested B2"
                            },
                            new TreeNode()
                            {
                                Name = "Nested B3"
                            },
                        }
                    },
                    new TreeNode()
                    {
                        Name = "C1",
                        Children = new List<object>()
                        {
                            new TreeNode()
                            {
                                Name = "Nested C1"
                            },
                            new TreeNode()
                            {
                                Name = "Nested C2"
                            },
                            new TreeNode()
                            {
                                Name = "Nested C3"
                            },
                        }
                    }
                }
            };

            Tree.UpdateParents();
        }

        private void GenerateTreeDynamic()
        {
            Tree.Root = new TreeNode() { Name = "Root" };
            CreateChildrenForNode(Tree.Root, 1);

            Tree.UpdateParents();
        }

        private void CreateChildrenForNode(TreeNode node, int depth)
        {
            if (depth > maxDepth)
            {
                return;
            }

            node.Children = new List<object>();

            for (int i = 0; i < 3; ++i)
            {
                string nodeName = "";
                if (depth == 1)
                {
                    // This will yield stuff as 'Node A' or 'Node B'
                    nodeName = "Node " + (char)('A' + i);
                }
                else
                {
                    nodeName = node.Name + "_" + (i + 1);
                }

                TreeNode child = new TreeNode() { Name = nodeName };
                node.Children.Add(child);
                CreateChildrenForNode(child, depth + 1);
            }
        }

        private void ItemsRepeater_ButtonClick(object sender, RoutedEventArgs e)
        {
            Button btn = sender as Button;
            TreeNode treeNode = btn.Content as TreeNode;
            ReplaceList(breadCrumbList, treeNode.GetBreadCrumbPath());
            UpdateChildrenList(treeNode);
        }

        private void WidthSlider_ValueChanged(object sender, RangeBaseValueChangedEventArgs e)
        {
            if (TwoPaneView == null)
            {
                return;
            }

            TwoPaneView.Pane1Length = new GridLength(e.NewValue, GridUnitType.Pixel);
            TwoPaneView.Pane2Length = new GridLength(1, GridUnitType.Star);
        }

        private void RTL_Checked(object sender, RoutedEventArgs e)
        {
            BreadCrumbControl.FlowDirection = FlowDirection.RightToLeft;
        }

        private void RTL_Unchecked(object sender, RoutedEventArgs e)
        {
            BreadCrumbControl.FlowDirection = FlowDirection.LeftToRight;
        }

        private void ReplaceList(ObservableCollection<object> oldItemsList, List<object> newItemsList)
        {
            oldItemsList.Clear();

            foreach (object child in newItemsList)
            {
                oldItemsList.Add(child);
            }
        }
        
        private void UpdateChildrenList(TreeNode node)
        {
            ReplaceList(currentNodeChildrenList, node.Children);
        }

        private void BreadcrumbControl_ItemClicked(Breadcrumb sender, Microsoft.UI.Xaml.Controls.BreadcrumbItemClickedEventArgs args)
        {
            TreeNode treeNode = (TreeNode)args.Item;
            ReplaceList(breadCrumbList, treeNode.GetBreadCrumbPath());
            UpdateChildrenList(treeNode);
        }

        private void Child_ElementPrepared(object sender, ItemsRepeaterElementPreparedEventArgs e)
        {
            Button button = e.Element as Button;
            TreeNode node = (currentNodeChildrenList.ToArray<object>().GetValue(e.Index) as TreeNode);

            button.Name = node.Name;
        }
    }
}
