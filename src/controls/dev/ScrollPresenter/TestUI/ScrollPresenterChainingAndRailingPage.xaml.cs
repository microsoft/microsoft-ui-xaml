﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Navigation;
using Microsoft.UI.Private.Controls;

namespace MUXControlsTestApp
{
    public sealed partial class ScrollPresenterChainingAndRailingPage : TestPage
    {
        private List<string> fullLogs = new List<string>();
        private int scrollPresenter3ZoomFactorChangeCorrelationId = -1;

        public ScrollPresenterChainingAndRailingPage()
        {
            InitializeComponent();
            InitializeUIFromScrollPresenter("0");
            InitializeUIFromScrollPresenter("1");
            InitializeUIFromScrollPresenter("3");
            InitializeUIFromScrollViewer("2");

            scrollPresenter0.StateChanged += ScrollPresenter_StateChanged;
            scrollPresenter1.StateChanged += ScrollPresenter_StateChanged;
            scrollPresenter3.StateChanged += ScrollPresenter_StateChanged;

            scrollPresenter3.ZoomCompleted += ScrollPresenter_ZoomCompleted;

            scrollViewer2.DirectManipulationStarted += ScrollViewer2_DirectManipulationStarted;
            scrollViewer2.DirectManipulationCompleted += ScrollViewer2_DirectManipulationCompleted;
        }

        ~ScrollPresenterChainingAndRailingPage()
        {
        }

        protected override void OnNavigatedFrom(NavigationEventArgs e)
        {
            MUXControlsTestHooks.SetLoggingLevelForType("ScrollPresenter", isLoggingInfoLevel: false, isLoggingVerboseLevel: false);
            MUXControlsTestHooks.LoggingMessage -= MUXControlsTestHooks_LoggingMessage;

            base.OnNavigatedFrom(e);
        }

        private void ScrollViewer2_DirectManipulationStarted(object sender, object e)
        {
            txtScrollViewer2State.Text = (sender as ScrollViewer).Name + " Interaction";
            txtScrollViewer2HSP.Text = String.Format("{0,5:N1}%", (sender as ScrollViewer).HorizontalOffset / (sender as ScrollViewer).ScrollableWidth * 100.0);
            txtScrollViewer2VSP.Text = String.Format("{0,5:N1}%", (sender as ScrollViewer).VerticalOffset / (sender as ScrollViewer).ScrollableHeight * 100.0);
        }

        private void ScrollViewer2_DirectManipulationCompleted(object sender, object e)
        {
            txtScrollViewer2State.Text = (sender as ScrollViewer).Name + " Idle";
            txtScrollViewer2HSP.Text = String.Format("{0,5:N1}%", (sender as ScrollViewer).HorizontalOffset / (sender as ScrollViewer).ScrollableWidth * 100.0);
            txtScrollViewer2VSP.Text = String.Format("{0,5:N1}%", (sender as ScrollViewer).VerticalOffset / (sender as ScrollViewer).ScrollableHeight * 100.0);
        }

        private void ScrollPresenter_StateChanged(ScrollPresenter sender, object args)
        {
            if (sender == scrollPresenter0)
            {
                txtScrollPresenter0State.Text = "scrollPresenter0 " + scrollPresenter0.State.ToString();
                txtScrollPresenter0HSP.Text = String.Format("{0,5:N1}%", sender.HorizontalOffset / 3.0);
                txtScrollPresenter0VSP.Text = String.Format("{0,5:N1}%", sender.VerticalOffset / 3.0);
            }
            else if (sender == scrollPresenter1)
            {
                txtScrollPresenter1State.Text = "scrollPresenter1 " + scrollPresenter1.State.ToString();
                txtScrollPresenter1HSP.Text = String.Format("{0,5:N1}%", sender.HorizontalOffset / 3.0);
                txtScrollPresenter1VSP.Text = String.Format("{0,5:N1}%", sender.VerticalOffset / 3.0);
            }
            else if (sender == scrollPresenter3)
            {
                txtScrollPresenter3State.Text = "scrollPresenter3 " + scrollPresenter3.State.ToString();
                txtScrollPresenter3HSP.Text = String.Format("{0,5:N1}%", sender.HorizontalOffset / 3.0);
                txtScrollPresenter3VSP.Text = String.Format("{0,5:N1}%", sender.VerticalOffset / 3.0);
            }
        }

