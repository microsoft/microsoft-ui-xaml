// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.Linq;
using System.Numerics;
using Microsoft.UI.Composition;
using Microsoft.UI.Private.Controls;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Navigation;

namespace MUXControlsTestApp.Samples
{
    public sealed partial class VariableSizedItemsPage : TestPage
    {
        private bool m_logEvents = false;
        private double m_extentHeight = 0;
        private int m_changeViewToVerticalOffsetWithAnimationCorrelationId = -1;
        private int m_changeViewToVerticalScrollPercentageWithAnimationCorrelationId = -1;
        private int m_scrollerChangeViewByVerticalScrollCount = 0;
        private StackLayout m_stackLayout = null;
        private UniformGridLayout m_uniformGridLayout = null;

        public VariableSizedItemsPage()
        {
            this.InitializeComponent();
            goBackButton.Click += delegate { Frame.GoBack(); };

            Loaded += VariableSizedItemsPage_Loaded;

            LayoutsTestHooks.LayoutAnchorIndexChanged += LayoutsTestHooks_LayoutAnchorIndexChanged;
            LayoutsTestHooks.LayoutAnchorOffsetChanged += LayoutsTestHooks_LayoutAnchorOffsetChanged;
        }

        ~VariableSizedItemsPage()
        {
        }

        protected override void OnNavigatedFrom(NavigationEventArgs e)
        {
            LayoutsTestHooks.LayoutAnchorIndexChanged -= LayoutsTestHooks_LayoutAnchorIndexChanged;
            LayoutsTestHooks.LayoutAnchorOffsetChanged -= LayoutsTestHooks_LayoutAnchorOffsetChanged;

            UnhookScrollersEvents();

            base.OnNavigatedFrom(e);
        }

        private void AppendEventMessage(string message)
        {
            if (lstEvents != null && m_logEvents)
            {
                lstEvents.Items.Add(message);
            }
        }

        private void HookScrollersEvents()
        {
            if (scrollViewer != null)
            {
                scrollViewer.ViewChanging += ScrollViewer_ViewChanging;
                scrollViewer.ViewChanged += ScrollViewer_ViewChanged;
            }
            if (scrollView != null)
            {
                scrollView.ViewChanged += ScrollView_ViewChanged;
                scrollView.ScrollStarting += ScrollView_ScrollStarting;
                scrollView.ZoomStarting += ScrollView_ZoomStarting;
                scrollView.ScrollAnimationStarting += ScrollView_ScrollAnimationStarting;
            }
        }

        private void UnhookScrollersEvents()
        {
            if (scrollViewer != null)
            {
                scrollViewer.ViewChanging -= ScrollViewer_ViewChanging;
                scrollViewer.ViewChanged -= ScrollViewer_ViewChanged;
            }
            if (scrollView != null)
            {
                scrollView.ViewChanged -= ScrollView_ViewChanged;
                scrollView.ScrollStarting -= ScrollView_ScrollStarting;
                scrollView.ZoomStarting -= ScrollView_ZoomStarting;
                scrollView.ScrollAnimationStarting -= ScrollView_ScrollAnimationStarting;
            }
        }

        private void UpdateItemsRepeaterVerticalCacheLength()
        {
            try
            {
                if (itemsRepeater != null)
                {
                    txtItemsRepeaterVerticalCacheLength.Text = itemsRepeater.VerticalCacheLength.ToString();
                }
            }
            catch (Exception ex)
            {
                string message = "UpdateItemsRepeaterVerticalCacheLength. Exception:" + ex.ToString();

                Debug.WriteLine(message);
                AppendEventMessage(message);
            }
        }

        private void UpdateLayoutSpacing()
        {
            if (itemsRepeater != null)
            {
                StackLayout stackLayout = itemsRepeater.Layout as StackLayout;

                if (stackLayout == null)
                {
                    UniformGridLayout uniformGridLayout = itemsRepeater.Layout as UniformGridLayout;
                    txtLayoutSpacing.Text = uniformGridLayout.MinColumnSpacing.ToString();
                }
                else
                {
                    txtLayoutSpacing.Text = stackLayout.Spacing.ToString();
                }
            }
        }

        private void UpdateLayoutLogItemIndexDbg()
        {
            if (itemsRepeater != null && txtLayoutLogItemIndexDbg != null)
            {
                Layout layout = itemsRepeater.Layout;

                if (layout != null)
                {
                    txtLayoutLogItemIndexDbg.Text = LayoutsTestHooks.GetLayoutLogItemIndex(layout).ToString();
                }
            }
        }

