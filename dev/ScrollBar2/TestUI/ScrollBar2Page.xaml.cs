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
    public sealed partial class ScrollBar2Page : TestPage
    {
        public ScrollBar2Page()
        {
#if !BUILD_WINDOWS
            LogController.InitializeLogging();
#endif

            this.InitializeComponent();

            navigateToDynamic.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ScrollBar2DynamicPage), 0); };
            navigateToLeakDetection.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ScrollBar2LeakDetectionPage), 0); };
        }

        private void CmbScrollBar2OutputDebugStringLevel_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            MUXControlsTestHooks.SetOutputDebugStringLevelForType(
                "ScrollBar2",
                cmbScrollBar2OutputDebugStringLevel.SelectedIndex == 1 || cmbScrollBar2OutputDebugStringLevel.SelectedIndex == 2,
                cmbScrollBar2OutputDebugStringLevel.SelectedIndex == 2);
        }
    }
}
