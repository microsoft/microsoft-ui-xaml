// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Navigation;
using Microsoft.UI.Xaml.Automation;
using Windows.ApplicationModel.Core;

using NavigationView = Microsoft.UI.Xaml.Controls.NavigationView;
using NavigationViewSelectionChangedEventArgs = Microsoft.UI.Xaml.Controls.NavigationViewSelectionChangedEventArgs;
using NavigationViewItem = Microsoft.UI.Xaml.Controls.NavigationViewItem;
using System.ComponentModel;
using System.Runtime.CompilerServices;
using System.Collections.Generic;
using System.Collections.ObjectModel;

namespace MUXControlsTestApp
{
    public sealed partial class NavigationViewMenuItemsSourcePage : TestPage
    {
        public ObservableCollection<NavViewPageData> Pages = new ObservableCollection<NavViewPageData>();


        public NavigationViewMenuItemsSourcePage()
        {
            this.InitializeComponent();

            Pages.Add(new NavViewPageData("First Item"));
            Pages.Add(new NavViewPageData("Second Item"));
            Pages.Add(new NavViewPageData("Third Item"));
            Pages.Add(new NavViewPageData("Fourth Item"));
            Pages.Add(new NavViewPageData("Fifth Item"));
            Pages.Add(new NavViewPageData("Sixth Item"));
            Pages.Add(new NavViewPageData("Seventh Item"));
        }

        private void NavigationView_ItemInvoked(NavigationView sender, Microsoft.UI.Xaml.Controls.NavigationViewItemInvokedEventArgs args)
        {
            Pages.Add(new NavViewPageData("Menu Item NEW"));
            Pages.RemoveAt(0);
        }
    }
}
