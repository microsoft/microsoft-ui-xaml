// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using Windows.Foundation;
using Windows.UI;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Media.Imaging;
using Windows.UI.Xaml.Shapes;

#if !BUILD_WINDOWS
using Scroller = Microsoft.UI.Xaml.Controls.Scroller;
using ScrollerView = Microsoft.UI.Xaml.Controls.ScrollerView;
using ScrollerChainingMode = Microsoft.UI.Xaml.Controls.ScrollerChainingMode;
using ScrollerRailingMode = Microsoft.UI.Xaml.Controls.ScrollerRailingMode;
using ScrollerScrollMode = Microsoft.UI.Xaml.Controls.ScrollerScrollMode;
using ScrollerZoomMode = Microsoft.UI.Xaml.Controls.ScrollerZoomMode;
using ScrollerInputKind = Microsoft.UI.Xaml.Controls.ScrollerInputKind;
using ScrollerViewScrollControllerVisibility = Microsoft.UI.Xaml.Controls.ScrollerViewScrollControllerVisibility;
using ScrollerChangingOffsetsEventArgs = Microsoft.UI.Xaml.Controls.ScrollerChangingOffsetsEventArgs;
using ScrollerChangingZoomFactorEventArgs = Microsoft.UI.Xaml.Controls.ScrollerChangingZoomFactorEventArgs;
using MUXControlsTestHooks = Microsoft.UI.Private.Controls.MUXControlsTestHooks;
using MUXControlsTestHooksLoggingMessageEventArgs = Microsoft.UI.Private.Controls.MUXControlsTestHooksLoggingMessageEventArgs;
using ScrollerViewTestHooks = Microsoft.UI.Private.Controls.ScrollerViewTestHooks;
#endif

namespace MUXControlsTestApp
{
    public sealed partial class ScrollerViewDynamicPage : TestPage
    {
        private Object asyncEventReportingLock = new Object();
        private List<string> lstAsyncEventMessage = new List<string>();
        private Image largeImg;
        private Rectangle rectangle = null;
        private Button button = null;
        private Border border = null;
        private StackPanel sp1 = null;
        private StackPanel sp2 = null;
        ScrollerView scrollerView = null;

        public ScrollerViewDynamicPage()
        {
            this.InitializeComponent();
            CreateChildren();
            UseScrollerView(this.markupScrollerView);
        }

        private void ChkScrollerViewProperties_Checked(object sender, RoutedEventArgs e)
        {
            if (grdScrollerViewProperties != null)
                grdScrollerViewProperties.Visibility = Visibility.Visible;
        }

        private void ChkScrollerViewProperties_Unchecked(object sender, RoutedEventArgs e)
        {
            if (grdScrollerViewProperties != null)
                grdScrollerViewProperties.Visibility = Visibility.Collapsed;
        }

        private void ChkScrollerClonedProperties_Checked(object sender, RoutedEventArgs e)
        {
            if (grdScrollerClonedProperties != null)
                grdScrollerClonedProperties.Visibility = Visibility.Visible;
        }

        private void ChkScrollerClonedProperties_Unchecked(object sender, RoutedEventArgs e)
        {
            if (grdScrollerClonedProperties != null)
                grdScrollerClonedProperties.Visibility = Visibility.Collapsed;
        }

        private void ChkContentProperties_Checked(object sender, RoutedEventArgs e)
        {
            if (grdContentProperties != null)
                grdContentProperties.Visibility = Visibility.Visible;
        }

        private void ChkContentProperties_Unchecked(object sender, RoutedEventArgs e)
        {
            if (grdContentProperties != null)
                grdContentProperties.Visibility = Visibility.Collapsed;
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

        private void BtnGetIsChildAvailableWidthConstrained_Click(object sender, RoutedEventArgs e)
        {
            UpdateIsChildAvailableWidthConstrained();
        }

        private void BtnGetIsChildAvailableHeightConstrained_Click(object sender, RoutedEventArgs e)
        {
            UpdateIsChildAvailableHeightConstrained();
        }

        private void BtnGetHorizontalScrollMode_Click(object sender, RoutedEventArgs e)
        {
            UpdateHorizontalScrollMode();
        }

        private void BtnGetHorizontalScrollChainingMode_Click(object sender, RoutedEventArgs e)
        {
            UpdateHorizontalScrollChainingMode();
        }

        private void BtnGetHorizontalScrollRailingMode_Click(object sender, RoutedEventArgs e)
        {
            UpdateHorizontalScrollRailingMode();
        }

        private void BtnGetVerticalScrollMode_Click(object sender, RoutedEventArgs e)
        {
            UpdateVerticalScrollMode();
        }

        private void BtnGetVerticalScrollChainingMode_Click(object sender, RoutedEventArgs e)
        {
            UpdateVerticalScrollChainingMode();
        }

        private void BtnGetVerticalScrollRailingMode_Click(object sender, RoutedEventArgs e)
        {
            UpdateVerticalScrollRailingMode();
        }

        private void BtnGetZoomMode_Click(object sender, RoutedEventArgs e)
        {
            UpdateZoomMode();
        }

        private void BtnGetZoomChainingMode_Click(object sender, RoutedEventArgs e)
        {
            UpdateZoomChainingMode();
        }

        private void BtnGetInputKind_Click(object sender, RoutedEventArgs e)
        {
            UpdateInputKind();
        }

        private void BtnGetMinZoomFactor_Click(object sender, RoutedEventArgs e)
        {
            UpdateMinZoomFactor();
        }

        private void BtnGetMaxZoomFactor_Click(object sender, RoutedEventArgs e)
        {
            UpdateMaxZoomFactor();
        }

        private void BtnSetIsChildAvailableWidthConstrained_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                scrollerView.IsChildAvailableWidthConstrained = cmbIsChildAvailableWidthConstrained.SelectedIndex == 0;
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstLogs.Items.Add(ex.ToString());
            }
        }

