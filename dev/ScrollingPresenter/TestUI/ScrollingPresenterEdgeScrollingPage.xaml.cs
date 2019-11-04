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
using Windows.UI.Xaml.Input;

using ScrollingPresenter = Microsoft.UI.Xaml.Controls.Primitives.ScrollingPresenter;

using ScrollingPresenterTestHooks = Microsoft.UI.Private.Controls.ScrollingPresenterTestHooks;
using ScrollingPresenterViewChangeResult = Microsoft.UI.Private.Controls.ScrollingPresenterViewChangeResult;
using MUXControlsTestHooks = Microsoft.UI.Private.Controls.MUXControlsTestHooks;
using MUXControlsTestHooksLoggingMessageEventArgs = Microsoft.UI.Private.Controls.MUXControlsTestHooksLoggingMessageEventArgs;
using System.ComponentModel;

namespace MUXControlsTestApp
{
    public sealed partial class ScrollingPresenterEdgeScrollingPage : TestPage
    {
        private Object asyncEventReportingLock = new Object();
        private List<string> lstAsyncEventMessage = new List<string>();

        public ScrollingPresenterEdgeScrollingPage()
        {
            InitializeComponent();

            SizeChanged += ScrollingPresenterEdgeScrollingPage_SizeChanged;

            scrollingPresenter.AddHandler(UIElement.PointerPressedEvent, new PointerEventHandler(ScrollingPresenter_PointerPressed), true);
            scrollingPresenter.AddHandler(UIElement.PointerReleasedEvent, new PointerEventHandler(ScrollingPresenter_PointerReleased), true);
            scrollingPresenter.AddHandler(UIElement.PointerMovedEvent, new PointerEventHandler(ScrollingPresenter_PointerMoved), true);
            scrollingPresenter.AddHandler(UIElement.PointerCanceledEvent, new PointerEventHandler(ScrollingPresenter_PointerCanceled), true);
            scrollingPresenter.AddHandler(UIElement.PointerCaptureLostEvent, new PointerEventHandler(ScrollingPresenter_PointerCaptureLost), true);
        }

        private void ScrollingPresenter_PointerPressed(object sender, PointerRoutedEventArgs e)
        {
            if (e.Pointer.PointerId == 1 &&
                e.KeyModifiers == Windows.System.VirtualKeyModifiers.None &&
                !e.Handled &&
                e.GetCurrentPoint(null).Properties.IsLeftButtonPressed)
            {
                if ((sender as UIElement).CapturePointer(e.Pointer))
                {
                    Window.Current.CoreWindow.PointerCursor = new Windows.UI.Core.CoreCursor(
                        Windows.UI.Core.CoreCursorType.Hand, 0);
                    AppendAsyncEventMessage($"Pointer capture initiated @ position {e.GetCurrentPoint(scrollingPresenter).Position.ToString()}");
                    txtRegisteredPointerId.Text = "1";
                    BtnRegisterPointerForEdgeScroll_Click(null, null);
                }
                else
                {
                    AppendAsyncEventMessage("Pointer capture failed");
                }
            }
        }

        private void ScrollingPresenter_PointerReleased(object sender, PointerRoutedEventArgs e)
        {
            if (e.Pointer.PointerId == 1)
            {
                PointerPointProperties ppp = e.GetCurrentPoint(null).Properties;

                if (!ppp.IsLeftButtonPressed)
                {
                    Point position = e.GetCurrentPoint(scrollingPresenter).Position;
                    (sender as UIElement).ReleasePointerCapture(e.Pointer);
                    AppendAsyncEventMessage($"Pointer capture released @ position {position.ToString()}");
                    Window.Current.CoreWindow.PointerCursor = new Windows.UI.Core.CoreCursor(
                        Windows.UI.Core.CoreCursorType.Arrow, 0);
                    BtnUnregisterPointerForEdgeScroll_Click(null, null);
                }
            }
        }

        private void ScrollingPresenter_PointerMoved(object sender, PointerRoutedEventArgs e)
        {
        }

        private void ScrollingPresenter_PointerCanceled(object sender, PointerRoutedEventArgs e)
        {
        }

        private void ScrollingPresenter_PointerCaptureLost(object sender, PointerRoutedEventArgs e)
        {
            AppendAsyncEventMessage($"Pointer capture lost - id {e.Pointer.PointerId}");
        }

