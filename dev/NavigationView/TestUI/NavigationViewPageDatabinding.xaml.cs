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

using NavigationViewDisplayMode = Microsoft.UI.Xaml.Controls.NavigationViewDisplayMode;
using NavigationView = Microsoft.UI.Xaml.Controls.NavigationView;
using NavigationViewSelectionChangedEventArgs = Microsoft.UI.Xaml.Controls.NavigationViewSelectionChangedEventArgs;
using NavigationViewItem = Microsoft.UI.Xaml.Controls.NavigationViewItem;
using NavigationViewDisplayModeChangedEventArgs = Microsoft.UI.Xaml.Controls.NavigationViewDisplayModeChangedEventArgs;
using NavigationViewPaneClosingEventArgs = Microsoft.UI.Xaml.Controls.NavigationViewPaneClosingEventArgs;
using NavigationViewBackButtonVisible = Microsoft.UI.Xaml.Controls.NavigationViewBackButtonVisible;
using NavigationViewBackRequestedEventArgs = Microsoft.UI.Xaml.Controls.NavigationViewBackRequestedEventArgs;
using NavigationViewPaneDisplayMode = Microsoft.UI.Xaml.Controls.NavigationViewPaneDisplayMode;
using MaterialHelperTestApi = Microsoft.UI.Private.Media.MaterialHelperTestApi;
using NavigationViewSelectionFollowsFocus = Microsoft.UI.Xaml.Controls.NavigationViewSelectionFollowsFocus;
using System.Collections.ObjectModel;
using System.Runtime.InteropServices;

namespace MUXControlsTestApp
{
    public sealed partial class NavigationViewPageDatabinding : TestPage
    {

        ObservableCollection<String> codeBehindItems = new ObservableCollection<String>();
        int addedItemNumber = 0;

        public NavigationViewPageDatabinding()
        {
            this.InitializeComponent();

            codeBehindItems.Add("Home");
            codeBehindItems.Add("Apps");
            codeBehindItems.Add("Games");
            codeBehindItems.Add("Music");
            codeBehindItems.Add("TV");
            codeBehindItems.Add("Item 1");
            codeBehindItems.Add("Item 2");
            codeBehindItems.Add("Item 3");
            codeBehindItems.Add("Item 4");

            //NavView.MenuItemsSource = codeBehindItems;
            //NavView.SelectedItem = codeBehindItems[0];
        }

        private void GetMenuItemForContainer(object sender, RoutedEventArgs e)
        {
            var container = NavView.ContainerFromMenuItem("Home") as NavigationViewItem;
            var isSelected = container.IsSelected;
        }
        private void AddMenuItem(object sender, RoutedEventArgs e)
        {
            codeBehindItems.Insert(0, "New Added Item #" + addedItemNumber);
            addedItemNumber++;
        }

        private void NavView_SelectionChanged(NavigationView sender, NavigationViewSelectionChangedEventArgs args)
        {
            //if (args.SelectedItem != null)
            //{
            //    var itemdata = args.SelectedItem as NavigationViewItem;
            //    if (itemdata != null)
            //    {
            //        if (itemdata.Content != null)
            //        {
            //            NavView.Header = itemdata.Content + " as header";
            //        }
            //        else if (args.IsSettingsSelected) // to handle settings without content case in top nav
            //        {
            //            NavView.Header = "Settings as header";
            //        }
            //    }
            //}
        }

        void NavView_ItemInvoked(NavigationView sender, Microsoft.UI.Xaml.Controls.NavigationViewItemInvokedEventArgs args)
        {
            //var item = args.InvokedItem;
            //var itemContainer = args.InvokedItemContainer;
        }

        private void SelectedItemCombobox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            var tag = Convert.ToString(((sender as ComboBox).SelectedItem as ComboBoxItem).Tag);
            if (tag == "Home")
            {
                NavView.SelectedItem = "Home";
            }
            else if (tag == "Apps")
            {
                NavView.SelectedItem = "Apps";
            }
            else if (tag == "Games")
            {
                var container = NavView.ContainerFromMenuItem("Games") as NavigationViewItem;
                container.IsSelected = true;
            }
            else if (tag == "Music")
            {
                NavView.SelectedItem = "Music";
            }
            else if (tag == "Movies")
            {
                NavView.SelectedItem = "Movies";
            }
            else if (tag == "TV")
            {
                NavView.SelectedItem = "TV";
            }
            else if (tag == "Settings")
            {
                NavView.SelectedItem = NavView.SettingsItem;
            }
            else if (tag == "Item 3")
            {
                NavView.SelectedItem = "Item 3";
            }
        }

        private void PaneDisplayModeCombobox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            var tag = Convert.ToString(((sender as ComboBox).SelectedItem as ComboBoxItem).Tag);
            var mode = (NavigationViewPaneDisplayMode)Enum.Parse(typeof(NavigationViewPaneDisplayMode), tag);
            NavView.PaneDisplayMode = mode;
        }

        private void GetNavViewActiveVisualStates_Click(object sender, RoutedEventArgs e)
        {
            var visualstates = Utilities.VisualStateHelper.GetCurrentVisualStateName(NavView);
            NavViewActiveVisualStatesResult.Text = string.Join(",", visualstates);
        }

    }
}
