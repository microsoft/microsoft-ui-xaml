// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Common;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Private.Controls;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

namespace MUXControlsTestApp
{
    [TopLevelTestPage(Name = "SelectorBar")]
    public sealed partial class SelectorBarPage : TestPage
    {
        public SelectorBarPage()
        {
            LogController.InitializeLogging();
            this.InitializeComponent();

            navigateToSummary.Click += delegate { Frame.NavigateWithoutAnimation(typeof(SelectorBarSummaryPage), 0); };
            navigateToSample.Click += delegate { Frame.NavigateWithoutAnimation(typeof(SelectorBarSamplePage), 0); };

            ApplyDebugLoggingOptions();
        }

        private void CmbSelectorBarOutputDebugStringLevel_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            ApplyDebugLoggingOptions();
        }

        private void ChkOutputDebugStringLevel_CheckedChanged(object sender, RoutedEventArgs e)
        {
            ApplyDebugLoggingOptions();
        }

        // Applies the OutputDebugString logging level selected in the ComboBox to every checked type, and
        // turns logging off for unchecked types. Reading the current ComboBox selection together with all
        // CheckBox states makes the configuration order-independent: the user can toggle the checkboxes and
        // pick the ComboBox value in any order and still get the requested logging.
        private void ApplyDebugLoggingOptions()
        {
            int selectedLevel = cmbSelectorBarOutputDebugStringLevel?.SelectedIndex ?? 0;
            bool isLoggingInfoLevel = selectedLevel == 1 || selectedLevel == 2;
            bool isLoggingVerboseLevel = selectedLevel == 2;

            SetOutputDebugStringLevelForType("SelectorBar", chkSelectorBar, isLoggingInfoLevel, isLoggingVerboseLevel);
            SetOutputDebugStringLevelForType("ItemsView", chkItemsView, isLoggingInfoLevel, isLoggingVerboseLevel);
            SetOutputDebugStringLevelForType("ItemContainer", chkItemContainer, isLoggingInfoLevel, isLoggingVerboseLevel);
            SetOutputDebugStringLevelForType("ItemsRepeater", chkItemsRepeater, isLoggingInfoLevel, isLoggingVerboseLevel);
        }

        private static void SetOutputDebugStringLevelForType(string type, CheckBox checkBox, bool isLoggingInfoLevel, bool isLoggingVerboseLevel)
        {
            bool isChecked = checkBox != null && checkBox.IsChecked == true;

            MUXControlsTestHooks.SetOutputDebugStringLevelForType(
                type,
                isChecked && isLoggingInfoLevel,
                isChecked && isLoggingVerboseLevel);
        }
    }
}
