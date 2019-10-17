// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.Generic;
using Windows.UI.Xaml;

using ScrollingPresenter = Microsoft.UI.Xaml.Controls.Primitives.ScrollingPresenter;
using ScrollingView = Microsoft.UI.Xaml.Controls.ScrollingView;
using ScrollAnimationStartingEventArgs = Microsoft.UI.Xaml.Controls.ScrollAnimationStartingEventArgs;
using ZoomAnimationStartingEventArgs = Microsoft.UI.Xaml.Controls.ZoomAnimationStartingEventArgs;
using ScrollCompletedEventArgs = Microsoft.UI.Xaml.Controls.ScrollCompletedEventArgs;
using ZoomCompletedEventArgs = Microsoft.UI.Xaml.Controls.ZoomCompletedEventArgs;
using MUXControlsTestHooks = Microsoft.UI.Private.Controls.MUXControlsTestHooks;
using MUXControlsTestHooksLoggingMessageEventArgs = Microsoft.UI.Private.Controls.MUXControlsTestHooksLoggingMessageEventArgs;
using ScrollingViewTestHooks = Microsoft.UI.Private.Controls.ScrollingViewTestHooks;

namespace MUXControlsTestApp
{
    public sealed partial class ScrollingViewBlankPage : TestPage
    {
        private object asyncEventReportingLock = new object();
        private List<string> lstAsyncEventMessage = new List<string>();

        public ScrollingViewBlankPage()
        {
            this.InitializeComponent();

            if (chkLogScrollingViewMessages.IsChecked == true || chkLogScrollingPresenterMessages.IsChecked == true)
            {
                MUXControlsTestHooks.LoggingMessage += MUXControlsTestHooks_LoggingMessage;

                if (chkLogScrollingPresenterMessages.IsChecked == true)
                {
                    MUXControlsTestHooks.SetLoggingLevelForType("ScrollingPresenter", isLoggingInfoLevel: true, isLoggingVerboseLevel: true);
                }
                if (chkLogScrollingViewMessages.IsChecked == true)
                {
                    MUXControlsTestHooks.SetLoggingLevelForType("ScrollingView", isLoggingInfoLevel: true, isLoggingVerboseLevel: true);
                }
            }
        }

        private void ChkCustomUI_Checked(object sender, RoutedEventArgs e)
        {
            if (grdCustomUI != null)
                grdCustomUI.Visibility = Visibility.Visible;
        }

