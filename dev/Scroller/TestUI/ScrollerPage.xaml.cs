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
using ScrollerTestHooks = Microsoft.UI.Private.Controls.ScrollerTestHooks;

namespace MUXControlsTestApp
{
    [TopLevelTestPage(Name = "Scroller", Icon = "ScrollViewer.png")]
    public sealed partial class ScrollerPage : TestPage
    {
        public ScrollerPage()
        {
            // Some pages we will navigate to will need the resources, so lets load them now!
            App.AppendResourceDictionaryToMergedDictionaries(App.AdditionStylesXaml);
            
            LogController.InitializeLogging();

            this.InitializeComponent();

            navigateToSimpleContents.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ScrollersWithSimpleContentsPage), 0); };
            navigateToDynamic.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ScrollerDynamicPage), 0); };
            navigateToExpressionAnimationSources.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ScrollerExpressionAnimationSourcesPage), 0); };
            navigateToChainingAndRailing.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ScrollerChainingAndRailingPage), 0); };
            navigateToStackPanelAnchoring.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ScrollerStackPanelAnchoringPage), 0); };
            navigateToRepeaterAnchoring.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ScrollerRepeaterAnchoringPage), 0); };
            navigateToScrollSnapPoints.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ScrollerScrollSnapPointsPage), 0); };
            navigateToZoomSnapPoints.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ScrollerZoomSnapPointsPage), 0); };
            navigateToBringIntoView.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ScrollerBringIntoViewPage), 0); };
            navigateToManipulationMode.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ScrollerManipulationModePage), 0); };
            navigateToAccessibility.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ScrollerAccessibilityPage), 0); };
            navigateToSimpleScrollControllers.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ScrollerWithSimpleScrollControllersPage), 0); };
            navigateToCompositionScrollControllers.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ScrollerWithCompositionScrollControllersPage), 0); };
            navigateToBiDirectionalScrollController.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ScrollerWithBiDirectionalScrollControllerPage), 0); };
            navigateToLeakDetection.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ScrollerLeakDetectionPage), 0); };
            navigateToMousePanning.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ScrollerMousePanningPage), 0); };

            try
            {
                int millisecondsPerUnit;
                int minMilliseconds;
                int maxMilliseconds;

                ScrollerTestHooks.GetOffsetsChangeVelocityParameters(
                    out millisecondsPerUnit,
                    out minMilliseconds,
                    out maxMilliseconds);

                txtOffsetsChangeMillisecondsPerUnit.Text = millisecondsPerUnit.ToString();
                txtOffsetsChangeMinMilliseconds.Text = minMilliseconds.ToString();
                txtOffsetsChangeMaxMilliseconds.Text = maxMilliseconds.ToString();

                ScrollerTestHooks.GetZoomFactorChangeVelocityParameters(
                    out millisecondsPerUnit,
                    out minMilliseconds,
                    out maxMilliseconds);

                txtZoomFactorChangeMillisecondsPerUnit.Text = millisecondsPerUnit.ToString();
                txtZoomFactorChangeMinMilliseconds.Text = minMilliseconds.ToString();
                txtZoomFactorChangeMaxMilliseconds.Text = maxMilliseconds.ToString();

                switch (ScrollerTestHooks.MouseWheelDeltaForVelocityUnit)
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

                txtMouseWheelInertiaDecayRate.Text = ScrollerTestHooks.MouseWheelInertiaDecayRate.ToString();
                txtMouseWheelScrollLines.Text = ScrollerTestHooks.MouseWheelScrollLines.ToString();
                txtMouseWheelScrollChars.Text = ScrollerTestHooks.MouseWheelScrollChars.ToString();

                chkIsInteractionTrackerPointerWheelRedirectionEnabled.IsChecked = ScrollerTestHooks.IsInteractionTrackerPointerWheelRedirectionEnabled;
            }
            catch (Exception ex)
            {
                tbException.Text = ex.ToString();
            }
        }

        private void CmbScrollerOutputDebugStringLevel_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            MUXControlsTestHooks.SetOutputDebugStringLevelForType(
                "Scroller",
                cmbScrollerOutputDebugStringLevel.SelectedIndex == 1 || cmbScrollerOutputDebugStringLevel.SelectedIndex == 2,
                cmbScrollerOutputDebugStringLevel.SelectedIndex == 2);
        }

        private void CmbMouseWheelDeltaForVelocityUnit_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            ScrollerTestHooks.MouseWheelDeltaForVelocityUnit = Convert.ToInt32((cmbMouseWheelDeltaForVelocityUnit.SelectedItem as ComboBoxItem).Content);
        }

        private void TxtMouseWheelDeltaForVelocityUnit_TextChanged(object sender, TextChangedEventArgs e)
        {
            try
            {
                ScrollerTestHooks.MouseWheelInertiaDecayRate = Convert.ToSingle(txtMouseWheelInertiaDecayRate.Text);
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
                ScrollerTestHooks.MouseWheelScrollLines = Convert.ToInt32(txtMouseWheelScrollLines.Text);
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
                ScrollerTestHooks.MouseWheelScrollChars = Convert.ToInt32(txtMouseWheelScrollChars.Text);
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
                ScrollerTestHooks.SetOffsetsChangeVelocityParameters(
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
                ScrollerTestHooks.SetZoomFactorChangeVelocityParameters(
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
            ScrollerTestHooks.IsInteractionTrackerPointerWheelRedirectionEnabled = true;
        }

        private void ChkIsInteractionTrackerPointerWheelRedirectionEnabled_Unchecked(object sender, RoutedEventArgs e)
        {
            ScrollerTestHooks.IsInteractionTrackerPointerWheelRedirectionEnabled = false;
        }
    }
}