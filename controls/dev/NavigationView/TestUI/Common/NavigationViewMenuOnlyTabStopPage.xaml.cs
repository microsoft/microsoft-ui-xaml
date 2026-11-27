// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml;

namespace MUXControlsTestApp
{
    public sealed partial class NavigationViewMenuOnlyTabStopPage : TestPage
    {
        public NavigationViewMenuOnlyTabStopPage()
        {
            this.InitializeComponent();

            // Hide the test frame bar so the menu items are the only tab stops in the window.
            UpdateTestFrameVisibility(Visibility.Collapsed);
        }
    }
}