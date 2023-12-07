// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Common;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Private.Controls;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

namespace MUXControlsTestApp
{
    [TopLevelTestPage(Name="AnnotatedScrollBar")]
    public sealed partial class AnnotatedScrollBarPage : TestPage
    {
        public AnnotatedScrollBarPage()
        {
            LogController.InitializeLogging();
            this.InitializeComponent();

            navigateToSummary.Click += delegate { Frame.NavigateWithoutAnimation(typeof(AnnotatedScrollBarSummaryPage), 0); };
            navigateToIScrollController.Click += delegate { Frame.NavigateWithoutAnimation(typeof(AnnotatedScrollBarIScrollControllerPage), 0); };
            navigateToRangeBase.Click += delegate { Frame.NavigateWithoutAnimation(typeof(AnnotatedScrollBarRangeBasePage), 0); };
        }

        private void CmbAnnotatedScrollBarOutputDebugStringLevel_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
           MUXControlsTestHooks.SetOutputDebugStringLevelForType(
               "AnnotatedScrollBar",
               cmbAnnotatedScrollBarOutputDebugStringLevel.SelectedIndex == 1 || cmbAnnotatedScrollBarOutputDebugStringLevel.SelectedIndex == 2,
               cmbAnnotatedScrollBarOutputDebugStringLevel.SelectedIndex == 2);
        }
    }
}

