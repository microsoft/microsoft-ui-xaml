// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml.Controls;
using System;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Media;
using Windows.Foundation;

using NavigationViewPaneDisplayMode = Microsoft.UI.Xaml.Controls.NavigationViewPaneDisplayMode;
using NavigationViewBackButtonVisible = Microsoft.UI.Xaml.Controls.NavigationViewBackButtonVisible;
using NavigationViewItem = Microsoft.UI.Xaml.Controls.NavigationViewItem;


namespace MUXControlsTestApp
{
    public sealed partial class NavigationViewCompactPaneLengthTestPage : TestPage
    {
        public NavigationViewCompactPaneLengthTestPage()
        {
            this.InitializeComponent();
        }
        
        private void CompactPaneLength_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            var tag = Convert.ToDouble(((sender as ComboBox).SelectedItem as ComboBoxItem).Tag);
            NavView.CompactPaneLength = tag;
        }

        private void OpenPaneLength_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            var tag = Convert.ToDouble(((sender as ComboBox).SelectedItem as ComboBoxItem).Tag);
            NavView.OpenPaneLength = tag;
        }

        private void PaneToggleButtonVisiblityCheckbox_Checked(object sender, RoutedEventArgs e)
        {
            NavView.IsPaneToggleButtonVisible = true;
        }

        private void PaneToggleButtonVisiblityCheckbox_Unchecked(object sender, RoutedEventArgs e)
        {
            NavView.IsPaneToggleButtonVisible = false;
        }

        private void PaneDisplayModeCombobox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            var tag = Convert.ToString(((sender as ComboBox).SelectedItem as ComboBoxItem).Tag);
            var mode = (NavigationViewPaneDisplayMode)Enum.Parse(typeof(NavigationViewPaneDisplayMode), tag);
            NavView.PaneDisplayMode = mode;
        }

        private void BackButtonVisibilityCheckbox_Checked(object sender, RoutedEventArgs e)
        {
            NavView.IsBackButtonVisible = NavigationViewBackButtonVisible.Visible;
        }

        private void BackButtonVisibilityCheckbox_Unchecked(object sender, RoutedEventArgs e)
        {
            NavView.IsBackButtonVisible = NavigationViewBackButtonVisible.Collapsed;
        }

        private void BackButtonEnabledCheckbox_Checked(object sender, RoutedEventArgs e)
        {
            NavView.IsBackEnabled = true;
        }

        private void BackButtonEnabledCheckbox_Unchecked(object sender, RoutedEventArgs e)
        {
            NavView.IsBackEnabled = false;
        }

        private void CheckMenuItemsOffset_Click(object sender, RoutedEventArgs e)
        {
            bool allCorrect = true;
            foreach(var item in NavView.MenuItems)
            {
                if(item as NavigationViewItem == null)
                {
                    continue;
                }
                var transform = GetContentBox(item as NavigationViewItem).TransformToVisual(null) as MatrixTransform;
                if(Math.Abs(transform.Matrix.OffsetX - NavView.CompactPaneLength) > double.Epsilon)
                {
                    allCorrect = false;
                }
            }
            MenuItemsCorrectOffset.IsChecked = allCorrect;

        }


        /* Helper functions */
        private UIElement GetContentBox(NavigationViewItem element)
        {
            if(element == null)
            {
                return null;
            }
            // Path we are using here: NVIGrid->NavigationViewItemPresenter->LayoutRoot
            // ->PresenterContentRootGrid->ContentGrid->ContentPresenter
            var elementGrid = VisualTreeHelper.GetChild(element,0);
            var presenter = VisualTreeHelper.GetChild(elementGrid, 0);
            var layoutRoot = VisualTreeHelper.GetChild(presenter, 0);
            var presenterContentRootGrid = VisualTreeHelper.GetChild(layoutRoot, 1);
            var contentGrid = VisualTreeHelper.GetChild(presenterContentRootGrid, 1);
            var contentPresenter = VisualTreeHelper.GetChild(contentGrid, 1);
            return contentPresenter as UIElement;

        }
    }
}
