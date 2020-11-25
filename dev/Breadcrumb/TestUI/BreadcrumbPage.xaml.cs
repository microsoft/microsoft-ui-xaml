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

using Breadcrumb = Microsoft.UI.Xaml.Controls.Breadcrumb;
using Breadcrumb_TestUI;
using System.Collections.Generic;
using System.Collections.ObjectModel;

namespace MUXControlsTestApp
{
    [TopLevelTestPage(Name = "Breadcrumb")]
    public sealed partial class BreadcrumbPage : TestPage
    {
        private TreeStructure Tree = new TreeStructure();
        public ObservableCollection<object> breadCrumbList { get; } = new ObservableCollection<object>();

        public ObservableCollection<object> currentNodeChildrenList { get; } = new ObservableCollection<object>();

        public BreadcrumbPage()
        {
            this.InitializeComponent();
            InitializeBreadcrumbAndChildren();
        }

        private void InitializeBreadcrumbAndChildren()
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

            breadCrumbList.Add(Tree.Root);
            UpdateChildrenList(Tree.Root);
        }

        private void ItemRepeater_ButtonClick(object sender, RoutedEventArgs e)
        {
            Button btn = sender as Button;
            TreeNode treeNode = btn.Content as TreeNode;

            ReplaceList(breadCrumbList, treeNode.GetBreadCrumbPath());
            UpdateChildrenList(treeNode);
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
        
    }
}
