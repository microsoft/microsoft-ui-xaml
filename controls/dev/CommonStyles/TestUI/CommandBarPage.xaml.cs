// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;

namespace MUXControlsTestApp
{
    [TopLevelTestPage(Name = "CommandBar")]
    public sealed partial class CommandBarPage : Page
    {
        public CommandBarPage()
        {
            this.InitializeComponent();
        }

        public void TopAppBarButton_Click(object sender, RoutedEventArgs args)
        {
            this.TopAppBar = null;
        }

        public void OpenCloseAppBarsButton_Click(object sender, RoutedEventArgs args)
        {
            TopAppBar.IsOpen = !TopAppBar.IsOpen;
            BottomAppBar.IsOpen = !BottomAppBar.IsOpen;

            if (TopAppBar.IsOpen)
            {
                OpenCloseAppBarsButton.Content = "Close App Bars";
            }
            else
            {
                OpenCloseAppBarsButton.Content = "Open App Bars";
            }
        }

        public void StickyUnstickyAppBarsButton_Click(object sender, RoutedEventArgs args)
        {
            TopAppBar.IsSticky = !TopAppBar.IsSticky;
            BottomAppBar.IsSticky = !BottomAppBar.IsSticky;

            if (TopAppBar.IsSticky)
            {
                StickyUnstickyAppBarsButton.Content = "Make App Bars Unsticky";
            }
            else
            {
                StickyUnstickyAppBarsButton.Content = "Make App Bars Sticky";
            }
        }
    }
}
