// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Numerics;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Navigation;

using ZoomSnapPoint = Microsoft.UI.Xaml.Controls.Primitives.ZoomSnapPoint;
using RepeatedZoomSnapPoint = Microsoft.UI.Xaml.Controls.Primitives.RepeatedZoomSnapPoint;
using ScrollPresenter = Microsoft.UI.Xaml.Controls.Primitives.ScrollPresenter;
using AnimationMode = Microsoft.UI.Xaml.Controls.AnimationMode;
using SnapPointsMode = Microsoft.UI.Xaml.Controls.SnapPointsMode;
using ScrollingZoomOptions = Microsoft.UI.Xaml.Controls.ScrollingZoomOptions;

using MUXControlsTestHooks = Microsoft.UI.Private.Controls.MUXControlsTestHooks;
using MUXControlsTestHooksLoggingMessageEventArgs = Microsoft.UI.Private.Controls.MUXControlsTestHooksLoggingMessageEventArgs;

namespace MUXControlsTestApp
{
    public sealed partial class ScrollPresenterZoomSnapPointsPage : TestPage
    {
        private List<string> fullLogs = new List<string>();

        public ScrollPresenterZoomSnapPointsPage()
        {
            this.InitializeComponent();
            Loaded += ScrollPresenterZoomSnapPointsPage_Loaded;
        }

        protected override void OnNavigatedFrom(NavigationEventArgs e)
        {
            if (chkLogScrollPresenterMessages.IsChecked == true)
            {
                MUXControlsTestHooks.SetLoggingLevelForType("ScrollPresenter", isLoggingInfoLevel: false, isLoggingVerboseLevel: false);
                MUXControlsTestHooks.LoggingMessage -= MUXControlsTestHooks_LoggingMessage;
            }

            base.OnNavigatedFrom(e);
        }

        private void ScrollPresenterZoomSnapPointsPage_Loaded(object sender, RoutedEventArgs e)
        {
            this.markupScrollPresenter.ViewChanged += MarkupScrollPresenter_ViewChanged;
            this.markupScrollPresenter.StateChanged += MarkupScrollPresenter_StateChanged;
        }

        private void MarkupScrollPresenter_ViewChanged(ScrollPresenter sender, object args)
        {
            this.txtScrollPresenterHorizontalOffset.Text = this.markupScrollPresenter.HorizontalOffset.ToString();
            this.txtScrollPresenterVerticalOffset.Text = this.markupScrollPresenter.VerticalOffset.ToString();
            this.txtScrollPresenterZoomFactor.Text = this.markupScrollPresenter.ZoomFactor.ToString();
        }

        private void MarkupScrollPresenter_StateChanged(ScrollPresenter sender, object args)
        {
            this.txtScrollPresenterState.Text = this.markupScrollPresenter.State.ToString();
        }

        private void BtnMIAddSnapPoint_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                double value = Convert.ToDouble(txtMISnapPointValue.Text);
                ZoomSnapPoint newSnapPoint = new ZoomSnapPoint(value);
                markupScrollPresenter.ZoomSnapPoints.Add(newSnapPoint);
            }
            catch (Exception ex)
            {
                this.fullLogs.Add(ex.ToString());
                txtExceptionReport.Text = ex.ToString();
            }
        }

#if ApplicableRangeType
        private void BtnOIAddSnapPoint_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                double value = Convert.ToDouble(txtOISnapPointValue.Text);
                double range = Convert.ToDouble(txtOIApplicableRange.Text);
                ZoomSnapPoint newSnapPoint = new ZoomSnapPoint(value, range);
                markupScrollPresenter.ZoomSnapPoints.Add(newSnapPoint);
            }
            catch (Exception ex)
            {
                this.fullLogs.Add(ex.ToString());
                txtExceptionReport.Text = ex.ToString();
            }
        }
#endif

        private void BtnMRAddSnapPoint_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                double offset = Convert.ToDouble(txtMRSnapPointOffset.Text);
                double interval = Convert.ToDouble(txtMRSnapPointInterval.Text);
                double start = Convert.ToDouble(txtMRSnapPointStart.Text);
                double end = Convert.ToDouble(txtMRSnapPointEnd.Text);
                RepeatedZoomSnapPoint newSnapPoint = new RepeatedZoomSnapPoint(offset, interval, start, end);
                markupScrollPresenter.ZoomSnapPoints.Add(newSnapPoint);
            }
            catch (Exception ex)
            {
                this.fullLogs.Add(ex.ToString());
                txtExceptionReport.Text = ex.ToString();
            }
        }

