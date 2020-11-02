// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

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

using Microsoft.Windows.Apps.Test.Automation;
using Microsoft.Windows.Apps.Test.Foundation;
using Microsoft.Windows.Apps.Test.Foundation.Controls;
using Microsoft.Windows.Apps.Test.Foundation.Patterns;
using Microsoft.Windows.Apps.Test.Foundation.Waiters;
using Point = System.Drawing.Point;

namespace Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Common
{
    public enum Direction
    {
        North,
        NorthEast,
        East,
        SouthEast,
        South,
        SouthWest,
        West,
        NorthWest
    }

    // Helper class to abstract some of Microsoft.Windows.Apps.Test's input methods.
    public static class InputHelper
    {
        public const uint DefaultPanHoldDuration = 100;
        public const float DefaultPanAcceleration = 0.1f;
        public const uint DefaultDragDuration = 100;

        private enum InputEvent
        {
            LeftClick,
            RightClick,
            Tap,
            Pan,
            Flick,
            Pinch,
            Stretch,
            Drag,
            MouseDown,
            MouseUp,
            MouseMove,
        }

        public static void LeftClick(UIObject obj)
        {
            Log.Comment("Left-click on {0}.", obj.GetIdentifier());
            using (var waiter = GetWaiterForInputEvent(obj, InputEvent.LeftClick))
            {
                obj.Click(PointerButtons.Primary);
            }
            Wait.ForIdle();
        }

        public static void LeftClick(UIObject obj, double offsetX, double offsetY)
        {
            Log.Comment("Left-click on {0}, at ({1}, {2}).", obj.GetIdentifier(), offsetX, offsetY);
            using (var waiter = GetWaiterForInputEvent(obj, InputEvent.LeftClick))
            {
                obj.Click(PointerButtons.Primary, offsetX, offsetY);
            }
            Wait.ForIdle();
        }

        public static void RightClick(UIObject obj, int offsetX = 0, int offsetY = 0)
        {
            if (offsetX == 0 && offsetY == 0)
            {
                Log.Comment("Right-click on {0}.", obj.GetIdentifier());
            }
            else
            {
                Log.Comment("Right-click on {0} at offset ({1}, {2}).", obj.GetIdentifier(), offsetX, offsetY);
            }

            using (var waiter = GetWaiterForInputEvent(obj, InputEvent.RightClick))
            {
                obj.Click(PointerButtons.Secondary, offsetX, offsetY);
            }

            Wait.ForIdle();
        }

        public static void RotateWheel(UIObject obj, int mouseWheelDelta)
        {
            Log.Comment("RotateWheel on {0} with mouseWheelDelta: {1}.", obj.GetIdentifier(), mouseWheelDelta);
            MouseWheelInput.RotateWheel(obj, mouseWheelDelta);
            Wait.ForIdle();
        }

        public static void LeftMouseButtonDown(UIObject obj, int offsetX = 0, int offsetY = 0)
        {
            if (offsetX == 0 && offsetY == 0)
            {
                Log.Comment("Left mouse button down on {0}.", obj.GetIdentifier());
            }
            else
            {
                Log.Comment("Left mouse button down on {0} at offset ({1}, {2}).", 
                    obj.GetIdentifier(), offsetX, offsetY);
            }

            using (var waiter = GetWaiterForInputEvent(obj, InputEvent.MouseDown))
            {
                var point = obj.GetClickablePoint();

                point.X += offsetX;
                point.Y += offsetY;

                PointerInput.Move(point);
                PointerInput.Press(PointerButtons.Primary);
            }
            Wait.ForIdle();
        }

        public static void LeftMouseButtonUp(UIObject obj, int offsetX = 0, int offsetY = 0)
        {
            if (offsetX == 0 && offsetY == 0)
            {
                Log.Comment("Left mouse button up on {0}.", obj.GetIdentifier());
            }
            else
            {
                Log.Comment("Left mouse button up on {0} at offset ({1}, {2}).",
                    obj.GetIdentifier(), offsetX, offsetY);
            }

            using (var waiter = GetWaiterForInputEvent(obj, InputEvent.MouseUp))
            {
                var point = obj.GetClickablePoint();

                point.X += offsetX;
                point.Y += offsetY;

                PointerInput.Move(point);
                PointerInput.Release(PointerButtons.Primary);
            }
            Wait.ForIdle();
        }

