// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.Generic;
using Windows.UI.Xaml;

using Scroller = Microsoft.UI.Xaml.Controls.Primitives.Scroller;
using ScrollViewer = Microsoft.UI.Xaml.Controls.ScrollViewer;
using ScrollAnimationStartingEventArgs = Microsoft.UI.Xaml.Controls.ScrollAnimationStartingEventArgs;
using ZoomAnimationStartingEventArgs = Microsoft.UI.Xaml.Controls.ZoomAnimationStartingEventArgs;
using ScrollCompletedEventArgs = Microsoft.UI.Xaml.Controls.ScrollCompletedEventArgs;
using ZoomCompletedEventArgs = Microsoft.UI.Xaml.Controls.ZoomCompletedEventArgs;
using MUXControlsTestHooks = Microsoft.UI.Private.Controls.MUXControlsTestHooks;
using MUXControlsTestHooksLoggingMessageEventArgs = Microsoft.UI.Private.Controls.MUXControlsTestHooksLoggingMessageEventArgs;
using ScrollViewerTestHooks = Microsoft.UI.Private.Controls.ScrollViewerTestHooks;

namespace MUXControlsTestApp
{
    public sealed partial class ScrollViewerBlankPage : TestPage
    {
        private object asyncEventReportingLock = new object();
        private List<string> lstAsyncEventMessage = new List<string>();