        private void ChkCustomUI_Unchecked(object sender, RoutedEventArgs e)
        {
            if (grdCustomUI != null)
                grdCustomUI.Visibility = Visibility.Collapsed;
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

        private void ScrollingView_Loaded(object sender, RoutedEventArgs e)
        {
            AppendAsyncEventMessage($"ScrollingView.Loaded");
            if (chkLogScrollingPresenterEvents.IsChecked == true)
            {
                LogScrollingPresenterInfo();
            }
            LogScrollingViewInfo();
        }

        private void ScrollingView_SizeChanged(object sender, SizeChangedEventArgs e)
        {
            AppendAsyncEventMessage($"ScrollingView.SizeChanged Size={scrollingView.ActualWidth}, {scrollingView.ActualHeight}");
            if (chkLogScrollingPresenterEvents.IsChecked == true)
            {
                LogScrollingPresenterInfo();
            }
            LogScrollingViewInfo();
        }

        private void ScrollingView_GettingFocus(UIElement sender, Windows.UI.Xaml.Input.GettingFocusEventArgs args)
        {
            FrameworkElement oldFE = args.OldFocusedElement as FrameworkElement;
            string oldFEName = (oldFE == null) ? "null" : oldFE.Name;
            FrameworkElement newFE = args.NewFocusedElement as FrameworkElement;
            string newFEName = (newFE == null) ? "null" : newFE.Name;

            AppendAsyncEventMessage($"ScrollingView.GettingFocus FocusState={args.FocusState}, Direction={args.Direction}, InputDevice={args.InputDevice}, oldFE={oldFEName}, newFE={newFEName}");
        }

        private void ScrollingView_LostFocus(object sender, RoutedEventArgs e)
        {
            AppendAsyncEventMessage("ScrollingView.LostFocus");
        }

        private void ScrollingView_LosingFocus(UIElement sender, Windows.UI.Xaml.Input.LosingFocusEventArgs args)
        {
            FrameworkElement oldFE = args.OldFocusedElement as FrameworkElement;
            string oldFEName = (oldFE == null) ? "null" : oldFE.Name;
            FrameworkElement newFE = args.NewFocusedElement as FrameworkElement;
            string newFEName = (newFE == null) ? "null" : newFE.Name;

            AppendAsyncEventMessage($"ScrollingView.LosingFocus FocusState={args.FocusState}, Direction={args.Direction}, InputDevice={args.InputDevice}, oldFE={oldFEName}, newFE={newFEName}");
        }

        private void ScrollingView_GotFocus(object sender, RoutedEventArgs e)
        {
            AppendAsyncEventMessage("ScrollingView.GotFocus");
        }

        private void ScrollingPresenter_Loaded(object sender, RoutedEventArgs e)
        {
            AppendAsyncEventMessage($"ScrollingPresenter.Loaded");
            LogScrollingPresenterInfo();
            if (chkLogScrollingViewEvents.IsChecked == true)
            {
                LogScrollingViewInfo();
            }
        }

        private void ScrollingPresenter_SizeChanged(object sender, SizeChangedEventArgs e)
        {
            AppendAsyncEventMessage($"ScrollingPresenter.SizeChanged Size={scrollingView.ActualWidth}, {scrollingView.ActualHeight}");
            LogScrollingPresenterInfo();
            if (chkLogScrollingViewEvents.IsChecked == true)
            {
                LogScrollingViewInfo();
            }
        }

        private void ScrollingPresenter_ExtentChanged(ScrollingPresenter sender, object args)
        {
            AppendAsyncEventMessage("ScrollingPresenter.ExtentChanged ExtentWidth={sender.ExtentWidth}, ExtentHeight={sender.ExtentHeight}");
        }

        private void ScrollingPresenter_StateChanged(ScrollingPresenter sender, object args)
        {
            AppendAsyncEventMessage($"ScrollingPresenter.StateChanged {sender.State.ToString()}");
        }

        private void ScrollingPresenter_ViewChanged(ScrollingPresenter sender, object args)
        {
            AppendAsyncEventMessage($"ScrollingPresenter.ViewChanged HorizontalOffset={sender.HorizontalOffset.ToString()}, VerticalOffset={sender.VerticalOffset}, ZoomFactor={sender.ZoomFactor}");
        }

        private void ScrollingPresenter_ScrollAnimationStarting(ScrollingPresenter sender, ScrollAnimationStartingEventArgs args)
        {
            AppendAsyncEventMessage($"ScrollingPresenter.ScrollAnimationStarting OffsetsChangeId={args.ScrollInfo.OffsetsChangeId}, SP=({args.StartPosition.X}, {args.StartPosition.Y}), EP=({args.EndPosition.X}, {args.EndPosition.Y})");
        }

        private void ScrollingPresenter_ZoomAnimationStarting(ScrollingPresenter sender, ZoomAnimationStartingEventArgs args)
        {
            AppendAsyncEventMessage($"ScrollingPresenter.ZoomAnimationStarting ZoomFactorChangeId={args.ZoomInfo.ZoomFactorChangeId}, CenterPoint={args.CenterPoint}, SZF={args.StartZoomFactor}, EZF={args.EndZoomFactor}");
        }

        private void ScrollingPresenter_ScrollCompleted(ScrollingPresenter sender, ScrollCompletedEventArgs args)
        {
            AppendAsyncEventMessage($"ScrollingPresenter.ScrollCompleted OffsetsChangeId={args.ScrollInfo.OffsetsChangeId}");
        }

        private void ScrollingPresenter_ZoomCompleted(ScrollingPresenter sender, ZoomCompletedEventArgs args)
        {
            AppendAsyncEventMessage($"ScrollingPresenter.ZoomCompleted ZoomFactorChangeId={args.ZoomInfo.ZoomFactorChangeId}");
        }

        private void ScrollingView_ExtentChanged(ScrollingView sender, object args)
        {
            AppendAsyncEventMessage($"ScrollingView.ExtentChanged ExtentWidth={sender.ExtentWidth}, ExtentHeight={sender.ExtentHeight}");
        }

        private void ScrollingView_StateChanged(ScrollingView sender, object args)
        {
            AppendAsyncEventMessage($"ScrollingView.StateChanged {sender.State.ToString()}");
        }

        private void ScrollingView_ViewChanged(ScrollingView sender, object args)
        {
            AppendAsyncEventMessage($"ScrollingView.ViewChanged HorizontalOffset={sender.HorizontalOffset.ToString()}, VerticalOffset={sender.VerticalOffset}, ZoomFactor={sender.ZoomFactor}");
        }

        private void ScrollingView_ScrollAnimationStarting(ScrollingView sender, ScrollAnimationStartingEventArgs args)
        {
            AppendAsyncEventMessage($"ScrollingView.ScrollAnimationStarting OffsetsChangeId={args.ScrollInfo.OffsetsChangeId}");
        }

        private void ScrollingView_ZoomAnimationStarting(ScrollingView sender, ZoomAnimationStartingEventArgs args)
        {
            AppendAsyncEventMessage($"ScrollingView.ZoomAnimationStarting ZoomFactorChangeId={args.ZoomInfo.ZoomFactorChangeId}, CenterPoint={args.CenterPoint}");
        }

        private void ScrollingView_ScrollCompleted(ScrollingView sender, ScrollCompletedEventArgs args)
        {
            AppendAsyncEventMessage($"ScrollingView.ScrollCompleted OffsetsChangeId={args.ScrollInfo.OffsetsChangeId}");
        }

        private void ScrollingView_ZoomCompleted(ScrollingView sender, ZoomCompletedEventArgs args)
        {
            AppendAsyncEventMessage($"ScrollingView.ZoomCompleted ZoomFactorChangeId={args.ZoomInfo.ZoomFactorChangeId}");
        }

        private void LogScrollingPresenterInfo()
        {
            ScrollingPresenter scrollingPresenter = ScrollingViewTestHooks.GetScrollingPresenterPart(scrollingView);

            AppendAsyncEventMessage($"ScrollingPresenter Info: HorizontalOffset={scrollingPresenter.HorizontalOffset}, VerticalOffset={scrollingPresenter.VerticalOffset}, ZoomFactor={scrollingPresenter.ZoomFactor}");
            AppendAsyncEventMessage($"ScrollingPresenter Info: ViewportWidth={scrollingPresenter.ViewportWidth}, ExtentHeight={scrollingPresenter.ViewportHeight}");
            AppendAsyncEventMessage($"ScrollingPresenter Info: ExtentWidth={scrollingPresenter.ExtentWidth}, ExtentHeight={scrollingPresenter.ExtentHeight}");
            AppendAsyncEventMessage($"ScrollingPresenter Info: ScrollableWidth={scrollingPresenter.ScrollableWidth}, ScrollableHeight={scrollingPresenter.ScrollableHeight}");
        }

        private void LogScrollingViewInfo()
        {
            AppendAsyncEventMessage($"ScrollingView Info: HorizontalOffset={scrollingView.HorizontalOffset}, VerticalOffset={scrollingView.VerticalOffset}, ZoomFactor={scrollingView.ZoomFactor}");
            AppendAsyncEventMessage($"ScrollingView Info: ViewportWidth={scrollingView.ViewportWidth}, ExtentHeight={scrollingView.ViewportHeight}");
            AppendAsyncEventMessage($"ScrollingView Info: ExtentWidth={scrollingView.ExtentWidth}, ExtentHeight={scrollingView.ExtentHeight}");
            AppendAsyncEventMessage($"ScrollingView Info: ScrollableWidth={scrollingView.ScrollableWidth}, ScrollableHeight={scrollingView.ScrollableHeight}");
        }

        private void BtnClearLogs_Click(object sender, RoutedEventArgs e)
        {
            lstLogs.Items.Clear();
        }

        private void ChkLogScrollingPresenterEvents_Checked(object sender, RoutedEventArgs e)
        {
            if (scrollingView != null)
            {
                ScrollingPresenter scrollingPresenter = ScrollingViewTestHooks.GetScrollingPresenterPart(scrollingView);

                if (scrollingPresenter != null)
                {
                    scrollingPresenter.Loaded += ScrollingPresenter_Loaded;
                    scrollingPresenter.SizeChanged += ScrollingPresenter_SizeChanged;
                    scrollingPresenter.ExtentChanged += ScrollingPresenter_ExtentChanged;
                    scrollingPresenter.StateChanged += ScrollingPresenter_StateChanged;
                    scrollingPresenter.ViewChanged += ScrollingPresenter_ViewChanged;
                    scrollingPresenter.ScrollAnimationStarting += ScrollingPresenter_ScrollAnimationStarting;
                    scrollingPresenter.ZoomAnimationStarting += ScrollingPresenter_ZoomAnimationStarting;
                    scrollingPresenter.ScrollCompleted += ScrollingPresenter_ScrollCompleted;
                    scrollingPresenter.ZoomCompleted += ScrollingPresenter_ZoomCompleted;
                }
            }
        }

        private void ChkLogScrollingPresenterEvents_Unchecked(object sender, RoutedEventArgs e)
        {
            if (scrollingView != null)
            {
                ScrollingPresenter scrollingPresenter = ScrollingViewTestHooks.GetScrollingPresenterPart(scrollingView);

                if (scrollingPresenter != null)
                {
                    scrollingPresenter.Loaded -= ScrollingPresenter_Loaded;
                    scrollingPresenter.SizeChanged -= ScrollingPresenter_SizeChanged;
                    scrollingPresenter.ExtentChanged -= ScrollingPresenter_ExtentChanged;
                    scrollingPresenter.StateChanged -= ScrollingPresenter_StateChanged;
                    scrollingPresenter.ViewChanged -= ScrollingPresenter_ViewChanged;
                    scrollingPresenter.ScrollAnimationStarting -= ScrollingPresenter_ScrollAnimationStarting;
                    scrollingPresenter.ZoomAnimationStarting -= ScrollingPresenter_ZoomAnimationStarting;
                    scrollingPresenter.ScrollCompleted -= ScrollingPresenter_ScrollCompleted;
                    scrollingPresenter.ZoomCompleted -= ScrollingPresenter_ZoomCompleted;
                }
            }
        }

        private void ChkLogScrollingViewEvents_Checked(object sender, RoutedEventArgs e)
        {
            if (scrollingView != null)
            {
                scrollingView.GettingFocus += ScrollingView_GettingFocus;
                scrollingView.GotFocus += ScrollingView_GotFocus;
                scrollingView.LosingFocus += ScrollingView_LosingFocus;
                scrollingView.LostFocus += ScrollingView_LostFocus;
                scrollingView.Loaded += ScrollingView_Loaded;
                scrollingView.SizeChanged += ScrollingView_SizeChanged;
                scrollingView.ExtentChanged += ScrollingView_ExtentChanged;
                scrollingView.StateChanged += ScrollingView_StateChanged;
                scrollingView.ViewChanged += ScrollingView_ViewChanged;
                scrollingView.ScrollAnimationStarting += ScrollingView_ScrollAnimationStarting;
                scrollingView.ZoomAnimationStarting += ScrollingView_ZoomAnimationStarting;
                scrollingView.ScrollCompleted += ScrollingView_ScrollCompleted;
                scrollingView.ZoomCompleted += ScrollingView_ZoomCompleted;
            }
        }

        private void ChkLogScrollingViewEvents_Unchecked(object sender, RoutedEventArgs e)
        {
            if (scrollingView != null)
            {
                scrollingView.GettingFocus -= ScrollingView_GettingFocus;
                scrollingView.GotFocus -= ScrollingView_GotFocus;
                scrollingView.LosingFocus -= ScrollingView_LosingFocus;
                scrollingView.LostFocus -= ScrollingView_LostFocus;
                scrollingView.Loaded -= ScrollingView_Loaded;
                scrollingView.SizeChanged -= ScrollingView_SizeChanged;
                scrollingView.ExtentChanged -= ScrollingView_ExtentChanged;
                scrollingView.StateChanged -= ScrollingView_StateChanged;
                scrollingView.ViewChanged -= ScrollingView_ViewChanged;
                scrollingView.ScrollAnimationStarting -= ScrollingView_ScrollAnimationStarting;
                scrollingView.ZoomAnimationStarting -= ScrollingView_ZoomAnimationStarting;
                scrollingView.ScrollCompleted -= ScrollingView_ScrollCompleted;
                scrollingView.ZoomCompleted -= ScrollingView_ZoomCompleted;
            }
        }

        private void ChkLogScrollingPresenterMessages_Checked(object sender, RoutedEventArgs e)
        {
            MUXControlsTestHooks.SetLoggingLevelForType("ScrollingPresenter", isLoggingInfoLevel: true, isLoggingVerboseLevel: true);
            if (chkLogScrollingViewMessages.IsChecked == false)
                MUXControlsTestHooks.LoggingMessage += MUXControlsTestHooks_LoggingMessage;
        }

        private void ChkLogScrollingPresenterMessages_Unchecked(object sender, RoutedEventArgs e)
        {
            MUXControlsTestHooks.SetLoggingLevelForType("ScrollingPresenter", isLoggingInfoLevel: false, isLoggingVerboseLevel: false);
            if (chkLogScrollingViewMessages.IsChecked == false)
                MUXControlsTestHooks.LoggingMessage -= MUXControlsTestHooks_LoggingMessage;
        }

        private void ChkLogScrollingViewMessages_Checked(object sender, RoutedEventArgs e)
        {
            MUXControlsTestHooks.SetLoggingLevelForType("ScrollingView", isLoggingInfoLevel: true, isLoggingVerboseLevel: true);
            if (chkLogScrollingPresenterMessages.IsChecked == false)
                MUXControlsTestHooks.LoggingMessage += MUXControlsTestHooks_LoggingMessage;
        }

        private void ChkLogScrollingViewMessages_Unchecked(object sender, RoutedEventArgs e)
        {
            MUXControlsTestHooks.SetLoggingLevelForType("ScrollingView", isLoggingInfoLevel: false, isLoggingVerboseLevel: false);
            if (chkLogScrollingPresenterMessages.IsChecked == false)
                MUXControlsTestHooks.LoggingMessage -= MUXControlsTestHooks_LoggingMessage;
        }

        private void MUXControlsTestHooks_LoggingMessage(object sender, MUXControlsTestHooksLoggingMessageEventArgs args)
        {
            // Cut off the terminating new line.
            string msg = args.Message.Substring(0, args.Message.Length - 1);
            string asyncEventMessage = string.Empty;
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
