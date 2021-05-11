// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.Generic;
using System.Linq;
using Microsoft.UI.Xaml.Controls;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;

namespace MUXControlsTestApp.Samples
{
    public sealed partial class UniformGridLayoutDemo : TestPage
    {
        public IEnumerable<int> collection;

        public UniformGridLayoutDemo()
        {
            collection = Enumerable.Range(0, 40);
            this.InitializeComponent();
        }

        public void GetRepeaterActualHeightButtonClick(object sender, RoutedEventArgs e)
        {
            RepeaterActualHeightLabel.Text = UniformGridRepeater.ActualHeight.ToString();
        }

        private void GetUniformGridLayoutMinColumnSpacingButtonClick(object sender, RoutedEventArgs e)
        {
            UniformGridLayoutMinColumnSpacing.Text = (UniformGridRepeater.Layout as UniformGridLayout).MinColumnSpacing.ToString();
        }

        private void SetUniformGridLayoutMinColumnSpacingButtonClick(object sender, RoutedEventArgs e)
        {
            LayoutHelper.SetMinColumnSpacing(UniformGridRepeater.Layout, double.Parse(UniformGridLayoutMinColumnSpacing.Text));
        }

        private void GetUniformGridLayoutMinRowSpacingButtonClick(object sender, RoutedEventArgs e)
        {
            UniformGridLayoutMinRowSpacing.Text = (UniformGridRepeater.Layout as UniformGridLayout).MinRowSpacing.ToString();
        }

        private void SetUniformGridLayoutMinRowSpacingButtonClick(object sender, RoutedEventArgs e)
        {
            LayoutHelper.SetMinRowSpacing(UniformGridRepeater.Layout, double.Parse(UniformGridLayoutMinRowSpacing.Text));
        }

        private void GetUniformGridLayoutMinItemWidthButtonClick(object sender, RoutedEventArgs e)
        {
            UniformGridLayoutMinItemWidth.Text = (UniformGridRepeater.Layout as UniformGridLayout).MinItemWidth.ToString();
        }

        private void SetUniformGridLayoutMinItemWidthButtonClick(object sender, RoutedEventArgs e)
        {
            (UniformGridRepeater.Layout as UniformGridLayout).MinItemWidth = double.Parse(UniformGridLayoutMinItemWidth.Text);
        }

        private void GetUniformGridLayoutMinItemHeightButtonClick(object sender, RoutedEventArgs e)
        {
            UniformGridLayoutMinItemHeight.Text = (UniformGridRepeater.Layout as UniformGridLayout).MinItemHeight.ToString();
        }

        private void SetUniformGridLayoutMinItemHeightButtonClick(object sender, RoutedEventArgs e)
        {
            (UniformGridRepeater.Layout as UniformGridLayout).MinItemHeight = double.Parse(UniformGridLayoutMinItemHeight.Text);
        }

        private void GetScrollViewerMaxWidthButtonClick(object sender, RoutedEventArgs e)
        {
            ScrollViewerMaxWidth.Text = ScrollViewer.MaxWidth.ToString();
        }
        
        private void SetScrollViewerMaxWidthButtonClick(object sender, RoutedEventArgs e)
        {
            ScrollViewer.MaxWidth = double.Parse(ScrollViewerMaxWidth.Text);
        }

        private void GetScrollViewerMaxHeightButtonClick(object sender, RoutedEventArgs e)
        {
            ScrollViewerMaxHeight.Text = ScrollViewer.MaxHeight.ToString();
        }

        private void SetScrollViewerMaxHeightButtonClick(object sender, RoutedEventArgs e)
        {
            ScrollViewer.MaxHeight = double.Parse(ScrollViewerMaxHeight.Text);
        }

        private void GetScrollViewerHorizontalOffsetButtonClick(object sender, RoutedEventArgs e)
        {
            ScrollViewerHorizontalOffset.Text = ScrollViewer.HorizontalOffset.ToString();
        }

        private void SetScrollViewerHorizontalOffsetButtonClick(object sender, RoutedEventArgs e)
        {
            ScrollViewer.ChangeView(double.Parse(ScrollViewerHorizontalOffset.Text), null, null);
        }

        private void GetScrollViewerVerticalOffsetButtonClick(object sender, RoutedEventArgs e)
        {
            ScrollViewerVerticalOffset.Text = ScrollViewer.VerticalOffset.ToString();
        }

        private void SetScrollViewerVerticalOffsetButtonClick(object sender, RoutedEventArgs e)
        {
            ScrollViewer.ChangeView(null, double.Parse(ScrollViewerVerticalOffset.Text), null);
        }

        private void CmbUniformGridLayoutItemsStretch_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (UniformGridRepeater != null)
            {
                var itemsStretch = (CmbUniformGridLayoutItemsStretch.SelectedItem as ComboBoxItem).Content.ToString();

                LayoutHelper.SetItemsStretch(UniformGridRepeater.Layout, itemsStretch);
            }
        }

        private void CmbUniformGridLayoutItemsJustification_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (UniformGridRepeater != null)
            {
                var itemsJustification = (CmbUniformGridLayoutItemsJustification.SelectedItem as ComboBoxItem).Content.ToString();

                LayoutHelper.SetLineAlignment(UniformGridRepeater.Layout, itemsJustification);
            }
        }

        private void CmbUniformGridLayoutOrientation_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (UniformGridRepeater != null)
            {
                (UniformGridRepeater.Layout as UniformGridLayout).Orientation = CmbUniformGridLayoutOrientation.SelectedIndex == 0 ? Orientation.Vertical : Orientation.Horizontal;
            }
        }

        private void CmbScrollViewerHorizontalScrollBarVisibility_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (ScrollViewer != null)
            {
                ScrollViewer.HorizontalScrollBarVisibility = (ScrollBarVisibility)CmbScrollViewerHorizontalScrollBarVisibility.SelectedIndex;
            }
        }
    }
}
