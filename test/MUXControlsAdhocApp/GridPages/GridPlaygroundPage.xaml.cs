using System;
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

            _columnStartIndex.ValueChanged += (object sender, RangeBaseValueChangedEventArgs e) => { IndexValueChanged(sender, GridLocationType.ColumnStart); };
            _columnEndIndex.ValueChanged += (object sender, RangeBaseValueChangedEventArgs e) => { IndexValueChanged(sender, GridLocationType.ColumnEnd); };
            _rowStartIndex.ValueChanged += (object sender, RangeBaseValueChangedEventArgs e) => { IndexValueChanged(sender, GridLocationType.RowStart); };
            _rowEndIndex.ValueChanged += (object sender, RangeBaseValueChangedEventArgs e) => { IndexValueChanged(sender, GridLocationType.RowEnd); };
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
            int index = (int)((Slider)sender).Value;
            GridLocation location = GetGridLocation(type);
            location.Index = index;
            _grid.InvalidateMeasure();
        }

        private void InvalidateButton_Click(object sender, RoutedEventArgs e)
        {
            _grid.InvalidateMeasure();
        }
    }
}
