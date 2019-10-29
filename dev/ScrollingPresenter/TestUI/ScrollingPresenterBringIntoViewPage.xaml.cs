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
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Navigation;
using Windows.UI.Xaml.Tests.MUXControls.ApiTests.RepeaterTests.Common;

using ScrollingPresenter = Microsoft.UI.Xaml.Controls.Primitives.ScrollingPresenter;
using ScrollSnapPoint = Microsoft.UI.Xaml.Controls.Primitives.ScrollSnapPoint;
using ScrollingSnapPointsMode = Microsoft.UI.Xaml.Controls.ScrollingSnapPointsMode;
using ScrollSnapPointsAlignment = Microsoft.UI.Xaml.Controls.Primitives.ScrollSnapPointsAlignment;
using ScrollingScrollCompletedEventArgs = Microsoft.UI.Xaml.Controls.ScrollingScrollCompletedEventArgs;
using ScrollingBringingIntoViewEventArgs = Microsoft.UI.Xaml.Controls.ScrollingBringingIntoViewEventArgs;
using ScrollingScrollAnimationStartingEventArgs = Microsoft.UI.Xaml.Controls.ScrollingScrollAnimationStartingEventArgs;
using ItemsRepeater = Microsoft.UI.Xaml.Controls.ItemsRepeater;
using ItemsRepeaterElementPreparedEventArgs = Microsoft.UI.Xaml.Controls.ItemsRepeaterElementPreparedEventArgs;

using ScrollingPresenterTestHooks = Microsoft.UI.Private.Controls.ScrollingPresenterTestHooks;
using ScrollingPresenterViewChangeResult = Microsoft.UI.Private.Controls.ScrollingPresenterViewChangeResult;
using MUXControlsTestHooks = Microsoft.UI.Private.Controls.MUXControlsTestHooks;
using MUXControlsTestHooksLoggingMessageEventArgs = Microsoft.UI.Private.Controls.MUXControlsTestHooksLoggingMessageEventArgs;

namespace MUXControlsTestApp
{
    public sealed partial class ScrollingPresenterBringIntoViewPage : TestPage
    {
        private SampleDataSource dataSource = null;
        private Object asyncEventReportingLock = new Object();
        private List<string> lstAsyncEventMessage = new List<string>();

        public ScrollingPresenterBringIntoViewPage()
        {
            InitializeComponent();

            Loaded += ScrollingPresenterBringIntoViewPage_Loaded;

            dataSource = new SampleDataSource(
                this,
                Enumerable.Range(0, 16).Select(i => string.Format("Item{0}", i)).ToList());

            repeater1.ItemTemplate = repeater1ElementFactory;
            repeater1.ItemsSource = dataSource;
            repeater1.ElementPrepared += Repeater_ElementPrepared;

            repeater2.ItemTemplate = repeater2ElementFactory;
            repeater2.ItemsSource = dataSource;
            repeater2.ElementPrepared += Repeater_ElementPrepared;

            repeater3.ItemTemplate = repeater3ElementFactory;
            repeater3.ItemsSource = dataSource;
            repeater3.ElementPrepared += Repeater_ElementPrepared;

            repeater4.ItemTemplate = repeater4ElementFactory;
            repeater4.ItemsSource = dataSource;
            repeater4.ElementPrepared += Repeater_ElementPrepared;
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
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
            }
        }

        protected override void OnNavigatedFrom(NavigationEventArgs e)
        {
            MUXControlsTestHooks.SetLoggingLevelForType("ScrollingPresenter", isLoggingInfoLevel: false, isLoggingVerboseLevel: false);
            MUXControlsTestHooks.LoggingMessage -= MUXControlsTestHooks_LoggingMessage;

            base.OnNavigatedFrom(e);
        }

