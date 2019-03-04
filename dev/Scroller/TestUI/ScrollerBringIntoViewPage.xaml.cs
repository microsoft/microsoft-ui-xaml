// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Linq;
using Windows.Foundation;
using Windows.UI.Composition;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Navigation;
using Windows.UI.Xaml.Tests.MUXControls.ApiTests.RepeaterTests.Common;

#if !BUILD_WINDOWS
using Scroller = Microsoft.UI.Xaml.Controls.Primitives.Scroller;
using ScrollCompletedEventArgs = Microsoft.UI.Xaml.Controls.ScrollCompletedEventArgs;
using ScrollerBringingIntoViewEventArgs = Microsoft.UI.Xaml.Controls.ScrollerBringingIntoViewEventArgs;
using ScrollAnimationStartingEventArgs = Microsoft.UI.Xaml.Controls.ScrollAnimationStartingEventArgs;
using ItemsRepeater = Microsoft.UI.Xaml.Controls.ItemsRepeater;
using ItemsRepeaterElementPreparedEventArgs = Microsoft.UI.Xaml.Controls.ItemsRepeaterElementPreparedEventArgs;

using ScrollerTestHooks = Microsoft.UI.Private.Controls.ScrollerTestHooks;
using ScrollerViewChangeResult = Microsoft.UI.Private.Controls.ScrollerViewChangeResult;
using MUXControlsTestHooks = Microsoft.UI.Private.Controls.MUXControlsTestHooks;
using MUXControlsTestHooksLoggingMessageEventArgs = Microsoft.UI.Private.Controls.MUXControlsTestHooksLoggingMessageEventArgs;
#endif

namespace MUXControlsTestApp
{
    public sealed partial class ScrollerBringIntoViewPage : TestPage
    {
        private SampleDataSource dataSource = null;
        private Object asyncEventReportingLock = new Object();
        private List<string> lstAsyncEventMessage = new List<string>();

        public ScrollerBringIntoViewPage()
        {
            InitializeComponent();

            Loaded += ScrollerBringIntoViewPage_Loaded;

            dataSource = new SampleDataSource(
                this,
                Enumerable.Range(0, 16).Select(i => string.Format("Item{0}", i)).ToList());

#if BUILD_WINDOWS
            repeater1.ItemTemplate = (Windows.UI.Xaml.IElementFactory)repeater1ElementFactory;
#else
            repeater1.ItemTemplate = repeater1ElementFactory;
#endif
            repeater1.ItemsSource = dataSource;
            repeater1.ElementPrepared += Repeater_ElementPrepared;

#if BUILD_WINDOWS
            repeater2.ItemTemplate = (Windows.UI.Xaml.IElementFactory)repeater2ElementFactory;
#else
            repeater2.ItemTemplate = repeater2ElementFactory;
#endif
            repeater2.ItemsSource = dataSource;
            repeater2.ElementPrepared += Repeater_ElementPrepared;

#if BUILD_WINDOWS
            repeater3.ItemTemplate = (Windows.UI.Xaml.IElementFactory)repeater3ElementFactory;
#else
            repeater3.ItemTemplate = repeater3ElementFactory;
#endif
            repeater3.ItemsSource = dataSource;
            repeater3.ElementPrepared += Repeater_ElementPrepared;
        }

        private void Repeater_ElementPrepared(ItemsRepeater sender, ItemsRepeaterElementPreparedEventArgs args)
        {
            try
            {
                Border b = args.Element as Border;
                b.Name = "B" + args.Index;

                TextBox txt = b.Child as TextBox;
                txt.Name = txt.Text;

                if (args.Index == 7)
                {
                    b.Visibility = Visibility.Collapsed;
                }
            }
            catch
            {
            }
        }

        protected override void OnNavigatedFrom(NavigationEventArgs e)
        {
            MUXControlsTestHooks.SetLoggingLevelForType("Scroller", isLoggingInfoLevel: false, isLoggingVerboseLevel: false);
            MUXControlsTestHooks.LoggingMessage -= MUXControlsTestHooks_LoggingMessage;

            base.OnNavigatedFrom(e);
        }

