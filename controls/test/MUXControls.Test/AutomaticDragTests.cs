// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Common;
using System;
using System.Globalization;
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
        public void ListViewMouseCanceledDragCanRestart()
        {
            VerifyListViewCanceledDragCanRestart(PointerInputType.Mouse);
        }

        [TestMethod]
        public void ListViewTouchCanceledDragCanRestart()
        {
            VerifyListViewCanceledDragCanRestart(PointerInputType.SingleTouch);
        }

        private static void VerifyListViewCanceledDragCanRestart(PointerInputType inputType)
        {
            using var controller = InputController.Activate(inputType);
            using var setup = new TestSetupHelper(new[] { "Automatic Drag Tests" });
            Log.Comment("Switching the fixture to a realized ListView source.");
            new CheckBox(FindElement.ById("UseListView")).Check();
            Wait.ForIdle();
            Verify.AreEqual("Visible", ReadText("ListMode"));
            var source = FindElement.ById("ListDragItem");
            var target = FindElement.ById("DropHandle");
            Verify.IsNotNull(source, "ListView source must be realized after changing visibility.");
            Verify.IsNotNull(target);
            for (int attempt = 1; attempt <= 2; ++attempt)
            {
                if (inputType == PointerInputType.Mouse)
                {
                    InputHelper.MouseDragToTarget(source, target);
                }
                else
                {
                    InputHelper.Pan(source, source.GetClickablePoint(), target.GetClickablePoint(),
                        holdDuration: 1000, panAcceleration: 0, dragDuration: 500);
                }
                Verify.AreEqual(inputType == PointerInputType.Mouse ? "Mouse" : "Touch", ReadText("LastPointerDevice"));
                Verify.AreEqual(attempt.ToString(), ReadText("DragCount"));
                new Button(FindElement.ById("Snapshot")).Invoke();
                Verify.AreEqual("0", ReadText("ListCaptureCount"));
            }
        }

        [TestMethod]
        public void SamePenHoverAfterHandledReleaseDoesNotDrag()
        {
            using var setup = new TestSetupHelper(new[] { "Automatic Drag Tests" });
            using var pen = new SyntheticPen();
            var start = FindElement.ById("DragHandle").GetClickablePoint();
            pen.Hover(start);
            pen.Press(start);
            pen.Release();
            int hoverCount = int.Parse(ReadText("PenHoverCount"));
            pen.Hover(new System.Drawing.Point(start.X + 40, start.Y));
            Verify.IsTrue(int.Parse(ReadText("PenHoverCount")) > hoverCount);
            Verify.AreEqual(ReadText("PenPressPointerId"), ReadText("PenHoverPointerId"));
            Verify.AreEqual("1", ReadText("ReleaseCount"));
            Verify.AreEqual("0", ReadText("DragCount"));
            VerifyCaptureReleased();
            pen.Press(start);
            pen.Move(new System.Drawing.Point(start.X + 40, start.Y));
            pen.Release();
            Verify.AreEqual("1", ReadText("DragCount"));
        }

        [TestMethod]
        public void PenHoverDuringMousePressPreservesMouseDrag()
        {
            using var controller = InputController.Activate(PointerInputType.Mouse);
            using var setup = new TestSetupHelper(new[] { "Automatic Drag Tests" });
            using var pen = new SyntheticPen();
            var mouse = PointerInput.Current;
            var start = FindElement.ById("DragHandle").GetClickablePoint();
            mouse.Move(start);
            mouse.Press(PointerButtons.Primary);
            try
            {
                pen.Hover(new System.Drawing.Point(start.X + 40, start.Y));
                Verify.IsTrue(int.Parse(ReadText("PenHoverCount")) > 0);
                Verify.AreNotEqual(ReadText("MousePointerId"), ReadText("PenHoverPointerId"));
                Verify.AreEqual("0", ReadText("DragCount"));
                new Button(FindElement.ById("Snapshot")).Invoke();
                Verify.AreEqual("1", ReadText("MouseCaptureCount"));
                mouse.Move(new System.Drawing.Point(start.X + 60, start.Y));
                Wait.ForIdle();
                Verify.AreEqual("1", ReadText("DragCount"));
            }
            finally
            {
                mouse.Release(PointerButtons.Primary);
            }
            VerifyCaptureReleased();
        }

        [TestMethod]
        public void PenCanDragToggleDuringContactRequiresFreshPress()
        {
            using var setup = new TestSetupHelper(new[] { "Automatic Drag Tests" });
            using var pen = new SyntheticPen();
            var start = FindElement.ById("DragHandle").GetClickablePoint();
            pen.Hover(start);
            pen.Press(start);
            new Button(FindElement.ById("ToggleDragDuringPress")).Invoke();
            pen.Move(new System.Drawing.Point(start.X + 40, start.Y));
            Verify.AreEqual("0", ReadText("DragCount"));
            pen.Release();
            VerifyCaptureReleased();
            pen.Press(start);
            pen.Move(new System.Drawing.Point(start.X + 40, start.Y));
            pen.Release();
            Verify.AreEqual("1", ReadText("DragCount"));
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
                var center = new System.Drawing.Point(
                    (int)(bounds.Left + bounds.Width / 2), (int)(bounds.Top + bounds.Height / 2));
                PointerInput.Move(center);
                // Pen injection supports whole gestures but not separate press/release.
                PointerInput.ClickDrag(center, PointerButtons.Primary, 100);
                Wait.ForIdle();
                Verify.AreEqual(inputType.ToString(), ReadText("LastPointerDevice"));
                Verify.AreEqual(attempt.ToString(), ReadText("ReleaseCount"));

                int hoverMoves = int.Parse(ReadText("HoverMoveCount"));
                // This legacy-wrapper guard uses mouse hover; SyntheticPen tests cover same-pen hover.
                using (InputController.Activate(PointerInputType.Mouse))
                {
                    foreach (double fraction in new[] { 0.7, 0.3 })
                    {
                        PointerInput.Move(new System.Drawing.Point(
                            (int)(bounds.Left + bounds.Width * fraction), (int)(bounds.Top + bounds.Height / 2)));
                        Wait.ForIdle();
                    }
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
                    // Touch injection supports whole gestures but not separate press/release.
                    InputHelper.Tap(FindElement.ById("TouchHandle"));
                }

                new Button(FindElement.ById("Snapshot")).Invoke();
                Wait.ForIdle();
                Verify.AreEqual("1", ReadText("ChildTouchPressCount"));
                Verify.AreNotEqual(mousePointerId, ReadText("ChildTouchPointerId"));
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
        public void PenMoveDuringMousePressPreservesMouseDrag()
        {
            using var mouseController = InputController.Activate(PointerInputType.Mouse);
            using var setup = new TestSetupHelper(new[] { "Automatic Drag Tests" });
            var mouse = PointerInput.Current;
            var source = FindElement.ById("DragHandle");
            var target = FindElement.ById("DropHandle");
            var bounds = source.BoundingRectangle;
            mouse.Move(source.GetClickablePoint());
            mouse.Press(PointerButtons.Primary);
            try
            {
                Wait.ForIdle();
                new Button(FindElement.ById("Snapshot")).Invoke();
                Wait.ForIdle();
                Verify.AreEqual("1", ReadText("MouseCaptureCount"));

                // While the mouse press is tracked, a pen moves over the element far beyond the
                // drag threshold from the mouse press point. Its moves must not end the mouse
                // gesture or start a drag. Pen injection supports whole gestures only.
                using (InputController.Activate(PointerInputType.Pen))
                {
                    int y = (int)(bounds.Top + bounds.Height / 2);
                    PointerInput.Move(new System.Drawing.Point((int)bounds.Left + 10, y));
                    PointerInput.ClickDrag(new System.Drawing.Point((int)bounds.Left + 60, y), PointerButtons.Primary, 1000);
                }
                Wait.ForIdle();

                new Button(FindElement.ById("Snapshot")).Invoke();
                Wait.ForIdle();
                Verify.AreEqual("Pen", ReadText("LastPointerDevice"));
                Verify.IsTrue(int.Parse(ReadText("PenMoveCount")) > 0, "A pen move must actually reach the source.");
                Verify.AreNotEqual(ReadText("MousePointerId"), ReadText("PenMovePointerId"));
                Verify.AreEqual("0", ReadText("DragCount"));
                Verify.AreEqual("1", ReadText("MouseCaptureCount"));
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
        public void ChildPenCaptureLossPreservesMouseDrag()
        {
            using var controller = InputController.Activate(PointerInputType.Mouse);
            using var setup = new TestSetupHelper(new[] { "Automatic Drag Tests" });
            new CheckBox(FindElement.ById("CaptureChildPointer")).Check();
            new CheckBox(FindElement.ById("HandleChildCaptureLoss")).Uncheck();
            var mouse = PointerInput.Current;
            var source = FindElement.ById("DragHandle");
            var target = FindElement.ById("DropHandle");
            using var pen = new SyntheticPen();
            mouse.Move(source.GetClickablePoint());
            mouse.Press(PointerButtons.Primary);
            try
            {
                pen.Press(FindElement.ById("TouchHandle").GetClickablePoint());
                Wait.ForIdle();
                Verify.AreEqual("1", ReadText("ChildCaptureCount"));
                Verify.AreEqual("0", ReadText("DragCount"));
                pen.Release();
                // Leaving range supplies the current point to the capture-loss path.
                // Explicit release during touch/pen up can omit that notification.
                pen.LeaveRange();
                Verify.AreEqual("1", ReadText("ChildCaptureLostCount"));
                Verify.AreNotEqual(ReadText("MousePointerId"), ReadText("PenPressPointerId"));
                new Button(FindElement.ById("Snapshot")).Invoke();
                Verify.AreEqual("1", ReadText("MouseCaptureCount"));
                Verify.AreEqual("0", ReadText("DragCount"));
                mouse.Move(target.GetClickablePoint());
                Wait.ForIdle();
                Verify.AreEqual("1", ReadText("DragCount"));
            }
            finally
            {
                mouse.Release(PointerButtons.Primary);
            }
            VerifyCaptureReleased();
            InputHelper.MouseDragToTarget(source, target);
            Verify.AreEqual("2", ReadText("DragCount"));
        }

        [TestMethod]
        public void SecondaryButtonDoesNotResetPrimaryDragOrigin()
        {
            using var controller = InputController.Activate(PointerInputType.Mouse);
            using var setup = new TestSetupHelper(new[] { "Automatic Drag Tests" });
            new CheckBox(FindElement.ById("HandleRelease")).Uncheck();
            new Button(FindElement.ById("Snapshot")).Invoke();
            double threshold = double.Parse(ReadText("MouseDragThreshold"), CultureInfo.InvariantCulture);
            double scale = double.Parse(ReadText("RasterizationScale"), CultureInfo.InvariantCulture);
            Verify.IsTrue(threshold >= 4, "Need space on both sides of the drag threshold.");
            var mouse = PointerInput.Current;
            var start = FindElement.ById("DragHandle").GetClickablePoint();
            var chord = new System.Drawing.Point(start.X + (int)Math.Floor(threshold * scale * 0.75), start.Y);
            var end = new System.Drawing.Point(start.X + (int)Math.Ceiling((threshold + 2) * scale), start.Y);
            mouse.Move(start);
            mouse.Press(PointerButtons.Primary);
            try
            {
                mouse.Move(chord);
                Wait.ForIdle();
                Verify.AreEqual("0", ReadText("DragCount"));
                mouse.Press(PointerButtons.Secondary);
                // Records whether the chord raised PointerPressed (RightButtonPressed) or was
                // reported as a move, in which case this test only guards the primary origin.
                Wait.ForIdle();
                Log.Comment($"Update kind of the last PointerPressed after the chord: {ReadText("PressUpdateKind")}");
                mouse.Release(PointerButtons.Secondary);
                mouse.Move(end);
                Wait.ForIdle();
                Verify.AreEqual("1", ReadText("DragCount"), "Measure from the primary press, not the secondary press.");
            }
            finally
            {
                mouse.Release(PointerButtons.Primary);
            }
            VerifyCaptureReleased();
        }

        [TestMethod]
        public void DisableAndReenableDuringPressRequiresFreshPress()
        {
            using var controller = InputController.Activate(PointerInputType.Mouse);
            using var setup = new TestSetupHelper(new[] { "Automatic Drag Tests" });
            var mouse = PointerInput.Current;
            var source = FindElement.ById("DragHandle");
            var target = FindElement.ById("DropHandle");
            for (int attempt = 0; attempt < 3; ++attempt)
            {
                mouse.Move(source.GetClickablePoint());
                mouse.Press(PointerButtons.Primary);
                try
                {
                    new Button(FindElement.ById("ToggleDragDuringPress")).Invoke();
                    mouse.Move(target.GetClickablePoint());
                    Wait.ForIdle();
                    Verify.AreEqual(attempt.ToString(), ReadText("DragCount"));
                }
                finally
                {
                    mouse.Release(PointerButtons.Primary);
                }
                VerifyCaptureReleased();
                InputHelper.MouseDragToTarget(source, target);
                Verify.AreEqual((attempt + 1).ToString(), ReadText("DragCount"));
                VerifyCaptureReleased();
            }
        }

        [TestMethod]
        public void PenCaptureReleasedDuringPressDoesNotDrag()
        {
            using var controller = InputController.Activate(PointerInputType.Pen);
            using var setup = new TestSetupHelper(new[] { "Automatic Drag Tests" });
            new CheckBox(FindElement.ById("ReleaseOnPress")).Check();
            var source = FindElement.ById("DragHandle");
            var target = FindElement.ById("DropHandle");
            InputHelper.MouseDragToTarget(source, target);
            Verify.AreEqual("Pen", ReadText("LastPointerDevice"));
            Verify.IsTrue(int.Parse(ReadText("PenMoveCount")) > 0);
            Verify.AreEqual("0", ReadText("DragCount"));
            VerifyCaptureReleased();
            new CheckBox(FindElement.ById("ReleaseOnPress")).Uncheck();
            InputHelper.MouseDragToTarget(source, target);
            Verify.AreEqual("1", ReadText("DragCount"));
        }

        [TestMethod]
        public void MouseDropCompletes()
        {
            using var controller = InputController.Activate(PointerInputType.Mouse);
            using var setup = new TestSetupHelper(new[] { "Automatic Drag Tests" });
            new CheckBox(FindElement.ById("CancelDrag")).Uncheck();
            var mouse = PointerInput.Current;
            var start = FindElement.ById("DragHandle").GetClickablePoint();
            var target = FindElement.ById("DropHandle").GetClickablePoint();
            mouse.Move(start);
            mouse.Press(PointerButtons.Primary);
            try
            {
                mouse.Move(new System.Drawing.Point(start.X + 40, start.Y));
                Wait.RetryUntilEvalFuncSuccessOrTimeout(() => ReadText("DragCount") == "1", retryTimoutByMilliseconds: 10000);
                Verify.AreEqual("1", ReadText("DragCount"));
                mouse.Move(target);
                Wait.ForIdle();
                mouse.Move(new System.Drawing.Point(target.X + 1, target.Y));
                Wait.RetryUntilEvalFuncSuccessOrTimeout(() => int.Parse(ReadText("DragOverCount")) > 0, retryTimoutByMilliseconds: 10000);
                Verify.IsTrue(int.Parse(ReadText("DragOverCount")) > 0, "Hold the mouse until the system drag reaches the target.");
            }
            finally
            {
                mouse.Release(PointerButtons.Primary);
            }
            Wait.RetryUntilEvalFuncSuccessOrTimeout(
                () => ReadText("CopyCompletionCount") == "1",
                retryTimoutByMilliseconds: 10000);
            Verify.AreEqual("1", ReadText("DragCount"));
            Verify.AreEqual("1", ReadText("DropCount"));
            Verify.AreEqual("1", ReadText("CopyCompletionCount"));
            Verify.AreEqual("AutomaticDragSource", ReadText("DroppedText"));
            VerifyCaptureReleased();
        }

        [TestMethod]
        public void RepeatedPenDropsComplete()
        {
            using var controller = InputController.Activate(PointerInputType.Pen);
            using var setup = new TestSetupHelper(new[] { "Automatic Drag Tests" });
            new CheckBox(FindElement.ById("CancelDrag")).Uncheck();
            for (int attempt = 1; attempt <= 3; ++attempt)
            {
                InputHelper.MouseDragToTarget(FindElement.ById("DragHandle"), FindElement.ById("DropHandle"));
                Wait.RetryUntilEvalFuncSuccessOrTimeout(
                    () => ReadText("CopyCompletionCount") == attempt.ToString(),
                    retryTimoutByMilliseconds: 10000);
                Verify.AreEqual(attempt.ToString(), ReadText("DragCount"));
                Verify.AreEqual(attempt.ToString(), ReadText("DropCount"));
                Verify.AreEqual(attempt.ToString(), ReadText("CopyCompletionCount"));
                Verify.AreEqual("AutomaticDragSource", ReadText("DroppedText"));
                VerifyCaptureReleased();
            }
        }

        [TestMethod]
        public void PenCanceledDragCanRestartAndDrop()
        {
            VerifyCanceledDragCanRestartAndDrop(PointerInputType.Pen);
        }

        [TestMethod]
        public void MouseCanceledDragCanRestart()
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

            if (inputType != PointerInputType.Pen)
            {
                // InputHelper.MouseDragToTarget's one-shot mouse gesture does not complete a system
                // drop on the test machine. MouseDropCompletes covers mouse drops with staged moves.
                return;
            }

            new CheckBox(FindElement.ById("CancelDrag")).Uncheck();
            Wait.ForIdle();
            for (int completed = 1; completed <= 2; ++completed)
            {
                InputHelper.MouseDragToTarget(source, target);
                Wait.RetryUntilEvalFuncSuccessOrTimeout(
                    () => ReadText("CopyCompletionCount") == completed.ToString(),
                    retryTimoutByMilliseconds: 10000);
                Verify.AreEqual((completed + 2).ToString(), ReadText("DragCount"));
                Verify.AreEqual(completed.ToString(), ReadText("DropCount"));
                Verify.AreEqual(completed.ToString(), ReadText("CopyCompletionCount"));
                Verify.AreEqual("AutomaticDragSource", ReadText("DroppedText"));
                Verify.AreEqual("Copy", ReadText("CompletionResult"));
                VerifyCaptureReleased();
            }

            // Both transfers must complete before checking another canceled restart.
            new CheckBox(FindElement.ById("CancelDrag")).Check();
            Wait.ForIdle();
            InputHelper.MouseDragToTarget(source, target);
            Verify.AreEqual("5", ReadText("DragCount"));
            Verify.AreEqual("2", ReadText("DropCount"));
            VerifyCaptureReleased();
        }

        [TestMethod]
        public void MouseCaptureLostHandlerCanDisableDrag()
        {
            using var inputController = InputController.Activate(PointerInputType.Mouse);
            using var setup = new TestSetupHelper(new[] { "Automatic Drag Tests" });
            var source = FindElement.ById("DragHandle");
            var target = FindElement.ById("DropHandle");

            // Releasing capture at drag start raises PointerCaptureLost synchronously.
            // Clearing CanDrag there removes the helper and must cancel the drag.
            new CheckBox(FindElement.ById("DisableOnCaptureLost")).Check();
            Wait.ForIdle();
            InputHelper.MouseDragToTarget(source, target);
            Verify.AreEqual("1", ReadText("CaptureLossActionCount"));
            Verify.AreEqual("0", ReadText("DragCount"));
            VerifyCaptureReleased();

            // Restoring CanDrag in the same callback keeps the drag.
            new CheckBox(FindElement.ById("ReenableOnCaptureLost")).Check();
            new CheckBox(FindElement.ById("Draggable")).Check();
            Wait.ForIdle();
            InputHelper.MouseDragToTarget(source, target);
            Verify.AreEqual("2", ReadText("CaptureLossActionCount"));
            Verify.AreEqual("1", ReadText("DragCount"));
            VerifyCaptureReleased();
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
