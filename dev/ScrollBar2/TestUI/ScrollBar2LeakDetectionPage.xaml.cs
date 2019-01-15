// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;

#if !BUILD_WINDOWS
using IScrollController = Microsoft.UI.Xaml.Controls.Primitives.IScrollController;
using ScrollBar2 = Microsoft.UI.Xaml.Controls.ScrollBar2;
#endif

namespace MUXControlsTestApp
{
    public sealed partial class ScrollBar2LeakDetectionPage : TestPage
    {
        private ScrollBar2 scrollBar2 = null;
        
        public ScrollBar2LeakDetectionPage()
        {
            this.InitializeComponent();
        }

        private void BtnCreateScrollBar2_Click(object sender, Windows.UI.Xaml.RoutedEventArgs e)
        {
            scrollBar2 = new ScrollBar2();
            scrollBar2.Name = "dynamicScrollBar2";
            scrollBar2.Width = 300.0;
            scrollBar2.Height = 16.0;
            scrollBar2.Margin = new Thickness(6);
            scrollBar2.VerticalAlignment = VerticalAlignment.Top;
            scrollBar2.Orientation = Orientation.Horizontal;

            IScrollController sc = scrollBar2 as IScrollController;
            if (sc != null)
            {
                sc.SetValues(1, 2, 3, 4);
            }

            btnCreateScrollBar2.IsEnabled = false;
            btnAddScrollBar2.IsEnabled = true;
            btnDeleteScrollBar2.IsEnabled = true;
        }

        private void BtnAddScrollBar2_Click(object sender, Windows.UI.Xaml.RoutedEventArgs e)
        {
            Grid.SetRow(scrollBar2, 5);
            grid.Children.Add(scrollBar2);

            btnAddScrollBar2.IsEnabled = false;
            btnDeleteScrollBar2.IsEnabled = false;
            btnRemoveScrollBar2.IsEnabled = true;
        }

        private void BtnRemoveScrollBar2_Click(object sender, Windows.UI.Xaml.RoutedEventArgs e)
        {
            grid.Children.Remove(scrollBar2);

            btnRemoveScrollBar2.IsEnabled = false;
            btnAddScrollBar2.IsEnabled = true;
            btnDeleteScrollBar2.IsEnabled = true;
        }

        private void BtnDeleteScrollBar2_Click(object sender, Windows.UI.Xaml.RoutedEventArgs e)
        {
            if (scrollBar2 == null || grid.Children.Count == 6)
                return;

            scrollBar2 = null;

            btnDeleteScrollBar2.IsEnabled = false;
            btnAddScrollBar2.IsEnabled = false;
            btnCreateScrollBar2.IsEnabled = true;
        }

        private void BtnGarbageCollect_Click(object sender, Windows.UI.Xaml.RoutedEventArgs e)
        {
            System.GC.Collect();
            System.GC.WaitForPendingFinalizers();
        }
    }
}