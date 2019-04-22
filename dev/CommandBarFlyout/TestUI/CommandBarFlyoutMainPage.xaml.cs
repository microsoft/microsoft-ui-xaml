// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;

using MUXControlsTestHooks = Microsoft.UI.Private.Controls.MUXControlsTestHooks;

namespace MUXControlsTestApp
{
    public sealed partial class CommandBarFlyoutMainPage : TestPage
    {
        public CommandBarFlyoutMainPage()
        {
            this.InitializeComponent();
        }

        public void OnCommandBarFlyoutTestsClicked(object sender, object args)
        {
            var rootFrame = Window.Current.Content as Frame;
            rootFrame.NavigateWithoutAnimation(typeof(CommandBarFlyoutPage), "CommandBarFlyout Tests");
        }

        public void OnTextCommandBarFlyoutTestsClicked(object sender, object args)
        {
            var rootFrame = Window.Current.Content as Frame;
            rootFrame.NavigateWithoutAnimation(typeof(TextCommandBarFlyoutPage), "TextCommandBarFlyout Tests");
        }

        public void OnExtraCommandBarFlyoutTestsClicked(object sender, object args)
        {
            var rootFrame = Window.Current.Content as Frame;
            rootFrame.NavigateWithoutAnimation(typeof(ExtraCommandBarFlyoutPage), "Extra CommandBarFlyout Tests");
        }
        private void CmbCommandBarFlyoutOutputDebugStringLevel_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            MUXControlsTestHooks.SetOutputDebugStringLevelForType(
                "CommandBarFlyout",
                cmbCommandBarFlyoutOutputDebugStringLevel.SelectedIndex == 1 || cmbCommandBarFlyoutOutputDebugStringLevel.SelectedIndex == 2,
                cmbCommandBarFlyoutOutputDebugStringLevel.SelectedIndex == 2);
        }
    }
}