        private void ScrollingPresenterEdgeScrollingPage_SizeChanged(object sender, SizeChangedEventArgs e)
        {
            lstScrollingPresenterEvents.MaxHeight = ActualHeight - 80;
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

                scrollingPresenter.HorizontalEdgeScrollParameters.PointerPositionAdjustment = pointerPositionAdjustment;
                scrollingPresenter.HorizontalEdgeScrollParameters.NearEdgeApplicableRange = leftEdgeApplicableRange;
                scrollingPresenter.HorizontalEdgeScrollParameters.FarEdgeApplicableRange = rightEdgeApplicableRange;
                scrollingPresenter.HorizontalEdgeScrollParameters.NearEdgeVelocity = leftEdgeVelocity;
                scrollingPresenter.HorizontalEdgeScrollParameters.FarEdgeVelocity = rightEdgeVelocity;

                if (leftEdgeVelocity == 0)
                {
                    rectLeftLeftEdge.Visibility = Visibility.Collapsed;
                    rectLeftRightEdge.Visibility = Visibility.Collapsed;
                }
                else
                {
                    rectLeftLeftEdge.Visibility = Visibility.Visible;
                    rectLeftRightEdge.Visibility = Visibility.Visible;

                    Canvas.SetLeft(rectLeftLeftEdge, scrollingPresenter.Margin.Left - leftEdgeApplicableRange - rectLeftLeftEdge.Width / 2.0);
                    Canvas.SetLeft(rectLeftRightEdge, scrollingPresenter.Margin.Left + leftEdgeApplicableRange - rectLeftLeftEdge.Width / 2.0);
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

                    Canvas.SetLeft(rectRightLeftEdge, scrollingPresenter.Margin.Left + scrollingPresenter.Width - rightEdgeApplicableRange - rectRightLeftEdge.Width / 2.0);
                    Canvas.SetLeft(rectRightRightEdge, scrollingPresenter.Margin.Left + scrollingPresenter.Width + rightEdgeApplicableRange - rectRightLeftEdge.Width / 2.0);
                }
            }
            catch (Exception ex)
            {
                AppendAsyncEventMessage(ex.ToString());
            }
        }

        //private void BtnCancelHorizontalEdgeScrolling_Click(object sender, RoutedEventArgs e)
        //{
        //    try
        //    {
        //        AppendAsyncEventMessage("Deactivating");

        //        scrollingPresenter.SetHorizontalEdgeScrolling(
        //            pointerDeviceType: PointerDeviceType.Mouse,
        //            pointerPositionAdjustment: new Point(0, 0),
        //            leftEdgeApplicableRange: 0.0,
        //            rightEdgeApplicableRange: 0.0,
        //            leftEdgeVelocity: 0.0f,
        //            rightEdgeVelocity: 0.0f);

        //        btnCancelHorizontalEdgeScrolling.IsEnabled = false;
        //        chkIsHorizontalActive.IsChecked = false;

        //        rectLeftLeftEdge.Visibility = Visibility.Collapsed;
        //        rectLeftRightEdge.Visibility = Visibility.Collapsed;
        //        rectRightLeftEdge.Visibility = Visibility.Collapsed;
        //        rectRightRightEdge.Visibility = Visibility.Collapsed;
        //    }
        //    catch (Exception ex)
        //    {
        //        AppendAsyncEventMessage(ex.ToString());
        //    }
        //}