        private void ScrollingPresenterBringIntoViewPage_Loaded(object sender, RoutedEventArgs e)
        {
            try
            {
                if (chkLogScrollingPresenterMessages.IsChecked == true)
                {
                    MUXControlsTestHooks.SetLoggingLevelForType("ScrollingPresenter", isLoggingInfoLevel: true, isLoggingVerboseLevel: true);
                    MUXControlsTestHooks.LoggingMessage += MUXControlsTestHooks_LoggingMessage;
                }

                if (chkLogScrollingPresenterEvents.IsChecked == true)
                {
                    HookScrollingPresenterEvents(innerScrollingPresenter);
                    HookScrollingPresenterEvents(innerScrollingPresenter2);
                    HookScrollingPresenterEvents(innerScrollingPresenter3);
                    HookScrollingPresenterEvents(outerScrollingPresenter);
                    HookScrollingPresenterEvents(outerScrollingPresenter2);
                    HookScrollingPresenterEvents(outerScrollingPresenter3);
                    HookScrollViewerEvents(innerScrollViewer);
                    HookScrollViewerEvents(innerScrollViewer2);
                    HookScrollViewerEvents(outerScrollViewer);
                    HookScrollViewerEvents(outerScrollViewer2);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollingPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void ScrollingPresenter_BringingIntoView(ScrollingPresenter sender, ScrollingBringingIntoViewEventArgs args)
        {
            string asyncEventMessage = "BringingIntoView ScrollingPresenter=" + sender.Name;
            asyncEventMessage += ", TargetHorizontalOffset=" + args.TargetHorizontalOffset + ", TargetVerticalOffset=" + args.TargetVerticalOffset;
            asyncEventMessage += ", OffsetsChangeId=" + args.ScrollInfo.OffsetsChangeId;
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

            args.SnapPointsMode = chkIgnoreSnapPoints.IsChecked == true ? ScrollingSnapPointsMode.Ignore : ScrollingSnapPointsMode.Default;

            if (chkCancelOperationInBringingIntoView.IsChecked == true)
            {
                args.Cancel = true;
            }
        }

        private void ScrollingPresenter_ScrollAnimationStarting(ScrollingPresenter sender, ScrollingScrollAnimationStartingEventArgs args)
        {
            try
            {
                if (chkLogBringIntoViewRequestedEvents.IsChecked == true)
                {
                    AppendAsyncEventMessage("ScrollAnimationStarting ScrollingPresenter=" + sender.Name + ", OffsetsChangeId=" + args.ScrollInfo.OffsetsChangeId);
                }

                Vector3KeyFrameAnimation stockKeyFrameAnimation = args.Animation as Vector3KeyFrameAnimation;

                if (stockKeyFrameAnimation != null)
                {
                    TextBox txtStockOffsetsChangeDuration = null;
                    string overriddenOffsetsChangeDuration = string.Empty;

                    if (sender == innerScrollingPresenter || sender == innerScrollingPresenter2)
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
                lstScrollingPresenterEvents.Items.Add(ex.ToString());
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

        private void ScrollingPresenter_ExtentChanged(ScrollingPresenter sender, object args)
        {
            AppendAsyncEventMessage("ExtentChanged ScrollingPresenter=" + sender.Name + ", ExtentWidth=" + sender.ExtentWidth.ToString() + ", ExtentHeight=" + sender.ExtentHeight.ToString());
        }

        private void ScrollingPresenter_StateChanged(ScrollingPresenter sender, object args)
        {
            AppendAsyncEventMessage("StateChanged ScrollingPresenter=" + sender.Name + ", State=" + sender.State.ToString());
        }

        private void ScrollingPresenter_ViewChanged(ScrollingPresenter sender, object args)
        {
            AppendAsyncEventMessage("ViewChanged ScrollingPresenter=" + sender.Name + ", H=" + sender.HorizontalOffset.ToString() + ", V=" + sender.VerticalOffset.ToString() + ", ZF=" + sender.ZoomFactor.ToString());
        }

        private void ScrollingPresenter_ScrollCompleted(ScrollingPresenter sender, ScrollingScrollCompletedEventArgs args)
        {
            ScrollingPresenterViewChangeResult result = ScrollingPresenterTestHooks.GetScrollCompletedResult(args);

            AppendAsyncEventMessage("ScrollCompleted ScrollingPresenter=" + sender.Name + ", OffsetsChangeId=" + args.ScrollInfo.OffsetsChangeId + ", Result=" + result);
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
                    case 0: //ScrollingPresenterInScrollingPresenter (1)
                        repeater = repeater1;
                        break;

                    case 2: //ScrollingPresenterInScrollViewer
                        repeater = repeater2;
                        break;

                    case 3: //ScrollViewerInScrollingPresenter
                        repeater = repeater3;
                        break;

                    case 4: //ScrollViewerInScrollViewer
                        repeater = repeater4;
                        break;
                }

                Border border = null;

                if (repeater != null)
                {
                    border = repeater.TryGetElement(Convert.ToInt16(txtTargetElement.Text)) as Border;
                }
                else
                {
                    // case ScrollingPresenterInScrollingPresenter (2)
                    border = (innerScrollingPresenter3.Content as StackPanel).Children[Convert.ToInt16(txtTargetElement.Text)] as Border;
                }

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

                        if (chkIgnoreSnapPoints.IsChecked == false)
                        {
                            ScrollingPresenter scrollingPresenter1 = null;
                            ScrollingPresenter scrollingPresenter2 = null;

                            switch (cmbNestingCombination.SelectedIndex)
                            {
                                case 0: //ScrollingPresenterInScrollingPresenter (1)
                                    scrollingPresenter1 = outerScrollingPresenter;
                                    scrollingPresenter2 = innerScrollingPresenter;
                                    break;

                                case 1: //ScrollingPresenterInScrollingPresenter (2)
                                    scrollingPresenter1 = outerScrollingPresenter3;
                                    scrollingPresenter2 = innerScrollingPresenter3;
                                    break;

                                case 2: //ScrollingPresenterInScrollViewer
                                    scrollingPresenter1 = innerScrollingPresenter2;
                                    break;

                                case 3: //ScrollViewerInScrollingPresenter
                                    scrollingPresenter1 = outerScrollingPresenter2;
                                    break;
                            }

                            if (scrollingPresenter1 != null)
                            {
                                RefreshSnapPoints(scrollingPresenter1, scrollingPresenter1.Content as StackPanel);
                            }

                            if (scrollingPresenter2 != null)
                            {
                                RefreshSnapPoints(scrollingPresenter2, scrollingPresenter2.Content as StackPanel);
                            }
                        }

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
                lstScrollingPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void CmbNestingCombination_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (outerScrollingPresenter == null ||
                outerScrollingPresenter2 == null ||
                outerScrollingPresenter3 == null ||
                outerScrollViewer == null ||
                outerScrollViewer2 == null) return;

            switch (cmbNestingCombination.SelectedIndex)
            {
                case 0: //ScrollingPresenterInScrollingPresenter (1)
                    outerScrollingPresenter.Visibility = Visibility.Visible;
                    outerScrollViewer.Visibility = Visibility.Collapsed;
                    outerScrollingPresenter2.Visibility = Visibility.Collapsed;
                    outerScrollViewer2.Visibility = Visibility.Collapsed;
                    outerScrollingPresenter3.Visibility = Visibility.Collapsed;
                    break;

                case 1: //ScrollingPresenterInScrollingPresenter (2)
                    outerScrollingPresenter.Visibility = Visibility.Collapsed;
                    outerScrollViewer.Visibility = Visibility.Collapsed;
                    outerScrollingPresenter2.Visibility = Visibility.Collapsed;
                    outerScrollViewer2.Visibility = Visibility.Collapsed;
                    outerScrollingPresenter3.Visibility = Visibility.Visible;
                    break;

                case 2: //ScrollingPresenterInScrollViewer
                    outerScrollingPresenter.Visibility = Visibility.Collapsed;
                    outerScrollViewer.Visibility = Visibility.Visible;
                    outerScrollingPresenter2.Visibility = Visibility.Collapsed;
                    outerScrollViewer2.Visibility = Visibility.Collapsed;
                    outerScrollingPresenter3.Visibility = Visibility.Collapsed;
                    break;

                case 3: //ScrollViewerInScrollingPresenter
                    outerScrollingPresenter.Visibility = Visibility.Collapsed;
                    outerScrollViewer.Visibility = Visibility.Collapsed;
                    outerScrollingPresenter2.Visibility = Visibility.Visible;
                    outerScrollViewer2.Visibility = Visibility.Collapsed;
                    outerScrollingPresenter3.Visibility = Visibility.Collapsed;
                    break;

                case 4: //ScrollViewerInScrollViewer
                    outerScrollingPresenter.Visibility = Visibility.Collapsed;
                    outerScrollViewer.Visibility = Visibility.Collapsed;
                    outerScrollingPresenter2.Visibility = Visibility.Collapsed;
                    outerScrollViewer2.Visibility = Visibility.Visible;
                    outerScrollingPresenter3.Visibility = Visibility.Collapsed;
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
            if (innerScrollingPresenter != null)
            {
                innerScrollingPresenter.BringingIntoView += ScrollingPresenter_BringingIntoView;
                innerScrollingPresenter2.BringingIntoView += ScrollingPresenter_BringingIntoView;
                innerScrollingPresenter3.BringingIntoView += ScrollingPresenter_BringingIntoView;
                outerScrollingPresenter.BringingIntoView += ScrollingPresenter_BringingIntoView;
                outerScrollingPresenter2.BringingIntoView += ScrollingPresenter_BringingIntoView;
                outerScrollingPresenter3.BringingIntoView += ScrollingPresenter_BringingIntoView;

                innerScrollingPresenter.ScrollAnimationStarting += ScrollingPresenter_ScrollAnimationStarting;
                innerScrollingPresenter2.ScrollAnimationStarting += ScrollingPresenter_ScrollAnimationStarting;
                innerScrollingPresenter3.ScrollAnimationStarting += ScrollingPresenter_ScrollAnimationStarting;
                outerScrollingPresenter.ScrollAnimationStarting += ScrollingPresenter_ScrollAnimationStarting;
                outerScrollingPresenter2.ScrollAnimationStarting += ScrollingPresenter_ScrollAnimationStarting;
                outerScrollingPresenter3.ScrollAnimationStarting += ScrollingPresenter_ScrollAnimationStarting;

                innerScrollingPresenter.BringIntoViewRequested += FrameworkElement_BringIntoViewRequested;
                innerScrollingPresenter2.BringIntoViewRequested += FrameworkElement_BringIntoViewRequested;
                innerScrollingPresenter3.BringIntoViewRequested += FrameworkElement_BringIntoViewRequested;
                outerScrollingPresenter.BringIntoViewRequested += FrameworkElement_BringIntoViewRequested;
                outerScrollingPresenter2.BringIntoViewRequested += FrameworkElement_BringIntoViewRequested;
                outerScrollingPresenter3.BringIntoViewRequested += FrameworkElement_BringIntoViewRequested;

                outerScrollViewer.BringIntoViewRequested += FrameworkElement_BringIntoViewRequested;
                innerScrollViewer.BringIntoViewRequested += FrameworkElement_BringIntoViewRequested;

                outerScrollViewer2.BringIntoViewRequested += FrameworkElement_BringIntoViewRequested;
                innerScrollViewer2.BringIntoViewRequested += FrameworkElement_BringIntoViewRequested;
            }
        }

        private void UnhookBringIntoViewEvents()
        {
            if (innerScrollingPresenter != null)
            {
                innerScrollingPresenter.BringingIntoView -= ScrollingPresenter_BringingIntoView;
                innerScrollingPresenter2.BringingIntoView -= ScrollingPresenter_BringingIntoView;
                innerScrollingPresenter3.BringingIntoView -= ScrollingPresenter_BringingIntoView;
                outerScrollingPresenter.BringingIntoView -= ScrollingPresenter_BringingIntoView;
                outerScrollingPresenter2.BringingIntoView -= ScrollingPresenter_BringingIntoView;
                outerScrollingPresenter3.BringingIntoView -= ScrollingPresenter_BringingIntoView;

                innerScrollingPresenter.ScrollAnimationStarting -= ScrollingPresenter_ScrollAnimationStarting;
                innerScrollingPresenter2.ScrollAnimationStarting -= ScrollingPresenter_ScrollAnimationStarting;
                innerScrollingPresenter3.ScrollAnimationStarting -= ScrollingPresenter_ScrollAnimationStarting;
                outerScrollingPresenter.ScrollAnimationStarting -= ScrollingPresenter_ScrollAnimationStarting;
                outerScrollingPresenter2.ScrollAnimationStarting -= ScrollingPresenter_ScrollAnimationStarting;
                outerScrollingPresenter3.ScrollAnimationStarting -= ScrollingPresenter_ScrollAnimationStarting;

                innerScrollingPresenter.BringIntoViewRequested -= FrameworkElement_BringIntoViewRequested;
                innerScrollingPresenter2.BringIntoViewRequested -= FrameworkElement_BringIntoViewRequested;
                innerScrollingPresenter3.BringIntoViewRequested -= FrameworkElement_BringIntoViewRequested;
                outerScrollingPresenter.BringIntoViewRequested -= FrameworkElement_BringIntoViewRequested;
                outerScrollingPresenter2.BringIntoViewRequested -= FrameworkElement_BringIntoViewRequested;
                outerScrollingPresenter3.BringIntoViewRequested -= FrameworkElement_BringIntoViewRequested;

                outerScrollViewer.BringIntoViewRequested -= FrameworkElement_BringIntoViewRequested;
                innerScrollViewer.BringIntoViewRequested -= FrameworkElement_BringIntoViewRequested;

                outerScrollViewer2.BringIntoViewRequested -= FrameworkElement_BringIntoViewRequested;
                innerScrollViewer2.BringIntoViewRequested -= FrameworkElement_BringIntoViewRequested;
            }
        }

        private void ChkLogScrollingPresenterEvents_Checked(object sender, RoutedEventArgs e)
        {
            HookScrollingPresenterEvents(innerScrollingPresenter);
            HookScrollingPresenterEvents(innerScrollingPresenter2);
            HookScrollingPresenterEvents(innerScrollingPresenter3);
            HookScrollingPresenterEvents(outerScrollingPresenter);
            HookScrollingPresenterEvents(outerScrollingPresenter2);
            HookScrollingPresenterEvents(outerScrollingPresenter3);
            HookScrollViewerEvents(innerScrollViewer);
            HookScrollViewerEvents(innerScrollViewer2);
            HookScrollViewerEvents(outerScrollViewer);
            HookScrollViewerEvents(outerScrollViewer2);
        }

        private void ChkLogScrollingPresenterEvents_Unchecked(object sender, RoutedEventArgs e)
        {
            UnhookScrollingPresenterEvents(innerScrollingPresenter);
            UnhookScrollingPresenterEvents(innerScrollingPresenter2);
            UnhookScrollingPresenterEvents(innerScrollingPresenter3);
            UnhookScrollingPresenterEvents(outerScrollingPresenter);
            UnhookScrollingPresenterEvents(outerScrollingPresenter2);
            UnhookScrollingPresenterEvents(outerScrollingPresenter3);
            UnhookScrollViewerEvents(innerScrollViewer);
            UnhookScrollViewerEvents(innerScrollViewer2);
            UnhookScrollViewerEvents(outerScrollViewer);
            UnhookScrollViewerEvents(outerScrollViewer2);
        }

        private void HookScrollingPresenterEvents(ScrollingPresenter scrollingPresenter)
        {
            scrollingPresenter.ExtentChanged += ScrollingPresenter_ExtentChanged;
            scrollingPresenter.StateChanged += ScrollingPresenter_StateChanged;
            scrollingPresenter.ViewChanged += ScrollingPresenter_ViewChanged;
            scrollingPresenter.ScrollCompleted += ScrollingPresenter_ScrollCompleted;
        }

        private void UnhookScrollingPresenterEvents(ScrollingPresenter scrollingPresenter)
        {
            scrollingPresenter.ExtentChanged -= ScrollingPresenter_ExtentChanged;
            scrollingPresenter.StateChanged -= ScrollingPresenter_StateChanged;
            scrollingPresenter.ViewChanged -= ScrollingPresenter_ViewChanged;
            scrollingPresenter.ScrollCompleted -= ScrollingPresenter_ScrollCompleted;
        }

        private void HookScrollViewerEvents(ScrollViewer scrollViewer)
        {
            scrollViewer.ViewChanged += ScrollViewer_ViewChanged;
        }

        private void UnhookScrollViewerEvents(ScrollViewer scrollViewer)
        {
            scrollViewer.ViewChanged -= ScrollViewer_ViewChanged;
        }

        private void ChkLogScrollingPresenterMessages_Checked(object sender, RoutedEventArgs e)
        {
            MUXControlsTestHooks.SetLoggingLevelForType("ScrollingPresenter", isLoggingInfoLevel: true, isLoggingVerboseLevel: true);
            MUXControlsTestHooks.LoggingMessage += MUXControlsTestHooks_LoggingMessage;
        }

        private void ChkLogScrollingPresenterMessages_Unchecked(object sender, RoutedEventArgs e)
        {
            MUXControlsTestHooks.SetLoggingLevelForType("ScrollingPresenter", isLoggingInfoLevel: false, isLoggingVerboseLevel: false);
            MUXControlsTestHooks.LoggingMessage -= MUXControlsTestHooks_LoggingMessage;
        }

        private void BtnClearScrollingPresenterEvents_Click(object sender, RoutedEventArgs e)
        {
            lstScrollingPresenterEvents.Items.Clear();
        }

        private void BtnClearExceptionReport_Click(object sender, RoutedEventArgs e)
        {
            txtExceptionReport.Text = string.Empty;
        }

        private void RefreshSnapPoints(ScrollingPresenter scrollingPresenter, StackPanel stackPanel)
        {
            if (scrollingPresenter != null && stackPanel != null && stackPanel.Children.Count > 0)
            {
                AppendAsyncEventMessage("Populating snap points for " + scrollingPresenter.Name + ":");

                ScrollSnapPoint scrollSnapPoint;
                GeneralTransform gt = stackPanel.TransformToVisual(scrollingPresenter.Content);
                Point stackPanelOriginPoint = new Point();
                stackPanelOriginPoint = gt.TransformPoint(stackPanelOriginPoint);
                
                if (stackPanel.Orientation == Orientation.Horizontal)
                {
                    scrollingPresenter.HorizontalSnapPoints.Clear();

                    scrollSnapPoint = new ScrollSnapPoint(stackPanelOriginPoint.X, ScrollSnapPointsAlignment.Near);
                    AppendAsyncEventMessage("Adding horizontal snap point to " + scrollingPresenter.Name + " at value " + stackPanelOriginPoint.X);
                    scrollingPresenter.HorizontalSnapPoints.Add(scrollSnapPoint);
                }
                else
                {
                    scrollingPresenter.VerticalSnapPoints.Clear();

                    scrollSnapPoint = new ScrollSnapPoint(stackPanelOriginPoint.Y, ScrollSnapPointsAlignment.Near);
                    AppendAsyncEventMessage("Adding vertical snap point to " + scrollingPresenter.Name + " at value " + stackPanelOriginPoint.Y);
                    scrollingPresenter.VerticalSnapPoints.Add(scrollSnapPoint);
                }

                foreach (UIElement child in stackPanel.Children)
                {
                    FrameworkElement childAsFE = child as FrameworkElement;

                    if (childAsFE != null)
                    {
                        gt = childAsFE.TransformToVisual(stackPanel);
                        Point childOriginPoint = new Point();
                        childOriginPoint = gt.TransformPoint(childOriginPoint);

                        double snapPointValue = 0.0;
                        Thickness margin = childAsFE.Margin;

                        if (stackPanel.Orientation == Orientation.Horizontal)
                        {
                            snapPointValue = margin.Right + childAsFE.ActualWidth + childOriginPoint.X;
                            if (snapPointValue <= scrollingPresenter.ScrollableWidth)
                            {
                                scrollSnapPoint = new ScrollSnapPoint(snapPointValue, ScrollSnapPointsAlignment.Near);
                                AppendAsyncEventMessage("Adding horizontal snap point to " + scrollingPresenter.Name + " at value " + snapPointValue);
                                scrollingPresenter.HorizontalSnapPoints.Add(scrollSnapPoint);
                            }
                            else
                            {
                                break;
                            }
                        }
                        else
                        {
                            snapPointValue = margin.Bottom + childAsFE.ActualHeight + childOriginPoint.Y;
                            if (snapPointValue <= scrollingPresenter.ScrollableHeight)
                            {
                                scrollSnapPoint = new ScrollSnapPoint(snapPointValue, ScrollSnapPointsAlignment.Near);
                                AppendAsyncEventMessage("Adding vertical snap point to " + scrollingPresenter.Name + " at value " + snapPointValue);
                                scrollingPresenter.VerticalSnapPoints.Add(scrollSnapPoint);
                            }
                            else
                            {
                                break;
                            }
                        }
                    }

                    RefreshSnapPoints(scrollingPresenter, child as StackPanel);
                }
            }
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
                    lstScrollingPresenterEvents.Items.Add(asyncEventMessage);
                }
                lstAsyncEventMessage.Clear();
            }
        }

        private class SampleDataSource : CustomItemsSourceViewWithUniqueIdMapping
        {
            List<string> _inner;

            public SampleDataSource(ScrollingPresenterBringIntoViewPage owner, List<string> source)
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

            private ScrollingPresenterBringIntoViewPage Owner
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
