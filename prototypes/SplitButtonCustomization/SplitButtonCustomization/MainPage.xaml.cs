using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.Networking.Sockets;
using Windows.Networking.Vpn;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Navigation;
using static SplitButtonCustomization.FlatBreadCrumbNode;

// The Blank Page item template is documented at https://go.microsoft.com/fwlink/?LinkId=402352&clcid=0x409

namespace SplitButtonCustomization
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class MainPage : Page
    {
        private TreeStructure Tree2 = new TreeStructure();

        public TreeNode treeRoot = new TreeNode() { Name = "Placeholder" };

        TreeStructureConverter treeStructureConverter = new TreeStructureConverterImpl();

        public IReadOnlyList<object> breadCrumbList { get; } = new List<TreeNode>();

        public MainPage()
        {
            this.InitializeComponent();

            Tree2.Root = new TreeNode()
            {
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

            treeRoot = Tree2.Root;

            Tree2.UpdateParents();

            // breadCrumbList = Tree2.GetRootAsList();
        }

        private void flatBreadCrumb_ItemExpanding(FlatListBreadCrumb sender, BreadcrumbItemExpandingEventArgs args)
        {
            args.Children = (sender.CurrentItem as TreeNode).Children;
        }

        private void flatBreadCrumb_ItemClicked(FlatListBreadCrumb sender, BreadcrumbItemClickedEventArgs args)
        {
            TreeNode node = (args.Item as TreeNode);
            sender.LastItemHasChildren = (node.Children.Count != 0);
            sender.ItemsSource = node.GetBreadCrumbPath();
        }
    }
}
