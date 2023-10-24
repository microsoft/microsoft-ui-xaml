// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Windows.Foundation.Metadata;
using Windows.UI;
using Microsoft.UI.Composition;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Automation;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Private.Media;
using MUXControlsTestApp.Utilities;

#if !BUILD_WINDOWS
#endif

namespace MUXControlsTestApp
{
    public sealed partial class RevealFallbackPage : TestPage
    {
        RevealTestApi _revealTestApi;
        RevealBrushTestApi _revealBrushTestApi;
        RevealBackgroundBrush _resourceButtonRevealBackgroundBrush;
        RevealBackgroundBrush _resourceButtonRevealBackgroundPointerOver;
        RevealBackgroundBrush _resourceButtonRevealBackgroundPressed;
        RevealBorderBrush _resourceButtonRevealBorderBrush;
        RevealBorderBrush _resourceButtonRevealBorderBrushPointerOver;
        RevealBorderBrush _resourceButtonRevealBorderBrushPressed;

        public RevealFallbackPage()
        {
            this.InitializeComponent();

            AutomationProperties.SetName(this, "RevealFallbackPage");
            AutomationProperties.SetAutomationId(this, "RevealFallbackPage");

            _revealTestApi = new RevealTestApi();
            _revealBrushTestApi = new RevealBrushTestApi();
        }

        private void RunTestButton_Clicked(object sender, RoutedEventArgs e)
        {
            using (var setup = new MaterialSetupHelper())
            {
                // Initialize local references to Page.Resources overrides for the RevealBrushes we'll use
                _resourceButtonRevealBackgroundBrush = Resources["ButtonRevealBackground"] as RevealBackgroundBrush;
                _resourceButtonRevealBackgroundPointerOver = Resources["ButtonRevealBackgroundPointerOver"] as RevealBackgroundBrush;
                _resourceButtonRevealBackgroundPressed = Resources["ButtonRevealBackgroundPressed"] as RevealBackgroundBrush;
                _resourceButtonRevealBorderBrush = Resources["ButtonRevealBorderBrush"] as RevealBorderBrush;
                _resourceButtonRevealBorderBrushPointerOver = Resources["ButtonRevealBorderBrushPointerOver"] as RevealBorderBrush;
                _resourceButtonRevealBorderBrushPressed = Resources["ButtonRevealBorderBrushPressed"] as RevealBorderBrush;

                // Apply reveal styles
                RevealButton.Style = Application.Current.Resources["ButtonRevealStyle"] as Style;
                RevealRepeatButton.Style = Application.Current.Resources["RepeatButtonRevealStyle"] as Style;
                RevealToggleButton.Style = Application.Current.Resources["ToggleButtonRevealStyle"] as Style;
                LargeButton.Style = Application.Current.Resources["ButtonRevealStyle"] as Style;
                NarrowButton.Style = Application.Current.Resources["ButtonRevealStyle"] as Style;

                switch (TestNameComboBox.GetSelectedText())
                {
                    case "RevealAlwaysUseFallback":
                        {
                            RunRevealAlwaysUseFallback();
                            break;
                        }
                }
            }
        }

