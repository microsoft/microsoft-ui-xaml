// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Navigation;
using Windows.UI.Xaml.Automation;
using Windows.ApplicationModel.Core;

using NavigationView = Microsoft.UI.Xaml.Controls.NavigationView;
using NavigationViewSelectionChangedEventArgs = Microsoft.UI.Xaml.Controls.NavigationViewSelectionChangedEventArgs;
using NavigationViewItem = Microsoft.UI.Xaml.Controls.NavigationViewItem;

namespace MUXControlsTestApp
{
    public sealed partial class NavigationViewPageDataContext : TestPage
    {
        public NavigationViewPageDataContext()
        {
            this.InitializeComponent();

            for (int i = 0; i < 8; i++)
            {
                var nvi = new NavigationViewItem();
                var itemString = "Item #" + i;
                nvi.Content = itemString;
                nvi.DataContext = itemString + "_DataContext";
                NavView.MenuItems.Add(nvi);
            }

            NavView.SelectedItem = NavView.MenuItems[0];
        }

        private void NavView_SelectionChanged(NavigationView sender, NavigationViewSelectionChangedEventArgs args)
        {
            var nvi = args.SelectedItem as NavigationViewItem;
            NavViewSelectedDataContext.Text = nvi.DataContext as string;
        }

        private void GetNavViewActiveVisualStates_Click(object sender, RoutedEventArgs e)
        {
            var visualstates = Utilities.VisualStateHelper.GetCurrentVisualStateName(NavView);
            NavViewActiveVisualStatesResult.Text = string.Join(",", visualstates);
        }
    }
}
