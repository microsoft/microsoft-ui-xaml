// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Linq;
using System.Collections.Generic;
using System.Threading;
using System.Threading.Tasks;
using Windows.Foundation.Metadata;
using Windows.UI.Composition;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Automation;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Media.Animation;
using Windows.UI.Xaml.Navigation;
using MUXControlsTestApp.Utilities;
using Common;

using RevealBrushState = Microsoft.UI.Xaml.Media.RevealBrushState;
using RevealBrush = Microsoft.UI.Xaml.Media.RevealBrush;
using MaterialHelperTestApi = Microsoft.UI.Private.Media.MaterialHelperTestApi;
using RevealTestApi = Microsoft.UI.Private.Media.RevealTestApi;
using RevealBrushTestApi = Microsoft.UI.Private.Media.RevealBrushTestApi;
using RevealBorderLight = Microsoft.UI.Private.Media.RevealBorderLight;
using RevealHoverLight = Microsoft.UI.Private.Media.RevealHoverLight;

namespace MUXControlsTestApp
{
    public sealed partial class RevealStatesPage : TestPage
    {
        RevealTestApi _revealTestApi;
        RevealBrushTestApi _revealBrushTestApi;
        int _normalStateCheckNumber = 1;
        CompositionPropertyLogger _compositionPropertyLogger;
        SpotLight _hoverSpotlight;
        SpotLight _pressSpotlight;

        // Expressions used to animate RevealHoverLight's Spotlight.Offset for pointer and keyboard input, respectively
#if DEBUG
        string revealHoverLightOffset_PointerExpression = "pointer.Position + Vector3(0, 0, props.SpotlightHeight)";
        string revealHoverLightOffset_KeyboardExpression = "Vector3((visual.Size.X / 2), (visual.Size.Y / 2), props.SpotlightHeight)";
#else
        string revealHoverLightOffset_PointerExpression = "pointer.Position + Vector3(0, 0, 256)";
        string revealHoverLightOffset_KeyboardExpression = "Vector3((visual.Size.X / 2), (visual.Size.Y / 2), 256)";
#endif

        const float coneIntensityTolerance = 0.1F;
        const float coneAngleTolerance = 0.1F;
        const float angleInRadiansTolerance = 0.1F;
        const float offsetCoordinateTolerance = 3.0F;

        const float c_hoverLight_OuterConeAngle_Expected = 0.469878F;         // Does not change for any state.

        public RevealStatesPage()
        {
            this.InitializeComponent();

            if (ApiInformation.IsTypePresent("Windows.UI.Xaml.Media.XamlCompositionBrushBase"))
            {
                AutomationProperties.SetName(this, "RevealStatesPage");
                AutomationProperties.SetAutomationId(this, "RevealStatesPage");

                _revealTestApi = new RevealTestApi();
                _revealBrushTestApi = new RevealBrushTestApi();

                _compositionPropertyLogger = new CompositionPropertyLogger();

                MaterialHelperTestApi.IgnoreAreEffectsFast = true;
                MaterialHelperTestApi.SimulateDisabledByPolicy = false;
            }
        }

        protected override void OnNavigatedFrom(NavigationEventArgs e)
        {
            // Unset all override flags to avoid impacting subsequent tests
            MaterialHelperTestApi.IgnoreAreEffectsFast = false;
            MaterialHelperTestApi.SimulateDisabledByPolicy = false;
            base.OnNavigatedFrom(e);
        }

        private void ValidationActionsComboBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            var selectedText = ValidationActionsComboBox.GetSelectedText();
            switch (selectedText)
            {
                case "HoverLight: Validate Normal state": HoverLight_ValidateNormalState(); break;
                case "HoverLight: Validate PointerOver state": HoverLight_ValidatePointerOverState(); break;
                case "BorderLight: Validate after panning away": BorderLight_ValidateBorderLightPanAway(); break;
            }
        }

