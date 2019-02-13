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
    }
}
