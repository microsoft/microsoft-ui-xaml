// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml.Controls;

namespace MUXControlsTestApp
{
    public sealed partial class NavigationViewPointerFocusPage : TestPage
    {
        public NavigationViewPointerFocusPage()
        {
            InitializeComponent();
            TopMode.Checked += delegate { NavView.PaneDisplayMode = NavigationViewPaneDisplayMode.Top; };
            TopMode.Unchecked += delegate { NavView.PaneDisplayMode = NavigationViewPaneDisplayMode.Left; };
        }
    }
}
