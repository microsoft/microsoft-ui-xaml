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
    [TopLevelTestPage(Name="ItemContainer", Icon="ItemContainer.png")]
    public sealed partial class ItemContainerPage : TestPage
    {
        public ItemContainerPage()
        {
            LogController.InitializeLogging();
            this.InitializeComponent();

            navigateToSummary.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ItemContainerSummaryPage), 0); };
            navigateToLayout.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ItemContainerLayoutPage), 0); };
        }

        private void CmbItemContainerOutputDebugStringLevel_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            MUXControlsTestHooks.SetOutputDebugStringLevelForType(
                "ItemContainer",
                cmbItemContainerOutputDebugStringLevel.SelectedIndex == 1 || cmbItemContainerOutputDebugStringLevel.SelectedIndex == 2,
                cmbItemContainerOutputDebugStringLevel.SelectedIndex == 2);
        }
    }
}

