﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using Windows.UI.Composition;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;

using ScrollPresenter = Microsoft.UI.Xaml.Controls.Primitives.ScrollPresenter;
using ScrollView = Microsoft.UI.Xaml.Controls.ScrollView;
using ScrollingScrollAnimationStartingEventArgs = Microsoft.UI.Xaml.Controls.ScrollingScrollAnimationStartingEventArgs;
using ScrollingZoomAnimationStartingEventArgs = Microsoft.UI.Xaml.Controls.ScrollingZoomAnimationStartingEventArgs;
using ScrollingScrollCompletedEventArgs = Microsoft.UI.Xaml.Controls.ScrollingScrollCompletedEventArgs;
using ScrollingZoomCompletedEventArgs = Microsoft.UI.Xaml.Controls.ScrollingZoomCompletedEventArgs;
using ScrollingBringingIntoViewEventArgs = Microsoft.UI.Xaml.Controls.ScrollingBringingIntoViewEventArgs;
using MUXControlsTestHooks = Microsoft.UI.Private.Controls.MUXControlsTestHooks;
using MUXControlsTestHooksLoggingMessageEventArgs = Microsoft.UI.Private.Controls.MUXControlsTestHooksLoggingMessageEventArgs;
using ScrollViewTestHooks = Microsoft.UI.Private.Controls.ScrollViewTestHooks;

namespace MUXControlsTestApp
{
    public sealed partial class ScrollViewBringIntoViewPage : TestPage
    {
        private object asyncEventReportingLock = new object();
        private List<string> lstAsyncEventMessage = new List<string>();

        public ScrollViewBringIntoViewPage()
        {
            this.InitializeComponent();

            if (chkLogScrollViewMessages.IsChecked == true || chkLogScrollPresenterMessages.IsChecked == true)
            {
                MUXControlsTestHooks.LoggingMessage += MUXControlsTestHooks_LoggingMessage;

                if (chkLogScrollPresenterMessages.IsChecked == true)
                {
                    MUXControlsTestHooks.SetLoggingLevelForType("ScrollPresenter", isLoggingInfoLevel: true, isLoggingVerboseLevel: true);
                }
                if (chkLogScrollViewMessages.IsChecked == true)
                {
                    MUXControlsTestHooks.SetLoggingLevelForType("ScrollView", isLoggingInfoLevel: true, isLoggingVerboseLevel: true);
                }
            }
        }

        private void BtnStartBringIntoView_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                txtStockOffsetsChangeDuration.Text = string.Empty;
                
                FrameworkElement targetElement = null;

                switch (cmbTargetElement.SelectedIndex)
                {
                    case 0:
                        targetElement = rectangle;
                        break;
                    case 1:
                        targetElement = scrollView;
                        break;
                }

                if (targetElement != null)
                {
                    BringIntoViewOptions options = new BringIntoViewOptions();

                    options.AnimationDesired = cmbAnimationDesired.SelectedIndex == 0;
                    options.HorizontalAlignmentRatio = Convert.ToDouble(txtHorizontalAlignmentRatio.Text);
                    options.VerticalAlignmentRatio = Convert.ToDouble(txtVerticalAlignmentRatio.Text);
                    options.HorizontalOffset = Convert.ToDouble(txtHorizontalOffset.Text);
                    options.VerticalOffset = Convert.ToDouble(txtVerticalOffset.Text);

                    if (chkLogScrollViewMessages.IsChecked == true)
                    {
                        AppendAsyncEventMessage("Invoking StartBringIntoView on " + targetElement.Name);
                    }
                    targetElement.StartBringIntoView(options);
                }

                if (targetElement == null && chkLogScrollViewMessages.IsChecked == true)
                {
                    AppendAsyncEventMessage("TargetElement not found");
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstLogs.Items.Add(ex.ToString());
            }
        }

        private void ChkBringIntoViewUI_Checked(object sender, RoutedEventArgs e)
        {
            if (grdBringIntoViewUI != null)
                grdBringIntoViewUI.Visibility = Visibility.Visible;
        }