        public static void MoveMouse(UIObject obj, int offsetX, int offsetY)
        {
            Log.Comment("Move mouse on {0} to offset ({1}, {2}).", obj.GetIdentifier(), offsetX, offsetY);
            using (var waiter = GetWaiterForInputEvent(obj, InputEvent.MouseMove))
            {
                var point = obj.GetClickablePoint();

                point.X += offsetX;
                point.Y += offsetY;

                PointerInput.Move(point);
            }
            Wait.ForIdle();
        }

        public static void TapAndHold(UIObject obj, double offsetX, double offsetY, uint durationMs)
        {
            Log.Comment("Tap and hold on {0} at ({1}, {2}) for {3} ms.", obj.GetIdentifier(), offsetX, offsetY, durationMs);
            using (var waiter = GetWaiterForInputEvent(obj, InputEvent.Tap))
            {
                if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone5))
                {
                    Log.Warning("Touch input is not available on OS versions less than RS5. Falling back to mouse input.");
                    PointerInput.Move(obj, offsetX, offsetY);
                    PointerInput.Press(PointerButtons.Primary);
                    Wait.ForMilliseconds(durationMs);
                    PointerInput.Release(PointerButtons.Primary);
                }
                else
                {
                    obj.TapAndHold(offsetX, offsetY, durationMs);
                }
            }
            Wait.ForIdle();
        }

        public static void Tap(UIObject obj)
        {
            // Allowing three attempts to work around occasional failure on Phone builds.
            int retries = 3;
            while (retries > 0)
            {
                try
                {
                    Log.Comment("Tap on {0}.", obj.GetIdentifier());
                    using (var waiter = GetWaiterForInputEvent(obj, InputEvent.Tap))
                    {
                        if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone5))
                        {
                            Log.Warning("Touch input is not available on OS versions less than RS5. Falling back to mouse input.");
                            obj.Click(PointerButtons.Primary);
                        }
                        else
                        {
                            obj.Tap();
                        }
                    }

                    retries = 0;
                }
                catch (Exception e)
                {
                    Log.Warning("Exception while tapping: " + e.Message);
                    retries--;
                }
            }

            Wait.ForIdle();
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="obj"></param>
        /// <param name="offsetX">value from top left</param>
        /// <param name="offsetY">value from top left</param>
        public static void Tap(UIObject obj, double offsetX, double offsetY)
        {
            // Allowing three attempts to work around occasional failure on Phone builds.
            int retries = 3;
            while (retries > 0)
            {
                try
                {
                    Log.Comment("Tap on {0} at ({1}, {2}).", obj.GetIdentifier(), offsetX, offsetY);
                    Log.Comment("Bounding rectangle is: " + obj.BoundingRectangle.ToString());

                    if (offsetX > obj.BoundingRectangle.Width)
                    {
                        Log.Warning("Warning, the tap offsetX is higher than the bounding rectangle width");
                    }

                    if (offsetY > obj.BoundingRectangle.Height)
                    {
                        Log.Warning("Warning, the tap offsetY is higher than the bounding rectangle height");
                    }

                    using (var waiter = GetWaiterForInputEvent(obj, InputEvent.Tap))
                    {
                        if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone5))
                        {
                            Log.Warning("Touch input is not available on OS versions less than RS5. Falling back to mouse input.");
                            obj.Click(PointerButtons.Primary, offsetX, offsetY);
                        }
                        else
                        {
                            obj.Tap(offsetX, offsetY);
                        }
                    }

                    retries = 0;
                }
                catch (Exception e)
                {
                    Log.Warning("Exception while tapping: " + e.Message);
                    retries--;
                }
            }

            Wait.ForIdle();
        }

        public static void TapAndHold(UIObject obj, uint durationMs)
        {
            Log.Comment("Tap and hold on {0} for {1} ms.", obj.GetIdentifier(), durationMs);
            using (var waiter = GetWaiterForInputEvent(obj, InputEvent.Tap))
            {
                if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone5))
                {
                    Log.Warning("Touch input is not available on OS versions less than RS5. Falling back to mouse input.");
                    PointerInput.Move(obj);
                    PointerInput.Press(PointerButtons.Primary);
                    Wait.ForMilliseconds(durationMs);
                    PointerInput.Release(PointerButtons.Primary);
                }
                else
                {
                    obj.TapAndHold(durationMs);
                }
            }
            Wait.ForIdle();
        }

        public static void Pan(UIObject obj, int distance, Direction direction, uint holdDuration = DefaultPanHoldDuration, float panAcceleration = DefaultPanAcceleration, uint dragDuration = DefaultDragDuration, bool waitForIdle = true)
        {
            Log.Comment("Pan on {0} for {1} pixels in the {2} direction, after holding {3} ms.", 
                obj.GetIdentifier(), distance, direction, holdDuration);
            Log.Comment("Clickable point of pan object is {0}", obj.GetClickablePoint());
            Log.Comment("Bounding rectangle of object is {0}", obj.BoundingRectangle);

            Pan(obj, obj.GetClickablePoint(), distance, direction, holdDuration, panAcceleration, dragDuration, waitForIdle);
        }

        public static void Pan(UIObject obj, Point start, int distance, Direction direction, uint holdDuration = DefaultPanHoldDuration, float panAcceleration = DefaultPanAcceleration, uint dragDuration = DefaultDragDuration, bool waitForIdle = true)
        {
            Log.Comment("Pan on {0}, starting at ({1}, {2}), for {3} pixels in the {4} direction, after holding {5} ms.",
                obj.GetIdentifier(), start.X, start.Y, distance, direction, holdDuration);
            double directionRadians = DirectionToAngle(direction) * Math.PI / 180d;
            Point end = new Point()
            {
                X = start.X + (int)Math.Round(distance * Math.Cos(directionRadians)),
                Y = start.Y - (int)Math.Round(distance * Math.Sin(directionRadians))
            };

            Pan(obj, start, end, holdDuration, panAcceleration, dragDuration, waitForIdle);
        }

        public static void Pan(UIObject obj, Point start, Point end, uint holdDuration = DefaultPanHoldDuration, float panAcceleration = DefaultPanAcceleration, uint dragDuration = DefaultDragDuration, bool waitForIdle = true)
        {
            Log.Comment("Pan on {0}, starting at ({1}, {2}), ending at ({3}, {4}), after holding {5} ms, with acceleration {6}.",
                obj.GetIdentifier(), start.X, start.Y, end.X, end.Y, holdDuration, panAcceleration);

            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone5))
            {
                using (var waiter = GetWaiterForInputEvent(obj, InputEvent.Drag))
                {
                    Log.Warning("Touch input is not available on OS versions less than RS5. Falling back to mouse input.");
                    PointerInput.Move(start);
                    PointerInput.ClickDrag(end, PointerButtons.Primary, dragDuration);
                }
            }
            else
            {
                if (panAcceleration == 0.0f)
                {
                    using (var waiter = GetWaiterForInputEvent(obj, InputEvent.Drag))
                    {
                        SinglePointGesture.Current.Move(start);
                        SinglePointGesture.Current.PressAndDrag(end, dragDuration, holdDuration);
                    }
                }
                else
                {
                    using (var waiter = GetWaiterForInputEvent(obj, InputEvent.Pan))
                    {
                        SinglePointGesture.Current.Move(start);
                        SinglePointGesture.Current.Pan(end, holdDuration, panAcceleration);
                    }
                }
            }

            if (waitForIdle)
            {
                Wait.ForIdle();
            }
        }
        
        public static void DragToTarget(UIObject obj, UIObject obj2)
        {
            Point startPoint = obj.GetClickablePoint();
            Log.Comment("Start Point X:{0} Y:{1}", startPoint.X, startPoint.Y);

            Point end = obj2.GetClickablePoint();
            Log.Comment("End Point X:{0} Y:{1}", end.X, end.Y);

            uint dragDuration = 2000;

            using (var waiter = GetWaiterForInputEvent(obj, InputEvent.Drag))
            {
                if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone5))
                {
                    Log.Warning("Touch input is not available on OS versions less than RS5. Falling back to mouse input.");
                    Log.Comment("Move input to Start Point.");
                    PointerInput.Move(startPoint);
                    Log.Comment("Begin Drag.");
                    PointerInput.ClickDrag(end, PointerButtons.Primary, dragDuration);
                    Log.Comment("Drag Complete.");
                }
                else
                {
                    Log.Comment("Move input to Start Point.");
                    SinglePointGesture.Current.Move(startPoint);
                    Log.Comment("Begin Drag.");
                    SinglePointGesture.Current.PressAndDrag(end, dragDuration, 500);
                    Log.Comment("Drag Complete.");
                }
            }
            Wait.ForIdle();
        }

        public static void DragDistance(UIObject obj, int distance, Direction direction, uint duration = 4000)
        {
            Log.Comment("Drag {0} {1} pixels in the {2} direction.", obj.GetIdentifier(), distance, direction);
            Point startPoint = obj.GetClickablePoint();
            double directionRadians = DirectionToAngle(direction) * Math.PI / 180d;

            Log.Comment("Start Point X:{0} Y:{1}", startPoint.X, startPoint.Y);

            Point end = new Point()
            {
                X = startPoint.X + (int)Math.Round(distance * Math.Cos(directionRadians)),
                Y = startPoint.Y - (int)Math.Round(distance * Math.Sin(directionRadians))
            };
            Log.Comment("End Point X:{0} Y:{1}", end.X, end.Y);

            using (var waiter = GetWaiterForInputEvent(obj, InputEvent.Drag))
            {
                if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone5))
                {
                    Log.Warning("Touch input is not available on OS versions less than RS5. Falling back to mouse input.");
                    Log.Comment("Move input to Start Point.");
                    PointerInput.Move(startPoint);
                    Log.Comment("Begin Drag.");
                    PointerInput.ClickDrag(end, PointerButtons.Primary, duration);
                    Log.Comment("Drag Complete.");
                }
                else
                {
                    Log.Comment("Move input to Start Point.");
                    SinglePointGesture.Current.Move(startPoint);
                    Log.Comment("Begin Drag.");
                    SinglePointGesture.Current.ClickDrag(end, PointerButtons.Primary, duration);
                    Log.Comment("Drag Complete.");
                }
            }
            Wait.ForIdle();
        }

        public static void Flick(UIObject obj, int distance, Direction direction)
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone5))
            {
                Log.Error("Touch input is not available on OS versions less than RS5, and there is no mouse fallback for flick. This test should be disabled pre-RS5.");
                return;
            }

            Log.Comment("Flick on {0} over {1} pixels in the {2} direction.", obj.GetIdentifier(), distance, direction);
            using (var waiter = GetWaiterForInputEvent(obj, InputEvent.Flick))
            {
                obj.Flick(distance, DirectionToAngle(direction));
            }
            Wait.ForIdle();
        }

        public static void Pinch(UIObject obj)
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone5))
            {
                Log.Error("Touch input is not available on OS versions less than RS5, and there is no mouse fallback for pinch. This test should be disabled pre-RS5.");
                return;
            }

            Log.Comment("Pinch on {0}.", obj.GetIdentifier());
            using (var waiter = GetWaiterForInputEvent(obj, InputEvent.Pinch))
            {
                MultiPointGesture.Pinch(obj);
            }
            Wait.ForIdle();
        }

        public static void Pinch(UIObject obj, uint distance, Direction direction, bool pivot)
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone5))
            {
                Log.Error("Touch input is not available on OS versions less than RS5, and there is no mouse fallback for pinch. This test should be disabled pre-RS5.");
                return;
            }

            string id = obj.GetIdentifier();
            string pivotMode;
            if (pivot)
            {
                pivotMode = "one finger stationary";
            }
            else
            {
                pivotMode = "both fingers moving";
            }
            Log.Comment("Pinch with {0} on {1}, in the {2} direction, over a distance of {3} pixels.",
                pivotMode, id, direction, distance);

            using (var waiter = GetWaiterForInputEvent(obj, InputEvent.Pinch))
            {
                MultiPointGesture.Pinch(obj, distance, DirectionToAngle(direction), pivot);
            }
            Wait.ForIdle();
        }

        public static void Stretch(UIObject obj)
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone5))
            {
                Log.Error("Touch input is not available on OS versions less than RS5, and there is no mouse fallback for stretch. This test should be disabled pre-RS5.");
                return;
            }

            Log.Comment("Stretch on {0}.", obj.GetIdentifier());
            using (var waiter = GetWaiterForInputEvent(obj, InputEvent.Pinch))
            {
                MultiPointGesture.Stretch(obj);
            }
            Wait.ForIdle();
        }

        public static void Stretch(UIObject obj, uint distance, Direction direction, bool pivot)
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone5))
            {
                Log.Error("Touch input is not available on OS versions less than RS5, and there is no mouse fallback for stretch. This test should be disabled pre-RS5.");
                return;
            }

            string id = obj.GetIdentifier();
            string pivotMode;
            if (pivot)
            {
                pivotMode = "one finger stationary";
            }
            else
            {
                pivotMode = "both fingers moving";
            }
            Log.Comment("Stretch with {0} on {1}, in the {2} direction, over a distance of {3} pixels.",
                pivotMode, id, direction, distance);

            using (var waiter = GetWaiterForInputEvent(obj, InputEvent.Pinch))
            {
                MultiPointGesture.Stretch(obj, distance, DirectionToAngle(direction), pivot);
            }
            Wait.ForIdle();
        }

        // Microsoft.Windows.Apps.Test's Pan input methods use angles to determine direction
        // with 0 corresponding to the positive x-axis and proceeding
        // counter-clockwise (up is 90, left is 180, and down is 270).
        private static float DirectionToAngle(Direction direction)
        {
            float angle = 0f;

            switch (direction)
            {
                default:
                case Direction.North:
                    angle = 90f;
                    break;
                case Direction.NorthEast:
                    angle = 45f;
                    break;
                case Direction.East:
                    angle = 0f;
                    break;
                case Direction.SouthEast:
                    angle = 315f;
                    break;
                case Direction.South:
                    angle = 270f;
                    break;
                case Direction.SouthWest:
                    angle = 225f;
                    break;
                case Direction.West:
                    angle = 180f;
                    break;
                case Direction.NorthWest:
                    angle = 135f;
                    break;
            }

            return angle;
        }

        private static Waiter GetWaiterForInputEvent(UIObject obj, InputEvent inputEvent)
        {
            Waiter waiter = null;

            if (obj is Button)
            {
                var button = obj as Button;
                if (inputEvent == InputEvent.LeftClick || inputEvent == InputEvent.Tap)
                {
                    waiter = button.GetInvokedWaiter();
                }
            }
            else if (obj is ToggleButton)
            {
                var toggleButton = obj as ToggleButton;
                if (inputEvent == InputEvent.LeftClick || inputEvent == InputEvent.Tap)
                {
                    waiter = toggleButton.GetToggledWaiter();
                }
            }
            else if (obj is ComboBox)
            {
                var comboBox = obj as ComboBox;
                if (comboBox.ExpandCollapseState == ExpandCollapseState.Collapsed &&
                    (inputEvent == InputEvent.LeftClick || inputEvent == InputEvent.Tap))
                {
                    waiter = comboBox.GetExpandedWaiter();
                }
            }
            #if COLORPICKER_INCLUDED
            else if (obj is ColorSpectrum)
            {
                var colorSpectrum = obj as ColorSpectrum;
                waiter = colorSpectrum.GetColorChangedWaiter();
            }
            else if (obj is ColorPickerSlider)
            {
                var colorPickerSlider = obj as ColorPickerSlider;
                waiter = colorPickerSlider.GetColorChangedWaiter();
            }
            #endif
            
            return waiter;
        }

        // Tests should scroll to their bottom most element to ensure the entire 
        // test UI is onscreen otherwise the fake input injection will fail. 
        //
        // Potentially has side effects depending on if a scroll item implementation can be retrieved
        public static void ScrollToElement(UIObject uiObject)
        {
            Log.Comment("ScrollToElement: " + uiObject.ToString());
            bool successfullyScrolled = false;
            ScrollItemImplementation scrollItem = new ScrollItemImplementation(uiObject);
            if (scrollItem.IsAvailable)
            {
                Log.Comment("Using ScrollItemImplementation to scroll");
                scrollItem.ScrollIntoView();
                successfullyScrolled = true;
            }
            
            if (!successfullyScrolled)
            {
                Log.Comment("Using ScrollItem UIA pattern to scroll");
                var previouslyFocusedElement = AutomationElement.FocusedElement;
                uiObject.SetFocus();
                Wait.ForIdle();
                var newFocusedElement = AutomationElement.FocusedElement;
                object scrollItemPatternAsObject;
                newFocusedElement.TryGetCurrentPattern(ScrollItemPattern.Pattern, out scrollItemPatternAsObject);
                if (scrollItemPatternAsObject != null)
                {
                    var scrollItemPattern = (ScrollItemPattern)scrollItemPatternAsObject;
                    scrollItemPattern.ScrollIntoView();
                }

                if (previouslyFocusedElement != null)
                {
                    previouslyFocusedElement.SetFocus();
                }
            }
            Wait.ForIdle();
        }
    }
}
