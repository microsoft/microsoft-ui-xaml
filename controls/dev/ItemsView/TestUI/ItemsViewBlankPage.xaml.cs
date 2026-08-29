// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.Generic;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Private.Controls;

namespace MUXControlsTestApp
{
    public sealed partial class ItemsViewBlankPage : TestPage
    {
        private object asyncEventReportingLock = new object();
        private List<string> lstAsyncEventMessage = new List<string>();

        public ItemsViewBlankPage()
        {
            this.InitializeComponent();

            if (chkLogItemsViewMessages.IsChecked == true || chkLogScrollViewMessages.IsChecked == true || chkLogItemsRepeaterMessages.IsChecked == true)
            {
                MUXControlsTestHooks.LoggingMessage += MUXControlsTestHooks_LoggingMessage;

                if (chkLogItemsRepeaterMessages.IsChecked == true)
                {
                    MUXControlsTestHooks.SetLoggingLevelForType("ItemsRepeater", isLoggingInfoLevel: true, isLoggingVerboseLevel: true);
                }
                if (chkLogScrollViewMessages.IsChecked == true)
                {
                    MUXControlsTestHooks.SetLoggingLevelForType("ScrollView", isLoggingInfoLevel: true, isLoggingVerboseLevel: true);
                }
                if (chkLogItemsViewMessages.IsChecked == true)
                {
                    MUXControlsTestHooks.SetLoggingLevelForType("ItemsView", isLoggingInfoLevel: true, isLoggingVerboseLevel: true);
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

        private void ItemsView_Loaded(object sender, RoutedEventArgs e)
        {
            AppendAsyncEventMessage($"ItemsView.Loaded");
            if (chkLogItemsRepeaterEvents.IsChecked == true)
            {
                LogItemsRepeaterInfo();
            }
            if (chkLogScrollViewEvents.IsChecked == true)
            {
                LogScrollViewInfo();
            }
            LogItemsViewInfo();
        }

        private void ItemsView_SizeChanged(object sender, SizeChangedEventArgs e)
        {
            AppendAsyncEventMessage($"ItemsView.SizeChanged Size={itemsView.ActualWidth}, {itemsView.ActualHeight}");
            if (chkLogItemsRepeaterEvents.IsChecked == true)
            {
                LogItemsRepeaterInfo();
            }
            if (chkLogScrollViewEvents.IsChecked == true)
            {
                LogScrollViewInfo();
            }
            LogItemsViewInfo();
        }

        private void ItemsView_GettingFocus(UIElement sender, Microsoft.UI.Xaml.Input.GettingFocusEventArgs args)
        {
            FrameworkElement oldFE = args.OldFocusedElement as FrameworkElement;
            string oldFEName = (oldFE == null) ? "null" : oldFE.Name;
            FrameworkElement newFE = args.NewFocusedElement as FrameworkElement;
            string newFEName = (newFE == null) ? "null" : newFE.Name;

            AppendAsyncEventMessage($"ItemsView.GettingFocus FocusState={args.FocusState}, Direction={args.Direction}, InputDevice={args.InputDevice}, oldFE={oldFEName}, newFE={newFEName}");
        }

        private void ItemsView_LostFocus(object sender, RoutedEventArgs e)
        {
            AppendAsyncEventMessage("ItemsView.LostFocus");
        }

        private void ItemsView_LosingFocus(UIElement sender, Microsoft.UI.Xaml.Input.LosingFocusEventArgs args)
        {
            FrameworkElement oldFE = args.OldFocusedElement as FrameworkElement;
            string oldFEName = (oldFE == null) ? "null" : oldFE.Name;
            FrameworkElement newFE = args.NewFocusedElement as FrameworkElement;
            string newFEName = (newFE == null) ? "null" : newFE.Name;

            AppendAsyncEventMessage($"ItemsView.LosingFocus FocusState={args.FocusState}, Direction={args.Direction}, InputDevice={args.InputDevice}, oldFE={oldFEName}, newFE={newFEName}");
        }

        private void ItemsView_GotFocus(object sender, RoutedEventArgs e)
        {
            AppendAsyncEventMessage("ItemsView.GotFocus");
        }

        private void ItemsRepeater_Loaded(object sender, RoutedEventArgs e)
        {
            AppendAsyncEventMessage($"ItemsRepeater.Loaded");
            LogItemsRepeaterInfo();
            if (chkLogItemsViewEvents.IsChecked == true)
            {
                LogItemsViewInfo();
            }
        }

        private void ItemsRepeater_SizeChanged(object sender, SizeChangedEventArgs e)
        {
            AppendAsyncEventMessage($"ItemsRepeater.SizeChanged Size={itemsView.ActualWidth}, {itemsView.ActualHeight}");
            LogItemsRepeaterInfo();
            if (chkLogItemsViewEvents.IsChecked == true)
            {
                LogItemsViewInfo();
            }
        }

        private void ScrollView_Loaded(object sender, RoutedEventArgs e)
        {
            AppendAsyncEventMessage($"ScrollView.Loaded");
            LogScrollViewInfo();
            if (chkLogItemsViewEvents.IsChecked == true)
            {
                LogItemsViewInfo();
            }
        }

        private void ScrollView_SizeChanged(object sender, SizeChangedEventArgs e)
        {
            AppendAsyncEventMessage($"ScrollView.SizeChanged Size={itemsView.ActualWidth}, {itemsView.ActualHeight}");
            LogScrollViewInfo();
            if (chkLogItemsViewEvents.IsChecked == true)
            {
                LogItemsViewInfo();
            }
        }

        private void ScrollView_ExtentChanged(ScrollView sender, object args)
        {
            AppendAsyncEventMessage("ScrollView.ExtentChanged ExtentWidth={sender.ExtentWidth}, ExtentHeight={sender.ExtentHeight}");
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
            AppendAsyncEventMessage($"ScrollView.ScrollAnimationStarting OffsetsChangeCorrelationId={args.CorrelationId}, SP=({args.StartPosition.X}, {args.StartPosition.Y}), EP=({args.EndPosition.X}, {args.EndPosition.Y})");
        }

        private void ScrollView_ZoomAnimationStarting(ScrollView sender, ScrollingZoomAnimationStartingEventArgs args)
        {
            AppendAsyncEventMessage($"ScrollView.ZoomAnimationStarting ZoomFactorChangeCorrelationId={args.CorrelationId}, CenterPoint={args.CenterPoint}, SZF={args.StartZoomFactor}, EZF={args.EndZoomFactor}");
        }

        private void ScrollView_ScrollCompleted(ScrollView sender, ScrollingScrollCompletedEventArgs args)
        {
            AppendAsyncEventMessage($"ScrollView.ScrollCompleted OffsetsChangeCorrelationId={args.CorrelationId}");
        }

        private void ScrollView_ZoomCompleted(ScrollView sender, ScrollingZoomCompletedEventArgs args)
        {
            AppendAsyncEventMessage($"ScrollView.ZoomCompleted ZoomFactorChangeCorrelationId={args.CorrelationId}");
        }

        private void LogItemsRepeaterInfo()
        {
            ItemsRepeater itemsRepeater = ItemsViewTestHooks.GetItemsRepeaterPart(itemsView);

            AppendAsyncEventMessage($"ItemsRepeater Info: ItemsSource={itemsRepeater.ItemsSource}, ItemTemplate={itemsRepeater.ItemTemplate}, Layout={itemsRepeater.Layout}");
        }

        private void LogScrollViewInfo()
        {
            ScrollView scrollView = ItemsViewTestHooks.GetScrollViewPart(itemsView);

            AppendAsyncEventMessage($"ScrollView Info: HorizontalOffset={scrollView.HorizontalOffset}, VerticalOffset={scrollView.VerticalOffset}, ZoomFactor={scrollView.ZoomFactor}");
            AppendAsyncEventMessage($"ScrollView Info: ViewportWidth={scrollView.ViewportWidth}, ExtentHeight={scrollView.ViewportHeight}");
            AppendAsyncEventMessage($"ScrollView Info: ExtentWidth={scrollView.ExtentWidth}, ExtentHeight={scrollView.ExtentHeight}");
            AppendAsyncEventMessage($"ScrollView Info: ScrollableWidth={scrollView.ScrollableWidth}, ScrollableHeight={scrollView.ScrollableHeight}");
        }

        private void LogItemsViewInfo()
        {
            //AppendAsyncEventMessage($"ItemsView Info: ItemsSource={itemsView.ItemsSource}, ItemTemplate={itemsView.ItemTemplate}, Layout={itemsView.Layout}");
        }

        private void BtnClearLogs_Click(object sender, RoutedEventArgs e)
        {
            lstLogs.Items.Clear();
        }

        private void ChkLogItemsRepeaterEvents_Checked(object sender, RoutedEventArgs e)
        {
            if (itemsView != null)
            {
                ItemsRepeater itemsRepeater = ItemsViewTestHooks.GetItemsRepeaterPart(itemsView);

                if (itemsRepeater != null)
                {
                    itemsRepeater.Loaded += ItemsRepeater_Loaded;
                    itemsRepeater.SizeChanged += ItemsRepeater_SizeChanged;
                }
            }
        }

        private void ChkLogItemsRepeaterEvents_Unchecked(object sender, RoutedEventArgs e)
        {
            if (itemsView != null)
            {
                ItemsRepeater itemsRepeater = ItemsViewTestHooks.GetItemsRepeaterPart(itemsView);

                if (itemsRepeater != null)
                {
                    itemsRepeater.Loaded -= ItemsRepeater_Loaded;
                    itemsRepeater.SizeChanged -= ItemsRepeater_SizeChanged;
                }
            }
        }

        private void ChkLogScrollViewEvents_Checked(object sender, RoutedEventArgs e)
        {
            if (itemsView != null)
            {
                ScrollView scrollView = ItemsViewTestHooks.GetScrollViewPart(itemsView);

                if (scrollView != null)
                {
                    scrollView.Loaded += ScrollView_Loaded;
                    scrollView.SizeChanged += ScrollView_SizeChanged;
                    scrollView.ExtentChanged += ScrollView_ExtentChanged;
                    scrollView.StateChanged += ScrollView_StateChanged;
                    scrollView.ViewChanged += ScrollView_ViewChanged;
                    scrollView.ScrollAnimationStarting += ScrollView_ScrollAnimationStarting;
                    scrollView.ZoomAnimationStarting += ScrollView_ZoomAnimationStarting;
                    scrollView.ScrollCompleted += ScrollView_ScrollCompleted;
                    scrollView.ZoomCompleted += ScrollView_ZoomCompleted;
                }
            }
        }

        private void ChkLogScrollViewEvents_Unchecked(object sender, RoutedEventArgs e)
        {
            if (itemsView != null)
            {
                ScrollView scrollView = ItemsViewTestHooks.GetScrollViewPart(itemsView);

                if (scrollView != null)
                {
                    scrollView.Loaded -= ScrollView_Loaded;
                    scrollView.SizeChanged -= ScrollView_SizeChanged;
                    scrollView.ExtentChanged -= ScrollView_ExtentChanged;
                    scrollView.StateChanged -= ScrollView_StateChanged;
                    scrollView.ViewChanged -= ScrollView_ViewChanged;
                    scrollView.ScrollAnimationStarting -= ScrollView_ScrollAnimationStarting;
                    scrollView.ZoomAnimationStarting -= ScrollView_ZoomAnimationStarting;
                    scrollView.ScrollCompleted -= ScrollView_ScrollCompleted;
                    scrollView.ZoomCompleted -= ScrollView_ZoomCompleted;
                }
            }
        }

        private void ChkLogItemsViewEvents_Checked(object sender, RoutedEventArgs e)
        {
            if (itemsView != null)
            {
                itemsView.GettingFocus += ItemsView_GettingFocus;
                itemsView.GotFocus += ItemsView_GotFocus;
                itemsView.LosingFocus += ItemsView_LosingFocus;
                itemsView.LostFocus += ItemsView_LostFocus;
                itemsView.Loaded += ItemsView_Loaded;
                itemsView.SizeChanged += ItemsView_SizeChanged;
            }
        }

        private void ChkLogItemsViewEvents_Unchecked(object sender, RoutedEventArgs e)
        {
            if (itemsView != null)
            {
                itemsView.GettingFocus -= ItemsView_GettingFocus;
                itemsView.GotFocus -= ItemsView_GotFocus;
                itemsView.LosingFocus -= ItemsView_LosingFocus;
                itemsView.LostFocus -= ItemsView_LostFocus;
                itemsView.Loaded -= ItemsView_Loaded;
                itemsView.SizeChanged -= ItemsView_SizeChanged;
            }
        }

        private void ChkLogItemsRepeaterMessages_Checked(object sender, RoutedEventArgs e)
        {
            MUXControlsTestHooks.SetLoggingLevelForType("ItemsRepeater", isLoggingInfoLevel: true, isLoggingVerboseLevel: true);
            if (chkLogItemsViewMessages.IsChecked == false && chkLogScrollViewMessages.IsChecked == false)
                MUXControlsTestHooks.LoggingMessage += MUXControlsTestHooks_LoggingMessage;
        }

        private void ChkLogItemsRepeaterMessages_Unchecked(object sender, RoutedEventArgs e)
        {
            MUXControlsTestHooks.SetLoggingLevelForType("ItemsRepeater", isLoggingInfoLevel: false, isLoggingVerboseLevel: false);
            if (chkLogItemsViewMessages.IsChecked == false && chkLogScrollViewMessages.IsChecked == false)
                MUXControlsTestHooks.LoggingMessage -= MUXControlsTestHooks_LoggingMessage;
        }

        private void ChkLogScrollViewMessages_Checked(object sender, RoutedEventArgs e)
        {
            MUXControlsTestHooks.SetLoggingLevelForType("ScrollView", isLoggingInfoLevel: true, isLoggingVerboseLevel: true);
            if (chkLogItemsViewMessages.IsChecked == false && chkLogItemsRepeaterMessages.IsChecked == false)
                MUXControlsTestHooks.LoggingMessage += MUXControlsTestHooks_LoggingMessage;
        }

        private void ChkLogScrollViewMessages_Unchecked(object sender, RoutedEventArgs e)
        {
            MUXControlsTestHooks.SetLoggingLevelForType("ScrollView", isLoggingInfoLevel: false, isLoggingVerboseLevel: false);
            if (chkLogItemsViewMessages.IsChecked == false && chkLogItemsRepeaterMessages.IsChecked == false)
                MUXControlsTestHooks.LoggingMessage -= MUXControlsTestHooks_LoggingMessage;
        }

        private void ChkLogItemsViewMessages_Checked(object sender, RoutedEventArgs e)
        {
            MUXControlsTestHooks.SetLoggingLevelForType("ItemsView", isLoggingInfoLevel: true, isLoggingVerboseLevel: true);
            if (chkLogScrollViewMessages.IsChecked == false && chkLogItemsRepeaterMessages.IsChecked == false)
                MUXControlsTestHooks.LoggingMessage += MUXControlsTestHooks_LoggingMessage;
        }

        private void ChkLogItemsViewMessages_Unchecked(object sender, RoutedEventArgs e)
        {
            MUXControlsTestHooks.SetLoggingLevelForType("ItemsView", isLoggingInfoLevel: false, isLoggingVerboseLevel: false);
            if (chkLogScrollViewMessages.IsChecked == false && chkLogItemsRepeaterMessages.IsChecked == false)
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
                var ignored = this.DispatcherQueue.TryEnqueue(Microsoft.UI.Dispatching.DispatcherQueuePriority.Normal, AppendAsyncEventMessage);

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
