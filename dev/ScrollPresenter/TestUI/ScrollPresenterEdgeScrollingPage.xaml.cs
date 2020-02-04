// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml.Controls;
using MUXControlsTestApp.Utilities;
using System;
using System.Collections.Generic;
using System.Numerics;
using Windows.Devices.Input;
using Windows.Foundation;
using Windows.UI.Composition;
using Windows.UI.Composition.Interactions;
using Windows.UI.Input;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Input;

using ScrollPresenter = Microsoft.UI.Xaml.Controls.Primitives.ScrollPresenter;

using ScrollPresenterTestHooks = Microsoft.UI.Private.Controls.ScrollPresenterTestHooks;
using ScrollPresenterViewChangeResult = Microsoft.UI.Private.Controls.ScrollPresenterViewChangeResult;
using MUXControlsTestHooks = Microsoft.UI.Private.Controls.MUXControlsTestHooks;
using MUXControlsTestHooksLoggingMessageEventArgs = Microsoft.UI.Private.Controls.MUXControlsTestHooksLoggingMessageEventArgs;

namespace MUXControlsTestApp
{
    public sealed partial class ScrollPresenterEdgeScrollingPage : TestPage
    {
        private Object asyncEventReportingLock = new Object();
        private List<string> lstAsyncEventMessage = new List<string>();
        private UInt32 capturedPointerId = 0;

        public ScrollPresenterEdgeScrollingPage()
        {
            InitializeComponent();

            SizeChanged += ScrollPresenterEdgeScrollingPage_SizeChanged;

            scrollPresenter.AddHandler(UIElement.PointerPressedEvent, new PointerEventHandler(ScrollPresenter_PointerPressed), true);
            scrollPresenter.AddHandler(UIElement.PointerReleasedEvent, new PointerEventHandler(ScrollPresenter_PointerReleased), true);
            scrollPresenter.AddHandler(UIElement.PointerCanceledEvent, new PointerEventHandler(ScrollPresenter_PointerCanceled), true);
            scrollPresenter.AddHandler(UIElement.PointerCaptureLostEvent, new PointerEventHandler(ScrollPresenter_PointerCaptureLost), true);

            canvas.AddHandler(UIElement.PointerPressedEvent, new PointerEventHandler(Canvas_PointerPressed), true);
            canvas.AddHandler(UIElement.PointerReleasedEvent, new PointerEventHandler(Canvas_PointerReleased), true);
            canvas.AddHandler(UIElement.PointerMovedEvent, new PointerEventHandler(Canvas_PointerMoved), true);
            canvas.AddHandler(UIElement.PointerCanceledEvent, new PointerEventHandler(Canvas_PointerCanceled), true);
            canvas.AddHandler(UIElement.PointerCaptureLostEvent, new PointerEventHandler(Canvas_PointerCaptureLost), true);
        }

        private bool IsPointerDeviceTypeRegistered(PointerDeviceType pointerDeviceType)
        {
            switch (pointerDeviceType)
            {
                case PointerDeviceType.Mouse:
                    return (bool)tglBtnRegisterMouseForEdgeScroll.IsChecked;
                case PointerDeviceType.Pen:
                    return (bool)tglBtnRegisterPenForEdgeScroll.IsChecked;
                case PointerDeviceType.Touch:
                    return (bool)tglBtnRegisterTouchForEdgeScroll.IsChecked;
                default:
                    return false;
            }
        }

        private bool IsPointerDeviceTypeStartedStopped(PointerDeviceType pointerDeviceType)
        {
            switch (pointerDeviceType)
            {
                case PointerDeviceType.Mouse:
                    return (bool)tglBtnStartStopMouseForEdgeScroll.IsChecked;
                case PointerDeviceType.Pen:
                    return (bool)tglBtnStartStopPenForEdgeScroll.IsChecked;
                case PointerDeviceType.Touch:
                    return (bool)tglBtnStartStopTouchForEdgeScroll.IsChecked;
                default:
                    return false;
            }
        }

        private ToggleButton RegisterUnregisterToggleButtonFromPointerDeviceType(PointerDeviceType pointerDeviceType)
        {
            switch (pointerDeviceType)
            {
                case PointerDeviceType.Mouse:
                    return tglBtnRegisterMouseForEdgeScroll;
                case PointerDeviceType.Pen:
                    return tglBtnRegisterPenForEdgeScroll;
                case PointerDeviceType.Touch:
                    return tglBtnRegisterTouchForEdgeScroll;
                default:
                    return null;
            }
        }

