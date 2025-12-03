// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;

using Common;
using Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.Infra;
using Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.Common;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

using Microsoft.Windows.Apps.Test.Foundation;
using Microsoft.Windows.Apps.Test.Foundation.Controls;

namespace Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests
{
    [TestClass]
    public class SystemBackdropElementTests
    {
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

        [TestMethod]
        public void CanNavigateToSystemBackdropElementPage()
        {
            using (var setup = new TestSetupHelper("SystemBackdropElement Tests"))
            {
                Log.Comment("Navigate to SystemBackdropElement test page");
                // Verify we're on the correct page by checking for SystemBackdropElement-specific elements
                var testBackdropHost = FindElement.ById("TestBackdropHost");
                Verify.IsNotNull(testBackdropHost, "TestBackdropHost should be found on the SystemBackdropElement page");

                var cornerRadiusSlider = FindElement.ByName("CornerRadiusSlider");
                Verify.IsNotNull(cornerRadiusSlider, "CornerRadiusSlider should be found on the SystemBackdropElement page");
            }
        }

        [TestMethod]
        public void Switch_ToAcrylic_Backdrop_UpdatesUI()
        {
            using (var setup = new TestSetupHelper("SystemBackdropElement Tests"))
            {
                Log.Comment("Click Acrylic button to switch backdrop");

                // Verify initial backdrop type is Mica
                ClickButton("StoreBackdropTypeForTest");
                string initialBackdropType = ReadResult();
                Log.Comment($"Initial backdrop type: {initialBackdropType}");
                Verify.IsTrue(initialBackdropType.Contains("MicaBackdrop"), "Initial backdrop should be MicaBackdrop");

                // Find and click the Acrylic button
                var acrylicButton = FindElement.ByName("AcrylicButton");
                Verify.IsNotNull(acrylicButton, "AcrylicButton should be found");

                var button = new Button(acrylicButton);
                button.Click();

                Log.Comment("Waiting for UI to update after clicking Acrylic button");
                Wait.ForIdle();
                Wait.ForMilliseconds(1000);

                // Verify the status text updated to "Current: Acrylic" with retries
                // NOTE: StatusTextBlock represents user-visible status - valid to test as it could be part of real app UI
                var statusTextBlock = FindElement.ByName("StatusTextBlock");
                Verify.IsNotNull(statusTextBlock, "StatusTextBlock should be found");

                var statusText = new TextBlock(statusTextBlock);
                string currentStatus = statusText.DocumentText;

                int retries = 0;
                const int maxRetries = 10;
                while (currentStatus != "Current: Acrylic" && retries < maxRetries)
                {
                    Log.Comment($"Attempt {retries + 1}/{maxRetries}: Status is '{currentStatus}', waiting for update...");
                    Wait.ForMilliseconds(500);
                    statusText = new TextBlock(statusTextBlock);
                    currentStatus = statusText.DocumentText;
                    retries++;
                }

                Log.Comment($"Final status after {retries} retries: '{currentStatus}'");
                Verify.AreEqual("Current: Acrylic", currentStatus, "Status should indicate Acrylic is the current backdrop");

                // Verify the TestBackdropHost is still accessible (same control, different backdrop)
                var testBackdropHost = FindElement.ById("TestBackdropHost");
                Verify.IsNotNull(testBackdropHost, "TestBackdropHost should still be found after switching backdrop");

                // MOST IMPORTANT: Verify the actual SystemBackdrop property changed
                ClickButton("StoreBackdropTypeForTest");
                string newBackdropType = ReadResult();
                Log.Comment($"New backdrop type from SystemBackdrop property: {newBackdropType}");
                Verify.IsTrue(newBackdropType.Contains("DesktopAcrylicBackdrop"), 
"SystemBackdrop property should be DesktopAcrylicBackdrop (validates actual property changed)");
                Verify.AreNotEqual(initialBackdropType, newBackdropType, 
   "SystemBackdrop property should have changed from Mica to Acrylic");

                Log.Comment("Test passed: Successfully switched SystemBackdrop property to DesktopAcrylicBackdrop (validated via code-behind)");
            }
        }

