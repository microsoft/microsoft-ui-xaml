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
using ScrollPresenterTestHooks = Microsoft.UI.Private.Controls.ScrollPresenterTestHooks;

namespace MUXControlsTestApp
{
    [TopLevelTestPage(Name = "ScrollPresenter", Icon = "ScrollPresenter.png")]
    public sealed partial class ScrollPresenterPage : TestPage
    {
        public ScrollPresenterPage()
        {
            // Some pages we will navigate to will need the resources, so lets load them now!
            App.AppendResourceDictionaryToMergedDictionaries(App.AdditionStylesXaml);
            
            LogController.InitializeLogging();

            this.InitializeComponent();

            navigateToSimpleContents.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ScrollPresentersWithSimpleContentsPage), 0); };
            navigateToDynamic.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ScrollPresenterDynamicPage), 0); };
            navigateToExpressionAnimationSources.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ScrollPresenterExpressionAnimationSourcesPage), 0); };
            navigateToChainingAndRailing.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ScrollPresenterChainingAndRailingPage), 0); };
            navigateToStackPanelAnchoring.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ScrollPresenterStackPanelAnchoringPage), 0); };
            navigateToRepeaterAnchoring.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ScrollPresenterRepeaterAnchoringPage), 0); };
            navigateToScrollSnapPoints.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ScrollPresenterScrollSnapPointsPage), 0); };
            navigateToZoomSnapPoints.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ScrollPresenterZoomSnapPointsPage), 0); };
            navigateToBringIntoView.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ScrollPresenterBringIntoViewPage), 0); };
            navigateToManipulationMode.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ScrollPresenterManipulationModePage), 0); };
            navigateToAccessibility.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ScrollPresenterAccessibilityPage), 0); };
            navigateToSimpleScrollControllers.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ScrollPresenterWithSimpleScrollControllersPage), 0); };
            navigateToCompositionScrollControllers.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ScrollPresenterWithCompositionScrollControllersPage), 0); };
            navigateToBiDirectionalScrollController.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ScrollPresenterWithBiDirectionalScrollControllerPage), 0); };
            navigateToLeakDetection.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ScrollPresenterLeakDetectionPage), 0); };
            navigateToMousePanning.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ScrollPresenterMousePanningPage), 0); };
            navigateToEdgeScrolling.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ScrollPresenterEdgeScrollingPage), 0); };

            try
            {
                int millisecondsPerUnit;
                int minMilliseconds;
                int maxMilliseconds;

                ScrollPresenterTestHooks.GetOffsetsChangeVelocityParameters(
                    out millisecondsPerUnit,
                    out minMilliseconds,
                    out maxMilliseconds);

                txtOffsetsChangeMillisecondsPerUnit.Text = millisecondsPerUnit.ToString();
                txtOffsetsChangeMinMilliseconds.Text = minMilliseconds.ToString();
                txtOffsetsChangeMaxMilliseconds.Text = maxMilliseconds.ToString();

                ScrollPresenterTestHooks.GetZoomFactorChangeVelocityParameters(
                    out millisecondsPerUnit,
                    out minMilliseconds,
                    out maxMilliseconds);

                txtZoomFactorChangeMillisecondsPerUnit.Text = millisecondsPerUnit.ToString();
                txtZoomFactorChangeMinMilliseconds.Text = minMilliseconds.ToString();
                txtZoomFactorChangeMaxMilliseconds.Text = maxMilliseconds.ToString();

                switch (ScrollPresenterTestHooks.MouseWheelDeltaForVelocityUnit)
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

                txtMouseWheelInertiaDecayRate.Text = ScrollPresenterTestHooks.MouseWheelInertiaDecayRate.ToString();
                txtMouseWheelScrollLines.Text = ScrollPresenterTestHooks.MouseWheelScrollLines.ToString();
                txtMouseWheelScrollChars.Text = ScrollPresenterTestHooks.MouseWheelScrollChars.ToString();

                chkIsInteractionTrackerPointerWheelRedirectionEnabled.IsChecked = ScrollPresenterTestHooks.IsInteractionTrackerPointerWheelRedirectionEnabled;
            }
            catch (Exception ex)
            {
                tbException.Text = ex.ToString();
            }
        }

        private void CmbScrollPresenterOutputDebugStringLevel_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            MUXControlsTestHooks.SetOutputDebugStringLevelForType(
                "ScrollPresenter",
                cmbScrollPresenterOutputDebugStringLevel.SelectedIndex == 1 || cmbScrollPresenterOutputDebugStringLevel.SelectedIndex == 2,
                cmbScrollPresenterOutputDebugStringLevel.SelectedIndex == 2);
        }

        private void CmbMouseWheelDeltaForVelocityUnit_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            ScrollPresenterTestHooks.MouseWheelDeltaForVelocityUnit = Convert.ToInt32((cmbMouseWheelDeltaForVelocityUnit.SelectedItem as ComboBoxItem).Content);
        }

        private void TxtMouseWheelDeltaForVelocityUnit_TextChanged(object sender, TextChangedEventArgs e)
        {
            try
            {
                ScrollPresenterTestHooks.MouseWheelInertiaDecayRate = Convert.ToSingle(txtMouseWheelInertiaDecayRate.Text);
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
                ScrollPresenterTestHooks.MouseWheelScrollLines = Convert.ToInt32(txtMouseWheelScrollLines.Text);
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
                ScrollPresenterTestHooks.MouseWheelScrollChars = Convert.ToInt32(txtMouseWheelScrollChars.Text);
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
                ScrollPresenterTestHooks.SetOffsetsChangeVelocityParameters(
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
                ScrollPresenterTestHooks.SetZoomFactorChangeVelocityParameters(
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
            ScrollPresenterTestHooks.IsInteractionTrackerPointerWheelRedirectionEnabled = true;
        }

        private void ChkIsInteractionTrackerPointerWheelRedirectionEnabled_Unchecked(object sender, RoutedEventArgs e)
        {
            ScrollPresenterTestHooks.IsInteractionTrackerPointerWheelRedirectionEnabled = false;
        }
    }
}