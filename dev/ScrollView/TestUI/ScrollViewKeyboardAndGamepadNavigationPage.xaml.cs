// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Private.Controls;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using System;
using System.Collections.Generic;
using Windows.UI.Xaml;

namespace MUXControlsTestApp
{
    public sealed partial class ScrollViewerKeyboardAndGamepadNavigationPage : TestPage
    {
        private Object asyncEventReportingLock = new Object();
        private List<string> lstAsyncEventMessage = new List<string>();

        public ScrollViewerKeyboardAndGamepadNavigationPage()
        {
            this.InitializeComponent();
        }

        private void Scroller_ExtentChanged(Scroller sender, object args)
        {
            AppendAsyncEventMessage("Scroller.ExtentChanged ExtentWidth=" + sender.ExtentWidth + ", ExtentHeight=" + sender.ExtentHeight);
        }

        private void Scroller_StateChanged(Scroller sender, object args)
        {
            AppendAsyncEventMessage("Scroller.StateChanged " + sender.State.ToString());
        }

        private void Scroller_ViewChanged(Scroller sender, object args)
        {
            AppendAsyncEventMessage("Scroller.ViewChanged H=" + sender.HorizontalOffset.ToString() + ", V=" + sender.VerticalOffset + ", ZF=" + sender.ZoomFactor);
        }

        private void Scroller_ScrollAnimationStarting(Scroller sender, ScrollAnimationStartingEventArgs args)
        {
            AppendAsyncEventMessage("Scroller.ScrollAnimationStarting OffsetsChangeId=" + args.ScrollInfo.OffsetsChangeId + " SP=(" + args.StartPosition.X + "," + args.StartPosition.Y + ") EP=(" + args.EndPosition.X + "," + args.EndPosition.Y + ")");
        }

        private void Scroller_ZoomAnimationStarting(Scroller sender, ZoomAnimationStartingEventArgs args)
        {
            AppendAsyncEventMessage("Scroller.ZoomAnimationStarting ZoomFactorChangeId=" + args.ZoomInfo.ZoomFactorChangeId + ", CenterPoint=" + args.CenterPoint + ", SZF=" + args.StartZoomFactor + ", EZF=" + args.EndZoomFactor);
        }

        private void ScrollViewer_GettingFocus(UIElement sender, Windows.UI.Xaml.Input.GettingFocusEventArgs args)
        {
            FrameworkElement oldFE = args.OldFocusedElement as FrameworkElement;
            string oldFEName = (oldFE == null) ? "null" : oldFE.Name;
            FrameworkElement newFE = args.NewFocusedElement as FrameworkElement;
            string newFEName = (newFE == null) ? "null" : newFE.Name;

            AppendAsyncEventMessage("ScrollViewer.GettingFocus FocusState=" + args.FocusState + ", Direction=" + args.Direction + ", InputDevice=" + args.InputDevice + ", oldFE=" + oldFEName + ", newFE=" + newFEName);
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

            AppendAsyncEventMessage("ScrollViewer.LosingFocus FocusState=" + args.FocusState + ", Direction=" + args.Direction + ", InputDevice=" + args.InputDevice + ", oldFE=" + oldFEName + ", newFE=" + newFEName);
        }

        private void ScrollViewer_GotFocus(object sender, RoutedEventArgs e)
        {
            AppendAsyncEventMessage("ScrollViewer.GotFocus");
        }

        private void ScrollViewer_ExtentChanged(ScrollViewer sender, object args)
        {
            AppendAsyncEventMessage("ScrollViewer.ExtentChanged ExtentWidth=" + sender.ExtentWidth + ", ExtentHeight=" + sender.ExtentHeight);
        }

        private void ScrollViewer_StateChanged(ScrollViewer sender, object args)
        {
            AppendAsyncEventMessage("ScrollViewer.StateChanged " + sender.State.ToString());
        }

        private void ScrollViewer_ViewChanged(ScrollViewer sender, object args)
        {
            AppendAsyncEventMessage("ScrollViewer.ViewChanged H=" + sender.HorizontalOffset.ToString() + ", V=" + sender.VerticalOffset + ", ZF=" + sender.ZoomFactor);
        }

        private void ScrollViewer_ScrollAnimationStarting(ScrollViewer sender, ScrollAnimationStartingEventArgs args)
        {
            AppendAsyncEventMessage("ScrollViewer.ScrollAnimationStarting OffsetsChangeId=" + args.ScrollInfo.OffsetsChangeId);
        }

