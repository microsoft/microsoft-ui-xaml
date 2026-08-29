// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Linq;
using System.Globalization;
using System.Threading;
using System.Threading.Tasks;
using Windows.Foundation;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Media;

using Private.Infrastructure;
using WEX.TestExecution;
using Microsoft.UI.Xaml.Tests.Common.EventsListeners;


namespace Microsoft.UI.Xaml.Tests.Common
{
    /// <summary>
    /// Provides 'wait' helpers for IActionQueue.
    /// </summary>
    public static class ActionQueueWaitExtensions
    {
        #region WaitForEvent
        /// <summary>
        /// Waits for an event to be raised.
        /// </summary>
        public static IActionQueue WaitForEvent<TSender>(this IActionQueue queue, TSender sender, EventListener<TSender> listener, TimeSpan timeout)
        {
            var resetEvent = new AutoResetEvent(false);
            Action resetEventAction = () => { resetEvent.Set(); };

            if (sender == null)
            {
                throw new ArgumentNullException("sender", "You cannot subscribe to an event on a null reference. Make sure you call Commit before you call WaitForEvent.");
            }

            queue.Dispatcher.Invoke(() =>
            {
                listener.AddActionHandler(sender, resetEventAction);
            });

            queue.EnqueueAction(() =>
            {
                if (!resetEvent.WaitOne(timeout))
                {
                    Verify.Fail(string.Format(CultureInfo.InvariantCulture, "Event '{0}' was not raised before timeout.", listener.GetType().Name));
                }

                queue.Dispatcher.Invoke(() =>
                {
                    listener.RemoveActionHandler(sender, resetEventAction);
                });
            });

            return queue;
        }

        /// <summary>
        /// Waits for an event to be raised.
        /// </summary>
        public static IActionQueue WaitForEvent<TSender>(this IActionQueue queue, TSender sender, EventListener<TSender> listener)
        {
            return WaitForEvent(queue, sender, listener, TimeSpan.FromSeconds(5));
        }
        #endregion

        #region WaitForHandle
        /// <summary>
        /// Waits for the given handle to be signaled.
        /// </summary>
        public static IActionQueue WaitForHandle(this IActionQueue queue, WaitHandle handle)
        {
            return
            queue.EnqueueAction(() =>
            {
                handle.WaitOne();
            });
        }

        /// <summary>
        /// Waits for the given handle to be signaled.
        /// </summary>
        public static IActionQueue WaitForHandle(this IActionQueue queue, WaitHandle handle, TimeSpan timeout, bool verifyWasSet, string errorMessage)
        {
            return
            queue.EnqueueAction(() =>
            {
                bool wasSet = handle.WaitOne(timeout);

                if (verifyWasSet)
                {
                    Verify.IsTrue(wasSet, errorMessage);
                }
            });
        }
        #endregion

        /// <summary>
        /// Waits for animations to complete.
        /// </summary>
        public static IActionQueue WaitForIdle(this IActionQueue queue)
        {
            return
            queue.EnqueueAction(() => TestServices.WindowHelper.WaitForIdle());
        }
    }

    /// <summary>
    /// Provides input helpers for IActionQueue.
    /// </summary>
    public static class ActionQueueInputExtensions
    {
        public static IActionQueue Tap(this IActionQueue queue, FrameworkElement target)
        {
            var resetEvent = new AutoResetEvent(false);
            TappedEventHandler resetEventAction = delegate { resetEvent.Set(); };
            UIElement innerMostTarget = null;

            if (target == null)
            {
                throw new ArgumentNullException("target", "You cannot call Tap with a null reference target. Make sure you call Commit before you call Tap.");
            }

            queue
            .ScheduleOnUIThread(() =>
            {
                innerMostTarget = GetInnerMostTarget(target);
                innerMostTarget.Tapped += resetEventAction;
            })
            .EnqueueAction(() =>
            {
                TestServices.InputHelper.Tap(target);
            })
            .WaitForHandle(resetEvent, TimeSpan.FromSeconds(1), true, "Element didn't get the tap.")
            .ScheduleOnUIThread(() =>
            {
                innerMostTarget.Tapped -= resetEventAction;
            });

            return queue;
        }

        public static IActionQueue DragFromCenter(this IActionQueue queue, FrameworkElement target, int relX, int relY, double velocity)
        {
            WEX.Logging.Interop.Log.Comment("DragFromCenter");

            if (target == null)
            {
                throw new ArgumentNullException("target", "You cannot call Drag with a null reference target. Make sure you call Commit before you call Tap.");
            }

            var resetEvent = new AutoResetEvent(false);
            PointerEventHandler resetEventAction = delegate { resetEvent.Set(); };
            UIElement innerMostTarget = null;

            queue
            .ScheduleOnUIThread(() =>
            {
                innerMostTarget = GetInnerMostTarget(target);
                innerMostTarget.PointerPressed += resetEventAction;
            })
            .EnqueueAction(() =>
            {
                TestServices.InputHelper.DragFromCenter(target, relX, relY, velocity);
            })
            .WaitForHandle(resetEvent, TimeSpan.FromSeconds(1), true, "Drag didn't start on the element.")
            .ScheduleOnUIThread(() =>
            {
                innerMostTarget.PointerPressed -= resetEventAction;
            });

            return queue;
        }

        public static IActionQueue PressHoldAndDragFromCenter(this IActionQueue queue, FrameworkElement target, int relX, int relY, double velocity, uint holdTime)
        {
            WEX.Logging.Interop.Log.Comment("PressHoldAndDragFromCenter");

            if (target == null)
            {
                throw new ArgumentNullException("target", "You cannot call Drag with a null reference target. Make sure you call Commit before you call PressHoldAndDragFromCenter.");
            }

            var resetEvent = new AutoResetEvent(false);
            PointerEventHandler resetEventAction = delegate { resetEvent.Set(); };
            UIElement innerMostTarget = null;

            queue
            .ScheduleOnUIThread(() =>
            {
                innerMostTarget = GetInnerMostTarget(target);
                innerMostTarget.PointerPressed += resetEventAction;
            })
            .EnqueueAction(() =>
            {
                TestServices.InputHelper.PressHoldAndPanFromCenter(target, relX, relY, velocity, holdTime);
            })
            .WaitForHandle(resetEvent, TimeSpan.FromSeconds(1), true, "Drag didn't start on the element.")
            .ScheduleOnUIThread(() =>
            {
                innerMostTarget.PointerPressed -= resetEventAction;
            });

            return queue;
        }

        private static UIElement GetInnerMostTarget(FrameworkElement target)
        {
            GeneralTransform transform = target.TransformToVisual(TestServices.WindowHelper.WindowContent);
            Rect bounds = transform.TransformBounds(new Rect(0, 0, target.ActualWidth, target.ActualHeight));
            Point center = new Point(bounds.X + bounds.Width / 2, bounds.Y + bounds.Height / 2);

            WEX.Logging.Interop.Log.Comment("Bounds: " + bounds.ToString());
            WEX.Logging.Interop.Log.Comment("Center: " + center.ToString());

            return VisualTreeHelper.FindElementsInHostCoordinates(center, target).First();
        }
    }
}
