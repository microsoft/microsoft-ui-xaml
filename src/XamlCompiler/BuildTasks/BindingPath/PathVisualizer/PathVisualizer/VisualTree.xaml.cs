// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;

namespace PathVisualizer
{
    public partial class VisualTree
    {
        public VisualTree()
        {
            InitializeComponent();
        }

        public PathTree Tree
        {
            private get { return _tree; }
            set
            {
                if (_tree != value)
                {
                    _tree = value;
                    UpdateVisual();
                }
            }
        }
        private PathTree _tree;

        private void UpdateVisual()
        {
            VisualRoot.Children.Clear();

            Extra.Text = Tree.Remainder;
            Extra.Visibility = string.IsNullOrEmpty(Tree.Remainder) ?
                Visibility.Collapsed : Visibility.Visible;

            VisualRoot.Children.Add(ProcessTreeNode(Tree.Nodes));
        }

        private Grid ProcessTreeNode(PathTreeNode node)
        {
            var root = new Grid();
            root.RowDefinitions.Add(new RowDefinition() { Height = new GridLength(0, GridUnitType.Auto) });
            root.RowDefinitions.Add(new RowDefinition() { Height = new GridLength(0, GridUnitType.Auto) });

            var data = new TextBlock();
            data.Text = string.IsNullOrEmpty(node.Name) ? node.Data : node.Name;
            data.TextAlignment = TextAlignment.Center;
            data.Margin = new Thickness(5.0);
            root.Children.Add(data);

            var border = new Border();
            border.BorderThickness = new Thickness(2.0);
            border.BorderBrush = new SolidColorBrush(Colors.Black);
            border.SetValue(Grid.RowSpanProperty, 3);
            root.Children.Add(border);

            if (node.Children.Count > 0)
            {
                root.RowDefinitions.Add(new RowDefinition() { Height = new GridLength(0, GridUnitType.Auto) });
                root.RowDefinitions.Add(new RowDefinition() { Height = new GridLength(0, GridUnitType.Auto) });

                var context = new TextBlock();
                context.SetValue(Grid.RowProperty, 1);
                context.Text = node.Data;
                context.Foreground = new SolidColorBrush(Colors.LightGray);
                context.TextAlignment = TextAlignment.Center;
                root.Children.Add(context);

                var childrenGrid = new Grid();
                childrenGrid.SetValue(Grid.RowProperty, 2);
                childrenGrid.HorizontalAlignment = HorizontalAlignment.Center;
                root.Children.Add(childrenGrid);

                for (int iChild = 0; iChild < node.Children.Count; iChild++)
                {
                    childrenGrid.ColumnDefinitions.Add(new ColumnDefinition() { Width = new GridLength(0, GridUnitType.Auto) });

                    var childGrid = ProcessTreeNode(node.Children[iChild]);
                    childGrid.SetValue(Grid.ColumnProperty, iChild);
                    childrenGrid.Children.Add(childGrid);
                }
            }

            return root;
        }
    }
}