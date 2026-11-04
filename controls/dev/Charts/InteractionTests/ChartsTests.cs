// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Common;
using Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.Infra;
using Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.Common;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

using Microsoft.Windows.Apps.Test.Foundation.Controls;
using MUXTestInfra.Shared.Infra;

namespace Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests
{
    [TestClass]
    public class ChartsTests
    {
        private const string InitialStatus = "Points=4 Series=3 Legend=On Theme=Light";

        [ClassInitialize]
        [TestProperty("RunAs", "User")]
        [TestProperty("Classification", "Integration")]
        [TestProperty("Platform", "Any")]
        [TestProperty("MUXControlsTestSuite", "SuiteB")]
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
        public void ChartIsExposedToUIAutomation()
        {
            using (var setup = new TestSetupHelper("Charts Tests"))
            {
                Verify.IsNotNull(FindElement.ByName("TestChart"), "The chart should be in the UI Automation tree under its AutomationProperties.Name.");
                Verify.AreEqual(InitialStatus, GetStatus());
            }
        }

        [TestMethod]
        public void ChartKeepsWorkingThroughDataSeriesLegendAndThemeChanges()
        {
            // Each button changes the chart and then rewrites StatusText, so the status only moves on if the
            // change did not throw. The test app does not handle exceptions, so a failure ends the app and
            // the next lookup fails.
            using (var setup = new TestSetupHelper("Charts Tests"))
            {
                InvokeAndVerify("AddPointButton", "Points=5 Series=3 Legend=On Theme=Light");
                InvokeAndVerify("AddPointButton", "Points=6 Series=3 Legend=On Theme=Light");
                InvokeAndVerify("ToggleLegendButton", "Points=6 Series=3 Legend=Off Theme=Light");
                InvokeAndVerify("ToggleThemeButton", "Points=6 Series=3 Legend=Off Theme=Dark");
                InvokeAndVerify("RemoveSeriesButton", "Points=6 Series=2 Legend=Off Theme=Dark");
                InvokeAndVerify("RemoveSeriesButton", "Points=6 Series=1 Legend=Off Theme=Dark");
                InvokeAndVerify("RemoveSeriesButton", "Points=6 Series=0 Legend=Off Theme=Dark");

                // Data changes on a chart without series must not fail either.
                InvokeAndVerify("AddPointButton", "Points=7 Series=0 Legend=Off Theme=Dark");
            }
        }

        private static void InvokeAndVerify(string automationId, string expectedStatus)
        {
            Log.Comment($"Invoking {automationId}");
            new Button(FindElement.ById(automationId)).Invoke();
            Wait.ForIdle();
            Verify.AreEqual(expectedStatus, GetStatus(), $"Status after {automationId}");
            Verify.IsNotNull(FindElement.ByName("TestChart"), $"The chart should still be in the UI Automation tree after {automationId}.");
        }

        private static string GetStatus()
        {
            return new TextBlock(FindElement.ById("StatusText")).DocumentText;
        }
    }
}
