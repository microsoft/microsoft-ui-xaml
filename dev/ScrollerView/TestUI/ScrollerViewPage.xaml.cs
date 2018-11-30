// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

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
#endif

namespace MUXControlsTestApp
{
    public sealed partial class ScrollerViewPage : TestPage
    {
        public ScrollerViewPage()
        {
#if !BUILD_WINDOWS
            LogController.InitializeLogging();
#endif

            this.InitializeComponent();

            navigateToSimpleContents.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ScrollerViewsWithSimpleContentsPage), 0); };
            navigateToDynamic.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ScrollerViewDynamicPage), 0); };
            navigateToScrollControllers.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ScrollerViewWithScrollControllersPage), 0); };
        }

        private void CmbScrollerViewOutputDebugStringLevel_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            MUXControlsTestHooks.SetOutputDebugStringLevelForType(
                "ScrollerView",
                cmbScrollerViewOutputDebugStringLevel.SelectedIndex == 1 || cmbScrollerViewOutputDebugStringLevel.SelectedIndex == 2,
                cmbScrollerViewOutputDebugStringLevel.SelectedIndex == 2);

            MUXControlsTestHooks.SetOutputDebugStringLevelForType(
                "ScrollBar2",
                cmbScrollerViewOutputDebugStringLevel.SelectedIndex == 1 || cmbScrollerViewOutputDebugStringLevel.SelectedIndex == 2,
                cmbScrollerViewOutputDebugStringLevel.SelectedIndex == 2);

            MUXControlsTestHooks.SetOutputDebugStringLevelForType(
                "Scroller",
                cmbScrollerViewOutputDebugStringLevel.SelectedIndex == 1 || cmbScrollerViewOutputDebugStringLevel.SelectedIndex == 2,
                cmbScrollerViewOutputDebugStringLevel.SelectedIndex == 2);
        }
    }
}