        private ToggleButton StartStopToggleButtonFromPointerDeviceType(PointerDeviceType pointerDeviceType)
        {
            switch (pointerDeviceType)
            {
                case PointerDeviceType.Mouse:
                    return tglBtnStartStopMouseForEdgeScroll;
                case PointerDeviceType.Pen:
                    return tglBtnStartStopPenForEdgeScroll;
                case PointerDeviceType.Touch:
                    return tglBtnStartStopTouchForEdgeScroll;
                default:
                    return null;
            }
        }

        private void ScrollPresenter_PointerPressed(object sender, PointerRoutedEventArgs e)
        {
            if (chkLogScrollPresenterEvents.IsChecked == true)
            {
                AppendAsyncEventMessage($"ScrollPresenter_PointerPressed PointerId {e.Pointer.PointerId}");
            }

            if (IsPointerDeviceTypeRegistered(e.Pointer.PointerDeviceType) &&
                e.KeyModifiers == Windows.System.VirtualKeyModifiers.None &&
                !e.Handled &&
                (e.Pointer.PointerDeviceType != PointerDeviceType.Mouse || e.GetCurrentPoint(null).Properties.IsLeftButtonPressed))
            {
                if ((sender as UIElement).CapturePointer(e.Pointer))
                {
                    Window.Current.CoreWindow.PointerCursor = new Windows.UI.Core.CoreCursor(
                        Windows.UI.Core.CoreCursorType.Hand, 0);
                    AppendAsyncEventMessage($"Pointer capture initiated @ position {e.GetCurrentPoint(scrollPresenter).Position.ToString()}");
                    AppendAsyncEventMessage($"RegisterPointerForEdgeScroll {e.Pointer.PointerId}");
                    scrollPresenter.RegisterPointerForEdgeScroll(e.Pointer.PointerId);
                    RegisterUnregisterToggleButtonFromPointerDeviceType(e.Pointer.PointerDeviceType).IsEnabled = false;
                }
                else
                {
                    AppendAsyncEventMessage("Pointer capture failed");
                }
            }
        }

        private void ScrollPresenter_PointerReleased(object sender, PointerRoutedEventArgs e)
        {
            if (chkLogScrollPresenterEvents.IsChecked == true)
            {
                AppendAsyncEventMessage($"ScrollPresenter_PointerReleased PointerId {e.Pointer.PointerId}");
            }

            if (IsPointerDeviceTypeRegistered(e.Pointer.PointerDeviceType))
            {
                PointerPointProperties ppp = e.GetCurrentPoint(null).Properties;

                if (e.Pointer.PointerDeviceType != PointerDeviceType.Mouse || !ppp.IsLeftButtonPressed)
                {
                    Point position = e.GetCurrentPoint(scrollPresenter).Position;
                    (sender as UIElement).ReleasePointerCapture(e.Pointer);
                    AppendAsyncEventMessage($"Pointer capture released @ position {position.ToString()}");
                    Window.Current.CoreWindow.PointerCursor = new Windows.UI.Core.CoreCursor(
                        Windows.UI.Core.CoreCursorType.Arrow, 0);
                    int offsetsChangeCorrelationId = scrollPresenter.UnregisterPointerForEdgeScroll();
                    AppendAsyncEventMessage($"UnregisterPointerForEdgeScroll OffsetsChangeCorrelationId {offsetsChangeCorrelationId}");
                    RegisterUnregisterToggleButtonFromPointerDeviceType(e.Pointer.PointerDeviceType).IsEnabled = true;
                }
            }
        }

        private void ScrollPresenter_PointerCanceled(object sender, PointerRoutedEventArgs e)
        {
            if (chkLogScrollPresenterEvents.IsChecked == true)
            {
                AppendAsyncEventMessage($"ScrollPresenter_PointerCanceled PointerId {e.Pointer.PointerId}");
            }
        }

        private void ScrollPresenter_PointerCaptureLost(object sender, PointerRoutedEventArgs e)
        {
            if (chkLogScrollPresenterEvents.IsChecked == true)
            {
                AppendAsyncEventMessage($"ScrollPresenter_PointerCaptureLost PointerId {e.Pointer.PointerId}");
            }
        }

