// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Windows.UI;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Shapes;

using ScrollPresenter = Microsoft.UI.Xaml.Controls.Primitives.ScrollPresenter;

namespace MUXControlsTestApp
{
    public sealed partial class ScrollPresenterLeakDetectionPage : TestPage
    {
        private ScrollPresenter scrollPresenter = null;
        
        public ScrollPresenterLeakDetectionPage()
        {
            this.InitializeComponent();
        }

        private void BtnCreateScrollPresenter_Click(object sender, Windows.UI.Xaml.RoutedEventArgs e)
        {
            scrollPresenter = new ScrollPresenter();
            scrollPresenter.Name = "dynamicScrollPresenter";
            scrollPresenter.Width = 300.0;
            scrollPresenter.Height = 400.0;
            scrollPresenter.Margin = new Thickness(1);
            scrollPresenter.Background = new SolidColorBrush(Colors.HotPink);
            scrollPresenter.VerticalAlignment = VerticalAlignment.Top;

            Rectangle rect = new Rectangle();
            rect.Width = 900.0;
            rect.Height = 1200.0;
            rect.Fill = new SolidColorBrush(Colors.DarkRed);
            scrollPresenter.Content = rect;

            btnCreateScrollPresenter.IsEnabled = false;
            btnAddScrollPresenter.IsEnabled = true;
            btnDeleteScrollPresenter.IsEnabled = true;
        }

        private void BtnAddScrollPresenter_Click(object sender, Windows.UI.Xaml.RoutedEventArgs e)
        {
            Grid.SetRow(scrollPresenter, 5);
            grid.Children.Add(scrollPresenter);

            btnAddScrollPresenter.IsEnabled = false;
            btnDeleteScrollPresenter.IsEnabled = false;
            btnRemoveScrollPresenter.IsEnabled = true;
        }

        private void BtnRemoveScrollPresenter_Click(object sender, Windows.UI.Xaml.RoutedEventArgs e)
        {
            grid.Children.Remove(scrollPresenter);

            btnRemoveScrollPresenter.IsEnabled = false;
            btnAddScrollPresenter.IsEnabled = true;
            btnDeleteScrollPresenter.IsEnabled = true;
        }

        private void BtnDeleteScrollPresenter_Click(object sender, Windows.UI.Xaml.RoutedEventArgs e)
        {
            if (scrollPresenter == null || grid.Children.Count == 6)
                return;

            scrollPresenter.Content = null;
            scrollPresenter = null;

            btnDeleteScrollPresenter.IsEnabled = false;
            btnAddScrollPresenter.IsEnabled = false;
            btnCreateScrollPresenter.IsEnabled = true;
        }

        private void BtnGarbageCollect_Click(object sender, Windows.UI.Xaml.RoutedEventArgs e)
        {
            System.GC.Collect();
            System.GC.WaitForPendingFinalizers();
        }
    }
}