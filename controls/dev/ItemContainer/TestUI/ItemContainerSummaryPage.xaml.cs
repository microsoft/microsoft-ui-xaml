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
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;

namespace MUXControlsTestApp
{
    public sealed partial class ItemContainerSummaryPage : TestPage
    {
        private double itemInvoked = 0;

        public ItemContainerSummaryPage()
        {
            this.InitializeComponent();
            this.Loaded += ItemContainerSummaryPage_Loaded;

#if MUX_PRERELEASE
            RoutedEventsItemContainer.ItemInvoked += ItemContainer_OnItemInvoked;
            ItemContainerWithButtons.ItemInvoked += ItemContainer_OnItemInvoked;
#endif
        }

        private void ItemContainerSummaryPage_Loaded(object sender, RoutedEventArgs e)
        {
            // Workaround for xaml parsing error crash: Bug 41701000
#if MUX_PRERELEASE
            ItemContainerWithButtons.MultiSelectMode = ItemContainerMultiSelectMode.Multiple;
#endif
        }

#if MUX_PRERELEASE
        private void ItemContainer_OnItemInvoked(object sender, ItemContainerInvokedEventArgs e)
        {
            
            var itemContainer = sender as ItemContainer;

            if (itemContainer.IsSelected == true)
            {
                itemContainer.IsSelected = false;
            }
            else
            {
                itemContainer.IsSelected = true;
            }

            itemInvoked++;

            ItemInvokedEventTextBlock.Text = itemInvoked.ToString();
        }
#endif

        private void BtnGetIsSelected_Click(object sender, RoutedEventArgs e)
        {
            IsSelectedTextBlock.Text = RoutedEventsItemContainer.IsSelected.ToString();
        }


        private void IsMultiSelectComboBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
#if MUX_PRERELEASE
            string multiSelectName = e.AddedItems[0].ToString();
            ItemContainerMultiSelectMode multiSelectMode;

            switch (multiSelectName)
            {
                case "Auto":
                    multiSelectMode = ItemContainerMultiSelectMode.Auto;
                    break;
                case "Single":
                    multiSelectMode = ItemContainerMultiSelectMode.Single;
                    break;
                case "Extended":
                    multiSelectMode = ItemContainerMultiSelectMode.Extended;
                    break;
                case "Multiple":
                    multiSelectMode = ItemContainerMultiSelectMode.Multiple;
                    break;
                default:
                    throw new Exception($"Invalid argument: {multiSelectName}");
            }

            RoutedEventsItemContainer.MultiSelectMode = multiSelectMode;
#endif
        }

        private void IsSelectedComboBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            string isSelectedName = e.AddedItems[0].ToString();
            bool isSelected;

            switch (isSelectedName)
            {
                case "True":
                    isSelected = true;
                    break;
                case "False":
                    isSelected = false;
                    break;
                default:
                    throw new Exception($"Invalid argument: {isSelectedName}");
            }

            RoutedEventsItemContainer.IsSelected = isSelected;
        }

        private void IsEnabledComboBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            string isEnabledName = e.AddedItems[0].ToString();
            bool isEnabled;

            switch (isEnabledName)
            {
                case "True":
                    isEnabled = true;
                    break;
                case "False":
                    isEnabled = false;
                    break;
                default:
                    throw new Exception($"Invalid argument: {isEnabledName}");
            }

            RoutedEventsItemContainer.IsEnabled = isEnabled;
        }

        private void HeartButton_Click(object sender, RoutedEventArgs e)
        {
            //HeartToggleButtonHandled = true;
        }

        private void CanUserSelectComboBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
#if MUX_PRERELEASE
            ComboBoxItem selectedItem = CanUserSelectComboBox.SelectedItem as ComboBoxItem;
            string CanUserSelectName = selectedItem.Content.ToString();
            ItemContainerUserSelectMode canUserSelect;

            switch (CanUserSelectName)
            {
                case "Auto":
                    canUserSelect = ItemContainerUserSelectMode.Auto;
                    break;
                case "UserCanSelect":
                    canUserSelect = ItemContainerUserSelectMode.UserCanSelect;
                    break;
                case "UserCannotSelect":
                    canUserSelect = ItemContainerUserSelectMode.UserCannotSelect;
                    break;
                default:
                    throw new Exception($"Invalid argument: {CanUserSelectName}");
            }

            RoutedEventsItemContainer.CanUserSelect = canUserSelect;
#endif
        }

        private void CanUserInvokeComboBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
#if MUX_PRERELEASE
            string CanUserInvokeName = e.AddedItems[0].ToString();
            ItemContainerUserInvokeMode canUserInvoke;

            switch (CanUserInvokeName)
            {
                case "Auto":
                    canUserInvoke = ItemContainerUserInvokeMode.Auto;
                    break;
                case "UserCanInvoke":
                    canUserInvoke = ItemContainerUserInvokeMode.UserCanInvoke;
                    break;
                case "UserCannotInvoke":
                    canUserInvoke = ItemContainerUserInvokeMode.UserCannotInvoke;
                    break;
                default:
                    throw new Exception($"Invalid argument: {CanUserInvokeName}");
            }

            RoutedEventsItemContainer.CanUserInvoke = canUserInvoke;
#endif
        }
    }
}