        private void TxtHEdgeVelocity_TextChanged(object sender, TextChangedEventArgs e)
        {
            //try
            //{
            //    Single leftEdgeVelocity = Convert.ToSingle(txtLeftEdgeVelocity.Text);
            //    Single rightEdgeVelocity = Convert.ToSingle(txtRightEdgeVelocity.Text);

            //    btnUpdateHorizontalEdgeScrolling.IsEnabled = leftEdgeVelocity != 0.0f || rightEdgeVelocity != 0.0f;
            //}
            //catch (Exception ex)
            //{
            //    AppendAsyncEventMessage(ex.ToString());
            //}
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

                scrollingPresenter.VerticalEdgeScrollParameters.PointerPositionAdjustment = pointerPositionAdjustment;
                scrollingPresenter.VerticalEdgeScrollParameters.NearEdgeApplicableRange = topEdgeApplicableRange;
                scrollingPresenter.VerticalEdgeScrollParameters.FarEdgeApplicableRange = bottomEdgeApplicableRange;
                scrollingPresenter.VerticalEdgeScrollParameters.NearEdgeVelocity = topEdgeVelocity;
                scrollingPresenter.VerticalEdgeScrollParameters.FarEdgeVelocity = bottomEdgeVelocity;

                if (topEdgeVelocity == 0)
                {
                    rectTopTopEdge.Visibility = Visibility.Collapsed;
                    rectTopBottomEdge.Visibility = Visibility.Collapsed;
                }
                else
                {
                    rectTopTopEdge.Visibility = Visibility.Visible;
                    rectTopBottomEdge.Visibility = Visibility.Visible;

                    Canvas.SetTop(rectTopTopEdge, scrollingPresenter.Margin.Top - topEdgeApplicableRange - rectTopTopEdge.Height / 2.0);
                    Canvas.SetTop(rectTopBottomEdge, scrollingPresenter.Margin.Top + topEdgeApplicableRange - rectTopTopEdge.Height / 2.0);
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

                    Canvas.SetTop(rectBottomTopEdge, scrollingPresenter.Margin.Top + scrollingPresenter.Height - bottomEdgeApplicableRange - rectBottomTopEdge.Height / 2.0);
                    Canvas.SetTop(rectBottomBottomEdge, scrollingPresenter.Margin.Top + scrollingPresenter.Height + bottomEdgeApplicableRange - rectBottomTopEdge.Height / 2.0);
                }
            }
            catch (Exception ex)
            {
                AppendAsyncEventMessage(ex.ToString());
            }
        }

        //private void BtnCancelVerticalEdgeScrolling_Click(object sender, RoutedEventArgs e)
        //{
        //    try
        //    {
        //        AppendAsyncEventMessage("Deactivating");

        //        scrollingPresenter.SetVerticalEdgeScrolling(
        //            pointerDeviceType: PointerDeviceType.Mouse,
        //            pointerPositionAdjustment: new Point(0, 0),
        //            topEdgeApplicableRange: 0.0,
        //            bottomEdgeApplicableRange: 0.0,
        //            topEdgeVelocity: 0.0f,
        //            bottomEdgeVelocity: 0.0f);

        //        btnCancelVerticalEdgeScrolling.IsEnabled = false;
        //        chkIsVerticalActive.IsChecked = false;

        //        rectTopTopEdge.Visibility = Visibility.Collapsed;
        //        rectTopBottomEdge.Visibility = Visibility.Collapsed;
        //        rectBottomTopEdge.Visibility = Visibility.Collapsed;
        //        rectBottomBottomEdge.Visibility = Visibility.Collapsed;
        //    }
        //    catch (Exception ex)
        //    {
        //        AppendAsyncEventMessage(ex.ToString());
        //    }
        //}

        private void TxtVEdgeVelocity_TextChanged(object sender, TextChangedEventArgs e)
        {
            //try
            //{
            //    Single topEdgeVelocity = Convert.ToSingle(txtTopEdgeVelocity.Text);
            //    Single bottomEdgeVelocity = Convert.ToSingle(txtBottomEdgeVelocity.Text);

            //    btnUpdateVerticalEdgeScrolling.IsEnabled = topEdgeVelocity != 0.0f || bottomEdgeVelocity != 0.0f;
            //}
            //catch (Exception ex)
            //{
            //    AppendAsyncEventMessage(ex.ToString());
            //}
        }

        private void BtnRegisterPointerForEdgeScroll_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                UInt32 pointerId = Convert.ToUInt32(txtRegisteredPointerId.Text);

                AppendAsyncEventMessage($"RegisterPointerForEdgeScroll {pointerId}");
                
                scrollingPresenter.RegisterPointerForEdgeScroll(pointerId);
            }
            catch (Exception ex)
            {
                AppendAsyncEventMessage(ex.ToString());
            }
        }

        private void BtnUnregisterPointerForEdgeScroll_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                AppendAsyncEventMessage($"UnregisterPointerForEdgeScroll");

                scrollingPresenter.UnregisterPointerForEdgeScroll();
                txtRegisteredPointerId.Text = string.Empty;
            }
            catch (Exception ex)
            {
                AppendAsyncEventMessage(ex.ToString());
            }
        }

        private void BtnClearScrollingPresenterEvents_Click(object sender, RoutedEventArgs e)
        {
            lstScrollingPresenterEvents.Items.Clear();
        }

        private void BtnCopyScrollingPresenterEvents_Click(object sender, RoutedEventArgs e)
        {
            string logs = string.Empty;

            foreach (object log in lstScrollingPresenterEvents.Items)
            {
                logs += log.ToString() + "\n";
            }

            var dataPackage = new Windows.ApplicationModel.DataTransfer.DataPackage();
            dataPackage.SetText(logs);
            Windows.ApplicationModel.DataTransfer.Clipboard.SetContent(dataPackage);
        }

        private void ChkLogScrollingPresenterMessages_Checked(object sender, RoutedEventArgs e)
        {
            //To turn on info and verbose logging for the ScrollingPresenter type:
            MUXControlsTestHooks.SetLoggingLevelForType("ScrollingPresenter", isLoggingInfoLevel: true, isLoggingVerboseLevel: true);

            MUXControlsTestHooks.LoggingMessage += MUXControlsTestHooks_LoggingMessage;
        }

        private void ChkLogScrollingPresenterMessages_Unchecked(object sender, RoutedEventArgs e)
        {
            //To turn off info and verbose logging for the ScrollingPresenter type:
            MUXControlsTestHooks.SetLoggingLevelForType("ScrollingPresenter", isLoggingInfoLevel: false, isLoggingVerboseLevel: false);

            MUXControlsTestHooks.LoggingMessage -= MUXControlsTestHooks_LoggingMessage;
        }

        private void ChkLogScrollingPresenterEvents_Checked(object sender, RoutedEventArgs e)
        {
            scrollingPresenter.ExtentChanged += ScrollingPresenter_ExtentChanged;
            scrollingPresenter.StateChanged += ScrollingPresenter_StateChanged;
            scrollingPresenter.ViewChanged += ScrollingPresenter_ViewChanged;
            scrollingPresenter.ScrollCompleted += ScrollingPresenter_ScrollCompleted;
        }

        private void ChkLogScrollingPresenterEvents_Unchecked(object sender, RoutedEventArgs e)
        {
            scrollingPresenter.ExtentChanged -= ScrollingPresenter_ExtentChanged;
            scrollingPresenter.StateChanged -= ScrollingPresenter_StateChanged;
            scrollingPresenter.ViewChanged -= ScrollingPresenter_ViewChanged;
            scrollingPresenter.ScrollCompleted -= ScrollingPresenter_ScrollCompleted;
        }

        private void ChkLogInteractionTrackerInfo_Checked(object sender, RoutedEventArgs e)
        {
            InteractionTracker interactionTracker = ScrollingPresenterTestHooks.GetInteractionTracker(scrollingPresenter);
            CompositionPropertySpy.StartSpyingVector3Property(interactionTracker, "PositionVelocityInPixelsPerSecond", Vector3.Zero);
            scrollingPresenter.ViewChanged += ScrollingPresenter_ViewChangedForVelocitySpying;
        }

        private void ChkLogInteractionTrackerInfo_Unchecked(object sender, RoutedEventArgs e)
        {
            InteractionTracker interactionTracker = ScrollingPresenterTestHooks.GetInteractionTracker(scrollingPresenter);
            CompositionPropertySpy.StopSpyingProperty(interactionTracker, "PositionVelocityInPixelsPerSecond");
            scrollingPresenter.ViewChanged -= ScrollingPresenter_ViewChangedForVelocitySpying;
        }

        private void ScrollingPresenter_ExtentChanged(ScrollingPresenter sender, object args)
        {
            AppendAsyncEventMessage($"ExtentChanged ExtentWidth={sender.ExtentWidth}, ExtentHeight={sender.ExtentHeight}");
        }

        private void ScrollingPresenter_StateChanged(ScrollingPresenter sender, object args)
        {
            AppendAsyncEventMessage($"StateChanged {sender.State.ToString()}");
        }

        private void ScrollingPresenter_ViewChanged(ScrollingPresenter sender, object args)
        {
            AppendAsyncEventMessage($"ViewChanged H={sender.HorizontalOffset.ToString()}, V={sender.VerticalOffset}, ZF={sender.ZoomFactor}");
        }

        private void ScrollingPresenter_ScrollCompleted(ScrollingPresenter sender, ScrollingScrollCompletedEventArgs args)
        {
            ScrollingPresenterViewChangeResult result = ScrollingPresenterTestHooks.GetScrollCompletedResult(args);

            AppendAsyncEventMessage($"ScrollCompleted OffsetsChangeId={args.ScrollInfo.OffsetsChangeId}, Result={result}");
        }

        //private void ScrollingPresenter_ZoomCompleted(ScrollingPresenter sender, ScrollingZoomCompletedEventArgs args)
        //{
        //    ScrollingPresenterViewChangeResult result = ScrollingPresenterTestHooks.GetZoomCompletedResult(args);

        //    AppendAsyncEventMessage($"ZoomCompleted ZoomFactorChangeId={args.ZoomInfo.ZoomFactorChangeId}, Result={result}");
        //}

        private void ScrollingPresenter_ViewChangedForVelocitySpying(ScrollingPresenter sender, object args)
        {
            Vector3 positionVelocityInPixelsPerSecond;
            InteractionTracker interactionTracker = ScrollingPresenterTestHooks.GetInteractionTracker(scrollingPresenter);
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
                    lstScrollingPresenterEvents.Items.Add(asyncEventMessage);
                }
                lstAsyncEventMessage.Clear();
            }
        }
    }
}
