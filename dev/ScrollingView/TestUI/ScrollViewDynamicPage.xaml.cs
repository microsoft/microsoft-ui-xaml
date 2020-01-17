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

using ScrollingPresenter = Microsoft.UI.Xaml.Controls.Primitives.ScrollingPresenter;
using ScrollingView = Microsoft.UI.Xaml.Controls.ScrollingView;
using ContentOrientation = Microsoft.UI.Xaml.Controls.ContentOrientation;
using ChainingMode = Microsoft.UI.Xaml.Controls.ChainingMode;
using RailingMode = Microsoft.UI.Xaml.Controls.RailingMode;
using ScrollMode = Microsoft.UI.Xaml.Controls.ScrollMode;
using ZoomMode = Microsoft.UI.Xaml.Controls.ZoomMode;
using InputKind = Microsoft.UI.Xaml.Controls.InputKind;
using ScrollBarVisibility = Microsoft.UI.Xaml.Controls.ScrollBarVisibility;
using ScrollingScrollAnimationStartingEventArgs = Microsoft.UI.Xaml.Controls.ScrollingScrollAnimationStartingEventArgs;
using ScrollingZoomAnimationStartingEventArgs = Microsoft.UI.Xaml.Controls.ScrollingZoomAnimationStartingEventArgs;
using MUXControlsTestHooks = Microsoft.UI.Private.Controls.MUXControlsTestHooks;
using MUXControlsTestHooksLoggingMessageEventArgs = Microsoft.UI.Private.Controls.MUXControlsTestHooksLoggingMessageEventArgs;
using ScrollingViewTestHooks = Microsoft.UI.Private.Controls.ScrollingViewTestHooks;

namespace MUXControlsTestApp
{
    public sealed partial class ScrollingViewDynamicPage : TestPage
    {
        private Object asyncEventReportingLock = new Object();
        private List<string> lstAsyncEventMessage = new List<string>();
        private Image largeImg;
        private Image wuxLargeImg;
        private Rectangle rectangle = null;
        private Rectangle wuxRectangle = null;
        private Button button = null;
        private Button wuxButton = null;
        private Border border = null;
        private Border wuxBorder = null;
        private StackPanel sp1 = null;
        private StackPanel wuxSp1 = null;
        private StackPanel sp2 = null;
        private StackPanel wuxSp2 = null;
        private Viewbox viewbox = null;
        private Viewbox wuxViewbox = null;
        ScrollingView scrollingView = null;

        public ScrollingViewDynamicPage()
        {
            this.InitializeComponent();
            CreateChildren();
            UseScrollingView(this.markupScrollingView);
        }

        private void ChkScrollingViewProperties_Checked(object sender, RoutedEventArgs e)
        {
            if (grdScrollingViewProperties != null)
                grdScrollingViewProperties.Visibility = Visibility.Visible;
        }

        private void ChkScrollingViewProperties_Unchecked(object sender, RoutedEventArgs e)
        {
            if (grdScrollingViewProperties != null)
                grdScrollingViewProperties.Visibility = Visibility.Collapsed;
        }

        private void ChkScrollingPresenterClonedProperties_Checked(object sender, RoutedEventArgs e)
        {
            if (grdScrollingPresenterClonedProperties != null)
                grdScrollingPresenterClonedProperties.Visibility = Visibility.Visible;
        }

        private void ChkScrollingPresenterClonedProperties_Unchecked(object sender, RoutedEventArgs e)
        {
            if (grdScrollingPresenterClonedProperties != null)
                grdScrollingPresenterClonedProperties.Visibility = Visibility.Collapsed;
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

        private void BtnGetIgnoredInputKind_Click(object sender, RoutedEventArgs e)
        {
            UpdateIgnoredInputKind();
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
                scrollingView.ContentOrientation = co;

                switch (co)
                {
                    case ContentOrientation.Horizontal:
                        wuxScrollViewer.HorizontalScrollBarVisibility = MuxScrollBarVisibilityToWuxScrollBarVisibility(scrollingView.HorizontalScrollBarVisibility);
                        wuxScrollViewer.VerticalScrollBarVisibility = Windows.UI.Xaml.Controls.ScrollBarVisibility.Disabled;
                        break;
                    case ContentOrientation.Vertical:
                        wuxScrollViewer.HorizontalScrollBarVisibility = Windows.UI.Xaml.Controls.ScrollBarVisibility.Disabled;
                        wuxScrollViewer.VerticalScrollBarVisibility = MuxScrollBarVisibilityToWuxScrollBarVisibility(scrollingView.VerticalScrollBarVisibility);
                        break;
                    case ContentOrientation.None:
                        wuxScrollViewer.HorizontalScrollBarVisibility = MuxScrollBarVisibilityToWuxScrollBarVisibility(scrollingView.HorizontalScrollBarVisibility);
                        wuxScrollViewer.VerticalScrollBarVisibility = MuxScrollBarVisibilityToWuxScrollBarVisibility(scrollingView.VerticalScrollBarVisibility);
                        break;
                    case ContentOrientation.Both:
                        wuxScrollViewer.HorizontalScrollBarVisibility = Windows.UI.Xaml.Controls.ScrollBarVisibility.Disabled;
                        wuxScrollViewer.VerticalScrollBarVisibility = Windows.UI.Xaml.Controls.ScrollBarVisibility.Disabled;
                        break;
                }
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
                scrollingView.HorizontalScrollMode = ssm;

                wuxScrollViewer.HorizontalScrollMode = MuxScrollModeToWuxScrollMode(ssm);
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
                scrollingView.HorizontalScrollChainingMode = scm;
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
                scrollingView.HorizontalScrollRailingMode = srm;

                wuxScrollViewer.IsHorizontalRailEnabled = MuxRailModeToWuxRailMode(srm);
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
                scrollingView.VerticalScrollMode = ssm;

                wuxScrollViewer.VerticalScrollMode = MuxScrollModeToWuxScrollMode(ssm);
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
                scrollingView.VerticalScrollChainingMode = scm;
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
                scrollingView.VerticalScrollRailingMode = srm;

                wuxScrollViewer.IsVerticalRailEnabled = MuxRailModeToWuxRailMode(srm);
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
                ZoomMode szm = (ZoomMode)cmbZoomMode.SelectedIndex;
                scrollingView.ZoomMode = szm;

                wuxScrollViewer.ZoomMode = MuxZoomModeToWuxZoomMode(szm);
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
                scrollingView.ZoomChainingMode = scm;
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstLogs.Items.Add(ex.ToString());
            }
        }

        private void BtnSetIgnoredInputKind_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                InputKind ignoredInputKind;

