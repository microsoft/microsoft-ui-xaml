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
    public sealed partial class VariableSizedItemsPage : Page
    {
        private double m_extentHeight = 0;
        private int m_changeViewToVerticalOffsetWithAnimationCorrelationId = -1;
        private int m_changeViewToVerticalScrollPercentageWithAnimationCorrelationId = -1;

        public VariableSizedItemsPage()
        {
            this.InitializeComponent();
            goBackButton.Click += delegate { Frame.GoBack(); };

            itemsRepeater.ItemsSource = Enumerable.Range(0, 10000).Select(x => new VariableSizeItem(x)
            {
                Text = "Item " + x
            });

            Loaded += VariableSizedItemsPage_Loaded;
        }

        ~VariableSizedItemsPage()
        {
        }

        protected override void OnNavigatedFrom(NavigationEventArgs e)
        {
            UnhookScrollersEvents();

            base.OnNavigatedFrom(e);
        }

        private void AppendEventMessage(string message)
        {
            if (lstEvents != null)
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
                    txtScrollerVerticalOffset.Text = scrollViewer.VerticalOffset.ToString();
                }
                else if (scrollView != null)
                {
                    txtScrollerVerticalOffset.Text = scrollView.VerticalOffset.ToString();
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
                    txtScrollerZoomFactor.Text = scrollViewer.ZoomFactor.ToString();
                }
                else if (scrollView != null)
                {
                    txtScrollerZoomFactor.Text = scrollView.ZoomFactor.ToString();
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
                        txtScrollerVerticalScrollPercentage.Text = (scrollViewer.VerticalOffset / scrollViewer.ScrollableHeight * 100.0).ToString();
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
                        txtScrollerVerticalScrollPercentage.Text = (scrollView.VerticalOffset / scrollView.ScrollableHeight * 100.0).ToString();
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
                    txtScrollerScrollableHeight.Text = scrollViewer.ScrollableHeight.ToString();
                }
                else if (scrollView != null)
                {
                    txtScrollerScrollableHeight.Text = scrollView.ScrollableHeight.ToString();
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
                    txtScrollerExtentHeight.Text = scrollViewer.ExtentHeight.ToString();
                }
                else if (scrollView != null)
                {
                    txtScrollerExtentHeight.Text = scrollView.ExtentHeight.ToString();
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
                AppendEventMessage("ExtentHeight changed. Old=" + m_extentHeight + ", New=" + extentHeight);

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
                        txtScrollerChangeViewVerticalScrollPercentage.Text = (verticalOffset / scrollViewer.ScrollableHeight * 100.0).ToString();
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
                        txtScrollerChangeViewVerticalScrollPercentage.Text = (verticalOffset / scrollView.ScrollableHeight * 100.0).ToString();
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
            UpdateItemsRepeaterVerticalCacheLength();
            UpdateCmbItemsRepeaterHorizontalAlignment();
            UpdateCmbItemsRepeaterVerticalAlignment();
            UpdateScrollerVerticalAnchorRatio();
            UpdateScrollerVerticalOffset();
            UpdateScrollerZoomFactor();
            UpdateScrollerVerticalScrollPercentage();
            UpdateScrollerScrollableHeight();
            UpdateScrollerExtentHeight();
            UpdateScrollerChangeViewVerticalScrollPercentage();
        }

        private string ScrollViewerViewToString(ScrollViewerView scrollViewerView)
        {
            return "H=" + scrollViewerView.HorizontalOffset.ToString() + ", V=" + scrollViewerView.VerticalOffset + ", ZF=" + scrollViewerView.ZoomFactor;
        }

        private void ScrollViewer_ViewChanging(object sender, ScrollViewerViewChangingEventArgs args)
        {
            AppendEventMessage("ViewChanging IsInertial=" + args.IsInertial + ", NextView=(" + ScrollViewerViewToString(args.NextView) + "), FinalView=(" + ScrollViewerViewToString(args.FinalView) + ")");
        }

        private void ScrollViewer_ViewChanged(object sender, ScrollViewerViewChangedEventArgs args)
        {
            AppendEventMessage("ViewChanged IsIntermediate=" + args.IsIntermediate + ", H=" + scrollViewer.HorizontalOffset.ToString() + ", V=" + scrollViewer.VerticalOffset + ", ZF=" + scrollViewer.ZoomFactor);

            UpdateExtentHeight();
        }

        private void ScrollView_ViewChanged(ScrollView sender, object args)
        {
            AppendEventMessage("ViewChanged H=" + scrollView.HorizontalOffset.ToString() + ", V=" + scrollView.VerticalOffset + ", ZF=" + scrollView.ZoomFactor);

            UpdateExtentHeight();
        }

        private void ScrollView_ScrollAnimationStarting(ScrollView sender, ScrollingScrollAnimationStartingEventArgs args)
        {
            AppendEventMessage($"ScrollAnimationStarting CorrelationId={args.CorrelationId}, SP=({args.StartPosition.X}, {args.StartPosition.Y}), EP=({args.EndPosition.X}, {args.EndPosition.Y})");

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
                scrollView.Visibility = Visibility.Collapsed;
                scrollViewer.Visibility = Visibility.Visible;
                scrollView.Content = null;
                scrollViewer.Content = itemsRepeater;
            }
        }

        private void BtnSwitchVariability_Click(object sender, RoutedEventArgs e)
        {
            if (btnSwitchVariability.Content as string == "Use fixed heights")
            {
                btnSwitchVariability.Content = "Use variable heights";
                itemsRepeater.ItemsSource = Enumerable.Range(0, 10000).Select(x => new FixedSizeItem()
                {
                    Text = "Item " + x
                });
            }
            else
            {
                btnSwitchVariability.Content = "Use fixed heights";
                itemsRepeater.ItemsSource = Enumerable.Range(0, 10000).Select(x => new VariableSizeItem(x)
                {
                    Text = "Item " + x
                });
            }
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
                    txtScrollerChangeViewVerticalOffset.Text = (scrollViewer.ScrollableHeight * verticalScrollPercentage / 100.0).ToString();
                }
                else if (scrollView != null)
                {
                    txtScrollerChangeViewVerticalOffset.Text = (scrollView.ScrollableHeight * verticalScrollPercentage / 100.0).ToString();
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
