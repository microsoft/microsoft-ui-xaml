// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;

using ScrollingView = Microsoft.UI.Xaml.Controls.ScrollingView;
using ContentOrientation = Microsoft.UI.Xaml.Controls.ScrollingContentOrientation;
using ScrollMode = Microsoft.UI.Xaml.Controls.ScrollingScrollMode;
using ZoomMode = Microsoft.UI.Xaml.Controls.ScrollingZoomMode;
using ScrollBarVisibility = Microsoft.UI.Xaml.Controls.ScrollingScrollBarVisibility;
using MUXControlsTestHooks = Microsoft.UI.Private.Controls.MUXControlsTestHooks;
using MUXControlsTestHooksLoggingMessageEventArgs = Microsoft.UI.Private.Controls.MUXControlsTestHooksLoggingMessageEventArgs;

namespace MUXControlsTestApp
{
    public sealed partial class ScrollingViewWithScrollControllersPage : TestPage
    {
        private Object asyncEventReportingLock = new Object();
        private List<string> lstAsyncEventMessage = new List<string>();
        private ScrollingView scrollingView = null;

        public ScrollingViewWithScrollControllersPage()
        {
            this.InitializeComponent();
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

        private void ChkScrollingPresenterAttachedProperties_Checked(object sender, RoutedEventArgs e)
        {
            if (grdScrollingPresenterAttachedProperties != null)
                grdScrollingPresenterAttachedProperties.Visibility = Visibility.Visible;
        }

        private void ChkScrollingPresenterAttachedProperties_Unchecked(object sender, RoutedEventArgs e)
        {
            if (grdScrollingPresenterAttachedProperties != null)
                grdScrollingPresenterAttachedProperties.Visibility = Visibility.Collapsed;
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

        private void BtnGetVerticalScrollMode_Click(object sender, RoutedEventArgs e)
        {
            UpdateVerticalScrollMode();
        }

        private void BtnGetZoomMode_Click(object sender, RoutedEventArgs e)
        {
            UpdateZoomMode();
        }

        private void BtnSetContentOrientation_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                ContentOrientation co = (ContentOrientation)cmbContentOrientation.SelectedIndex;
                scrollingView.ContentOrientation = co;
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
                scrollingView.ZoomMode = ssm;
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstLogs.Items.Add(ex.ToString());
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
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstLogs.Items.Add(ex.ToString());
            }
        }

        private void BtnGetHorizontalScrollBarVisibility_Click(object sender, RoutedEventArgs e)
        {
            UpdateHorizontalScrollBarVisibility();
        }