        private void BtnSetIsChildAvailableHeightConstrained_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                scrollerView.IsChildAvailableHeightConstrained = cmbIsChildAvailableHeightConstrained.SelectedIndex == 0;
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstLogs.Items.Add(ex.ToString());
            }
        }

        private void BtnSetHorizontalScrollMode_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                ScrollerScrollMode ssm = (ScrollerScrollMode)cmbHorizontalScrollMode.SelectedIndex;
                scrollerView.HorizontalScrollMode = ssm;
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstLogs.Items.Add(ex.ToString());
            }
        }

        private void BtnSetHorizontalScrollChainingMode_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                ScrollerChainingMode scm = (ScrollerChainingMode)cmbHorizontalScrollChainingMode.SelectedIndex;
                scrollerView.HorizontalScrollChainingMode = scm;
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstLogs.Items.Add(ex.ToString());
            }
        }

        private void BtnSetHorizontalScrollRailingMode_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                ScrollerRailingMode srm = (ScrollerRailingMode)cmbHorizontalScrollRailingMode.SelectedIndex;
                scrollerView.HorizontalScrollRailingMode = srm;
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstLogs.Items.Add(ex.ToString());
            }
        }

        private void BtnSetVerticalScrollMode_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                ScrollerScrollMode ssm = (ScrollerScrollMode)cmbVerticalScrollMode.SelectedIndex;
                scrollerView.VerticalScrollMode = ssm;
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstLogs.Items.Add(ex.ToString());
            }
        }

        private void BtnSetVerticalScrollChainingMode_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                ScrollerChainingMode scm = (ScrollerChainingMode)cmbVerticalScrollChainingMode.SelectedIndex;
                scrollerView.VerticalScrollChainingMode = scm;
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstLogs.Items.Add(ex.ToString());
            }
        }

        private void BtnSetVerticalScrollRailingMode_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                ScrollerRailingMode srm = (ScrollerRailingMode)cmbVerticalScrollRailingMode.SelectedIndex;
                scrollerView.VerticalScrollRailingMode = srm;
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstLogs.Items.Add(ex.ToString());
            }
        }

        private void BtnSetZoomMode_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                ScrollerZoomMode ssm = (ScrollerZoomMode)cmbZoomMode.SelectedIndex;
                scrollerView.ZoomMode = ssm;
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstLogs.Items.Add(ex.ToString());
            }
        }

        private void BtnSetZoomChainingMode_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                ScrollerChainingMode scm = (ScrollerChainingMode)cmbZoomChainingMode.SelectedIndex;
                scrollerView.ZoomChainingMode = scm;
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstLogs.Items.Add(ex.ToString());
            }
        }

        private void BtnSetInputKind_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                ScrollerInputKind sik;

                switch (cmbInputKind.SelectedIndex)
                {
                    case 0:
                        sik = ScrollerInputKind.All;
                        break;
                    case 1:
                        sik = ScrollerInputKind.Touch;
                        break;
                    case 2:
                        sik = ScrollerInputKind.Pen;
                        break;
                    case 3:
                        sik = ScrollerInputKind.MouseWheel;
                        break;
                    case 4:
                        sik = ScrollerInputKind.Touch | ScrollerInputKind.MouseWheel;
                        break;
                    case 5:
                        sik = ScrollerInputKind.Touch | ScrollerInputKind.Pen;
                        break;
                    case 6:
                        sik = ScrollerInputKind.Pen | ScrollerInputKind.MouseWheel;
                        break;
                    default:
                        sik = ScrollerInputKind.Touch | ScrollerInputKind.Pen | ScrollerInputKind.MouseWheel;
                        break;
                }

                scrollerView.InputKind = sik;
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstLogs.Items.Add(ex.ToString());
            }
        }

        private void BtnSetMinZoomFactor_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                scrollerView.MinZoomFactor = Convert.ToDouble(txtMinZoomFactor.Text);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstLogs.Items.Add(ex.ToString());
            }
        }

        private void BtnSetMaxZoomFactor_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                scrollerView.MaxZoomFactor = Convert.ToDouble(txtMaxZoomFactor.Text);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstLogs.Items.Add(ex.ToString());
            }
        }

        private void BtnGetIsAnchoredAtHorizontalExtent_Click(object sender, RoutedEventArgs e)
        {
            UpdateCmbIsAnchoredAtHorizontalExtent();
        }

        private void BtnSetIsAnchoredAtHorizontalExtent_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                scrollerView.IsAnchoredAtHorizontalExtent = cmbIsAnchoredAtHorizontalExtent.SelectedIndex == 0;
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstLogs.Items.Add(ex.ToString());
            }
        }

        private void BtnGetIsAnchoredAtVerticalExtent_Click(object sender, RoutedEventArgs e)
        {
            UpdateCmbIsAnchoredAtVerticalExtent();
        }

        private void BtnSetIsAnchoredAtVerticalExtent_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                scrollerView.IsAnchoredAtVerticalExtent = cmbIsAnchoredAtVerticalExtent.SelectedIndex == 0;
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstLogs.Items.Add(ex.ToString());
            }
        }

        private void BtnGetHorizontalAnchorRatio_Click(object sender, RoutedEventArgs e)
        {
            UpdateHorizontalAnchorRatio();
        }

        private void BtnSetHorizontalAnchorRatio_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                scrollerView.HorizontalAnchorRatio = Convert.ToDouble(txtHorizontalAnchorRatio.Text);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstLogs.Items.Add(ex.ToString());
            }
        }

        private void BtnGetVerticalAnchorRatio_Click(object sender, RoutedEventArgs e)
        {
            UpdateVerticalAnchorRatio();
        }

        private void BtnSetVerticalAnchorRatio_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                scrollerView.VerticalAnchorRatio = Convert.ToDouble(txtVerticalAnchorRatio.Text);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstLogs.Items.Add(ex.ToString());
            }
        }

        private void BtnGetComputedHorizontalScrollMode_Click(object sender, RoutedEventArgs e)
        {
            UpdateTblComputedHorizontalScrollMode();
        }

        private void BtnGetComputedVerticalScrollMode_Click(object sender, RoutedEventArgs e)
        {
            UpdateTblComputedVerticalScrollMode();
        }

        private void UpdateTblComputedHorizontalScrollMode()
        { 
            try
            {
                tblComputedHorizontalScrollMode.Text = scrollerView.ComputedHorizontalScrollMode.ToString();
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstLogs.Items.Add(ex.ToString());
            }
        }

        private void UpdateTblComputedVerticalScrollMode()
        {
            try
            {
                tblComputedVerticalScrollMode.Text = scrollerView.ComputedVerticalScrollMode.ToString();
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstLogs.Items.Add(ex.ToString());
            }
        }

        private void UpdateCmbHorizontalScrollControllerVisibility()
        {
            try
            {
                cmbHorizontalScrollControllerVisibility.SelectedIndex = (int)scrollerView.HorizontalScrollControllerVisibility;
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstLogs.Items.Add(ex.ToString());
            }
        }

        private void UpdateCmbVerticalScrollControllerVisibility()
        {
            try
            {
                cmbVerticalScrollControllerVisibility.SelectedIndex = (int)scrollerView.VerticalScrollControllerVisibility;
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstLogs.Items.Add(ex.ToString());
            }
        }

        private void UpdateCmbXYFocusKeyboardNavigation()
        {
            try
            {
                cmbXYFocusKeyboardNavigation.SelectedIndex = (int)scrollerView.XYFocusKeyboardNavigation;
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstLogs.Items.Add(ex.ToString());
            }
        }

        private void UpdateCmbIsAnchoredAtHorizontalExtent()
        {
            try
            {
                cmbIsAnchoredAtHorizontalExtent.SelectedIndex = scrollerView.IsAnchoredAtHorizontalExtent ? 0 : 1;
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstLogs.Items.Add(ex.ToString());
            }
        }

        private void UpdateCmbIsAnchoredAtVerticalExtent()
        {
            try
            {
                cmbIsAnchoredAtVerticalExtent.SelectedIndex = scrollerView.IsAnchoredAtVerticalExtent ? 0 : 1;
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstLogs.Items.Add(ex.ToString());
            }
        }

        private void UpdateHorizontalAnchorRatio()
        {
            txtHorizontalAnchorRatio.Text = scrollerView.HorizontalAnchorRatio.ToString();
        }

        private void UpdateVerticalAnchorRatio()
        {
            txtVerticalAnchorRatio.Text = scrollerView.VerticalAnchorRatio.ToString();
        }

        private void BtnGetWidth_Click(object sender, RoutedEventArgs e)
        {
            UpdateWidth();
        }

        private void BtnSetWidth_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                scrollerView.Width = Convert.ToDouble(txtWidth.Text);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstLogs.Items.Add(ex.ToString());
            }
        }

        private void BtnGetHeight_Click(object sender, RoutedEventArgs e)
        {
            UpdateHeight();
        }

        private void BtnSetHeight_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                scrollerView.Height = Convert.ToDouble(txtHeight.Text);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstLogs.Items.Add(ex.ToString());
            }
        }

        private void CmbBackground_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            switch (cmbBackground.SelectedIndex)
            {
                case 0:
                    scrollerView.Background = null;
                    break;
                case 1:
                    scrollerView.Background = new SolidColorBrush(Colors.Transparent);
                    break;
                case 2:
                    scrollerView.Background = new SolidColorBrush(Colors.AliceBlue);
                    break;
                case 3:
                    scrollerView.Background = new SolidColorBrush(Colors.Aqua);
                    break;
            }
        }

        private void CmbContent_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            try
            {
                FrameworkElement currentContent = scrollerView.Content as FrameworkElement;
                FrameworkElement newContent = null;

                switch (cmbContent.SelectedIndex)
                {
                    case 0:
                        newContent = null;
                        break;
                    case 1:
                        newContent = smallImg;
                        break;
                    case 2:
                        newContent = largeImg;
                        break;
                    case 3:
                        newContent = rectangle;
                        break;
                    case 4:
                        newContent = button;
                        break;
                    case 5:
                        newContent = border;
                        break;
                    case 6:
                        newContent = sp1;
                        break;
                    case 7:
                        newContent = sp2;
                        break;
                }

                if (chkPreserveProperties.IsChecked == true && currentContent != null && newContent != null)
                {
                    newContent.Width = currentContent.Width;
                    newContent.Height = currentContent.Height;
                    newContent.Margin = currentContent.Margin;
                    newContent.HorizontalAlignment = currentContent.HorizontalAlignment;
                    newContent.VerticalAlignment = currentContent.VerticalAlignment;

                    if (currentContent is Control && newContent is Control)
                    {
                        ((Control)newContent).Padding = ((Control)currentContent).Padding;
                    }
                }

                scrollerView.Content = newContent;
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstLogs.Items.Add(ex.ToString());

                UpdateContent();
            }
        }

        private void CmbHorizontalScrollControllerVisibility_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            try
            {
                scrollerView.HorizontalScrollControllerVisibility = (ScrollerViewScrollControllerVisibility)cmbHorizontalScrollControllerVisibility.SelectedIndex;
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstLogs.Items.Add(ex.ToString());
            }
        }

        private void CmbVerticalScrollControllerVisibility_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            try
            {
                scrollerView.VerticalScrollControllerVisibility = (ScrollerViewScrollControllerVisibility)cmbVerticalScrollControllerVisibility.SelectedIndex;
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstLogs.Items.Add(ex.ToString());
            }
        }

        private void CmbXYFocusKeyboardNavigation_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            try
            {
                scrollerView.XYFocusKeyboardNavigation = (XYFocusKeyboardNavigationMode)cmbXYFocusKeyboardNavigation.SelectedIndex;
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstLogs.Items.Add(ex.ToString());
            }
        }

        private void BtnGetMargin_Click(object sender, RoutedEventArgs e)
        {
            UpdateMargin();
        }

        private void BtnSetMargin_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                scrollerView.Margin = GetThicknessFromString(txtMargin.Text);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstLogs.Items.Add(ex.ToString());
            }
        }

        private void BtnGetPadding_Click(object sender, RoutedEventArgs e)
        {
            UpdatePadding();
        }

        private void BtnSetPadding_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                scrollerView.Padding = GetThicknessFromString(txtPadding.Text);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstLogs.Items.Add(ex.ToString());
            }
        }

        private void ChkIsEnabled_Checked(object sender, RoutedEventArgs e)
        {
            scrollerView.IsEnabled = true;
        }

        private void ChkIsEnabled_Unchecked(object sender, RoutedEventArgs e)
        {
            scrollerView.IsEnabled = false;
        }

        private void ChkIsTabStop_Checked(object sender, RoutedEventArgs e)
        {
            scrollerView.IsTabStop = true;
        }

        private void ChkIsTabStop_Unchecked(object sender, RoutedEventArgs e)
        {
            scrollerView.IsTabStop = false;
        }


        private void BtnGetContentWidth_Click(object sender, RoutedEventArgs e)
        {
            if (scrollerView.Content == null || !(scrollerView.Content is FrameworkElement))
                txtContentWidth.Text = string.Empty;
            else
                txtContentWidth.Text = ((FrameworkElement)(scrollerView.Content)).Width.ToString();
        }

        private void BtnSetContentWidth_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (scrollerView.Content is FrameworkElement)
                {
                    ((FrameworkElement)(scrollerView.Content)).Width = Convert.ToDouble(txtContentWidth.Text);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstLogs.Items.Add(ex.ToString());
            }
        }

        private void BtnGetContentHeight_Click(object sender, RoutedEventArgs e)
        {
            if (scrollerView.Content == null || !(scrollerView.Content is FrameworkElement))
                txtContentHeight.Text = string.Empty;
            else
                txtContentHeight.Text = ((FrameworkElement)(scrollerView.Content)).Height.ToString();
        }

        private void BtnSetContentHeight_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (scrollerView.Content is FrameworkElement)
                {
                    ((FrameworkElement)(scrollerView.Content)).Height = Convert.ToDouble(txtContentHeight.Text);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstLogs.Items.Add(ex.ToString());
            }
        }

        private void BtnGetContentMargin_Click(object sender, RoutedEventArgs e)
        {
            if (scrollerView.Content == null || !(scrollerView.Content is FrameworkElement))
                txtContentMargin.Text = string.Empty;
            else
                txtContentMargin.Text = ((FrameworkElement)(scrollerView.Content)).Margin.ToString();
        }

        private void BtnSetContentMargin_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (scrollerView.Content is FrameworkElement)
                {
                    ((FrameworkElement)(scrollerView.Content)).Margin = GetThicknessFromString(txtContentMargin.Text);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstLogs.Items.Add(ex.ToString());
            }
        }

        private void BtnGetContentPadding_Click(object sender, RoutedEventArgs e)
        {
            if (scrollerView.Content == null || !(scrollerView.Content is Control || scrollerView.Content is Border || scrollerView.Content is StackPanel))
                txtContentPadding.Text = string.Empty;
            else if (scrollerView.Content is Control)
                txtContentPadding.Text = ((Control)(scrollerView.Content)).Padding.ToString();
            else if (scrollerView.Content is Border)
                txtContentPadding.Text = ((Border)(scrollerView.Content)).Padding.ToString();
            else
                txtContentPadding.Text = ((StackPanel)(scrollerView.Content)).Padding.ToString();
        }

        private void BtnSetContentPadding_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (scrollerView.Content is Control)
                {
                    ((Control)(scrollerView.Content)).Padding = GetThicknessFromString(txtContentPadding.Text);
                }
                else if (scrollerView.Content is Border)
                {
                    ((Border)(scrollerView.Content)).Padding = GetThicknessFromString(txtContentPadding.Text);
                }
                else if (scrollerView.Content is StackPanel)
                {
                    ((StackPanel)(scrollerView.Content)).Padding = GetThicknessFromString(txtContentPadding.Text);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstLogs.Items.Add(ex.ToString());
            }
        }

        private void CmbContentHorizontalAlignment_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            try
            {
                if (scrollerView.Content is FrameworkElement)
                {
                    ((FrameworkElement)scrollerView.Content).HorizontalAlignment = (HorizontalAlignment)cmbContentHorizontalAlignment.SelectedIndex;
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstLogs.Items.Add(ex.ToString());
            }
        }

        private void CmbContentVerticalAlignment_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            try
            {
                if (scrollerView.Content is FrameworkElement)
                {
                    ((FrameworkElement)scrollerView.Content).VerticalAlignment = (VerticalAlignment)cmbContentVerticalAlignment.SelectedIndex;
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstLogs.Items.Add(ex.ToString());
            }
        }

        private void CmbContentManipulationMode_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            try
            {
                if (scrollerView.Content is FrameworkElement)
                {
                    switch (cmbContentManipulationMode.SelectedIndex)
                    {
                        case 0:
                            scrollerView.Content.ManipulationMode = ManipulationModes.System;
                            break;
                        case 1:
                            scrollerView.Content.ManipulationMode = ManipulationModes.None;
                            break;
                    }
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstLogs.Items.Add(ex.ToString());
            }
        }

        private void UpdateCmbContentHorizontalAlignment()
        {
            try
            {
                if (scrollerView.Content is FrameworkElement)
                {
                    cmbContentHorizontalAlignment.SelectedIndex = (int)((FrameworkElement)scrollerView.Content).HorizontalAlignment;
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstLogs.Items.Add(ex.ToString());
            }
        }

        private void UpdateCmbContentVerticalAlignment()
        {
            try
            {
                if (scrollerView.Content is FrameworkElement)
                {
                    cmbContentVerticalAlignment.SelectedIndex = (int)((FrameworkElement)scrollerView.Content).VerticalAlignment;
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstLogs.Items.Add(ex.ToString());
            }
        }

        private void UpdateCmbContentManipulationMode()
        {
            try
            {
                if (scrollerView.Content != null)
                {
                    switch (scrollerView.Content.ManipulationMode)
                    {
                        case ManipulationModes.System:
                            cmbContentManipulationMode.SelectedIndex = 0;
                            break;
                        case ManipulationModes.None:
                            cmbContentManipulationMode.SelectedIndex = 1;
                            break;
                    }
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstLogs.Items.Add(ex.ToString());
            }
        }

        private void UpdateIsChildAvailableWidthConstrained()
        {
            try
            {
                cmbIsChildAvailableWidthConstrained.SelectedIndex = scrollerView.IsChildAvailableWidthConstrained ? 0 : 1;
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstLogs.Items.Add(ex.ToString());
            }
        }

        private void UpdateIsChildAvailableHeightConstrained()
        {
            try
            {
                cmbIsChildAvailableHeightConstrained.SelectedIndex = scrollerView.IsChildAvailableHeightConstrained ? 0 : 1;
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstLogs.Items.Add(ex.ToString());
            }
        }

        private void UpdateHorizontalScrollMode()
        {
            try
            {
                cmbHorizontalScrollMode.SelectedIndex = (int)scrollerView.HorizontalScrollMode;
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstLogs.Items.Add(ex.ToString());
            }
        }

        private void UpdateHorizontalScrollChainingMode()
        {
            try
            {
                cmbHorizontalScrollChainingMode.SelectedIndex = (int)scrollerView.HorizontalScrollChainingMode;
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstLogs.Items.Add(ex.ToString());
            }
        }

        private void UpdateHorizontalScrollRailingMode()
        {
            try
            {
                cmbHorizontalScrollRailingMode.SelectedIndex = (int)scrollerView.HorizontalScrollRailingMode;
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstLogs.Items.Add(ex.ToString());
            }
        }

        private void UpdateVerticalScrollMode()
        {
            try
            {
                cmbVerticalScrollMode.SelectedIndex = (int)scrollerView.VerticalScrollMode;
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstLogs.Items.Add(ex.ToString());
            }
        }

        private void UpdateVerticalScrollChainingMode()
        {
            try
            {
                cmbVerticalScrollChainingMode.SelectedIndex = (int)scrollerView.VerticalScrollChainingMode;
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstLogs.Items.Add(ex.ToString());
            }
        }

        private void UpdateVerticalScrollRailingMode()
        {
            try
            {
                cmbVerticalScrollRailingMode.SelectedIndex = (int)scrollerView.VerticalScrollRailingMode;
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstLogs.Items.Add(ex.ToString());
            }
        }

        private void UpdateZoomMode()
        {
            try
            {
                cmbZoomMode.SelectedIndex = (int)scrollerView.ZoomMode;
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstLogs.Items.Add(ex.ToString());
            }
        }

        private void UpdateMinZoomFactor()
        {
            try
            {
                txtMinZoomFactor.Text = scrollerView.MinZoomFactor.ToString();
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstLogs.Items.Add(ex.ToString());
            }
        }

        private void UpdateMaxZoomFactor()
        {
            try
            {
                txtMaxZoomFactor.Text = scrollerView.MaxZoomFactor.ToString();
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstLogs.Items.Add(ex.ToString());
            }
        }

        private void UpdateZoomChainingMode()
        {
            try
            {
                cmbZoomChainingMode.SelectedIndex = (int)scrollerView.ZoomChainingMode;
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstLogs.Items.Add(ex.ToString());
            }
        }

        private void UpdateInputKind()
        {
            try
            {
                switch (scrollerView.InputKind)
                {
                    case ScrollerInputKind.All:
                        cmbInputKind.SelectedIndex = 0;
                        break;
                    case ScrollerInputKind.Touch:
                        cmbInputKind.SelectedIndex = 1;
                        break;
                    case ScrollerInputKind.Pen:
                        cmbInputKind.SelectedIndex = 2;
                        break;
                    case ScrollerInputKind.MouseWheel:
                        cmbInputKind.SelectedIndex = 3;
                        break;
                    case ScrollerInputKind.Touch | ScrollerInputKind.MouseWheel:
                        cmbInputKind.SelectedIndex = 4;
                        break;
                    case ScrollerInputKind.Touch | ScrollerInputKind.Pen:
                        cmbInputKind.SelectedIndex = 5;
                        break;
                    case ScrollerInputKind.Pen | ScrollerInputKind.MouseWheel:
                        cmbInputKind.SelectedIndex = 6;
                        break;
                    case ScrollerInputKind.Touch | ScrollerInputKind.Pen | ScrollerInputKind.MouseWheel:
                        cmbInputKind.SelectedIndex = 7;
                        break;
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstLogs.Items.Add(ex.ToString());
            }
        }

        private void UpdateWidth()
        {
            txtWidth.Text = scrollerView.Width.ToString();
        }

        private void UpdateHeight()
        {
            txtHeight.Text = scrollerView.Height.ToString();
        }

        private void UpdateBackground()
        {
            if (scrollerView.Background == null)
            {
                cmbBackground.SelectedIndex = 0;
            }
            else if ((scrollerView.Background as SolidColorBrush).Color == Colors.Transparent)
            {
                cmbBackground.SelectedIndex = 1;
            }
            else if ((scrollerView.Background as SolidColorBrush).Color == Colors.AliceBlue)
            {
                cmbBackground.SelectedIndex = 2;
            }
            else if ((scrollerView.Background as SolidColorBrush).Color == Colors.Aqua)
            {
                cmbBackground.SelectedIndex = 3;
            }
        }

        private void UpdateContent()
        {
            if (scrollerView.Content == null)
            {
                cmbContent.SelectedIndex = 0;
            }
            else if (scrollerView.Content is Image)
            {
                if (((scrollerView.Content as Image).Source as BitmapImage).UriSource.AbsolutePath.ToLower().Contains("large"))
                {
                    cmbContent.SelectedIndex = 2;
                }
                else
                {
                    cmbContent.SelectedIndex = 1;
                }
            }
            else if (scrollerView.Content is Rectangle)
            {
                cmbContent.SelectedIndex = 3;
            }
            else if (scrollerView.Content is Button)
            {
                cmbContent.SelectedIndex = 4;
            }
            else if (scrollerView.Content is Border)
            {
                cmbContent.SelectedIndex = 5;
            }
        }

        private void UpdateMargin()
        {
            txtMargin.Text = scrollerView.Margin.ToString();
        }

        private void UpdatePadding()
        {
            txtPadding.Text = scrollerView.Padding.ToString();
        }

        private void CreateChildren()
        {
            largeImg = new Image() { Source = new BitmapImage(new Uri("ms-appx:/Assets/LargeWisteria.jpg")) };
            LinearGradientBrush lgb = new LinearGradientBrush() { StartPoint = new Point(0, 0), EndPoint = new Point(1, 1) };
            GradientStop gs = new GradientStop() { Color = Colors.Blue, Offset = 0.0 };
            lgb.GradientStops.Add(gs);
            gs = new GradientStop() { Color = Colors.White, Offset = 0.5 };
            lgb.GradientStops.Add(gs);
            gs = new GradientStop() { Color = Colors.Red, Offset = 1.0 };
            lgb.GradientStops.Add(gs);
            rectangle = new Rectangle() { Fill = lgb };
            rectangle.Name = "rect";
            button = new Button() { Content = "Button" };
            button.Name = "btn";
            Rectangle borderChild = new Rectangle() { Fill = lgb };
            border = new Border() { BorderBrush = new SolidColorBrush(Colors.Chartreuse), BorderThickness = new Thickness(5), Child = borderChild };
            border.Name = "bdr";

            sp1 = new StackPanel();
            sp1.Name = "sp1";
            Button button1 = new Button() { Content = "Button1" };
            button1.Name = "btn1";
            button1.Margin = new Thickness(50);
            Button button2 = new Button() { Content = "Button2" };
            button2.Name = "btn2";
            button2.Margin = new Thickness(50);
            sp1.Children.Add(button1);
            sp1.Children.Add(button2);

            sp2 = new StackPanel();
            sp2.Name = "sp2";
            sp2.Children.Add(new Rectangle() { Height = 200, Fill = new SolidColorBrush(Colors.Indigo) });
            sp2.Children.Add(new Rectangle() { Height = 200, Fill = new SolidColorBrush(Colors.Orange) });
            sp2.Children.Add(new Rectangle() { Height = 200, Fill = new SolidColorBrush(Colors.Purple) });
            sp2.Children.Add(new Rectangle() { Height = 200, Fill = new SolidColorBrush(Colors.Goldenrod) });
        }

        private Thickness GetThicknessFromString(string thickness)
        {
            string[] lengths = thickness.Split(',');
            if (lengths.Length < 4)
                return new Thickness(Convert.ToDouble(lengths[0]));
            else
                return new Thickness(
                    Convert.ToDouble(lengths[0]), Convert.ToDouble(lengths[1]), Convert.ToDouble(lengths[2]), Convert.ToDouble(lengths[3]));
        }

        private void UseScrollerView(ScrollerView sv2)
        {
            if (scrollerView == sv2 || sv2 == null)
                return;

            try
            {
                if (scrollerView == null && (chkLogScrollerViewMessages.IsChecked == true || chkLogScrollBar2Messages.IsChecked == true || chkLogScrollerMessages.IsChecked == true))
                {
                    MUXControlsTestHooks.LoggingMessage += MUXControlsTestHooks_LoggingMessage;

                    if (chkLogScrollerMessages.IsChecked == true)
                    {
                        MUXControlsTestHooks.SetLoggingLevelForType("Scroller", isLoggingInfoLevel: true, isLoggingVerboseLevel: true);
                    }
                    if (chkLogScrollBar2Messages.IsChecked == true)
                    {
                        MUXControlsTestHooks.SetLoggingLevelForType("ScrollBar2", isLoggingInfoLevel: true, isLoggingVerboseLevel: true);
                    }
                    if (chkLogScrollerViewMessages.IsChecked == true)
                    {
                        MUXControlsTestHooks.SetLoggingLevelForType("ScrollerView", isLoggingInfoLevel: true, isLoggingVerboseLevel: true);
                    }
                }

                if (scrollerView != null)
                {
                    scrollerView.GettingFocus -= ScrollerView_GettingFocus;
                    scrollerView.GotFocus -= ScrollerView_GotFocus;
                    scrollerView.LosingFocus -= ScrollerView_LosingFocus;
                    scrollerView.LostFocus -= ScrollerView_LostFocus;

                    if (chkLogScrollerViewEvents.IsChecked == true)
                    {
                        scrollerView.ExtentChanged -= ScrollerView_ExtentChanged;
                        scrollerView.StateChanged -= ScrollerView_StateChanged;
                        scrollerView.ViewChanged -= ScrollerView_ViewChanged;
                        scrollerView.ChangingOffsets -= ScrollerView_ChangingOffsets;
                        scrollerView.ChangingZoomFactor -= ScrollerView_ChangingZoomFactor;
                    }

                    Scroller scroller = ScrollerViewTestHooks.GetScrollerPart(scrollerView);

                    if (scroller != null && chkLogScrollerEvents.IsChecked == true)
                    {
                        scroller.ExtentChanged -= Scroller_ExtentChanged;
                        scroller.StateChanged -= Scroller_StateChanged;
                        scroller.ViewChanged -= Scroller_ViewChanged;
                        scroller.ChangingOffsets -= Scroller_ChangingOffsets;
                        scroller.ChangingZoomFactor -= Scroller_ChangingZoomFactor;
                    }
                }

                scrollerView = sv2;

                UpdateIsChildAvailableWidthConstrained();
                UpdateIsChildAvailableHeightConstrained();
                UpdateHorizontalScrollMode();
                UpdateHorizontalScrollChainingMode();
                UpdateHorizontalScrollRailingMode();
                UpdateVerticalScrollMode();
                UpdateVerticalScrollChainingMode();
                UpdateVerticalScrollRailingMode();
                UpdateZoomMode();
                UpdateZoomChainingMode();
                UpdateInputKind();
                UpdateMinZoomFactor();
                UpdateMaxZoomFactor();

                UpdateWidth();
                UpdateHeight();
                UpdateBackground();
                UpdateContent();
                UpdateMargin();
                UpdatePadding();

                UpdateCmbHorizontalScrollControllerVisibility();
                UpdateCmbVerticalScrollControllerVisibility();
                UpdateCmbXYFocusKeyboardNavigation();
                UpdateCmbIsAnchoredAtHorizontalExtent();
                UpdateCmbIsAnchoredAtVerticalExtent();
                UpdateHorizontalAnchorRatio();
                UpdateVerticalAnchorRatio();
                UpdateTblComputedHorizontalScrollMode();
                UpdateTblComputedVerticalScrollMode();

                UpdateCmbContentHorizontalAlignment();
                UpdateCmbContentVerticalAlignment();
                UpdateCmbContentManipulationMode();

                chkIsEnabled.IsChecked = scrollerView.IsEnabled;
                chkIsTabStop.IsChecked = scrollerView.IsTabStop;

                if (scrollerView != null)
                {
                    scrollerView.GettingFocus += ScrollerView_GettingFocus;
                    scrollerView.GotFocus += ScrollerView_GotFocus;
                    scrollerView.LosingFocus += ScrollerView_LosingFocus;
                    scrollerView.LostFocus += ScrollerView_LostFocus;

                    if (chkLogScrollerViewEvents.IsChecked == true)
                    {
                        scrollerView.ExtentChanged += ScrollerView_ExtentChanged;
                        scrollerView.StateChanged += ScrollerView_StateChanged;
                        scrollerView.ViewChanged += ScrollerView_ViewChanged;
                        scrollerView.ChangingOffsets += ScrollerView_ChangingOffsets;
                        scrollerView.ChangingZoomFactor += ScrollerView_ChangingZoomFactor;
                    }

                    Scroller scroller = ScrollerViewTestHooks.GetScrollerPart(scrollerView);

                    if (scroller != null && chkLogScrollerEvents.IsChecked == true)
                    {
                        scroller.ExtentChanged += Scroller_ExtentChanged;
                        scroller.StateChanged += Scroller_StateChanged;
                        scroller.ViewChanged += Scroller_ViewChanged;
                        scroller.ChangingOffsets += Scroller_ChangingOffsets;
                        scroller.ChangingZoomFactor += Scroller_ChangingZoomFactor;
                    }
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstLogs.Items.Add(ex.ToString());
            }
        }

        private void ScrollerView_GettingFocus(UIElement sender, Windows.UI.Xaml.Input.GettingFocusEventArgs args)
        {
            FrameworkElement oldFE = args.OldFocusedElement as FrameworkElement;
            string oldFEName = (oldFE == null) ? "null" : oldFE.Name;
            FrameworkElement newFE = args.NewFocusedElement as FrameworkElement;
            string newFEName = (newFE == null) ? "null" : newFE.Name;

            AppendAsyncEventMessage("ScrollerView.GettingFocus FocusState=" + args.FocusState + ", Direction=" + args.Direction + ", InputDevice=" + args.InputDevice + ", oldFE=" + oldFEName + ", newFE=" + newFEName);
        }

        private void ScrollerView_LostFocus(object sender, RoutedEventArgs e)
        {
            AppendAsyncEventMessage("ScrollerView.LostFocus");
        }

        private void ScrollerView_LosingFocus(UIElement sender, Windows.UI.Xaml.Input.LosingFocusEventArgs args)
        {
            FrameworkElement oldFE = args.OldFocusedElement as FrameworkElement;
            string oldFEName = (oldFE == null) ? "null" : oldFE.Name;
            FrameworkElement newFE = args.NewFocusedElement as FrameworkElement;
            string newFEName = (newFE == null) ? "null" : newFE.Name;

            AppendAsyncEventMessage("ScrollerView.LosingFocus FocusState=" + args.FocusState + ", Direction=" + args.Direction + ", InputDevice=" + args.InputDevice + ", oldFE=" + oldFEName + ", newFE=" + newFEName);
        }

        private void ScrollerView_GotFocus(object sender, RoutedEventArgs e)
        {
            AppendAsyncEventMessage("ScrollerView.GotFocus");
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

        private void Scroller_ChangingOffsets(Scroller sender, ScrollerChangingOffsetsEventArgs args)
        {
            AppendAsyncEventMessage("Scroller.ChangingOffsets ViewChangeId=" + args.ViewChangeId + " SP=(" + args.StartPosition.X + "," + args.StartPosition.Y +") EP=(" + args.EndPosition.X + "," + args.EndPosition.Y +")");
        }

        private void Scroller_ChangingZoomFactor(Scroller sender, ScrollerChangingZoomFactorEventArgs args)
        {
            AppendAsyncEventMessage("Scroller.ChangingZoomFactor ViewChangeId=" + args.ViewChangeId + ", CenterPoint=" + args.CenterPoint + ", SZF=" + args.StartZoomFactor + ", EZF=" + args.EndZoomFactor);
        }

        private void ScrollerView_ExtentChanged(ScrollerView sender, object args)
        {
            AppendAsyncEventMessage("ScrollerView.ExtentChanged ExtentWidth=" + sender.ExtentWidth + ", ExtentHeight=" + sender.ExtentHeight);
        }

        private void ScrollerView_StateChanged(ScrollerView sender, object args)
        {
            AppendAsyncEventMessage("ScrollerView.StateChanged " + sender.State.ToString());
        }

        private void ScrollerView_ViewChanged(ScrollerView sender, object args)
        {
            AppendAsyncEventMessage("ScrollerView.ViewChanged H=" + sender.HorizontalOffset.ToString() + ", V=" + sender.VerticalOffset + ", ZF=" + sender.ZoomFactor);
        }

        private void ScrollerView_ChangingOffsets(ScrollerView sender, ScrollerChangingOffsetsEventArgs args)
        {
            AppendAsyncEventMessage("ScrollerView.ChangingOffsets ViewChangeId=" + args.ViewChangeId);
        }

        private void ScrollerView_ChangingZoomFactor(ScrollerView sender, ScrollerChangingZoomFactorEventArgs args)
        {
            AppendAsyncEventMessage("ScrollerView.ChangingZoomFactor ViewChangeId=" + args.ViewChangeId + ", CenterPoint=" + args.CenterPoint);
        }

        private void BtnClearLogs_Click(object sender, RoutedEventArgs e)
        {
            lstLogs.Items.Clear();
        }

        private void ChkLogScrollerEvents_Checked(object sender, RoutedEventArgs e)
        {
            if (scrollerView != null)
            {
                Scroller scroller = ScrollerViewTestHooks.GetScrollerPart(scrollerView);

                if (scroller != null)
                {
                    scroller.ExtentChanged += Scroller_ExtentChanged;
                    scroller.StateChanged += Scroller_StateChanged;
                    scroller.ViewChanged += Scroller_ViewChanged;
                    scroller.ChangingOffsets += Scroller_ChangingOffsets;
                    scroller.ChangingZoomFactor += Scroller_ChangingZoomFactor;
                }
            }
        }

        private void ChkLogScrollerEvents_Unchecked(object sender, RoutedEventArgs e)
        {
            if (scrollerView != null)
            {
                Scroller scroller = ScrollerViewTestHooks.GetScrollerPart(scrollerView);

                if (scroller != null)
                {
                    scroller.ExtentChanged -= Scroller_ExtentChanged;
                    scroller.StateChanged -= Scroller_StateChanged;
                    scroller.ViewChanged -= Scroller_ViewChanged;
                    scroller.ChangingOffsets -= Scroller_ChangingOffsets;
                    scroller.ChangingZoomFactor -= Scroller_ChangingZoomFactor;
                }
            }
        }

        private void ChkLogScrollerViewEvents_Checked(object sender, RoutedEventArgs e)
        {
            if (scrollerView != null)
            {
                scrollerView.ExtentChanged += ScrollerView_ExtentChanged;
                scrollerView.StateChanged += ScrollerView_StateChanged;
                scrollerView.ViewChanged += ScrollerView_ViewChanged;
                scrollerView.ChangingOffsets += ScrollerView_ChangingOffsets;
                scrollerView.ChangingZoomFactor += ScrollerView_ChangingZoomFactor;
            }
        }

        private void ChkLogScrollerViewEvents_Unchecked(object sender, RoutedEventArgs e)
        {
            if (scrollerView != null)
            {
                scrollerView.ExtentChanged -= ScrollerView_ExtentChanged;
                scrollerView.StateChanged -= ScrollerView_StateChanged;
                scrollerView.ViewChanged -= ScrollerView_ViewChanged;
                scrollerView.ChangingOffsets -= ScrollerView_ChangingOffsets;
                scrollerView.ChangingZoomFactor -= ScrollerView_ChangingZoomFactor;
            }
        }

        private void ChkLogScrollerMessages_Checked(object sender, RoutedEventArgs e)
        {
            MUXControlsTestHooks.SetLoggingLevelForType("Scroller", isLoggingInfoLevel: true, isLoggingVerboseLevel: true);
            if (chkLogScrollBar2Messages.IsChecked == false && chkLogScrollerViewMessages.IsChecked == false)
                MUXControlsTestHooks.LoggingMessage += MUXControlsTestHooks_LoggingMessage;
        }

        private void ChkLogScrollerMessages_Unchecked(object sender, RoutedEventArgs e)
        {
            MUXControlsTestHooks.SetLoggingLevelForType("Scroller", isLoggingInfoLevel: false, isLoggingVerboseLevel: false);
            if (chkLogScrollBar2Messages.IsChecked == false && chkLogScrollerViewMessages.IsChecked == false)
                MUXControlsTestHooks.LoggingMessage -= MUXControlsTestHooks_LoggingMessage;
        }

        private void ChkLogScrollBar2Messages_Checked(object sender, RoutedEventArgs e)
        {
            MUXControlsTestHooks.SetLoggingLevelForType("ScrollBar2", isLoggingInfoLevel: true, isLoggingVerboseLevel: true);
            if (chkLogScrollerMessages.IsChecked == false && chkLogScrollerViewMessages.IsChecked == false)
                MUXControlsTestHooks.LoggingMessage += MUXControlsTestHooks_LoggingMessage;
        }

        private void ChkLogScrollBar2Messages_Unchecked(object sender, RoutedEventArgs e)
        {
            MUXControlsTestHooks.SetLoggingLevelForType("ScrollBar2", isLoggingInfoLevel: false, isLoggingVerboseLevel: false);
            if (chkLogScrollerMessages.IsChecked == false && chkLogScrollerViewMessages.IsChecked == false)
                MUXControlsTestHooks.LoggingMessage -= MUXControlsTestHooks_LoggingMessage;
        }

        private void ChkLogScrollerViewMessages_Checked(object sender, RoutedEventArgs e)
        {
            MUXControlsTestHooks.SetLoggingLevelForType("ScrollerView", isLoggingInfoLevel: true, isLoggingVerboseLevel: true);
            if (chkLogScrollBar2Messages.IsChecked == false && chkLogScrollerMessages.IsChecked == false)
                MUXControlsTestHooks.LoggingMessage += MUXControlsTestHooks_LoggingMessage;
        }

        private void ChkLogScrollerViewMessages_Unchecked(object sender, RoutedEventArgs e)
        {
            MUXControlsTestHooks.SetLoggingLevelForType("ScrollerView", isLoggingInfoLevel: false, isLoggingVerboseLevel: false);
            if (chkLogScrollBar2Messages.IsChecked == false && chkLogScrollerMessages.IsChecked == false)
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
