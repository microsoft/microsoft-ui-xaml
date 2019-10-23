// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Common;
using System;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;

#if USING_TAEF
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;
#else
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.VisualStudio.TestTools.UnitTesting.Logging;
#endif

using MUXControlsTestHooks = Microsoft.UI.Private.Controls.MUXControlsTestHooks;
using ScrollingPresenterTestHooks = Microsoft.UI.Private.Controls.ScrollingPresenterTestHooks;

namespace MUXControlsTestApp
{
    [TopLevelTestPage(Name = "ScrollingPresenter", Icon = "ScrollingPresenter.png")]
    public sealed partial class ScrollingPresenterPage : TestPage
    {
        public ScrollingPresenterPage()
        {
            LogController.InitializeLogging();

            this.InitializeComponent();

            navigateToSimpleContents.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ScrollingPresentersWithSimpleContentsPage), 0); };
            navigateToDynamic.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ScrollingPresenterDynamicPage), 0); };
            navigateToExpressionAnimationSources.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ScrollingPresenterExpressionAnimationSourcesPage), 0); };
            navigateToChainingAndRailing.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ScrollingPresenterChainingAndRailingPage), 0); };
            navigateToStackPanelAnchoring.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ScrollingPresenterStackPanelAnchoringPage), 0); };
            navigateToRepeaterAnchoring.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ScrollingPresenterRepeaterAnchoringPage), 0); };
            navigateToSnapPoints.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ScrollingPresenterSnapPointsPage), 0); };
            navigateToBringIntoView.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ScrollingPresenterBringIntoViewPage), 0); };
            navigateToManipulationMode.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ScrollingPresenterManipulationModePage), 0); };
            navigateToAccessibility.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ScrollingPresenterAccessibilityPage), 0); };
            navigateToSimpleScrollControllers.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ScrollingPresenterWithSimpleScrollControllersPage), 0); };
            navigateToCompositionScrollControllers.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ScrollingPresenterWithCompositionScrollControllersPage), 0); };
            navigateToBiDirectionalScrollController.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ScrollingPresenterWithBiDirectionalScrollControllerPage), 0); };
            navigateToLeakDetection.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ScrollingPresenterLeakDetectionPage), 0); };
            navigateToMousePanning.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ScrollingPresenterMousePanningPage), 0); };

            try
            {
                int millisecondsPerUnit;
                int minMilliseconds;
                int maxMilliseconds;

                ScrollingPresenterTestHooks.GetOffsetsChangeVelocityParameters(
                    out millisecondsPerUnit,
                    out minMilliseconds,
                    out maxMilliseconds);

                txtOffsetsChangeMillisecondsPerUnit.Text = millisecondsPerUnit.ToString();
                txtOffsetsChangeMinMilliseconds.Text = minMilliseconds.ToString();
                txtOffsetsChangeMaxMilliseconds.Text = maxMilliseconds.ToString();

                ScrollingPresenterTestHooks.GetZoomFactorChangeVelocityParameters(
                    out millisecondsPerUnit,
                    out minMilliseconds,
                    out maxMilliseconds);

                txtZoomFactorChangeMillisecondsPerUnit.Text = millisecondsPerUnit.ToString();
                txtZoomFactorChangeMinMilliseconds.Text = minMilliseconds.ToString();
                txtZoomFactorChangeMaxMilliseconds.Text = maxMilliseconds.ToString();

                switch (ScrollingPresenterTestHooks.MouseWheelDeltaForVelocityUnit)
                {
                    case 15:
                        cmbMouseWheelDeltaForVelocityUnit.SelectedIndex = 0;
                        break;
                    case 30:
                        cmbMouseWheelDeltaForVelocityUnit.SelectedIndex = 1;
                        break;
                    case 60:
                        cmbMouseWheelDeltaForVelocityUnit.SelectedIndex = 2;
                        break;
                    case 90:
                        cmbMouseWheelDeltaForVelocityUnit.SelectedIndex = 3;
                        break;
                    case 120:
                        cmbMouseWheelDeltaForVelocityUnit.SelectedIndex = 4;
                        break;
                }

                txtMouseWheelInertiaDecayRate.Text = ScrollingPresenterTestHooks.MouseWheelInertiaDecayRate.ToString();
                txtMouseWheelScrollLines.Text = ScrollingPresenterTestHooks.MouseWheelScrollLines.ToString();
                txtMouseWheelScrollChars.Text = ScrollingPresenterTestHooks.MouseWheelScrollChars.ToString();

                chkIsInteractionTrackerPointerWheelRedirectionEnabled.IsChecked = ScrollingPresenterTestHooks.IsInteractionTrackerPointerWheelRedirectionEnabled;
            }
            catch (Exception ex)
            {
                tbException.Text = ex.ToString();
            }
        }

        private void CmbScrollingPresenterOutputDebugStringLevel_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            MUXControlsTestHooks.SetOutputDebugStringLevelForType(
                "ScrollingPresenter",
                cmbScrollingPresenterOutputDebugStringLevel.SelectedIndex == 1 || cmbScrollingPresenterOutputDebugStringLevel.SelectedIndex == 2,
                cmbScrollingPresenterOutputDebugStringLevel.SelectedIndex == 2);
        }

        private void CmbMouseWheelDeltaForVelocityUnit_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            ScrollingPresenterTestHooks.MouseWheelDeltaForVelocityUnit = Convert.ToInt32((cmbMouseWheelDeltaForVelocityUnit.SelectedItem as ComboBoxItem).Content);
        }

        private void TxtMouseWheelDeltaForVelocityUnit_TextChanged(object sender, TextChangedEventArgs e)
        {
            try
            {
                ScrollingPresenterTestHooks.MouseWheelInertiaDecayRate = Convert.ToSingle(txtMouseWheelInertiaDecayRate.Text);
            }
            catch (Exception ex)
            {
                tbException.Text = ex.ToString();
            }
        }

        private void TxtMouseWheelScrollLines_TextChanged(object sender, TextChangedEventArgs e)
        {
            try
            {
                ScrollingPresenterTestHooks.MouseWheelScrollLines = Convert.ToInt32(txtMouseWheelScrollLines.Text);
            }
            catch (Exception ex)
            {
                tbException.Text = ex.ToString();
            }
        }

        private void TxtMouseWheelScrollChars_TextChanged(object sender, TextChangedEventArgs e)
        {
            try
            {
                ScrollingPresenterTestHooks.MouseWheelScrollChars = Convert.ToInt32(txtMouseWheelScrollChars.Text);
            }
            catch (Exception ex)
            {
                tbException.Text = ex.ToString();
            }
        }

        private void BtnSetOffsetsChangeVelocityParameters_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                ScrollingPresenterTestHooks.SetOffsetsChangeVelocityParameters(
                    Convert.ToInt32(txtOffsetsChangeMillisecondsPerUnit.Text),
                    Convert.ToInt32(txtOffsetsChangeMinMilliseconds.Text),
                    Convert.ToInt32(txtOffsetsChangeMaxMilliseconds.Text));
            }
            catch (Exception ex)
            {
                tbException.Text = ex.ToString();
            }
        }

        private void BtnSetZoomFactorChangeVelocityParameters_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                ScrollingPresenterTestHooks.SetZoomFactorChangeVelocityParameters(
                    Convert.ToInt32(txtZoomFactorChangeMillisecondsPerUnit.Text),
                    Convert.ToInt32(txtZoomFactorChangeMinMilliseconds.Text),
                    Convert.ToInt32(txtZoomFactorChangeMaxMilliseconds.Text));
            }
            catch (Exception ex)
            {
                tbException.Text = ex.ToString();
            }
        }

        private void ChkIsInteractionTrackerPointerWheelRedirectionEnabled_Checked(object sender, RoutedEventArgs e)
        {
            ScrollingPresenterTestHooks.IsInteractionTrackerPointerWheelRedirectionEnabled = true;
        }

        private void ChkIsInteractionTrackerPointerWheelRedirectionEnabled_Unchecked(object sender, RoutedEventArgs e)
        {
            ScrollingPresenterTestHooks.IsInteractionTrackerPointerWheelRedirectionEnabled = false;
        }
    }
}