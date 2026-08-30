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
    [TopLevelTestPage(Name = "ItemsView", Icon = "ItemsView.png")]
    public sealed partial class ItemsViewPage : TestPage
    {
        public ItemsViewPage()
        {
            LogController.InitializeLogging();
            this.InitializeComponent();

            navigateToSummary.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ItemsViewSummaryPage), 0); };
            navigateToInteractiveTests.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ItemsViewInteractiveTestsPage), 0); };
            navigateToIntegration.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ItemsViewIntegrationPage), 0); };
            navigateToBlank.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ItemsViewBlankPage), 0); };
            navigateToTransitionProvider.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ItemsViewTransitionPage), 0); };
            navigateToPictureLibrary.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ItemsViewPictureLibraryPage), 0); };

            ApplyDebugLoggingOptions();
        }

        private void CmbItemsViewOutputDebugStringLevel_SelectionChanged(object sender, SelectionChangedEventArgs e)
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
            int selectedLevel = cmbItemsViewOutputDebugStringLevel?.SelectedIndex ?? 0;
            bool isLoggingInfoLevel = selectedLevel == 1 || selectedLevel == 2;
            bool isLoggingVerboseLevel = selectedLevel == 2;

            SetOutputDebugStringLevelForType("ItemsView", chkItemsView, isLoggingInfoLevel, isLoggingVerboseLevel);
            SetOutputDebugStringLevelForType("ScrollView", chkScrollView, isLoggingInfoLevel, isLoggingVerboseLevel);
            SetOutputDebugStringLevelForType("ScrollPresenter", chkScrollPresenter, isLoggingInfoLevel, isLoggingVerboseLevel);
            SetOutputDebugStringLevelForType("ItemContainer", chkItemContainer, isLoggingInfoLevel, isLoggingVerboseLevel);
            SetOutputDebugStringLevelForType("ItemsRepeater", chkItemsRepeater, isLoggingInfoLevel, isLoggingVerboseLevel);
            SetOutputDebugStringLevelForType("LinedFlowLayout", chkLinedFlowLayout, isLoggingInfoLevel, isLoggingVerboseLevel);
            SetOutputDebugStringLevelForType("AnnotatedScrollBar", chkAnnotatedScrollBar, isLoggingInfoLevel, isLoggingVerboseLevel);
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
