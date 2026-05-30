// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;

namespace MUXControlsTestApp
{
    public sealed partial class CustomNavigationViewItem : NavigationViewItem
    {
        public string Text { get => TextBlock_CustomNavigationViewItem.Text; set => TextBlock_CustomNavigationViewItem.Text = value; }
        public CustomNavigationViewItem()
        {
            this.InitializeComponent();
        }
    }
}
