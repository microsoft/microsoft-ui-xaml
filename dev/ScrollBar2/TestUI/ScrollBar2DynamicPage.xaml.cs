// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using Windows.Foundation;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;

using IScrollController = Microsoft.UI.Xaml.Controls.Primitives.IScrollController;
using ScrollMode = Microsoft.UI.Xaml.Controls.ScrollMode;
using ScrollerViewChangeResult = Microsoft.UI.Xaml.Controls.ScrollerViewChangeResult;
using ScrollControllerInteractionRequestedEventArgs = Microsoft.UI.Xaml.Controls.Primitives.ScrollControllerInteractionRequestedEventArgs;
using ScrollControllerOffsetChangeRequestedEventArgs = Microsoft.UI.Xaml.Controls.Primitives.ScrollControllerOffsetChangeRequestedEventArgs;
using ScrollControllerOffsetChangeWithAdditionalVelocityRequestedEventArgs = Microsoft.UI.Xaml.Controls.Primitives.ScrollControllerOffsetChangeWithAdditionalVelocityRequestedEventArgs;
using ScrollBar2 = Microsoft.UI.Xaml.Controls.ScrollBar2;
using MUXControlsTestHooks = Microsoft.UI.Private.Controls.MUXControlsTestHooks;
using MUXControlsTestHooksLoggingMessageEventArgs = Microsoft.UI.Private.Controls.MUXControlsTestHooksLoggingMessageEventArgs;

namespace MUXControlsTestApp
{
    public sealed partial class ScrollBar2DynamicPage : TestPage
    {
        private Object asyncEventReportingLock = new Object();
        private Object asyncOperationLock = new Object();
        private List<string> lstAsyncEventMessage = new List<string>();
        private ScrollBar2 scrollBar2 = null;
        private bool provideViewChangeIds = false;
        private int viewChangeId = 0;
        private List<int> viewChangeIds = new List<int>();

        public ScrollBar2DynamicPage()
        {
            this.InitializeComponent();

            UseScrollBar2(markupScrollBar2);
        }

        private void ChkLogScrollBar2Messages_Checked(object sender, RoutedEventArgs e)
        {
            //To turn on info and verbose logging for the ScrollBar2 type:
            MUXControlsTestHooks.SetLoggingLevelForType("ScrollBar2", isLoggingInfoLevel: true, isLoggingVerboseLevel: true);

            MUXControlsTestHooks.LoggingMessage += MUXControlsTestHooks_LoggingMessage;
        }

        private void ChkLogScrollBar2Messages_Unchecked(object sender, RoutedEventArgs e)
        {
            //To turn off info and verbose logging for the ScrollBar2 type:
            MUXControlsTestHooks.SetLoggingLevelForType("ScrollBar2", isLoggingInfoLevel: false, isLoggingVerboseLevel: false);

            MUXControlsTestHooks.LoggingMessage -= MUXControlsTestHooks_LoggingMessage;
        }

        private void ChkLogScrollBar2Events_Checked(object sender, RoutedEventArgs e)
        {
            HookIScrollControllerEvents();
        }

        private void ChkLogScrollBar2Events_Unchecked(object sender, RoutedEventArgs e)
        {
            UnhookIScrollControllerEvents();
        }

        private void HookIScrollControllerEvents()
        {
            IScrollController sc = scrollBar2 as IScrollController;

            sc.InteractionInfoChanged += IScrollController_InteractionInfoChanged;
            sc.InteractionRequested += IScrollController_InteractionRequested;
            sc.OffsetChangeRequested += IScrollController_OffsetChangeRequested;
            sc.OffsetChangeWithAdditionalVelocityRequested += IScrollController_OffsetChangeWithAdditionalVelocityRequested;
        }

        private void UnhookIScrollControllerEvents()
        {
            IScrollController sc = scrollBar2 as IScrollController;

            sc.InteractionInfoChanged -= IScrollController_InteractionInfoChanged;
            sc.InteractionRequested -= IScrollController_InteractionRequested;
            sc.OffsetChangeRequested -= IScrollController_OffsetChangeRequested;
            sc.OffsetChangeWithAdditionalVelocityRequested -= IScrollController_OffsetChangeWithAdditionalVelocityRequested;
        }

        private void ChkProvideViewChangeIds_Checked(object sender, RoutedEventArgs e)
        {
            provideViewChangeIds = true;
        }