#if ApplicableRangeType
        private void BtnORAddSnapPoint_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                double offset = Convert.ToDouble(txtORSnapPointOffset.Text);
                double interval = Convert.ToDouble(txtORSnapPointInterval.Text);
                double range = Convert.ToDouble(txtORApplicableRange.Text);
                double start = Convert.ToDouble(txtORSnapPointStart.Text);
                double end = Convert.ToDouble(txtORSnapPointEnd.Text);

                RepeatedZoomSnapPoint newSnapPoint = new RepeatedZoomSnapPoint(offset, interval, start, end, range);
                markupScrollPresenter.ZoomSnapPoints.Add(newSnapPoint);
            }
            catch (Exception ex)
            {
                this.fullLogs.Add(ex.ToString());
                txtExceptionReport.Text = ex.ToString();
            }
        }
#endif

        private void BtnRemoveFirst_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                markupScrollPresenter.ZoomSnapPoints.RemoveAt(0);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
            }
        }

        private void BtnRemoveAll_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                markupScrollPresenter.ZoomSnapPoints.Clear();
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
            }
        }

        private void BtnScrollPresenterZoomFactorChange_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                float changeAmount = Convert.ToSingle(txtScrollPresenterZoomFactorChange.Text);
                markupScrollPresenter.ZoomFrom(changeAmount + 0.05f, null, null);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
            }
        }

        private void BtnZoomFactorPlus1With_Click(object sender, RoutedEventArgs e)
        {
            try
            { 
                markupScrollPresenter.ZoomBy(1.0f, null, new ScrollingZoomOptions(AnimationMode.Auto, SnapPointsMode.Default));
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
            }
        }

        private void BtnZoomFactorPlus1Without_Click(object sender, RoutedEventArgs e)
        {
            try
            {                
                markupScrollPresenter.ZoomBy(1.0f, null, new ScrollingZoomOptions(AnimationMode.Auto, SnapPointsMode.Ignore));
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
            }
        }

        private void BtnClearExceptionReport_Click(object sender, RoutedEventArgs e)
        {
            txtExceptionReport.Text = string.Empty;
        }

        private void ChkLogScrollPresenterMessages_Checked(object sender, RoutedEventArgs e)
        {
            //Turn on info and verbose logging for the ScrollPresenter type:
            MUXControlsTestHooks.SetLoggingLevelForType("ScrollPresenter", isLoggingInfoLevel: true, isLoggingVerboseLevel: true);

            MUXControlsTestHooks.LoggingMessage += MUXControlsTestHooks_LoggingMessage;
        }

        private void ChkLogScrollPresenterMessages_Unchecked(object sender, RoutedEventArgs e)
        {
            //Turn off info and verbose logging for the ScrollPresenter type:
            MUXControlsTestHooks.SetLoggingLevelForType("ScrollPresenter", isLoggingInfoLevel: false, isLoggingVerboseLevel: false);

            MUXControlsTestHooks.LoggingMessage -= MUXControlsTestHooks_LoggingMessage;
        }

        private void MUXControlsTestHooks_LoggingMessage(object sender, MUXControlsTestHooksLoggingMessageEventArgs args)
        {
            // Cut off the terminating new line.
            string msg = args.Message.Substring(0, args.Message.Length - 1);
            string senderName = string.Empty;
            FrameworkElement fe = sender as FrameworkElement;

            if (fe != null)
            {
                senderName = "s:" + fe.Name + ", ";
            }

            if (args.IsVerboseLevel)
            {
                this.fullLogs.Add("Verbose: " + senderName + "m:" + msg);
            }
            else
            {
                this.fullLogs.Add("Info: " + senderName + "m:" + msg);
            }
        }

        private void btnGetFullLog_Click(object sender, RoutedEventArgs e)
        {
            foreach (string log in this.fullLogs)
            {
                this.cmbFullLog.Items.Add(log);
            }
        }

        private void btnClearFullLog_Click(object sender, RoutedEventArgs e)
        {
            this.fullLogs.Clear();
            this.cmbFullLog.Items.Clear();
        }
    }
}