        private void UpdateItemsSource()
        {
            try
            {
                if (itemsRepeater == null || txtItemsSourceItemsCount == null || btnSwitchVariability == null)
                {
                    return;
                }

                int itemsCount = int.Parse(txtItemsSourceItemsCount.Text);

                if (btnSwitchVariability.Content as string == "Use variable heights")
                {
                    itemsRepeater.ItemsSource = Enumerable.Range(0, itemsCount).Select(x => new FixedSizeItem()
                    {
                        Text = "Item " + x
                    });
                }
                else
                {
                    itemsRepeater.ItemsSource = Enumerable.Range(0, itemsCount).Select(x => new VariableSizeItem(x)
                    {
                        Text = "Item " + x
                    });
                }
            }
            catch (Exception ex)
            {
                string message = "UpdateItemsSource. Exception:" + ex.ToString();

                Debug.WriteLine(message);
                AppendEventMessage(message);
            }
        }

        private void UpdateItemsSourceItemsCount()
        {
            if (itemsRepeater != null && itemsRepeater.ItemsSourceView != null && txtItemsSourceItemsCount != null)
            {
                txtItemsSourceItemsCount.Text = itemsRepeater.ItemsSourceView.Count.ToString();
            }
        }

        private void UpdateScrollerHorizontalAnchorRatio()
        {
            try
            {
                if (scrollViewer != null && scrollViewer.Content != null)
                {
                    txtScrollerHorizontalAnchorRatio.Text = scrollViewer.HorizontalAnchorRatio.ToString();
                }
                else if (scrollView != null)
                {
                    txtScrollerHorizontalAnchorRatio.Text = scrollView.HorizontalAnchorRatio.ToString();
                }
            }
            catch (Exception ex)
            {
                string message = "UpdateScrollerHorizontalAnchorRatio. Exception:" + ex.ToString();

                Debug.WriteLine(message);
                AppendEventMessage(message);
            }
        }

        private void UpdateScrollerVerticalAnchorRatio()
        {
            try
            {
                if (scrollViewer != null && scrollViewer.Content != null)
                {
                    txtScrollerVerticalAnchorRatio.Text = scrollViewer.VerticalAnchorRatio.ToString();
                }
                else if (scrollView != null)
                {
                    txtScrollerVerticalAnchorRatio.Text = scrollView.VerticalAnchorRatio.ToString();
                }
            }
            catch (Exception ex)
            {
                string message = "UpdateScrollerVerticalAnchorRatio. Exception:" + ex.ToString();

                Debug.WriteLine(message);
                AppendEventMessage(message);
            }
        }

        private void UpdateScrollerVerticalOffset()
        {
            try
            {
                if (scrollViewer != null && scrollViewer.Content != null)
                {
                    txtScrollerVerticalOffset.Text = scrollViewer.VerticalOffset.ToString("F");
                }
                else if (scrollView != null)
                {
                    txtScrollerVerticalOffset.Text = scrollView.VerticalOffset.ToString("F");
                }
            }
            catch (Exception ex)
            {
                string message = "UpdateScrollerVerticalOffset. Exception:" + ex.ToString();

                Debug.WriteLine(message);
                AppendEventMessage(message);
            }
        }

        private void UpdateScrollerZoomFactor()
        {
            try
            {
                if (scrollViewer != null && scrollViewer.Content != null)
                {
                    txtScrollerZoomFactor.Text = scrollViewer.ZoomFactor.ToString("F");
                }
                else if (scrollView != null)
                {
                    txtScrollerZoomFactor.Text = scrollView.ZoomFactor.ToString("F");
                }
            }
            catch (Exception ex)
            {
                string message = "UpdateScrollerZoomFactor. Exception:" + ex.ToString();

                Debug.WriteLine(message);
                AppendEventMessage(message);
            }
        }

        private void UpdateScrollerVerticalScrollPercentage()
        {
            try
            {
                if (scrollViewer != null && scrollViewer.Content != null)
                {
                    if (scrollViewer.ScrollableHeight == 0)
                    {
                        txtScrollerVerticalScrollPercentage.Text = "N/A";
                    }
                    else
                    {
                        txtScrollerVerticalScrollPercentage.Text = (scrollViewer.VerticalOffset / scrollViewer.ScrollableHeight * 100.0).ToString("F");
                    }
                }
                else if (scrollView != null)
                {
                    if (scrollView.ScrollableHeight == 0)
                    {
                        txtScrollerVerticalScrollPercentage.Text = "N/A";
                    }
                    else
                    {
                        txtScrollerVerticalScrollPercentage.Text = (scrollView.VerticalOffset / scrollView.ScrollableHeight * 100.0).ToString("F");
                    }
                }
            }
            catch (Exception ex)
            {
                string message = "UpdateScrollerVerticalScrollPercentage. Exception:" + ex.ToString();

                Debug.WriteLine(message);
                AppendEventMessage(message);
            }
        }

