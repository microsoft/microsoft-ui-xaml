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
            using (var setup = new TestSetupHelper("InkCanvas"))
            {
                var inkCanvas = FindElement.ByName("TestInkCanvas");
                Verify.IsNotNull(inkCanvas, "InkCanvas should be present in the visual tree.");

                // The canvas renders through a composition visual; its automation peer must still
                // report real, on-screen bounds so assistive technology does not skip it.
                var bounds = inkCanvas.BoundingRectangle;
                Log.Comment($"InkCanvas bounds: {bounds.Width}x{bounds.Height} at {bounds.X},{bounds.Y}");
                Verify.IsGreaterThan(bounds.Width, 0, "InkCanvas should report a non-zero width.");
                Verify.IsGreaterThan(bounds.Height, 0, "InkCanvas should report a non-zero height.");
            }
        }
    }
}
