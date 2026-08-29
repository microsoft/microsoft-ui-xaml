// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Markup;
using Windows.UI;
using System.Windows.Input;
using Windows.Foundation.Metadata;
using System.Collections.Generic;
using Microsoft.UI.Xaml.Automation;

namespace MUXControlsTestApp
{
    [TopLevelTestPage(Name = "RadioMenuFlyoutItem")]
    public sealed partial class RadioMenuFlyoutItemPage : TestPage
    {
        Dictionary<string, TextBlock> itemStates;

        public RadioMenuFlyoutItemPage()
        {
            this.InitializeComponent();

            itemStates = new Dictionary<string, TextBlock>();

            IconMenuFlyoutItem.Icon = new SymbolIcon(Symbol.Calendar);
            IconRadioMenuFlyoutItem.Icon = new SymbolIcon(Symbol.Calculator);
            IconRadioMenuFlyoutItem2.Icon = new SymbolIcon(Symbol.Calculator);

            ItemNames.Spacing = 4;
            ItemStates.Spacing = 4;

            // register all RadioMenuFlyoutItems
            foreach (MenuFlyoutItemBase item in ButtonMenuFlyout.Items)
            {
                RegisterItem(item);
            }
            foreach(MenuFlyoutItemBase item in ButtonSubMenuFlyout.Items)
            {
                RegisterItem(item);
            }
            foreach (MenuFlyoutItemBase item in RadioSubMenu.Items)
            {
                RegisterItem(item);
            }
        }

        private void RegisterItem(MenuFlyoutItemBase item)
        {
            if (item is RadioMenuFlyoutItem)
            {
                RadioMenuFlyoutItem radioItem = item as RadioMenuFlyoutItem;

                radioItem.RegisterPropertyChangedCallback(RadioMenuFlyoutItem.IsCheckedProperty, new DependencyPropertyChangedCallback(IsCheckedChanged));

                TextBlock nameText = new TextBlock();
                nameText.Text = radioItem.Text;
                ItemNames.Children.Add(nameText);

                TextBlock stateText = new TextBlock();
                AutomationProperties.SetName(stateText, radioItem.Text + "State");
                UpdateTextState(radioItem, stateText);
                ItemStates.Children.Add(stateText);

                itemStates.Add(radioItem.Text, stateText);
            }
        }

        private void IsCheckedChanged(DependencyObject o, DependencyProperty p)
        {
            RadioMenuFlyoutItem radioItem = o as RadioMenuFlyoutItem;
            TextBlock stateText;
            if (itemStates.TryGetValue(radioItem.Text, out stateText))
            {
                UpdateTextState(radioItem, stateText);
            }
        }

        private void UpdateTextState(RadioMenuFlyoutItem item, TextBlock textBlock)
        {
            textBlock.Text = item.IsChecked ? "Checked" : "Unchecked";
        }
    }
}
