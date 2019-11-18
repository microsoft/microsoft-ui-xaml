// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Common;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;

#if USING_TAEF
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;
#else
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.VisualStudio.TestTools.UnitTesting.Logging;
#endif

using MUXControlsTestHooks = Microsoft.UI.Private.Controls.MUXControlsTestHooks;
using ScrollerTestHooks = Microsoft.UI.Private.Controls.ScrollerTestHooks;

namespace MUXControlsTestApp
{
    [TopLevelTestPage(Name = "ScrollViewer", Icon = "ScrollViewer.png")]
    public sealed partial class ScrollViewerPage : TestPage
    {
        public ScrollViewerPage()
        {
            LogController.InitializeLogging();
            this.InitializeComponent();

            navigateToSimpleContents.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ScrollViewersWithSimpleContentsPage), 0); };
            navigateToDynamic.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ScrollViewerDynamicPage), 0); };
            navigateToScrollControllers.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ScrollViewerWithScrollControllersPage), 0); };
            navigateToRTL.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ScrollViewerWithRTLFlowDirectionPage), 0); };
            navigateToKeyboardAndGamepadNavigation.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ScrollViewerKeyboardAndGamepadNavigationPage), 0); };
            navigateToBlank.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ScrollViewerBlankPage), 0); };

            chkIsInteractionTrackerPointerWheelRedirectionEnabled.IsChecked = ScrollerTestHooks.IsInteractionTrackerPointerWheelRedirectionEnabled;
        }

        private void CmbScrollViewerOutputDebugStringLevel_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            MUXControlsTestHooks.SetOutputDebugStringLevelForType(
                "ScrollViewer",
                cmbScrollViewerOutputDebugStringLevel.SelectedIndex == 1 || cmbScrollViewerOutputDebugStringLevel.SelectedIndex == 2,
                cmbScrollViewerOutputDebugStringLevel.SelectedIndex == 2);

            MUXControlsTestHooks.SetOutputDebugStringLevelForType(
                "Scroller",
                cmbScrollViewerOutputDebugStringLevel.SelectedIndex == 1 || cmbScrollViewerOutputDebugStringLevel.SelectedIndex == 2,
                cmbScrollViewerOutputDebugStringLevel.SelectedIndex == 2);
        }

        private void ChkIsInteractionTrackerPointerWheelRedirectionEnabled_Checked(object sender, RoutedEventArgs e)
        {
            ScrollerTestHooks.IsInteractionTrackerPointerWheelRedirectionEnabled = true;
        }

        private void ChkIsInteractionTrackerPointerWheelRedirectionEnabled_Unchecked(object sender, RoutedEventArgs e)
        {
            ScrollerTestHooks.IsInteractionTrackerPointerWheelRedirectionEnabled = false;
        }
    }
}
