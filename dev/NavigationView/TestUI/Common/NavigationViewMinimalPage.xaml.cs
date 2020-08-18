// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using Windows.UI.Xaml;

namespace MUXControlsTestApp
{
    public sealed partial class NavigationViewMinimalPage : TestPage
    {
        public NavigationViewMinimalPage()
        {
            this.InitializeComponent();
        }

        private void GetNavViewActiveVisualStates_Click(object sender, RoutedEventArgs e)
        {
            var visualstates = Utilities.VisualStateHelper.GetCurrentVisualStateName(NavView);
            NavViewActiveVisualStatesResult.Text = string.Join(",", visualstates);
        }
    }
}
