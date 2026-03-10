// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using Common;
using Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.Infra;
using Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.Common;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

using Microsoft.Windows.Apps.Test.Automation;
using Microsoft.Windows.Apps.Test.Foundation;
using Microsoft.Windows.Apps.Test.Foundation.Controls;
using Microsoft.Windows.Apps.Test.Foundation.Waiters;
using MUXTestInfra.Shared.Infra;

namespace Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests
{
    [TestClass]
    public class InkCanvasTests
    {
        [ClassInitialize]
        [TestProperty("RunAs", "User")]
        [TestProperty("Classification", "Integration")]
        [TestProperty("Platform", "Any")]
        [TestProperty("MUXControlsTestSuite", "SuiteB")]
        public static void ClassInitialize(TestContext testContext)
        {
            TestEnvironment.Initialize(testContext);
        }

        public void TestCleanup()
        {
            TestCleanupHelper.Cleanup();
        }

        [TestMethod]
        public void InkCanvasRendersInVisualTree()
        {
            using (var setup = new TestSetupHelper("InkCanvas Tests"))
            {
                var inkCanvas = FindElement.ByName("TestInkCanvas");
                Verify.IsNotNull(inkCanvas, "InkCanvas should be present in the visual tree.");
            }
        }

        [TestMethod]
        public void InkCanvasModeSwitching()
        {
            using (var setup = new TestSetupHelper("InkCanvas Tests"))
            {
                var modeSelector = FindElement.ByName<ComboBox>("ModeSelector");
                Verify.IsNotNull(modeSelector, "ModeSelector should be present.");

                var modeText = FindElement.ByName("ModeText");
                Verify.IsNotNull(modeText, "ModeText should be present.");

                // Switch to Erase
                modeSelector.SelectItemByName("Erase");
                Wait.ForIdle();

                var statusText = FindElement.ByName("StatusText");
                Log.Comment($"Status after Erase: {statusText.GetText()}");

                // Switch to Select
                modeSelector.SelectItemByName("Select");
                Wait.ForIdle();

                // Switch back to Draw
                modeSelector.SelectItemByName("Draw");
                Wait.ForIdle();
            }
        }

        [TestMethod]
        public void InkCanvasInputTypeConfiguration()
        {
            using (var setup = new TestSetupHelper("InkCanvas Tests"))
            {
                var penInput = FindElement.ByName<CheckBox>("PenInput");
                var mouseInput = FindElement.ByName<CheckBox>("MouseInput");
                var touchInput = FindElement.ByName<CheckBox>("TouchInput");

                Verify.IsNotNull(penInput, "PenInput checkbox should be present.");
                Verify.IsNotNull(mouseInput, "MouseInput checkbox should be present.");
                Verify.IsNotNull(touchInput, "TouchInput checkbox should be present.");

                // Enable touch
                touchInput.Check();
                Wait.ForIdle();

                // Disable pen
                penInput.Uncheck();
                Wait.ForIdle();

                // Re-enable pen
                penInput.Check();
                Wait.ForIdle();

                var inputTypesText = FindElement.ByName("InputTypesText");
                Verify.IsNotNull(inputTypesText, "InputTypesText should be present.");
            }
        }

        [TestMethod]
        public void InkCanvasClearStrokes()
        {
            using (var setup = new TestSetupHelper("InkCanvas Tests"))
            {
                var clearButton = FindElement.ByName<Button>("ClearButton");
                Verify.IsNotNull(clearButton, "ClearButton should be present.");

                clearButton.Invoke();
                Wait.ForIdle();

                var statusText = FindElement.ByName("StatusText");
                Verify.IsNotNull(statusText, "StatusText should be present.");
                Log.Comment($"Status after clear: {statusText.GetText()}");
            }
        }

        [TestMethod]
        public void InkCanvasSaveLoad()
        {
            using (var setup = new TestSetupHelper("InkCanvas Tests"))
            {
                var saveButton = FindElement.ByName<Button>("SaveButton");
                var loadButton = FindElement.ByName<Button>("LoadButton");
                var statusText = FindElement.ByName("StatusText");

                Verify.IsNotNull(saveButton, "SaveButton should be present.");
                Verify.IsNotNull(loadButton, "LoadButton should be present.");

                // Save
                saveButton.Invoke();
                Wait.ForIdle();
                Log.Comment($"Status after save: {statusText.GetText()}");

                // Load
                loadButton.Invoke();
                Wait.ForIdle();
                Log.Comment($"Status after load: {statusText.GetText()}");
            }
        }

        [TestMethod]
        public void InkCanvasStrokeCountDisplay()
        {
            using (var setup = new TestSetupHelper("InkCanvas Tests"))
            {
                var strokeCountText = FindElement.ByName("StrokeCountText");
                Verify.IsNotNull(strokeCountText, "StrokeCountText should be present.");
                Log.Comment($"Stroke count: {strokeCountText.GetText()}");
            }
        }
    }
}
