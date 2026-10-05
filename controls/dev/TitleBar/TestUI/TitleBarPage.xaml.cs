// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Private.Controls;
using WEX.Logging.Interop;

namespace MUXControlsTestApp
{
    [TopLevelTestPage(Name = "TitleBar", Icon = "DefaultIcon.png")]
    public sealed partial class TitleBarPage : TestPage
    {
        private string originalMainWindowTitle;

        public TitleBarPage()
        {
            LogController.InitializeLogging();

            // The in-page TitleBar is hosted in the main test window, so it renames that window to its Title.
            // The test infrastructure finds the main window by its original title, so restore it when leaving the page.
            originalMainWindowTitle = App.CurrentWindow.AppWindow.Title;
            this.Unloaded += (sender, args) => App.CurrentWindow.AppWindow.Title = originalMainWindowTitle;

            this.InitializeComponent();
        }

        private void CmbTitleBarOutputDebugStringLevel_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            MUXControlsTestHooks.SetOutputDebugStringLevelForType(
                "TitleBar",
                cmbTitleBarOutputDebugStringLevel.SelectedIndex == 1 || cmbTitleBarOutputDebugStringLevel.SelectedIndex == 2,
                cmbTitleBarOutputDebugStringLevel.SelectedIndex == 2);
        }

        private void TitleBarWindowingButton_Click(object sender, RoutedEventArgs e)
        {
            var newWindow = new TitleBarPageWindow();
            newWindow.Activate();
        }
    }
}
