// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.ObjectModel;
using Microsoft.UI.Xaml.Automation;
using Microsoft.UI.Xaml.Controls;

using NavigationView = Microsoft.UI.Xaml.Controls.NavigationView;
using NavigationViewItem = Microsoft.UI.Xaml.Controls.NavigationViewItem;
using NavigationViewSelectionChangedEventArgs = Microsoft.UI.Xaml.Controls.NavigationViewSelectionChangedEventArgs;

namespace MUXControlsTestApp
{
    public sealed partial class NavigationViewFooterMenuItemsSourcePage : TestPage
    {
        public ObservableCollection<NavigationViewItem> FooterItems { get; } = new ObservableCollection<NavigationViewItem>();

        public NavigationViewFooterMenuItemsSourcePage()
        {
            this.InitializeComponent();

            FooterItems.Add(CreateFooterItem("Footer 0"));
            FooterItems.Add(CreateFooterItem("Footer 1"));
            FooterItems.Add(CreateFooterItem("Footer 2"));

            NavView.SelectionChanged += NavView_SelectionChanged;
        }

        private static NavigationViewItem CreateFooterItem(string name)
        {
            var item = new NavigationViewItem { Content = name };
            AutomationProperties.SetName(item, name);
            return item;
        }

        private void NavView_SelectionChanged(NavigationView sender, NavigationViewSelectionChangedEventArgs e)
        {
            if (e.SelectedItemContainer is NavigationViewItem container)
            {
                SelectionChangedResult.Text = container.Content?.ToString() ?? "Null";
            }
            else
            {
                SelectionChangedResult.Text = "Null";
            }
        }
    }
}
