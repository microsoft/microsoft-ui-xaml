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
using ScrollPresenterTestHooks = Microsoft.UI.Private.Controls.ScrollPresenterTestHooks;

namespace MUXControlsTestApp
{
    [TopLevelTestPage(Name = "ScrollView", Icon = "ScrollView.png")]
    public sealed partial class ScrollViewPage : TestPage
    {
        public ScrollViewPage()
        {
            LogController.InitializeLogging();
            this.InitializeComponent();

            navigateToSimpleContents.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ScrollViewsWithSimpleContentsPage), 0); };
            navigateToDynamic.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ScrollViewDynamicPage), 0); };
            navigateToScrollControllers.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ScrollViewWithScrollControllersPage), 0); };
            navigateToRTL.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ScrollViewWithRTLFlowDirectionPage), 0); };
            navigateToKeyboardAndGamepadNavigation.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ScrollViewKeyboardAndGamepadNavigationPage), 0); };
            navigateToBlank.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ScrollViewBlankPage), 0); };

            chkIsInteractionTrackerPointerWheelRedirectionEnabled.IsChecked = ScrollPresenterTestHooks.IsInteractionTrackerPointerWheelRedirectionEnabled;
        }

        private void CmbScrollViewOutputDebugStringLevel_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            MUXControlsTestHooks.SetOutputDebugStringLevelForType(
                "ScrollView",
                cmbScrollViewOutputDebugStringLevel.SelectedIndex == 1 || cmbScrollViewOutputDebugStringLevel.SelectedIndex == 2,
                cmbScrollViewOutputDebugStringLevel.SelectedIndex == 2);

            MUXControlsTestHooks.SetOutputDebugStringLevelForType(
                "ScrollPresenter",
                cmbScrollViewOutputDebugStringLevel.SelectedIndex == 1 || cmbScrollViewOutputDebugStringLevel.SelectedIndex == 2,
                cmbScrollViewOutputDebugStringLevel.SelectedIndex == 2);
        }

        private void ChkIsInteractionTrackerPointerWheelRedirectionEnabled_Checked(object sender, RoutedEventArgs e)
        {
            ScrollPresenterTestHooks.IsInteractionTrackerPointerWheelRedirectionEnabled = true;
        }

        private void ChkIsInteractionTrackerPointerWheelRedirectionEnabled_Unchecked(object sender, RoutedEventArgs e)
        {
            ScrollPresenterTestHooks.IsInteractionTrackerPointerWheelRedirectionEnabled = false;
        }
    }
}
