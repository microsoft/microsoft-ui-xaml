// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Private.Controls;
using Common;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

namespace MUXControlsTestApp
{
    [TopLevelTestPage(Name = "SwipeControl", Icon = "Swipe.png")]
    public sealed partial class SwipePage : TestPage
    {
        public SwipePage()
        {
            LogController.InitializeLogging();

            this.InitializeComponent();

            navigateToSimpleContents.Click += delegate { Frame.NavigateWithoutAnimation(typeof(SwipeControlPage), 0); };
            navigateToDynamic.Click += delegate { Frame.NavigateWithoutAnimation(typeof(SwipeControlPage2), 0); };
            navigateToClear.Click += delegate { Frame.NavigateWithoutAnimation(typeof(SwipeControlClearPage), 0); };
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
