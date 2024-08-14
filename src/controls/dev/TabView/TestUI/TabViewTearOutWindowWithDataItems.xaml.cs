// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Navigation;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Collections.Specialized;
using System.Linq;
using Windows.Foundation.Collections;
using Windows.Graphics;

namespace MUXControlsTestApp
{
    public sealed partial class TabViewTearOutWindowWithDataItems : Window
    {
        public static readonly SizeInt32 DefaultWindowSize = new(1000, 300);

        public ObservableCollection<object> StringList { get; set; }

        public TabViewTearOutWindowWithDataItems()
        {
            this.InitializeComponent();

            StringList = [
                "Document 1",
                "Document 2",
                "Document 3",
            ];

            Title = "MUXControlsTestApp.Desktop - Secondary TabViewTearOutWindow";
        }

        // A user has begun interacting with a tab, such that they might want to tear it out into its own window.
        // We'll create a hidden window with the tab's contents that will be shown if the user tears out the tab.
        private void OnTabTearOutWindowRequested(TabView _, TabViewTabTearOutWindowRequestedEventArgs args)
        {
            TabViewTearOutWindowWithDataItems newWindow = new() { StringList = [.. args.Items], ExtendsContentIntoTitleBar = true };
            newWindow.AppWindow.Hide();
            newWindow.AppWindow.Resize(DefaultWindowSize);
            args.NewWindowId = newWindow.AppWindow.Id;
        }

        // A user has torn out a tab, so now is the time we need to remove the torn-out item from our item list.
        private void OnTabTearOutRequested(TabView _, TabViewTabTearOutRequestedEventArgs args)
        {
            for (int i = 0; i < args.Items.Length; i++)
            {
                int index = StringList.IndexOf(args.Items[i]);

                if (index >= 0)
                {
                    StringList.RemoveAt(index);
                }
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
        private void OnExternalTornOutTabsDropped(TabView _, TabViewExternalTornOutTabsDroppedEventArgs args)
        {
            for (int i = 0; i < args.Items.Length; i++)
            {
                int index = StringList.IndexOf(args.Items[i]);

                if (index >= 0)
                {
                    StringList.RemoveAt(index);
                }
            }

            for (int i = 0; i < args.Items.Length; i++)
            {
                StringList.Insert(Math.Min(args.DropIndex + i, StringList.Count), args.Items[i]);
            }
        }
    }
}
