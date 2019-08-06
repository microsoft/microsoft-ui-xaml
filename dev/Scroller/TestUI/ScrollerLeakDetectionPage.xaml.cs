// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Windows.UI;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Shapes;

using Scroller = Microsoft.UI.Xaml.Controls.Primitives.Scroller;

namespace MUXControlsTestApp
{
    public sealed partial class ScrollerLeakDetectionPage : TestPage
    {
        private Scroller scroller = null;
        
        public ScrollerLeakDetectionPage()
        {
            this.InitializeComponent();
        }

        private void BtnCreateScroller_Click(object sender, Windows.UI.Xaml.RoutedEventArgs e)
        {
            scroller = new Scroller();
            scroller.Name = "dynamicScroller";
            scroller.Width = 300.0;
            scroller.Height = 400.0;
            scroller.Margin = new Thickness(1);
            scroller.Background = new SolidColorBrush(Colors.HotPink);
            scroller.VerticalAlignment = VerticalAlignment.Top;

            Rectangle rect = new Rectangle();
            rect.Width = 900.0;
            rect.Height = 1200.0;
            rect.Fill = new SolidColorBrush(Colors.DarkRed);
            scroller.Content = rect;

            btnCreateScroller.IsEnabled = false;
            btnAddScroller.IsEnabled = true;
            btnDeleteScroller.IsEnabled = true;
        }

        private void BtnAddScroller_Click(object sender, Windows.UI.Xaml.RoutedEventArgs e)
        {
            Grid.SetRow(scroller, 5);
            grid.Children.Add(scroller);

            btnAddScroller.IsEnabled = false;
            btnDeleteScroller.IsEnabled = false;
            btnRemoveScroller.IsEnabled = true;
        }

        private void BtnRemoveScroller_Click(object sender, Windows.UI.Xaml.RoutedEventArgs e)
        {
            grid.Children.Remove(scroller);

            btnRemoveScroller.IsEnabled = false;
            btnAddScroller.IsEnabled = true;
            btnDeleteScroller.IsEnabled = true;
        }

        private void BtnDeleteScroller_Click(object sender, Windows.UI.Xaml.RoutedEventArgs e)
        {
            if (scroller == null || grid.Children.Count == 6)
                return;

            scroller.Content = null;
            scroller = null;

            btnDeleteScroller.IsEnabled = false;
            btnAddScroller.IsEnabled = false;
            btnCreateScroller.IsEnabled = true;
        }

        private void BtnGarbageCollect_Click(object sender, Windows.UI.Xaml.RoutedEventArgs e)
        {
            System.GC.Collect();
            System.GC.WaitForPendingFinalizers();
        }
    }
}