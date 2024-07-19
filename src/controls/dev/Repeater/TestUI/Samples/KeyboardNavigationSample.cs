﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Linq;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Input;

namespace MUXControlsTestApp.Samples
{
    public sealed partial class KeyboardNavigationSample : TestPage
    {
        public KeyboardNavigationSample()
        {
            this.InitializeComponent();
            HookItemsRepeaterEvents(itemsRepeater);

            GotFocus += KeyboardNavigationSample_GotFocus;
        }

        ~KeyboardNavigationSample()
        {
        }

        private void KeyboardNavigationSample_GotFocus(object sender, RoutedEventArgs e)
        {
            FrameworkElement fe = e.OriginalSource as FrameworkElement;

            if (fe != null)
            {
                AppendLog($"KeyboardNavigationSample.GotFocus OriginalSource={e.OriginalSource}, Name={fe.Name}");
            }
            else
            {
                AppendLog($"KeyboardNavigationSample.GotFocus OriginalSource={e.OriginalSource}");
            }
        }

        private void CmbScrollViewTabNavigation_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (scrollView != null && cmbScrollViewTabNavigation != null)
            {
                scrollView.TabNavigation = (KeyboardNavigationMode)cmbScrollViewTabNavigation.SelectedIndex;
            }
        }

        private void CmbItemsRepeaterTabFocusNavigation_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (itemsRepeater != null && cmbItemsRepeaterTabFocusNavigation != null)
            {
                itemsRepeater.TabFocusNavigation = (KeyboardNavigationMode)cmbItemsRepeaterTabFocusNavigation.SelectedIndex;
            }
        }

        private void BtnSetItemsSource_Click(object sender, RoutedEventArgs e)
        {
            itemsRepeater.ItemsSource = Enumerable.Range(0, 20).Select(i => string.Format("Item #{0}", i));
        }

        private void BtnResetItemsSource_Click(object sender, RoutedEventArgs e)
        {
            itemsRepeater.ItemsSource = null;
        }

        private void BtnClearLogs_Click(object sender, RoutedEventArgs e)
        {
            lvLogs.Items.Clear();
        }

        private void HookItemsRepeaterEvents(ItemsRepeater itemsRepeater)
        {
            if (itemsRepeater != null)
            {
                itemsRepeater.ElementPrepared += ItemsRepeater_ElementPrepared;
                itemsRepeater.ElementClearing += ItemsRepeater_ElementClearing;
            }
        }

        private void AppendLog(string log)
        {
            lvLogs.Items.Add(log);
        }

        private void ItemsRepeater_ElementPrepared(ItemsRepeater sender, ItemsRepeaterElementPreparedEventArgs args)
        {
            AppendLog($"ItemsRepeater.ElementPrepared Index={args.Index}, Element={args.Element}");
        }

        private void ItemsRepeater_ElementClearing(ItemsRepeater sender, ItemsRepeaterElementClearingEventArgs args)
        {
            AppendLog($"ItemsRepeater.ElementClearing Element={args.Element}");
        }
    }
}
