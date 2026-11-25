// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml;

namespace MUXControlsTestApp
{
    [TopLevelTestPage(Name = "TabView Focus")]
    public sealed partial class TabViewFocusPage : TestPage
    {
        public TabViewFocusPage()
        {
            this.InitializeComponent();
        }

        private void SelectThirdTabButton_Click(object sender, RoutedEventArgs e)
        {
            Tabs.SelectedIndex = 2;
        }
    }
}
