// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using MUXControlsTestApp.Utilities;

namespace MUXControlsTestApp
{
    public sealed partial class NavigationViewStackPanelHostPage : TestPage
    {
        public NavigationViewStackPanelHostPage()
        {
            this.InitializeComponent();
        }

        // Negative control for #8291: an explicit Height gives NavigationView a finite height inside the StackPanel.
        private void UseFixedHeightHost_Click(object sender, RoutedEventArgs e)
        {
            NavView.Height = 600;
        }

        private void GetLayoutState_Click(object sender, RoutedEventArgs e)
        {
            var chevron = VisualTreeUtils.FindVisualChildByName(Pancakes, "ExpandCollapseChevron") as UIElement;
            var presenter = VisualTreeUtils.FindVisualChildByName(Pancakes, "NavigationViewItemPresenter") as FrameworkElement;
            var scrollViewer = VisualTreeUtils.FindVisualChildByName(NavView, "MenuItemsScrollViewer") as ScrollViewer;
            double createTop = Create.TransformToVisual(NavView).TransformPoint(new global::Windows.Foundation.Point(0, 0)).Y;
            LayoutState.Text =
                $"IsPaneOpen={NavView.IsPaneOpen};IsExpanded={Pancakes.IsExpanded};" +
                $"Chevron={(chevron == null ? "None" : chevron.Visibility.ToString())};" +
                $"PresenterWidth={presenter?.ActualWidth};ChildHeight={WithSyrup.ActualHeight};CreateTop={createTop};" +
                $"MenuMaxHeight={scrollViewer?.MaxHeight};MenuHeight={scrollViewer?.ActualHeight};MenuScrollable={scrollViewer?.ScrollableHeight};" +
                $"NavViewHeight={NavView.ActualHeight};PancakesHeight={Pancakes.ActualHeight};CreateOffset={Create.ActualOffset.Y};IsSelected={Pancakes.IsSelected}";
        }
    }
}
