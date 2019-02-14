// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Automation;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Navigation;

#if !BUILD_WINDOWS
using NavigationViewPaneDisplayMode = Microsoft.UI.Xaml.Controls.NavigationViewPaneDisplayMode;
using MaterialHelperTestApi = Microsoft.UI.Private.Media.MaterialHelperTestApi;
using NavigationView = Microsoft.UI.Xaml.Controls.NavigationView;
using NavigationViewItemInvokedEventArgs = Microsoft.UI.Xaml.Controls.NavigationViewItemInvokedEventArgs;
#endif

namespace MUXControlsTestApp
{

    public sealed partial class NavigationViewInitPage : TestPage
    {
        ObservableCollection<string> m_menuItems;
        LinkedList<string> m_menuItemsEnumerable = null;

        public NavigationViewInitPage()
        {
            this.InitializeComponent();

            MaterialHelperTestApi.IgnoreAreEffectsFast = true;
            MaterialHelperTestApi.SimulateDisabledByPolicy = false;

            m_menuItems = new ObservableCollection<string>();

            m_menuItems.Add("Menu Item 1");
            m_menuItems.Add("Menu Item 2");
            m_menuItems.Add("Menu Item 3");

            NavView.MenuItemsSource = m_menuItems;
            NavView.SelectedItem = m_menuItems[0];
        }

        protected override void OnNavigatedFrom(NavigationEventArgs e)
        {
            // Unset all override flags to avoid impacting subsequent tests
            MaterialHelperTestApi.IgnoreAreEffectsFast = false;
            MaterialHelperTestApi.SimulateDisabledByPolicy = false;
            base.OnNavigatedFrom(e);
        }

        private void AddButton_Click(object sender, RoutedEventArgs e)
        {
            if (m_menuItemsEnumerable == null)
            {
                m_menuItems.Add("New Menu Item");
            }
            else
            {
                m_menuItemsEnumerable.AddLast("New Menu Item");
            }
        }

        private void RemoveButton_Click(object sender, RoutedEventArgs e)
        {
            if (m_menuItemsEnumerable == null)
            {
                m_menuItems.RemoveAt(m_menuItems.Count - 1);
            }
            else
            {
                m_menuItemsEnumerable.RemoveLast();
            }
        }

        private void ChangeToIEnumerableButton_Clicks(object sender, RoutedEventArgs e)
        {
            var newMenuItems = new LinkedList<string>();
            newMenuItems.AddLast("IIterator/Enumerable/LinkedList Item 1");
            newMenuItems.AddLast("IIterator/Enumerable/LinkedList Item 2");
            newMenuItems.AddLast("IIterator/Enumerable/LinkedList Item 3");

            NavView.MenuItemsSource = newMenuItems;
        }

        private void FlipOrientation_Click(object sender, RoutedEventArgs e)
        {
            NavView.PaneDisplayMode = NavView.PaneDisplayMode == NavigationViewPaneDisplayMode.Top ? NavigationViewPaneDisplayMode.Auto : NavigationViewPaneDisplayMode.Top;
        }

        private void SwitchFrame_Click(object sender, RoutedEventArgs e)
        {
            if (Frame2.Content == null)
            {
                var content = Frame1.Content;
                Frame1.Content = null;
                Frame2.Content = content;
            }
            else
            {
                var content = Frame2.Content;
                Frame2.Content = null;
                Frame1.Content = content;
            }
        }

        private void NavView2_ItemInvoked(NavigationView sender, NavigationViewItemInvokedEventArgs args)
        {
            if (Frame2.Content == null)
            {
                MyLocationResult.Text = "Frame1";
            }
            else
            {
                MyLocationResult.Text = "Frame2";
            }
        }
    }
}
