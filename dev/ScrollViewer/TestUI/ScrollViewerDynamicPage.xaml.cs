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

using Scroller = Microsoft.UI.Xaml.Controls.Primitives.Scroller;
using ScrollViewer = Microsoft.UI.Xaml.Controls.ScrollViewer;
using ContentOrientation = Microsoft.UI.Xaml.Controls.ContentOrientation;
using ChainingMode = Microsoft.UI.Xaml.Controls.ChainingMode;
using RailingMode = Microsoft.UI.Xaml.Controls.RailingMode;
using ScrollMode = Microsoft.UI.Xaml.Controls.ScrollMode;
using ZoomMode = Microsoft.UI.Xaml.Controls.ZoomMode;
using InputKind = Microsoft.UI.Xaml.Controls.InputKind;
using ScrollBarVisibility = Microsoft.UI.Xaml.Controls.ScrollBarVisibility;
using ScrollerChangingOffsetsEventArgs = Microsoft.UI.Xaml.Controls.ScrollerChangingOffsetsEventArgs;
using ScrollerChangingZoomFactorEventArgs = Microsoft.UI.Xaml.Controls.ScrollerChangingZoomFactorEventArgs;
using MUXControlsTestHooks = Microsoft.UI.Private.Controls.MUXControlsTestHooks;
using MUXControlsTestHooksLoggingMessageEventArgs = Microsoft.UI.Private.Controls.MUXControlsTestHooksLoggingMessageEventArgs;
using ScrollViewerTestHooks = Microsoft.UI.Private.Controls.ScrollViewerTestHooks;

namespace MUXControlsTestApp
{
    public sealed partial class ScrollViewerDynamicPage : TestPage
    {
        private Object asyncEventReportingLock = new Object();
        private List<string> lstAsyncEventMessage = new List<string>();
        private Image largeImg;
        private Rectangle rectangle = null;
        private Button button = null;
        private Border border = null;
        private StackPanel sp1 = null;
        private StackPanel sp2 = null;
        ScrollViewer scrollViewer = null;

        public ScrollViewerDynamicPage()
        {
            this.InitializeComponent();
            CreateChildren();
            UseScrollViewer(this.markupScrollViewer);
        }

        private void ChkScrollViewerProperties_Checked(object sender, RoutedEventArgs e)
        {
            if (grdScrollViewerProperties != null)
                grdScrollViewerProperties.Visibility = Visibility.Visible;
        }

        private void ChkScrollViewerProperties_Unchecked(object sender, RoutedEventArgs e)
        {
            if (grdScrollViewerProperties != null)
                grdScrollViewerProperties.Visibility = Visibility.Collapsed;
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

        private void BtnGetContentOrientation_Click(object sender, RoutedEventArgs e)
        {
            UpdateContentOrientation();
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

        private void BtnSetContentOrientation_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                ContentOrientation co = (ContentOrientation)cmbContentOrientation.SelectedIndex;
                scrollViewer.ContentOrientation = co;
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
                ScrollMode ssm = (ScrollMode)cmbHorizontalScrollMode.SelectedIndex;
                scrollViewer.HorizontalScrollMode = ssm;
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
                ChainingMode scm = (ChainingMode)cmbHorizontalScrollChainingMode.SelectedIndex;
                scrollViewer.HorizontalScrollChainingMode = scm;
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
                RailingMode srm = (RailingMode)cmbHorizontalScrollRailingMode.SelectedIndex;
                scrollViewer.HorizontalScrollRailingMode = srm;
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
                ScrollMode ssm = (ScrollMode)cmbVerticalScrollMode.SelectedIndex;
                scrollViewer.VerticalScrollMode = ssm;
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
                ChainingMode scm = (ChainingMode)cmbVerticalScrollChainingMode.SelectedIndex;
                scrollViewer.VerticalScrollChainingMode = scm;
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
                RailingMode srm = (RailingMode)cmbVerticalScrollRailingMode.SelectedIndex;
                scrollViewer.VerticalScrollRailingMode = srm;
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
                ZoomMode ssm = (ZoomMode)cmbZoomMode.SelectedIndex;
                scrollViewer.ZoomMode = ssm;
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
                ChainingMode scm = (ChainingMode)cmbZoomChainingMode.SelectedIndex;
                scrollViewer.ZoomChainingMode = scm;
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
                InputKind sik;

                switch (cmbInputKind.SelectedIndex)
                {
                    case 0:
                        sik = InputKind.All;
                        break;
                    case 1:
                        sik = InputKind.Touch;
                        break;
                    case 2:
                        sik = InputKind.Pen;
                        break;
                    case 3:
                        sik = InputKind.MouseWheel;
                        break;
                    case 4:
                        sik = InputKind.Touch | InputKind.MouseWheel;
                        break;
                    case 5:
                        sik = InputKind.Touch | InputKind.Pen;
                        break;
                    case 6:
                        sik = InputKind.Pen | InputKind.MouseWheel;
                        break;
                    default:
                        sik = InputKind.Touch | InputKind.Pen | InputKind.MouseWheel;
                        break;
                }

                scrollViewer.InputKind = sik;
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
                scrollViewer.MinZoomFactor = Convert.ToDouble(txtMinZoomFactor.Text);
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
                scrollViewer.MaxZoomFactor = Convert.ToDouble(txtMaxZoomFactor.Text);
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
                scrollViewer.HorizontalAnchorRatio = Convert.ToDouble(txtHorizontalAnchorRatio.Text);
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
                scrollViewer.VerticalAnchorRatio = Convert.ToDouble(txtVerticalAnchorRatio.Text);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstLogs.Items.Add(ex.ToString());
            }
        }

#if USE_SCROLLMODE_AUTO
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
                tblComputedHorizontalScrollMode.Text = scrollViewer.ComputedHorizontalScrollMode.ToString();
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
                tblComputedVerticalScrollMode.Text = scrollViewer.ComputedVerticalScrollMode.ToString();
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstLogs.Items.Add(ex.ToString());
            }
        }
