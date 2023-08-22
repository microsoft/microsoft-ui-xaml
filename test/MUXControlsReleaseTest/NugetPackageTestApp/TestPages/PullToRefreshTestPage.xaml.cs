// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using MUXControlsTestApp;

namespace NugetPackageTestApp
{
    public sealed partial class PullToRefreshTestPage : TestPage
    {
        public PullToRefreshTestPage()
        {
            this.InitializeComponent();
        }

        private void RequestRefresh_Clicked(object sender, Windows.UI.Xaml.RoutedEventArgs e)
        {
            refreshContainer.RequestRefresh();
        }
    }
}