        public ScrollViewerBlankPage()
        {
            this.InitializeComponent();

            if (chkLogScrollViewerMessages.IsChecked == true || chkLogScrollerMessages.IsChecked == true)
            {
                MUXControlsTestHooks.LoggingMessage += MUXControlsTestHooks_LoggingMessage;

                if (chkLogScrollerMessages.IsChecked == true)
                {
                    MUXControlsTestHooks.SetLoggingLevelForType("Scroller", isLoggingInfoLevel: true, isLoggingVerboseLevel: true);
                }
                if (chkLogScrollViewerMessages.IsChecked == true)
                {
                    MUXControlsTestHooks.SetLoggingLevelForType("ScrollViewer", isLoggingInfoLevel: true, isLoggingVerboseLevel: true);
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

        private void ScrollViewer_Loaded(object sender, RoutedEventArgs e)
        {
            AppendAsyncEventMessage($"ScrollViewer.Loaded");
            if (chkLogScrollerEvents.IsChecked == true)
            {
                LogScrollerInfo();
            }
            LogScrollViewerInfo();
        }

        private void ScrollViewer_SizeChanged(object sender, SizeChangedEventArgs e)
        {
            AppendAsyncEventMessage($"ScrollViewer.SizeChanged Size={scrollViewer.ActualWidth}, {scrollViewer.ActualHeight}");
            if (chkLogScrollerEvents.IsChecked == true)
            {
                LogScrollerInfo();
            }
            LogScrollViewerInfo();
        }

        private void ScrollViewer_GettingFocus(UIElement sender, Windows.UI.Xaml.Input.GettingFocusEventArgs args)
        {
            FrameworkElement oldFE = args.OldFocusedElement as FrameworkElement;
            string oldFEName = (oldFE == null) ? "null" : oldFE.Name;
            FrameworkElement newFE = args.NewFocusedElement as FrameworkElement;
            string newFEName = (newFE == null) ? "null" : newFE.Name;

            AppendAsyncEventMessage($"ScrollViewer.GettingFocus FocusState={args.FocusState}, Direction={args.Direction}, InputDevice={args.InputDevice}, oldFE={oldFEName}, newFE={newFEName}");
        }

        private void ScrollViewer_LostFocus(object sender, RoutedEventArgs e)
        {
            AppendAsyncEventMessage("ScrollViewer.LostFocus");
        }

        private void ScrollViewer_LosingFocus(UIElement sender, Windows.UI.Xaml.Input.LosingFocusEventArgs args)
        {
            FrameworkElement oldFE = args.OldFocusedElement as FrameworkElement;
            string oldFEName = (oldFE == null) ? "null" : oldFE.Name;
            FrameworkElement newFE = args.NewFocusedElement as FrameworkElement;
            string newFEName = (newFE == null) ? "null" : newFE.Name;

            AppendAsyncEventMessage($"ScrollViewer.LosingFocus FocusState={args.FocusState}, Direction={args.Direction}, InputDevice={args.InputDevice}, oldFE={oldFEName}, newFE={newFEName}");
        }

        private void ScrollViewer_GotFocus(object sender, RoutedEventArgs e)
        {
            AppendAsyncEventMessage("ScrollViewer.GotFocus");
        }

        private void Scroller_Loaded(object sender, RoutedEventArgs e)
        {
            AppendAsyncEventMessage($"Scroller.Loaded");
            LogScrollerInfo();
            if (chkLogScrollViewerEvents.IsChecked == true)
            {
                LogScrollViewerInfo();
            }
        }

        private void Scroller_SizeChanged(object sender, SizeChangedEventArgs e)
        {
            AppendAsyncEventMessage($"Scroller.SizeChanged Size={scrollViewer.ActualWidth}, {scrollViewer.ActualHeight}");
            LogScrollerInfo();
            if (chkLogScrollViewerEvents.IsChecked == true)
            {
                LogScrollViewerInfo();
            }
        }

        private void Scroller_ExtentChanged(Scroller sender, object args)
        {
            AppendAsyncEventMessage("Scroller.ExtentChanged ExtentWidth={sender.ExtentWidth}, ExtentHeight={sender.ExtentHeight}");
        }

        private void Scroller_StateChanged(Scroller sender, object args)
        {
            AppendAsyncEventMessage($"Scroller.StateChanged {sender.State.ToString()}");
        }

        private void Scroller_ViewChanged(Scroller sender, object args)
        {
            AppendAsyncEventMessage($"Scroller.ViewChanged HorizontalOffset={sender.HorizontalOffset.ToString()}, VerticalOffset={sender.VerticalOffset}, ZoomFactor={sender.ZoomFactor}");
        }

        private void Scroller_ScrollAnimationStarting(Scroller sender, ScrollAnimationStartingEventArgs args)
        {
            AppendAsyncEventMessage($"Scroller.ScrollAnimationStarting OffsetsChangeId={args.ScrollInfo.OffsetsChangeId}, SP=({args.StartPosition.X}, {args.StartPosition.Y}), EP=({args.EndPosition.X}, {args.EndPosition.Y})");
        }

        private void Scroller_ZoomAnimationStarting(Scroller sender, ZoomAnimationStartingEventArgs args)
        {
            AppendAsyncEventMessage($"Scroller.ZoomAnimationStarting ZoomFactorChangeId={args.ZoomInfo.ZoomFactorChangeId}, CenterPoint={args.CenterPoint}, SZF={args.StartZoomFactor}, EZF={args.EndZoomFactor}");
        }

        private void Scroller_ScrollCompleted(Scroller sender, ScrollCompletedEventArgs args)
        {
            AppendAsyncEventMessage($"Scroller.ScrollCompleted OffsetsChangeId={args.ScrollInfo.OffsetsChangeId}");
        }

        private void Scroller_ZoomCompleted(Scroller sender, ZoomCompletedEventArgs args)
        {
            AppendAsyncEventMessage($"Scroller.ZoomCompleted ZoomFactorChangeId={args.ZoomInfo.ZoomFactorChangeId}");
        }

        private void ScrollViewer_ExtentChanged(ScrollViewer sender, object args)
        {
            AppendAsyncEventMessage($"ScrollViewer.ExtentChanged ExtentWidth={sender.ExtentWidth}, ExtentHeight={sender.ExtentHeight}");
        }

        private void ScrollViewer_StateChanged(ScrollViewer sender, object args)
        {
            AppendAsyncEventMessage($"ScrollViewer.StateChanged {sender.State.ToString()}");
        }

        private void ScrollViewer_ViewChanged(ScrollViewer sender, object args)
        {
            AppendAsyncEventMessage($"ScrollViewer.ViewChanged HorizontalOffset={sender.HorizontalOffset.ToString()}, VerticalOffset={sender.VerticalOffset}, ZoomFactor={sender.ZoomFactor}");
        }

        private void ScrollViewer_ScrollAnimationStarting(ScrollViewer sender, ScrollAnimationStartingEventArgs args)
        {
            AppendAsyncEventMessage($"ScrollViewer.ScrollAnimationStarting OffsetsChangeId={args.ScrollInfo.OffsetsChangeId}");
        }

        private void ScrollViewer_ZoomAnimationStarting(ScrollViewer sender, ZoomAnimationStartingEventArgs args)
        {
            AppendAsyncEventMessage($"ScrollViewer.ZoomAnimationStarting ZoomFactorChangeId={args.ZoomInfo.ZoomFactorChangeId}, CenterPoint={args.CenterPoint}");
        }

        private void ScrollViewer_ScrollCompleted(ScrollViewer sender, ScrollCompletedEventArgs args)
        {
            AppendAsyncEventMessage($"ScrollViewer.ScrollCompleted OffsetsChangeId={args.ScrollInfo.OffsetsChangeId}");
        }

        private void ScrollViewer_ZoomCompleted(ScrollViewer sender, ZoomCompletedEventArgs args)
        {
            AppendAsyncEventMessage($"ScrollViewer.ZoomCompleted ZoomFactorChangeId={args.ZoomInfo.ZoomFactorChangeId}");
        }

        private void LogScrollerInfo()
        {
            Scroller scroller = ScrollViewerTestHooks.GetScrollerPart(scrollViewer);

            AppendAsyncEventMessage($"Scroller Info: HorizontalOffset={scroller.HorizontalOffset}, VerticalOffset={scroller.VerticalOffset}, ZoomFactor={scroller.ZoomFactor}");
            AppendAsyncEventMessage($"Scroller Info: ViewportWidth={scroller.ViewportWidth}, ExtentHeight={scroller.ViewportHeight}");
            AppendAsyncEventMessage($"Scroller Info: ExtentWidth={scroller.ExtentWidth}, ExtentHeight={scroller.ExtentHeight}");
            AppendAsyncEventMessage($"Scroller Info: ScrollableWidth={scroller.ScrollableWidth}, ScrollableHeight={scroller.ScrollableHeight}");
        }

        private void LogScrollViewerInfo()
        {
            AppendAsyncEventMessage($"ScrollViewer Info: HorizontalOffset={scrollViewer.HorizontalOffset}, VerticalOffset={scrollViewer.VerticalOffset}, ZoomFactor={scrollViewer.ZoomFactor}");
            AppendAsyncEventMessage($"ScrollViewer Info: ViewportWidth={scrollViewer.ViewportWidth}, ExtentHeight={scrollViewer.ViewportHeight}");
            AppendAsyncEventMessage($"ScrollViewer Info: ExtentWidth={scrollViewer.ExtentWidth}, ExtentHeight={scrollViewer.ExtentHeight}");
            AppendAsyncEventMessage($"ScrollViewer Info: ScrollableWidth={scrollViewer.ScrollableWidth}, ScrollableHeight={scrollViewer.ScrollableHeight}");
        }

        private void BtnClearLogs_Click(object sender, RoutedEventArgs e)
        {
            lstLogs.Items.Clear();
        }

        private void ChkLogScrollerEvents_Checked(object sender, RoutedEventArgs e)
        {
            if (scrollViewer != null)
            {
                Scroller scroller = ScrollViewerTestHooks.GetScrollerPart(scrollViewer);

                if (scroller != null)
                {
                    scroller.Loaded += Scroller_Loaded;
                    scroller.SizeChanged += Scroller_SizeChanged;
                    scroller.ExtentChanged += Scroller_ExtentChanged;
                    scroller.StateChanged += Scroller_StateChanged;
                    scroller.ViewChanged += Scroller_ViewChanged;
                    scroller.ScrollAnimationStarting += Scroller_ScrollAnimationStarting;
                    scroller.ZoomAnimationStarting += Scroller_ZoomAnimationStarting;
                    scroller.ScrollCompleted += Scroller_ScrollCompleted;
                    scroller.ZoomCompleted += Scroller_ZoomCompleted;
                }
            }
        }

        private void ChkLogScrollerEvents_Unchecked(object sender, RoutedEventArgs e)
        {
            if (scrollViewer != null)
            {
                Scroller scroller = ScrollViewerTestHooks.GetScrollerPart(scrollViewer);

                if (scroller != null)
                {
                    scroller.Loaded -= Scroller_Loaded;
                    scroller.SizeChanged -= Scroller_SizeChanged;
                    scroller.ExtentChanged -= Scroller_ExtentChanged;
                    scroller.StateChanged -= Scroller_StateChanged;
                    scroller.ViewChanged -= Scroller_ViewChanged;
                    scroller.ScrollAnimationStarting -= Scroller_ScrollAnimationStarting;
                    scroller.ZoomAnimationStarting -= Scroller_ZoomAnimationStarting;
                    scroller.ScrollCompleted -= Scroller_ScrollCompleted;
                    scroller.ZoomCompleted -= Scroller_ZoomCompleted;
                }
            }
        }

        private void ChkLogScrollViewerEvents_Checked(object sender, RoutedEventArgs e)
        {
            if (scrollViewer != null)
            {
                scrollViewer.GettingFocus += ScrollViewer_GettingFocus;
                scrollViewer.GotFocus += ScrollViewer_GotFocus;
                scrollViewer.LosingFocus += ScrollViewer_LosingFocus;
                scrollViewer.LostFocus += ScrollViewer_LostFocus;
                scrollViewer.Loaded += ScrollViewer_Loaded;
                scrollViewer.SizeChanged += ScrollViewer_SizeChanged;
                scrollViewer.ExtentChanged += ScrollViewer_ExtentChanged;
                scrollViewer.StateChanged += ScrollViewer_StateChanged;
                scrollViewer.ViewChanged += ScrollViewer_ViewChanged;
                scrollViewer.ScrollAnimationStarting += ScrollViewer_ScrollAnimationStarting;
                scrollViewer.ZoomAnimationStarting += ScrollViewer_ZoomAnimationStarting;
                scrollViewer.ScrollCompleted += ScrollViewer_ScrollCompleted;
                scrollViewer.ZoomCompleted += ScrollViewer_ZoomCompleted;
            }
        }

        private void ChkLogScrollViewerEvents_Unchecked(object sender, RoutedEventArgs e)
        {
            if (scrollViewer != null)
            {
                scrollViewer.GettingFocus -= ScrollViewer_GettingFocus;
                scrollViewer.GotFocus -= ScrollViewer_GotFocus;
                scrollViewer.LosingFocus -= ScrollViewer_LosingFocus;
                scrollViewer.LostFocus -= ScrollViewer_LostFocus;
                scrollViewer.Loaded -= ScrollViewer_Loaded;
                scrollViewer.SizeChanged -= ScrollViewer_SizeChanged;
                scrollViewer.ExtentChanged -= ScrollViewer_ExtentChanged;
                scrollViewer.StateChanged -= ScrollViewer_StateChanged;
                scrollViewer.ViewChanged -= ScrollViewer_ViewChanged;
                scrollViewer.ScrollAnimationStarting -= ScrollViewer_ScrollAnimationStarting;
                scrollViewer.ZoomAnimationStarting -= ScrollViewer_ZoomAnimationStarting;
                scrollViewer.ScrollCompleted -= ScrollViewer_ScrollCompleted;
                scrollViewer.ZoomCompleted -= ScrollViewer_ZoomCompleted;
            }
        }

        private void ChkLogScrollerMessages_Checked(object sender, RoutedEventArgs e)
        {
            MUXControlsTestHooks.SetLoggingLevelForType("Scroller", isLoggingInfoLevel: true, isLoggingVerboseLevel: true);
            if (chkLogScrollViewerMessages.IsChecked == false)
                MUXControlsTestHooks.LoggingMessage += MUXControlsTestHooks_LoggingMessage;
        }

        private void ChkLogScrollerMessages_Unchecked(object sender, RoutedEventArgs e)
        {
            MUXControlsTestHooks.SetLoggingLevelForType("Scroller", isLoggingInfoLevel: false, isLoggingVerboseLevel: false);
            if (chkLogScrollViewerMessages.IsChecked == false)
                MUXControlsTestHooks.LoggingMessage -= MUXControlsTestHooks_LoggingMessage;
        }

        private void ChkLogScrollViewerMessages_Checked(object sender, RoutedEventArgs e)
        {
            MUXControlsTestHooks.SetLoggingLevelForType("ScrollViewer", isLoggingInfoLevel: true, isLoggingVerboseLevel: true);
            if (chkLogScrollerMessages.IsChecked == false)
                MUXControlsTestHooks.LoggingMessage += MUXControlsTestHooks_LoggingMessage;
        }

        private void ChkLogScrollViewerMessages_Unchecked(object sender, RoutedEventArgs e)
        {
            MUXControlsTestHooks.SetLoggingLevelForType("ScrollViewer", isLoggingInfoLevel: false, isLoggingVerboseLevel: false);
            if (chkLogScrollerMessages.IsChecked == false)
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
