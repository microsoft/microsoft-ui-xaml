// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Threading;
using Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Infra;
using Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Common;
using Windows.Foundation.Metadata;
using Common;
using System;

#if USING_TAEF
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;
#else
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.VisualStudio.TestTools.UnitTesting.Logging;
#endif

#if BUILD_WINDOWS
using System.Windows.Automation;
using MS.Internal.Mita.Foundation;
using MS.Internal.Mita.Foundation.Controls;
using MS.Internal.Mita.Foundation.Patterns;
using MS.Internal.Mita.Foundation.Waiters;
#else
using Microsoft.Windows.Apps.Test.Automation;
using Microsoft.Windows.Apps.Test.Foundation;
using Microsoft.Windows.Apps.Test.Foundation.Controls;
using Microsoft.Windows.Apps.Test.Foundation.Patterns;
using Microsoft.Windows.Apps.Test.Foundation.Waiters;
#endif

namespace Windows.UI.Xaml.Tests.MUXControls.InteractionTests
{
#if BUILD_WINDOWS
    using Window = MS.Internal.Mita.Foundation.Controls.Window;
#else
    using Window = Microsoft.Windows.Apps.Test.Foundation.Controls.Window;
#endif

    [TestClass]
    public class AcrylicBrushTests : IDisposable
    {
        private const string AcrylicRectangleName = "Rectangle1";
        private const string TintOpacitySliderName = "TintOpacity";
        private const string SizeSliderName = "SizeSlider";

        private AutoResetEvent HideAndShowWindow_GotWindowHiddenEvent = new AutoResetEvent(false);
        private AutoResetEvent HideAndShowWindow_GotWindowVisibleEvent = new AutoResetEvent(false);

        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }

        protected virtual void Dispose(bool disposing)
        {
            HideAndShowWindow_GotWindowHiddenEvent.Dispose();
            HideAndShowWindow_GotWindowVisibleEvent.Dispose();
        }

        [ClassInitialize]
        [TestProperty("RunAs", "User")]
        [TestProperty("Classification", "Integration")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        [TestProperty("MUXControlsTestEnabledForPhone", "True")]
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

        private bool OnRS2OrGreater()
        {
            bool result = true;
            if (!ApiInformation.IsTypePresent("Windows.UI.Xaml.Media.XamlCompositionBrushBase"))
            {
                Log.Comment("AcrylicBrush only supported on RS2 or greater builds... skipping test");
                result = false;
            }

            return result;
        }

        [TestMethod]
        [TestProperty("EnableForPGOTraining", "True")]
        public void BasicAcrylicOnRectangle()
        {
            if (!OnRS2OrGreater()) { return; }

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
            if (!OnRS2OrGreater()) { return; }

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

                if (PlatformConfiguration.IsDevice(DeviceType.Phone))
                {
                    // On Phone, HostBackdrop validation will fail since Comp will just render black
                    Verify.AreEqual(result.Value, "AcrylicPropertyChanges: Failed (True,False,True,True,False,True,True)");
                }
                else
                {
                    Verify.AreEqual(result.Value, "AcrylicPropertyChanges: Passed");
                }
            }
        }


        [TestMethod]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        public void HideAndShowWindow()
        {
            if (!OnRS2OrGreater()) { return; }

            using (var setup = new TestSetupHelper(new[] { "Acrylic Tests", "navigateToBasicAcrylic" }))
            {
                ChooseFromComboBox("TestNameComboBox", "HideAndShowWindow");

                if (PlatformConfiguration.IsDevice(DeviceType.Phone))
                {
                    Log.Warning("Test is disabled on phone.");
                    return;
                }

                Button runTestButton;
                var result = new Edit(FindElement.ById("TestResult"));
                using (var waiter = new ValueChangedEventWaiter(result))
                {
                    runTestButton = new Button(FindElement.ById("RunTestButton"));
                    runTestButton.Invoke();
                    LogBrushSate();
                    waiter.Wait();
                }

                if (result.Value.Equals("HideAndShowWindow: Skipped"))
                {
                    Log.Error("Error: FallbackBrush in use - expecting effect brush");
                    return;
                }
                else if (TestEnvironment.Application.ApplicationFrameWindow == null)
                {
                    Log.Comment("Skipping test: No ApplicationFrameWindow (likely unsupported platform)");
                    return;
                }
                else
                {
                    Thread waiterThread = new Thread(HideAndShowWindow_WaiterThreadProc);
                    waiterThread.Name = " HideAndShowWindow_WaiterThread";
                    waiterThread.Start();

                    Window window = new Window(TestEnvironment.Application.ApplicationFrameWindow);
                    WindowVisualState initialVisualState = window.WindowVisualState;
                    Verify.AreNotEqual(initialVisualState, WindowVisualState.Minimized);

                    // Minimize the app, which will also trigger it to supsend. Wait for Suspending event from app.
                    Log.Comment("Minimizing the window...");
                    window.SetWindowVisualState(WindowVisualState.Minimized);
                    HideAndShowWindow_GotWindowHiddenEvent.WaitOne();

                    // Restore the app. Wait for VisibilityChanged -> Visible event from app.
                    Log.Comment("Restoring the window...");
                    window.SetWindowVisualState(initialVisualState);
                    HideAndShowWindow_GotWindowVisibleEvent.WaitOne();

                    // Trigger test to validate that noise has been recreated (see Bug 11144540)
                    using (var waiter = new ValueChangedEventWaiter(result))
                    {
                        runTestButton.Invoke();
                        waiter.Wait();
                    }

                    // Read off validation result and complete the test
                    Verify.AreEqual(result.Value, "HideAndShowWindow: Passed");

                    Wait.ForIdle();
                }
            }
        }

