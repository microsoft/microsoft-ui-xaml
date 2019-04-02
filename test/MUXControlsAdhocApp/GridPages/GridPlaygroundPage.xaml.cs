using System;
using System.Collections.Generic;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;

namespace MUXControlsAdhocApp.GridPages
{
    public sealed partial class GridPlaygroundPage : Page
    {
        public GridPlaygroundPage()
        {
            this.InitializeComponent();

            // Traverse a couple levels down the hierarchy looking for specifically Tagged elements.
            // TODO: There are likely better utilities for doing a limited traversal.
            List<UIElement> childrenAndGrandchildren = new List<UIElement>();
            foreach (var child in _itemControls.Children)
            {
                childrenAndGrandchildren.Add(child);

                Panel panel = child as Panel;
                if (panel == null)
                {
                    continue;
                }

                foreach (var grandchild in panel.Children)
                {
                    childrenAndGrandchildren.Add(grandchild);
                }
            }

            foreach (var descendent in childrenAndGrandchildren)
            {
                Panel panel = descendent as Panel;
                if (panel == null)
                {
                    continue;
                }

                GridLocationType type;
                if (Enum.TryParse<GridLocationType>(panel.Tag as string, out type))
                {
                    HookUpCellControls(panel, type);
                }
            }
        }

        private void HookUpCellControls(Panel parent, GridLocationType type)
        {
            foreach (var child in parent.Children)
            {
                FrameworkElement element = (FrameworkElement)child;
                switch (element.Tag)
                {
                    case "Index":
                        ((Slider)element).ValueChanged += (object sender, RangeBaseValueChangedEventArgs e) => { IndexValueChanged(sender, type); };
                        break;

                    case "LineName":
                        ((TextBox)element).TextChanged += (object sender, TextChangedEventArgs e) => { LineNameChanged(sender, type); };
                        break;

                    case "Span":
                        ((Slider)element).ValueChanged += (object sender, RangeBaseValueChangedEventArgs e) => { SpanValueChanged(sender, type); };
                        break;
                }
            }
        }

        private enum GridLocationType
        {
            ColumnStart,
            ColumnEnd,
            RowStart,
            RowEnd
        }

        private GridLocation GetGridLocation(GridLocationType type)
        {
            switch (type)
            {
                case GridLocationType.ColumnStart: return Grid.GetColumnStart(_item);
                case GridLocationType.ColumnEnd: return Grid.GetColumnEnd(_item);
                case GridLocationType.RowStart: return Grid.GetRowStart(_item);
                case GridLocationType.RowEnd: return Grid.GetRowEnd(_item);
            }

            throw new System.ArgumentException();
        }

        private void IndexValueChanged(object sender, GridLocationType type)
        {
            int value = (int)((Slider)sender).Value;
            GridLocation location = GetGridLocation(type);
            location.Index = value;
            _grid.InvalidateMeasure();
        }

        private void LineNameChanged(object sender, GridLocationType type)
        {
            string value = ((TextBox)sender).Text;
            GridLocation location = GetGridLocation(type);
            location.LineName = value;
            _grid.InvalidateMeasure();
        }

        private void SpanValueChanged(object sender, GridLocationType type)
        {
            int value = (int)((Slider)sender).Value;
            GridLocation location = GetGridLocation(type);
            location.Span = value;
            _grid.InvalidateMeasure();
        }

        private void InvalidateButton_Click(object sender, RoutedEventArgs e)
        {
            _grid.InvalidateMeasure();
        }

        private void JustifyItemsComboBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            _grid.JustifyItems = EnumValueFromComboBox<GridJustifyItems>(sender);
        }

        private T EnumValueFromComboBox<T>(object sender) where T : struct, IComparable
        {
            string selection = (string)((ComboBox)sender).SelectedItem;
            return Enum.Parse<T>(selection);
        }
    }
}