        private void ChkBringIntoViewUI_Unchecked(object sender, RoutedEventArgs e)
        {
            if (grdBringIntoViewUI != null)
                grdBringIntoViewUI.Visibility = Visibility.Collapsed;
        }

        private void ChkLogs_Checked(object sender, RoutedEventArgs e)
        {
            if (grdLogs != null)
                grdLogs.Visibility = Visibility.Visible;
        }

        private void ChkLogs_Unchecked(object sender, RoutedEventArgs e)
        {
            if (grdLogs != null)
                grdLogs.Visibility = Visibility.Collapsed;
        }

        private void ScrollView_Loaded(object sender, RoutedEventArgs e)
        {
            AppendAsyncEventMessage($"ScrollView.Loaded");
            if (chkLogScrollPresenterEvents.IsChecked == true)
            {
                LogScrollPresenterInfo();
            }
            LogScrollViewInfo();
        }

        private void ScrollView_SizeChanged(object sender, SizeChangedEventArgs e)
        {
            AppendAsyncEventMessage($"ScrollView.SizeChanged Size={scrollView.ActualWidth}, {scrollView.ActualHeight}");
            if (chkLogScrollPresenterEvents.IsChecked == true)
            {
                LogScrollPresenterInfo();
            }
            LogScrollViewInfo();
        }

        private void ScrollView_GettingFocus(UIElement sender, Windows.UI.Xaml.Input.GettingFocusEventArgs args)
        {
            FrameworkElement oldFE = args.OldFocusedElement as FrameworkElement;
            string oldFEName = (oldFE == null) ? "null" : oldFE.Name;
            FrameworkElement newFE = args.NewFocusedElement as FrameworkElement;
            string newFEName = (newFE == null) ? "null" : newFE.Name;

            AppendAsyncEventMessage($"ScrollView.GettingFocus FocusState={args.FocusState}, Direction={args.Direction}, InputDevice={args.InputDevice}, oldFE={oldFEName}, newFE={newFEName}");
        }

        private void ScrollView_LostFocus(object sender, RoutedEventArgs e)
        {
            AppendAsyncEventMessage("ScrollView.LostFocus");
        }

        private void ScrollView_LosingFocus(UIElement sender, Windows.UI.Xaml.Input.LosingFocusEventArgs args)
        {
            FrameworkElement oldFE = args.OldFocusedElement as FrameworkElement;
            string oldFEName = (oldFE == null) ? "null" : oldFE.Name;
            FrameworkElement newFE = args.NewFocusedElement as FrameworkElement;
            string newFEName = (newFE == null) ? "null" : newFE.Name;

            AppendAsyncEventMessage($"ScrollView.LosingFocus FocusState={args.FocusState}, Direction={args.Direction}, InputDevice={args.InputDevice}, oldFE={oldFEName}, newFE={newFEName}");
        }

        private void ScrollView_GotFocus(object sender, RoutedEventArgs e)
        {
            AppendAsyncEventMessage("ScrollView.GotFocus");
        }

        private void ScrollPresenter_Loaded(object sender, RoutedEventArgs e)
        {
            AppendAsyncEventMessage($"ScrollPresenter.Loaded");
            LogScrollPresenterInfo();
            if (chkLogScrollViewEvents.IsChecked == true)
            {
                LogScrollViewInfo();
            }
        }

        private void ScrollPresenter_SizeChanged(object sender, SizeChangedEventArgs e)
        {
            AppendAsyncEventMessage($"ScrollPresenter.SizeChanged Size={scrollView.ActualWidth}, {scrollView.ActualHeight}");
            LogScrollPresenterInfo();
            if (chkLogScrollViewEvents.IsChecked == true)
            {
                LogScrollViewInfo();
            }
        }

        private void ScrollPresenter_ExtentChanged(ScrollPresenter sender, object args)
        {
            AppendAsyncEventMessage("ScrollPresenter.ExtentChanged ExtentWidth={sender.ExtentWidth}, ExtentHeight={sender.ExtentHeight}");
        }