        // Helper thread that waits on AutomationEvents from the app and then signals the main test thread.
        // This ensures events aren't missed while the main thread is calling into Mita API's to manipulate the UI.
        void HideAndShowWindow_WaiterThreadProc()
        {
            UIObject acrylicBrushPage = FindElement.ById("AcrylicBrushPage");

            // Note: Tooltip AutomationEvents are being used here to simulate window visiblity change notifications.
            //       See comments in AcrylicBrushPageXaml.cs for more detail.
            using (var waiter = new AutomationEventWaiter(AutomationElementIdentifiers.AsyncContentLoadedEvent, acrylicBrushPage, Scope.Element))
            {
                // Wait for VisibiltyChanged -> Visible notification from app.
                waiter.Wait();
                waiter.Reset();
                Log.Comment("Got Window VisibilityChanged -> Hidden notificaiton.");
                HideAndShowWindow_GotWindowHiddenEvent.Set();

                // Wait for VisibiltyChanged -> Visible notification  from app.
                waiter.Wait();
                waiter.Reset();
                Log.Comment("Got Window  VisibilityChanged -> Visible notification.");
                HideAndShowWindow_GotWindowVisibleEvent.Set();
            }
        }

        [TestMethod]
        public void AcrylicAlwaysUseFallback()
        {
            if (!OnRS2OrGreater()) { return; }

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
            if (!OnRS2OrGreater()) { return; }

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
            if (!OnRS2OrGreater()) { return; }

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
            if (!OnRS2OrGreater()) { return; }

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

                if (PlatformConfiguration.IsDevice(DeviceType.Phone))
                {
                    // On Phone, HostBackdrop validation will fail since Comp will just render black
                    Verify.AreEqual(result.Value, "AcrylicFromMarkup: Failed (True,False,True)");
                }
                else
                {
                    Verify.AreEqual(result.Value, "AcrylicFromMarkup: Passed");
                }
            }
        }

        [TestMethod]
        public void AcrylicRendering()
        {
            if (!OnRS2OrGreater()) { return; }

            if (PlatformConfiguration.IsDevice(DeviceType.Phone))
            {
                Log.Warning("Test is disabled on phone due to Bug 10952754: [Watson Failure] caused by NULL_CLASS_PTR_READ_c0000005_Windows.UI.Xaml.dll!RenderTargetBitmapImplUsingSpriteVisuals::PostDraw.");
                return;
            }

            using (var setup = new TestSetupHelper(new[] { "Acrylic Tests", "navigateToRenderingAcrylic" }))
            {
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
        public void VerifyOpaqueTintOptimization()
        {
            if (!OnRS2OrGreater()) { return; }

            // Opaque Tint Optimization removed with Luminosity-based Acrylic recipe added in 19H1
            if (PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.NineteenH1)) { return; }

            using (var setup = new TestSetupHelper(new[] { "Acrylic Tests", "navigateToBasicAcrylic" }))
            {
                ChooseFromComboBox("TestNameComboBox", "VerifyOpaqueTintOptimization");

                var result = new Edit(FindElement.ById("TestResult"));
                using (var waiter = new ValueChangedEventWaiter(result))
                {
                    Button runTestButton = new Button(FindElement.ById("RunTestButton"));
                    runTestButton.Invoke();
                    LogBrushSate();

                    if (result.Value.Equals("VerifyOpaqueTintOptimization: Skipped"))
                    {
                        Log.Error("Error: FallbackBrush in use - expecting effect brush");
                        return;
                    }

                    waiter.Wait();
                }

                Verify.AreEqual(result.Value, "VerifyOpaqueTintOptimization: Passed");
            }
        }

        [TestMethod]
        public void TintTransitionDuration()
        {
            if (!OnRS2OrGreater()) { return; }

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
            if (!OnRS2OrGreater()) { return; }

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
            if (!OnRS2OrGreater()) { return; }

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
        public void VerifyAcrylicEffectBrushForShell()
        {
            if (!OnRS2OrGreater()) { return; }

            using (var setup = new TestSetupHelper(new[] { "Acrylic Tests", "navigateToBasicAcrylic" }))
            {
                // Just click the button to show acrylic and then hide it to make sure that the API works.
                // There's not much we can validate beyond that because we can't peer into the effect brush.
                Button runTestButton = new Button(FindElement.ById("CreateAcrylicEffectBrush"));
                runTestButton.Invoke();

                Wait.ForIdle();

                runTestButton.Invoke();
            }
        }

        [TestMethod]
        public void AcrylicRendering_LuminosityBlend()
        {
            if (!OnRS2OrGreater()) { return; }

            if (PlatformConfiguration.IsDevice(DeviceType.Phone))
            {
                Log.Warning("Test is disabled on phone due to Bug 10952754: [Watson Failure] caused by NULL_CLASS_PTR_READ_c0000005_Windows.UI.Xaml.dll!RenderTargetBitmapImplUsingSpriteVisuals::PostDraw.");
                return;
            }

            using (var setup = new TestSetupHelper(new[] { "Acrylic Tests", "navigateToRenderingAcrylic" }))
            {
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
