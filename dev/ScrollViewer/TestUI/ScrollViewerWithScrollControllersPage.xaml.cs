// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;

using ScrollViewer = Microsoft.UI.Xaml.Controls.ScrollViewer;
using ContentOrientation = Microsoft.UI.Xaml.Controls.ContentOrientation;
using ScrollMode = Microsoft.UI.Xaml.Controls.ScrollMode;
using ZoomMode = Microsoft.UI.Xaml.Controls.ZoomMode;
using ScrollBarVisibility = Microsoft.UI.Xaml.Controls.ScrollBarVisibility;
using MUXControlsTestHooks = Microsoft.UI.Private.Controls.MUXControlsTestHooks;
using MUXControlsTestHooksLoggingMessageEventArgs = Microsoft.UI.Private.Controls.MUXControlsTestHooksLoggingMessageEventArgs;

namespace MUXControlsTestApp
{
    public sealed partial class ScrollViewerWithScrollControllersPage : TestPage
    {
        private Object asyncEventReportingLock = new Object();
        private List<string> lstAsyncEventMessage = new List<string>();
        private ScrollViewer scrollViewer = null;

        public ScrollViewerWithScrollControllersPage()
        {
            // We need the styles of the CompositionScrollController, so lets load it
            App.AppendResourceDictionaryToMergedDictionaries(App.AdditionStylesXaml);

            this.InitializeComponent();
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

        private void BtnGetHorizontalScrollBarVisibility_Click(object sender, RoutedEventArgs e)
        {
            UpdateHorizontalScrollBarVisibility();
        }

        private void BtnSetHorizontalScrollBarVisibility_Click(object sender, RoutedEventArgs e)
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

        private void BtnGetVerticalScrollBarVisibility_Click(object sender, RoutedEventArgs e)
        {
            UpdateVerticalScrollBarVisibility();
        }

        private void BtnSetVerticalScrollBarVisibility_Click(object sender, RoutedEventArgs e)
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
            UpdateContentWidth();
        }

        private void BtnSetContentWidth_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                FrameworkElement contentAsFE = scrollViewer.Content as FrameworkElement;
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
                FrameworkElement contentAsFE = scrollViewer.Content as FrameworkElement;
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
                FrameworkElement contentAsFE = scrollViewer.Content as FrameworkElement;
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
            Control contentAsC = scrollViewer.Content as Control;
            if (contentAsC != null)
                contentAsC.IsEnabled = true;
        }

        private void ChkIsContentEnabled_Unchecked(object sender, RoutedEventArgs e)
        {
            Control contentAsC = scrollViewer.Content as Control;
            if (contentAsC != null)
                contentAsC.IsEnabled = false;
        }

        private void ChkIsContentTabStop_Checked(object sender, RoutedEventArgs e)
        {
            Control contentAsC = scrollViewer.Content as Control;
            if (contentAsC != null)
                contentAsC.IsTabStop = true;
        }

        private void ChkIsContentTabStop_Unchecked(object sender, RoutedEventArgs e)
        {
            Control contentAsC = scrollViewer.Content as Control;
            if (contentAsC != null)
                contentAsC.IsTabStop = false;
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

        private void UpdateWidth()
        {
            txtWidth.Text = scrollViewer.Width.ToString();
        }

        private void UpdateHeight()
        {
            txtHeight.Text = scrollViewer.Height.ToString();
        }

        private void UpdatePadding()
        {
            txtPadding.Text = scrollViewer.Padding.ToString();
        }

        private void UpdateHorizontalScrollBarVisibility()
        {
            cmbHorizontalScrollBarVisibility.SelectedIndex = (int)scrollViewer.HorizontalScrollBarVisibility;
        }

        private void UpdateVerticalScrollBarVisibility()
        {
            cmbVerticalScrollBarVisibility.SelectedIndex = (int)scrollViewer.VerticalScrollBarVisibility;
        }

        private void UpdateContentWidth()
        {
            FrameworkElement contentAsFE = scrollViewer.Content as FrameworkElement;
            if (contentAsFE == null)
                txtContentWidth.Text = string.Empty;
            else
                txtContentWidth.Text = contentAsFE.Width.ToString();
        }

        private void UpdateContentHeight()
        {
            FrameworkElement contentAsFE = scrollViewer.Content as FrameworkElement;
            if (contentAsFE == null)
                txtContentHeight.Text = string.Empty;
            else
                txtContentHeight.Text = contentAsFE.Height.ToString();
        }

        private void UpdateContentMargin()
        {
            FrameworkElement contentAsFE = scrollViewer.Content as FrameworkElement;
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

                scrollViewer = sv2;

                UpdateContentOrientation();
                UpdateHorizontalScrollMode();
                UpdateVerticalScrollMode();
                UpdateZoomMode();

                UpdateWidth();
                UpdateHeight();
                UpdatePadding();
                UpdateHorizontalScrollBarVisibility();
                UpdateVerticalScrollBarVisibility();

                chkIsEnabled.IsChecked = scrollViewer.IsEnabled;
                chkIsTabStop.IsChecked = scrollViewer.IsTabStop;

                UpdateContentWidth();
                UpdateContentHeight();
                UpdateContentMargin();

                Control contentAsC = scrollViewer.Content as Control;
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