        private bool IsStartStopMouseForEdgeScrollEnabled
        {
            get
            {
                return tglBtnStartStopMouseForEdgeScroll.IsChecked == true ||
                    tglBtnStartStopTouchForEdgeScroll.IsChecked == true ||
                    tglBtnStartStopPenForEdgeScroll.IsChecked == true;
            }
        }

        private void Canvas_PointerPressed(object sender, PointerRoutedEventArgs e)
        {
            if (IsStartStopMouseForEdgeScrollEnabled)
            {
                AppendAsyncEventMessage($"Canvas_PointerPressed PointerId {e.Pointer.PointerId}");

                if (capturedPointerId == 0 &&
                    IsPointerDeviceTypeStartedStopped(e.Pointer.PointerDeviceType) &&
                    e.KeyModifiers == Windows.System.VirtualKeyModifiers.None &&
                    !e.Handled &&
                    (e.Pointer.PointerDeviceType != PointerDeviceType.Mouse || e.GetCurrentPoint(null).Properties.IsLeftButtonPressed))
                {
                    if ((sender as UIElement).CapturePointer(e.Pointer))
                    {
                        Window.Current.CoreWindow.PointerCursor = new Windows.UI.Core.CoreCursor(
                            Windows.UI.Core.CoreCursorType.Hand, 0);
                        AppendAsyncEventMessage($"Pointer capture initiated @ position {e.GetCurrentPoint(scrollPresenter).Position.ToString()}");
                        StartStopToggleButtonFromPointerDeviceType(e.Pointer.PointerDeviceType).IsEnabled = false;
                        capturedPointerId = e.Pointer.PointerId;
                    }
                    else
                    {
                        AppendAsyncEventMessage("Pointer capture failed");
                    }
                }
            }
        }

        private void Canvas_PointerReleased(object sender, PointerRoutedEventArgs e)
        {
            if (IsStartStopMouseForEdgeScrollEnabled)
            {
                AppendAsyncEventMessage($"Canvas_PointerReleased PointerId {e.Pointer.PointerId}");

                if (capturedPointerId != 0 && IsPointerDeviceTypeStartedStopped(e.Pointer.PointerDeviceType))
                {
                    PointerPointProperties ppp = e.GetCurrentPoint(null).Properties;

                    if (e.Pointer.PointerDeviceType != PointerDeviceType.Mouse || !ppp.IsLeftButtonPressed)
                    {
                        Point position = e.GetCurrentPoint(scrollPresenter).Position;
                        (sender as UIElement).ReleasePointerCapture(e.Pointer);
                        AppendAsyncEventMessage($"Pointer capture released @ position {position.ToString()}");
                        Window.Current.CoreWindow.PointerCursor = new Windows.UI.Core.CoreCursor(
                            Windows.UI.Core.CoreCursorType.Arrow, 0);
                        StartStopToggleButtonFromPointerDeviceType(e.Pointer.PointerDeviceType).IsEnabled = true;
                        capturedPointerId = 0;
                        int offsetsChangeCorrelationId = scrollPresenter.StopEdgeScrollWithPointer();
                        AppendAsyncEventMessage($"StopEdgeScrollWithPointer OffsetsChangeCorrelationId {offsetsChangeCorrelationId}");
                    }
                }
            }
        }

        private void Canvas_PointerMoved(object sender, PointerRoutedEventArgs e)
        {
            if (capturedPointerId == e.Pointer.PointerId)
            {
                int offsetsChangeCorrelationId = scrollPresenter.StartEdgeScrollWithPointer(e);
                AppendAsyncEventMessage($"StartEdgeScrollWithPointer OffsetsChangeCorrelationId {offsetsChangeCorrelationId}");
            }
        }

        private void Canvas_PointerCanceled(object sender, PointerRoutedEventArgs e)
        {
            if (IsStartStopMouseForEdgeScrollEnabled)
            {
                AppendAsyncEventMessage($"Canvas_PointerCanceled PointerId {e.Pointer.PointerId}");
            }
        }

        private void Canvas_PointerCaptureLost(object sender, PointerRoutedEventArgs e)
        {
            if (IsStartStopMouseForEdgeScrollEnabled)
            {
                AppendAsyncEventMessage($"Canvas_PointerCaptureLost PointerId {e.Pointer.PointerId}");
            }
        }

