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
        }

        private void CmbItemsViewOutputDebugStringLevel_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (chkItemsView != null && chkItemsView.IsChecked == true)
            {
                MUXControlsTestHooks.SetOutputDebugStringLevelForType(
                    "ItemsView",
                    cmbItemsViewOutputDebugStringLevel.SelectedIndex == 1 || cmbItemsViewOutputDebugStringLevel.SelectedIndex == 2,
                    cmbItemsViewOutputDebugStringLevel.SelectedIndex == 2);
            }

            if (chkScrollView != null && chkScrollView.IsChecked == true)
            {
                MUXControlsTestHooks.SetOutputDebugStringLevelForType(
                    "ScrollView",
                    cmbItemsViewOutputDebugStringLevel.SelectedIndex == 1 || cmbItemsViewOutputDebugStringLevel.SelectedIndex == 2,
                    cmbItemsViewOutputDebugStringLevel.SelectedIndex == 2);
            }

            if (chkScrollPresenter != null && chkScrollPresenter.IsChecked == true)
            {
                MUXControlsTestHooks.SetOutputDebugStringLevelForType(
                    "ScrollPresenter",
                    cmbItemsViewOutputDebugStringLevel.SelectedIndex == 1 || cmbItemsViewOutputDebugStringLevel.SelectedIndex == 2,
                    cmbItemsViewOutputDebugStringLevel.SelectedIndex == 2);
            }

            if (chkItemContainer != null && chkItemContainer.IsChecked == true)
            {
                MUXControlsTestHooks.SetOutputDebugStringLevelForType(
                    "ItemContainer",
                    cmbItemsViewOutputDebugStringLevel.SelectedIndex == 1 || cmbItemsViewOutputDebugStringLevel.SelectedIndex == 2,
                    cmbItemsViewOutputDebugStringLevel.SelectedIndex == 2);
            }

            if (chkItemsRepeater != null && chkItemsRepeater.IsChecked == true)
            {
                MUXControlsTestHooks.SetOutputDebugStringLevelForType(
                    "ItemsRepeater",
                    cmbItemsViewOutputDebugStringLevel.SelectedIndex == 1 || cmbItemsViewOutputDebugStringLevel.SelectedIndex == 2,
                    cmbItemsViewOutputDebugStringLevel.SelectedIndex == 2);
            }

            if (chkLinedFlowLayout != null && chkLinedFlowLayout.IsChecked == true)
            {
                MUXControlsTestHooks.SetOutputDebugStringLevelForType(
                    "LinedFlowLayout",
                    cmbItemsViewOutputDebugStringLevel.SelectedIndex == 1 || cmbItemsViewOutputDebugStringLevel.SelectedIndex == 2,
                    cmbItemsViewOutputDebugStringLevel.SelectedIndex == 2);
            }

            if (chkAnnotatedScrollBar != null && chkAnnotatedScrollBar.IsChecked == true)
            {
                MUXControlsTestHooks.SetOutputDebugStringLevelForType(
                    "AnnotatedScrollBar",
                    cmbItemsViewOutputDebugStringLevel.SelectedIndex == 1 || cmbItemsViewOutputDebugStringLevel.SelectedIndex == 2,
                    cmbItemsViewOutputDebugStringLevel.SelectedIndex == 2);
            }
        }
    }
}