        // Toggle ForceFallback property and validate brush + light state of two easily accessible (Default VSM state) brushes
        private void RunRevealAlwaysUseFallback()
        {
            _revealBrushTestApi.RevealBrush = LargeButton.Background as RevealBrush;
            bool isInFallbackMode = _revealBrushTestApi.IsInFallbackMode;

            if (isInFallbackMode)
            {
                TestResult.Text = "RevealAlwaysUseFallback: Skipped";
            }
            else
            {
                // 1. Force fallback + validate
                AlwaysUseFallbackHelper(true);
                RevealBrush largeButtonBackgroundBrush = LargeButton.Background as RevealBrush;
                RevealBrush largeButtonBorderBrush = LargeButton.BorderBrush as RevealBrush;

                // Validate brushes
                bool fallbackBackgroundBrushResult = VerifyFallbackColor(largeButtonBackgroundBrush, Microsoft.UI.Colors.Orange);
                bool fallbackBorderBrushResult = VerifyFallbackColor(largeButtonBorderBrush, Microsoft.UI.Colors.Transparent);

                // Validate lights
                bool fallbackBackroundLightResult = VerifyAmbientLight(largeButtonBackgroundBrush, true) &&
                                                     VerifyBorderLight(largeButtonBackgroundBrush, false) &&
                                                     VerifyHoverLight(largeButtonBackgroundBrush, false);

                bool fallbackBorderLightResult = VerifyAmbientLight(largeButtonBorderBrush, true) &&
                                                     VerifyBorderLight(largeButtonBorderBrush, false) &&
                                                     VerifyHoverLight(largeButtonBorderBrush, false);

                // 2. Unforce fallback + validate
                AlwaysUseFallbackHelper(false);

                // Validate brushes
                bool backgroundBrushResult = VerifyRevealBrush(largeButtonBackgroundBrush);
                bool borderBrushResult = VerifyRevealBrush(largeButtonBorderBrush);

                // Validate lights
                bool backroundLightResult = VerifyAmbientLight(largeButtonBackgroundBrush, true) &&
                                                     VerifyBorderLight(largeButtonBackgroundBrush, false) &&
                                                     VerifyHoverLight(largeButtonBackgroundBrush, true);

                bool borderLightResult = VerifyAmbientLight(largeButtonBorderBrush, true) &&
                                                     VerifyBorderLight(largeButtonBorderBrush, true) &&
                                                     VerifyHoverLight(largeButtonBorderBrush, false);

                bool brushesResult = fallbackBackgroundBrushResult && fallbackBorderBrushResult && backgroundBrushResult && borderBrushResult;
                bool lightsResult = fallbackBackroundLightResult && fallbackBorderLightResult && backroundLightResult && borderLightResult;
                bool result = brushesResult && lightsResult;

                string brushesResultString = System.String.Format("({0},{1},{2},{3})", fallbackBackgroundBrushResult, fallbackBorderBrushResult, backgroundBrushResult, borderBrushResult);
                string lightsResultString = System.String.Format("({0},{1},{2},{3})", fallbackBackroundLightResult, fallbackBorderLightResult, backroundLightResult, borderLightResult);

                TestResult.Text = result ? "" : "Errors: brushes=" + brushesResultString + ", lights=" + lightsResultString;
            }
        }

        private void AlwaysUseFallbackButton_Checked(object sender, RoutedEventArgs e)
        {
            AlwaysUseFallbackHelper(true);
        }

        private void AlwaysUseFallbackButton_Unchecked(object sender, RoutedEventArgs e)
        {
            AlwaysUseFallbackHelper(false);
        }

        void AlwaysUseFallbackHelper(bool value)
        {
            _resourceButtonRevealBackgroundBrush.AlwaysUseFallback = value;
            _resourceButtonRevealBackgroundPointerOver.AlwaysUseFallback = value;
            _resourceButtonRevealBackgroundPressed.AlwaysUseFallback = value;
            _resourceButtonRevealBorderBrush.AlwaysUseFallback = value;
            _resourceButtonRevealBorderBrushPointerOver.AlwaysUseFallback = value;
            _resourceButtonRevealBorderBrushPressed.AlwaysUseFallback = value;
        }

        bool VerifyRevealBrush(RevealBrush testBrush)
        {
            _revealBrushTestApi.RevealBrush = testBrush;
            bool isInFallbackMode = _revealBrushTestApi.IsInFallbackMode;
            CompositionBrush compositionBrush = _revealBrushTestApi.CompositionBrush;

            return !isInFallbackMode &&
                   (compositionBrush != null && compositionBrush is CompositionEffectBrush);
        }

        bool VerifyFallbackColor(RevealBrush testBrush, Color fallbackColor)
        {
            _revealBrushTestApi.RevealBrush = testBrush;
            bool isInFallbackMode = _revealBrushTestApi.IsInFallbackMode;
            CompositionBrush compositionBrush = _revealBrushTestApi.CompositionBrush;

            // In fallbackmode, comp brush is set to null if color is transparent
            // it's used to boost performance for LVIP.
            if (isInFallbackMode && fallbackColor.Equals(Microsoft.UI.Colors.Transparent))
            {
                return compositionBrush == null;
            }

            return isInFallbackMode &&
                   (compositionBrush != null && compositionBrush is CompositionColorBrush) &&
                   (compositionBrush as CompositionColorBrush).Color.Equals(fallbackColor);
        }

        bool VerifyAmbientLight(RevealBrush testBrush, bool value)
        {
            _revealBrushTestApi.RevealBrush = testBrush;
            bool ambientLightSet = _revealBrushTestApi.IsAmbientLightSet;
            return value ? ambientLightSet : !ambientLightSet;
        }

        bool VerifyBorderLight(RevealBrush testBrush, bool value)
        {
            _revealBrushTestApi.RevealBrush = testBrush;
            bool borderLightSet = _revealBrushTestApi.IsBorderLightSet;
            return value ? borderLightSet : !borderLightSet;
        }

        bool VerifyHoverLight(RevealBrush testBrush, bool value)
        {
            _revealBrushTestApi.RevealBrush = testBrush;
            bool hoverLightSet = _revealBrushTestApi.IsHoverLightSet;
            return value ? hoverLightSet : !hoverLightSet;
        }
    }
}