        private void ScrollPresenterEdgeScrollingPage_SizeChanged(object sender, SizeChangedEventArgs e)
        {
            lstScrollPresenterEvents.MaxHeight = ActualHeight - 80;
        }

        private void BtnUpdateHorizontalEdgeScrollParameters_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                Point pointerPositionAdjustment = new Point(
                    Convert.ToDouble(txtHorizontalPositionAdjustmentX.Text),
                    Convert.ToDouble(txtHorizontalPositionAdjustmentY.Text));

                Double leftEdgeApplicableRange = Convert.ToDouble(txtLeftEdgeApplicableRange.Text);
                Double rightEdgeApplicableRange = Convert.ToDouble(txtRightEdgeApplicableRange.Text);

                Single leftEdgeVelocity = Convert.ToSingle(txtLeftEdgeVelocity.Text);
                Single rightEdgeVelocity = Convert.ToSingle(txtRightEdgeVelocity.Text);

                AppendAsyncEventMessage($"Updating horizontal parameters with pointerPositionAdjustment:{pointerPositionAdjustment}, leftEdgeApplicableRange:{leftEdgeApplicableRange}, rightEdgeApplicableRange:{rightEdgeApplicableRange}, leftEdgeVelocity:{leftEdgeVelocity}, rightEdgeVelocity:{rightEdgeVelocity}");

                scrollPresenter.HorizontalEdgeScrollParameters.PointerPositionAdjustment = pointerPositionAdjustment;
                scrollPresenter.HorizontalEdgeScrollParameters.NearEdgeApplicableRange = leftEdgeApplicableRange;
                scrollPresenter.HorizontalEdgeScrollParameters.FarEdgeApplicableRange = rightEdgeApplicableRange;
                scrollPresenter.HorizontalEdgeScrollParameters.NearEdgeVelocity = leftEdgeVelocity;
                scrollPresenter.HorizontalEdgeScrollParameters.FarEdgeVelocity = rightEdgeVelocity;

                if (leftEdgeVelocity == 0)
                {
                    rectLeftLeftEdge.Visibility = Visibility.Collapsed;
                    rectLeftRightEdge.Visibility = Visibility.Collapsed;
                }
                else
                {
                    rectLeftLeftEdge.Visibility = Visibility.Visible;
                    rectLeftRightEdge.Visibility = Visibility.Visible;

                    Canvas.SetLeft(rectLeftLeftEdge, scrollPresenter.Margin.Left - leftEdgeApplicableRange - rectLeftLeftEdge.Width / 2.0);
                    Canvas.SetLeft(rectLeftRightEdge, scrollPresenter.Margin.Left + leftEdgeApplicableRange - rectLeftLeftEdge.Width / 2.0);
                }

