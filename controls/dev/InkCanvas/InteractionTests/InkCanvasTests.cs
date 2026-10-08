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
            using (var setup = new TestSetupHelper("InkCanvas Tests"))
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

        // The OS raises UnprocessedInput on the ink thread and its PointerPoint can't be marshaled, so the
        // args handed to the app on the UI thread must still be readable (custom erasers depend on this).
        [TestMethod]
        public void InkCanvasUnprocessedInputArgsAreReadable()
        {
            using (var setup = new TestSetupHelper("InkCanvas Tests"))
            {
                new Button(FindElement.ById("ListenForUnprocessedInputButton")).Invoke();
                Wait.ForIdle();

                InputHelper.MouseDragDistance(FindElement.ByName("TestInkCanvas"), 100, Direction.East);
                Wait.ForIdle();

                var status = new TextBlock(FindElement.ById("UnprocessedInputStatus")).DocumentText.Split(',');
                Log.Comment($"UnprocessedInput pressed,moved,errors,released = {string.Join(",", status)}");
                Verify.IsGreaterThan(int.Parse(status[0]), 0, "PointerPressed should have been read.");
                Verify.IsGreaterThan(int.Parse(status[1]), 0, "PointerMoved should have been read.");
                Verify.IsGreaterThan(int.Parse(status[3]), 0, "PointerReleased should have been read.");
                Verify.AreEqual(0, int.Parse(status[2]), "Reading CurrentPoint/Properties/GetIntermediatePoints/PointerDevice must not throw.");
            }
        }

        // A collapsed InkCanvas (here via its host) must not ink; input must reach the element beneath it.
        [TestMethod]
        public void CollapsedInkCanvasDoesNotTakeInput()
        {
            using (var setup = new TestSetupHelper("InkCanvas Tests"))
            {
                var underlay = FindElement.ById("CanvasUnderlay");
                Verify.IsNotNull(underlay, "CanvasUnderlay should be present.");

                new Button(FindElement.ById("CollapseCanvasHostButton")).Invoke();
                Wait.ForIdle();

                InputHelper.MouseDragDistance(underlay, 100, Direction.East);
                Wait.ForIdle();

                Verify.AreEqual("0", new TextBlock(FindElement.ById("StrokesCollectedCount")).DocumentText,
                    "A collapsed InkCanvas should not collect strokes.");
                Verify.AreNotEqual("0", new TextBlock(FindElement.ById("UnderlayPointerCount")).DocumentText,
                    "Input should reach the element under a collapsed InkCanvas.");
            }
        }
    }
}