                switch (cmbIgnoredInputKind.SelectedIndex)
                {
                    case 0:
                        ignoredInputKind = InputKind.None;
                        break;
                    case 1:
                        ignoredInputKind = InputKind.Touch;
                        break;
                    case 2:
                        ignoredInputKind = InputKind.Pen;
                        break;
                    case 3:
                        ignoredInputKind = InputKind.MouseWheel;
                        break;
                    case 4:
                        ignoredInputKind = InputKind.Keyboard;
                        break;
                    case 5:
                        ignoredInputKind = InputKind.Gamepad;
                        break;
                    default:
                        ignoredInputKind = InputKind.All;
                        break;
                }

                scrollingView.IgnoredInputKind = ignoredInputKind;
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
                scrollingView.MinZoomFactor = Convert.ToDouble(txtMinZoomFactor.Text);

                wuxScrollViewer.MinZoomFactor = (float)scrollingView.MinZoomFactor;
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
                scrollingView.MaxZoomFactor = Convert.ToDouble(txtMaxZoomFactor.Text);

                wuxScrollViewer.MaxZoomFactor = (float)scrollingView.MaxZoomFactor;
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
                scrollingView.HorizontalAnchorRatio = Convert.ToDouble(txtHorizontalAnchorRatio.Text);

                wuxScrollViewer.HorizontalAnchorRatio = scrollingView.HorizontalAnchorRatio;
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
                scrollingView.VerticalAnchorRatio = Convert.ToDouble(txtVerticalAnchorRatio.Text);

