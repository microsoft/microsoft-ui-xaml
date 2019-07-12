// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
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
#endif

namespace MUXControlsTestApp
{
    [TopLevelTestPage(Name = "SwipeControl", Icon = "Swipe.png")]
    public sealed partial class SwipePage : TestPage
    {
        public SwipePage()
        {
#if !BUILD_WINDOWS
            LogController.InitializeLogging();
#endif

            this.InitializeComponent();

            navigateToSimpleContents.Click += delegate { Frame.NavigateWithoutAnimation(typeof(SwipeControlPage), 0); };
            navigateToDynamic.Click += delegate { Frame.NavigateWithoutAnimation(typeof(SwipeControlPage2), 0); };
        }

        private void CmbSwipeControlOutputDebugStringLevel_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            MUXControlsTestHooks.SetOutputDebugStringLevelForType(
                "SwipeControl",
                cmbSwipeControlOutputDebugStringLevel.SelectedIndex == 1 || cmbSwipeControlOutputDebugStringLevel.SelectedIndex == 2,
                cmbSwipeControlOutputDebugStringLevel.SelectedIndex == 2);
        }
    }
}