        private void BtnSetHorizontalScrollBarVisibility_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                scrollingView.HorizontalScrollBarVisibility = (ScrollBarVisibility)cmbHorizontalScrollBarVisibility.SelectedIndex;
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstLogs.Items.Add(ex.ToString());
            }
        }

        private void BtnGetVerticalScrollBarVisibility_Click(object sender, RoutedEventArgs e)
        {
            UpdateVerticalScrollBarVisibility();
        }

        private void BtnSetVerticalScrollBarVisibility_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                scrollingView.VerticalScrollBarVisibility = (ScrollBarVisibility)cmbVerticalScrollBarVisibility.SelectedIndex;
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
        }

        private void ChkIsEnabled_Unchecked(object sender, RoutedEventArgs e)
        {
            scrollingView.IsEnabled = false;
        }

        private void ChkIsTabStop_Checked(object sender, RoutedEventArgs e)
        {
            scrollingView.IsTabStop = true;
        }

        private void ChkIsTabStop_Unchecked(object sender, RoutedEventArgs e)
        {
            scrollingView.IsTabStop = false;
        }

        private void BtnGetContentWidth_Click(object sender, RoutedEventArgs e)
        {
            UpdateContentWidth();
        }

        private void BtnSetContentWidth_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                FrameworkElement contentAsFE = scrollingView.Content as FrameworkElement;
                if (contentAsFE != null)
                    contentAsFE.Width = Convert.ToDouble(txtContentWidth.Text);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstLogs.Items.Add(ex.ToString());
            }
        }

        private void BtnGetContentHeight_Click(object sender, RoutedEventArgs e)
        {
            UpdateContentHeight();
        }

        private void BtnSetContentHeight_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                FrameworkElement contentAsFE = scrollingView.Content as FrameworkElement;
                if (contentAsFE != null)
                    contentAsFE.Height = Convert.ToDouble(txtContentHeight.Text);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstLogs.Items.Add(ex.ToString());
            }
        }

        private void BtnGetContentMargin_Click(object sender, RoutedEventArgs e)
        {
            UpdateContentMargin();
        }

        private void BtnSetContentMargin_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                FrameworkElement contentAsFE = scrollingView.Content as FrameworkElement;
                if (contentAsFE != null)
                    contentAsFE.Margin = GetThicknessFromString(txtContentMargin.Text);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstLogs.Items.Add(ex.ToString());
            }
        }

        private void ChkIsContentEnabled_Checked(object sender, RoutedEventArgs e)
        {
            Control contentAsC = scrollingView.Content as Control;
            if (contentAsC != null)
                contentAsC.IsEnabled = true;
        }

        private void ChkIsContentEnabled_Unchecked(object sender, RoutedEventArgs e)
        {
            Control contentAsC = scrollingView.Content as Control;
            if (contentAsC != null)
                contentAsC.IsEnabled = false;
        }

        private void ChkIsContentTabStop_Checked(object sender, RoutedEventArgs e)
        {
            Control contentAsC = scrollingView.Content as Control;
            if (contentAsC != null)
                contentAsC.IsTabStop = true;
        }

        private void ChkIsContentTabStop_Unchecked(object sender, RoutedEventArgs e)
        {
            Control contentAsC = scrollingView.Content as Control;
            if (contentAsC != null)
                contentAsC.IsTabStop = false;
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

        private void UpdateWidth()
        {
            txtWidth.Text = scrollingView.Width.ToString();
        }

        private void UpdateHeight()
        {
            txtHeight.Text = scrollingView.Height.ToString();
        }

        private void UpdatePadding()
        {
            txtPadding.Text = scrollingView.Padding.ToString();
        }

        private void UpdateHorizontalScrollBarVisibility()
        {
            cmbHorizontalScrollBarVisibility.SelectedIndex = (int)scrollingView.HorizontalScrollBarVisibility;
        }

        private void UpdateVerticalScrollBarVisibility()
        {
            cmbVerticalScrollBarVisibility.SelectedIndex = (int)scrollingView.VerticalScrollBarVisibility;
        }

        private void UpdateContentWidth()
        {
            FrameworkElement contentAsFE = scrollingView.Content as FrameworkElement;
            if (contentAsFE == null)
                txtContentWidth.Text = string.Empty;
            else
                txtContentWidth.Text = contentAsFE.Width.ToString();
        }

        private void UpdateContentHeight()
        {
            FrameworkElement contentAsFE = scrollingView.Content as FrameworkElement;
            if (contentAsFE == null)
                txtContentHeight.Text = string.Empty;
            else
                txtContentHeight.Text = contentAsFE.Height.ToString();
        }

        private void UpdateContentMargin()
        {
            FrameworkElement contentAsFE = scrollingView.Content as FrameworkElement;
            if (contentAsFE == null)
                txtContentMargin.Text = string.Empty;
            else
                txtContentMargin.Text = contentAsFE.Margin.ToString();
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

                scrollingView = sv2;

                UpdateContentOrientation();
                UpdateHorizontalScrollMode();
                UpdateVerticalScrollMode();
                UpdateZoomMode();

                UpdateWidth();
                UpdateHeight();
                UpdatePadding();
                UpdateHorizontalScrollBarVisibility();
                UpdateVerticalScrollBarVisibility();

                chkIsEnabled.IsChecked = scrollingView.IsEnabled;
                chkIsTabStop.IsChecked = scrollingView.IsTabStop;

                UpdateContentWidth();
                UpdateContentHeight();
                UpdateContentMargin();

                Control contentAsC = scrollingView.Content as Control;
                if (contentAsC != null)
                {
                    chkIsContentEnabled.IsChecked = contentAsC.IsEnabled;
                    chkIsContentTabStop.IsChecked = contentAsC.IsTabStop;
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstLogs.Items.Add(ex.ToString());
            }
        }

        private void BtnClearLogs_Click(object sender, RoutedEventArgs e)
        {
            lstLogs.Items.Clear();
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

            lock (asyncEventReportingLock)
            {
                lstAsyncEventMessage.Add(asyncEventMessage);

                var ignored = this.Dispatcher.RunAsync(Windows.UI.Core.CoreDispatcherPriority.Normal,
                                                       AppendAsyncEventMessage);
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