        private void ScrollPresenter_StateChanged(ScrollPresenter sender, object args)
        {
            AppendAsyncEventMessage($"ScrollPresenter.StateChanged {sender.State.ToString()}");
        }

        private void ScrollPresenter_ViewChanged(ScrollPresenter sender, object args)
        {
            AppendAsyncEventMessage($"ScrollPresenter.ViewChanged HorizontalOffset={sender.HorizontalOffset.ToString()}, VerticalOffset={sender.VerticalOffset}, ZoomFactor={sender.ZoomFactor}");
        }

        private void ScrollPresenter_ScrollAnimationStarting(ScrollPresenter sender, ScrollingScrollAnimationStartingEventArgs args)
        {
            AppendAsyncEventMessage($"ScrollPresenter.ScrollAnimationStarting OffsetsChangeCorrelationId={args.CorrelationId}, SP=({args.StartPosition.X}, {args.StartPosition.Y}), EP=({args.EndPosition.X}, {args.EndPosition.Y})");

            try
            {
                Vector3KeyFrameAnimation stockKeyFrameAnimation = args.Animation as Vector3KeyFrameAnimation;

                if (stockKeyFrameAnimation != null)
                {
                    txtStockOffsetsChangeDuration.Text = stockKeyFrameAnimation.Duration.TotalMilliseconds.ToString();

                    if (!string.IsNullOrWhiteSpace(txtOverriddenOffsetsChangeDuration.Text))
                    {
                        double durationOverride = Convert.ToDouble(txtOverriddenOffsetsChangeDuration.Text);
                        stockKeyFrameAnimation.Duration = TimeSpan.FromMilliseconds(durationOverride);
                    }
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstLogs.Items.Add(ex.ToString());
            }
        }

        private void ScrollPresenter_ZoomAnimationStarting(ScrollPresenter sender, ScrollingZoomAnimationStartingEventArgs args)
        {
            AppendAsyncEventMessage($"ScrollPresenter.ZoomAnimationStarting ZoomFactorChangeCorrelationId={args.CorrelationId}, CenterPoint={args.CenterPoint}, SZF={args.StartZoomFactor}, EZF={args.EndZoomFactor}");
        }

        private void ScrollPresenter_ScrollCompleted(ScrollPresenter sender, ScrollingScrollCompletedEventArgs args)
        {
            AppendAsyncEventMessage($"ScrollPresenter.ScrollCompleted OffsetsChangeCorrelationId={args.CorrelationId}");
        }

        private void ScrollPresenter_ZoomCompleted(ScrollPresenter sender, ScrollingZoomCompletedEventArgs args)
        {
            AppendAsyncEventMessage($"ScrollPresenter.ZoomCompleted ZoomFactorChangeCorrelationId={args.CorrelationId}");
        }

        private void ScrollPresenter_BringingIntoView(ScrollPresenter sender, ScrollingBringingIntoViewEventArgs args)
        {
            string asyncEventMessage = "BringingIntoView ScrollPresenter=" + sender.Name;
            asyncEventMessage += ", TargetHorizontalOffset=" + args.TargetHorizontalOffset + ", TargetVerticalOffset=" + args.TargetVerticalOffset;
            asyncEventMessage += ", OffsetsChangeCorrelationId=" + args.CorrelationId;
            asyncEventMessage += ", SnapPointsMode=" + args.SnapPointsMode;
            asyncEventMessage += ", RequestEventArgs.AnimationDesired=" + args.RequestEventArgs.AnimationDesired + ", RequestEventArgs.Handled=" + args.RequestEventArgs.Handled;
            asyncEventMessage += ", RequestEventArgs.HorizontalAlignmentRatio=" + args.RequestEventArgs.HorizontalAlignmentRatio + ", RequestEventArgs.VerticalAlignmentRatio=" + args.RequestEventArgs.VerticalAlignmentRatio;
            asyncEventMessage += ", RequestEventArgs.HorizontalOffset=" + args.RequestEventArgs.HorizontalOffset + ", RequestEventArgs.VerticalOffset=" + args.RequestEventArgs.VerticalOffset;
            asyncEventMessage += ", RequestEventArgs.TargetRect=" + args.RequestEventArgs.TargetRect + ", RequestEventArgs.TargetElement=" + (args.RequestEventArgs.TargetElement as FrameworkElement).Name;
            asyncEventMessage += ", OriginalSource=";
            if (args.RequestEventArgs.OriginalSource == null)
                asyncEventMessage += "null";
            else
                asyncEventMessage += (args.RequestEventArgs.OriginalSource as FrameworkElement).Name;
            AppendAsyncEventMessage(asyncEventMessage);

            if (chkCancelOperationInBringingIntoView.IsChecked == true)
            {
                args.Cancel = true;
            }
        }

        private void ScrollView_ExtentChanged(ScrollView sender, object args)
        {
            AppendAsyncEventMessage($"ScrollView.ExtentChanged ExtentWidth={sender.ExtentWidth}, ExtentHeight={sender.ExtentHeight}");
        }

        private void ScrollView_StateChanged(ScrollView sender, object args)
        {
            AppendAsyncEventMessage($"ScrollView.StateChanged {sender.State.ToString()}");
        }

        private void ScrollView_ViewChanged(ScrollView sender, object args)
        {
            AppendAsyncEventMessage($"ScrollView.ViewChanged HorizontalOffset={sender.HorizontalOffset.ToString()}, VerticalOffset={sender.VerticalOffset}, ZoomFactor={sender.ZoomFactor}");
        }

        private void ScrollView_ScrollAnimationStarting(ScrollView sender, ScrollingScrollAnimationStartingEventArgs args)
        {
            AppendAsyncEventMessage($"ScrollView.ScrollAnimationStarting OffsetsChangeCorrelationId={args.CorrelationId}");
        }

        private void ScrollView_ZoomAnimationStarting(ScrollView sender, ScrollingZoomAnimationStartingEventArgs args)
        {
            AppendAsyncEventMessage($"ScrollView.ZoomAnimationStarting ZoomFactorChangeCorrelationId={args.CorrelationId}, CenterPoint={args.CenterPoint}");
        }

        private void ScrollView_ScrollCompleted(ScrollView sender, ScrollingScrollCompletedEventArgs args)
        {
            AppendAsyncEventMessage($"ScrollView.ScrollCompleted OffsetsChangeCorrelationId={args.CorrelationId}");
        }

        private void ScrollView_ZoomCompleted(ScrollView sender, ScrollingZoomCompletedEventArgs args)
        {
            AppendAsyncEventMessage($"ScrollView.ZoomCompleted ZoomFactorChangeCorrelationId={args.CorrelationId}");
        }

        private void ScrollView_BringingIntoView(ScrollView sender, ScrollingBringingIntoViewEventArgs args)
        {
            string asyncEventMessage = "BringingIntoView ScrollPresenter=" + sender.Name;
            asyncEventMessage += ", TargetHorizontalOffset=" + args.TargetHorizontalOffset + ", TargetVerticalOffset=" + args.TargetVerticalOffset;
            asyncEventMessage += ", OffsetsChangeCorrelationId=" + args.CorrelationId;
            asyncEventMessage += ", SnapPointsMode=" + args.SnapPointsMode;
            asyncEventMessage += ", RequestEventArgs.AnimationDesired=" + args.RequestEventArgs.AnimationDesired + ", RequestEventArgs.Handled=" + args.RequestEventArgs.Handled;
            asyncEventMessage += ", RequestEventArgs.HorizontalAlignmentRatio=" + args.RequestEventArgs.HorizontalAlignmentRatio + ", RequestEventArgs.VerticalAlignmentRatio=" + args.RequestEventArgs.VerticalAlignmentRatio;
            asyncEventMessage += ", RequestEventArgs.HorizontalOffset=" + args.RequestEventArgs.HorizontalOffset + ", RequestEventArgs.VerticalOffset=" + args.RequestEventArgs.VerticalOffset;
            asyncEventMessage += ", RequestEventArgs.TargetRect=" + args.RequestEventArgs.TargetRect + ", RequestEventArgs.TargetElement=" + (args.RequestEventArgs.TargetElement as FrameworkElement).Name;
            asyncEventMessage += ", OriginalSource=";
            if (args.RequestEventArgs.OriginalSource == null)
                asyncEventMessage += "null";
            else
                asyncEventMessage += (args.RequestEventArgs.OriginalSource as FrameworkElement).Name;
            AppendAsyncEventMessage(asyncEventMessage);

            if (chkCancelOperationInBringingIntoView.IsChecked == true)
            {
                args.Cancel = true;
            }
        }        

        private void LogScrollPresenterInfo()
        {
            ScrollPresenter scrollPresenter = ScrollViewTestHooks.GetScrollPresenterPart(scrollView);

            AppendAsyncEventMessage($"ScrollPresenter Info: HorizontalOffset={scrollPresenter.HorizontalOffset}, VerticalOffset={scrollPresenter.VerticalOffset}, ZoomFactor={scrollPresenter.ZoomFactor}");
            AppendAsyncEventMessage($"ScrollPresenter Info: ViewportWidth={scrollPresenter.ViewportWidth}, ExtentHeight={scrollPresenter.ViewportHeight}");
            AppendAsyncEventMessage($"ScrollPresenter Info: ExtentWidth={scrollPresenter.ExtentWidth}, ExtentHeight={scrollPresenter.ExtentHeight}");
            AppendAsyncEventMessage($"ScrollPresenter Info: ScrollableWidth={scrollPresenter.ScrollableWidth}, ScrollableHeight={scrollPresenter.ScrollableHeight}");
        }

        private void LogScrollViewInfo()
        {
            AppendAsyncEventMessage($"ScrollView Info: HorizontalOffset={scrollView.HorizontalOffset}, VerticalOffset={scrollView.VerticalOffset}, ZoomFactor={scrollView.ZoomFactor}");
            AppendAsyncEventMessage($"ScrollView Info: ViewportWidth={scrollView.ViewportWidth}, ExtentHeight={scrollView.ViewportHeight}");
            AppendAsyncEventMessage($"ScrollView Info: ExtentWidth={scrollView.ExtentWidth}, ExtentHeight={scrollView.ExtentHeight}");
            AppendAsyncEventMessage($"ScrollView Info: ScrollableWidth={scrollView.ScrollableWidth}, ScrollableHeight={scrollView.ScrollableHeight}");
        }

        private void BtnClearLogs_Click(object sender, RoutedEventArgs e)
        {
            lstLogs.Items.Clear();
        }

        private void FrameworkElement_BringIntoViewRequested(UIElement sender, BringIntoViewRequestedEventArgs args)
        {
            string asyncEventMessage = "BringIntoViewRequested Sender=" + (sender as FrameworkElement).Name;
            asyncEventMessage += ", AnimationDesired=" + args.AnimationDesired + ", Handled=" + args.Handled;
            asyncEventMessage += ", HorizontalAlignmentRatio=" + args.HorizontalAlignmentRatio + ", VerticalAlignmentRatio=" + args.VerticalAlignmentRatio;
            asyncEventMessage += ", HorizontalOffset=" + args.HorizontalOffset + ", VerticalOffset=" + args.VerticalOffset;
            asyncEventMessage += ", TargetRect=" + args.TargetRect + ", TargetElement=" + (args.TargetElement as FrameworkElement).Name;
            asyncEventMessage += ", OriginalSource=";
            if (args.OriginalSource == null)
                asyncEventMessage += "null";
            else
                asyncEventMessage += (args.OriginalSource as FrameworkElement).Name;
            AppendAsyncEventMessage(asyncEventMessage);
        }

        private void ChkLogScrollPresenterEvents_Checked(object sender, RoutedEventArgs e)
        {
            if (scrollView != null)
            {
                ScrollPresenter scrollPresenter = ScrollViewTestHooks.GetScrollPresenterPart(scrollView);

                if (scrollPresenter != null)
                {
                    scrollPresenter.Loaded += ScrollPresenter_Loaded;
                    scrollPresenter.SizeChanged += ScrollPresenter_SizeChanged;
                    scrollPresenter.ExtentChanged += ScrollPresenter_ExtentChanged;
                    scrollPresenter.StateChanged += ScrollPresenter_StateChanged;
                    scrollPresenter.ViewChanged += ScrollPresenter_ViewChanged;
                    scrollPresenter.ScrollAnimationStarting += ScrollPresenter_ScrollAnimationStarting;
                    scrollPresenter.ZoomAnimationStarting += ScrollPresenter_ZoomAnimationStarting;
                    scrollPresenter.ScrollCompleted += ScrollPresenter_ScrollCompleted;
                    scrollPresenter.ZoomCompleted += ScrollPresenter_ZoomCompleted;
                    scrollPresenter.BringingIntoView += ScrollPresenter_BringingIntoView;
                    scrollPresenter.BringIntoViewRequested += FrameworkElement_BringIntoViewRequested;
                }
            }
        }

        private void ChkLogScrollPresenterEvents_Unchecked(object sender, RoutedEventArgs e)
        {
            if (scrollView != null)
            {
                ScrollPresenter scrollPresenter = ScrollViewTestHooks.GetScrollPresenterPart(scrollView);

                if (scrollPresenter != null)
                {
                    scrollPresenter.Loaded -= ScrollPresenter_Loaded;
                    scrollPresenter.SizeChanged -= ScrollPresenter_SizeChanged;
                    scrollPresenter.ExtentChanged -= ScrollPresenter_ExtentChanged;
                    scrollPresenter.StateChanged -= ScrollPresenter_StateChanged;
                    scrollPresenter.ViewChanged -= ScrollPresenter_ViewChanged;
                    scrollPresenter.ScrollAnimationStarting -= ScrollPresenter_ScrollAnimationStarting;
                    scrollPresenter.ZoomAnimationStarting -= ScrollPresenter_ZoomAnimationStarting;
                    scrollPresenter.ScrollCompleted -= ScrollPresenter_ScrollCompleted;
                    scrollPresenter.ZoomCompleted -= ScrollPresenter_ZoomCompleted;
                    scrollPresenter.BringingIntoView -= ScrollPresenter_BringingIntoView;
                    scrollPresenter.BringIntoViewRequested -= FrameworkElement_BringIntoViewRequested;
                }
            }
        }

        private void ChkLogScrollViewEvents_Checked(object sender, RoutedEventArgs e)
        {
            if (scrollView != null)
            {
                scrollView.GettingFocus += ScrollView_GettingFocus;
                scrollView.GotFocus += ScrollView_GotFocus;
                scrollView.LosingFocus += ScrollView_LosingFocus;
                scrollView.LostFocus += ScrollView_LostFocus;
                scrollView.Loaded += ScrollView_Loaded;
                scrollView.SizeChanged += ScrollView_SizeChanged;
                scrollView.ExtentChanged += ScrollView_ExtentChanged;
                scrollView.StateChanged += ScrollView_StateChanged;
                scrollView.ViewChanged += ScrollView_ViewChanged;
                scrollView.ScrollAnimationStarting += ScrollView_ScrollAnimationStarting;
                scrollView.ZoomAnimationStarting += ScrollView_ZoomAnimationStarting;
                scrollView.ScrollCompleted += ScrollView_ScrollCompleted;
                scrollView.ZoomCompleted += ScrollView_ZoomCompleted;
                scrollView.BringingIntoView += ScrollView_BringingIntoView;
                scrollView.BringIntoViewRequested += FrameworkElement_BringIntoViewRequested;
            }
        }

        private void ChkLogScrollViewEvents_Unchecked(object sender, RoutedEventArgs e)
        {
            if (scrollView != null)
            {
                scrollView.GettingFocus -= ScrollView_GettingFocus;
                scrollView.GotFocus -= ScrollView_GotFocus;
                scrollView.LosingFocus -= ScrollView_LosingFocus;
                scrollView.LostFocus -= ScrollView_LostFocus;
                scrollView.Loaded -= ScrollView_Loaded;
                scrollView.SizeChanged -= ScrollView_SizeChanged;
                scrollView.ExtentChanged -= ScrollView_ExtentChanged;
                scrollView.StateChanged -= ScrollView_StateChanged;
                scrollView.ViewChanged -= ScrollView_ViewChanged;
                scrollView.ScrollAnimationStarting -= ScrollView_ScrollAnimationStarting;
                scrollView.ZoomAnimationStarting -= ScrollView_ZoomAnimationStarting;
                scrollView.ScrollCompleted -= ScrollView_ScrollCompleted;
                scrollView.ZoomCompleted -= ScrollView_ZoomCompleted;
                scrollView.BringingIntoView -= ScrollView_BringingIntoView;
                scrollView.BringIntoViewRequested -= FrameworkElement_BringIntoViewRequested;
            }
        }

        private void ChkLogScrollPresenterMessages_Checked(object sender, RoutedEventArgs e)
        {
            MUXControlsTestHooks.SetLoggingLevelForType("ScrollPresenter", isLoggingInfoLevel: true, isLoggingVerboseLevel: true);
            if (chkLogScrollViewMessages.IsChecked == false)
                MUXControlsTestHooks.LoggingMessage += MUXControlsTestHooks_LoggingMessage;
        }

        private void ChkLogScrollPresenterMessages_Unchecked(object sender, RoutedEventArgs e)
        {
            MUXControlsTestHooks.SetLoggingLevelForType("ScrollPresenter", isLoggingInfoLevel: false, isLoggingVerboseLevel: false);
            if (chkLogScrollViewMessages.IsChecked == false)
                MUXControlsTestHooks.LoggingMessage -= MUXControlsTestHooks_LoggingMessage;
        }

        private void ChkLogScrollViewMessages_Checked(object sender, RoutedEventArgs e)
        {
            MUXControlsTestHooks.SetLoggingLevelForType("ScrollView", isLoggingInfoLevel: true, isLoggingVerboseLevel: true);
            if (chkLogScrollPresenterMessages.IsChecked == false)
                MUXControlsTestHooks.LoggingMessage += MUXControlsTestHooks_LoggingMessage;
        }

        private void ChkLogScrollViewMessages_Unchecked(object sender, RoutedEventArgs e)
        {
            MUXControlsTestHooks.SetLoggingLevelForType("ScrollView", isLoggingInfoLevel: false, isLoggingVerboseLevel: false);
            if (chkLogScrollPresenterMessages.IsChecked == false)
                MUXControlsTestHooks.LoggingMessage -= MUXControlsTestHooks_LoggingMessage;
        }

        private void MUXControlsTestHooks_LoggingMessage(object sender, MUXControlsTestHooksLoggingMessageEventArgs args)
        {
            // Cut off the terminating new line.
            string msg = args.Message.Substring(0, args.Message.Length - 1);
            string asyncEventMessage;
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
            }

            if (args.IsVerboseLevel)
            {
                asyncEventMessage = "Verbose: " + senderName + "m:" + msg;
            }
            else
            {
                asyncEventMessage = "Info: " + senderName + "m:" + msg;
            }

            AppendAsyncEventMessage(asyncEventMessage);
        }

        private void AppendAsyncEventMessage(string asyncEventMessage)
        {
            lock (asyncEventReportingLock)
            {
                while (asyncEventMessage.Length > 0)
                {
                    string msgHead = asyncEventMessage;

                    if (asyncEventMessage.Length > 110)
                    {
                        int commaIndex = asyncEventMessage.IndexOf(',', 110);
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
                    lstLogs.Items.Add(asyncEventMessage);
                }
                lstAsyncEventMessage.Clear();
            }
        }

        private void BtnClearExceptionReport_Click(object sender, RoutedEventArgs e)
        {
            txtExceptionReport.Text = string.Empty;
        }
    }
}
