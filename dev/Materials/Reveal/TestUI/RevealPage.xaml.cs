// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Windows.UI.Xaml.Controls;

namespace MUXControlsTestApp
{
    public sealed partial class RevealPage : TestPage
    {
        public RevealPage()
        {
            this.InitializeComponent();

            navigateToBasicReveal.Click += delegate { Frame.NavigateWithoutAnimation(typeof(RevealBasicPage), 0); };
            navigateToAdvReveal.Click += delegate { Frame.NavigateWithoutAnimation(typeof(RevealListPage), 0); };
            navigateToColorReveal.Click += delegate { Frame.NavigateWithoutAnimation(typeof(RevealColorPage), 0); };
            navigateToSimpleListReveal.Click += delegate { Frame.NavigateWithoutAnimation(typeof(RevealSimpleListPage), 0); };
            navigateToRevealScenarios.Click += delegate { Frame.NavigateWithoutAnimation(typeof(RevealScenarios), 0); };
            navigateToRevealFallback.Click += delegate { Frame.NavigateWithoutAnimation(typeof(RevealFallbackPage), 0); };
            navigateToRevealStates.Click += delegate { Frame.NavigateWithoutAnimation(typeof(RevealStatesPage), 0); };
            navigateToRevealRegressionTests.Click += delegate { Frame.NavigateWithoutAnimation(typeof(RevealRegressionTestsPage), 0); };
            navigateToCoreWindowEventsTests.Click += delegate { Frame.NavigateWithoutAnimation(typeof(CoreWindowEventsPage), 0); };
#if FeatureNavigationViewEnabled
            navigateToRevealMarkup.Click += delegate { Frame.NavigateWithoutAnimation(typeof(RevealMarkupPage), 0); };
#endif
        }
    }
}