        private void ScrollViewer_ZoomAnimationStarting(ScrollViewer sender, ZoomAnimationStartingEventArgs args)
        {
            AppendAsyncEventMessage("ScrollViewer.ZoomAnimationStarting ZoomFactorChangeId=" + args.ZoomInfo.ZoomFactorChangeId + ", CenterPoint=" + args.CenterPoint);
        }

        private void BtnClearLogs_Click(object sender, RoutedEventArgs e)
        {
            lstLogs.Items.Clear();
        }

        private void ChkLogScrollerEvents_Checked(object sender, RoutedEventArgs e)
        {
            if (muxScrollViewer != null)
            {
                Scroller scroller = ScrollViewerTestHooks.GetScrollerPart(muxScrollViewer);

                if (scroller != null)
                {
                    scroller.ExtentChanged += Scroller_ExtentChanged;
                    scroller.StateChanged += Scroller_StateChanged;
                    scroller.ViewChanged += Scroller_ViewChanged;
                    scroller.ScrollAnimationStarting += Scroller_ScrollAnimationStarting;
                    scroller.ZoomAnimationStarting += Scroller_ZoomAnimationStarting;
                }
            }
        }

        private void ChkLogScrollerEvents_Unchecked(object sender, RoutedEventArgs e)
        {
            if (muxScrollViewer != null)
            {
                Scroller scroller = ScrollViewerTestHooks.GetScrollerPart(muxScrollViewer);

                if (scroller != null)
                {
                    scroller.ExtentChanged -= Scroller_ExtentChanged;
                    scroller.StateChanged -= Scroller_StateChanged;
                    scroller.ViewChanged -= Scroller_ViewChanged;
                    scroller.ScrollAnimationStarting -= Scroller_ScrollAnimationStarting;
                    scroller.ZoomAnimationStarting -= Scroller_ZoomAnimationStarting;
                }
            }
        }

        private void ChkLogScrollViewerEvents_Checked(object sender, RoutedEventArgs e)
        {
            if (muxScrollViewer != null)
            {
                muxScrollViewer.GettingFocus += ScrollViewer_GettingFocus;
                muxScrollViewer.GotFocus += ScrollViewer_GotFocus;
                muxScrollViewer.LosingFocus += ScrollViewer_LosingFocus;
                muxScrollViewer.LostFocus += ScrollViewer_LostFocus;
                muxScrollViewer.ExtentChanged += ScrollViewer_ExtentChanged;
                muxScrollViewer.StateChanged += ScrollViewer_StateChanged;
                muxScrollViewer.ViewChanged += ScrollViewer_ViewChanged;
                muxScrollViewer.ScrollAnimationStarting += ScrollViewer_ScrollAnimationStarting;
                muxScrollViewer.ZoomAnimationStarting += ScrollViewer_ZoomAnimationStarting;
            }
        }

        private void ChkLogScrollViewerEvents_Unchecked(object sender, RoutedEventArgs e)
        {
            if (muxScrollViewer != null)
            {
                muxScrollViewer.GettingFocus -= ScrollViewer_GettingFocus;
                muxScrollViewer.GotFocus -= ScrollViewer_GotFocus;
                muxScrollViewer.LosingFocus -= ScrollViewer_LosingFocus;
                muxScrollViewer.LostFocus -= ScrollViewer_LostFocus;
                muxScrollViewer.ExtentChanged -= ScrollViewer_ExtentChanged;
                muxScrollViewer.StateChanged -= ScrollViewer_StateChanged;
                muxScrollViewer.ViewChanged -= ScrollViewer_ViewChanged;
                muxScrollViewer.ScrollAnimationStarting -= ScrollViewer_ScrollAnimationStarting;
                muxScrollViewer.ZoomAnimationStarting -= ScrollViewer_ZoomAnimationStarting;
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
            if ((chkLogScrollerMessages.IsChecked == false && sender is Scroller) ||
                (chkLogScrollViewerMessages.IsChecked == false && sender is ScrollViewer))
            {
                return;
            }

            // Cut off the terminating new line.
            string msg = args.Message.Substring(0, args.Message.Length - 1);
            string asyncEventMessage = string.Empty;
            string senderName = string.Empty;

            FrameworkElement fe = sender as FrameworkElement;

            if (fe != null)
            {
                senderName = "s:" + fe.Name + ", ";
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

                    if (asyncEventMessage.Length > 100)
                    {
                        int commaIndex = asyncEventMessage.IndexOf(',', 100);
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
    }
}
