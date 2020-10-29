// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.Generic;
using System.Collections.ObjectModel;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Media;
using System.Linq;
using Common;
using MUXControlsTestApp.Utilities;
using TreeViewItem = Microsoft.UI.Xaml.Controls.TreeViewItem;
using NavigationViewItem = Microsoft.UI.Xaml.Controls.NavigationViewItem;
using controls = Microsoft.UI.Xaml.Controls;
using System;

namespace MUXControlsTestApp
{
    [TopLevelTestPage(Name = "Compact", Icon = "CompactSizing.png")]
    public sealed partial class CompactPage : TestPage
    {
        public CompactPage()
        {
            this.InitializeComponent();
        }

        private void AutoSuggestBox_TextChanged(AutoSuggestBox sender, AutoSuggestBoxTextChangedEventArgs args)
        {
            if (args.Reason == AutoSuggestionBoxTextChangeReason.UserInput)
            {
                var suggestList = new List<string>();
                suggestList.Add("001");
                suggestList.Add("002");
                suggestList.Add("003");
                suggestList.Add("004");
                suggestList.Add("005");

                sender.ItemsSource = suggestList.Select(text => sender.Text + text);
            }
        }

        private void VerifyHeight(SimpleVerify simpleVerify, FrameworkElement item, int expectHeight, string controlName)
        {
            if (item != null)
            {
                // Avoid Height because of CornerRadius
                if (Math.Abs(item.ActualHeight - expectHeight) > 0.5)
                {
                    simpleVerify.IsEqual(item.ActualHeight.ToString(), expectHeight, controlName + ".ActualHeight=" + expectHeight);
                }
            }
            else
            {
                simpleVerify.IsTrue(false, "Can't find " + controlName);
            }
        }

        private void VerifyChildHeight(SimpleVerify simpleVerify, FrameworkElement item, string childXName, int expectHeight, string controlName)
        {
            FrameworkElement child = GetChild(item, childXName);
            VerifyHeight(simpleVerify, child, expectHeight, controlName + "." + childXName);
        }

        private FrameworkElement GetChild(FrameworkElement item, string childXName)
        {
            FrameworkElement root = (item == null) ? null: (FrameworkElement)VisualTreeHelper.GetChild(item, 0);
            FrameworkElement child = (root == null) ? null: (FrameworkElement)root.FindName(childXName);
            return child;
        }
        private void RunTest_Click(object sender, RoutedEventArgs e)
        {           
            SimpleVerify simpleVerify = new SimpleVerify();
            FrameworkElement item = ListView.FindVisualChildByType<ListViewItem>();
            VerifyHeight(simpleVerify, item, 32, "ListViewItem");
            item = TreeView.FindVisualChildByType<TreeViewItem>();
            VerifyHeight(simpleVerify, item, 24, "TreeViewItem");

            VerifyHeight(simpleVerify, NavItem1, 32, "NavigationViewItem");

            VerifyChildHeight(simpleVerify, TextBox, "BorderElement", 24, "TextBox");
            VerifyChildHeight(simpleVerify, PasswordBox, "BorderElement", 24, "PasswordBox");

            // AutoSuggestBox used TextBox directly, so we check TextBox
            FrameworkElement child = GetChild(AutoSuggestBox, "TextBox");
            VerifyChildHeight(simpleVerify, child, "BorderElement", 24, "AutoSuggestBox.TextBox");

            VerifyChildHeight(simpleVerify, RichEditBox, "BorderElement", 24, "RichEditBox");
            VerifyChildHeight(simpleVerify, ComboBox, "Background", 24, "TextBox");

            VerifyChildHeight(simpleVerify, TimePicker, "FlyoutButton", 24, "TimePicker");
            VerifyChildHeight(simpleVerify, DatePicker, "FlyoutButton", 24, "DatePicker");

            CompactTestResult.Text = simpleVerify.ToString();
        }

        private void ToggleCompactInAppLevel_Click(object sender, RoutedEventArgs e)
        {
            var dict = Application.Current.Resources.MergedDictionaries.First() as controls.XamlControlsResources;
            dict.UseCompactResources = !dict.UseCompactResources;
        }
    }
}
