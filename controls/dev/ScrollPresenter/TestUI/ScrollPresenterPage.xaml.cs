// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Common;
using System;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Private.Controls;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

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
    }
}
