// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Navigation;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Collections.Specialized;
using System.Linq;
using System.Reflection;
using Windows.Foundation.Collections;

namespace MUXControlsTestApp
{
    public sealed partial class TabViewTearOutWindowWithoutDataItems : Window
    {
        TabViewTearOutWindowWithoutDataItems tearOutWindow;

        public TabViewTearOutWindowWithoutDataItems()
        {
            this.InitializeComponent();
            Title = "MUXControlsTestApp.Desktop - Secondary TabViewTearOutWindow";
        }

        // A user has begun interacting with a tab, such that they might want to tear it out into its own window.
        // We'll create a hidden window with the tab's contents that will be shown if the user tears out the tab.
        private void OnTabTearOutWindowRequested(TabView _, TabViewTabTearOutWindowRequestedEventArgs args)
        {
            tearOutWindow = new();
            tearOutWindow.TearOutTabsTabView.TabItems.Clear();
            tearOutWindow.AppWindow.Hide();
            tearOutWindow.AppWindow.Resize(new Windows.Graphics.SizeInt32(800, 300));
            args.NewWindowId = tearOutWindow.AppWindow.Id;
        }

        // A user has torn out a tab, so now is the time we need to remove the torn-out item from our item list.
        private void OnTabTearOutRequested(TabView tabView, TabViewTabTearOutRequestedEventArgs args)
        {
            foreach (var tab in args.Tabs.Cast<TabViewItem>())
            {
                tabView.TabItems.Remove(tab);
                tearOutWindow.TearOutTabsTabView.TabItems.Add(tab);
            }
        }

        // A tab being dragged from within this process has been dropped on another tab view.  This gives the tab view
        // the opportunity to check to see whether it actually want to accepts the new tab.  In the case of this test app,
        // however, we'll just always accept a dropped tab.
        private void OnExternalTornOutTabsDropping(TabView _, TabViewExternalTornOutTabsDroppingEventArgs args)
        {
            args.AllowDrop = true;
        }

        // The tab being dragged has now been dropped onto another tab view.  We'll add its item to our item list.
        // The tab view will take care of disposing of the window.
        private void OnExternalTornOutTabsDropped(TabView tabView, TabViewExternalTornOutTabsDroppedEventArgs args)
        {
            int insertionIndex = args.DropIndex;

            foreach (var tab in args.Tabs.Cast<TabViewItem>())
            {
                TabView otherTabView = null;
                DependencyObject current = tab;

                while (otherTabView == null)
                {
                    if (current is TabView foundTabView)
                    {
                        otherTabView = foundTabView;
                    }

                    current = VisualTreeHelper.GetParent(current);
                }

                otherTabView?.TabItems.Remove(tab);
                tabView.TabItems.Insert(insertionIndex++, tab);
            }
        }
    }
}
