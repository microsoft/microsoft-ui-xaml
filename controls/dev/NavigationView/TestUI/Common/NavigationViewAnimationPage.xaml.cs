// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;

namespace MUXControlsTestApp
{
    public sealed partial class NavigationViewAnimationPage : TestPage
    {
        public NavigationViewAnimationPage()
        {
            this.InitializeComponent();
        }
        private void NavView_OnItemInvoked(NavigationView sender, NavigationViewItemInvokedEventArgs args)
        {
            NavigateToPage(args.InvokedItemContainer.Tag);
        }

        private void NavigateToPage(object pageTag)
        {
            if (pageTag == null)
            {
                pageTag = "BlankPage1";
            }
            var pageName = "MUXControlsTestApp.NavigationView" + pageTag;
            var pageType = Type.GetType(pageName);

            ContentFrame.Navigate(pageType);
        }

        private void FlipOrientation_Click(object sender, RoutedEventArgs e)
        {
            NavView.PaneDisplayMode = NavView.PaneDisplayMode == NavigationViewPaneDisplayMode.Top ? NavigationViewPaneDisplayMode.Auto : NavigationViewPaneDisplayMode.Top;
        }
    }
}
