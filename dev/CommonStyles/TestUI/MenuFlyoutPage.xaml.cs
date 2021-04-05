// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Windows.Foundation.Metadata;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Input;
using SplitButtonTestApi = Microsoft.UI.Private.Controls.SplitButtonTestApi;
namespace MUXControlsTestApp
{

    [TopLevelTestPage(Name = "MenuFlyout", Icon = "MenuFlyout.png")]
    public sealed partial class MenuFlyoutPage : TestPage
    {
        public MenuFlyoutPage()
        {
            this.InitializeComponent();
        }

        private void TestMenuFlyoutItemClick(object sender, object e)
        {
            TestMenuFlyoutItemHeightTextBlock.Text = $"{TestMenuFlyoutItem.ActualHeight}";
            TestMenuFlyoutItemWidthTextBlock.Text = $"{TestMenuFlyoutItem.ActualWidth}";
        }
    }
}
