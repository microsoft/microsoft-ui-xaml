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
    public sealed partial class ItemContainerLayoutPage : TestPage
    {
        public ItemContainerLayoutPage()
        {
            this.InitializeComponent();
            this.Loaded += ItemContainerLayoutPage_Loaded;
        }

        private void ItemContainerLayoutPage_Loaded(object sender, RoutedEventArgs e)
        {
            // Workaround for xaml parsing error crash: Bug 41701000
            itemContainer3.MultiSelectMode = ItemContainerMultiSelectMode.Multiple;
            itemContainer4.MultiSelectMode = ItemContainerMultiSelectMode.Multiple;
            itemContainer13.MultiSelectMode = ItemContainerMultiSelectMode.Multiple;
            itemContainer14.MultiSelectMode = ItemContainerMultiSelectMode.Multiple;
            itemContainer23.MultiSelectMode = ItemContainerMultiSelectMode.Multiple;
            itemContainer24.MultiSelectMode = ItemContainerMultiSelectMode.Multiple;
        }
    }
}