        private void StartLoggingValues_Click(object sender, RoutedEventArgs e)
        {
            var selectedText = ValidationActionsComboBox.GetSelectedText();
            switch (selectedText)
            {
                case "HoverLight: Validate PointerOver state (Values)": HoverLight_ValidatePointerOverState_Values(); break;
                case "HoverLight: Validate Pressed state (Values)": HoverLight_ValidatePressedState_Values(); break;
                case "HoverLight: Validate SlowRelease state (Values)": HoverLight_ValidateSlowRelease_Values(); break;
                case "HoverLight: Validate FastRelease state (Values)": HoverLight_ValidateFastRelease_Values(); break;
                case "HoverLight: Validate Position Offset1 (Values)": HoverLight_ValidatePosition_Offset1_Values(); break;
                case "HoverLight: Validate Position Offset2 (Values)": HoverLight_ValidatePosition_Offset2_Values(); break;
                case "HoverLight: Validate Position Offset3 (Values)": HoverLight_ValidatePosition_Offset3_Values(); break;
            }
        }

        private UIElement GetElementForHoverLight(object element)
        {
            if (Common.PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone2))
            {
                var target = element as Control;
                return VisualTreeHelper.GetChild(target, 0) as UIElement;
            }
            return element as UIElement;
        }

        private void HoverLight_ValidateNormalState()
        {
            if (!ValidateEffectsPresent()) { return; }

            using (var logger = new ResultsLogger("Normal", TestResult))
            {
                if (_normalStateCheckNumber < 1 || _normalStateCheckNumber > 2)
                {
                    logger.LogError("Error: unpexected call to HoverLight_ValidateNormalState");
                    return;
                }

                bool isInitialCheck = _normalStateCheckNumber == 1;
                _normalStateCheckNumber++;

                string targetText = TargetComboBox.GetSelectedText();
                Control target = FindName(targetText) as Control;

                // It is more complex to validate Reveal hover bursh targeting on ListViewItem 
                // since it uses several brushes applied to the internal ListViewItemChrome. Skip the check here.
                bool lightTargetingResult = true;
                if (targetText != "NormalListViewItem")
                {
                    RevealBrush revealHoverBrush = target.Background as RevealBrush;
                    lightTargetingResult = VerifyHoverBrushTargeting(revealHoverBrush);
                }
                logger.Verify(lightTargetingResult, "lightTargetingResult:" + lightTargetingResult);
                
                var lights = GetElementForHoverLight(target).Lights;

                // No lights expected if we've never hovered over this Button
                bool lightCountResult = isInitialCheck ? lights.Count == 0 : lights.Count == 2;
                logger.Verify(lightCountResult, "lightCountResult:" + lightCountResult);
                bool lightValidationResult = true;

                // If we have hovered over the target, validate Reveal light is properly in normal state
                if (!isInitialCheck)
                {
                    var hoverLight = _revealTestApi.GetAsRevealHoverLight(lights[0]);
                    lightValidationResult = HoverLight_ValidateNormal(hoverLight);
                }
                logger.Verify(lightValidationResult, "lightValidationResult: " + lightValidationResult);
            }
        }

        private void HoverLight_ValidatePointerOverState()
        {
            if (!ValidateEffectsPresent()) { return; }

            using (var logger = new ResultsLogger("PointerOver", TestResult))
            {
                string targetText = TargetComboBox.GetSelectedText();
                Control target = FindName(targetText) as Control;

                // It is more complex to validate Reveal hover brush targeting on ListViewItem 
                // since it uses several brushes applied to the internal ListViewItemChrome. Skip the check here.
                bool lightTargetingResult = true;
                if (targetText != "NormalListViewItem")
                {
                    RevealBrush revealHoverBrush = target.Background as RevealBrush;
                    lightTargetingResult = VerifyHoverBrushTargeting(revealHoverBrush);
                }

                logger.Verify(lightTargetingResult, "lightTargetingResult:" + lightTargetingResult);

                var lights = GetElementForHoverLight(target).Lights;

                bool lightCountResult = lights.Count == 2;          // Reveal Press light is also present
                logger.Verify(lightCountResult, "lightCountResult:" + lightCountResult);

                var hoverLight = _revealTestApi.GetAsRevealHoverLight(lights[0]);
                logger.Verify(hoverLight != null, "HoverLight should be non-null");

                bool lightValidationResult = HoverLight_ValidatePointerOver(hoverLight);
                logger.Verify(lightValidationResult, "lightValidationResult: " + lightValidationResult);
            }
        }

        private void LargeButton_Click(object sender, RoutedEventArgs e)
        {
            ValidateLightsOnPressed();
        }

        private void TestListView_ItemClick(object sender, Windows.UI.Xaml.Controls.ItemClickEventArgs e)
        {
            ValidateLightsOnPressed();
        }

        private void ValidateLightsOnPressed()
        {
            if (!ValidateEffectsPresent()) { return; }

            string targetText = TargetComboBox.GetSelectedText();
            if (targetText == "OneMoreListViewItem") { return; }            // We don't run this test for OneMoreListViewItem

            using (var logger = new ResultsLogger("Pressed", TestResult))
            {
                Control target = FindName(targetText) as Control;

                // It is more complex to validate Reveal hover brush targeting on ListViewItem 
                // since it uses several brushes applied to the internal ListViewItemChrome. Skip the check here.
                bool lightTargetingResult = true;
                if (targetText != "NormalListViewItem")
                {
                    RevealBrush revealHoverBrush = target.Background as RevealBrush;
                    lightTargetingResult = VerifyHoverBrushTargeting(revealHoverBrush);
                }
                logger.Verify(lightTargetingResult, "lightTargetingResult:" + lightTargetingResult);

                var lights = GetElementForHoverLight(target).Lights;

                bool lightCountResult = lights.Count == 2;   // Reveal Press light is also present
                logger.Verify(lightCountResult, "lightCountResult:" + lightCountResult);
                var hoverLight = _revealTestApi.GetAsRevealHoverLight(lights[0]);
                bool lightValidationResult = HoverLight_ValidatePressed(hoverLight);
                logger.Verify(lightValidationResult, "lightValidationResult: " + lightValidationResult);
            }
        }

        bool ValidateEffectsPresent()
        {
            _revealBrushTestApi.RevealBrush = LargeButton.Background as RevealBrush;
            if (_revealBrushTestApi.IsInFallbackMode)
            {
                TestResult.Text = "RevealStates: Skipped";
                return false;
            }

            return true;
        }

        bool VerifyHoverBrushTargeting(RevealBrush testBrush)
        {
            return VerifyAmbientLight(testBrush, true) &&
                   VerifyBorderLight(testBrush, false) &&
                   VerifyHoverLight(testBrush, true);
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

        private void ElementTheme_Changed(object sender, SelectionChangedEventArgs e)
        {
            var selectedText = ElementThemeComboBox.GetSelectedText();
            switch (selectedText)
            {
                case "Default": this.RequestedTheme = ElementTheme.Default; break;
                case "Light": this.RequestedTheme = ElementTheme.Light; break;
                case "Dark": this.RequestedTheme = ElementTheme.Dark; break;
            }
        }

        private void AnotherListViewItem_Holding(object sender, Windows.UI.Xaml.Input.HoldingRoutedEventArgs e)
        {
            if (!ValidateEffectsPresent()) { return; }
            using (var logger = new ResultsLogger("BorderLight_TapAndHold", TestResult))
            {
                bool shouldBorderLightBeOn = ShouldBorderLightBeOn();
                logger.Verify(shouldBorderLightBeOn == true, "ShouldBorderLightBeOn: " + shouldBorderLightBeOn);
            }
        }

        private void NarrowButton_Holding(object sender, Windows.UI.Xaml.Input.HoldingRoutedEventArgs e)
        {
            if (!ValidateEffectsPresent()) { return; }

            using (var logger = new ResultsLogger("HoverLightExpression_Button_Touch", TestResult))
            {
                Control target = FindName("NarrowButton") as Control;
                var lights = GetElementForHoverLight(target).Lights;

                var hoverLight = _revealTestApi.GetAsRevealHoverLight(lights[0]);
                var offsetExpression = _revealTestApi.GetHoverLightOffsetExpression(hoverLight);
                logger.Verify(String.Equals(offsetExpression.Expression, revealHoverLightOffset_PointerExpression), "HoverLightOffset using incorrect expression: " + offsetExpression.Expression + ", expected: " + revealHoverLightOffset_PointerExpression);
            }
        }

        private void AnotherButton_Click(object sender, RoutedEventArgs e)
        {
            if (!ValidateEffectsPresent()) { return; }

            using (var logger = new ResultsLogger("HoverLightExpression_Button_Keyboard", TestResult))
            {
                Control target = FindName("AnotherButton") as Control;
                var lights = GetElementForHoverLight(target).Lights;

                var hoverLight = _revealTestApi.GetAsRevealHoverLight(lights[1]);           // Look at the press light
                var offsetExpression = _revealTestApi.GetHoverLightOffsetExpression(hoverLight);
                logger.Verify(String.Equals(offsetExpression.Expression, revealHoverLightOffset_KeyboardExpression), "HoverLightOffset using incorrect expression: " + offsetExpression.Expression + ", expected: " + revealHoverLightOffset_KeyboardExpression);
            }
        }

        private void OneMoreListViewItem_Holding(object sender, Windows.UI.Xaml.Input.HoldingRoutedEventArgs e)
        {
            if (!ValidateEffectsPresent()) { return; }

            using (var logger = new ResultsLogger("HoverLightExpression_ListViewItem_Touch", TestResult))
            {
                Control target = FindName("OneMoreListViewItem") as Control;
                var lights = GetElementForHoverLight(target).Lights;

                var hoverLight = _revealTestApi.GetAsRevealHoverLight(lights[0]);
                var offsetExpression = _revealTestApi.GetHoverLightOffsetExpression(hoverLight);
                logger.Verify(String.Equals(offsetExpression.Expression, revealHoverLightOffset_PointerExpression), "HoverLightOffset using incorrect expression: " + offsetExpression.Expression + ", expected: " + revealHoverLightOffset_PointerExpression);
            }
        }

        private void BorderLight_ValidateBorderLightPanAway()
        {
            if (!ValidateEffectsPresent()) { return; }

            using (var logger = new ResultsLogger("BorderLight_PanAway", TestResult))
            {
                bool shouldBorderLightBeOn = ShouldBorderLightBeOn();
                logger.Verify(shouldBorderLightBeOn == false, "ShouldBorderLightBeOn: " + shouldBorderLightBeOn);
            }
        }

        private bool HoverLight_ValidateNormal(RevealHoverLight hoverLight)
        {
            bool isPressed = _revealTestApi.HoverLight_IsPressed(hoverLight);
            bool isPointerOver = _revealTestApi.HoverLight_IsPointerOver(hoverLight);
            bool shouldLightBeOn = _revealTestApi.HoverLight_ShouldBeOn(hoverLight);

            return !isPressed && !isPointerOver && !shouldLightBeOn;
        }

        private bool HoverLight_ValidatePointerOver(RevealHoverLight hoverLight)
        {
            bool isPressed = _revealTestApi.HoverLight_IsPressed(hoverLight);
            bool isPointerOver = _revealTestApi.HoverLight_IsPointerOver(hoverLight);
            bool shouldLightBeOn = _revealTestApi.HoverLight_ShouldBeOn(hoverLight);

            return !isPressed && isPointerOver && shouldLightBeOn;
        }

        private bool HoverLight_ValidatePressed(RevealHoverLight hoverLight)
        {
            bool isPressed = _revealTestApi.HoverLight_IsPressed(hoverLight);
            bool isPointerOver = _revealTestApi.HoverLight_IsPointerOver(hoverLight);
            bool shouldLightBeOn = _revealTestApi.HoverLight_ShouldBeOn(hoverLight);

            return isPressed && isPointerOver && shouldLightBeOn;
        }

        bool ShouldBorderLightBeOn()
        {
            Windows.UI.Xaml.Media.XamlLight xamlLight = _revealTestApi.BorderLight;
            RevealBorderLight borderLight = _revealTestApi.GetAsRevealBorderLight(xamlLight);
            return _revealTestApi.BorderLight_ShouldBeOn(borderLight);
        }

        private void SetState_Click(object sender, RoutedEventArgs e)
        {
            RevealBrushState state = RevealBrushState.Pressed;
            RevealBrush.SetState(Border4SetStatePressed, state);
            TestResult.Text = "SetState_Click: Clicked";
        }

        private void HoverLight_ValidatePointerOverState_Values()
        {
            HoverLightStateValuesValidationHelper(
                1.0F, 1.0F, c_hoverLight_OuterConeAngle_Expected,
                0.0F, 0.0F, 0.1845F,
                "PointerOver_Values",
                "LargeButton2");
        }

        private void HoverLight_ValidatePressedState_Values()
        {
            HoverLightStateValuesValidationHelper(
                1.0F, 1.0F, c_hoverLight_OuterConeAngle_Expected,
                1.0F, 1.0F, 0.1845F,
                "Pressed_Values",
                "LargeButton2");
        }

        private void HoverLight_ValidateSlowRelease_Values()
        {
            ChainedHoverLightStateValuesValidationHelper(
                1.0F, 1.0F, c_hoverLight_OuterConeAngle_Expected,
                1.0F, 1.0F, 0.1845F,
                1.0F, 1.0F, c_hoverLight_OuterConeAngle_Expected,
                0.0F, 0.0F, 0.6865F,
                "SlowRelease_Values",
                "LargeButton2");
        }

        private void HoverLight_ValidateFastRelease_Values()
        {
            // Note that for FastRelease, we expect to go through the same intermiedate and final values 
            // as SlowReleaseabove, it just happens more quickly. 
            ChainedHoverLightStateValuesValidationHelper(
                1.0F, 1.0F, c_hoverLight_OuterConeAngle_Expected,
                1.0F, 1.0F, 0.1845F,
                1.0F, 1.0F, c_hoverLight_OuterConeAngle_Expected,
                0.0F, 0.0F, 0.6865F,
                "FastRelease_Values",
                "LargeButton2");
        }

        private void HoverLightStateValuesValidationHelper(
            float hoverLight_InnerConeIntensityExpected, float hoverLight_outerConeIntensityExpected, float hoverLight_outerConeAngleExpected,
            float pressLight_InnerConeIntensityExpected, float pressLight_outerConeIntensityExpected, float pressLight_outerConeAngleExpected,
            string testName, 
            string targetName)
        {
            if (!ValidateEffectsPresent()) { return; }

            WaitHandle[] hoverLight_ValueValidationEvents = new WaitHandle[3];
            WaitHandle[] pressLight_ValueValidationEvents = new WaitHandle[3];

            Control target = FindName(targetName) as Control;
            var lights = GetElementForHoverLight(target).Lights;

            var hoverLight = _revealTestApi.GetAsRevealHoverLight(lights[0]);
            var pressLight = _revealTestApi.GetAsRevealHoverLight(lights[1]);
            _hoverSpotlight = _revealTestApi.GetSpotLight(hoverLight as Windows.UI.Xaml.Media.XamlLight);
            _pressSpotlight = _revealTestApi.GetSpotLight(pressLight as Windows.UI.Xaml.Media.XamlLight);

            hoverLight_ValueValidationEvents[0] = _compositionPropertyLogger.RegisterProperty(_hoverSpotlight, "InnerConeIntensity", CompositionPropertyLogger.LoggableType.Float, 0.0f, hoverLight_InnerConeIntensityExpected, coneIntensityTolerance);
            hoverLight_ValueValidationEvents[1] = _compositionPropertyLogger.RegisterProperty(_hoverSpotlight, "OuterConeIntensity", CompositionPropertyLogger.LoggableType.Float, 0.0f, hoverLight_outerConeIntensityExpected, coneIntensityTolerance);
            hoverLight_ValueValidationEvents[2] = _compositionPropertyLogger.RegisterProperty(_hoverSpotlight, "OuterConeAngle", CompositionPropertyLogger.LoggableType.Float, 0.0f, hoverLight_outerConeAngleExpected, coneAngleTolerance);

            pressLight_ValueValidationEvents[0] = _compositionPropertyLogger.RegisterProperty(_pressSpotlight, "InnerConeIntensity", CompositionPropertyLogger.LoggableType.Float, 0.0f, pressLight_InnerConeIntensityExpected, coneIntensityTolerance);
            pressLight_ValueValidationEvents[1] = _compositionPropertyLogger.RegisterProperty(_pressSpotlight, "OuterConeIntensity", CompositionPropertyLogger.LoggableType.Float, 0.0f, pressLight_outerConeIntensityExpected, coneIntensityTolerance);
            pressLight_ValueValidationEvents[2] = _compositionPropertyLogger.RegisterProperty(_pressSpotlight, "OuterConeAngle", CompositionPropertyLogger.LoggableType.Float, 0.0f, pressLight_outerConeAngleExpected, coneAngleTolerance);


            // Off-thread validation waits until each registered property hits its expected value
            Task.Run(() =>
            {
                var combined = hoverLight_ValueValidationEvents.Concat(pressLight_ValueValidationEvents).ToArray();
                bool hoverLight_Result = WaitHandle.WaitAll(hoverLight_ValueValidationEvents, 10000);
                bool pressLight_Result = WaitHandle.WaitAll(pressLight_ValueValidationEvents, 10000);
                RunOnUIThread.Execute(() =>
                {
                    using (var logger = new ResultsLogger(testName, TestResult))
                    {
                        var hoverLight_InnerConeIntensityValues = _compositionPropertyLogger.GetValues(_hoverSpotlight, "InnerConeIntensity");
                        var hoverLight_OuterConeIntensityValues = _compositionPropertyLogger.GetValues(_hoverSpotlight, "OuterConeIntensity");
                        var hoverLight_OuterConeAngleValues = _compositionPropertyLogger.GetValues(_hoverSpotlight, "OuterConeAngle");

                        logger.LogMessage("HoverLight - InnerConeIntensity: " + string.Join(", ", hoverLight_InnerConeIntensityValues));
                        logger.LogMessage("HoverLight - OuterConeIntensity: " + string.Join(", ", hoverLight_OuterConeIntensityValues));
                        logger.LogMessage("HoverLight - OuterConeAngle: " + string.Join(", ", hoverLight_OuterConeAngleValues));

                        var pressLight_InnerConeIntensityValues = _compositionPropertyLogger.GetValues(_pressSpotlight, "InnerConeIntensity");
                        var pressLight_OuterConeIntensityValues = _compositionPropertyLogger.GetValues(_pressSpotlight, "OuterConeIntensity");
                        var pressLight_OuterConeAngleValues = _compositionPropertyLogger.GetValues(_pressSpotlight, "OuterConeAngle");

                        logger.LogMessage("PressLight - InnerConeIntensity: " + string.Join(", ", pressLight_InnerConeIntensityValues));
                        logger.LogMessage("PressLight - OuterConeIntensity: " + string.Join(", ", pressLight_OuterConeIntensityValues));
                        logger.LogMessage("PressLight - OuterConeAngle: " + string.Join(", ", pressLight_OuterConeAngleValues));

                        _compositionPropertyLogger.UnregisterProperty(_hoverSpotlight, "InnerConeIntensity", CompositionPropertyLogger.LoggableType.Float);
                        _compositionPropertyLogger.UnregisterProperty(_hoverSpotlight, "OuterConeIntensity", CompositionPropertyLogger.LoggableType.Float);
                        _compositionPropertyLogger.UnregisterProperty(_hoverSpotlight, "OuterConeAngle", CompositionPropertyLogger.LoggableType.Float);
                        Array.Clear(hoverLight_ValueValidationEvents, 0, 3);

                        _compositionPropertyLogger.UnregisterProperty(_pressSpotlight, "InnerConeIntensity", CompositionPropertyLogger.LoggableType.Float);
                        _compositionPropertyLogger.UnregisterProperty(_pressSpotlight, "OuterConeIntensity", CompositionPropertyLogger.LoggableType.Float);
                        _compositionPropertyLogger.UnregisterProperty(_pressSpotlight, "OuterConeAngle", CompositionPropertyLogger.LoggableType.Float);
                        Array.Clear(pressLight_ValueValidationEvents, 0, 3);

                        logger.Verify(hoverLight_Result && hoverLight_Result, "StateValidationResult: error occured");
                    }
                });
            });
        }

        // Validates that we hit expected values for a sequence of two states
        // TODO: Generalize and convert to more elegant pattern (if required by test scenarios)
        private void ChainedHoverLightStateValuesValidationHelper(
            float hoverLight_InnerConeIntensityExpected, float hoverLight_outerConeIntensityExpected, float hoverLight_outerConeAngleExpected,
            float pressLight_InnerConeIntensityExpected, float pressLight_outerConeIntensityExpected, float pressLight_outerConeAngleExpected,
            float hoverLight_InnerConeIntensityExpected2, float hoverLight_outerConeIntensityExpected2, float hoverLight_outerConeAngleExpected2,
            float pressLight_InnerConeIntensityExpected2, float pressLight_outerConeIntensityExpected2, float pressLight_outerConeAngleExpected2,
            string testName,
            string targetName)
        {
            if (!ValidateEffectsPresent()) { return; }

            WaitHandle[] hoverLight_ValueValidationEvents = new WaitHandle[3];
            WaitHandle[] pressLight_ValueValidationEvents = new WaitHandle[3];

            Control target = FindName(targetName) as Control;
            var lights = GetElementForHoverLight(target).Lights;

            var hoverLight = _revealTestApi.GetAsRevealHoverLight(lights[0]);
            var pressLight = _revealTestApi.GetAsRevealHoverLight(lights[1]);
            _hoverSpotlight = _revealTestApi.GetSpotLight(hoverLight as Windows.UI.Xaml.Media.XamlLight);
            _pressSpotlight = _revealTestApi.GetSpotLight(pressLight as Windows.UI.Xaml.Media.XamlLight);

            hoverLight_ValueValidationEvents[0] = _compositionPropertyLogger.RegisterProperty(_hoverSpotlight, "InnerConeIntensity", CompositionPropertyLogger.LoggableType.Float, 0.0f, hoverLight_InnerConeIntensityExpected, coneIntensityTolerance);
            hoverLight_ValueValidationEvents[1] = _compositionPropertyLogger.RegisterProperty(_hoverSpotlight, "OuterConeIntensity", CompositionPropertyLogger.LoggableType.Float, 0.0f, hoverLight_outerConeIntensityExpected, coneIntensityTolerance);
            hoverLight_ValueValidationEvents[2] = _compositionPropertyLogger.RegisterProperty(_hoverSpotlight, "OuterConeAngle", CompositionPropertyLogger.LoggableType.Float, 0.0f, hoverLight_outerConeAngleExpected, coneAngleTolerance);

            pressLight_ValueValidationEvents[0] = _compositionPropertyLogger.RegisterProperty(_pressSpotlight, "InnerConeIntensity", CompositionPropertyLogger.LoggableType.Float, 0.0f, pressLight_InnerConeIntensityExpected, coneIntensityTolerance);
            pressLight_ValueValidationEvents[1] = _compositionPropertyLogger.RegisterProperty(_pressSpotlight, "OuterConeIntensity", CompositionPropertyLogger.LoggableType.Float, 0.0f, pressLight_outerConeIntensityExpected, coneIntensityTolerance);
            pressLight_ValueValidationEvents[2] = _compositionPropertyLogger.RegisterProperty(_pressSpotlight, "OuterConeAngle", CompositionPropertyLogger.LoggableType.Float, 0.0f, pressLight_outerConeAngleExpected, coneAngleTolerance);


            // Off-thread validation waits until each registered property hits its expected value
            Task.Run(() =>
            {
                var combined = hoverLight_ValueValidationEvents.Concat(pressLight_ValueValidationEvents).ToArray();
                bool hoverLight_Result = WaitHandle.WaitAll(hoverLight_ValueValidationEvents, 10000);
                bool pressLight_Result = WaitHandle.WaitAll(pressLight_ValueValidationEvents, 10000);
                RunOnUIThread.Execute(() =>
                {
                    // We will log values when we hit the secod expected state, so don't log at this point.
                    _compositionPropertyLogger.UnregisterProperty(_hoverSpotlight, "InnerConeIntensity", CompositionPropertyLogger.LoggableType.Float);
                    _compositionPropertyLogger.UnregisterProperty(_hoverSpotlight, "OuterConeIntensity", CompositionPropertyLogger.LoggableType.Float);
                    _compositionPropertyLogger.UnregisterProperty(_hoverSpotlight, "OuterConeAngle", CompositionPropertyLogger.LoggableType.Float);
                    Array.Clear(hoverLight_ValueValidationEvents, 0, 3);

                    _compositionPropertyLogger.UnregisterProperty(_pressSpotlight, "InnerConeIntensity", CompositionPropertyLogger.LoggableType.Float);
                    _compositionPropertyLogger.UnregisterProperty(_pressSpotlight, "OuterConeIntensity", CompositionPropertyLogger.LoggableType.Float);
                    _compositionPropertyLogger.UnregisterProperty(_pressSpotlight, "OuterConeAngle", CompositionPropertyLogger.LoggableType.Float);
                    Array.Clear(pressLight_ValueValidationEvents, 0, 3);

                    // Now validate we hit values for the second expected state
                    HoverLightStateValuesValidationHelper(
                        hoverLight_InnerConeIntensityExpected2, hoverLight_outerConeIntensityExpected2, hoverLight_outerConeAngleExpected2,
                        pressLight_InnerConeIntensityExpected2, pressLight_outerConeIntensityExpected2, pressLight_outerConeAngleExpected2,
                        testName,
                        targetName);
                });
            });
        }

        private void HoverLight_ValidatePosition_Offset1_Values()
        {
            HoverLightPositionValuesValidationHelper(new System.Numerics.Vector3(10F, 90F, 256F), "Offset1_Values", "LargeButton2");
        }

        private void HoverLight_ValidatePosition_Offset2_Values()
        {
            HoverLightPositionValuesValidationHelper(new System.Numerics.Vector3(50F, 50F, 256F), "Offset2_Values", "LargeButton2");
        }

        private void HoverLight_ValidatePosition_Offset3_Values()
        {
            HoverLightPositionValuesValidationHelper(new System.Numerics.Vector3(10F, 40F, 256F), "Offset3_Values", "NarrowButton2");
        }

        private void HoverLightPositionValuesValidationHelper(System.Numerics.Vector3 positionExpected, string testName, string targetName)
        {
            if (!ValidateEffectsPresent()) { return; }

            WaitHandle[] valueValidationEvents = new WaitHandle[1];

            Control target = FindName(targetName) as Control;
            var lights = GetElementForHoverLight(target).Lights;

            var hoverLight = _revealTestApi.GetAsRevealHoverLight(lights[0]);
            _hoverSpotlight = _revealTestApi.GetSpotLight(hoverLight as Windows.UI.Xaml.Media.XamlLight);

            valueValidationEvents[0] = _compositionPropertyLogger.RegisterProperty(_hoverSpotlight, "Offset", CompositionPropertyLogger.LoggableType.Vector3, new System.Numerics.Vector3(0F, 0F, 0F), positionExpected, offsetCoordinateTolerance);

            // Off-thread validation waits until each registered property hits its expected value
            Task.Run(() =>
            {
                bool result = WaitHandle.WaitAll(valueValidationEvents, 10000);
                RunOnUIThread.Execute(() =>
                {
                    using (var logger = new ResultsLogger(testName, TestResult))
                    {
                        var positionValues = _compositionPropertyLogger.GetValues(_hoverSpotlight, "Offset");
                        logger.LogMessage("Offset: " + string.Join(", ", positionValues));

                        _compositionPropertyLogger.UnregisterProperty(_hoverSpotlight, "Offset", CompositionPropertyLogger.LoggableType.Vector3);
                        Array.Clear(valueValidationEvents, 0, 1);

                        logger.Verify(result, "PositionValidationResult: error occured");
                    }
                });
            });
        }

        private float DegreesToRadians(float angle)
        {
            return ((float)Math.PI) * angle / 180.0F;
        }

        private float RadiansToDegrees(float angle)
        {
            return 180.0F * angle / ((float)Math.PI);
        }


        private void NotificationsTestButton_Click(object sender, RoutedEventArgs e)
        {

        }
    }
}