// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Windows.UI;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Shapes;

using ScrollingPresenter = Microsoft.UI.Xaml.Controls.Primitives.ScrollingPresenter;

namespace MUXControlsTestApp
{
    public sealed partial class ScrollingPresenterLeakDetectionPage : TestPage
    {
        private ScrollingPresenter scrollingPresenter = null;
        
        public ScrollingPresenterLeakDetectionPage()
        {
            this.InitializeComponent();
        }

        private void BtnCreateScrollingPresenter_Click(object sender, Windows.UI.Xaml.RoutedEventArgs e)
        {
            scrollingPresenter = new ScrollingPresenter();
            scrollingPresenter.Name = "dynamicScrollingPresenter";
            scrollingPresenter.Width = 300.0;
            scrollingPresenter.Height = 400.0;
            scrollingPresenter.Margin = new Thickness(1);
            scrollingPresenter.Background = new SolidColorBrush(Colors.HotPink);
            scrollingPresenter.VerticalAlignment = VerticalAlignment.Top;

            Rectangle rect = new Rectangle();
            rect.Width = 900.0;
            rect.Height = 1200.0;
            rect.Fill = new SolidColorBrush(Colors.DarkRed);
            scrollingPresenter.Content = rect;

            btnCreateScrollingPresenter.IsEnabled = false;
            btnAddScrollingPresenter.IsEnabled = true;
            btnDeleteScrollingPresenter.IsEnabled = true;
        }

        private void BtnAddScrollingPresenter_Click(object sender, Windows.UI.Xaml.RoutedEventArgs e)
        {
            Grid.SetRow(scrollingPresenter, 5);
            grid.Children.Add(scrollingPresenter);

            btnAddScrollingPresenter.IsEnabled = false;
            btnDeleteScrollingPresenter.IsEnabled = false;
            btnRemoveScrollingPresenter.IsEnabled = true;
        }

        private void BtnRemoveScrollingPresenter_Click(object sender, Windows.UI.Xaml.RoutedEventArgs e)
        {
            grid.Children.Remove(scrollingPresenter);

            btnRemoveScrollingPresenter.IsEnabled = false;
            btnAddScrollingPresenter.IsEnabled = true;
            btnDeleteScrollingPresenter.IsEnabled = true;
        }

        private void BtnDeleteScrollingPresenter_Click(object sender, Windows.UI.Xaml.RoutedEventArgs e)
        {
            if (scrollingPresenter == null || grid.Children.Count == 6)
                return;

            scrollingPresenter.Content = null;
            scrollingPresenter = null;

            btnDeleteScrollingPresenter.IsEnabled = false;
            btnAddScrollingPresenter.IsEnabled = false;
            btnCreateScrollingPresenter.IsEnabled = true;
        }

        private void BtnGarbageCollect_Click(object sender, Windows.UI.Xaml.RoutedEventArgs e)
        {
            System.GC.Collect();
            System.GC.WaitForPendingFinalizers();
        }
    }
}