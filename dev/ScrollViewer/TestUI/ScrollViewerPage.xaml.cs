// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Common;

#if USING_TAEF
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;
#else
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.VisualStudio.TestTools.UnitTesting.Logging;
#endif

#if !BUILD_WINDOWS
using MUXControlsTestHooks = Microsoft.UI.Private.Controls.MUXControlsTestHooks;
using ScrollerTestHooks = Microsoft.UI.Private.Controls.ScrollerTestHooks;
using ScrollViewerTestHooks = Microsoft.UI.Private.Controls.ScrollViewerTestHooks;
#endif

namespace MUXControlsTestApp
{
    public sealed partial class ScrollViewerPage : TestPage
    {
        public ScrollViewerPage()
        {
#if !BUILD_WINDOWS
            LogController.InitializeLogging();
#endif

            this.InitializeComponent();

            navigateToSimpleContents.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ScrollViewersWithSimpleContentsPage), 0); };
            navigateToDynamic.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ScrollViewerDynamicPage), 0); };
            navigateToScrollControllers.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ScrollViewerWithScrollControllersPage), 0); };
            navigateToRTL.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ScrollViewerWithRTLFlowDirectionPage), 0); };

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

        private void ChkAutoHideScrollControllers_Indeterminate(object sender, RoutedEventArgs e)
        {
            ScrollViewerTestHooks.AutoHideScrollControllers = null;
        }

        private void ChkAutoHideScrollControllers_Checked(object sender, RoutedEventArgs e)
        {
            ScrollViewerTestHooks.AutoHideScrollControllers = true;
        }

        private void ChkAutoHideScrollControllers_Unchecked(object sender, RoutedEventArgs e)
        {
            ScrollViewerTestHooks.AutoHideScrollControllers = false;
        }
    }
}
