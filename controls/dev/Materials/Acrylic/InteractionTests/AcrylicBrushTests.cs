// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Threading;
using Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.Infra;
using Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.Common;
using Windows.Foundation.Metadata;
using Common;
using System;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

using Microsoft.Windows.Apps.Test.Automation;
using Microsoft.Windows.Apps.Test.Foundation;
using Microsoft.Windows.Apps.Test.Foundation.Controls;
using Microsoft.Windows.Apps.Test.Foundation.Patterns;
using Microsoft.Windows.Apps.Test.Foundation.Waiters;

namespace Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests
{
    using Window = Microsoft.Windows.Apps.Test.Foundation.Controls.Window;

    [TestClass]
    public class AcrylicBrushTests
    {
        private const string AcrylicRectangleName = "Rectangle1";
        private const string TintOpacitySliderName = "TintOpacity";
        private const string SizeSliderName = "SizeSlider";

        [ClassInitialize]
        [TestProperty("RunAs", "User")]
        [TestProperty("Classification", "Integration")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        public static void ClassInitialize(TestContext testContext)
        {
            TestEnvironment.Initialize(testContext);
        }

        [TestCleanup]
        public void TestCleanup()
        {
            TestCleanupHelper.Cleanup();
        }

        private bool IsUsingAcrylicBrush()
        {
            TextBlock IsUsingAcrylicBrush_TextBlock = new TextBlock(FindElement.ById("IsUsingAcrylicBrush"));
            return IsUsingAcrylicBrush_TextBlock.DocumentText.Equals("True");
        }

        private void LogBrushSate()
        {
            Log.Comment("Using " + (IsUsingAcrylicBrush() ? "AcrylicBrush" : "FallbackBrush"));

            TextBlock CompositionBrushPointer_TextBlock = new TextBlock(FindElement.ById("CompositionBrushPointer"));
            Log.Comment("CompositionBrushPointer: " + CompositionBrushPointer_TextBlock.DocumentText);

            TextBlock NoiseBrushPointer_TextBlock = new TextBlock(FindElement.ById("NoiseBrushPointer"));
            Log.Comment("NoiseBrushPointer: " + NoiseBrushPointer_TextBlock.DocumentText);
        }

        [TestMethod]
        [TestProperty("EnableForPGOTraining", "True")]
        public void BasicAcrylicOnRectangle()
        {
            using (var setup = new TestSetupHelper(new[] { "Acrylic Tests", "navigateToBasicAcrylic" }))
            {
                ChooseFromComboBox("TestNameComboBox", "BasicAcrylicOnRectangle");

                var result = new Edit(FindElement.ById("TestResult"));
                using (var waiter = new ValueChangedEventWaiter(result))
                {
                    Button runTestButton = new Button(FindElement.ById("RunTestButton"));
                    runTestButton.Invoke();
                    LogBrushSate();
                    waiter.Wait();
                }

                Verify.AreEqual(result.Value, "BasicAcrylicOnRectangle: Passed");
            }
        }

        [TestMethod]
        public void AcrylicPropertyChanges()
        {
            using (var setup = new TestSetupHelper(new[] { "Acrylic Tests", "navigateToBasicAcrylic" }))
            {
                ChooseFromComboBox("TestNameComboBox", "AcrylicPropertyChanges");

                var result = new Edit(FindElement.ById("TestResult"));
                using (var waiter = new ValueChangedEventWaiter(result))
                {
                    Button runTestButton = new Button(FindElement.ById("RunTestButton"));
                    runTestButton.Invoke();
                    LogBrushSate();
                    waiter.Wait();
                }

                Verify.AreEqual(result.Value, "AcrylicPropertyChanges: Passed");
            }
        }


        [TestMethod]
        public void AcrylicAlwaysUseFallback()
        {
            using (var setup = new TestSetupHelper(new[] { "Acrylic Tests", "navigateToBasicAcrylic" }))
            {
                ChooseFromComboBox("TestNameComboBox", "AcrylicAlwaysUseFallback");

                var result = new Edit(FindElement.ById("TestResult"));
                using (var waiter = new ValueChangedEventWaiter(result))
                {
                    Button runTestButton = new Button(FindElement.ById("RunTestButton"));
                    runTestButton.Invoke();
                    LogBrushSate();

                    waiter.Wait();
                }

                if (result.Value.Equals("AcrylicAlwaysUseFallback: Skipped"))
                {
                    Log.Error("Error: FallbackBrush in use - expecting effect brush");
                    return;
                }

                Verify.AreEqual(result.Value, "AcrylicAlwaysUseFallback: Passed");
            }
        }

        [TestMethod]
        public void AcrylicCreatedInFallbackMode()
        {
            using (var setup = new TestSetupHelper(new[] { "Acrylic Tests", "navigateToBasicAcrylic" }))
            {
                ChooseFromComboBox("TestNameComboBox", "AcrylicCreatedInFallbackMode");

                var result = new Edit(FindElement.ById("TestResult"));
                using (var waiter = new ValueChangedEventWaiter(result))
                {
                    Button runTestButton = new Button(FindElement.ById("RunTestButton"));
                    runTestButton.Invoke();
                    LogBrushSate();
                    waiter.Wait();
                }

                Verify.AreEqual(result.Value, "AcrylicCreatedInFallbackMode: Passed");
            }
        }

        [TestMethod]
        public void VerifyDisconnectedState()
        {
            using (var setup = new TestSetupHelper(new[] { "Acrylic Tests", "navigateToBasicAcrylic" }))
            {
                ChooseFromComboBox("TestNameComboBox", "VerifyDisconnectedState");

                var result = new Edit(FindElement.ById("TestResult"));
                using (var waiter = new ValueChangedEventWaiter(result))
                {
                    Button runTestButton = new Button(FindElement.ById("RunTestButton"));
                    runTestButton.Invoke();
                    LogBrushSate();

                    if (result.Value.Equals("VerifyDisconnectedState: Skipped"))
                    {
                        Log.Error("Error: FallbackBrush in use - expecting effect brush");
                        return;
                    }

                    waiter.Wait();
                }

                Verify.AreEqual(result.Value, "VerifyDisconnectedState: Passed");
            }
        }

        [TestMethod]
        public void AcrylicFromMarkup()
        {
            using (var setup = new TestSetupHelper(new[] { "Acrylic Tests", "navigateToMarkupAcrylic" }))
            {
                var result = new Edit(FindElement.ById("TestResult"));
                using (var waiter = new ValueChangedEventWaiter(result))
                {
                    Button runTestButton = new Button(FindElement.ById("RunTestButton"));
                    runTestButton.Invoke();
                    LogBrushSate();

                    if (result.Value.Equals("AcrylicFromMarkup: Skipped"))
                    {
                        Log.Error("Error: FallbackBrush in use - expecting effect brush");
                        return;
                    }

                    waiter.Wait();
                }

                Verify.AreEqual(result.Value, "AcrylicFromMarkup: Passed");
            }
        }

        [TestMethod]
        public void AcrylicRendering()
        {
            using (var setup = new TestSetupHelper(new[] { "Acrylic Tests", "navigateToRenderingAcrylic" }))
            {
                CheckBox cbComplexBackground = new CheckBox(FindElement.ById("cbComplexBackground"));
                cbComplexBackground.Uncheck();

                var result = new Edit(FindElement.ById("TestResult"));
                using (var waiter = new ValueChangedEventWaiter(result))
                {
                    Button runTestButton = new Button(FindElement.ById("RunTestButton"));
                    runTestButton.Invoke();

                    waiter.Wait();
                }

                Verify.AreEqual(result.Value, "AcrylicRendering: Passed");
            }
        }

        [TestMethod]
        public void TintTransitionDuration()
        {
            using (var setup = new TestSetupHelper(new[] { "Acrylic Tests", "navigateToBasicAcrylic" }))
            {
                ChooseFromComboBox("TestNameComboBox", "TintTransitionDuration");

                Button runTestButton = new Button(FindElement.ById("RunTestButton"));

                // Iteration 0 : TintTransitionDuration = 0
                runTestButton.Invoke();
                Wait.ForSeconds(1);

                // Iteration 1 : TintTransitionDuration = 500ms
                runTestButton.Invoke();
                Wait.ForSeconds(1);

                // Iteration 2 : TintTransitionDuration = 999ms
                runTestButton.Invoke();
                Wait.ForSeconds(2);
            }
        }

        [TestMethod]
        public void AcrylicNoiseCache()
        {
            using (var setup = new TestSetupHelper(new[] { "Acrylic Tests", "navigateToBasicAcrylic" }))
            {
                ChooseFromComboBox("TestNameComboBox", "AcrylicNoiseCache");

                var result = new Edit(FindElement.ById("TestResult"));
                using (var waiter = new ValueChangedEventWaiter(result))
                {
                    Button runTestButton = new Button(FindElement.ById("RunTestButton"));
                    runTestButton.Invoke();
                    LogBrushSate();
                    waiter.Wait();
                }

                Verify.AreEqual(result.Value, "AcrylicNoiseCache: Passed");
            }
        }

        [TestMethod]
        public void VerifyAcrylicBrushEffect()
        {
            using (var setup = new TestSetupHelper(new[] { "Acrylic Tests", "navigateToBasicAcrylic" }))
            {
                ChooseFromComboBox("TestNameComboBox", "VerifyAcrylicBrushEffect");

                var result = new Edit(FindElement.ById("TestResult"));
                using (var waiter = new ValueChangedEventWaiter(result))
                {
                    Button runTestButton = new Button(FindElement.ById("RunTestButton"));
                    runTestButton.Invoke();
                    LogBrushSate();
                    waiter.Wait();
                }

                Verify.AreEqual(result.Value, "VerifyAcrylicBrushEffect: Passed");
            }
        }

        [TestMethod]
        public void AcrylicRendering_LuminosityBlend()
        {
            using (var setup = new TestSetupHelper(new[] { "Acrylic Tests", "navigateToRenderingAcrylic" }))
            {
                CheckBox cbComplexBackground = new CheckBox(FindElement.ById("cbComplexBackground"));
                cbComplexBackground.Uncheck();

                var result = new Edit(FindElement.ById("TestResult"));

                using (var waiter = new ValueChangedEventWaiter(result))
                {
                    Button setLuminosityBlendButton = new Button(FindElement.ById("SetLuminosityButton"));
                    setLuminosityBlendButton.Invoke();

                    waiter.Wait();
                }
                LogResult(result, "SetLuminosityBlend");

                using (var waiter = new ValueChangedEventWaiter(result))
                {
                    Button unsetLuminosityBlendButton = new Button(FindElement.ById("UnsetLuminosityButton"));
                    unsetLuminosityBlendButton.Invoke();

                    waiter.Wait();
                }
                LogResult(result, "ClearLuminosityBlend");
            }
        }

        [TestMethod]
        public void AcrylicRendering_AcrylicOnComplexTransparentBG()
        {
            // Bug 39932644: Verify that Acrylic renders properly even if its background includes transparency.
            using (var setup = new TestSetupHelper(new[] { "Acrylic Tests", "navigateToRenderingAcrylic" }))
            {
                CheckBox cbComplexBackground = new CheckBox(FindElement.ById("cbComplexBackground"));
                cbComplexBackground.Check();

                var result = new Edit(FindElement.ById("TestResult"));
                using (var waiter = new ValueChangedEventWaiter(result))
                {
                    Button runTestButton = new Button(FindElement.ById("RunTestButton"));
                    runTestButton.Invoke();

                    waiter.Wait();
                }

                Verify.AreEqual(result.Value, "AcrylicOnComplexTransparentBG: Passed");
            }
        }

        private void LogResult(Edit result, string stateName)
        {
            string expectedResult = stateName + ": Passed";
            bool testPassed = (0 == String.Compare(expectedResult, 0, result.Value, 0, expectedResult.Length));

            if (testPassed)
            {
                Log.Comment(result.Value);
            }
            else
            {
                Log.Error(result.Value);
            }
        }

        private void ChooseFromComboBox(string textBoxName, string text)
        {
            Log.Comment("Retrieve text box with name '{0}'.", textBoxName);
            ComboBox comboBox = new ComboBox(FindElement.ById(textBoxName));
            comboBox.SelectItemByName(text);
            Wait.ForIdle();
        }
    }
}
