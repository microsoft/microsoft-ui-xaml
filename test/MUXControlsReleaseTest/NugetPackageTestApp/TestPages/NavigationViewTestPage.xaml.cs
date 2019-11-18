// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using MUXControlsTestApp;
using System.Collections.Generic;
using NavigationViewItem = Microsoft.UI.Xaml.Controls.NavigationViewItem;

namespace NugetPackageTestApp
{
    public sealed partial class NavigationViewTestPage : TestPage
    {
        public NavigationViewTestPage()
        {
            this.InitializeComponent();
            navView.MenuItemsSource = new List<NavigationViewItem>()
            {
                new NavigationViewItem() { Content = "Item1" },
                new NavigationViewItem() { Content = "Item2" },
                new NavigationViewItem() { Content = "Item3" },
            };
        }
    }
}
