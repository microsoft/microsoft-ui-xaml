// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Windows.Foundation;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Dispatching;
using System.Collections.Generic;

namespace MUXControlsTestApp
{

    [TopLevelTestPage(Name = "MenuFlyout", Icon = "MenuFlyout.png")]
    public sealed partial class MenuFlyoutPage : TestPage
    {
        private MenuFlyout sharedFlyout;
        public MenuFlyoutPage()
        {
            this.InitializeComponent();

            sharedFlyout = (MenuFlyout)Resources["SampleContextMenu"];
        }

        private void TestMenuFlyoutItemClick(object sender, object e)
        {
            TestMenuFlyoutItemHeightTextBlock.Text = $"{TestMenuFlyoutItem.ActualHeight}";
            TestMenuFlyoutItemWidthTextBlock.Text = $"{TestMenuFlyoutItem.ActualWidth}";
        }

        private void Grid_ContextRequested(UIElement sender, ContextRequestedEventArgs args)
        {
            var requestedElement = sender as FrameworkElement;

            if (args.TryGetPosition(requestedElement, out Point point))
            {
                sharedFlyout.ShowAt(requestedElement, point);
            }
            else
            {
                sharedFlyout.ShowAt(requestedElement);
            }
        }

        DispatcherQueueTimer itemChangeTimer;
        List<MenuFlyoutItemBase> itemsToToggle;

        private void OnMenuFlyoutChangingItemsOpened(object sender, object e)
        {
            IList<MenuFlyoutItemBase> items = MenuFlyoutChangingItems.Items;

            if (itemChangeTimer == null)
            {
                itemsToToggle = new List<MenuFlyoutItemBase> {
                    items[4],
                    items[5],
                    items[6],
                    items[7]
                };

                itemChangeTimer = DispatcherQueue.GetForCurrentThread().CreateTimer();
                itemChangeTimer.Interval = System.TimeSpan.FromSeconds(1);
                itemChangeTimer.Tick += (timer, args) =>
                {
                    if (items.Count == 8)
                    {
                        itemsToToggle.ForEach(item => items.Remove(item));
                    }
                    else
                    {
                        itemsToToggle.ForEach(item => items.Add(item));
                    }
                };
            }

            itemChangeTimer.Start();
        }

        private void OnMenuFlyoutChangingItemsClosed(object sender, object e)
        {
            if (itemChangeTimer != null)
            {
                itemChangeTimer.Stop();
            }
        }

        DispatcherQueueTimer subItemChangeTimer;
        List<MenuFlyoutItemBase> subItemsToToggle;

        private void OnMenuFlyoutChangingSubItemsOpened(object sender, object e)
        {
            IList<MenuFlyoutItemBase> subItems = ((MenuFlyoutSubItem)MenuFlyoutChangingSubItems.Items[0]).Items;

            if (subItemChangeTimer == null)
            {
                subItemsToToggle = new List<MenuFlyoutItemBase> {
                    subItems[4],
                    subItems[5],
                    subItems[6],
                    subItems[7]
                };

                subItemChangeTimer = DispatcherQueue.GetForCurrentThread().CreateTimer();
                subItemChangeTimer.Interval = System.TimeSpan.FromSeconds(1);
                subItemChangeTimer.Tick += (timer, args) =>
                {
                    if (subItems.Count == 8)
                    {
                        subItemsToToggle.ForEach(item => subItems.Remove(item));
                    }
                    else
                    {
                        subItemsToToggle.ForEach(item => subItems.Add(item));
                    }
                };
            }

            subItemChangeTimer.Start();
        }

        private void OnMenuFlyoutChangingSubItemsClosed(object sender, object e)
        {
            if (itemChangeTimer != null)
            {
                itemChangeTimer.Stop();
            }
        }
    }
}