        [TestMethod]
        public void Adjust_CornerRadius_UpdatesUI()
        {
            using (var setup = new TestSetupHelper("SystemBackdropElement Tests"))
            {
                Log.Comment("Test corner radius slider adjustment");

                // Find the corner radius slider
                var cornerRadiusSlider = FindElement.ByName("CornerRadiusSlider");
                Verify.IsNotNull(cornerRadiusSlider, "CornerRadiusSlider should be found");

                var slider = new RangeValueSlider(cornerRadiusSlider);

                // Get initial value from slider
                Log.Comment("Getting initial corner radius value from slider");
                double initialSliderValue = slider.Value;
                Log.Comment($"Initial corner radius slider value: {initialSliderValue}");

                // Get initial CornerRadius property value from SystemBackdropElement
                ClickButton("StoreCornerRadiusForTest");
                string initialResult = ReadResult();
                Log.Comment($"Initial CornerRadius from SystemBackdropElement: {initialResult}");

                // Get the backdrop host
                var testBackdropHost = FindElement.ById("TestBackdropHost");
                Verify.IsNotNull(testBackdropHost, "TestBackdropHost should be found");

                // Set a new corner radius value (e.g., 25)
                Log.Comment("Adjusting corner radius slider to 25");
                slider.SetValue(25);

                // Wait for UI to update
                Wait.ForIdle();
                Wait.ForMilliseconds(300);

                // Verify the slider value changed
                double newSliderValue = slider.Value;
                Log.Comment($"New corner radius slider value: {newSliderValue}");
                Verify.IsTrue(newSliderValue > 0, "Corner radius slider value should be greater than 0 after adjustment");

                // Verify the actual CornerRadius property on SystemBackdropElement changed
                ClickButton("StoreCornerRadiusForTest");
                string newResult = ReadResult();
                Log.Comment($"New CornerRadius from SystemBackdropElement: {newResult}");

                // Validate that the CornerRadius property actually changed to 25
                Verify.IsTrue(newResult.Contains("25"), "SystemBackdropElement.CornerRadius should be 25 after slider adjustment (validates actual property changed)");
                Verify.AreNotEqual(initialResult, newResult, "CornerRadius property should change after slider adjustment");

                // Verify the SystemBackdropElement control is still accessible after adjustment
                Log.Comment("Verifying SystemBackdropElement is still accessible after CornerRadius change");
                var updatedBackdropHost = FindElement.ById("TestBackdropHost");
                Verify.IsNotNull(updatedBackdropHost, "TestBackdropHost should still be found after adjustment");

                // The backdrop should have updated dimensions
                var newBounds = updatedBackdropHost.BoundingRectangle;
                Log.Comment($"TestBackdropHost bounds after corner radius adjustment: {newBounds.Width}x{newBounds.Height}");

                Log.Comment("Test passed: Corner radius slider updates SystemBackdropElement.CornerRadius property (validated via code-behind)");
            }
        }

        [TestMethod]
        public void Verify_Buttons_AreClickable_WithBackdrop()
        {
            using (var setup = new TestSetupHelper("SystemBackdropElement Tests"))
            {
                Log.Comment("Test that buttons overlaid on SystemBackdropElement are clickable");

                // Verify the SystemBackdropElement is visible
                var testBackdropHost = FindElement.ById("TestBackdropHost");
                Verify.IsNotNull(testBackdropHost, "TestBackdropHost should be found");

                // Find Button1 which is overlaid on the SystemBackdropElement
                var button1 = FindElement.ByName("Button1");
                Verify.IsNotNull(button1, "Button1 should be found on the SystemBackdropElement");

                Log.Comment("Clicking Button1 to verify backdrop doesn't block interaction");
                var clickableButton = new Button(button1);
                clickableButton.Click();

                // Wait for any potential state changes
                Wait.ForIdle();
                Wait.ForMilliseconds(300);

                // The test passes if we successfully clicked the button without exception
                // The button is interactive despite being overlaid on the backdrop
                Log.Comment("Button1 was successfully clicked - backdrop does not block interaction");

                // Find Button2 and Button3 to verify all buttons are accessible
                var button2 = FindElement.ByName("Button2");
                Verify.IsNotNull(button2, "Button2 should be found and accessible on the SystemBackdropElement");

                var button3 = FindElement.ByName("Button3");
                Verify.IsNotNull(button3, "Button3 should be found and accessible on the SystemBackdropElement");

                Log.Comment("All buttons are accessible and clickable");

                // Test with Acrylic backdrop as well
                Log.Comment("Switching to Acrylic backdrop to verify buttons remain clickable");
                var acrylicButton = FindElement.ByName("AcrylicButton");
                Verify.IsNotNull(acrylicButton, "AcrylicButton should be found");

                var backdrop2Button = new Button(acrylicButton);
                backdrop2Button.Click();

                // Wait for UI to update
                Wait.ForIdle();
                Wait.ForMilliseconds(500);

                // Verify backdrop actually changed to Acrylic
                ClickButton("StoreBackdropTypeForTest");
                string backdropType = ReadResult();
                Log.Comment($"Current backdrop type: {backdropType}");
                Verify.IsTrue(backdropType.Contains("DesktopAcrylicBackdrop"), 
   "Backdrop should have switched to DesktopAcrylicBackdrop");

                // Verify buttons are still clickable on Acrylic backdrop
                var acrylicButton1 = FindElement.ByName("Button1");
                Verify.IsNotNull(acrylicButton1, "Button1 should still be found on Acrylic backdrop");

                var clickableAcrylicButton = new Button(acrylicButton1);
                clickableAcrylicButton.Click();

                Log.Comment("Button1 on Acrylic backdrop was successfully clicked");

                Log.Comment("Test passed: Buttons remain clickable on both Mica and Acrylic backdrops - backdrop does not block interaction");
            }
        }

        private void ClickButton(string buttonName)
        {
            var button = new Button(FindElement.ByName(buttonName));
            button.InvokeAndWait();
            Wait.ForIdle();
        }

        private string ReadResult()
        {
            var results = new TextBlock(FindElement.ByName("Results"));
            return results.DocumentText;
        }
    }
}
