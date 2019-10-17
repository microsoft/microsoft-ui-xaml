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
using ScrollingPresenterTestHooks = Microsoft.UI.Private.Controls.ScrollingPresenterTestHooks;

namespace MUXControlsTestApp
{
    [TopLevelTestPage(Name = "ScrollingView", Icon = "ScrollingView.png")]
    public sealed partial class ScrollingViewPage : TestPage
    {
        public ScrollingViewPage()
        {
            LogController.InitializeLogging();
            this.InitializeComponent();

            navigateToSimpleContents.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ScrollingViewsWithSimpleContentsPage), 0); };
            navigateToDynamic.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ScrollingViewDynamicPage), 0); };
            navigateToScrollControllers.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ScrollingViewWithScrollControllersPage), 0); };
            navigateToRTL.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ScrollingViewWithRTLFlowDirectionPage), 0); };
            navigateToKeyboardAndGamepadNavigation.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ScrollingViewKeyboardAndGamepadNavigationPage), 0); };
            navigateToBlank.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ScrollingViewBlankPage), 0); };

            chkIsInteractionTrackerPointerWheelRedirectionEnabled.IsChecked = ScrollingPresenterTestHooks.IsInteractionTrackerPointerWheelRedirectionEnabled;
        }

        private void CmbScrollingViewOutputDebugStringLevel_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            MUXControlsTestHooks.SetOutputDebugStringLevelForType(
                "ScrollingView",
                cmbScrollingViewOutputDebugStringLevel.SelectedIndex == 1 || cmbScrollingViewOutputDebugStringLevel.SelectedIndex == 2,
                cmbScrollingViewOutputDebugStringLevel.SelectedIndex == 2);

            MUXControlsTestHooks.SetOutputDebugStringLevelForType(
                "ScrollingPresenter",
                cmbScrollingViewOutputDebugStringLevel.SelectedIndex == 1 || cmbScrollingViewOutputDebugStringLevel.SelectedIndex == 2,
                cmbScrollingViewOutputDebugStringLevel.SelectedIndex == 2);
        }

        private void ChkIsInteractionTrackerPointerWheelRedirectionEnabled_Checked(object sender, RoutedEventArgs e)
        {
            ScrollingPresenterTestHooks.IsInteractionTrackerPointerWheelRedirectionEnabled = true;
        }

        private void ChkIsInteractionTrackerPointerWheelRedirectionEnabled_Unchecked(object sender, RoutedEventArgs e)
        {
            ScrollingPresenterTestHooks.IsInteractionTrackerPointerWheelRedirectionEnabled = false;
        }
    }
}
