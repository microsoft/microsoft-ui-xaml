// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using MUXControlsTestApp;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Media;

namespace NugetPackageTestApp
{
    public sealed partial class NavigationViewTestPage : TestPage
    {
        public NavigationViewTestPage()
        {
            this.InitializeComponent();
            Loaded += NavigationViewTestPage_Loaded;
        }

        private void NavigationViewTestPage_Loaded(object sender, RoutedEventArgs e)
        {
            GetHeaderContentMargins();
        }

        private void BtnGetHeaderContentMargins_Click(object sender, RoutedEventArgs e)
        {
            GetHeaderContentMargins();
        }

        private void GetHeaderContentMargins()
        {
            tblTopHeaderContentMarginResult.Text = GetHeaderContentMargin(navViewTop);
            tblLeftMinimalHeaderContentMarginResult.Text = GetHeaderContentMargin(navViewLeftMinimal);
        }

        private string GetHeaderContentMargin(Microsoft.UI.Xaml.Controls.NavigationView navView)
        {
            Grid rootGrid = VisualTreeHelper.GetChild(navView, 0) as Grid;
            if (rootGrid != null)
            {
                var fe = rootGrid.FindName("HeaderContent") as FrameworkElement;
                if (fe != null)
                {
                    return fe.Margin.ToString();
                }
            }
            return string.Empty;
        }
    }
}
