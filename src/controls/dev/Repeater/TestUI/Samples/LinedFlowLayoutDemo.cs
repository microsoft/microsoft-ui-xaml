// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using Microsoft.UI;
using Microsoft.UI.Private.Controls;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Media.Imaging;
using Microsoft.UI.Xaml.Navigation;
using MUXControlsTestApp.Samples.Model;
using Windows.Foundation;

namespace MUXControlsTestApp.Samples
{
    public sealed partial class LinedFlowLayoutDemo : TestPage
    {
        private object _asyncEventReportingLock = new object();
        private List<string> _lstAsyncEventMessage = new List<string>();
        private List<int> _lstLinedFlowLayoutLockedItemIndexes = new List<int>();
        private SolidColorBrush _grayBrush = new SolidColorBrush(Colors.Gray);
        private SolidColorBrush _whiteBrush = new SolidColorBrush(Colors.White);
        private Thickness _thicknessFour = new Thickness(4);
        private CornerRadius _cornerRadiusFour = new CornerRadius(4);
        private Layout _layout = null;

        public LinedFlowLayoutDemo()
        {
            this.InitializeComponent();

            if (chkLogScrollViewMessages.IsChecked == true || chkLogLinedFlowLayoutMessages.IsChecked == true)
            {
                MUXControlsTestHooks.LoggingMessage += MUXControlsTestHooks_LoggingMessage;

                if (chkLogLinedFlowLayoutMessages.IsChecked == true)
                {
                    LayoutsTestHooks.LinedFlowLayoutInvalidated += LayoutsTestHooks_LinedFlowLayoutInvalidated;
                    LayoutsTestHooks.LinedFlowLayoutItemLocked += LayoutsTestHooks_LinedFlowLayoutItemLocked;
                    LayoutsTestHooks.LinedFlowLayoutSnappedAverageItemsPerLineChanged += LayoutsTestHooks_LinedFlowLayoutSnappedAverageItemsPerLineChanged;

                    MUXControlsTestHooks.SetLoggingLevelForType("LinedFlowLayout", isLoggingInfoLevel: true, isLoggingVerboseLevel: true);
                }

                if (chkLogScrollViewMessages.IsChecked == true)
                {
                    MUXControlsTestHooks.SetLoggingLevelForType("ScrollView", isLoggingInfoLevel: true, isLoggingVerboseLevel: true);
                }
            }

            HookLinedFlowLayoutEvents();

            if (chkLogScrollViewEvents.IsChecked == true)
            {
                HookScrollViewEvents();
            }

            this.Loaded += LinedFlowLayoutDemo_Loaded;
            LayoutsTestHooks.LinedFlowLayoutItemLocked += LayoutsTestHooks_LinedFlowLayoutItemLocked2;
        }

        ~LinedFlowLayoutDemo()
        {
        }

        protected override void OnNavigatedFrom(NavigationEventArgs e)
        {
            LayoutsTestHooks.LinedFlowLayoutItemLocked -= LayoutsTestHooks_LinedFlowLayoutItemLocked2;
            LayoutsTestHooks.LinedFlowLayoutInvalidated -= LayoutsTestHooks_LinedFlowLayoutInvalidated;
            LayoutsTestHooks.LinedFlowLayoutItemLocked -= LayoutsTestHooks_LinedFlowLayoutItemLocked;
            LayoutsTestHooks.LinedFlowLayoutSnappedAverageItemsPerLineChanged -= LayoutsTestHooks_LinedFlowLayoutSnappedAverageItemsPerLineChanged;

            MUXControlsTestHooks.SetLoggingLevelForType("LinedFlowLayout", isLoggingInfoLevel: false, isLoggingVerboseLevel: false);
            MUXControlsTestHooks.SetLoggingLevelForType("ScrollView", isLoggingInfoLevel: false, isLoggingVerboseLevel: false);

            MUXControlsTestHooks.LoggingMessage -= MUXControlsTestHooks_LoggingMessage;

            UnhookScrollViewEvents();

            base.OnNavigatedFrom(e);
        }