        private void UpdateScrollerScrollableHeight()
        {
            try
            {
                if (scrollViewer != null && scrollViewer.Content != null)
                {
                    txtScrollerScrollableHeight.Text = scrollViewer.ScrollableHeight.ToString("F");
                }
                else if (scrollView != null)
                {
                    txtScrollerScrollableHeight.Text = scrollView.ScrollableHeight.ToString("F");
                }
            }
            catch (Exception ex)
            {
                string message = "UpdateScrollerScrollableHeight. Exception:" + ex.ToString();

                Debug.WriteLine(message);
                AppendEventMessage(message);
            }
        }

        private void UpdateScrollerExtentHeight()
        {
            try
            {
                if (scrollViewer != null && scrollViewer.Content != null)
                {
                    txtScrollerExtentHeight.Text = scrollViewer.ExtentHeight.ToString("F");
                }
                else if (scrollView != null)
                {
                    txtScrollerExtentHeight.Text = scrollView.ExtentHeight.ToString("F");
                }

                UpdateExtentHeight();
            }
            catch (Exception ex)
            {
                string message = "UpdateScrollerExtentHeight. Exception:" + ex.ToString();

                Debug.WriteLine(message);
                AppendEventMessage(message);
            }
        }

        private void UpdateExtentHeight()
        {
            double extentHeight = 0;

            if (scrollViewer != null && scrollViewer.Content != null)
            {
                extentHeight = scrollViewer.ExtentHeight;
            }
            else if (scrollView != null)
            {
                extentHeight = scrollView.ExtentHeight;
            }

            if (m_extentHeight != extentHeight)
            {
                AppendEventMessage("ExtentHeight changed. Old=" + m_extentHeight.ToString("F") + ", New=" + extentHeight.ToString("F"));

                m_extentHeight = extentHeight;
            }
        }

        private void UpdateScrollerChangeViewVerticalScrollPercentage()
        {
            try
            {
                double verticalOffset = Convert.ToDouble(txtScrollerChangeViewVerticalOffset.Text);

                if (scrollViewer != null && scrollViewer.Content != null)
                {
                    if (scrollViewer.ScrollableHeight == 0)
                    {
                        txtScrollerChangeViewVerticalScrollPercentage.Text = "N/A";
                    }
                    else
                    {
                        txtScrollerChangeViewVerticalScrollPercentage.Text = (verticalOffset / scrollViewer.ScrollableHeight * 100.0).ToString("F");
                    }
                }
                else if (scrollView != null)
                {
                    if (scrollView.ScrollableHeight == 0)
                    {
                        txtScrollerChangeViewVerticalScrollPercentage.Text = "N/A";
                    }
                    else
                    {
                        txtScrollerChangeViewVerticalScrollPercentage.Text = (verticalOffset / scrollView.ScrollableHeight * 100.0).ToString("F");
                    }
                }
            }
            catch (Exception ex)
            {
                string message = "UpdateScrollerChangeViewVerticalScrollPercentage. Exception:" + ex.ToString();

                Debug.WriteLine(message);
                AppendEventMessage(message);
            }
        }

        private void VariableSizedItemsPage_Loaded(object sender, RoutedEventArgs e)
        {
            HookScrollersEvents();
            UpdateItemsSource();
            UpdateItemsRepeaterVerticalCacheLength();
            UpdateCmbItemsRepeaterHorizontalAlignment();
            UpdateCmbItemsRepeaterVerticalAlignment();
            UpdateLayoutSpacing();
            UpdateItemsSourceItemsCount();
            UpdateScrollerHorizontalAnchorRatio();
            UpdateScrollerVerticalAnchorRatio();
            UpdateScrollerVerticalOffset();
            UpdateScrollerZoomFactor();
            UpdateScrollerVerticalScrollPercentage();
            UpdateScrollerScrollableHeight();
            UpdateScrollerExtentHeight();
            UpdateScrollerChangeViewVerticalScrollPercentage();
        }

        private void LayoutsTestHooks_LayoutAnchorIndexChanged(object sender, object args)
        {
            AppendEventMessage("LayoutAnchorIndexChanged LayoutAnchorIndex=" + LayoutsTestHooks.GetLayoutAnchorIndex(sender));
        }

        private void LayoutsTestHooks_LayoutAnchorOffsetChanged(object sender, object args)
        {
            AppendEventMessage("LayoutAnchorOffsetChanged LayoutAnchorOffset=" + LayoutsTestHooks.GetLayoutAnchorOffset(sender).ToString("F"));
        }

        private string ScrollViewerViewToString(ScrollViewerView scrollViewerView)
        {
            return "H=" + scrollViewerView.HorizontalOffset.ToString("F") + ", V=" + scrollViewerView.VerticalOffset.ToString("F") + ", ZF=" + scrollViewerView.ZoomFactor.ToString("F");
        }

