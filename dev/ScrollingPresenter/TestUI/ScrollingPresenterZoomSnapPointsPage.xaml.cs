// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Numerics;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Navigation;

using ZoomSnapPoint = Microsoft.UI.Xaml.Controls.Primitives.ZoomSnapPoint;
using RepeatedZoomSnapPoint = Microsoft.UI.Xaml.Controls.Primitives.RepeatedZoomSnapPoint;
using ScrollingPresenter = Microsoft.UI.Xaml.Controls.Primitives.ScrollingPresenter;
using AnimationMode = Microsoft.UI.Xaml.Controls.AnimationMode;
using SnapPointsMode = Microsoft.UI.Xaml.Controls.SnapPointsMode;
using ScrollingZoomOptions = Microsoft.UI.Xaml.Controls.ScrollingZoomOptions;

using MUXControlsTestHooks = Microsoft.UI.Private.Controls.MUXControlsTestHooks;
using MUXControlsTestHooksLoggingMessageEventArgs = Microsoft.UI.Private.Controls.MUXControlsTestHooksLoggingMessageEventArgs;

namespace MUXControlsTestApp
{
    public sealed partial class ScrollingPresenterZoomSnapPointsPage : TestPage
    {
        private List<string> fullLogs = new List<string>();

        public ScrollingPresenterZoomSnapPointsPage()
        {
            this.InitializeComponent();
            Loaded += ScrollingPresenterZoomSnapPointsPage_Loaded;
        }

        protected override void OnNavigatedFrom(NavigationEventArgs e)
        {
            if (chkLogScrollingPresenterMessages.IsChecked == true)
            {
                MUXControlsTestHooks.SetLoggingLevelForType("ScrollingPresenter", isLoggingInfoLevel: false, isLoggingVerboseLevel: false);
                MUXControlsTestHooks.LoggingMessage -= MUXControlsTestHooks_LoggingMessage;
            }

            base.OnNavigatedFrom(e);
        }

        private void ScrollingPresenterZoomSnapPointsPage_Loaded(object sender, RoutedEventArgs e)
        {
            this.markupScrollingPresenter.ViewChanged += MarkupScrollingPresenter_ViewChanged;
            this.markupScrollingPresenter.StateChanged += MarkupScrollingPresenter_StateChanged;
        }

        private void MarkupScrollingPresenter_ViewChanged(ScrollingPresenter sender, object args)
        {
            this.txtScrollingPresenterHorizontalOffset.Text = this.markupScrollingPresenter.HorizontalOffset.ToString();
            this.txtScrollingPresenterVerticalOffset.Text = this.markupScrollingPresenter.VerticalOffset.ToString();
            this.txtScrollingPresenterZoomFactor.Text = this.markupScrollingPresenter.ZoomFactor.ToString();
        }

        private void MarkupScrollingPresenter_StateChanged(ScrollingPresenter sender, object args)
        {
            this.txtScrollingPresenterState.Text = this.markupScrollingPresenter.State.ToString();
        }

        private void BtnMIAddSnapPoint_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                double value = Convert.ToDouble(txtMISnapPointValue.Text);
                ZoomSnapPoint newSnapPoint = new ZoomSnapPoint(value);
                markupScrollingPresenter.ZoomSnapPoints.Add(newSnapPoint);
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
                markupScrollingPresenter.ZoomSnapPoints.Add(newSnapPoint);
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
                markupScrollingPresenter.ZoomSnapPoints.Add(newSnapPoint);
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
                markupScrollingPresenter.ZoomSnapPoints.Add(newSnapPoint);
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
                markupScrollingPresenter.ZoomSnapPoints.RemoveAt(0);
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
                markupScrollingPresenter.ZoomSnapPoints.Clear();
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
            }
        }

        private void BtnScrollingPresenterZoomFactorChange_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                float changeAmount = Convert.ToSingle(txtScrollingPresenterZoomFactorChange.Text);
                markupScrollingPresenter.ZoomFrom(changeAmount + 0.05f, null, null);
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
                markupScrollingPresenter.ZoomBy(1.0f, null, new ScrollingZoomOptions(AnimationMode.Auto, SnapPointsMode.Default));
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
                markupScrollingPresenter.ZoomBy(1.0f, null, new ScrollingZoomOptions(AnimationMode.Auto, SnapPointsMode.Ignore));
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

        private void ChkLogScrollingPresenterMessages_Checked(object sender, RoutedEventArgs e)
        {
            //Turn on info and verbose logging for the ScrollingPresenter type:
            MUXControlsTestHooks.SetLoggingLevelForType("ScrollingPresenter", isLoggingInfoLevel: true, isLoggingVerboseLevel: true);

            MUXControlsTestHooks.LoggingMessage += MUXControlsTestHooks_LoggingMessage;
        }

        private void ChkLogScrollingPresenterMessages_Unchecked(object sender, RoutedEventArgs e)
        {
            //Turn off info and verbose logging for the ScrollingPresenter type:
            MUXControlsTestHooks.SetLoggingLevelForType("ScrollingPresenter", isLoggingInfoLevel: false, isLoggingVerboseLevel: false);

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