        private void ScrollerBringIntoViewPage_Loaded(object sender, RoutedEventArgs e)
        {
            try
            {
                if (chkLogScrollerMessages.IsChecked == true)
                {
                    MUXControlsTestHooks.SetLoggingLevelForType("Scroller", isLoggingInfoLevel: true, isLoggingVerboseLevel: true);
                    MUXControlsTestHooks.LoggingMessage += MUXControlsTestHooks_LoggingMessage;
                }

                if (chkLogScrollerEvents.IsChecked == true)
                {
                    HookScrollerEvents(innerScroller);
                    HookScrollerEvents(innerScroller2);
                    HookScrollerEvents(outerScroller);
                    HookScrollerEvents(outerScroller2);
                    HookScrollViewerEvents(innerScrollViewer);
                    HookScrollViewerEvents(outerScrollViewer);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
            }
        }

        private void Scroller_BringingIntoView(Scroller sender, ScrollerBringingIntoViewEventArgs args)
        {
            string asyncEventMessage = "BringingIntoView Scroller=" + sender.Name;
            asyncEventMessage += ", TargetHorizontalOffset=" + args.TargetHorizontalOffset + ", TargetVerticalOffset=" + args.TargetVerticalOffset;
            asyncEventMessage += ", OffsetsChangeId=" + args.ScrollInfo.OffsetsChangeId;
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

        private void Scroller_ScrollAnimationStarting(Scroller sender, ScrollAnimationStartingEventArgs args)
        {
            try
            {
                if (chkLogBringIntoViewRequestedEvents.IsChecked == true)
                {
                    AppendAsyncEventMessage("ScrollAnimationStarting Scroller=" + sender.Name + ", OffsetsChangeId=" + args.ScrollInfo.OffsetsChangeId);
                }

                Vector3KeyFrameAnimation stockKeyFrameAnimation = args.Animation as Vector3KeyFrameAnimation;

                if (stockKeyFrameAnimation != null)
                {
                    TextBox txtStockOffsetsChangeDuration = null;
                    string overriddenOffsetsChangeDuration = string.Empty;

                    if (sender == innerScroller || sender == innerScroller2)
                    {
                        txtStockOffsetsChangeDuration = txtInnerStockOffsetsChangeDuration;
                        overriddenOffsetsChangeDuration = txtInnerOverriddenOffsetsChangeDuration.Text;
                    }
                    else
                    {
                        txtStockOffsetsChangeDuration = txtOuterStockOffsetsChangeDuration;
                        overriddenOffsetsChangeDuration = txtOuterOverriddenOffsetsChangeDuration.Text;
                    }

                    txtStockOffsetsChangeDuration.Text = stockKeyFrameAnimation.Duration.TotalMilliseconds.ToString();

                    if (!string.IsNullOrWhiteSpace(overriddenOffsetsChangeDuration))
                    {
                        double durationOverride = Convert.ToDouble(overriddenOffsetsChangeDuration);
                        stockKeyFrameAnimation.Duration = TimeSpan.FromMilliseconds(durationOverride);
                    }
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
            }
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

        private void Scroller_ExtentChanged(Scroller sender, object args)
        {
            AppendAsyncEventMessage("ExtentChanged Scroller=" + sender.Name + ", ExtentWidth=" + sender.ExtentWidth.ToString() + ", ExtentHeight=" + sender.ExtentHeight.ToString());
        }

        private void Scroller_StateChanged(Scroller sender, object args)
        {
            AppendAsyncEventMessage("StateChanged Scroller=" + sender.Name + ", State=" + sender.State.ToString());
        }

        private void Scroller_ViewChanged(Scroller sender, object args)
        {
            AppendAsyncEventMessage("ViewChanged Scroller=" + sender.Name + ", H=" + sender.HorizontalOffset.ToString() + ", V=" + sender.VerticalOffset.ToString() + ", ZF=" + sender.ZoomFactor.ToString());
        }

        private void Scroller_ScrollCompleted(Scroller sender, ScrollCompletedEventArgs args)
        {
            ScrollerViewChangeResult result = ScrollerTestHooks.GetScrollCompletedResult(args);

            AppendAsyncEventMessage("ScrollCompleted Scroller=" + sender.Name + ", OffsetsChangeId=" + args.ScrollInfo.OffsetsChangeId + ", Result=" + result);
        }

        private void ScrollViewer_ViewChanged(object sender, ScrollViewerViewChangedEventArgs e)
        {
            ScrollViewer sv = sender as ScrollViewer;
            AppendAsyncEventMessage("ViewChanged ScrollViewer=" + sv.Name + ", H=" + sv.HorizontalOffset + ", V=" + sv.VerticalOffset + ", S=" + sv.ZoomFactor);
        }

        private void BtnStartBringIntoView_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                txtInnerStockOffsetsChangeDuration.Text = string.Empty;
                txtOuterStockOffsetsChangeDuration.Text = string.Empty;

                FrameworkElement targetElement = null;
                ItemsRepeater repeater = null;

                switch (cmbNestingCombination.SelectedIndex)
                {
                    case 0: //ScrollerInScroller
                        repeater = repeater1;
                        break;

                    case 1: //ScrollerInScrollViewer
                        repeater = repeater2;
                        break;

                    case 2: //ScrollViewerInScroller
                        repeater = repeater3;
                        break;
                }

                Border border = repeater.TryGetElement(Convert.ToInt16(txtTargetElement.Text)) as Border;

                if (border != null)
                {
                    targetElement = border.Child as FrameworkElement;

                    if (targetElement != null)
                    {
                        BringIntoViewOptions options = new BringIntoViewOptions();

                        options.AnimationDesired = cmbAnimationDesired.SelectedIndex == 0;
                        options.HorizontalAlignmentRatio = Convert.ToDouble(txtHorizontalAlignmentRatio.Text);
                        options.VerticalAlignmentRatio = Convert.ToDouble(txtVerticalAlignmentRatio.Text);
                        options.HorizontalOffset = Convert.ToDouble(txtHorizontalOffset.Text);
                        options.VerticalOffset = Convert.ToDouble(txtVerticalOffset.Text);

                        if (chkLogBringIntoViewRequestedEvents.IsChecked == true)
                        {
                            AppendAsyncEventMessage("Invoking StartBringIntoView on " + targetElement.Name);
                        }
                        targetElement.StartBringIntoView(options);
                    }
                }

                if (targetElement == null && chkLogBringIntoViewRequestedEvents.IsChecked == true)
                {
                    AppendAsyncEventMessage("TargetElement not found");
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollerEvents.Items.Add(ex.ToString());
            }
        }

        private void CmbNestingCombination_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (outerScroller == null || outerScrollViewer == null || outerScroller2 == null) return;

            switch (cmbNestingCombination.SelectedIndex)
            {
                case 0: //ScrollerInScroller
                    outerScroller.Visibility = Visibility.Visible;
                    outerScrollViewer.Visibility = Visibility.Collapsed;
                    outerScroller2.Visibility = Visibility.Collapsed;
                    break;

                case 1: //ScrollerInScrollViewer
                    outerScroller.Visibility = Visibility.Collapsed;
                    outerScrollViewer.Visibility = Visibility.Visible;
                    outerScroller2.Visibility = Visibility.Collapsed;
                    break;

                case 2: //ScrollViewerInScroller
                    outerScroller.Visibility = Visibility.Collapsed;
                    outerScrollViewer.Visibility = Visibility.Collapsed;
                    outerScroller2.Visibility = Visibility.Visible;
                    break;
            }
        }

        private void ChkLogBringIntoViewRequestedEvents_Checked(object sender, RoutedEventArgs e)
        {
            HookBringIntoViewEvents();

            txtInnerStockOffsetsChangeDuration.IsEnabled = true;
            txtInnerOverriddenOffsetsChangeDuration.IsEnabled = true;
            txtOuterStockOffsetsChangeDuration.IsEnabled = true;
            txtOuterOverriddenOffsetsChangeDuration.IsEnabled = true;
            chkCancelOperationInBringingIntoView.IsEnabled = true;
        }

        private void ChkLogBringIntoViewRequestedEvents_Unchecked(object sender, RoutedEventArgs e)
        {
            UnhookBringIntoViewEvents();

            txtInnerStockOffsetsChangeDuration.IsEnabled = false;
            txtInnerOverriddenOffsetsChangeDuration.IsEnabled = false;
            txtOuterStockOffsetsChangeDuration.IsEnabled = false;
            txtOuterOverriddenOffsetsChangeDuration.IsEnabled = false;
            chkCancelOperationInBringingIntoView.IsEnabled = false;
        }

        private void HookBringIntoViewEvents()
        {
            if (innerScroller != null)
            {
                innerScroller.BringingIntoView += Scroller_BringingIntoView;
                innerScroller2.BringingIntoView += Scroller_BringingIntoView;
                outerScroller.BringingIntoView += Scroller_BringingIntoView;
                outerScroller2.BringingIntoView += Scroller_BringingIntoView;

                innerScroller.ScrollAnimationStarting += Scroller_ScrollAnimationStarting;
                innerScroller2.ScrollAnimationStarting += Scroller_ScrollAnimationStarting;
                outerScroller.ScrollAnimationStarting += Scroller_ScrollAnimationStarting;
                outerScroller2.ScrollAnimationStarting += Scroller_ScrollAnimationStarting;

                innerScroller.BringIntoViewRequested += FrameworkElement_BringIntoViewRequested;
                innerScroller2.BringIntoViewRequested += FrameworkElement_BringIntoViewRequested;
                outerScroller.BringIntoViewRequested += FrameworkElement_BringIntoViewRequested;
                outerScroller2.BringIntoViewRequested += FrameworkElement_BringIntoViewRequested;

                outerScrollViewer.BringIntoViewRequested += FrameworkElement_BringIntoViewRequested;
                innerScrollViewer.BringIntoViewRequested += FrameworkElement_BringIntoViewRequested;
            }
        }

        private void UnhookBringIntoViewEvents()
        {
            if (innerScroller != null)
            {
                innerScroller.BringingIntoView -= Scroller_BringingIntoView;
                innerScroller2.BringingIntoView -= Scroller_BringingIntoView;
                outerScroller.BringingIntoView -= Scroller_BringingIntoView;
                outerScroller2.BringingIntoView -= Scroller_BringingIntoView;

                innerScroller.ScrollAnimationStarting -= Scroller_ScrollAnimationStarting;
                innerScroller2.ScrollAnimationStarting -= Scroller_ScrollAnimationStarting;
                outerScroller.ScrollAnimationStarting -= Scroller_ScrollAnimationStarting;
                outerScroller2.ScrollAnimationStarting -= Scroller_ScrollAnimationStarting;

                innerScroller.BringIntoViewRequested -= FrameworkElement_BringIntoViewRequested;
                innerScroller2.BringIntoViewRequested -= FrameworkElement_BringIntoViewRequested;
                outerScroller.BringIntoViewRequested -= FrameworkElement_BringIntoViewRequested;
                outerScroller2.BringIntoViewRequested -= FrameworkElement_BringIntoViewRequested;

                outerScrollViewer.BringIntoViewRequested -= FrameworkElement_BringIntoViewRequested;
                innerScrollViewer.BringIntoViewRequested -= FrameworkElement_BringIntoViewRequested;
            }
        }

        private void ChkLogScrollerEvents_Checked(object sender, RoutedEventArgs e)
        {
            HookScrollerEvents(innerScroller);
            HookScrollerEvents(innerScroller2);
            HookScrollerEvents(outerScroller);
            HookScrollerEvents(outerScroller2);
            HookScrollViewerEvents(innerScrollViewer);
            HookScrollViewerEvents(outerScrollViewer);
        }

        private void ChkLogScrollerEvents_Unchecked(object sender, RoutedEventArgs e)
        {
            UnhookScrollerEvents(innerScroller);
            UnhookScrollerEvents(innerScroller2);
            UnhookScrollerEvents(outerScroller);
            UnhookScrollerEvents(outerScroller2);
            UnhookScrollViewerEvents(innerScrollViewer);
            UnhookScrollViewerEvents(outerScrollViewer);
        }

        private void HookScrollerEvents(Scroller scroller)
        {
            scroller.ExtentChanged += Scroller_ExtentChanged;
            scroller.StateChanged += Scroller_StateChanged;
            scroller.ViewChanged += Scroller_ViewChanged;
            scroller.ScrollCompleted += Scroller_ScrollCompleted;
        }

        private void UnhookScrollerEvents(Scroller scroller)
        {
            scroller.ExtentChanged -= Scroller_ExtentChanged;
            scroller.StateChanged -= Scroller_StateChanged;
            scroller.ViewChanged -= Scroller_ViewChanged;
            scroller.ScrollCompleted -= Scroller_ScrollCompleted;
        }

        private void HookScrollViewerEvents(ScrollViewer scrollViewer)
        {
            scrollViewer.ViewChanged += ScrollViewer_ViewChanged;
        }

        private void UnhookScrollViewerEvents(ScrollViewer scrollViewer)
        {
            scrollViewer.ViewChanged -= ScrollViewer_ViewChanged;
        }

        private void ChkLogScrollerMessages_Checked(object sender, RoutedEventArgs e)
        {
            MUXControlsTestHooks.SetLoggingLevelForType("Scroller", isLoggingInfoLevel: true, isLoggingVerboseLevel: true);
            MUXControlsTestHooks.LoggingMessage += MUXControlsTestHooks_LoggingMessage;
        }

        private void ChkLogScrollerMessages_Unchecked(object sender, RoutedEventArgs e)
        {
            MUXControlsTestHooks.SetLoggingLevelForType("Scroller", isLoggingInfoLevel: false, isLoggingVerboseLevel: false);
            MUXControlsTestHooks.LoggingMessage -= MUXControlsTestHooks_LoggingMessage;
        }

        private void BtnClearScrollerEvents_Click(object sender, RoutedEventArgs e)
        {
            lstScrollerEvents.Items.Clear();
        }

        private void BtnClearExceptionReport_Click(object sender, RoutedEventArgs e)
        {
            txtExceptionReport.Text = string.Empty;
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
                AppendAsyncEventMessage("Verbose: " + senderName + "m:" + msg);
            }
            else
            {
                AppendAsyncEventMessage("Info: " + senderName + "m:" + msg);
            }
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
                    lstScrollerEvents.Items.Add(asyncEventMessage);
                }
                lstAsyncEventMessage.Clear();
            }
        }

        private class SampleDataSource : CustomItemsSourceViewWithUniqueIdMapping
        {
            List<string> _inner;

            public SampleDataSource(ScrollerBringIntoViewPage owner, List<string> source)
            {
                Owner = owner;
                Inner = source;
            }

            protected override int GetSizeCore()
            {
                return Inner.Count;
            }

            public List<string> Inner
            {
                get
                {
                    return _inner;
                }

                set
                {
                    _inner = value;
                }
            }

            private ScrollerBringIntoViewPage Owner
            {
                get;
                set;
            }

            protected override object GetAtCore(int index)
            {
                return Inner[index];
            }

            protected override string KeyFromIndexCore(int index)
            {
                // data is the same as its unique id
                return Inner[index].ToString();
            }
        }
    }
}