        private void ScrollViewer_ViewChanging(object sender, ScrollViewerViewChangingEventArgs args)
        {
            AppendEventMessage("ViewChanging IsInertial=" + args.IsInertial + ", NextView=(" + ScrollViewerViewToString(args.NextView) + "), FinalView=(" + ScrollViewerViewToString(args.FinalView) + ")");
        }

        private void ScrollViewer_ViewChanged(object sender, ScrollViewerViewChangedEventArgs args)
        {
            AppendEventMessage("ViewChanged IsIntermediate=" + args.IsIntermediate + ", H=" + scrollViewer.HorizontalOffset.ToString("F") + ", V=" + scrollViewer.VerticalOffset.ToString("F") + ", ZF=" + scrollViewer.ZoomFactor.ToString("F"));

            UpdateExtentHeight();
        }

        private void ScrollView_ViewChanged(ScrollView sender, object args)
        {
            AppendEventMessage("ViewChanged H=" + scrollView.HorizontalOffset.ToString("F") + ", V=" + scrollView.VerticalOffset.ToString("F") + ", ZF=" + scrollView.ZoomFactor.ToString("F"));

            UpdateExtentHeight();
        }

        private void ScrollView_ScrollStarting(ScrollView sender, ScrollingScrollStartingEventArgs args)
        {
            AppendEventMessage($"ScrollView.ScrollStarting OffsetsChangeCorrelationId={args.CorrelationId}, H={args.HorizontalOffset.ToString("F")}, V={args.VerticalOffset.ToString("F")}, ZF={args.ZoomFactor.ToString("F")}");

            UpdateExtentHeight();
            ScrollerChangeViewByVerticalScrollDecrement();
        }

        private void ScrollView_ZoomStarting(ScrollView sender, ScrollingZoomStartingEventArgs args)
        {
            AppendEventMessage($"ScrollView.ZoomStarting ZoomFactorChangeCorrelationId={args.CorrelationId}, H={args.HorizontalOffset.ToString("F")}, V={args.VerticalOffset.ToString("F")}, ZF={args.ZoomFactor.ToString("F")}");

            UpdateExtentHeight();
        }

        private void ScrollView_ScrollAnimationStarting(ScrollView sender, ScrollingScrollAnimationStartingEventArgs args)
        {
            AppendEventMessage($"ScrollAnimationStarting CorrelationId={args.CorrelationId}, SP=({args.StartPosition.X.ToString("F")}, {args.StartPosition.Y.ToString("F")}), EP=({args.EndPosition.X.ToString("F")}, {args.EndPosition.Y.ToString("F")})");

            try
            {
                Vector3KeyFrameAnimation stockKeyFrameAnimation = args.Animation as Vector3KeyFrameAnimation;

                if (stockKeyFrameAnimation != null)
                {
                    TextBox txtChangeViewWithAnimationDuration = null;

                    if (m_changeViewToVerticalOffsetWithAnimationCorrelationId == args.CorrelationId)
                    {
                        txtChangeViewWithAnimationDuration = txtScrollerChangeViewToVerticalOffsetWithAnimationDuration;
                    }
                    else if (m_changeViewToVerticalScrollPercentageWithAnimationCorrelationId == args.CorrelationId)
                    {
                        txtChangeViewWithAnimationDuration = txtScrollerChangeViewToVerticalScrollPercentageWithAnimationDuration;
                    }

                    if (txtChangeViewWithAnimationDuration != null && 
                        txtChangeViewWithAnimationDuration.Text != "N/A" && 
                        !string.IsNullOrWhiteSpace(txtChangeViewWithAnimationDuration.Text))
                    {
                        double durationOverride = Convert.ToDouble(txtChangeViewWithAnimationDuration.Text);

                        stockKeyFrameAnimation.Duration = TimeSpan.FromMilliseconds(durationOverride);
                    }
                }
            }
            catch (Exception ex)
            {
                string message = "ScrollView_ScrollAnimationStarting. Exception:" + ex.ToString();

                Debug.WriteLine(message);
                AppendEventMessage(message);
            }
        }

