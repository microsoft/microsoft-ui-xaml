// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml.Controls;

namespace MUXControlsTestApp
{
    public sealed partial class NavigationViewIsPaneOpenPage : TestPage
    {
        public NavigationViewIsPaneOpenPage()
        {
            this.InitializeComponent();

        }

        private void NavigationView_ItemInvoked(Microsoft.UI.Xaml.Controls.NavigationView sender, Microsoft.UI.Xaml.Controls.NavigationViewItemInvokedEventArgs args)
        {
            if((args.InvokedItem as string) == "Apps")
            {
                CollapsedItem.Visibility = Windows.UI.Xaml.Visibility.Visible;

                NavView.SelectedItem = CollapsedItem;
            }
        }
    }
}
