// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml.Controls;

namespace MUXControlsTestApp
{
    [TopLevelTestPage(Name="CommandBar")]
    public sealed partial class CommandBarCenterPage : TestPage
    {
        public CommandBarCenterPage()
        {
            this.InitializeComponent();

            navigateToCommandBarSummary.Click += delegate { Frame.NavigateWithoutAnimation(typeof(CommandBarSummaryPage)); };
            navigateToCommandBar.Click += delegate { Frame.NavigateWithoutAnimation(typeof(CommandBarPage)); };
        }
    }
}
