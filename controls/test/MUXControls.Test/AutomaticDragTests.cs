// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Common;
using Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.Common;
using Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.Infra;
using Microsoft.Windows.Apps.Test.Automation;
using Microsoft.Windows.Apps.Test.Foundation;
using Microsoft.Windows.Apps.Test.Foundation.Controls;
using WEX.Logging.Interop;
using WEX.TestExecution;
using WEX.TestExecution.Markup;

namespace Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests
{
    [TestClass]
    public class AutomaticDragTests
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
        public void PenHandledReleaseDoesNotStartDragOnHover()
        {
            VerifyHandledReleaseDoesNotStartDragOnHover(PointerInputType.Pen);
        }

        [TestMethod]
        public void MouseHandledReleaseDoesNotStartDragOnHover()
        {
            VerifyHandledReleaseDoesNotStartDragOnHover(PointerInputType.Mouse);
        }

        private static void VerifyHandledReleaseDoesNotStartDragOnHover(PointerInputType inputType)
        {
            using var inputController = InputController.Activate(inputType);
            using var setup = new TestSetupHelper(new[] { "Automatic Drag Tests" });
            var source = FindElement.ById("DragHandle");
            var bounds = source.BoundingRectangle;

            for (int attempt = 1; attempt <= 3; ++attempt)
            {
                PointerInput.Move(new System.Drawing.Point(
                    (int)(bounds.Left + bounds.Width / 2), (int)(bounds.Top + bounds.Height / 2)));
                PointerInput.Press(PointerButtons.Primary);
                PointerInput.Release(PointerButtons.Primary);
                Wait.ForIdle();
                Verify.AreEqual(inputType.ToString(), ReadText("LastPointerDevice"));
                Verify.AreEqual(attempt.ToString(), ReadText("ReleaseCount"));

                int hoverMoves = int.Parse(ReadText("HoverMoveCount"));
                foreach (double fraction in new[] { 0.7, 0.3 })
                {
                    PointerInput.Move(new System.Drawing.Point(
                        (int)(bounds.Left + bounds.Width * fraction), (int)(bounds.Top + bounds.Height / 2)));
                    Wait.ForIdle();
                }
                Verify.IsTrue(int.Parse(ReadText("HoverMoveCount")) > hoverMoves);
                Verify.AreEqual("0", ReadText("DragCount"));
                VerifyCaptureReleased();
            }

            new CheckBox(FindElement.ById("HandleRelease")).Uncheck();
            Wait.ForIdle();
            InputHelper.LeftClick(source);
            Verify.AreEqual("4", ReadText("ReleaseCount"));
            VerifyCaptureReleased();
            InputHelper.MouseDragToTarget(source, FindElement.ById("DropHandle"));
            Verify.AreEqual("1", ReadText("DragCount"));
            VerifyCaptureReleased();
        }

        [TestMethod]
        public void HandledChildTouchReleasePreservesMouseDrag()
        {
            using var mouseController = InputController.Activate(PointerInputType.Mouse);
            using var setup = new TestSetupHelper(new[] { "Automatic Drag Tests" });
            var mouse = PointerInput.Current;
            var source = FindElement.ById("DragHandle");
            var target = FindElement.ById("DropHandle");
            mouse.Move(source.GetClickablePoint());
            mouse.Press(PointerButtons.Primary);
            try
            {
                Wait.ForIdle();
                new Button(FindElement.ById("Snapshot")).Invoke();
                Wait.ForIdle();
                Verify.AreEqual("1", ReadText("MouseCaptureCount"));
                string mousePointerId = ReadText("MousePointerId");

                using (InputController.Activate(PointerInputType.SingleTouch))
                {
                    var touch = PointerInput.Current;
                    touch.Move(FindElement.ById("TouchHandle").GetClickablePoint());
                    touch.Press(PointerButtons.Primary);
                    try
                    {
                        Wait.ForIdle();
                        Verify.AreEqual("1", ReadText("ChildTouchPressCount"));
                        Verify.AreNotEqual(mousePointerId, ReadText("ChildTouchPointerId"));
                    }
                    finally
                    {
                        touch.Release(PointerButtons.Primary);
                    }
                    Wait.ForIdle();
                }

                new Button(FindElement.ById("Snapshot")).Invoke();
                Wait.ForIdle();
                Verify.AreEqual("1", ReadText("ChildTouchReleaseCount"));
                Verify.AreEqual("1", ReadText("MouseCaptureCount"));
                Verify.AreEqual("0", ReadText("MouseCaptureLostCount"));
                Verify.AreEqual("0", ReadText("DragCount"));
                mouse.Move(target.GetClickablePoint());
                Wait.ForIdle();
            }
            finally
            {
                mouse.Release(PointerButtons.Primary);
            }

            Wait.ForIdle();
            Verify.AreEqual("1", ReadText("DragCount"));
            VerifyCaptureReleased();
            InputHelper.MouseDragToTarget(source, target);
            Verify.AreEqual("2", ReadText("DragCount"));
            VerifyCaptureReleased();
        }

        [TestMethod]
        public void PenCanceledDragCanRestartAndDrop()
        {
            VerifyCanceledDragCanRestartAndDrop(PointerInputType.Pen);
        }

        [TestMethod]
        public void MouseCanceledDragCanRestartAndDrop()
        {
            VerifyCanceledDragCanRestartAndDrop(PointerInputType.Mouse);
        }

        private static void VerifyCanceledDragCanRestartAndDrop(PointerInputType inputType)
        {
            using var inputController = InputController.Activate(inputType);
            using var setup = new TestSetupHelper(new[] { "Automatic Drag Tests" });
            var source = FindElement.ById("DragHandle");
            var target = FindElement.ById("DropHandle");

            for (int attempt = 1; attempt <= 2; ++attempt)
            {
                InputHelper.MouseDragToTarget(source, target);
                Verify.AreEqual(inputType.ToString(), ReadText("LastPointerDevice"));
                Verify.AreEqual(attempt.ToString(), ReadText("DragCount"));
                Verify.AreEqual("0", ReadText("DropCount"));
                VerifyCaptureReleased();
            }

            new CheckBox(FindElement.ById("CancelDrag")).Uncheck();
            Wait.ForIdle();
            for (int attempt = 1; attempt <= 2; ++attempt)
            {
                InputHelper.MouseDragToTarget(source, target);
                Wait.RetryUntilEvalFuncSuccessOrTimeout(
                    () => ReadText("CopyCompletionCount") == attempt.ToString(),
                    retryTimoutByMilliseconds: 3000);
                Verify.AreEqual((attempt + 2).ToString(), ReadText("DragCount"));
                Verify.AreEqual(attempt.ToString(), ReadText("DropCount"));
                Verify.AreEqual(attempt.ToString(), ReadText("CopyCompletionCount"));
                Verify.AreEqual("AutomaticDragSource", ReadText("DroppedText"));
                Verify.AreEqual("Copy", ReadText("CompletionResult"));
                VerifyCaptureReleased();
            }
        }

        private static string ReadText(string id)
        {
            return new TextBlock(FindElement.ById(id)).GetText();
        }

        private static void VerifyCaptureReleased()
        {
            new Button(FindElement.ById("Snapshot")).Invoke();
            Wait.ForIdle();
            Verify.AreEqual("0", ReadText("CaptureCount"));
        }
    }
}
