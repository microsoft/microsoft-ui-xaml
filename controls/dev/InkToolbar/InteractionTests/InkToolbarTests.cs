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
    public class InkToolbarTests
    {
        [ClassInitialize]
        [TestProperty("RunAs", "User")]
        [TestProperty("Classification", "Integration")]
        [TestProperty("Platform", "Any")]
        [TestProperty("MUXControlsTestSuite", "SuiteB")]
        [TestProperty("Ignore", "True")] // Enable once the InkToolbar interaction test page + harness are registered in MUXControlsTestApp.
        public static void ClassInitialize(TestContext testContext)
        {
            TestEnvironment.Initialize(testContext);
        }

        public void TestCleanup()
        {
            TestCleanupHelper.Cleanup();
        }

        [TestMethod]
        public void InkToolbarRendersInVisualTree()
        {
            using (var setup = new TestSetupHelper("InkToolbar Tests"))
            {
                var toolbar = FindElement.ByName("TestInkToolbar");
                Verify.IsNotNull(toolbar, "InkToolbar should be present in the visual tree.");

                var canvas = FindElement.ByName("TestInkCanvas");
                Verify.IsNotNull(canvas, "Target InkCanvas should be present.");
            }
        }

        [TestMethod]
        public void InkToolbarInitialControlsSelection()
        {
            using (var setup = new TestSetupHelper("InkToolbar Tests"))
            {
                var selector = FindElement.ByName<ComboBox>("InitialControlsSelector");
                Verify.IsNotNull(selector, "InitialControlsSelector should be present.");

                // Switch to None
                selector.SelectItemByName("None");
                Wait.ForIdle();

                var statusText = FindElement.ByName("StatusText");
                Log.Comment($"Status after None: {statusText.GetText()}");

                // Switch to PensOnly
                selector.SelectItemByName("PensOnly");
                Wait.ForIdle();
                Log.Comment($"Status after PensOnly: {statusText.GetText()}");

                // Switch to AllExceptPens
                selector.SelectItemByName("AllExceptPens");
                Wait.ForIdle();
                Log.Comment($"Status after AllExceptPens: {statusText.GetText()}");

                // Switch back to All
                selector.SelectItemByName("All");
                Wait.ForIdle();
            }
        }

        [TestMethod]
        public void InkToolbarOrientationSwitching()
        {
            using (var setup = new TestSetupHelper("InkToolbar Tests"))
            {
                var selector = FindElement.ByName<ComboBox>("OrientationSelector");
                Verify.IsNotNull(selector, "OrientationSelector should be present.");

                // Switch to Vertical
                selector.SelectItemByName("Vertical");
                Wait.ForIdle();

                var orientationText = FindElement.ByName("OrientationText");
                Log.Comment($"Orientation: {orientationText.GetText()}");

                // Switch back to Horizontal
                selector.SelectItemByName("Horizontal");
                Wait.ForIdle();
            }
        }

        [TestMethod]
        public void InkToolbarFlyoutPlacementSwitching()
        {
            using (var setup = new TestSetupHelper("InkToolbar Tests"))
            {
                var selector = FindElement.ByName<ComboBox>("FlyoutPlacementSelector");
                Verify.IsNotNull(selector, "FlyoutPlacementSelector should be present.");

                foreach (var placement in new[] { "Top", "Bottom", "Left", "Right", "Auto" })
                {
                    selector.SelectItemByName(placement);
                    Wait.ForIdle();

                    var flyoutText = FindElement.ByName("FlyoutPlacementText");
                    Log.Comment($"Flyout placement: {flyoutText.GetText()}");
                }
            }
        }

        [TestMethod]
        public void InkToolbarRulerToggle()
        {
            using (var setup = new TestSetupHelper("InkToolbar Tests"))
            {
                var rulerCheckBox = FindElement.ByName<CheckBox>("RulerCheckBox");
                Verify.IsNotNull(rulerCheckBox, "RulerCheckBox should be present.");

                // Check ruler
                rulerCheckBox.Check();
                Wait.ForIdle();

                var statusText = FindElement.ByName("StatusText");
                Log.Comment($"Status after ruler check: {statusText.GetText()}");

                // Uncheck ruler
                rulerCheckBox.Uncheck();
                Wait.ForIdle();
                Log.Comment($"Status after ruler uncheck: {statusText.GetText()}");
            }
        }

        [TestMethod]
        public void InkToolbarStencilToggle()
        {
            using (var setup = new TestSetupHelper("InkToolbar Tests"))
            {
                var stencilCheckBox = FindElement.ByName<CheckBox>("StencilCheckBox");
                Verify.IsNotNull(stencilCheckBox, "StencilCheckBox should be present.");

                stencilCheckBox.Check();
                Wait.ForIdle();

                var statusText = FindElement.ByName("StatusText");
                Log.Comment($"Status after stencil check: {statusText.GetText()}");

                stencilCheckBox.Uncheck();
                Wait.ForIdle();
            }
        }

        [TestMethod]
        public void InkToolbarActiveToolDisplay()
        {
            using (var setup = new TestSetupHelper("InkToolbar Tests"))
            {
                var activeToolText = FindElement.ByName("ActiveToolText");
                Verify.IsNotNull(activeToolText, "ActiveToolText should be present.");
                Log.Comment($"Active tool: {activeToolText.GetText()}");
            }
        }
    }
}