                wuxScrollViewer.VerticalAnchorRatio = scrollingView.VerticalAnchorRatio;
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstLogs.Items.Add(ex.ToString());
            }
        }

        private void BtnGetExtentWidth_Click(object sender, RoutedEventArgs e)
        {
            UpdateExtentWidth();
        }

        private void BtnGetExtentHeight_Click(object sender, RoutedEventArgs e)
        {
            UpdateExtentHeight();
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
                tblComputedHorizontalScrollMode.Text = scrollingView.ComputedHorizontalScrollMode.ToString();
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
                tblComputedVerticalScrollMode.Text = scrollingView.ComputedVerticalScrollMode.ToString();
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
                cmbHorizontalScrollBarVisibility.SelectedIndex = (int)scrollingView.HorizontalScrollBarVisibility;
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
                cmbVerticalScrollBarVisibility.SelectedIndex = (int)scrollingView.VerticalScrollBarVisibility;
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
                cmbXYFocusKeyboardNavigation.SelectedIndex = (int)scrollingView.XYFocusKeyboardNavigation;
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstLogs.Items.Add(ex.ToString());
            }
        }

        private void UpdateHorizontalAnchorRatio()
        {
            txtHorizontalAnchorRatio.Text = scrollingView.HorizontalAnchorRatio.ToString();
        }

        private void UpdateVerticalAnchorRatio()
        {
            txtVerticalAnchorRatio.Text = scrollingView.VerticalAnchorRatio.ToString();
        }

        private void UpdateExtentWidth()
        {
            txtExtentWidth.Text = scrollingView.ExtentWidth.ToString();

            if (Math.Abs(scrollingView.ExtentWidth - wuxScrollViewer.ExtentWidth / wuxScrollViewer.ZoomFactor) > 0.0001)
            {
                lstLogs.Items.Add($"muxScrollingView.ExtentWidth={scrollingView.ExtentWidth} != wuxScrollViewer.ExtentWidth/wuxScrollViewer.ZoomFactor={wuxScrollViewer.ExtentWidth / wuxScrollViewer.ZoomFactor}");
            }
        }

        private void UpdateExtentHeight()
        {
            txtExtentHeight.Text = scrollingView.ExtentHeight.ToString();

            if (Math.Abs(scrollingView.ExtentHeight - wuxScrollViewer.ExtentHeight / wuxScrollViewer.ZoomFactor) > 0.0001)
            {
                lstLogs.Items.Add($"muxScrollingView.ExtentHeight={scrollingView.ExtentHeight} != wuxScrollViewer.ExtentHeight/wuxScrollViewer.ZoomFactor={wuxScrollViewer.ExtentHeight / wuxScrollViewer.ZoomFactor}");
            }
        }

        private void BtnGetWidth_Click(object sender, RoutedEventArgs e)
        {
            UpdateWidth();
        }

        private void BtnSetWidth_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                scrollingView.Width = Convert.ToDouble(txtWidth.Text);

                wuxScrollViewer.Width = scrollingView.Width;
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
                scrollingView.Height = Convert.ToDouble(txtHeight.Text);

                wuxScrollViewer.Height = scrollingView.Height;
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
                    scrollingView.Background = null;
                    break;
                case 1:
                    scrollingView.Background = new SolidColorBrush(Colors.Transparent);
                    break;
                case 2:
                    scrollingView.Background = new SolidColorBrush(Colors.AliceBlue);
                    break;
                case 3:
                    scrollingView.Background = new SolidColorBrush(Colors.Aqua);
                    break;
            }

            wuxScrollViewer.Background = scrollingView.Background;
        }

        private void CmbContent_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            try
            {
                FrameworkElement currentContent = scrollingView.Content as FrameworkElement;
                FrameworkElement newContent = null;
                FrameworkElement wuxNewContent = null;

                switch (cmbContent.SelectedIndex)
                {
                    case 1:
                        newContent = smallImg;
                        wuxNewContent = wuxSmallImg;
                        break;
                    case 2:
                        newContent = largeImg;
                        wuxNewContent = wuxLargeImg;
                        break;
                    case 3:
                        newContent = rectangle;
                        wuxNewContent = wuxRectangle;
                        break;
                    case 4:
                        newContent = button;
                        wuxNewContent = wuxButton;
                        break;
                    case 5:
                        newContent = border;
                        wuxNewContent = wuxBorder;
                        break;
                    case 6:
                        newContent = sp1;
                        wuxNewContent = wuxSp1;
                        break;
                    case 7:
                        newContent = sp2;
                        wuxNewContent = wuxSp2;
                        break;
                    case 8:
                        newContent = viewbox;
                        wuxNewContent = wuxViewbox;
                        break;
                }

                if (chkPreserveProperties.IsChecked == true && currentContent != null && newContent != null && wuxNewContent != null)
                {
                    newContent.MinWidth = currentContent.MinWidth;
                    newContent.Width = currentContent.Width;
                    newContent.MaxWidth = currentContent.MaxWidth;
                    newContent.MinHeight = currentContent.MinHeight;
                    newContent.Height = currentContent.Height;
                    newContent.MaxHeight = currentContent.MaxHeight;
                    newContent.Margin = currentContent.Margin;
                    newContent.HorizontalAlignment = currentContent.HorizontalAlignment;
                    newContent.VerticalAlignment = currentContent.VerticalAlignment;

                    if (currentContent is Control && newContent is Control)
                    {
                        ((Control)newContent).Padding = ((Control)currentContent).Padding;
                    }

                    wuxNewContent.MinWidth = currentContent.MinWidth;
                    wuxNewContent.Width = currentContent.Width;
                    wuxNewContent.MaxWidth = currentContent.MaxWidth;
                    wuxNewContent.MinHeight = currentContent.MinHeight;
                    wuxNewContent.Height = currentContent.Height;
                    wuxNewContent.MaxHeight = currentContent.MaxHeight;
                    wuxNewContent.Margin = currentContent.Margin;
                    wuxNewContent.HorizontalAlignment = currentContent.HorizontalAlignment;
                    wuxNewContent.VerticalAlignment = currentContent.VerticalAlignment;

                    if (currentContent is Control && wuxNewContent is Control)
                    {
                        ((Control)wuxNewContent).Padding = ((Control)currentContent).Padding;
                    }
                }

                scrollingView.Content = newContent;

                wuxScrollViewer.Content = wuxNewContent;
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
                scrollingView.HorizontalScrollBarVisibility = (ScrollBarVisibility)cmbHorizontalScrollBarVisibility.SelectedIndex;

                ContentOrientation co = (ContentOrientation)cmbContentOrientation.SelectedIndex;
                switch (co)
                {
                    case ContentOrientation.Vertical:
                    case ContentOrientation.Both:
                        wuxScrollViewer.HorizontalScrollBarVisibility = Windows.UI.Xaml.Controls.ScrollBarVisibility.Disabled;
                        break;
                    default:
                        wuxScrollViewer.HorizontalScrollBarVisibility = MuxScrollBarVisibilityToWuxScrollBarVisibility(scrollingView.HorizontalScrollBarVisibility);
                        break;
                }
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
                scrollingView.VerticalScrollBarVisibility = (ScrollBarVisibility)cmbVerticalScrollBarVisibility.SelectedIndex;

                ContentOrientation co = (ContentOrientation)cmbContentOrientation.SelectedIndex;
                switch (co)
                {
                    case ContentOrientation.Horizontal:
                    case ContentOrientation.Both:
                        wuxScrollViewer.VerticalScrollBarVisibility = Windows.UI.Xaml.Controls.ScrollBarVisibility.Disabled;
                        break;
                    default:
                        wuxScrollViewer.VerticalScrollBarVisibility = MuxScrollBarVisibilityToWuxScrollBarVisibility(scrollingView.VerticalScrollBarVisibility);
                        break;
                }
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
                scrollingView.XYFocusKeyboardNavigation = (XYFocusKeyboardNavigationMode)cmbXYFocusKeyboardNavigation.SelectedIndex;

                wuxScrollViewer.XYFocusKeyboardNavigation = scrollingView.XYFocusKeyboardNavigation;
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
                scrollingView.Margin = GetThicknessFromString(txtMargin.Text);

                wuxScrollViewer.Margin = scrollingView.Margin;
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
                scrollingView.Padding = GetThicknessFromString(txtPadding.Text);

                wuxScrollViewer.Padding = scrollingView.Padding;
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstLogs.Items.Add(ex.ToString());
            }
        }

        private void ChkIsEnabled_Checked(object sender, RoutedEventArgs e)
        {
            scrollingView.IsEnabled = true;

            wuxScrollViewer.IsEnabled = true;
        }

        private void ChkIsEnabled_Unchecked(object sender, RoutedEventArgs e)
        {
            scrollingView.IsEnabled = false;

            wuxScrollViewer.IsEnabled = false;
        }

        private void ChkIsTabStop_Checked(object sender, RoutedEventArgs e)
        {
            scrollingView.IsTabStop = true;

            wuxScrollViewer.IsTabStop = true;
        }

        private void ChkIsTabStop_Unchecked(object sender, RoutedEventArgs e)
        {
            scrollingView.IsTabStop = false;

            wuxScrollViewer.IsTabStop = false;
        }

        private void BtnGetContentMinWidth_Click(object sender, RoutedEventArgs e)
        {
            if (scrollingView.Content == null || !(scrollingView.Content is FrameworkElement))
                txtContentMinWidth.Text = string.Empty;
            else
                txtContentMinWidth.Text = ((FrameworkElement)scrollingView.Content).MinWidth.ToString();
        }

        private void BtnGetContentWidth_Click(object sender, RoutedEventArgs e)
        {
            if (scrollingView.Content == null || !(scrollingView.Content is FrameworkElement))
                txtContentWidth.Text = string.Empty;
            else
                txtContentWidth.Text = ((FrameworkElement)scrollingView.Content).Width.ToString();
        }

        private void BtnGetContentMaxWidth_Click(object sender, RoutedEventArgs e)
        {
            if (scrollingView.Content == null || !(scrollingView.Content is FrameworkElement))
                txtContentMaxWidth.Text = string.Empty;
            else
                txtContentMaxWidth.Text = ((FrameworkElement)scrollingView.Content).MaxWidth.ToString();
        }

        private void BtnSetContentMinWidth_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                double minWidth = Convert.ToDouble(txtContentMinWidth.Text);

                if (scrollingView.Content is FrameworkElement)
                {
                    ((FrameworkElement)scrollingView.Content).MinWidth = minWidth;
                }

                if (wuxScrollViewer.Content is FrameworkElement)
                {
                    ((FrameworkElement)wuxScrollViewer.Content).MinWidth = minWidth;
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstLogs.Items.Add(ex.ToString());
            }
        }

        private void BtnSetContentWidth_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                double width = Convert.ToDouble(txtContentWidth.Text);

                if (scrollingView.Content is FrameworkElement)
                {
                    ((FrameworkElement)scrollingView.Content).Width = width;
                }

                if (wuxScrollViewer.Content is FrameworkElement)
                {
                    ((FrameworkElement)wuxScrollViewer.Content).Width = width;
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstLogs.Items.Add(ex.ToString());
            }
        }

        private void BtnSetContentMaxWidth_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                double maxWidth = Convert.ToDouble(txtContentMaxWidth.Text);

                if (scrollingView.Content is FrameworkElement)
                {
                    ((FrameworkElement)scrollingView.Content).MaxWidth = maxWidth;
                }

                if (wuxScrollViewer.Content is FrameworkElement)
                {
                    ((FrameworkElement)wuxScrollViewer.Content).MaxWidth = maxWidth;
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstLogs.Items.Add(ex.ToString());
            }
        }

        private void BtnGetContentMinHeight_Click(object sender, RoutedEventArgs e)
        {
            if (scrollingView.Content == null || !(scrollingView.Content is FrameworkElement))
                txtContentMinHeight.Text = string.Empty;
            else
                txtContentMinHeight.Text = ((FrameworkElement)scrollingView.Content).MinHeight.ToString();
        }

        private void BtnGetContentHeight_Click(object sender, RoutedEventArgs e)
        {
            if (scrollingView.Content == null || !(scrollingView.Content is FrameworkElement))
                txtContentHeight.Text = string.Empty;
            else
                txtContentHeight.Text = ((FrameworkElement)scrollingView.Content).Height.ToString();
        }

        private void BtnGetContentMaxHeight_Click(object sender, RoutedEventArgs e)
        {
            if (scrollingView.Content == null || !(scrollingView.Content is FrameworkElement))
                txtContentMaxHeight.Text = string.Empty;
            else
                txtContentMaxHeight.Text = ((FrameworkElement)scrollingView.Content).MaxHeight.ToString();
        }

        private void BtnSetContentMinHeight_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                double minHeight = Convert.ToDouble(txtContentMinHeight.Text);

                if (scrollingView.Content is FrameworkElement)
                {
                    ((FrameworkElement)scrollingView.Content).MinHeight = minHeight;
                }

                if (wuxScrollViewer.Content is FrameworkElement)
                {
                    ((FrameworkElement)wuxScrollViewer.Content).MinHeight = minHeight;
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstLogs.Items.Add(ex.ToString());
            }
        }

        private void BtnSetContentHeight_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                double height = Convert.ToDouble(txtContentHeight.Text);

                if (scrollingView.Content is FrameworkElement)
                {
                    ((FrameworkElement)scrollingView.Content).Height = height;
                }

                if (wuxScrollViewer.Content is FrameworkElement)
                {
                    ((FrameworkElement)wuxScrollViewer.Content).Height = height;
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstLogs.Items.Add(ex.ToString());
            }
        }

        private void BtnSetContentMaxHeight_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                double maxHeight = Convert.ToDouble(txtContentMaxHeight.Text);

                if (scrollingView.Content is FrameworkElement)
                {
                    ((FrameworkElement)scrollingView.Content).MaxHeight = maxHeight;
                }

                if (wuxScrollViewer.Content is FrameworkElement)
                {
                    ((FrameworkElement)wuxScrollViewer.Content).MaxHeight = maxHeight;
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
            if (scrollingView.Content == null || !(scrollingView.Content is FrameworkElement))
                txtContentMargin.Text = string.Empty;
            else
                txtContentMargin.Text = ((FrameworkElement)scrollingView.Content).Margin.ToString();
        }

        private void BtnSetContentMargin_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                Thickness margin = GetThicknessFromString(txtContentMargin.Text);

                if (scrollingView.Content is FrameworkElement)
                {
                    ((FrameworkElement)scrollingView.Content).Margin = margin;
                }

                if (wuxScrollViewer.Content is FrameworkElement)
                {
                    ((FrameworkElement)wuxScrollViewer.Content).Margin = margin;
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
            if (scrollingView.Content == null || !(scrollingView.Content is Control || scrollingView.Content is Border || scrollingView.Content is StackPanel))
                txtContentPadding.Text = string.Empty;
            else if (scrollingView.Content is Control)
                txtContentPadding.Text = ((Control)scrollingView.Content).Padding.ToString();
            else if (scrollingView.Content is Border)
                txtContentPadding.Text = ((Border)scrollingView.Content).Padding.ToString();
            else
                txtContentPadding.Text = ((StackPanel)scrollingView.Content).Padding.ToString();
        }

        private void BtnSetContentPadding_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                Thickness padding = GetThicknessFromString(txtContentPadding.Text);

                if (scrollingView.Content is Control)
                {
                    ((Control)scrollingView.Content).Padding = padding;
                }
                else if (scrollingView.Content is Border)
                {
                    ((Border)scrollingView.Content).Padding = padding;
                }
                else if (scrollingView.Content is StackPanel)
                {
                    ((StackPanel)scrollingView.Content).Padding = padding;
                }

                if (wuxScrollViewer.Content is Control)
                {
                    ((Control)wuxScrollViewer.Content).Padding = padding;
                }
                else if (wuxScrollViewer.Content is Border)
                {
                    ((Border)wuxScrollViewer.Content).Padding = padding;
                }
                else if (wuxScrollViewer.Content is StackPanel)
                {
                    ((StackPanel)wuxScrollViewer.Content).Padding = padding;
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstLogs.Items.Add(ex.ToString());
            }
        }

        private void BtnGetContentActualWidth_Click(object sender, RoutedEventArgs e)
        {
            if (scrollingView.Content == null || !(scrollingView.Content is FrameworkElement))
                txtContentActualWidth.Text = string.Empty;
            else
            {
                txtContentActualWidth.Text = (scrollingView.Content as FrameworkElement).ActualWidth.ToString();

                if ((scrollingView.Content as FrameworkElement).ActualWidth != (wuxScrollViewer.Content as FrameworkElement).ActualWidth)
                {
                    lstLogs.Items.Add($"muxScrollingView.Content.ActualWidth={(scrollingView.Content as FrameworkElement).ActualWidth} != wuxScrollViewer.Content.ActualWidth={(wuxScrollViewer.Content as FrameworkElement).ActualWidth}");
                }
            }
        }

        private void BtnGetContentActualHeight_Click(object sender, RoutedEventArgs e)
        {
            if (scrollingView.Content == null || !(scrollingView.Content is FrameworkElement))
                txtContentActualHeight.Text = string.Empty;
            else
            {
                txtContentActualHeight.Text = (scrollingView.Content as FrameworkElement).ActualHeight.ToString();

                if ((scrollingView.Content as FrameworkElement).ActualHeight != (wuxScrollViewer.Content as FrameworkElement).ActualHeight)
                {
                    lstLogs.Items.Add($"muxScrollingView.Content.ActualHeight={(scrollingView.Content as FrameworkElement).ActualHeight} != wuxScrollViewer.Content.ActualHeight={(wuxScrollViewer.Content as FrameworkElement).ActualHeight}");
                }
            }
        }

        private void BtnGetContentDesiredSize_Click(object sender, RoutedEventArgs e)
        {
            if (scrollingView.Content == null)
                txtContentDesiredSize.Text = string.Empty;
            else
            {
                txtContentDesiredSize.Text = scrollingView.Content.DesiredSize.ToString();

                if (scrollingView.Content.DesiredSize != (wuxScrollViewer.Content as UIElement).DesiredSize)
                {
                    lstLogs.Items.Add($"muxScrollingView.Content.DesiredSize={scrollingView.Content.DesiredSize} != wuxScrollViewer.Content.DesiredSize={(wuxScrollViewer.Content as UIElement).DesiredSize}");
                }
            }
        }

        private void BtnGetContentRenderSize_Click(object sender, RoutedEventArgs e)
        {
            if (scrollingView.Content == null)
                txtContentRenderSize.Text = string.Empty;
            else
            {
                txtContentRenderSize.Text = scrollingView.Content.RenderSize.ToString();

                if (scrollingView.Content.RenderSize != (wuxScrollViewer.Content as UIElement).RenderSize)
                {
                    lstLogs.Items.Add($"muxScrollingView.Content.RenderSize={scrollingView.Content.RenderSize} != wuxScrollViewer.Content.RenderSize={(wuxScrollViewer.Content as UIElement).RenderSize}");
                }
            }
        }

        private void CmbContentHorizontalAlignment_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            try
            {
                if (scrollingView.Content is FrameworkElement)
                {
                    ((FrameworkElement)scrollingView.Content).HorizontalAlignment = (HorizontalAlignment)cmbContentHorizontalAlignment.SelectedIndex;
                }

                if (wuxScrollViewer.Content is FrameworkElement)
                {
                    wuxScrollViewer.HorizontalContentAlignment =
                    ((FrameworkElement)wuxScrollViewer.Content).HorizontalAlignment = (HorizontalAlignment)cmbContentHorizontalAlignment.SelectedIndex;
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
                if (scrollingView.Content is FrameworkElement)
                {
                    ((FrameworkElement)scrollingView.Content).VerticalAlignment = (VerticalAlignment)cmbContentVerticalAlignment.SelectedIndex;
                }

                if (wuxScrollViewer.Content is FrameworkElement)
                {
                    wuxScrollViewer.VerticalContentAlignment =
                    ((FrameworkElement)wuxScrollViewer.Content).VerticalAlignment = (VerticalAlignment)cmbContentVerticalAlignment.SelectedIndex;
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
                if (scrollingView.Content is FrameworkElement)
                {
                    switch (cmbContentManipulationMode.SelectedIndex)
                    {
                        case 0:
                            scrollingView.Content.ManipulationMode = ManipulationModes.System;
                            break;
                        case 1:
                            scrollingView.Content.ManipulationMode = ManipulationModes.None;
                            break;
                    }
                }

                if (wuxScrollViewer.Content is FrameworkElement)
                {
                    switch (cmbContentManipulationMode.SelectedIndex)
                    {
                        case 0:
                            (wuxScrollViewer.Content as FrameworkElement).ManipulationMode = ManipulationModes.System;
                            break;
                        case 1:
                            (wuxScrollViewer.Content as FrameworkElement).ManipulationMode = ManipulationModes.None;
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
                if (scrollingView.Content is FrameworkElement)
                {
                    cmbContentHorizontalAlignment.SelectedIndex = (int)((FrameworkElement)scrollingView.Content).HorizontalAlignment;
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
                if (scrollingView.Content is FrameworkElement)
                {
                    cmbContentVerticalAlignment.SelectedIndex = (int)((FrameworkElement)scrollingView.Content).VerticalAlignment;
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
                if (scrollingView.Content != null)
                {
                    switch (scrollingView.Content.ManipulationMode)
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
                cmbContentOrientation.SelectedIndex = (int)scrollingView.ContentOrientation;
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
                cmbHorizontalScrollMode.SelectedIndex = (int)scrollingView.HorizontalScrollMode;
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
                cmbHorizontalScrollChainingMode.SelectedIndex = (int)scrollingView.HorizontalScrollChainingMode;
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
                cmbHorizontalScrollRailingMode.SelectedIndex = (int)scrollingView.HorizontalScrollRailingMode;
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
                cmbVerticalScrollMode.SelectedIndex = (int)scrollingView.VerticalScrollMode;
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
                cmbVerticalScrollChainingMode.SelectedIndex = (int)scrollingView.VerticalScrollChainingMode;
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
                cmbVerticalScrollRailingMode.SelectedIndex = (int)scrollingView.VerticalScrollRailingMode;
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
                cmbZoomMode.SelectedIndex = (int)scrollingView.ZoomMode;
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
                txtMinZoomFactor.Text = scrollingView.MinZoomFactor.ToString();
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
                txtMaxZoomFactor.Text = scrollingView.MaxZoomFactor.ToString();
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
                cmbZoomChainingMode.SelectedIndex = (int)scrollingView.ZoomChainingMode;
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstLogs.Items.Add(ex.ToString());
            }
        }

        private void UpdateIgnoredInputKind()
        {
            try
            {
                switch (scrollingView.IgnoredInputKind)
                {
                    case InputKind.None:
                        cmbIgnoredInputKind.SelectedIndex = 0;
                        break;
                    case InputKind.Touch:
                        cmbIgnoredInputKind.SelectedIndex = 1;
                        break;
                    case InputKind.Pen:
                        cmbIgnoredInputKind.SelectedIndex = 2;
                        break;
                    case InputKind.MouseWheel:
                        cmbIgnoredInputKind.SelectedIndex = 3;
                        break;
                    case InputKind.Keyboard:
                        cmbIgnoredInputKind.SelectedIndex = 4;
                        break;
                    case InputKind.Gamepad:
                        cmbIgnoredInputKind.SelectedIndex = 5;
                        break;
                    case InputKind.All:
                        cmbIgnoredInputKind.SelectedIndex = 6;
                        break;
                    default:
                        lstLogs.Items.Add("Unexpected IgnoredInputKind value.");
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
            txtWidth.Text = scrollingView.Width.ToString();
        }

        private void UpdateHeight()
        {
            txtHeight.Text = scrollingView.Height.ToString();
        }

        private void UpdateBackground()
        {
            if (scrollingView.Background == null)
            {
                cmbBackground.SelectedIndex = 0;
            }
            else if ((scrollingView.Background as SolidColorBrush).Color == Colors.Transparent)
            {
                cmbBackground.SelectedIndex = 1;
            }
            else if ((scrollingView.Background as SolidColorBrush).Color == Colors.AliceBlue)
            {
                cmbBackground.SelectedIndex = 2;
            }
            else if ((scrollingView.Background as SolidColorBrush).Color == Colors.Aqua)
            {
                cmbBackground.SelectedIndex = 3;
            }
        }

        private void UpdateContent()
        {
            if (scrollingView.Content == null)
            {
                cmbContent.SelectedIndex = 0;
            }
            else if (scrollingView.Content is Image)
            {
                if (((scrollingView.Content as Image).Source as BitmapImage).UriSource.AbsolutePath.ToLower().Contains("large"))
                {
                    cmbContent.SelectedIndex = 2;
                }
                else
                {
                    cmbContent.SelectedIndex = 1;
                }
            }
            else if (scrollingView.Content is Rectangle)
            {
                cmbContent.SelectedIndex = 3;
            }
            else if (scrollingView.Content is Button)
            {
                cmbContent.SelectedIndex = 4;
            }
            else if (scrollingView.Content is Border)
            {
                cmbContent.SelectedIndex = 5;
            }
            else if (scrollingView.Content is StackPanel)
            {
                if ((scrollingView.Content as StackPanel).Children.Count == 2)
                {
                    cmbContent.SelectedIndex = 6;
                }
                else
                {
                    cmbContent.SelectedIndex = 7;
                }
            }
            else if (scrollingView.Content is Viewbox)
            {
                cmbContent.SelectedIndex = 8;
            }
        }

        private void UpdateMargin()
        {
            txtMargin.Text = scrollingView.Margin.ToString();
        }

        private void UpdatePadding()
        {
            txtPadding.Text = scrollingView.Padding.ToString();
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

            viewbox = new Viewbox();
            viewbox.Name = "viewbox";
            Rectangle viewboxChild = new Rectangle() { Fill = lgb, Width = 600, Height = 400 };
            viewbox.Child = viewboxChild;

            wuxLargeImg = new Image() { Source = new BitmapImage(new Uri("ms-appx:/Assets/LargeWisteria.jpg")) };
            wuxRectangle = new Rectangle() { Fill = lgb };
            wuxRectangle.Name = "wuxRect";
            wuxButton = new Button() { Content = "Button" };
            wuxButton.Name = "wuxBtn";
            borderChild = new Rectangle() { Fill = lgb };
            wuxBorder = new Border() { BorderBrush = new SolidColorBrush(Colors.Chartreuse), BorderThickness = new Thickness(5), Child = borderChild };
            wuxBorder.Name = "wuxBdr";

            wuxSp1 = new StackPanel();
            wuxSp1.Name = "wuxSp1";
            button1 = new Button() { Content = "Button1" };
            button1.Name = "wuxBtn1";
            button1.Margin = new Thickness(50);
            button2 = new Button() { Content = "Button2" };
            button2.Name = "wuxBtn2";
            button2.Margin = new Thickness(50);
            wuxSp1.Children.Add(button1);
            wuxSp1.Children.Add(button2);

            wuxSp2 = new StackPanel();
            wuxSp2.Name = "wuxSp2";
            wuxSp2.Children.Add(new Rectangle() { Height = 200, Fill = new SolidColorBrush(Colors.Indigo) });
            wuxSp2.Children.Add(new Rectangle() { Height = 200, Fill = new SolidColorBrush(Colors.Orange) });
            wuxSp2.Children.Add(new Rectangle() { Height = 200, Fill = new SolidColorBrush(Colors.Purple) });
            wuxSp2.Children.Add(new Rectangle() { Height = 200, Fill = new SolidColorBrush(Colors.Goldenrod) });

            wuxViewbox = new Viewbox();
            wuxViewbox.Name = "wuxViewbox";
            viewboxChild = new Rectangle() { Fill = lgb, Width = 600, Height = 400 };
            wuxViewbox.Child = viewboxChild;
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

        private void UseScrollingView(ScrollingView sv2)
        {
            if (scrollingView == sv2 || sv2 == null)
                return;

            try
            {
                if (scrollingView == null && (chkLogScrollingViewMessages.IsChecked == true || chkLogScrollingPresenterMessages.IsChecked == true))
                {
                    MUXControlsTestHooks.LoggingMessage += MUXControlsTestHooks_LoggingMessage;

                    if (chkLogScrollingPresenterMessages.IsChecked == true)
                    {
                        MUXControlsTestHooks.SetLoggingLevelForType("ScrollingPresenter", isLoggingInfoLevel: true, isLoggingVerboseLevel: true);
                    }
                    if (chkLogScrollingViewMessages.IsChecked == true)
                    {
                        MUXControlsTestHooks.SetLoggingLevelForType("ScrollingView", isLoggingInfoLevel: true, isLoggingVerboseLevel: true);
                    }
                }

                if (scrollingView != null)
                {
                    scrollingView.GettingFocus -= ScrollingView_GettingFocus;
                    scrollingView.GotFocus -= ScrollingView_GotFocus;
                    scrollingView.LosingFocus -= ScrollingView_LosingFocus;
                    scrollingView.LostFocus -= ScrollingView_LostFocus;

                    if (chkLogScrollingViewEvents.IsChecked == true)
                    {
                        scrollingView.ExtentChanged -= ScrollingView_ExtentChanged;
                        scrollingView.StateChanged -= ScrollingView_StateChanged;
                        scrollingView.ViewChanged -= ScrollingView_ViewChanged;
                        scrollingView.ScrollAnimationStarting -= ScrollingView_ScrollAnimationStarting;
                        scrollingView.ZoomAnimationStarting -= ScrollingView_ZoomAnimationStarting;
                    }

                    ScrollingPresenter scrollingPresenter = ScrollingViewTestHooks.GetScrollingPresenterPart(scrollingView);

                    if (scrollingPresenter != null && chkLogScrollingPresenterEvents.IsChecked == true)
                    {
                        scrollingPresenter.ExtentChanged -= ScrollingPresenter_ExtentChanged;
                        scrollingPresenter.StateChanged -= ScrollingPresenter_StateChanged;
                        scrollingPresenter.ViewChanged -= ScrollingPresenter_ViewChanged;
                        scrollingPresenter.ScrollAnimationStarting -= ScrollingPresenter_ScrollAnimationStarting;
                        scrollingPresenter.ZoomAnimationStarting -= ScrollingPresenter_ZoomAnimationStarting;
                    }
                }

                scrollingView = sv2;

                UpdateContentOrientation();
                UpdateHorizontalScrollMode();
                UpdateHorizontalScrollChainingMode();
                UpdateHorizontalScrollRailingMode();
                UpdateVerticalScrollMode();
                UpdateVerticalScrollChainingMode();
                UpdateVerticalScrollRailingMode();
                UpdateZoomMode();
                UpdateZoomChainingMode();
                UpdateIgnoredInputKind();
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
                UpdateExtentWidth();
                UpdateExtentHeight();
#if USE_SCROLLMODE_AUTO
                UpdateTblComputedHorizontalScrollMode();
                UpdateTblComputedVerticalScrollMode();
#endif
                UpdateCmbContentHorizontalAlignment();
                UpdateCmbContentVerticalAlignment();
                UpdateCmbContentManipulationMode();

                chkIsEnabled.IsChecked = scrollingView.IsEnabled;
                chkIsTabStop.IsChecked = scrollingView.IsTabStop;

                if (scrollingView != null)
                {
                    scrollingView.GettingFocus += ScrollingView_GettingFocus;
                    scrollingView.GotFocus += ScrollingView_GotFocus;
                    scrollingView.LosingFocus += ScrollingView_LosingFocus;
                    scrollingView.LostFocus += ScrollingView_LostFocus;

                    if (chkLogScrollingViewEvents.IsChecked == true)
                    {
                        scrollingView.ExtentChanged += ScrollingView_ExtentChanged;
                        scrollingView.StateChanged += ScrollingView_StateChanged;
                        scrollingView.ViewChanged += ScrollingView_ViewChanged;
                        scrollingView.ScrollAnimationStarting += ScrollingView_ScrollAnimationStarting;
                        scrollingView.ZoomAnimationStarting += ScrollingView_ZoomAnimationStarting;
                    }

                    ScrollingPresenter scrollingPresenter = ScrollingViewTestHooks.GetScrollingPresenterPart(scrollingView);

                    if (scrollingPresenter != null && chkLogScrollingPresenterEvents.IsChecked == true)
                    {
                        scrollingPresenter.ExtentChanged += ScrollingPresenter_ExtentChanged;
                        scrollingPresenter.StateChanged += ScrollingPresenter_StateChanged;
                        scrollingPresenter.ViewChanged += ScrollingPresenter_ViewChanged;
                        scrollingPresenter.ScrollAnimationStarting += ScrollingPresenter_ScrollAnimationStarting;
                        scrollingPresenter.ZoomAnimationStarting += ScrollingPresenter_ZoomAnimationStarting;
                    }
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstLogs.Items.Add(ex.ToString());
            }
        }

        private Windows.UI.Xaml.Controls.ScrollBarVisibility MuxScrollBarVisibilityToWuxScrollBarVisibility(ScrollBarVisibility muxScrollBarVisibility)
        {
            switch (muxScrollBarVisibility)
            {
                case ScrollBarVisibility.Auto:
                    return Windows.UI.Xaml.Controls.ScrollBarVisibility.Auto;
                case ScrollBarVisibility.Hidden:
                    return Windows.UI.Xaml.Controls.ScrollBarVisibility.Hidden;
                default:
                    return Windows.UI.Xaml.Controls.ScrollBarVisibility.Visible;
            }
        }

        private Windows.UI.Xaml.Controls.ScrollMode MuxScrollModeToWuxScrollMode(ScrollMode muxScrollMode)
        {
            switch (muxScrollMode)
            {
                case ScrollMode.Disabled:
                    return Windows.UI.Xaml.Controls.ScrollMode.Disabled;
                default:
                    return Windows.UI.Xaml.Controls.ScrollMode.Enabled;
            }
        }

        private Windows.UI.Xaml.Controls.ZoomMode MuxZoomModeToWuxZoomMode(ZoomMode muxZoomMode)
        {
            switch (muxZoomMode)
            {
                case ZoomMode.Disabled:
                    return Windows.UI.Xaml.Controls.ZoomMode.Disabled;
                default:
                    return Windows.UI.Xaml.Controls.ZoomMode.Enabled;
            }
        }

        private bool MuxRailModeToWuxRailMode(RailingMode muxRailingMode)
        {
            switch (muxRailingMode)
            {
                case RailingMode.Disabled:
                    return false;
                default:
                    return true;
            }
        }

        private void ScrollingView_GettingFocus(UIElement sender, Windows.UI.Xaml.Input.GettingFocusEventArgs args)
        {
            FrameworkElement oldFE = args.OldFocusedElement as FrameworkElement;
            string oldFEName = (oldFE == null) ? "null" : oldFE.Name;
            FrameworkElement newFE = args.NewFocusedElement as FrameworkElement;
            string newFEName = (newFE == null) ? "null" : newFE.Name;

            AppendAsyncEventMessage("ScrollingView.GettingFocus FocusState=" + args.FocusState + ", Direction=" + args.Direction + ", InputDevice=" + args.InputDevice + ", oldFE=" + oldFEName + ", newFE=" + newFEName);
        }

        private void ScrollingView_LostFocus(object sender, RoutedEventArgs e)
        {
            AppendAsyncEventMessage("ScrollingView.LostFocus");
        }

        private void ScrollingView_LosingFocus(UIElement sender, Windows.UI.Xaml.Input.LosingFocusEventArgs args)
        {
            FrameworkElement oldFE = args.OldFocusedElement as FrameworkElement;
            string oldFEName = (oldFE == null) ? "null" : oldFE.Name;
            FrameworkElement newFE = args.NewFocusedElement as FrameworkElement;
            string newFEName = (newFE == null) ? "null" : newFE.Name;

            AppendAsyncEventMessage("ScrollingView.LosingFocus FocusState=" + args.FocusState + ", Direction=" + args.Direction + ", InputDevice=" + args.InputDevice + ", oldFE=" + oldFEName + ", newFE=" + newFEName);
        }

        private void ScrollingView_GotFocus(object sender, RoutedEventArgs e)
        {
            AppendAsyncEventMessage("ScrollingView.GotFocus");
        }

        private void ScrollingPresenter_ExtentChanged(ScrollingPresenter sender, object args)
        {
            AppendAsyncEventMessage("ScrollingPresenter.ExtentChanged ExtentWidth=" + sender.ExtentWidth + ", ExtentHeight=" + sender.ExtentHeight);
        }

        private void ScrollingPresenter_StateChanged(ScrollingPresenter sender, object args)
        {
            AppendAsyncEventMessage("ScrollingPresenter.StateChanged " + sender.State.ToString());
        }

        private void ScrollingPresenter_ViewChanged(ScrollingPresenter sender, object args)
        {
            AppendAsyncEventMessage("ScrollingPresenter.ViewChanged H=" + sender.HorizontalOffset.ToString() + ", V=" + sender.VerticalOffset + ", ZF=" + sender.ZoomFactor);
        }

        private void ScrollingPresenter_ScrollAnimationStarting(ScrollingPresenter sender, ScrollingScrollAnimationStartingEventArgs args)
        {
            AppendAsyncEventMessage("ScrollingPresenter.ScrollAnimationStarting OffsetsChangeId=" + args.ScrollInfo.OffsetsChangeId + " SP=(" + args.StartPosition.X + "," + args.StartPosition.Y +") EP=(" + args.EndPosition.X + "," + args.EndPosition.Y +")");
        }

        private void ScrollingPresenter_ZoomAnimationStarting(ScrollingPresenter sender, ScrollingZoomAnimationStartingEventArgs args)
        {
            AppendAsyncEventMessage("ScrollingPresenter.ZoomAnimationStarting ZoomFactorChangeId=" + args.ZoomInfo.ZoomFactorChangeId + ", CenterPoint=" + args.CenterPoint + ", SZF=" + args.StartZoomFactor + ", EZF=" + args.EndZoomFactor);
        }

        private void ScrollingView_ExtentChanged(ScrollingView sender, object args)
        {
            AppendAsyncEventMessage("ScrollingView.ExtentChanged ExtentWidth=" + sender.ExtentWidth + ", ExtentHeight=" + sender.ExtentHeight);
        }

        private void ScrollingView_StateChanged(ScrollingView sender, object args)
        {
            AppendAsyncEventMessage("ScrollingView.StateChanged " + sender.State.ToString());
        }

        private void ScrollingView_ViewChanged(ScrollingView sender, object args)
        {
            AppendAsyncEventMessage("ScrollingView.ViewChanged H=" + sender.HorizontalOffset.ToString() + ", V=" + sender.VerticalOffset + ", ZF=" + sender.ZoomFactor);
        }

        private void ScrollingView_ScrollAnimationStarting(ScrollingView sender, ScrollingScrollAnimationStartingEventArgs args)
        {
            AppendAsyncEventMessage("ScrollingView.ScrollAnimationStarting OffsetsChangeId=" + args.ScrollInfo.OffsetsChangeId);
        }

        private void ScrollingView_ZoomAnimationStarting(ScrollingView sender, ScrollingZoomAnimationStartingEventArgs args)
        {
            AppendAsyncEventMessage("ScrollingView.ZoomAnimationStarting ZoomFactorChangeId=" + args.ZoomInfo.ZoomFactorChangeId + ", CenterPoint=" + args.CenterPoint);
        }

        private void BtnClearLogs_Click(object sender, RoutedEventArgs e)
        {
            lstLogs.Items.Clear();
        }

        private void ChkLogScrollingPresenterEvents_Checked(object sender, RoutedEventArgs e)
        {
            if (scrollingView != null)
            {
                ScrollingPresenter scrollingPresenter = ScrollingViewTestHooks.GetScrollingPresenterPart(scrollingView);

                if (scrollingPresenter != null)
                {
                    scrollingPresenter.ExtentChanged += ScrollingPresenter_ExtentChanged;
                    scrollingPresenter.StateChanged += ScrollingPresenter_StateChanged;
                    scrollingPresenter.ViewChanged += ScrollingPresenter_ViewChanged;
                    scrollingPresenter.ScrollAnimationStarting += ScrollingPresenter_ScrollAnimationStarting;
                    scrollingPresenter.ZoomAnimationStarting += ScrollingPresenter_ZoomAnimationStarting;
                }
            }
        }

        private void ChkLogScrollingPresenterEvents_Unchecked(object sender, RoutedEventArgs e)
        {
            if (scrollingView != null)
            {
                ScrollingPresenter scrollingPresenter = ScrollingViewTestHooks.GetScrollingPresenterPart(scrollingView);

                if (scrollingPresenter != null)
                {
                    scrollingPresenter.ExtentChanged -= ScrollingPresenter_ExtentChanged;
                    scrollingPresenter.StateChanged -= ScrollingPresenter_StateChanged;
                    scrollingPresenter.ViewChanged -= ScrollingPresenter_ViewChanged;
                    scrollingPresenter.ScrollAnimationStarting -= ScrollingPresenter_ScrollAnimationStarting;
                    scrollingPresenter.ZoomAnimationStarting -= ScrollingPresenter_ZoomAnimationStarting;
                }
            }
        }

        private void ChkLogScrollingViewEvents_Checked(object sender, RoutedEventArgs e)
        {
            if (scrollingView != null)
            {
                scrollingView.ExtentChanged += ScrollingView_ExtentChanged;
                scrollingView.StateChanged += ScrollingView_StateChanged;
                scrollingView.ViewChanged += ScrollingView_ViewChanged;
                scrollingView.ScrollAnimationStarting += ScrollingView_ScrollAnimationStarting;
                scrollingView.ZoomAnimationStarting += ScrollingView_ZoomAnimationStarting;
            }
        }

        private void ChkLogScrollingViewEvents_Unchecked(object sender, RoutedEventArgs e)
        {
            if (scrollingView != null)
            {
                scrollingView.ExtentChanged -= ScrollingView_ExtentChanged;
                scrollingView.StateChanged -= ScrollingView_StateChanged;
                scrollingView.ViewChanged -= ScrollingView_ViewChanged;
                scrollingView.ScrollAnimationStarting -= ScrollingView_ScrollAnimationStarting;
                scrollingView.ZoomAnimationStarting -= ScrollingView_ZoomAnimationStarting;
            }
        }

        private void ChkLogScrollingPresenterMessages_Checked(object sender, RoutedEventArgs e)
        {
            MUXControlsTestHooks.SetLoggingLevelForType("ScrollingPresenter", isLoggingInfoLevel: true, isLoggingVerboseLevel: true);
            if (chkLogScrollingViewMessages.IsChecked == false)
                MUXControlsTestHooks.LoggingMessage += MUXControlsTestHooks_LoggingMessage;
        }

        private void ChkLogScrollingPresenterMessages_Unchecked(object sender, RoutedEventArgs e)
        {
            MUXControlsTestHooks.SetLoggingLevelForType("ScrollingPresenter", isLoggingInfoLevel: false, isLoggingVerboseLevel: false);
            if (chkLogScrollingViewMessages.IsChecked == false)
                MUXControlsTestHooks.LoggingMessage -= MUXControlsTestHooks_LoggingMessage;
        }

        private void ChkLogScrollingViewMessages_Checked(object sender, RoutedEventArgs e)
        {
            MUXControlsTestHooks.SetLoggingLevelForType("ScrollingView", isLoggingInfoLevel: true, isLoggingVerboseLevel: true);
            if (chkLogScrollingPresenterMessages.IsChecked == false)
                MUXControlsTestHooks.LoggingMessage += MUXControlsTestHooks_LoggingMessage;
        }

        private void ChkLogScrollingViewMessages_Unchecked(object sender, RoutedEventArgs e)
        {
            MUXControlsTestHooks.SetLoggingLevelForType("ScrollingView", isLoggingInfoLevel: false, isLoggingVerboseLevel: false);
            if (chkLogScrollingPresenterMessages.IsChecked == false)
                MUXControlsTestHooks.LoggingMessage -= MUXControlsTestHooks_LoggingMessage;
        }

        private void ChkAutoHideScrollControllers_Indeterminate(object sender, RoutedEventArgs e)
        {
            ScrollingViewTestHooks.SetAutoHideScrollControllers(scrollingView, null);
        }

        private void ChkAutoHideScrollControllers_Checked(object sender, RoutedEventArgs e)
        {
            ScrollingViewTestHooks.SetAutoHideScrollControllers(scrollingView, true);
        }

        private void ChkAutoHideScrollControllers_Unchecked(object sender, RoutedEventArgs e)
        {
            ScrollingViewTestHooks.SetAutoHideScrollControllers(scrollingView, false);
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