        private void BtnSwitchScroller_Click(object sender, RoutedEventArgs e)
        {
            if (btnSwitchScroller.Content as string == "Use ScrollView")
            {
                btnSwitchScroller.Content = "Use ScrollViewer";
                btnScrollerChangeViewToZoomFactor.Content = "Jump to ZoomFactor";
                btnScrollerChangeViewToZoomFactorWithAnimation.Content = "Animate to ZoomFactor";
                txtScrollerChangeViewToVerticalOffsetWithAnimationDuration.IsEnabled = true;
                txtScrollerChangeViewToVerticalScrollPercentageWithAnimationDuration.IsEnabled = true;
                txtScrollerChangeViewZoomCenterPoint.IsEnabled = true;
                btnScrollerChangeViewByVerticalScroll.IsEnabled = true;
                scrollViewer.Visibility = Visibility.Collapsed;
                scrollView.Visibility = Visibility.Visible;
                scrollViewer.Content = null;
                scrollView.Content = itemsRepeater;
            }
            else
            {
                btnSwitchScroller.Content = "Use ScrollView";
                btnScrollerChangeViewToZoomFactor.Content = "Jump to VerticalOffset/ZoomFactor";
                btnScrollerChangeViewToZoomFactorWithAnimation.Content = "Animate to VerticalOffset/ZoomFactor";
                txtScrollerChangeViewToVerticalOffsetWithAnimationDuration.IsEnabled = false;
                txtScrollerChangeViewToVerticalScrollPercentageWithAnimationDuration.IsEnabled = false;
                txtScrollerChangeViewZoomCenterPoint.IsEnabled = false;
                btnScrollerChangeViewByVerticalScroll.IsEnabled = false;
                scrollView.Visibility = Visibility.Collapsed;
                scrollViewer.Visibility = Visibility.Visible;
                scrollView.Content = null;
                scrollViewer.Content = itemsRepeater;
            }
        }

        private void BtnSwitchVariability_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (btnSwitchVariability == null)
                {
                    return;
                }

                if (btnSwitchVariability.Content as string == "Use fixed heights")
                {
                    btnSwitchVariability.Content = "Use variable heights";
                }
                else
                {
                    btnSwitchVariability.Content = "Use fixed heights";
                }