                if (rightEdgeVelocity == 0)
                {
                    rectRightLeftEdge.Visibility = Visibility.Collapsed;
                    rectRightRightEdge.Visibility = Visibility.Collapsed;
                }
                else
                {
                    rectRightLeftEdge.Visibility = Visibility.Visible;
                    rectRightRightEdge.Visibility = Visibility.Visible;

                    Canvas.SetLeft(rectRightLeftEdge, scrollPresenter.Margin.Left + scrollPresenter.Width - rightEdgeApplicableRange - rectRightLeftEdge.Width / 2.0);
                    Canvas.SetLeft(rectRightRightEdge, scrollPresenter.Margin.Left + scrollPresenter.Width + rightEdgeApplicableRange - rectRightLeftEdge.Width / 2.0);
                }
            }
            catch (Exception ex)
            {
                AppendAsyncEventMessage(ex.ToString());
            }
        }

        private void BtnUpdateVerticalEdgeScrollParameters_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                Point pointerPositionAdjustment = new Point(
                    Convert.ToDouble(txtVerticalPositionAdjustmentX.Text),
                    Convert.ToDouble(txtVerticalPositionAdjustmentY.Text));

                Double topEdgeApplicableRange = Convert.ToDouble(txtTopEdgeApplicableRange.Text);
                Double bottomEdgeApplicableRange = Convert.ToDouble(txtBottomEdgeApplicableRange.Text);

                Single topEdgeVelocity = Convert.ToSingle(txtTopEdgeVelocity.Text);
                Single bottomEdgeVelocity = Convert.ToSingle(txtBottomEdgeVelocity.Text);

                AppendAsyncEventMessage($"Updating vertical parameters with pointerPositionAdjustment:{pointerPositionAdjustment}, topEdgeApplicableRange:{topEdgeApplicableRange}, bottomEdgeApplicableRange:{bottomEdgeApplicableRange}, topEdgeVelocity:{topEdgeVelocity}, bottomEdgeVelocity:{bottomEdgeVelocity}");

                scrollPresenter.VerticalEdgeScrollParameters.PointerPositionAdjustment = pointerPositionAdjustment;
                scrollPresenter.VerticalEdgeScrollParameters.NearEdgeApplicableRange = topEdgeApplicableRange;
                scrollPresenter.VerticalEdgeScrollParameters.FarEdgeApplicableRange = bottomEdgeApplicableRange;
                scrollPresenter.VerticalEdgeScrollParameters.NearEdgeVelocity = topEdgeVelocity;
                scrollPresenter.VerticalEdgeScrollParameters.FarEdgeVelocity = bottomEdgeVelocity;

                if (topEdgeVelocity == 0)
                {
                    rectTopTopEdge.Visibility = Visibility.Collapsed;
                    rectTopBottomEdge.Visibility = Visibility.Collapsed;
                }
                else
                {
                    rectTopTopEdge.Visibility = Visibility.Visible;
                    rectTopBottomEdge.Visibility = Visibility.Visible;

                    Canvas.SetTop(rectTopTopEdge, scrollPresenter.Margin.Top - topEdgeApplicableRange - rectTopTopEdge.Height / 2.0);
                    Canvas.SetTop(rectTopBottomEdge, scrollPresenter.Margin.Top + topEdgeApplicableRange - rectTopTopEdge.Height / 2.0);
                }

                if (bottomEdgeVelocity == 0)
                {
                    rectBottomTopEdge.Visibility = Visibility.Collapsed;
                    rectBottomBottomEdge.Visibility = Visibility.Collapsed;
                }
                else
                {
                    rectBottomTopEdge.Visibility = Visibility.Visible;
                    rectBottomBottomEdge.Visibility = Visibility.Visible;

                    Canvas.SetTop(rectBottomTopEdge, scrollPresenter.Margin.Top + scrollPresenter.Height - bottomEdgeApplicableRange - rectBottomTopEdge.Height / 2.0);
                    Canvas.SetTop(rectBottomBottomEdge, scrollPresenter.Margin.Top + scrollPresenter.Height + bottomEdgeApplicableRange - rectBottomTopEdge.Height / 2.0);
                }
            }
            catch (Exception ex)
            {
                AppendAsyncEventMessage(ex.ToString());
            }
        }

        private void TglBtnRegisterPointerIdForEdgeScroll_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if ((bool)tglBtnRegisterPointerIdForEdgeScroll.IsChecked)
                {
                    UInt32 pointerId = Convert.ToUInt32(txtRegisteredPointerId.Text);

                    AppendAsyncEventMessage($"RegisterPointerForEdgeScroll {pointerId}");

                    scrollPresenter.RegisterPointerForEdgeScroll(pointerId);
                }
                else
                {
                    AppendAsyncEventMessage($"UnregisterPointerForEdgeScroll");

                    scrollPresenter.UnregisterPointerForEdgeScroll();
                    txtRegisteredPointerId.Text = string.Empty;
                }
            }
            catch (Exception ex)
            {
                AppendAsyncEventMessage(ex.ToString());
                tglBtnRegisterPointerIdForEdgeScroll.IsChecked = false;
            }
        }

        private void TglBtnStartStopMouseForEdgeScroll_Click(object sender, RoutedEventArgs e)
        {
            StartStopPointerForEdgeScroll(PointerDeviceType.Mouse, sender as ToggleButton);
        }

        private void TglBtnStartStopTouchForEdgeScroll_Click(object sender, RoutedEventArgs e)
        {
            StartStopPointerForEdgeScroll(PointerDeviceType.Touch, sender as ToggleButton);
        }

        private void TglBtnStartStopPenForEdgeScroll_Click(object sender, RoutedEventArgs e)
        {
            StartStopPointerForEdgeScroll(PointerDeviceType.Pen, sender as ToggleButton);
        }

        private void StartStopPointerForEdgeScroll(PointerDeviceType pointerDeviceType, ToggleButton toggleButton)
        {
            if (toggleButton.IsChecked == true)
            {
                toggleButton.Content = (toggleButton.Content as string).Replace("Start", "Stop");
            }
            else
            {
                toggleButton.Content = (toggleButton.Content as string).Replace("Stop", "Start");
            }
        }

        private void BtnClearScrollPresenterEvents_Click(object sender, RoutedEventArgs e)
        {
            lstScrollPresenterEvents.Items.Clear();
        }

        private void BtnCopyScrollPresenterEvents_Click(object sender, RoutedEventArgs e)
        {
            string logs = string.Empty;

            foreach (object log in lstScrollPresenterEvents.Items)
            {
                logs += log.ToString() + "\n";
            }

            var dataPackage = new Windows.ApplicationModel.DataTransfer.DataPackage();
            dataPackage.SetText(logs);
            Windows.ApplicationModel.DataTransfer.Clipboard.SetContent(dataPackage);
        }

        private void ChkLogScrollPresenterMessages_Checked(object sender, RoutedEventArgs e)
        {
            //To turn on info and verbose logging for the ScrollPresenter type:
            MUXControlsTestHooks.SetLoggingLevelForType("ScrollPresenter", isLoggingInfoLevel: true, isLoggingVerboseLevel: true);

            MUXControlsTestHooks.LoggingMessage += MUXControlsTestHooks_LoggingMessage;
        }

        private void ChkLogScrollPresenterMessages_Unchecked(object sender, RoutedEventArgs e)
        {
            //To turn off info and verbose logging for the ScrollPresenter type:
            MUXControlsTestHooks.SetLoggingLevelForType("ScrollPresenter", isLoggingInfoLevel: false, isLoggingVerboseLevel: false);

            MUXControlsTestHooks.LoggingMessage -= MUXControlsTestHooks_LoggingMessage;
        }

        private void ChkLogScrollPresenterEvents_Checked(object sender, RoutedEventArgs e)
        {
            scrollPresenter.ExtentChanged += ScrollPresenter_ExtentChanged;
            scrollPresenter.StateChanged += ScrollPresenter_StateChanged;
            scrollPresenter.ViewChanged += ScrollPresenter_ViewChanged;
            scrollPresenter.EdgeScrollQueued += ScrollPresenter_EdgeScrollQueued;
            scrollPresenter.ScrollCompleted += ScrollPresenter_ScrollCompleted;
        }

        private void ChkLogScrollPresenterEvents_Unchecked(object sender, RoutedEventArgs e)
        {
            scrollPresenter.ExtentChanged -= ScrollPresenter_ExtentChanged;
            scrollPresenter.StateChanged -= ScrollPresenter_StateChanged;
            scrollPresenter.ViewChanged -= ScrollPresenter_ViewChanged;
            scrollPresenter.EdgeScrollQueued -= ScrollPresenter_EdgeScrollQueued;
            scrollPresenter.ScrollCompleted -= ScrollPresenter_ScrollCompleted;
        }

        private void ChkLogInteractionTrackerInfo_Checked(object sender, RoutedEventArgs e)
        {
            InteractionTracker interactionTracker = ScrollPresenterTestHooks.GetInteractionTracker(scrollPresenter);
            CompositionPropertySpy.StartSpyingVector3Property(interactionTracker, "PositionVelocityInPixelsPerSecond", Vector3.Zero);
            scrollPresenter.ViewChanged += ScrollPresenter_ViewChangedForVelocitySpying;
        }

        private void ChkLogInteractionTrackerInfo_Unchecked(object sender, RoutedEventArgs e)
        {
            InteractionTracker interactionTracker = ScrollPresenterTestHooks.GetInteractionTracker(scrollPresenter);
            CompositionPropertySpy.StopSpyingProperty(interactionTracker, "PositionVelocityInPixelsPerSecond");
            scrollPresenter.ViewChanged -= ScrollPresenter_ViewChangedForVelocitySpying;
        }

        private void ScrollPresenter_ExtentChanged(ScrollPresenter sender, object args)
        {
            AppendAsyncEventMessage($"ScrollPresenter_ExtentChanged ExtentWidth={sender.ExtentWidth}, ExtentHeight={sender.ExtentHeight}");
        }

        private void ScrollPresenter_StateChanged(ScrollPresenter sender, object args)
        {
            AppendAsyncEventMessage($"ScrollPresenter_StateChanged {sender.State.ToString()}");
        }

        private void ScrollPresenter_ViewChanged(ScrollPresenter sender, object args)
        {
            AppendAsyncEventMessage($"ScrollPresenter_ViewChanged H={sender.HorizontalOffset.ToString()}, V={sender.VerticalOffset}, ZF={sender.ZoomFactor}");
        }

        private void ScrollPresenter_EdgeScrollQueued(ScrollPresenter sender, ScrollingEdgeScrollEventArgs e)
        {
            AppendAsyncEventMessage($"ScrollPresenter_EdgeScrollQueued PointerId {e.PointerId}, OffsetsVelocity {e.OffsetsVelocity}, OffsetsChangeId {e.CorrelationId}");
        }

        private void ScrollPresenter_ScrollCompleted(ScrollPresenter sender, ScrollingScrollCompletedEventArgs args)
        {
            ScrollPresenterViewChangeResult result = ScrollPresenterTestHooks.GetScrollCompletedResult(args);

            AppendAsyncEventMessage($"ScrollPresenter_ScrollCompleted OffsetsChangeId={args.CorrelationId}, Result={result}");
        }

        private void ScrollPresenter_ViewChangedForVelocitySpying(ScrollPresenter sender, object args)
        {
            Vector3 positionVelocityInPixelsPerSecond;
            InteractionTracker interactionTracker = ScrollPresenterTestHooks.GetInteractionTracker(scrollPresenter);
            CompositionGetValueStatus status = CompositionPropertySpy.TryGetVector3(interactionTracker, "PositionVelocityInPixelsPerSecond", out positionVelocityInPixelsPerSecond);
            AppendAsyncEventMessage($"InteractionTracker.PositionVelocityInPixelsPerSecond={positionVelocityInPixelsPerSecond.X}, {positionVelocityInPixelsPerSecond.Y}");
            AppendAsyncEventMessage($"InteractionTracker.PositionInertiaDecayRate={interactionTracker.PositionInertiaDecayRate}, InteractionTracker.ScaleInertiaDecayRate={interactionTracker.ScaleInertiaDecayRate}");
        }

        private void MUXControlsTestHooks_LoggingMessage(object sender, MUXControlsTestHooksLoggingMessageEventArgs args)
        {
            string msg = args.Message.Substring(0, args.Message.Length - 1);
            string senderName = string.Empty;

            try
            {
                FrameworkElement fe = sender as FrameworkElement;

                if (fe != null)
                {
                    senderName = "s:" + fe.Name + ", ";
                }
            }
            catch
            {
                AppendAsyncEventMessage("Warning: Failure while accessing sender's Name");
            }

            if (args.IsVerboseLevel)
            {
                AppendAsyncEventMessage($"Verbose: {senderName}m:{msg}");
            }
            else
            {
                AppendAsyncEventMessage($"Info: {senderName}m:{msg}");
            }
        }

        private void AppendAsyncEventMessage(string asyncEventMessage)
        {
            lock (asyncEventReportingLock)
            {
                while (asyncEventMessage.Length > 0)
                {
                    string msgHead = asyncEventMessage;

                    if (asyncEventMessage.Length > 150)
                    {
                        int commaIndex = asyncEventMessage.IndexOf(',', 150);
                        if (commaIndex != -1)
                        {
                            msgHead = asyncEventMessage.Substring(0, commaIndex);
                            asyncEventMessage = asyncEventMessage.Substring(commaIndex + 1);
                        }
                        else
                        {
                            asyncEventMessage = string.Empty;
                        }
                    }
                    else
                    {
                        asyncEventMessage = string.Empty;
                    }

                    lstAsyncEventMessage.Add(msgHead);
                }

                var ignored = this.Dispatcher.RunAsync(Windows.UI.Core.CoreDispatcherPriority.Normal, AppendAsyncEventMessage);
            }
        }

        private void AppendAsyncEventMessage()
        {
            lock (asyncEventReportingLock)
            {
                foreach (string asyncEventMessage in lstAsyncEventMessage)
                {
                    lstScrollPresenterEvents.Items.Add(asyncEventMessage);
                }
                lstAsyncEventMessage.Clear();
            }
        }
    }
}