        private void ScrollPresenter_ZoomCompleted(ScrollPresenter sender, ScrollingZoomCompletedEventArgs args)
        {
            if (args.CorrelationId == scrollPresenter3ZoomFactorChangeCorrelationId)
                this.txtResetStatus.Text = "Views reset";
        }

        private void CmbHorizontalScrollChainMode_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            ComboBox cmb = sender as ComboBox;
            string suffix = cmb.Name.Substring(cmb.Name.Length - 1);
            ScrollPresenter scrollPresenter = GetScrollPresenterWithSuffix(suffix);
            if (scrollPresenter != null)
            {
                scrollPresenter.HorizontalScrollChainMode = (ScrollingChainMode)cmb.SelectedIndex;
            }
            else
            {
                ScrollViewer scrollViewer = GetScrollViewerWithSuffix(suffix);
                if (scrollViewer != null)
                {
                    scrollViewer.IsHorizontalScrollChainingEnabled = cmb.SelectedIndex == 0;
                }
            }
        }

        private void CmbVerticalScrollChainMode_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            ComboBox cmb = sender as ComboBox;
            string suffix = cmb.Name.Substring(cmb.Name.Length - 1);
            ScrollPresenter scrollPresenter = GetScrollPresenterWithSuffix(suffix);
            if (scrollPresenter != null)
            {
                scrollPresenter.VerticalScrollChainMode = (ScrollingChainMode)cmb.SelectedIndex;
            }
            else
            {
                ScrollViewer scrollViewer = GetScrollViewerWithSuffix(suffix);
                if (scrollViewer != null)
                {
                    scrollViewer.IsVerticalScrollChainingEnabled = cmb.SelectedIndex == 0;
                }
            }
        }

        private void CmbZoomChainMode_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            ComboBox cmb = sender as ComboBox;
            string suffix = cmb.Name.Substring(cmb.Name.Length - 1);
            ScrollPresenter scrollPresenter = GetScrollPresenterWithSuffix(suffix);
            if (scrollPresenter != null)
            {
                scrollPresenter.ZoomChainMode = (ScrollingChainMode)cmb.SelectedIndex;
            }
            else
            {
                ScrollViewer scrollViewer = GetScrollViewerWithSuffix(suffix);
                if (scrollViewer != null)
                {
                    scrollViewer.IsZoomChainingEnabled = cmb.SelectedIndex == 0;
                }
            }
        }

        private void CmbHorizontalScrollRailMode_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            ComboBox cmb = sender as ComboBox;
            string suffix = cmb.Name.Substring(cmb.Name.Length - 1);
            ScrollPresenter scrollPresenter = GetScrollPresenterWithSuffix(suffix);
            if (scrollPresenter != null)
            {
                scrollPresenter.HorizontalScrollRailMode = (ScrollingRailMode)cmb.SelectedIndex;
            }
            else
            {
                ScrollViewer scrollViewer = GetScrollViewerWithSuffix(suffix);
                if (scrollViewer != null)
                {
                    scrollViewer.IsHorizontalRailEnabled = cmb.SelectedIndex == 0;
                }
            }
        }

        private void CmbVerticalScrollRailMode_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            ComboBox cmb = sender as ComboBox;
            string suffix = cmb.Name.Substring(cmb.Name.Length - 1);
            ScrollPresenter scrollPresenter = GetScrollPresenterWithSuffix(suffix);
            if (scrollPresenter != null)
            {
                scrollPresenter.VerticalScrollRailMode = (ScrollingRailMode)cmb.SelectedIndex;
            }
            else
            {
                ScrollViewer scrollViewer = GetScrollViewerWithSuffix(suffix);
                if (scrollViewer != null)
                {
                    scrollViewer.IsVerticalRailEnabled = cmb.SelectedIndex == 0;
                }
            }
        }

        private ScrollPresenter GetScrollPresenterWithSuffix(string suffix)
        {
            switch (suffix)
            {
                case "0":
                    return scrollPresenter0;
                case "1":
                    return scrollPresenter1;
                case "3":
                    return scrollPresenter3;
                default:
                    return null;
            }
        }

        private ScrollViewer GetScrollViewerWithSuffix(string suffix)
        {
            switch (suffix)
            {
                case "2":
                    return scrollViewer2;
                default:
                    return null;
            }
        }

        private void InitializeUIFromScrollPresenter(string suffix)
        {
            ScrollPresenter scrollPresenter = FindName("scrollPresenter" + suffix) as ScrollPresenter;
            if (scrollPresenter == null)
                return;

            ComboBox cmb = FindName("cmbHorizontalScrollChainMode" + suffix) as ComboBox;
            if (cmb != null)
                cmb.SelectedIndex = (int)scrollPresenter.HorizontalScrollChainMode;

            cmb = FindName("cmbVerticalScrollChainMode" + suffix) as ComboBox;
            if (cmb != null)
                cmb.SelectedIndex = (int)scrollPresenter.VerticalScrollChainMode;

            cmb = FindName("cmbZoomChainMode" + suffix) as ComboBox;
            if (cmb != null)
                cmb.SelectedIndex = (int)scrollPresenter.ZoomChainMode;

            cmb = FindName("cmbHorizontalScrollRailMode" + suffix) as ComboBox;
            if (cmb != null)
                cmb.SelectedIndex = (int)scrollPresenter.HorizontalScrollRailMode;

            cmb = FindName("cmbVerticalScrollRailMode" + suffix) as ComboBox;
            if (cmb != null)
                cmb.SelectedIndex = (int)scrollPresenter.VerticalScrollRailMode;
        }

        private void InitializeUIFromScrollViewer(string suffix)
        {
            ScrollViewer scrollViewer = FindName("scrollViewer" + suffix) as ScrollViewer;
            if (scrollViewer == null)
                return;

            ComboBox cmb = FindName("cmbHorizontalScrollChainMode" + suffix) as ComboBox;
            if (cmb != null)
                cmb.SelectedIndex = scrollViewer.IsHorizontalScrollChainingEnabled ? 0 : 1;

            cmb = FindName("cmbVerticalScrollChainMode" + suffix) as ComboBox;
            if (cmb != null)
                cmb.SelectedIndex = scrollViewer.IsVerticalScrollChainingEnabled ? 0 : 1;

            cmb = FindName("cmbZoomChainMode" + suffix) as ComboBox;
            if (cmb != null)
                cmb.SelectedIndex = scrollViewer.IsZoomChainingEnabled ? 0 : 1;

            cmb = FindName("cmbHorizontalScrollRailMode" + suffix) as ComboBox;
            if (cmb != null)
                cmb.SelectedIndex = scrollViewer.IsHorizontalRailEnabled ? 0 : 1;

            cmb = FindName("cmbVerticalScrollRailMode" + suffix) as ComboBox;
            if (cmb != null)
                cmb.SelectedIndex = scrollViewer.IsVerticalRailEnabled ? 0 : 1;
        }

        private void btnResetViews_Click(object sender, RoutedEventArgs e)
        {
            this.txtResetStatus.Text = "Resetting views ...";
            ResetView(this.scrollViewer2);
            ResetView(this.scrollPresenter0);
            ResetView(this.scrollPresenter1);
            ResetView(this.scrollPresenter3);
        }

        private void ResetView(UIElement element)
        {
            ScrollPresenter scrollPresenter = element as ScrollPresenter;

            if (scrollPresenter != null)
            {
                scrollPresenter.ScrollTo(
                    0.0,
                    0.0,
                    new ScrollingScrollOptions(ScrollingAnimationMode.Disabled, ScrollingSnapPointsMode.Ignore));

                int viewChangeCorrelationId = scrollPresenter.ZoomTo(
                    1.0f,
                    System.Numerics.Vector2.Zero,
                    new ScrollingZoomOptions(ScrollingAnimationMode.Disabled, ScrollingSnapPointsMode.Ignore));

                if (this.scrollPresenter3 == scrollPresenter)
                    scrollPresenter3ZoomFactorChangeCorrelationId = viewChangeCorrelationId;
            }
            else
            {
                ScrollViewer scrollViewer = element as ScrollViewer;
                scrollViewer.ChangeView(0, 0, 1.0f, true /*disableAnimation*/);
            }
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