        private void ChkProvideViewChangeIds_Unchecked(object sender, RoutedEventArgs e)
        {
            provideViewChangeIds = false;
        }

        private void MUXControlsTestHooks_LoggingMessage(object sender, MUXControlsTestHooksLoggingMessageEventArgs args)
        {
            // Cut off the terminating new line.
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

        private void ChkScrollBar2Properties_Checked(object sender, RoutedEventArgs e)
        {
            if (grdScrollBar2Properties != null)
                grdScrollBar2Properties.Visibility = Visibility.Visible;
        }

        private void ChkScrollBar2Properties_Unchecked(object sender, RoutedEventArgs e)
        {
            if (grdScrollBar2Properties != null)
                grdScrollBar2Properties.Visibility = Visibility.Collapsed;
        }

        private void ChkScrollBar2Methods_Checked(object sender, RoutedEventArgs e)
        {
            if (grdScrollBar2Methods != null)
                grdScrollBar2Methods.Visibility = Visibility.Visible;
        }

        private void ChkScrollBar2Methods_Unchecked(object sender, RoutedEventArgs e)
        {
            if (grdScrollBar2Methods != null)
                grdScrollBar2Methods.Visibility = Visibility.Collapsed;
        }

        private void ChkScrollBar2Events_Checked(object sender, RoutedEventArgs e)
        {
            if (grdScrollBar2Events != null)
                grdScrollBar2Events.Visibility = Visibility.Visible;
        }

        private void ChkScrollBar2Events_Unchecked(object sender, RoutedEventArgs e)
        {
            if (grdScrollBar2Events != null)
                grdScrollBar2Events.Visibility = Visibility.Collapsed;
        }

        private void BtnGetIsEnabled_Click(object sender, RoutedEventArgs e)
        {
            UpdateCmbIsEnabled();
        }

        private void BtnSetIsEnabled_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                scrollBar2.IsEnabled = cmbIsEnabled.SelectedIndex == 0;
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollBar2Events.Items.Add(ex.ToString());
            }
        }

        private void BtnGetOrientation_Click(object sender, RoutedEventArgs e)
        {
            UpdateCmbOrientation();
        }

