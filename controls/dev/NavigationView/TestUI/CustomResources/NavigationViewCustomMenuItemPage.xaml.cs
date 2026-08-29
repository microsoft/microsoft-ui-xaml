// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Automation;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Navigation;

namespace MUXControlsTestApp
{
    public sealed partial class NavigationViewCustomMenuItemPage : TestPage
    {
        public NavigationViewCustomMenuItemPage()
        {
            this.InitializeComponent();
            NavView.SelectedItem = nvi1;
        }
    }
}
