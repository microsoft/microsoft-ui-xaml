﻿using Microsoft.UI.Xaml.Controls;
using MUXControlsTestApp.Utilities;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Navigation;

// The Blank Page item template is documented at https://go.microsoft.com/fwlink/?LinkId=234238

namespace MUXControlsTestApp
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class TabViewTabClosingBehaviorPage : Page
    {
        private int _newTabNumber = 0;

        public TabViewTabClosingBehaviorPage()
        {
            this.InitializeComponent();
        }

        public void AddButtonClick(object sender, object e)
        {
            if (Tabs != null)
            {
                TabViewItem item = new TabViewItem();
                item.Header = "New Tab " + _newTabNumber;
                item.Content = item.Header;

                Tabs.TabItems.Add(item);

                _newTabNumber++;
            }
        }

        private void TabViewTabCloseRequested(object sender, Microsoft.UI.Xaml.Controls.TabViewTabCloseRequestedEventArgs e)
        {
            Tabs.TabItems.Remove(e.Tab);

            TabViewWidth.Text = Tabs.ActualWidth.ToString();
            TabViewHeaderWidth.Text = Tab0.Width.ToString();

            var scrollButtonStateValue = "";

            var scrollIncreaseButton = VisualTreeUtils.FindVisualChildByName(Tabs, "ScrollIncreaseButton") as RepeatButton;
            var scrollDecreaseButton = VisualTreeUtils.FindVisualChildByName(Tabs, "ScrollDecreaseButton") as RepeatButton;

            scrollButtonStateValue += scrollIncreaseButton.IsEnabled + ";";
            scrollButtonStateValue += scrollDecreaseButton.IsEnabled + ";";

            ScrollButtonStatus.Text = scrollButtonStateValue;
        }

        public void GetFirstItemWidthButton_Click(object sender, RoutedEventArgs e)
        {
            // This is the smallest width that fits our content without any scrolling.
            TabViewWidth.Text = Tabs.ActualWidth.ToString();

            // Header width
            TabViewHeaderWidth.Text = Tab0.Width.ToString();
        }
        public void GetActualWidthsButton_Click(object sender, RoutedEventArgs e)
        {
            // This is the smallest width that fits our content without any scrolling.
            TabViewWidth.Text = Tabs.ActualWidth.ToString();

            // Header width
            TabViewHeaderWidth.Text = Tab0.Width.ToString();
        }

        public void IncreaseScrollButton_Click(object sender, RoutedEventArgs e)
        {
            var sv = VisualTreeUtils.FindVisualChildByName(Tabs, "ScrollViewer") as ScrollViewer;
            sv.ChangeView(10000, null, null, disableAnimation: true);
        }

        private void RemoveMiddleItem_Click(object sender, RoutedEventArgs e)
        {
            Tabs.TabItems.RemoveAt(1);
        }

        private void RemoveLastItem_Click(object sender, RoutedEventArgs e)
        {
            Tabs.TabItems.RemoveAt(Tabs.TabItems.Count - 1);
        }
    }
}