                UpdateItemsSource();
            }
            catch (Exception ex)
            {
                string message = "BtnSwitchVariability_Click. Exception:" + ex.ToString();

                Debug.WriteLine(message);
                AppendEventMessage(message);
            }
        }

        private void BtnGetScrollerHorizontalAnchorRatio_Click(object sender, RoutedEventArgs e)
        {
            UpdateScrollerHorizontalAnchorRatio();
        }

        private void BtnGetScrollerVerticalAnchorRatio_Click(object sender, RoutedEventArgs e)
        {
            UpdateScrollerVerticalAnchorRatio();
        }

        private void BtnGetScrollerVerticalOffset_Click(object sender, RoutedEventArgs e)
        {
            UpdateScrollerVerticalOffset();
        }

        private void BtnGetScrollerZoomFactor_Click(object sender, RoutedEventArgs e)
        {
            UpdateScrollerZoomFactor();
        }

        private void BtnGetScrollerVerticalScrollPercentage_Click(object sender, RoutedEventArgs e)
        {
            UpdateScrollerVerticalScrollPercentage();
        }

        private void BtnGetScrollerScrollableHeight_Click(object sender, RoutedEventArgs e)
        {
            UpdateScrollerScrollableHeight();
        }

        private void BtnGetScrollerExtentHeight_Click(object sender, RoutedEventArgs e)
        {
            UpdateScrollerExtentHeight();
        }

        private void BtnSetScrollerHorizontalAnchorRatio_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                double horizontalAnchorRatio = Convert.ToDouble(txtScrollerHorizontalAnchorRatio.Text);

                if (scrollViewer != null)
                {
                    scrollViewer.HorizontalAnchorRatio = horizontalAnchorRatio;
                }
                if (scrollView != null)
                {
                    scrollView.HorizontalAnchorRatio = horizontalAnchorRatio;
                }
            }
            catch (Exception ex)
            {
                string message = "BtnSetScrollerHorizontalAnchorRatio_Click. Exception:" + ex.ToString();

                Debug.WriteLine(message);
                AppendEventMessage(message);
            }
        }

        private void BtnSetScrollerVerticalAnchorRatio_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                double verticalAnchorRatio = Convert.ToDouble(txtScrollerVerticalAnchorRatio.Text);

                if (scrollViewer != null)
                {
                    scrollViewer.VerticalAnchorRatio = verticalAnchorRatio;
                }
                if (scrollView != null)
                {
                    scrollView.VerticalAnchorRatio = verticalAnchorRatio;
                }
            }
            catch (Exception ex)
            {
                string message = "BtnSetScrollerVerticalAnchorRatio_Click. Exception:" + ex.ToString();

                Debug.WriteLine(message);
                AppendEventMessage(message);
            }
        }

        private void BtnScrollerChangeViewToVerticalOffset_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                double verticalOffset = Convert.ToDouble(txtScrollerChangeViewVerticalOffset.Text);

                if (scrollViewer != null && scrollViewer.Content != null)
                {
                    scrollViewer.ChangeView(null, verticalOffset, null, true);
                }
                else if (scrollView != null)
                {
                    scrollView.ScrollTo(scrollView.HorizontalOffset, verticalOffset, new ScrollingScrollOptions(ScrollingAnimationMode.Disabled, ScrollingSnapPointsMode.Ignore));
                }
            }
            catch (Exception ex)
            {
                string message = "BtnScrollerChangeView_Click. Exception:" + ex.ToString();

                Debug.WriteLine(message);
                AppendEventMessage(message);
            }
        }

        private void BtnScrollerChangeViewToVerticalScrollPercentage_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                double verticalScrollPercentage = Convert.ToDouble(txtScrollerChangeViewVerticalScrollPercentage.Text);

                if (scrollViewer != null && scrollViewer.Content != null)
                {
                    double verticalOffset = scrollViewer.ScrollableHeight * verticalScrollPercentage / 100.0;

                    scrollViewer.ChangeView(null, verticalOffset, null, true);
                }
                else if (scrollView != null)
                {
                    double verticalOffset = scrollView.ScrollableHeight * verticalScrollPercentage / 100.0;

                    scrollView.ScrollTo(scrollView.HorizontalOffset, verticalOffset, new ScrollingScrollOptions(ScrollingAnimationMode.Disabled, ScrollingSnapPointsMode.Ignore));
                }
            }
            catch (Exception ex)
            {
                string message = "BtnScrollerChangeViewToVerticalScrollPercentage_Click. Exception:" + ex.ToString();

                Debug.WriteLine(message);
                AppendEventMessage(message);
            }
        }

        private void BtnScrollerChangeViewToVerticalOffsetWithAnimation_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                double verticalOffset = Convert.ToDouble(txtScrollerChangeViewVerticalOffset.Text);

                if (scrollViewer != null && scrollViewer.Content != null)
                {
                    scrollViewer.ChangeView(null, verticalOffset, null, false);
                }
                else if (scrollView != null)
                {
                    m_changeViewToVerticalOffsetWithAnimationCorrelationId = scrollView.ScrollTo(scrollView.HorizontalOffset, verticalOffset);
                }
            }
            catch (Exception ex)
            {
                string message = "BtnScrollerChangeViewWithAnimation_Click. Exception:" + ex.ToString();

                Debug.WriteLine(message);
                AppendEventMessage(message);
            }
        }

        private void BtnScrollerChangeViewToVerticalScrollPercentageWithAnimation_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                double verticalScrollPercentage = Convert.ToDouble(txtScrollerChangeViewVerticalScrollPercentage.Text);

                if (scrollViewer != null && scrollViewer.Content != null)
                {
                    double verticalOffset = scrollViewer.ScrollableHeight * verticalScrollPercentage / 100.0;

                    scrollViewer.ChangeView(null, verticalOffset, null, false);
                }
                else if (scrollView != null)
                {
                    double verticalOffset = scrollView.ScrollableHeight * verticalScrollPercentage / 100.0;

                    m_changeViewToVerticalScrollPercentageWithAnimationCorrelationId = scrollView.ScrollTo(scrollView.HorizontalOffset, verticalOffset);
                }
            }
            catch (Exception ex)
            {
                string message = "BtnScrollerChangeViewToVerticalScrollPercentageWithAnimation_Click. Exception:" + ex.ToString();

                Debug.WriteLine(message);
                AppendEventMessage(message);
            }
        }

        private void BtnScrollerChangeViewByVerticalScroll_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (scrollView != null && txtScrollerChangeViewByVerticalScrollCount != null)
                {
                    m_scrollerChangeViewByVerticalScrollCount = Convert.ToInt32(txtScrollerChangeViewByVerticalScrollCount.Text);

                    ScrollerChangeViewByVerticalScrollDecrement();
                }
            }
            catch (Exception ex)
            {
                string message = "BtnScrollerChangeViewByVerticalScroll_Click. Exception:" + ex.ToString();

                Debug.WriteLine(message);
                AppendEventMessage(message);
            }
        }

        private void ScrollerChangeViewByVerticalScrollDecrement()
        {
            try
            {
                if (scrollView != null && m_scrollerChangeViewByVerticalScrollCount > 0)
                {
                    double verticalOffsetDelta = Convert.ToDouble(txtScrollerChangeViewVerticalOffset.Text);

                    scrollView.ScrollBy(0, verticalOffsetDelta, new ScrollingScrollOptions(ScrollingAnimationMode.Disabled, ScrollingSnapPointsMode.Ignore));

                    m_scrollerChangeViewByVerticalScrollCount--;
                }
            }
            catch (Exception ex)
            {
                string message = "ScrollerChangeViewByVerticalScrollDecrement. Exception:" + ex.ToString();

                Debug.WriteLine(message);
                AppendEventMessage(message);
            }
        }

        private void BtnScrollerChangeViewToZoomFactor_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                double verticalOffset = Convert.ToDouble(txtScrollerChangeViewVerticalOffset.Text);
                float zoomFactor = Convert.ToSingle(txtScrollerChangeViewZoomZoomFactor.Text);

                if (scrollViewer != null && scrollViewer.Content != null)
                {
                    scrollViewer.ChangeView(
                        null, 
                        verticalOffset, 
                        zoomFactor, 
                        true);
                }
                else if (scrollView != null)
                {
                    scrollView.ZoomTo(
                        zoomFactor, 
                        (txtScrollerChangeViewZoomCenterPoint.Text == "null") ? (Vector2?)null : ConvertFromStringToVector2(txtScrollerChangeViewZoomCenterPoint.Text),
                        new ScrollingZoomOptions(ScrollingAnimationMode.Disabled, ScrollingSnapPointsMode.Ignore));
                }
            }
            catch (Exception ex)
            {
                string message = "BtnScrollerChangeViewToZoomFactor_Click. Exception:" + ex.ToString();

                Debug.WriteLine(message);
                AppendEventMessage(message);
            }
        }

        private void BtnScrollerChangeViewToZoomFactorWithAnimation_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                double verticalOffset = Convert.ToDouble(txtScrollerChangeViewVerticalOffset.Text);
                float zoomFactor = Convert.ToSingle(txtScrollerChangeViewZoomZoomFactor.Text);

                if (scrollViewer != null && scrollViewer.Content != null)
                {
                    scrollViewer.ChangeView(
                        null,
                        verticalOffset,
                        zoomFactor,
                        false);
                }
                else if (scrollView != null)
                {
                    scrollView.ZoomTo(
                        zoomFactor,
                        (txtScrollerChangeViewZoomCenterPoint.Text == "null") ? (Vector2?)null : ConvertFromStringToVector2(txtScrollerChangeViewZoomCenterPoint.Text));
                }
            }
            catch (Exception ex)
            {
                string message = "BtnScrollerChangeViewToZoomFactorWithAnimation_Click. Exception:" + ex.ToString();

                Debug.WriteLine(message);
                AppendEventMessage(message);
            }
        }

        private void BtnGetScrollerChangeViewVerticalOffset_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (txtScrollerChangeViewVerticalScrollPercentage.Text == "N/A")
                {
                    txtScrollerChangeViewVerticalOffset.Text = "N/A";
                    return;
                }

                double verticalScrollPercentage = Convert.ToDouble(txtScrollerChangeViewVerticalScrollPercentage.Text);

                if (scrollViewer != null && scrollViewer.Content != null)
                {
                    txtScrollerChangeViewVerticalOffset.Text = (scrollViewer.ScrollableHeight * verticalScrollPercentage / 100.0).ToString("F");
                }
                else if (scrollView != null)
                {
                    txtScrollerChangeViewVerticalOffset.Text = (scrollView.ScrollableHeight * verticalScrollPercentage / 100.0).ToString("F");
                }
            }
            catch (Exception ex)
            {
                string message = "BtnGetScrollerChangeViewVerticalOffset_Click. Exception:" + ex.ToString();

                Debug.WriteLine(message);
                AppendEventMessage(message);
            }
        }

        private void BtnGetScrollerChangeViewVerticalScrollPercentage_Click(object sender, RoutedEventArgs e)
        {
            UpdateScrollerChangeViewVerticalScrollPercentage();
        }

        private void BtnGetItemsRepeaterVerticalCacheLength_Click(object sender, RoutedEventArgs e)
        {
            UpdateItemsRepeaterVerticalCacheLength();
        }

        private void BtnSetItemsRepeaterVerticalCacheLength_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (itemsRepeater != null)
                {
                    itemsRepeater.VerticalCacheLength = Convert.ToDouble(txtItemsRepeaterVerticalCacheLength.Text);
                }
            }
            catch (Exception ex)
            {
                string message = "BtnSetItemsRepeaterVerticalCacheLength_Click. Exception:" + ex.ToString();

                Debug.WriteLine(message);
                AppendEventMessage(message);
            }
        }

        private void CmbItemsRepeaterLayout_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (itemsRepeater != null)
            {
                if (cmbItemsRepeaterLayout.SelectedIndex == 0 && itemsRepeater.Layout is UniformGridLayout)
                {
                    itemsRepeater.Layout = m_stackLayout;
                }
                else if (cmbItemsRepeaterLayout.SelectedIndex == 1 && itemsRepeater.Layout is StackLayout)
                {
                    if (m_stackLayout == null)
                    {
                        m_stackLayout = itemsRepeater.Layout as StackLayout;
                    }
                    if (m_uniformGridLayout == null)
                    {
                        m_uniformGridLayout = new UniformGridLayout()
                        {
                            MaximumRowsOrColumns = 2,
                            MinItemWidth = 100,
                            MinItemHeight = 100
                        };
                    }
                    itemsRepeater.Layout = m_uniformGridLayout;
                }
            }
        }

        private void CmbItemsRepeaterHorizontalAlignment_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (itemsRepeater != null)
            {
                itemsRepeater.HorizontalAlignment = (HorizontalAlignment)cmbItemsRepeaterHorizontalAlignment.SelectedIndex;
            }
        }

        private void UpdateCmbItemsRepeaterHorizontalAlignment()
        {
            if (itemsRepeater != null)
            {
                cmbItemsRepeaterHorizontalAlignment.SelectedIndex = (int)itemsRepeater.HorizontalAlignment;
            }
        }

        private void CmbItemsRepeaterVerticalAlignment_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (itemsRepeater != null)
            {
                itemsRepeater.VerticalAlignment = (VerticalAlignment)cmbItemsRepeaterVerticalAlignment.SelectedIndex;
            }
        }

        private void UpdateCmbItemsRepeaterVerticalAlignment()
        {
            if (itemsRepeater != null)
            {
                cmbItemsRepeaterVerticalAlignment.SelectedIndex = (int)itemsRepeater.VerticalAlignment;
            }
        }

        private void BtnGetLayoutSpacing_Click(object sender, RoutedEventArgs e)
        {
            UpdateLayoutSpacing();
        }

        private void BtnGetLayoutLogItemIndexDbg_Click(object sender, RoutedEventArgs e)
        {
            UpdateLayoutLogItemIndexDbg();
        }

        private void BtnGetItemsSourceItemsCount_Click(object sender, RoutedEventArgs e)
        {
            UpdateItemsSourceItemsCount();
        }

        private void BtnSetLayoutSpacing_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (itemsRepeater != null)
                {
                    double spacing = Convert.ToDouble(txtLayoutSpacing.Text);
                    StackLayout stackLayout = itemsRepeater.Layout as StackLayout;

                    if (stackLayout == null)
                    {
                        UniformGridLayout uniformGridLayout = itemsRepeater.Layout as UniformGridLayout;
                        uniformGridLayout.MinColumnSpacing = spacing;
                        uniformGridLayout.MinRowSpacing = spacing;
                    }
                    else
                    {
                        stackLayout.Spacing = spacing;
                    }
                }
            }
            catch (Exception ex)
            {
                string message = "BtnSetLayoutSpacing_Click. Exception:" + ex.ToString();

                Debug.WriteLine(message);
                AppendEventMessage(message);
            }
        }

        private void BtnSetLayoutLogItemIndexDbg_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (itemsRepeater != null)
                {
                    Layout layout = itemsRepeater.Layout;

                    if (layout != null)
                    {
                        LayoutsTestHooks.SetLayoutLogItemIndex(layout, Convert.ToInt32(txtLayoutLogItemIndexDbg.Text));
                    }
                }
            }
            catch (Exception ex)
            {
                string message = "BtnSetLayoutLogItemIndexDbg_Click. Exception:" + ex.ToString();

                Debug.WriteLine(message);
                AppendEventMessage(message);
            }
        }

        private void BtnSetItemsSourceItemsCount_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (itemsRepeater != null && itemsRepeater.ItemsSourceView != null && txtItemsSourceItemsCount != null)
                {
                    if (int.Parse(txtItemsSourceItemsCount.Text) != itemsRepeater.ItemsSourceView.Count)
                    {
                        UpdateItemsSource();
                    }
                }
            }
            catch (Exception ex)
            {
                string message = "BtnSetItemsSourceItemsCount_Click. Exception:" + ex.ToString();

                Debug.WriteLine(message);
                AppendEventMessage(message);
            }
        }

        private void ChkLogEvents_CheckedChanged(object sender, RoutedEventArgs e)
        {
            m_logEvents = (bool)chkLogEvents.IsChecked;
        }

        private void BtnClearEvents_Click(object sender, RoutedEventArgs e)
        {
            if (lstEvents != null)
            {
                lstEvents.Items.Clear();
            }
        }

        private Vector2 ConvertFromStringToVector2(string value)
        {
            float x = 0, y = 0;

            int commaIndex = value.IndexOf(',');
            if (commaIndex != -1)
            {
                x = Convert.ToSingle(value.Substring(0, commaIndex));
                y = Convert.ToSingle(value.Substring(commaIndex + 1));
            }

            return new Vector2(x, y);
        }
    }

    public class FixedSizeItem
    {
        public FixedSizeItem()
        {
            Size = 50;
        }

        public string Text { get; set; }
        public int Size { get; set; }
    }

    public class VariableSizeItem
    {
        public VariableSizeItem(int sizeSeed)
        {
            long size = (sizeSeed + 9) * (sizeSeed + 10) * (sizeSeed + 11);
            string sizeStr = size.ToString();
            sizeStr = sizeStr.Substring(sizeStr.Length - 2);
            size = long.Parse(sizeStr) * 2;
            Size = Math.Max((int)size, 24);
        }

        public string Text { get; set; }
        public int Size { get; set; }
    }
}