#endif

        private void UpdateCmbHorizontalScrollBarVisibility()
        {
            try
            {
                cmbHorizontalScrollBarVisibility.SelectedIndex = (int)scrollViewer.HorizontalScrollBarVisibility;
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstLogs.Items.Add(ex.ToString());
            }
        }

        private void UpdateCmbVerticalScrollBarVisibility()
        {
            try
            {
                cmbVerticalScrollBarVisibility.SelectedIndex = (int)scrollViewer.VerticalScrollBarVisibility;
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
                cmbXYFocusKeyboardNavigation.SelectedIndex = (int)scrollViewer.XYFocusKeyboardNavigation;
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstLogs.Items.Add(ex.ToString());
            }
        }

        private void UpdateHorizontalAnchorRatio()
        {
            txtHorizontalAnchorRatio.Text = scrollViewer.HorizontalAnchorRatio.ToString();
        }

        private void UpdateVerticalAnchorRatio()
        {
            txtVerticalAnchorRatio.Text = scrollViewer.VerticalAnchorRatio.ToString();
        }

        private void BtnGetWidth_Click(object sender, RoutedEventArgs e)
        {
            UpdateWidth();
        }

        private void BtnSetWidth_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                scrollViewer.Width = Convert.ToDouble(txtWidth.Text);
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
                scrollViewer.Height = Convert.ToDouble(txtHeight.Text);
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
                    scrollViewer.Background = null;
                    break;
                case 1:
                    scrollViewer.Background = new SolidColorBrush(Colors.Transparent);
                    break;
                case 2:
                    scrollViewer.Background = new SolidColorBrush(Colors.AliceBlue);
                    break;
                case 3:
                    scrollViewer.Background = new SolidColorBrush(Colors.Aqua);
                    break;
            }
        }

        private void CmbContent_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            try
            {
                FrameworkElement currentContent = scrollViewer.Content as FrameworkElement;
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

                scrollViewer.Content = newContent;
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstLogs.Items.Add(ex.ToString());

                UpdateContent();
            }
        }

        private void CmbHorizontalScrollBarVisibility_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            try
            {
                scrollViewer.HorizontalScrollBarVisibility = (ScrollBarVisibility)cmbHorizontalScrollBarVisibility.SelectedIndex;
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstLogs.Items.Add(ex.ToString());
            }
        }

        private void CmbVerticalScrollBarVisibility_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            try
            {
                scrollViewer.VerticalScrollBarVisibility = (ScrollBarVisibility)cmbVerticalScrollBarVisibility.SelectedIndex;
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
                scrollViewer.XYFocusKeyboardNavigation = (XYFocusKeyboardNavigationMode)cmbXYFocusKeyboardNavigation.SelectedIndex;
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
                scrollViewer.Margin = GetThicknessFromString(txtMargin.Text);
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
                scrollViewer.Padding = GetThicknessFromString(txtPadding.Text);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstLogs.Items.Add(ex.ToString());
            }
        }

        private void ChkIsEnabled_Checked(object sender, RoutedEventArgs e)
        {
            scrollViewer.IsEnabled = true;
        }

        private void ChkIsEnabled_Unchecked(object sender, RoutedEventArgs e)
        {
            scrollViewer.IsEnabled = false;
        }

        private void ChkIsTabStop_Checked(object sender, RoutedEventArgs e)
        {
            scrollViewer.IsTabStop = true;
        }

        private void ChkIsTabStop_Unchecked(object sender, RoutedEventArgs e)
        {
            scrollViewer.IsTabStop = false;
        }


        private void BtnGetContentWidth_Click(object sender, RoutedEventArgs e)
        {
            if (scrollViewer.Content == null || !(scrollViewer.Content is FrameworkElement))
                txtContentWidth.Text = string.Empty;
            else
                txtContentWidth.Text = ((FrameworkElement)(scrollViewer.Content)).Width.ToString();
        }

        private void BtnSetContentWidth_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (scrollViewer.Content is FrameworkElement)
                {
                    ((FrameworkElement)(scrollViewer.Content)).Width = Convert.ToDouble(txtContentWidth.Text);
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
            if (scrollViewer.Content == null || !(scrollViewer.Content is FrameworkElement))
                txtContentHeight.Text = string.Empty;
            else
                txtContentHeight.Text = ((FrameworkElement)(scrollViewer.Content)).Height.ToString();
        }

        private void BtnSetContentHeight_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (scrollViewer.Content is FrameworkElement)
                {
                    ((FrameworkElement)(scrollViewer.Content)).Height = Convert.ToDouble(txtContentHeight.Text);
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
            if (scrollViewer.Content == null || !(scrollViewer.Content is FrameworkElement))
                txtContentMargin.Text = string.Empty;
            else
                txtContentMargin.Text = ((FrameworkElement)(scrollViewer.Content)).Margin.ToString();
        }

        private void BtnSetContentMargin_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (scrollViewer.Content is FrameworkElement)
                {
                    ((FrameworkElement)(scrollViewer.Content)).Margin = GetThicknessFromString(txtContentMargin.Text);
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
            if (scrollViewer.Content == null || !(scrollViewer.Content is Control || scrollViewer.Content is Border || scrollViewer.Content is StackPanel))
                txtContentPadding.Text = string.Empty;
            else if (scrollViewer.Content is Control)
                txtContentPadding.Text = ((Control)(scrollViewer.Content)).Padding.ToString();
            else if (scrollViewer.Content is Border)
                txtContentPadding.Text = ((Border)(scrollViewer.Content)).Padding.ToString();
            else
                txtContentPadding.Text = ((StackPanel)(scrollViewer.Content)).Padding.ToString();
        }

        private void BtnSetContentPadding_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (scrollViewer.Content is Control)
                {
                    ((Control)(scrollViewer.Content)).Padding = GetThicknessFromString(txtContentPadding.Text);
                }
                else if (scrollViewer.Content is Border)
                {
                    ((Border)(scrollViewer.Content)).Padding = GetThicknessFromString(txtContentPadding.Text);
                }
                else if (scrollViewer.Content is StackPanel)
                {
                    ((StackPanel)(scrollViewer.Content)).Padding = GetThicknessFromString(txtContentPadding.Text);
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
                if (scrollViewer.Content is FrameworkElement)
                {
                    ((FrameworkElement)scrollViewer.Content).HorizontalAlignment = (HorizontalAlignment)cmbContentHorizontalAlignment.SelectedIndex;
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
                if (scrollViewer.Content is FrameworkElement)
                {
                    ((FrameworkElement)scrollViewer.Content).VerticalAlignment = (VerticalAlignment)cmbContentVerticalAlignment.SelectedIndex;
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
                if (scrollViewer.Content is FrameworkElement)
                {
                    switch (cmbContentManipulationMode.SelectedIndex)
                    {
                        case 0:
                            scrollViewer.Content.ManipulationMode = ManipulationModes.System;
                            break;
                        case 1:
                            scrollViewer.Content.ManipulationMode = ManipulationModes.None;
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
                if (scrollViewer.Content is FrameworkElement)
                {
                    cmbContentHorizontalAlignment.SelectedIndex = (int)((FrameworkElement)scrollViewer.Content).HorizontalAlignment;
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
                if (scrollViewer.Content is FrameworkElement)
                {
                    cmbContentVerticalAlignment.SelectedIndex = (int)((FrameworkElement)scrollViewer.Content).VerticalAlignment;
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
                if (scrollViewer.Content != null)
                {
                    switch (scrollViewer.Content.ManipulationMode)
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

        private void UpdateContentOrientation()
        {
            try
            {
                cmbContentOrientation.SelectedIndex = (int)scrollViewer.ContentOrientation;
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
                cmbHorizontalScrollMode.SelectedIndex = (int)scrollViewer.HorizontalScrollMode;
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
                cmbHorizontalScrollChainingMode.SelectedIndex = (int)scrollViewer.HorizontalScrollChainingMode;
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
                cmbHorizontalScrollRailingMode.SelectedIndex = (int)scrollViewer.HorizontalScrollRailingMode;
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
                cmbVerticalScrollMode.SelectedIndex = (int)scrollViewer.VerticalScrollMode;
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
                cmbVerticalScrollChainingMode.SelectedIndex = (int)scrollViewer.VerticalScrollChainingMode;
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
                cmbVerticalScrollRailingMode.SelectedIndex = (int)scrollViewer.VerticalScrollRailingMode;
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
                cmbZoomMode.SelectedIndex = (int)scrollViewer.ZoomMode;
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
                txtMinZoomFactor.Text = scrollViewer.MinZoomFactor.ToString();
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
                txtMaxZoomFactor.Text = scrollViewer.MaxZoomFactor.ToString();
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
                cmbZoomChainingMode.SelectedIndex = (int)scrollViewer.ZoomChainingMode;
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
                switch (scrollViewer.InputKind)
                {
                    case InputKind.All:
                        cmbInputKind.SelectedIndex = 0;
                        break;
                    case InputKind.Touch:
                        cmbInputKind.SelectedIndex = 1;
                        break;
                    case InputKind.Pen:
                        cmbInputKind.SelectedIndex = 2;
                        break;
                    case InputKind.MouseWheel:
                        cmbInputKind.SelectedIndex = 3;
                        break;
                    case InputKind.Touch | InputKind.MouseWheel:
                        cmbInputKind.SelectedIndex = 4;
                        break;
                    case InputKind.Touch | InputKind.Pen:
                        cmbInputKind.SelectedIndex = 5;
                        break;
                    case InputKind.Pen | InputKind.MouseWheel:
                        cmbInputKind.SelectedIndex = 6;
                        break;
                    case InputKind.Touch | InputKind.Pen | InputKind.MouseWheel:
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
            txtWidth.Text = scrollViewer.Width.ToString();
        }

        private void UpdateHeight()
        {
            txtHeight.Text = scrollViewer.Height.ToString();
        }

        private void UpdateBackground()
        {
            if (scrollViewer.Background == null)
            {
                cmbBackground.SelectedIndex = 0;
            }
            else if ((scrollViewer.Background as SolidColorBrush).Color == Colors.Transparent)
            {
                cmbBackground.SelectedIndex = 1;
            }
            else if ((scrollViewer.Background as SolidColorBrush).Color == Colors.AliceBlue)
            {
                cmbBackground.SelectedIndex = 2;
            }
            else if ((scrollViewer.Background as SolidColorBrush).Color == Colors.Aqua)
            {
                cmbBackground.SelectedIndex = 3;
            }
        }

        private void UpdateContent()
        {
            if (scrollViewer.Content == null)
            {
                cmbContent.SelectedIndex = 0;
            }
            else if (scrollViewer.Content is Image)
            {
                if (((scrollViewer.Content as Image).Source as BitmapImage).UriSource.AbsolutePath.ToLower().Contains("large"))
                {
                    cmbContent.SelectedIndex = 2;
                }
                else
                {
                    cmbContent.SelectedIndex = 1;
                }
            }
            else if (scrollViewer.Content is Rectangle)
            {
                cmbContent.SelectedIndex = 3;
            }
            else if (scrollViewer.Content is Button)
            {
                cmbContent.SelectedIndex = 4;
            }
            else if (scrollViewer.Content is Border)
            {
                cmbContent.SelectedIndex = 5;
            }
        }

        private void UpdateMargin()
        {
            txtMargin.Text = scrollViewer.Margin.ToString();
        }

        private void UpdatePadding()
        {
            txtPadding.Text = scrollViewer.Padding.ToString();
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

        private void UseScrollViewer(ScrollViewer sv2)
        {
            if (scrollViewer == sv2 || sv2 == null)
                return;

            try
            {
                if (scrollViewer == null && (chkLogScrollViewerMessages.IsChecked == true || chkLogScrollerMessages.IsChecked == true))
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

                if (scrollViewer != null)
                {
                    scrollViewer.GettingFocus -= ScrollViewer_GettingFocus;
                    scrollViewer.GotFocus -= ScrollViewer_GotFocus;
                    scrollViewer.LosingFocus -= ScrollViewer_LosingFocus;
                    scrollViewer.LostFocus -= ScrollViewer_LostFocus;

                    if (chkLogScrollViewerEvents.IsChecked == true)
                    {
                        scrollViewer.ExtentChanged -= ScrollViewer_ExtentChanged;
                        scrollViewer.StateChanged -= ScrollViewer_StateChanged;
                        scrollViewer.ViewChanged -= ScrollViewer_ViewChanged;
                        scrollViewer.ChangingOffsets -= ScrollViewer_ChangingOffsets;
                        scrollViewer.ChangingZoomFactor -= ScrollViewer_ChangingZoomFactor;
                    }

                    Scroller scroller = ScrollViewerTestHooks.GetScrollerPart(scrollViewer);

                    if (scroller != null && chkLogScrollerEvents.IsChecked == true)
                    {
                        scroller.ExtentChanged -= Scroller_ExtentChanged;
                        scroller.StateChanged -= Scroller_StateChanged;
                        scroller.ViewChanged -= Scroller_ViewChanged;
                        scroller.ChangingOffsets -= Scroller_ChangingOffsets;
                        scroller.ChangingZoomFactor -= Scroller_ChangingZoomFactor;
                    }
                }

                scrollViewer = sv2;

                UpdateContentOrientation();
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

                UpdateCmbHorizontalScrollBarVisibility();
                UpdateCmbVerticalScrollBarVisibility();
                UpdateCmbXYFocusKeyboardNavigation();
                UpdateHorizontalAnchorRatio();
                UpdateVerticalAnchorRatio();
#if USE_SCROLLMODE_AUTO
                UpdateTblComputedHorizontalScrollMode();
                UpdateTblComputedVerticalScrollMode();
#endif
                UpdateCmbContentHorizontalAlignment();
                UpdateCmbContentVerticalAlignment();
                UpdateCmbContentManipulationMode();

                chkIsEnabled.IsChecked = scrollViewer.IsEnabled;
                chkIsTabStop.IsChecked = scrollViewer.IsTabStop;

                if (scrollViewer != null)
                {
                    scrollViewer.GettingFocus += ScrollViewer_GettingFocus;
                    scrollViewer.GotFocus += ScrollViewer_GotFocus;
                    scrollViewer.LosingFocus += ScrollViewer_LosingFocus;
                    scrollViewer.LostFocus += ScrollViewer_LostFocus;

                    if (chkLogScrollViewerEvents.IsChecked == true)
                    {
                        scrollViewer.ExtentChanged += ScrollViewer_ExtentChanged;
                        scrollViewer.StateChanged += ScrollViewer_StateChanged;
                        scrollViewer.ViewChanged += ScrollViewer_ViewChanged;
                        scrollViewer.ChangingOffsets += ScrollViewer_ChangingOffsets;
                        scrollViewer.ChangingZoomFactor += ScrollViewer_ChangingZoomFactor;
                    }

                    Scroller scroller = ScrollViewerTestHooks.GetScrollerPart(scrollViewer);

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

        private void ScrollViewer_ChangingOffsets(ScrollViewer sender, ScrollerChangingOffsetsEventArgs args)
        {
            AppendAsyncEventMessage("ScrollViewer.ChangingOffsets ViewChangeId=" + args.ViewChangeId);
        }

        private void ScrollViewer_ChangingZoomFactor(ScrollViewer sender, ScrollerChangingZoomFactorEventArgs args)
        {
            AppendAsyncEventMessage("ScrollViewer.ChangingZoomFactor ViewChangeId=" + args.ViewChangeId + ", CenterPoint=" + args.CenterPoint);
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
            if (scrollViewer != null)
            {
                Scroller scroller = ScrollViewerTestHooks.GetScrollerPart(scrollViewer);

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

        private void ChkLogScrollViewerEvents_Checked(object sender, RoutedEventArgs e)
        {
            if (scrollViewer != null)
            {
                scrollViewer.ExtentChanged += ScrollViewer_ExtentChanged;
                scrollViewer.StateChanged += ScrollViewer_StateChanged;
                scrollViewer.ViewChanged += ScrollViewer_ViewChanged;
                scrollViewer.ChangingOffsets += ScrollViewer_ChangingOffsets;
                scrollViewer.ChangingZoomFactor += ScrollViewer_ChangingZoomFactor;
            }
        }

        private void ChkLogScrollViewerEvents_Unchecked(object sender, RoutedEventArgs e)
        {
            if (scrollViewer != null)
            {
                scrollViewer.ExtentChanged -= ScrollViewer_ExtentChanged;
                scrollViewer.StateChanged -= ScrollViewer_StateChanged;
                scrollViewer.ViewChanged -= ScrollViewer_ViewChanged;
                scrollViewer.ChangingOffsets -= ScrollViewer_ChangingOffsets;
                scrollViewer.ChangingZoomFactor -= ScrollViewer_ChangingZoomFactor;
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
