// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;

#if !BUILD_WINDOWS
using ScrollerView = Microsoft.UI.Xaml.Controls.ScrollerView;
using ScrollerScrollMode = Microsoft.UI.Xaml.Controls.ScrollerScrollMode;
using ScrollerZoomMode = Microsoft.UI.Xaml.Controls.ScrollerZoomMode;
using ScrollerViewScrollControllerVisibility = Microsoft.UI.Xaml.Controls.ScrollerViewScrollControllerVisibility;
using MUXControlsTestHooks = Microsoft.UI.Private.Controls.MUXControlsTestHooks;
using MUXControlsTestHooksLoggingMessageEventArgs = Microsoft.UI.Private.Controls.MUXControlsTestHooksLoggingMessageEventArgs;
#endif

namespace MUXControlsTestApp
{
    public sealed partial class ScrollerViewWithScrollControllersPage : TestPage
    {
        private Object asyncEventReportingLock = new Object();
        private List<string> lstAsyncEventMessage = new List<string>();
        private ScrollerView scrollerView = null;

        public ScrollerViewWithScrollControllersPage()
        {
            this.InitializeComponent();
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

        private void ChkScrollerAttachedProperties_Checked(object sender, RoutedEventArgs e)
        {
            if (grdScrollerAttachedProperties != null)
                grdScrollerAttachedProperties.Visibility = Visibility.Visible;
        }

        private void ChkScrollerAttachedProperties_Unchecked(object sender, RoutedEventArgs e)
        {
            if (grdScrollerAttachedProperties != null)
                grdScrollerAttachedProperties.Visibility = Visibility.Collapsed;
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

        private void BtnGetVerticalScrollMode_Click(object sender, RoutedEventArgs e)
        {
            UpdateVerticalScrollMode();
        }

        private void BtnGetZoomMode_Click(object sender, RoutedEventArgs e)
        {
            UpdateZoomMode();
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

        private void BtnGetHorizontalScrollControllerVisibility_Click(object sender, RoutedEventArgs e)
        {
            UpdateHorizontalScrollControllerVisibility();
        }

        private void BtnSetHorizontalScrollControllerVisibility_Click(object sender, RoutedEventArgs e)
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

        private void BtnGetVerticalScrollControllerVisibility_Click(object sender, RoutedEventArgs e)
        {
            UpdateVerticalScrollControllerVisibility();
        }

        private void BtnSetVerticalScrollControllerVisibility_Click(object sender, RoutedEventArgs e)
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
            UpdateContentWidth();
        }

        private void BtnSetContentWidth_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                FrameworkElement contentAsFE = scrollerView.Content as FrameworkElement;
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
                FrameworkElement contentAsFE = scrollerView.Content as FrameworkElement;
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
                FrameworkElement contentAsFE = scrollerView.Content as FrameworkElement;
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
            Control contentAsC = scrollerView.Content as Control;
            if (contentAsC != null)
                contentAsC.IsEnabled = true;
        }

        private void ChkIsContentEnabled_Unchecked(object sender, RoutedEventArgs e)
        {
            Control contentAsC = scrollerView.Content as Control;
            if (contentAsC != null)
                contentAsC.IsEnabled = false;
        }

        private void ChkIsContentTabStop_Checked(object sender, RoutedEventArgs e)
        {
            Control contentAsC = scrollerView.Content as Control;
            if (contentAsC != null)
                contentAsC.IsTabStop = true;
        }

        private void ChkIsContentTabStop_Unchecked(object sender, RoutedEventArgs e)
        {
            Control contentAsC = scrollerView.Content as Control;
            if (contentAsC != null)
                contentAsC.IsTabStop = false;
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

        private void UpdateWidth()
        {
            txtWidth.Text = scrollerView.Width.ToString();
        }

        private void UpdateHeight()
        {
            txtHeight.Text = scrollerView.Height.ToString();
        }

        private void UpdatePadding()
        {
            txtPadding.Text = scrollerView.Padding.ToString();
        }

        private void UpdateHorizontalScrollControllerVisibility()
        {
            cmbHorizontalScrollControllerVisibility.SelectedIndex = (int)scrollerView.HorizontalScrollControllerVisibility;
        }

        private void UpdateVerticalScrollControllerVisibility()
        {
            cmbVerticalScrollControllerVisibility.SelectedIndex = (int)scrollerView.VerticalScrollControllerVisibility;
        }

        private void UpdateContentWidth()
        {
            FrameworkElement contentAsFE = scrollerView.Content as FrameworkElement;
            if (contentAsFE == null)
                txtContentWidth.Text = string.Empty;
            else
                txtContentWidth.Text = contentAsFE.Width.ToString();
        }

        private void UpdateContentHeight()
        {
            FrameworkElement contentAsFE = scrollerView.Content as FrameworkElement;
            if (contentAsFE == null)
                txtContentHeight.Text = string.Empty;
            else
                txtContentHeight.Text = contentAsFE.Height.ToString();
        }

        private void UpdateContentMargin()
        {
            FrameworkElement contentAsFE = scrollerView.Content as FrameworkElement;
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

        private void UseScrollerView(ScrollerView sv2)
        {
            if (scrollerView == sv2 || sv2 == null)
                return;

            try
            {
                if (scrollerView == null && (chkLogScrollerViewMessages.IsChecked == true || chkLogScrollerMessages.IsChecked == true))
                {
                    MUXControlsTestHooks.LoggingMessage += MUXControlsTestHooks_LoggingMessage;

                    if (chkLogScrollerMessages.IsChecked == true)
                    {
                        MUXControlsTestHooks.SetLoggingLevelForType("Scroller", isLoggingInfoLevel: true, isLoggingVerboseLevel: true);
                    }
                    if (chkLogScrollerViewMessages.IsChecked == true)
                    {
                        MUXControlsTestHooks.SetLoggingLevelForType("ScrollerView", isLoggingInfoLevel: true, isLoggingVerboseLevel: true);
                    }
                }

                scrollerView = sv2;

                UpdateIsChildAvailableWidthConstrained();
                UpdateIsChildAvailableHeightConstrained();
                UpdateHorizontalScrollMode();
                UpdateVerticalScrollMode();
                UpdateZoomMode();

                UpdateWidth();
                UpdateHeight();
                UpdatePadding();
                UpdateHorizontalScrollControllerVisibility();
                UpdateVerticalScrollControllerVisibility();

                chkIsEnabled.IsChecked = scrollerView.IsEnabled;
                chkIsTabStop.IsChecked = scrollerView.IsTabStop;

                UpdateContentWidth();
                UpdateContentHeight();
                UpdateContentMargin();

                Control contentAsC = scrollerView.Content as Control;
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

        private void ChkLogScrollerMessages_Checked(object sender, RoutedEventArgs e)
        {
            MUXControlsTestHooks.SetLoggingLevelForType("Scroller", isLoggingInfoLevel: true, isLoggingVerboseLevel: true);
            if (chkLogScrollerViewMessages.IsChecked == false)
                MUXControlsTestHooks.LoggingMessage += MUXControlsTestHooks_LoggingMessage;
        }

        private void ChkLogScrollerMessages_Unchecked(object sender, RoutedEventArgs e)
        {
            MUXControlsTestHooks.SetLoggingLevelForType("Scroller", isLoggingInfoLevel: false, isLoggingVerboseLevel: false);
            if (chkLogScrollerViewMessages.IsChecked == false)
                MUXControlsTestHooks.LoggingMessage -= MUXControlsTestHooks_LoggingMessage;
        }

        private void ChkLogScrollerViewMessages_Checked(object sender, RoutedEventArgs e)
        {
            MUXControlsTestHooks.SetLoggingLevelForType("ScrollerView", isLoggingInfoLevel: true, isLoggingVerboseLevel: true);
            if (chkLogScrollerMessages.IsChecked == false)
                MUXControlsTestHooks.LoggingMessage += MUXControlsTestHooks_LoggingMessage;
        }

        private void ChkLogScrollerViewMessages_Unchecked(object sender, RoutedEventArgs e)
        {
            MUXControlsTestHooks.SetLoggingLevelForType("ScrollerView", isLoggingInfoLevel: false, isLoggingVerboseLevel: false);
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