        private void AddChild()
        {
            try
            {
                if (layoutPanel != null)
                {
                    layoutPanel.Children.Add(CreateChild(layoutPanel.Children.Count));
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void AppendAsyncEventMessage(string asyncEventMessage)
        {
            lock (_asyncEventReportingLock)
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

                    _lstAsyncEventMessage.Add(msgHead);
                }
                var ignored = this.DispatcherQueue.TryEnqueue(Microsoft.UI.Dispatching.DispatcherQueuePriority.Normal, AppendAsyncEventMessage);

            }
        }

        private void AppendAsyncEventMessage()
        {
            lock (_asyncEventReportingLock)
            {
                foreach (string asyncEventMessage in _lstAsyncEventMessage)
                {
                    AppendEventMessage(asyncEventMessage);
                }
                _lstAsyncEventMessage.Clear();
            }
        }

        private void AppendEventMessage(string eventMessage)
        {
            lstEvents.Items.Add(eventMessage);

            if ((bool)chkOutputDebugString.IsChecked)
            {
                System.Diagnostics.Debug.WriteLine(eventMessage);
            }
        }

        private void BtnAddChild_Click(object sender, RoutedEventArgs e)
        {
            AppendAsyncEventMessage("AddChild");
            AddChild();
        }

        private void BtnClearExceptionReport_Click(object sender, RoutedEventArgs e)
        {
            txtExceptionReport.Text = string.Empty;
        }

        private void BtnClearEvents_Click(object sender, RoutedEventArgs e)
        {
            lstEvents.Items.Clear();
        }

        private void BtnCopyEvents_Click(object sender, RoutedEventArgs e)
        {
            string logs = string.Empty;

            foreach (object log in lstEvents.Items)
            {
                logs += log.ToString() + "\n";
            }

            var dataPackage = new Windows.ApplicationModel.DataTransfer.DataPackage();
            dataPackage.SetText(logs);
            Windows.ApplicationModel.DataTransfer.Clipboard.SetContent(dataPackage);
        }

        private void BtnGetChildrenCount_Click(object sender, RoutedEventArgs e)
        {
            UpdateChildrenCount();
        }

        private void BtnGetLayoutPanelBorderThickness_Click(object sender, RoutedEventArgs e)
        {
            UpdateLayoutPanelBorderThickness();
        }

        private void BtnGetLayoutPanelCornerRadius_Click(object sender, RoutedEventArgs e)
        {
            UpdateLayoutPanelCornerRadius();
        }

        private void BtnGetLayoutPanelElementActualHeight_Click(object sender, RoutedEventArgs e)
        {
            UpdateLayoutPanelElementActualHeight();
        }

        private void BtnGetLayoutPanelElementActualWidth_Click(object sender, RoutedEventArgs e)
        {
            UpdateLayoutPanelElementActualWidth();
        }

        private void BtnGetLayoutPanelElementCornerRadius_Click(object sender, RoutedEventArgs e)
        {
            UpdateLayoutPanelElementCornerRadius();
        }

        private void BtnGetLayoutPanelElementIsEnabled_Click(object sender, RoutedEventArgs e)
        {
            UpdateLayoutPanelElementIsEnabled();
        }

        private void BtnGetLayoutPanelElementMinWidth_Click(object sender, RoutedEventArgs e)
        {
            UpdateLayoutPanelElementMinWidth();
        }

        private void BtnGetLayoutPanelElementMaxWidth_Click(object sender, RoutedEventArgs e)
        {
            UpdateLayoutPanelElementMaxWidth();
        }

        private void BtnGetLayoutPanelElementWidth_Click(object sender, RoutedEventArgs e)
        {
            UpdateLayoutPanelElementWidth();
        }

        private void BtnGetLayoutPanelElementMinHeight_Click(object sender, RoutedEventArgs e)
        {
            UpdateLayoutPanelElementMinHeight();
        }

        private void BtnGetLayoutPanelElementMaxHeight_Click(object sender, RoutedEventArgs e)
        {
            UpdateLayoutPanelElementMaxHeight();
        }

        private void BtnGetLayoutPanelElementHeight_Click(object sender, RoutedEventArgs e)
        {
            UpdateLayoutPanelElementHeight();
        }

        private void BtnGetLayoutPanelHeight_Click(object sender, RoutedEventArgs e)
        {
            UpdateLayoutPanelHeight();
        }

        private void BtnGetLayoutPanelMargin_Click(object sender, RoutedEventArgs e)
        {
            UpdateLayoutPanelMargin();
        }

        private void BtnGetLayoutPanelWidth_Click(object sender, RoutedEventArgs e)
        {
            UpdateLayoutPanelWidth();
        }

        private void BtnGetLayoutPanelMaxHeight_Click(object sender, RoutedEventArgs e)
        {
            UpdateLayoutPanelMaxHeight();
        }

        private void BtnGetLayoutPanelMaxWidth_Click(object sender, RoutedEventArgs e)
        {
            UpdateLayoutPanelMaxWidth();
        }

        private void BtnGetLayoutPanelPadding_Click(object sender, RoutedEventArgs e)
        {
            UpdateLayoutPanelPadding();
        }

        private void BtnGetLayoutPanelHorizontalAlignment_Click(object sender, RoutedEventArgs e)
        {
            UpdateLayoutPanelHorizontalAlignment();
        }

        private void BtnGetLayoutPanelVerticalAlignment_Click(object sender, RoutedEventArgs e)
        {
            UpdateLayoutPanelVerticalAlignment();
        }

        private void BtnGetLinedFlowLayoutActualLineHeight_Click(object sender, RoutedEventArgs e)
        {
            UpdateLinedFlowLayoutActualLineHeight();
        }

        private void BtnGetLinedFlowLayoutAverageItemAspectRatioDbg_Click(object sender, RoutedEventArgs e)
        {
            UpdateLinedFlowLayoutAverageItemAspectRatioDbg();
        }

        private void BtnGetLinedFlowLayoutForcedAverageItemAspectRatioDbg_Click(object sender, RoutedEventArgs e)
        {
            UpdateLinedFlowLayoutForcedAverageItemAspectRatioDbg();
        }

        private void BtnGetLinedFlowLayoutForcedAverageItemsPerLineDividerDbg_Click(object sender, RoutedEventArgs e)
        {
            UpdateLinedFlowLayoutForcedAverageItemsPerLineDividerDbg();
        }

        private void BtnGetLinedFlowLayoutForcedWrapMultiplierDbg_Click(object sender, RoutedEventArgs e)
        {
            UpdateLinedFlowLayoutForcedWrapMultiplierDbg();
        }

        private void BtnGetLinedFlowLayoutFrozenItemIndexesDbg_Click(object sender, RoutedEventArgs e)
        {
            UpdateLinedFlowLayoutFrozenItemIndexes();
        }

        private void BtnGetLinedFlowLayoutLineHeight_Click(object sender, RoutedEventArgs e)
        {
            UpdateLinedFlowLayoutLineHeight();
        }

        private void BtnGetLinedFlowLayoutLineSpacing_Click(object sender, RoutedEventArgs e)
        {
            UpdateLinedFlowLayoutLineSpacing();
        }

        private void BtnGetLinedFlowLayoutLogItemIndexDbg_Click(object sender, RoutedEventArgs e)
        {
            UpdateLinedFlowLayoutLogItemIndexDbg();
        }

        private void BtnGetLinedFlowLayoutMinItemSpacing_Click(object sender, RoutedEventArgs e)
        {
            UpdateLinedFlowLayoutMinItemSpacing();
        }

        private void BtnGetLinedFlowLayoutRealizedItemIndexesDbg_Click(object sender, RoutedEventArgs e)
        {
            UpdateLinedFlowLayoutRealizedItemIndexes();
        }

        private void BtnGetLinedFlowLayoutSnappedAverageItemsPerLineDbg_Click(object sender, RoutedEventArgs e)
        {
            UpdateLinedFlowLayoutSnappedAverageItemsPerLineDbg();
        }

        private void BtnGetScrollViewBorderThickness_Click(object sender, RoutedEventArgs e)
        {
            UpdateScrollViewBorderThickness();
        }

        private void BtnGetScrollViewContentOrientation_Click(object sender, RoutedEventArgs e)
        {
            UpdateScrollViewContentOrientation();
        }

        private void BtnGetScrollViewCornerRadius_Click(object sender, RoutedEventArgs e)
        {
            UpdateScrollViewCornerRadius();
        }

        private void BtnGetScrollViewExtentHeight_Click(object sender, RoutedEventArgs e)
        {
            UpdateScrollViewExtentHeight();
        }

        private void BtnGetScrollViewExtentWidth_Click(object sender, RoutedEventArgs e)
        {
            UpdateScrollViewExtentWidth();
        }

        private void BtnGetScrollViewHeight_Click(object sender, RoutedEventArgs e)
        {
            UpdateScrollViewHeight();
        }

        private void BtnGetScrollViewHorizontalAnchorRatio_Click(object sender, RoutedEventArgs e)
        {
            UpdateScrollViewHorizontalAnchorRatio();
        }

        private void BtnGetScrollViewHorizontalOffset_Click(object sender, RoutedEventArgs e)
        {
            UpdateScrollViewHorizontalOffset();
        }

        private void BtnGetScrollViewHorizontalScrollMode_Click(object sender, RoutedEventArgs e)
        {
            UpdateScrollViewHorizontalScrollMode();
        }

        private void BtnGetScrollViewHorizontalScrollBarVisibility_Click(object sender, RoutedEventArgs e)
        {
            UpdateScrollViewHorizontalScrollBarVisibility();
        }

        private void BtnGetScrollViewMargin_Click(object sender, RoutedEventArgs e)
        {
            UpdateScrollViewMargin();
        }

        private void BtnGetScrollViewMaxHeight_Click(object sender, RoutedEventArgs e)
        {
            UpdateScrollViewMaxHeight();
        }

        private void BtnGetScrollViewMaxWidth_Click(object sender, RoutedEventArgs e)
        {
            UpdateScrollViewMaxWidth();
        }

        private void BtnGetScrollViewPadding_Click(object sender, RoutedEventArgs e)
        {
            UpdateScrollViewPadding();
        }

        private void BtnInsertChild_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (txtChildIndex != null)
                {
                    int newChildIndex = int.Parse(txtChildIndex.Text);

                    AppendAsyncEventMessage("InsertChild " + newChildIndex);
                    InsertChild(newChildIndex);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void BtnLayoutPanelResetLayout_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (layoutPanel != null && layoutPanel.Layout != null)
                {
                    _layout = layoutPanel.Layout;
                    layoutPanel.Layout = null;
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void BtnLayoutPanelSetLayout_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (layoutPanel != null && layoutPanel.Layout == null)
                {
                    layoutPanel.Layout = _layout;
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void BtnLayoutInvalidateItemsInfo_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (linedFlowLayout != null)
                {
                    linedFlowLayout.InvalidateItemsInfo();
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void BtnLayoutInvalidateMeasure_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (linedFlowLayout != null)
                {
                    LayoutsTestHooks.LayoutInvalidateMeasure(linedFlowLayout, relayout: true);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void BtnRemoveAllChildren_Click(object sender, RoutedEventArgs e)
        {
            AppendAsyncEventMessage("RemoveAllChildren");
            RemoveAllChildren();
        }

        private void BtnRemoveChild_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (txtChildIndex != null)
                {
                    int oldChildIndex = int.Parse(txtChildIndex.Text);

                    AppendAsyncEventMessage("RemoveChild " + oldChildIndex);
                    RemoveChild(oldChildIndex);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void BtnReplaceChild_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (txtChildIndex != null)
                {
                    int childIndex = int.Parse(txtChildIndex.Text);

                    AppendAsyncEventMessage("ReplaceChild " + childIndex);
                    ReplaceChild(childIndex);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void BtnLinedFlowLayoutLockItemToLine_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (linedFlowLayout != null)
                {
                    int itemIndex = int.Parse(txtLinedFlowLayoutItemIndex.Text);
                    int lineIndex = linedFlowLayout.LockItemToLine(itemIndex);

                    txtLinedFlowLayoutLockLine.Text = lineIndex.ToString();
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void BtnLinedFlowLayoutUnlockItems_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (linedFlowLayout != null)
                {
                    LayoutsTestHooks.UnlockLinedFlowLayoutItems(linedFlowLayout);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void BtnSetChildrenCount_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (layoutPanel != null && txtChildrenCount != null)
                {
                    int newChildrenCount = int.Parse(txtChildrenCount.Text);

                    if (layoutPanel.Children.Count < newChildrenCount)
                    {
                        int addCount = newChildrenCount - layoutPanel.Children.Count;

                        for (int childIndex = 0; childIndex < addCount; childIndex++)
                        {
                            layoutPanel.Children.Add(CreateChild(layoutPanel.Children.Count));
                        }
                    }
                    else
                    {
                        while (layoutPanel.Children.Count > newChildrenCount)
                        {
                            layoutPanel.Children.RemoveAt(layoutPanel.Children.Count - 1);
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void BtnSetLayoutPanelBorderThickness_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (layoutPanel != null)
                {
                    layoutPanel.BorderThickness = GetThicknessFromString(txtLayoutPanelBorderThickness.Text);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void BtnSetLayoutPanelElementCornerRadius_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (layoutPanel != null)
                {
                    UIElement element = GetLayoutPanelElement();

                    if (element is Grid grid)
                    {
                        grid.CornerRadius = GetCornerRadiusFromString(txtLayoutPanelElementCornerRadius.Text);
                    }
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void BtnSetLayoutPanelElementIsEnabled_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (layoutPanel != null)
                {
                    UIElement element = GetLayoutPanelElement();

                    if (element is Control control)
                    {
                        control.IsEnabled = (bool)chkLayoutPanelElementIsEnabled.IsChecked;
                    }
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void BtnSetLayoutPanelElementMinWidth_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (layoutPanel != null)
                {
                    UIElement element = GetLayoutPanelElement();

                    if (element is FrameworkElement frameworkElement)
                    {
                        frameworkElement.MinWidth = double.Parse(txtLayoutPanelElementMinWidth.Text);
                    }
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void BtnSetLayoutPanelElementMaxWidth_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (layoutPanel != null)
                {
                    UIElement element = GetLayoutPanelElement();

                    if (element is FrameworkElement frameworkElement)
                    {
                        frameworkElement.MaxWidth = double.Parse(txtLayoutPanelElementMaxWidth.Text);
                    }
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void BtnSetLayoutPanelElementWidth_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (layoutPanel != null)
                {
                    UIElement element = GetLayoutPanelElement();

                    if (element is FrameworkElement frameworkElement)
                    {
                        frameworkElement.Width = double.Parse(txtLayoutPanelElementWidth.Text);
                    }
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void BtnSetLayoutPanelElementMinHeight_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (layoutPanel != null)
                {
                    UIElement element = GetLayoutPanelElement();

                    if (element is FrameworkElement frameworkElement)
                    {
                        frameworkElement.MinHeight = double.Parse(txtLayoutPanelElementMinHeight.Text);
                    }
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void BtnSetLayoutPanelElementMaxHeight_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (layoutPanel != null)
                {
                    UIElement element = GetLayoutPanelElement();

                    if (element is FrameworkElement frameworkElement)
                    {
                        frameworkElement.MaxHeight = double.Parse(txtLayoutPanelElementMaxHeight.Text);
                    }
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void BtnSetLayoutPanelElementHeight_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (layoutPanel != null)
                {
                    UIElement element = GetLayoutPanelElement();

                    if (element is FrameworkElement frameworkElement)
                    {
                        frameworkElement.Height = double.Parse(txtLayoutPanelElementHeight.Text);
                    }
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void BtnSetLayoutPanelHorizontalAlignment_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (layoutPanel != null)
                {
                    layoutPanel.HorizontalAlignment = (HorizontalAlignment)cmbLayoutPanelHorizontalAlignment.SelectedIndex;
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void BtnSetLayoutPanelVerticalAlignment_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (layoutPanel != null)
                {
                    layoutPanel.VerticalAlignment = (VerticalAlignment)cmbLayoutPanelVerticalAlignment.SelectedIndex;
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void BtnSetLayoutPanelCornerRadius_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (layoutPanel != null)
                {
                    layoutPanel.CornerRadius = GetCornerRadiusFromString(txtLayoutPanelCornerRadius.Text);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void BtnSetLayoutPanelHeight_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (layoutPanel != null)
                {
                    layoutPanel.Height = Convert.ToDouble(txtLayoutPanelHeight.Text);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void BtnSetLayoutPanelMargin_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (layoutPanel != null)
                {
                    layoutPanel.Margin = GetThicknessFromString(txtLayoutPanelMargin.Text);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void BtnSetLayoutPanelMaxHeight_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (layoutPanel != null)
                {
                    layoutPanel.MaxHeight = Convert.ToDouble(txtLayoutPanelMaxHeight.Text);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void BtnSetLayoutPanelWidth_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (layoutPanel != null)
                {
                    layoutPanel.Width = Convert.ToDouble(txtLayoutPanelWidth.Text);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void BtnSetLayoutPanelMaxWidth_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (layoutPanel != null)
                {
                    layoutPanel.MaxWidth = Convert.ToDouble(txtLayoutPanelMaxWidth.Text);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void BtnSetLayoutPanelPadding_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (layoutPanel != null)
                {
                    layoutPanel.Padding = GetThicknessFromString(txtLayoutPanelPadding.Text);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void BtnSetLinedFlowLayoutForcedAverageItemAspectRatioDbg_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (linedFlowLayout != null)
                {
                    LayoutsTestHooks.SetLinedFlowLayoutForcedAverageItemAspectRatio(linedFlowLayout, Convert.ToDouble(txtLinedFlowLayoutForcedAverageItemAspectRatioDbg.Text));
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void BtnSetLinedFlowLayoutForcedAverageItemsPerLineDividerDbg_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (linedFlowLayout != null)
                {
                    LayoutsTestHooks.SetLinedFlowLayoutForcedAverageItemsPerLineDivider(linedFlowLayout, Convert.ToDouble(txtLinedFlowLayoutForcedAverageItemsPerLineDividerDbg.Text));
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void BtnSetLinedFlowLayoutForcedWrapMultiplierDbg_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (linedFlowLayout != null)
                {
                    LayoutsTestHooks.SetLinedFlowLayoutForcedWrapMultiplier(linedFlowLayout, Convert.ToDouble(txtLinedFlowLayoutForcedWrapMultiplierDbg.Text));
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void BtnSetLinedFlowLayoutLineHeight_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (linedFlowLayout != null)
                {
                    linedFlowLayout.LineHeight = Convert.ToDouble(txtLinedFlowLayoutLineHeight.Text);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void BtnSetLinedFlowLayoutLineSpacing_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (linedFlowLayout != null)
                {
                    linedFlowLayout.LineSpacing = Convert.ToDouble(txtLinedFlowLayoutLineSpacing.Text);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void BtnSetLinedFlowLayoutLogItemIndexDbg_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (linedFlowLayout != null)
                {
                    LayoutsTestHooks.SetLinedFlowLayoutLogItemIndex(linedFlowLayout, Convert.ToInt32(txtLinedFlowLayoutLogItemIndexDbg.Text));
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void BtnSetLinedFlowLayoutMinItemSpacing_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (linedFlowLayout != null)
                {
                    linedFlowLayout.MinItemSpacing = Convert.ToDouble(txtLinedFlowLayoutMinItemSpacing.Text);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void BtnSetScrollViewBorderThickness_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (scrollView != null)
                {
                    scrollView.BorderThickness = GetThicknessFromString(txtScrollViewBorderThickness.Text);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void BtnSetScrollViewContentOrientation_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (scrollView != null)
                {
                    ScrollingContentOrientation co = (ScrollingContentOrientation)cmbScrollViewContentOrientation.SelectedIndex;

                    scrollView.ContentOrientation = co;
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void BtnSetScrollViewCornerRadius_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (scrollView != null)
                {
                    scrollView.CornerRadius = GetCornerRadiusFromString(txtScrollViewCornerRadius.Text);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void BtnSetScrollViewHeight_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (scrollView != null)
                {
                    scrollView.Height = Convert.ToDouble(txtScrollViewHeight.Text);

                    if (canvasLinedFlowLayoutLockedItems != null)
                    {
                        canvasLinedFlowLayoutLockedItems.Height = scrollView.Height;
                    }
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void BtnSetScrollViewWidth_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (scrollView != null)
                {
                    scrollView.Width = Convert.ToDouble(txtScrollViewWidth.Text);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void BtnSetScrollViewHorizontalAnchorRatio_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (scrollView != null)
                {
                    scrollView.HorizontalAnchorRatio = Convert.ToDouble(txtScrollViewHorizontalAnchorRatio.Text);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void BtnSetScrollViewHorizontalOffset_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (scrollView != null)
                {
                    scrollView.ScrollTo(Convert.ToDouble(txtScrollViewHorizontalOffset.Text), scrollView.VerticalOffset);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void BtnSetScrollViewHorizontalScrollBarVisibility_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (scrollView != null)
                {
                    scrollView.HorizontalScrollBarVisibility = (ScrollingScrollBarVisibility)cmbScrollViewHorizontalScrollBarVisibility.SelectedIndex;
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void BtnSetScrollViewHorizontalScrollMode_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (scrollView != null)
                {
                    ScrollingScrollMode ssm = (ScrollingScrollMode)cmbScrollViewHorizontalScrollMode.SelectedIndex;

                    scrollView.HorizontalScrollMode = ssm;
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void BtnSetScrollViewMargin_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (scrollView != null)
                {
                    scrollView.Margin = GetThicknessFromString(txtScrollViewMargin.Text);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void BtnSetScrollViewMaxHeight_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (scrollView != null)
                {
                    scrollView.MaxHeight = Convert.ToDouble(txtScrollViewMaxHeight.Text);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void BtnSetScrollViewMaxWidth_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (scrollView != null)
                {
                    scrollView.MaxWidth = Convert.ToDouble(txtScrollViewMaxWidth.Text);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void BtnSetScrollViewPadding_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (scrollView != null)
                {
                    scrollView.Padding = GetThicknessFromString(txtScrollViewPadding.Text);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void BtnSetScrollViewZoomMode_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (scrollView != null)
                {
                    ScrollingZoomMode szm = (ScrollingZoomMode)cmbScrollViewZoomMode.SelectedIndex;

                    scrollView.ZoomMode = szm;
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void BtnGetScrollViewWidth_Click(object sender, RoutedEventArgs e)
        {
            UpdateScrollViewWidth();
        }

        private void BtnGetScrollViewVerticalAnchorRatio_Click(object sender, RoutedEventArgs e)
        {
            UpdateScrollViewVerticalAnchorRatio();
        }

        private void BtnGetScrollViewVerticalOffset_Click(object sender, RoutedEventArgs e)
        {
            UpdateScrollViewVerticalOffset();
        }

        private void BtnGetScrollViewVerticalScrollBarVisibility_Click(object sender, RoutedEventArgs e)
        {
            UpdateScrollViewVerticalScrollBarVisibility();
        }

        private void BtnGetScrollViewVerticalScrollMode_Click(object sender, RoutedEventArgs e)
        {
            UpdateScrollViewVerticalScrollMode();
        }

        private void BtnGetScrollViewZoomMode_Click(object sender, RoutedEventArgs e)
        {
            UpdateScrollViewZoomMode();
        }

        private void BtnSetScrollViewVerticalAnchorRatio_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (scrollView != null)
                {
                    scrollView.VerticalAnchorRatio = Convert.ToDouble(txtScrollViewVerticalAnchorRatio.Text);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void BtnSetScrollViewVerticalConstantVelocity_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (scrollView != null)
                {
                    float verticalConstantVelocity = float.Parse(this.txtScrollViewVerticalConstantVelocity.Text);

                    if (verticalConstantVelocity == 0.0f)
                    {
                        scrollView.ScrollBy(0, 0, new ScrollingScrollOptions(ScrollingAnimationMode.Disabled, ScrollingSnapPointsMode.Ignore));
                    }
                    else
                    {
                        scrollView.AddScrollVelocity(new System.Numerics.Vector2(0.0f, verticalConstantVelocity), new System.Numerics.Vector2(0.0f, 0.0f));
                    }
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void BtnSetScrollViewVerticalOffset_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (scrollView != null)
                {
                    scrollView.ScrollTo(scrollView.HorizontalOffset, Convert.ToDouble(txtScrollViewVerticalOffset.Text));
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void BtnSetScrollViewVerticalScrollBarVisibility_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (scrollView != null)
                {
                    scrollView.VerticalScrollBarVisibility = (ScrollingScrollBarVisibility)cmbScrollViewVerticalScrollBarVisibility.SelectedIndex;
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void BtnSetScrollViewVerticalScrollMode_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (scrollView != null)
                {
                    ScrollingScrollMode ssm = (ScrollingScrollMode)cmbScrollViewVerticalScrollMode.SelectedIndex;

                    scrollView.VerticalScrollMode = ssm;
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void ChkEvents_Checked(object sender, RoutedEventArgs e)
        {
            if (grdEvents != null)
            {
                grdEvents.Visibility = Visibility.Visible;
                cdEvents.Width = new GridLength(1, GridUnitType.Star);
            }
        }

        private void ChkEvents_Unchecked(object sender, RoutedEventArgs e)
        {
            if (grdEvents != null)
            {
                grdEvents.Visibility = Visibility.Collapsed;
                cdEvents.Width = new GridLength(0);
            }
        }

        private void ChkLayoutPanel_Checked(object sender, RoutedEventArgs e)
        {
            if (svLayoutPanel != null)
                svLayoutPanel.Visibility = Visibility.Visible;
        }

        private void ChkLayoutPanel_Unchecked(object sender, RoutedEventArgs e)
        {
            if (svLayoutPanel != null)
                svLayoutPanel.Visibility = Visibility.Collapsed;
        }

        private void ChkLogLinedFlowLayoutMessages_Checked(object sender, RoutedEventArgs e)
        {
            LayoutsTestHooks.LinedFlowLayoutInvalidated += LayoutsTestHooks_LinedFlowLayoutInvalidated;
            LayoutsTestHooks.LinedFlowLayoutItemLocked += LayoutsTestHooks_LinedFlowLayoutItemLocked;
            LayoutsTestHooks.LinedFlowLayoutSnappedAverageItemsPerLineChanged += LayoutsTestHooks_LinedFlowLayoutSnappedAverageItemsPerLineChanged;

            MUXControlsTestHooks.SetLoggingLevelForType("LinedFlowLayout", isLoggingInfoLevel: true, isLoggingVerboseLevel: true);
            if (chkLogScrollViewMessages.IsChecked == false)
            {
                MUXControlsTestHooks.LoggingMessage += MUXControlsTestHooks_LoggingMessage;
            }
        }

        private void ChkLogLinedFlowLayoutMessages_Unchecked(object sender, RoutedEventArgs e)
        {
            LayoutsTestHooks.LinedFlowLayoutInvalidated -= LayoutsTestHooks_LinedFlowLayoutInvalidated;
            LayoutsTestHooks.LinedFlowLayoutItemLocked -= LayoutsTestHooks_LinedFlowLayoutItemLocked;
            LayoutsTestHooks.LinedFlowLayoutSnappedAverageItemsPerLineChanged -= LayoutsTestHooks_LinedFlowLayoutSnappedAverageItemsPerLineChanged;

            MUXControlsTestHooks.SetLoggingLevelForType("LinedFlowLayout", isLoggingInfoLevel: false, isLoggingVerboseLevel: false);
            if (chkLogScrollViewMessages.IsChecked == false)
            {
                MUXControlsTestHooks.LoggingMessage -= MUXControlsTestHooks_LoggingMessage;
            }
        }

        private void ChkLogScrollViewMessages_Checked(object sender, RoutedEventArgs e)
        {
            MUXControlsTestHooks.SetLoggingLevelForType("ScrollView", isLoggingInfoLevel: true, isLoggingVerboseLevel: true);
            if (chkLogLinedFlowLayoutMessages.IsChecked == false)
            {
                MUXControlsTestHooks.LoggingMessage += MUXControlsTestHooks_LoggingMessage;
            }
        }

        private void ChkLogScrollViewMessages_Unchecked(object sender, RoutedEventArgs e)
        {
            MUXControlsTestHooks.SetLoggingLevelForType("ScrollView", isLoggingInfoLevel: false, isLoggingVerboseLevel: false);
            if (chkLogLinedFlowLayoutMessages.IsChecked == false)
            {
                MUXControlsTestHooks.LoggingMessage -= MUXControlsTestHooks_LoggingMessage;
            }
        }

        private void ChkLogScrollViewEvents_Checked(object sender, RoutedEventArgs e)
        {
            HookScrollViewEvents();
        }

        private void ChkLogScrollViewEvents_Unchecked(object sender, RoutedEventArgs e)
        {
            UnhookScrollViewEvents();
        }

        private void ChkScrollViewIsEnabled_Checked(object sender, RoutedEventArgs e)
        {
            try
            {
                if (scrollView != null)
                {
                    scrollView.IsEnabled = true;
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void ChkScrollViewIsEnabled_Unchecked(object sender, RoutedEventArgs e)
        {
            try
            {
                if (scrollView != null)
                {
                    scrollView.IsEnabled = false;
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void ChkScrollViewIsTabStop_Checked(object sender, RoutedEventArgs e)
        {
            try
            {
                if (scrollView != null)
                {
                    scrollView.IsTabStop = true;
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void ChkScrollViewIsTabStop_Unchecked(object sender, RoutedEventArgs e)
        {
            try
            {
                if (scrollView != null)
                {
                    scrollView.IsTabStop = false;
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void ChkLinedFlowLayout_Checked(object sender, RoutedEventArgs e)
        {
            if (svLinedFlowLayout != null)
                svLinedFlowLayout.Visibility = Visibility.Visible;
        }

        private void ChkLinedFlowLayout_Unchecked(object sender, RoutedEventArgs e)
        {
            if (svLinedFlowLayout != null)
                svLinedFlowLayout.Visibility = Visibility.Collapsed;
        }

        private void ChkLinedFlowLayoutHandleItemsInfoRequested_IsCheckedChanged(object sender, RoutedEventArgs e)
        {
            BtnLayoutInvalidateItemsInfo_Click(null, null);
        }

        private void ChkLinedFlowLayoutIsFastPathSupportedDbg_IsCheckedChanged(object sender, RoutedEventArgs e)
        {
            try
            {
                if (linedFlowLayout != null)
                {
                    LayoutsTestHooks.SetLinedFlowLayoutIsFastPathSupported(linedFlowLayout, (bool)chkLinedFlowLayoutIsFastPathSupportedDbg.IsChecked);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void ChkLinedFlowLayoutShowLockedItems_Checked(object sender, RoutedEventArgs e)
        {
            UpdateLinedFlowLayoutLockedItemsVisuals();

            if (scrollView != null)
            {
                scrollView.ViewChanged += ScrollView_ViewChangedForLockedItemsVisuals;
            }
        }

        private void ChkLinedFlowLayoutShowLockedItems_Unchecked(object sender, RoutedEventArgs e)
        {
            UpdateLinedFlowLayoutLockedItemsVisuals();

            if (scrollView != null)
            {
                scrollView.ViewChanged -= ScrollView_ViewChangedForLockedItemsVisuals;
            }
        }

        private void ChkScrollView_Checked(object sender, RoutedEventArgs e)
        {
            if (svScrollView != null)
                svScrollView.Visibility = Visibility.Visible;
        }

        private void ChkScrollView_Unchecked(object sender, RoutedEventArgs e)
        {
            if (svScrollView != null)
                svScrollView.Visibility = Visibility.Collapsed;
        }

        private void CmbLayoutPanelBackground_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            try
            {
                if (layoutPanel != null)
                {
                    switch (cmbLayoutPanelBackground.SelectedIndex)
                    {
                        case 0:
                            layoutPanel.Background = null;
                            break;
                        case 1:
                            layoutPanel.Background = new SolidColorBrush(Colors.Transparent);
                            break;
                        case 2:
                            layoutPanel.Background = new SolidColorBrush(Colors.AliceBlue);
                            break;
                        case 3:
                            layoutPanel.Background = new SolidColorBrush(Colors.Aqua);
                            break;
                    }
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void CmbLayoutPanelBorderBrush_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            try
            {
                if (layoutPanel != null)
                {
                    switch (cmbLayoutPanelBorderBrush.SelectedIndex)
                    {
                        case 0:
                            layoutPanel.BorderBrush = null;
                            break;
                        case 1:
                            layoutPanel.BorderBrush = new SolidColorBrush(Colors.Transparent);
                            break;
                        case 2:
                            layoutPanel.BorderBrush = new SolidColorBrush(Colors.Blue);
                            break;
                        case 3:
                            layoutPanel.BorderBrush = new SolidColorBrush(Colors.Green);
                            break;
                    }
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void CmbLinedFlowLayoutItemsJustification_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            try
            {
                if (linedFlowLayout != null && cmbLinedFlowLayoutItemsJustification != null && linedFlowLayout.ItemsJustification != (LinedFlowLayoutItemsJustification)cmbLinedFlowLayoutItemsJustification.SelectedIndex)
                {
                    linedFlowLayout.ItemsJustification = (LinedFlowLayoutItemsJustification)cmbLinedFlowLayoutItemsJustification.SelectedIndex;
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void CmbLinedFlowLayoutItemsStretch_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            try
            {
                if (linedFlowLayout != null && cmbLinedFlowLayoutItemsStretch != null && linedFlowLayout.ItemsStretch != (LinedFlowLayoutItemsStretch)cmbLinedFlowLayoutItemsStretch.SelectedIndex)
                {
                    linedFlowLayout.ItemsStretch = (LinedFlowLayoutItemsStretch)cmbLinedFlowLayoutItemsStretch.SelectedIndex;
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void CmbScrollViewBackground_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            try
            {
                if (scrollView != null)
                {
                    switch (cmbScrollViewBackground.SelectedIndex)
                    {
                        case 0:
                            scrollView.Background = null;
                            break;
                        case 1:
                            scrollView.Background = new SolidColorBrush(Colors.Transparent);
                            break;
                        case 2:
                            scrollView.Background = new SolidColorBrush(Colors.AliceBlue);
                            break;
                        case 3:
                            scrollView.Background = new SolidColorBrush(Colors.Aqua);
                            break;
                    }
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void CmbScrollViewBorderBrush_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            try
            {
                if (scrollView != null)
                {
                    switch (cmbScrollViewBorderBrush.SelectedIndex)
                    {
                        case 0:
                            scrollView.BorderBrush = null;
                            break;
                        case 1:
                            scrollView.BorderBrush = new SolidColorBrush(Colors.Transparent);
                            break;
                        case 2:
                            scrollView.BorderBrush = new SolidColorBrush(Colors.Blue);
                            break;
                        case 3:
                            scrollView.BorderBrush = new SolidColorBrush(Colors.Green);
                            break;
                    }
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private UIElement CreateChild(int id)
        {
            FrameworkElement child;

            if (cmbLayoutPanelElementType.SelectedIndex != 2)
            {
                Uri uri = new Uri(string.Format("ms-appx:///Images/vette{0}.jpg", id % 126 + 1));

                BitmapImage bitmapImage = new BitmapImage()
                {
                    DecodePixelHeight = 120,
                    UriSource = uri
                };

                Image image = new Image()
                {
                    Source = bitmapImage,
                    Stretch = Stretch.UniformToFill,
                    HorizontalAlignment = HorizontalAlignment.Center,
                    VerticalAlignment = VerticalAlignment.Center
                };

                TextBlock textBlock = new TextBlock()
                {
                    Text = id.ToString(),
                    Margin = _thicknessFour,
                    Foreground = _whiteBrush,
                    FontSize = 14.0,
                    MinWidth = 36.0,
                    MaxWidth = 72.0,
                    MaxHeight = 48.0
                };

                Grid gridTB = new Grid()
                {
                    Background = _grayBrush,
                    CornerRadius = _cornerRadiusFour,
                    HorizontalAlignment = HorizontalAlignment.Center,
                    VerticalAlignment = VerticalAlignment.Bottom
                };

                gridTB.Children.Add(textBlock);

                Grid grid = new Grid()
                {
                    Background = _grayBrush,
                    CornerRadius = _cornerRadiusFour
                };

                grid.Children.Add(image);
                grid.Children.Add(gridTB);

                if (cmbLayoutPanelElementType.SelectedIndex == 0)
                {
                    child = grid;
                }
                else
                {
                    ItemContainer itemContainer = new ItemContainer()
                    {
                        Child = grid
                    };

                    child = itemContainer;
                }
            }
            else
            {
                child = new Border()
                {
                    BorderBrush = _grayBrush,
                    BorderThickness = _thicknessFour,
                    CornerRadius = _cornerRadiusFour,
                    MinWidth = 48.0,
                    MinHeight = 48.0
                };
            }

            if (chkSetLayoutPanelElementMinWidth.IsChecked == true)
            {
                child.MinWidth = double.Parse(txtLayoutPanelElementMinWidthAll.Text);
            }

            if (chkSetLayoutPanelElementMaxWidth.IsChecked == true)
            {
                child.MaxWidth = double.Parse(txtLayoutPanelElementMaxWidthAll.Text);
            }

            return child;
        }

        private CornerRadius GetCornerRadiusFromString(string cornerRadius)
        {
            string[] lengths = cornerRadius.Split(',');
            if (lengths.Length < 4)
                return new CornerRadius(
                    Convert.ToDouble(lengths[0]));
            else
                return new CornerRadius(
                    Convert.ToDouble(lengths[0]), Convert.ToDouble(lengths[1]), Convert.ToDouble(lengths[2]), Convert.ToDouble(lengths[3]));
        }

        private UIElement GetLayoutPanelElement()
        {
            try
            {
                if (layoutPanel != null)
                {
                    int index = int.Parse(txtLayoutPanelElementIndex.Text);

                    return layoutPanel.Children[index];
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }

            return null;
        }

        private Thickness GetThicknessFromString(string thickness)
        {
            string[] lengths = thickness.Split(',');
            if (lengths.Length < 4)
                return new Thickness(
                    Convert.ToDouble(lengths[0]));
            else
                return new Thickness(
                    Convert.ToDouble(lengths[0]), Convert.ToDouble(lengths[1]), Convert.ToDouble(lengths[2]), Convert.ToDouble(lengths[3]));
        }

        private void HookLinedFlowLayoutEvents()
        {
            if (linedFlowLayout != null)
            {
                linedFlowLayout.ItemsInfoRequested += LinedFlowLayout_ItemsInfoRequested;
                linedFlowLayout.ItemsUnlocked += LinedFlowLayout_ItemsUnlocked;
            }
        }

        private void HookScrollViewEvents()
        {
            if (scrollView != null)
            {
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

        private void InsertChild(int newChildIndex)
        {
            try
            {
                if (layoutPanel != null)
                {
                    layoutPanel.Children.Insert(newChildIndex, CreateChild(layoutPanel.Children.Count));
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void LayoutsTestHooks_LinedFlowLayoutInvalidated(object sender, LayoutsTestHooksLinedFlowLayoutInvalidatedEventArgs args)
        {
            AppendAsyncEventMessage("LinedFlowLayout invalidated. InvalidationTrigger: " + args.InvalidationTrigger.ToString());
        }

        private void LayoutsTestHooks_LinedFlowLayoutItemLocked(object sender, LayoutsTestHooksLinedFlowLayoutItemLockedEventArgs args)
        {
            AppendAsyncEventMessage("LinedFlowLayout item locked. ItemIndex: " + args.ItemIndex + ", LineIndex: " + args.LineIndex.ToString());
        }

        private void LayoutsTestHooks_LinedFlowLayoutItemLocked2(object sender, LayoutsTestHooksLinedFlowLayoutItemLockedEventArgs args)
        {
            if (!_lstLinedFlowLayoutLockedItemIndexes.Contains(args.ItemIndex))
            {
                _lstLinedFlowLayoutLockedItemIndexes.Add(args.ItemIndex);
                UpdateLinedFlowLayoutLockedItemsVisuals();
            }
        }

        private void LayoutsTestHooks_LinedFlowLayoutSnappedAverageItemsPerLineChanged(object sender, object args)
        {
            AppendAsyncEventMessage("LinedFlowLayout snapped average items per line changed. SnappedAverageItemsPerLine: " + LayoutsTestHooks.GetLinedFlowLayoutSnappedAverageItemsPerLine(sender as LinedFlowLayout).ToString() + ", AverageItemAspectRatio: " + LayoutsTestHooks.GetLinedFlowLayoutAverageItemAspectRatio(sender as LinedFlowLayout).ToString());
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

        private void RemoveAllChildren()
        {
            try
            {
                if (layoutPanel != null)
                {
                    layoutPanel.Children.Clear();
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void RemoveChild(int oldChildIndex)
        {
            try
            {
                if (layoutPanel != null)
                {
                    layoutPanel.Children.RemoveAt(oldChildIndex);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void ReplaceChild(int childIndex)
        {
            try
            {
                if (layoutPanel != null)
                {
                    layoutPanel.Children[childIndex] = CreateChild(childIndex);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void LinedFlowLayout_ItemsInfoRequested(LinedFlowLayout sender, LinedFlowLayoutItemsInfoRequestedEventArgs args)
        {
            if (chkLogLinedFlowLayoutEvents.IsChecked == true)
            {
                AppendAsyncEventMessage($"LinedFlowLayout.ItemsInfoRequested ItemsRangeStartIndex={args.ItemsRangeStartIndex}, ItemsRangeRequestedLength={args.ItemsRangeRequestedLength}");
            }

            if (!(bool)chkLinedFlowLayoutHandleItemsInfoRequested.IsChecked)
            {
                return;
            }

            int arrayLength = args.ItemsRangeRequestedLength;
            double[] desiredAspectRatios = new double[arrayLength];

            if (chkSetLayoutPanelElementMinWidth.IsChecked == true)
            {
                args.MinWidth = double.Parse(txtLayoutPanelElementMinWidthAll.Text);
            }
            else
            {
                args.MinWidth = 48.0;
            }

            if (chkSetLayoutPanelElementMaxWidth.IsChecked == true)
            {
                args.MaxWidth = double.Parse(txtLayoutPanelElementMaxWidthAll.Text);
            }

            for (int index = 0; index < arrayLength; index++)
            {
                desiredAspectRatios[index] = Recipe.GetKnownAspectRatioFromIndex(index + 1);
            }

            args.SetDesiredAspectRatios(desiredAspectRatios);
        }

        private void LinedFlowLayout_ItemsUnlocked(LinedFlowLayout sender, object args)
        {
            if (chkLogLinedFlowLayoutEvents.IsChecked == true)
            {
                AppendAsyncEventMessage("LinedFlowLayout.ItemsUnlocked");
            }

            _lstLinedFlowLayoutLockedItemIndexes.Clear();
            UpdateLinedFlowLayoutLockedItemsVisuals();
        }

        private void LinedFlowLayoutDemo_Loaded(object sender, RoutedEventArgs e)
        {
            try
            {
                UpdateScrollViewBackground();
                UpdateScrollViewBorderBrush();
                UpdateScrollViewBorderThickness();
                UpdateScrollViewMargin();
                UpdateScrollViewPadding();
                UpdateScrollViewWidth();
                UpdateScrollViewHeight();
                UpdateScrollViewMaxWidth();
                UpdateScrollViewMaxHeight();
                UpdateScrollViewCornerRadius();
                UpdateScrollViewIsEnabled();
                UpdateScrollViewIsTabStop();
                UpdateScrollViewContentOrientation();
                UpdateScrollViewHorizontalScrollMode();
                UpdateScrollViewVerticalScrollMode();
                UpdateScrollViewZoomMode();
                UpdateScrollViewHorizontalScrollBarVisibility();
                UpdateScrollViewVerticalScrollBarVisibility();
                UpdateScrollViewHorizontalAnchorRatio();
                UpdateScrollViewVerticalAnchorRatio();
                UpdateScrollViewHorizontalOffset();
                UpdateScrollViewVerticalOffset();
                UpdateScrollViewExtentWidth();
                UpdateScrollViewExtentHeight();

                UpdateLayoutPanelBackground();
                UpdateLayoutPanelBorderBrush();
                UpdateLayoutPanelBorderThickness();
                UpdateLayoutPanelHorizontalAlignment();
                UpdateLayoutPanelVerticalAlignment();
                UpdateLayoutPanelMargin();
                UpdateLayoutPanelPadding();
                UpdateLayoutPanelWidth();
                UpdateLayoutPanelHeight();
                UpdateLayoutPanelMaxWidth();
                UpdateLayoutPanelMaxHeight();
                UpdateLayoutPanelCornerRadius();

                UpdateLinedFlowLayoutActualLineHeight();
                UpdateLinedFlowLayoutAverageItemAspectRatioDbg();
                UpdateLinedFlowLayoutForcedAverageItemAspectRatioDbg();
                UpdateLinedFlowLayoutForcedAverageItemsPerLineDividerDbg();
                UpdateLinedFlowLayoutForcedWrapMultiplierDbg();
                UpdateLinedFlowLayoutFrozenItemIndexes();
                UpdateLinedFlowLayoutLineHeight();
                UpdateLinedFlowLayoutLineSpacing();
                UpdateLinedFlowLayoutItemsJustification();
                UpdateLinedFlowLayoutItemsStretch();
                UpdateLinedFlowLayoutMinItemSpacing();
                UpdateLinedFlowLayoutRealizedItemIndexes();
                UpdateLinedFlowLayoutSnappedAverageItemsPerLineDbg();
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void ScrollView_SizeChanged(object sender, SizeChangedEventArgs e)
        {
            AppendAsyncEventMessage($"ScrollView.SizeChanged Size={scrollView.ActualWidth}, {scrollView.ActualHeight}");
        }

        private void ScrollView_ExtentChanged(ScrollView sender, object args)
        {
            AppendAsyncEventMessage($"ScrollView.ExtentChanged ExtentWidth={sender.ExtentWidth}, ExtentHeight={sender.ExtentHeight}, ScrollableWidth={sender.ScrollableWidth}, ScrollableHeight={sender.ScrollableHeight}");
        }

        private void ScrollView_StateChanged(ScrollView sender, object args)
        {
            AppendAsyncEventMessage($"ScrollView.StateChanged {sender.State.ToString()}");
        }

        private void ScrollView_ViewChanged(ScrollView sender, object args)
        {
            AppendAsyncEventMessage($"ScrollView.ViewChanged HorizontalOffset={sender.HorizontalOffset.ToString()}, VerticalOffset={sender.VerticalOffset}, ZoomFactor={sender.ZoomFactor}");
        }

        private void ScrollView_ViewChangedForLockedItemsVisuals(ScrollView sender, object args)
        {
            UpdateLinedFlowLayoutLockedItemsVisuals();
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

        private void UnhookScrollViewEvents()
        {
            if (scrollView != null)
            {
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

        private void UpdateChildrenCount()
        {
            if (layoutPanel != null && txtChildrenCount != null)
            {
                txtChildrenCount.Text = layoutPanel.Children.Count.ToString();
            }
        }

        private void UpdateLayoutPanelBackground()
        {
            if (layoutPanel != null && cmbLayoutPanelBackground != null)
            {
                SolidColorBrush bg = layoutPanel.Background as SolidColorBrush;

                if (bg == null)
                {
                    cmbLayoutPanelBackground.SelectedIndex = 0;
                }
                else if (bg.Color == Colors.Transparent)
                {
                    cmbLayoutPanelBackground.SelectedIndex = 1;
                }
                else if (bg.Color == Colors.AliceBlue)
                {
                    cmbLayoutPanelBackground.SelectedIndex = 2;
                }
                else if (bg.Color == Colors.Aqua)
                {
                    cmbLayoutPanelBackground.SelectedIndex = 3;
                }
                else
                {
                    cmbLayoutPanelBackground.SelectedIndex = -1;
                }
            }
        }

        private void UpdateLayoutPanelBorderBrush()
        {
            if (layoutPanel != null && cmbLayoutPanelBorderBrush != null)
            {
                SolidColorBrush bb = layoutPanel.BorderBrush as SolidColorBrush;

                if (bb == null)
                {
                    cmbLayoutPanelBorderBrush.SelectedIndex = 0;
                }
                else if (bb.Color == Colors.Transparent)
                {
                    cmbLayoutPanelBorderBrush.SelectedIndex = 1;
                }
                else if (bb.Color == Colors.Blue)
                {
                    cmbLayoutPanelBorderBrush.SelectedIndex = 2;
                }
                else if (bb.Color == Colors.Green)
                {
                    cmbLayoutPanelBorderBrush.SelectedIndex = 3;
                }
                else
                {
                    cmbLayoutPanelBorderBrush.SelectedIndex = -1;
                }
            }
        }

        private void UpdateLayoutPanelBorderThickness()
        {
            if (layoutPanel != null && txtLayoutPanelBorderThickness != null)
            {
                txtLayoutPanelBorderThickness.Text = layoutPanel.BorderThickness.ToString();
            }
        }

        private void UpdateLayoutPanelCornerRadius()
        {
            if (layoutPanel != null && txtLayoutPanelCornerRadius != null)
            {
                txtLayoutPanelCornerRadius.Text = layoutPanel.CornerRadius.ToString();
            }
        }

        private void UpdateLayoutPanelElementActualHeight()
        {
            try
            {
                if (layoutPanel != null && txtLayoutPanelElementActualHeight != null)
                {
                    UIElement element = GetLayoutPanelElement();

                    if (element is FrameworkElement frameworkElement)
                    {
                        txtLayoutPanelElementActualHeight.Text = frameworkElement.ActualHeight.ToString();
                    }
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void UpdateLayoutPanelElementActualWidth()
        {
            try
            {
                if (layoutPanel != null && txtLayoutPanelElementActualWidth != null)
                {
                    UIElement element = GetLayoutPanelElement();

                    if (element is FrameworkElement frameworkElement)
                    {
                        txtLayoutPanelElementActualWidth.Text = frameworkElement.ActualWidth.ToString();
                    }
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void UpdateLayoutPanelElementCornerRadius()
        {
            try
            {
                if (layoutPanel != null && txtLayoutPanelElementCornerRadius != null)
                {
                    UIElement element = GetLayoutPanelElement();

                    if (element is Grid grid)
                    {
                        txtLayoutPanelElementCornerRadius.Text = grid.CornerRadius.ToString();
                    }
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void UpdateLayoutPanelElementIsEnabled()
        {
            try
            {
                if (layoutPanel != null && chkLayoutPanelElementIsEnabled != null)
                {
                    UIElement element = GetLayoutPanelElement();

                    if (element is Control control)
                    {
                        chkLayoutPanelElementIsEnabled.IsChecked = control != null && control.IsEnabled;
                    }
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void UpdateLayoutPanelElementMinWidth()
        {
            try
            {
                if (layoutPanel != null && txtLayoutPanelElementMinWidth != null)
                {
                    UIElement element = GetLayoutPanelElement();

                    if (element is FrameworkElement frameworkElement)
                    {
                        txtLayoutPanelElementMinWidth.Text = frameworkElement.MinWidth.ToString();
                    }
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void UpdateLayoutPanelElementMaxWidth()
        {
            try
            {
                if (layoutPanel != null && txtLayoutPanelElementMaxWidth != null)
                {
                    UIElement element = GetLayoutPanelElement();

                    if (element is FrameworkElement frameworkElement)
                    {
                        txtLayoutPanelElementMaxWidth.Text = frameworkElement.MaxWidth.ToString();
                    }
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void UpdateLayoutPanelElementHeight()
        {
            try
            {
                if (layoutPanel != null && txtLayoutPanelElementHeight != null)
                {
                    UIElement element = GetLayoutPanelElement();

                    if (element is FrameworkElement frameworkElement)
                    {
                        txtLayoutPanelElementHeight.Text = frameworkElement.Height.ToString();
                    }
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void UpdateLayoutPanelElementMaxHeight()
        {
            try
            {
                if (layoutPanel != null && txtLayoutPanelElementMaxHeight != null)
                {
                    UIElement element = GetLayoutPanelElement();

                    if (element is FrameworkElement frameworkElement)
                    {
                        txtLayoutPanelElementMaxHeight.Text = frameworkElement.MaxHeight.ToString();
                    }
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void UpdateLayoutPanelElementMinHeight()
        {
            try
            {
                if (layoutPanel != null && txtLayoutPanelElementMinHeight != null)
                {
                    UIElement element = GetLayoutPanelElement();

                    if (element is FrameworkElement frameworkElement)
                    {
                        txtLayoutPanelElementMinHeight.Text = frameworkElement.MinHeight.ToString();
                    }
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void UpdateLayoutPanelElementWidth()
        {
            try
            {
                if (layoutPanel != null && txtLayoutPanelElementWidth != null)
                {
                    UIElement element = GetLayoutPanelElement();

                    if (element is FrameworkElement frameworkElement)
                    {
                        txtLayoutPanelElementWidth.Text = frameworkElement.Width.ToString();
                    }
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void UpdateLayoutPanelHeight()
        {
            if (layoutPanel != null && txtLayoutPanelHeight != null)
            {
                txtLayoutPanelHeight.Text = layoutPanel.Height.ToString();
            }
        }

        private void UpdateLayoutPanelMargin()
        {
            if (layoutPanel != null && txtLayoutPanelMargin != null)
            {
                txtLayoutPanelMargin.Text = layoutPanel.Margin.ToString();
            }
        }

        private void UpdateLayoutPanelMaxHeight()
        {
            if (layoutPanel != null && txtLayoutPanelMaxHeight != null)
            {
                txtLayoutPanelMaxHeight.Text = layoutPanel.MaxHeight.ToString();
            }
        }

        private void UpdateLayoutPanelMaxWidth()
        {
            if (layoutPanel != null && txtLayoutPanelMaxWidth != null)
            {
                txtLayoutPanelMaxWidth.Text = layoutPanel.MaxWidth.ToString();
            }
        }

        private void UpdateLayoutPanelPadding()
        {
            if (layoutPanel != null && txtLayoutPanelPadding != null)
            {
                txtLayoutPanelPadding.Text = layoutPanel.Padding.ToString();
            }
        }

        private void UpdateLayoutPanelHorizontalAlignment()
        {
            try
            {
                if (layoutPanel != null && cmbLayoutPanelHorizontalAlignment != null)
                {
                    cmbLayoutPanelHorizontalAlignment.SelectedIndex = (int)layoutPanel.HorizontalAlignment;
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void UpdateLayoutPanelVerticalAlignment()
        {
            try
            {
                if (layoutPanel != null && cmbLayoutPanelVerticalAlignment != null)
                {
                    cmbLayoutPanelVerticalAlignment.SelectedIndex = (int)layoutPanel.VerticalAlignment;
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void UpdateLayoutPanelWidth()
        {
            if (layoutPanel != null && txtLayoutPanelWidth != null)
            {
                txtLayoutPanelWidth.Text = layoutPanel.Width.ToString();
            }
        }

        private void UpdateLinedFlowLayoutActualLineHeight()
        {
            if (linedFlowLayout != null)
            {
                txtLinedFlowLayoutActualLineHeight.Text = linedFlowLayout.ActualLineHeight.ToString();
            }
        }

        private void UpdateLinedFlowLayoutAverageItemAspectRatioDbg()
        {
            if (linedFlowLayout != null)
            {
                txtLinedFlowLayoutAverageItemAspectRatioDbg.Text = LayoutsTestHooks.GetLinedFlowLayoutAverageItemAspectRatio(linedFlowLayout).ToString();
            }
        }

        private void UpdateLinedFlowLayoutForcedAverageItemAspectRatioDbg()
        {
            if (linedFlowLayout != null)
            {
                txtLinedFlowLayoutForcedAverageItemAspectRatioDbg.Text = LayoutsTestHooks.GetLinedFlowLayoutForcedAverageItemAspectRatio(linedFlowLayout).ToString();
            }
        }

        private void UpdateLinedFlowLayoutForcedAverageItemsPerLineDividerDbg()
        {
            if (linedFlowLayout != null)
            {
                txtLinedFlowLayoutForcedAverageItemsPerLineDividerDbg.Text = LayoutsTestHooks.GetLinedFlowLayoutForcedAverageItemsPerLineDivider(linedFlowLayout).ToString();
            }
        }

        private void UpdateLinedFlowLayoutForcedWrapMultiplierDbg()
        {
            if (linedFlowLayout != null)
            {
                txtLinedFlowLayoutForcedWrapMultiplierDbg.Text = LayoutsTestHooks.GetLinedFlowLayoutForcedWrapMultiplier(linedFlowLayout).ToString();
            }
        }

        private void UpdateLinedFlowLayoutFrozenItemIndexes()
        {
            if (linedFlowLayout != null)
            {
                txtLinedFlowLayoutFirstFrozenItemIndexDbg.Text = LayoutsTestHooks.GetLinedFlowLayoutFirstFrozenItemIndex(linedFlowLayout).ToString();
                txtLinedFlowLayoutLastFrozenItemIndexDbg.Text = LayoutsTestHooks.GetLinedFlowLayoutLastFrozenItemIndex(linedFlowLayout).ToString();
            }
        }

        private void UpdateLinedFlowLayoutLineHeight()
        {
            if (linedFlowLayout != null)
            {
                txtLinedFlowLayoutLineHeight.Text = linedFlowLayout.LineHeight.ToString();
            }
        }

        private void UpdateLinedFlowLayoutLineSpacing()
        {
            if (linedFlowLayout != null)
            {
                txtLinedFlowLayoutLineSpacing.Text = linedFlowLayout.LineSpacing.ToString();
            }
        }

        private void UpdateLinedFlowLayoutLockedItemsVisuals()
        {
            if (canvasLinedFlowLayoutLockedItems != null && layoutPanel != null)
            {
                bool showLockedItems = (bool)chkLinedFlowLayoutShowLockedItems.IsChecked;

                canvasLinedFlowLayoutLockedItems.Visibility = showLockedItems ? Visibility.Visible : Visibility.Collapsed;
                canvasLinedFlowLayoutLockedItems.Width = scrollView.ActualWidth;
                canvasLinedFlowLayoutLockedItems.Height = scrollView.ActualHeight;
                canvasLinedFlowLayoutLockedItems.Children.Clear();

                if (showLockedItems && _lstLinedFlowLayoutLockedItemIndexes.Count > 0)
                {
                    foreach (int lockedItemIndex in _lstLinedFlowLayoutLockedItemIndexes)
                    {
                        UIElement lockedItem = layoutPanel.Children[lockedItemIndex];
                        if (lockedItem != null)
                        {
                            GeneralTransform gt = lockedItem.TransformToVisual(layoutPanel);
                            Point lockedItemOriginPoint = new Point();
                            lockedItemOriginPoint = gt.TransformPoint(lockedItemOriginPoint);

                            if (lockedItemOriginPoint.X - scrollView.HorizontalOffset >= 0 && lockedItemOriginPoint.Y - scrollView.VerticalOffset >= 0 &&
                                lockedItemOriginPoint.X - scrollView.HorizontalOffset + 8 <= canvasLinedFlowLayoutLockedItems.Width &&
                                lockedItemOriginPoint.Y - scrollView.VerticalOffset + 8 <= canvasLinedFlowLayoutLockedItems.Height)
                            {
                                Microsoft.UI.Xaml.Shapes.Rectangle lockedItemRectangle = new Microsoft.UI.Xaml.Shapes.Rectangle()
                                {
                                    Fill = new SolidColorBrush(Colors.Red),
                                    Width = 8,
                                    Height = 8
                                };

                                canvasLinedFlowLayoutLockedItems.Children.Add(lockedItemRectangle);
                                Canvas.SetLeft(lockedItemRectangle, lockedItemOriginPoint.X - scrollView.HorizontalOffset);
                                Canvas.SetTop(lockedItemRectangle, lockedItemOriginPoint.Y - scrollView.VerticalOffset);
                            }
                        }
                    }
                }
            }
        }

        private void UpdateLinedFlowLayoutLogItemIndexDbg()
        {
            if (linedFlowLayout != null)
            {
                txtLinedFlowLayoutLogItemIndexDbg.Text = LayoutsTestHooks.GetLinedFlowLayoutLogItemIndex(linedFlowLayout).ToString();
            }
        }

        private void UpdateLinedFlowLayoutItemsJustification()
        {
            if (linedFlowLayout != null)
            {
                cmbLinedFlowLayoutItemsJustification.SelectedIndex = (int)linedFlowLayout.ItemsJustification;
            }
        }

        private void UpdateLinedFlowLayoutItemsStretch()
        {
            if (linedFlowLayout != null)
            {
                cmbLinedFlowLayoutItemsStretch.SelectedIndex = (int)linedFlowLayout.ItemsStretch;
            }
        }

        private void UpdateLinedFlowLayoutMinItemSpacing()
        {
            if (linedFlowLayout != null)
            {
                txtLinedFlowLayoutMinItemSpacing.Text = linedFlowLayout.MinItemSpacing.ToString();
            }
        }

        private void UpdateLinedFlowLayoutRealizedItemIndexes()
        {
            if (linedFlowLayout != null)
            {
                txtLinedFlowLayoutFirstRealizedItemIndexDbg.Text = LayoutsTestHooks.GetLayoutFirstRealizedItemIndex(linedFlowLayout).ToString();
                txtLinedFlowLayoutLastRealizedItemIndexDbg.Text = LayoutsTestHooks.GetLayoutLastRealizedItemIndex(linedFlowLayout).ToString();
            }
        }

        private void UpdateLinedFlowLayoutSnappedAverageItemsPerLineDbg()
        {
            if (linedFlowLayout != null)
            {
                txtLinedFlowLayoutSnappedAverageItemsPerLineDbg.Text = LayoutsTestHooks.GetLinedFlowLayoutSnappedAverageItemsPerLine(linedFlowLayout).ToString();
            }
        }

        private void UpdateScrollViewBackground()
        {
            if (scrollView != null && cmbScrollViewBackground != null)
            {
                SolidColorBrush bg = scrollView.Background as SolidColorBrush;

                if (bg == null)
                {
                    cmbScrollViewBackground.SelectedIndex = 0;
                }
                else if (bg.Color == Colors.Transparent)
                {
                    cmbScrollViewBackground.SelectedIndex = 1;
                }
                else if (bg.Color == Colors.AliceBlue)
                {
                    cmbScrollViewBackground.SelectedIndex = 2;
                }
                else if (bg.Color == Colors.Aqua)
                {
                    cmbScrollViewBackground.SelectedIndex = 3;
                }
                else
                {
                    cmbScrollViewBackground.SelectedIndex = -1;
                }
            }
        }

        private void UpdateScrollViewBorderBrush()
        {
            if (scrollView != null && cmbScrollViewBorderBrush != null)
            {
                SolidColorBrush bb = scrollView.BorderBrush as SolidColorBrush;

                if (bb == null)
                {
                    cmbScrollViewBorderBrush.SelectedIndex = 0;
                }
                else if (bb.Color == Colors.Transparent)
                {
                    cmbScrollViewBorderBrush.SelectedIndex = 1;
                }
                else if (bb.Color == Colors.Blue)
                {
                    cmbScrollViewBorderBrush.SelectedIndex = 2;
                }
                else if (bb.Color == Colors.Green)
                {
                    cmbScrollViewBorderBrush.SelectedIndex = 3;
                }
                else
                {
                    cmbScrollViewBorderBrush.SelectedIndex = -1;
                }
            }
        }

        private void UpdateScrollViewBorderThickness()
        {
            if (scrollView != null && txtScrollViewBorderThickness != null)
            {
                txtScrollViewBorderThickness.Text = scrollView.BorderThickness.ToString();
            }
        }

        private void UpdateScrollViewContentOrientation()
        {
            try
            {
                if (scrollView != null && cmbScrollViewContentOrientation != null)
                {
                    cmbScrollViewContentOrientation.SelectedIndex = (int)scrollView.ContentOrientation;
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void UpdateScrollViewCornerRadius()
        {
            if (scrollView != null && txtScrollViewCornerRadius != null)
            {
                txtScrollViewCornerRadius.Text = scrollView.CornerRadius.ToString();
            }
        }

        private void UpdateScrollViewExtentHeight()
        {
            if (scrollView != null && txtScrollViewExtentHeight != null)
            {
                txtScrollViewExtentHeight.Text = scrollView.ExtentHeight.ToString();
            }
        }

        private void UpdateScrollViewExtentWidth()
        {
            if (scrollView != null && txtScrollViewExtentWidth != null)
            {
                txtScrollViewExtentWidth.Text = scrollView.ExtentWidth.ToString();
            }
        }

        private void UpdateScrollViewHeight()
        {
            if (scrollView != null && txtScrollViewHeight != null)
            {
                txtScrollViewHeight.Text = scrollView.Height.ToString();
            }
        }

        private void UpdateScrollViewHorizontalAnchorRatio()
        {
            if (scrollView != null && txtScrollViewHorizontalAnchorRatio != null)
            {
                txtScrollViewHorizontalAnchorRatio.Text = scrollView.HorizontalAnchorRatio.ToString();
            }
        }

        private void UpdateScrollViewHorizontalOffset()
        {
            if (scrollView != null && txtScrollViewHorizontalOffset != null)
            {
                txtScrollViewHorizontalOffset.Text = scrollView.HorizontalOffset.ToString();
            }
        }

        private void UpdateScrollViewHorizontalScrollBarVisibility()
        {
            try
            {
                if (scrollView != null && cmbScrollViewHorizontalScrollBarVisibility != null)
                {
                    cmbScrollViewHorizontalScrollBarVisibility.SelectedIndex = (int)scrollView.HorizontalScrollBarVisibility;
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void UpdateScrollViewHorizontalScrollMode()
        {
            try
            {
                if (scrollView != null && cmbScrollViewHorizontalScrollMode != null)
                {
                    cmbScrollViewHorizontalScrollMode.SelectedIndex = (int)scrollView.HorizontalScrollMode;
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void UpdateScrollViewIsEnabled()
        {
            try
            {
                if (scrollView != null && chkScrollViewIsEnabled != null)
                {
                    chkScrollViewIsEnabled.IsChecked = scrollView.IsEnabled;
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void UpdateScrollViewIsTabStop()
        {
            try
            {
                if (scrollView != null && chkScrollViewIsTabStop != null)
                {
                    chkScrollViewIsTabStop.IsChecked = scrollView.IsTabStop;
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void UpdateScrollViewMargin()
        {
            if (scrollView != null && txtScrollViewMargin != null)
            {
                txtScrollViewMargin.Text = scrollView.Margin.ToString();
            }
        }

        private void UpdateScrollViewMaxHeight()
        {
            if (scrollView != null && txtScrollViewMaxHeight != null)
            {
                txtScrollViewMaxHeight.Text = scrollView.MaxHeight.ToString();
            }
        }

        private void UpdateScrollViewMaxWidth()
        {
            if (scrollView != null && txtScrollViewMaxWidth != null)
            {
                txtScrollViewMaxWidth.Text = scrollView.MaxWidth.ToString();
            }
        }

        private void UpdateScrollViewPadding()
        {
            if (scrollView != null && txtScrollViewPadding != null)
            {
                txtScrollViewPadding.Text = scrollView.Padding.ToString();
            }
        }

        private void UpdateScrollViewVerticalAnchorRatio()
        {
            if (scrollView != null && txtScrollViewVerticalAnchorRatio != null)
            {
                txtScrollViewVerticalAnchorRatio.Text = scrollView.VerticalAnchorRatio.ToString();
            }
        }

        private void UpdateScrollViewVerticalOffset()
        {
            if (scrollView != null && txtScrollViewVerticalOffset != null)
            {
                txtScrollViewVerticalOffset.Text = scrollView.VerticalOffset.ToString();
            }
        }

        private void UpdateScrollViewVerticalScrollBarVisibility()
        {
            try
            {
                if (scrollView != null && cmbScrollViewVerticalScrollBarVisibility != null)
                {
                    cmbScrollViewVerticalScrollBarVisibility.SelectedIndex = (int)scrollView.VerticalScrollBarVisibility;
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void UpdateScrollViewVerticalScrollMode()
        {
            try
            {
                if (scrollView != null && cmbScrollViewVerticalScrollMode != null)
                {
                    cmbScrollViewVerticalScrollMode.SelectedIndex = (int)scrollView.VerticalScrollMode;
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void UpdateScrollViewWidth()
        {
            if (scrollView != null && txtScrollViewWidth != null)
            {
                txtScrollViewWidth.Text = scrollView.Width.ToString();
            }
        }

        private void UpdateScrollViewZoomMode()
        {
            try
            {
                if (scrollView != null && cmbScrollViewZoomMode != null)
                {
                    cmbScrollViewZoomMode.SelectedIndex = (int)scrollView.ZoomMode;
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }
    }
}