        private void BtnSetOrientation_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (cmbOrientation.SelectedIndex == 0 && scrollBar2.Orientation == Orientation.Vertical)
                {
                    scrollBar2.Orientation = Orientation.Horizontal;
                    Grid.SetColumnSpan(scrollBar2, 4);
                    Grid.SetRowSpan(scrollBar2, 1);
                    double width = scrollBar2.Width;
                    scrollBar2.Width = scrollBar2.Height;
                    scrollBar2.Height = width;
                }
                else if (scrollBar2.Orientation == Orientation.Horizontal)
                {
                    scrollBar2.Orientation = Orientation.Vertical;
                    Grid.SetColumnSpan(scrollBar2, 1);
                    Grid.SetRowSpan(scrollBar2, 2);
                    double width = scrollBar2.Width;
                    scrollBar2.Width = scrollBar2.Height;
                    scrollBar2.Height = width;
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollBar2Events.Items.Add(ex.ToString());
            }
        }

        private void BtnGetScrollMode_Click(object sender, RoutedEventArgs e)
        {
            UpdateCmbScrollMode();
        }

        private void BtnSetScrollMode_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                scrollBar2.ScrollMode = (ScrollMode)cmbScrollMode.SelectedIndex;
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollBar2Events.Items.Add(ex.ToString());
            }
        }

        private void BtnGetIndicatorMode_Click(object sender, RoutedEventArgs e)
        {
            UpdateCmbIndicatorMode();
        }

        private void BtnSetIndicatorMode_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                scrollBar2.IndicatorMode = (ScrollingIndicatorMode)cmbIndicatorMode.SelectedIndex;
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollBar2Events.Items.Add(ex.ToString());
            }
        }

        private void UpdateCmbIsEnabled()
        {
            try
            {
                cmbIsEnabled.SelectedIndex = scrollBar2.IsEnabled ? 0 : 1;
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollBar2Events.Items.Add(ex.ToString());
            }
        }

        private void UpdateCmbOrientation()
        {
            try
            {
                cmbOrientation.SelectedIndex = scrollBar2.Orientation == Orientation.Horizontal ? 0 : 1;
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollBar2Events.Items.Add(ex.ToString());
            }
        }

        private void UpdateCmbScrollMode()
        {
            try
            {
                cmbScrollMode.SelectedIndex = (int)scrollBar2.ScrollMode;
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollBar2Events.Items.Add(ex.ToString());
            }
        }

        private void UpdateCmbIndicatorMode()
        {
            try
            {
                cmbIndicatorMode.SelectedIndex = (int)scrollBar2.IndicatorMode;
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollBar2Events.Items.Add(ex.ToString());
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
                ((FrameworkElement)scrollBar2).Margin = GetThicknessFromString(txtMargin.Text);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollBar2Events.Items.Add(ex.ToString());
            }
        }

        private void UpdateMargin()
        {
            txtMargin.Text = ((FrameworkElement)scrollBar2).Margin.ToString();
        }

        private void UpdateMinOffset()
        {
            lblMinOffset.Text = scrollBar2.MinOffset.ToString();
        }

        private void UpdateMaxOffset()
        {
            lblMaxOffset.Text = scrollBar2.MaxOffset.ToString();
        }

        private void UpdateOffset()
        {
            lblOffset.Text = scrollBar2.Offset.ToString();
        }

        private void UpdateViewport()
        {
            lblViewport.Text = scrollBar2.Viewport.ToString();
        }

        private void UpdateSmallChange()
        {
            txtSmallChange.Text = scrollBar2.SmallChange.ToString();
        }

        private void UpdateLargeChange()
        {
            txtLargeChange.Text = scrollBar2.LargeChange.ToString();
        }

        private void UpdateWidth()
        {
            txtWidth.Text = scrollBar2.Width.ToString();
        }

        private void UpdateHeight()
        {
            txtHeight.Text = scrollBar2.Height.ToString();
        }

        private void UpdateStyle()
        {
            cmbStyle.SelectedIndex = scrollBar2.ScrollBarStyle == null ? 0 : 1;
        }

        private void BtnGetMinOffset_Click(object sender, RoutedEventArgs e)
        {
            UpdateMinOffset();
        }

        private void BtnGetMaxOffset_Click(object sender, RoutedEventArgs e)
        {
            UpdateMaxOffset();
        }

        private void BtnGetOffset_Click(object sender, RoutedEventArgs e)
        {
            UpdateOffset();
        }

        private void BtnGetViewport_Click(object sender, RoutedEventArgs e)
        {
            UpdateViewport();
        }

        private void BtnGetStyle_Click(object sender, RoutedEventArgs e)
        {
            UpdateStyle();
        }

        private void BtnSetStyle_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                Style hairStyle = cmbStyle.SelectedIndex == 0 ? null : Resources["hairStyle"] as Style;

                scrollBar2.ScrollBarStyle = hairStyle;
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollBar2Events.Items.Add(ex.ToString());
            }
        }

        private void BtnGetSmallChange_Click(object sender, RoutedEventArgs e)
        {
            UpdateSmallChange();
        }

        private void BtnSetSmallChange_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                scrollBar2.SmallChange = Convert.ToDouble(txtSmallChange.Text);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollBar2Events.Items.Add(ex.ToString());
            }
        }

        private void BtnGetLargeChange_Click(object sender, RoutedEventArgs e)
        {
            UpdateLargeChange();
        }

        private void BtnSetLargeChange_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                scrollBar2.LargeChange = Convert.ToDouble(txtLargeChange.Text);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollBar2Events.Items.Add(ex.ToString());
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
                scrollBar2.Width = Convert.ToDouble(txtWidth.Text);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollBar2Events.Items.Add(ex.ToString());
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
                scrollBar2.Height = Convert.ToDouble(txtHeight.Text);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollBar2Events.Items.Add(ex.ToString());
            }
        }

        private void BtnClearScrollBar2Events_Click(object sender, RoutedEventArgs e)
        {
            lstScrollBar2Events.Items.Clear();
        }

        private void BtnSetValues_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                IScrollController sc = scrollBar2 as IScrollController;

                sc.SetValues(
                    Convert.ToDouble(txtMinOffset.Text),
                    Convert.ToDouble(txtMaxOffset.Text),
                    Convert.ToDouble(txtOffset.Text),
                    Convert.ToDouble(txtViewport.Text));
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollBar2Events.Items.Add(ex.ToString());
            }
        }

        private void IScrollController_InteractionInfoChanged(IScrollController sender, object args)
        {
            try
            {
                AppendAsyncEventMessage("InteractionInfoChanged");
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollBar2Events.Items.Add(ex.ToString());
            }
        }

        private void IScrollController_InteractionRequested(IScrollController sender, ScrollControllerInteractionRequestedEventArgs args)
        {
            try
            {
                AppendAsyncEventMessage("InteractionRequested PointerPoint=" + args.PointerPoint.ToString());
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollBar2Events.Items.Add(ex.ToString());
            }
        }

        private void IScrollController_OffsetChangeRequested(IScrollController sender, ScrollControllerOffsetChangeRequestedEventArgs args)
        {
            try
            {
                if (provideViewChangeIds)
                {
                    viewChangeId++;
                    args.ViewChangeId = viewChangeId;
                    AppendAsyncEventMessage("OffsetChangeRequested Created ViewChangeId=" + args.ViewChangeId);

                    lock (asyncEventReportingLock)
                    {
                        viewChangeIds.Add(args.ViewChangeId);
                    }

                    var ignored = this.Dispatcher.RunAsync(Windows.UI.Core.CoreDispatcherPriority.Normal, ProcessAsyncOperation);
                }
                else
                {
                    AppendAsyncEventMessage("OffsetChangeRequested");
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollBar2Events.Items.Add(ex.ToString());
            }
        }

        private void IScrollController_OffsetChangeWithAdditionalVelocityRequested(IScrollController sender, ScrollControllerOffsetChangeWithAdditionalVelocityRequestedEventArgs args)
        {
            try
            {
                AppendAsyncEventMessage("OffsetChangeWithAdditionalVelocityRequested ViewChangeId=" + args.ViewChangeId);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollBar2Events.Items.Add(ex.ToString());
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
                    lstScrollBar2Events.Items.Add(asyncEventMessage);
                }
                lstAsyncEventMessage.Clear();
            }
        }

        private void ProcessAsyncOperation()
        {
            lock (asyncOperationLock)
            {
                foreach (int offsetChangeId in viewChangeIds)
                {
                    IScrollController sc = scrollBar2 as IScrollController;

                    sc.OnOffsetChangeCompleted(offsetChangeId, ScrollerViewChangeResult.Completed);
                }
                viewChangeIds.Clear();
            }
        }

        private void BtnClearExceptionReport_Click(object sender, RoutedEventArgs e)
        {
            txtExceptionReport.Text = string.Empty;
        }

        private void UseScrollBar2(ScrollBar2 sb2)
        {
            if (scrollBar2 == sb2 || sb2 == null)
                return;

            try
            {
                if (scrollBar2 != null)
                {
                    if (chkLogScrollBar2Messages.IsChecked == true)
                    {
                        MUXControlsTestHooks.SetLoggingLevelForType("Scroller", isLoggingInfoLevel: false, isLoggingVerboseLevel: false);
                        MUXControlsTestHooks.LoggingMessage -= MUXControlsTestHooks_LoggingMessage;
                    }

                    if (chkLogScrollBar2Events.IsChecked == true)
                    {
                        UnhookIScrollControllerEvents();
                    }
                }

                scrollBar2 = sb2;

                if (chkLogScrollBar2Messages.IsChecked == true)
                {
                    MUXControlsTestHooks.SetLoggingLevelForType("ScrollBar2", isLoggingInfoLevel: true, isLoggingVerboseLevel: true);
                    MUXControlsTestHooks.LoggingMessage += MUXControlsTestHooks_LoggingMessage;
                }

                UpdateCmbScrollMode();
                UpdateCmbIndicatorMode();
                UpdateCmbIsEnabled();
                UpdateCmbOrientation();
                UpdateMinOffset();
                UpdateMaxOffset();
                UpdateOffset();
                UpdateViewport();
                UpdateSmallChange();
                UpdateLargeChange();
                UpdateMargin();
                UpdateWidth();
                UpdateHeight();
                UpdateStyle();

                if (chkLogScrollBar2Events.IsChecked == true)
                {
                    HookIScrollControllerEvents();
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollBar2Events.Items.Add(ex.ToString());
            }
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
    }
}
