// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;

using ScrollingPresenter = Microsoft.UI.Xaml.Controls.Primitives.ScrollingPresenter;
using ChainingMode = Microsoft.UI.Xaml.Controls.ChainingMode;
using RailingMode = Microsoft.UI.Xaml.Controls.RailingMode;
using AnimationMode = Microsoft.UI.Xaml.Controls.AnimationMode;
using SnapPointsMode = Microsoft.UI.Xaml.Controls.SnapPointsMode;
using ScrollingScrollOptions = Microsoft.UI.Xaml.Controls.ScrollingScrollOptions;
using ScrollingZoomOptions = Microsoft.UI.Xaml.Controls.ScrollingZoomOptions;
using ScrollingZoomCompletedEventArgs = Microsoft.UI.Xaml.Controls.ScrollingZoomCompletedEventArgs;
using MUXControlsTestHooks = Microsoft.UI.Private.Controls.MUXControlsTestHooks;
using MUXControlsTestHooksLoggingMessageEventArgs = Microsoft.UI.Private.Controls.MUXControlsTestHooksLoggingMessageEventArgs;

namespace MUXControlsTestApp
{
    public sealed partial class ScrollingPresenterChainingAndRailingPage : TestPage
    {
        private List<string> fullLogs = new List<string>();
        private int scrollingPresenter3ZoomFactorChangeId = -1;

        public ScrollingPresenterChainingAndRailingPage()
        {
            InitializeComponent();
            InitializeUIFromScrollingPresenter("0");
            InitializeUIFromScrollingPresenter("1");
            InitializeUIFromScrollingPresenter("3");
            InitializeUIFromScrollViewer("2");

            scrollingPresenter0.StateChanged += ScrollingPresenter_StateChanged;
            scrollingPresenter1.StateChanged += ScrollingPresenter_StateChanged;
            scrollingPresenter3.StateChanged += ScrollingPresenter_StateChanged;

            scrollingPresenter3.ZoomCompleted += ScrollingPresenter_ZoomCompleted;

            scrollViewer2.DirectManipulationStarted += ScrollViewer2_DirectManipulationStarted;
            scrollViewer2.DirectManipulationCompleted += ScrollViewer2_DirectManipulationCompleted;
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

        private void ScrollingPresenter_StateChanged(ScrollingPresenter sender, object args)
        {
            if (sender == scrollingPresenter0)
            {
                txtScrollingPresenter0State.Text = "scrollingPresenter0 " + scrollingPresenter0.State.ToString();
                txtScrollingPresenter0HSP.Text = String.Format("{0,5:N1}%", sender.HorizontalOffset / 3.0);
                txtScrollingPresenter0VSP.Text = String.Format("{0,5:N1}%", sender.VerticalOffset / 3.0);
            }
            else if (sender == scrollingPresenter1)
            {
                txtScrollingPresenter1State.Text = "scrollingPresenter1 " + scrollingPresenter1.State.ToString();
                txtScrollingPresenter1HSP.Text = String.Format("{0,5:N1}%", sender.HorizontalOffset / 3.0);
                txtScrollingPresenter1VSP.Text = String.Format("{0,5:N1}%", sender.VerticalOffset / 3.0);
            }
            else if (sender == scrollingPresenter3)
            {
                txtScrollingPresenter3State.Text = "scrollingPresenter3 " + scrollingPresenter3.State.ToString();
                txtScrollingPresenter3HSP.Text = String.Format("{0,5:N1}%", sender.HorizontalOffset / 3.0);
                txtScrollingPresenter3VSP.Text = String.Format("{0,5:N1}%", sender.VerticalOffset / 3.0);
            }
        }

        private void ScrollingPresenter_ZoomCompleted(ScrollingPresenter sender, ScrollingZoomCompletedEventArgs args)
        {
            if (args.ZoomInfo.ZoomFactorChangeId == scrollingPresenter3ZoomFactorChangeId)
                this.txtResetStatus.Text = "Views reset";
        }

        private void CmbHorizontalScrollChainingMode_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            ComboBox cmb = sender as ComboBox;
            string suffix = cmb.Name.Substring(cmb.Name.Length - 1);
            ScrollingPresenter scrollingPresenter = GetScrollingPresenterWithSuffix(suffix);
            if (scrollingPresenter != null)
            {
                scrollingPresenter.HorizontalScrollChainingMode = (ChainingMode)cmb.SelectedIndex;
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

        private void CmbVerticalScrollChainingMode_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            ComboBox cmb = sender as ComboBox;
            string suffix = cmb.Name.Substring(cmb.Name.Length - 1);
            ScrollingPresenter scrollingPresenter = GetScrollingPresenterWithSuffix(suffix);
            if (scrollingPresenter != null)
            {
                scrollingPresenter.VerticalScrollChainingMode = (ChainingMode)cmb.SelectedIndex;
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

        private void CmbZoomChainingMode_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            ComboBox cmb = sender as ComboBox;
            string suffix = cmb.Name.Substring(cmb.Name.Length - 1);
            ScrollingPresenter scrollingPresenter = GetScrollingPresenterWithSuffix(suffix);
            if (scrollingPresenter != null)
            {
                scrollingPresenter.ZoomChainingMode = (ChainingMode)cmb.SelectedIndex;
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

        private void CmbHorizontalScrollRailingMode_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            ComboBox cmb = sender as ComboBox;
            string suffix = cmb.Name.Substring(cmb.Name.Length - 1);
            ScrollingPresenter scrollingPresenter = GetScrollingPresenterWithSuffix(suffix);
            if (scrollingPresenter != null)
            {
                scrollingPresenter.HorizontalScrollRailingMode = (RailingMode)cmb.SelectedIndex;
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

        private void CmbVerticalScrollRailingMode_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            ComboBox cmb = sender as ComboBox;
            string suffix = cmb.Name.Substring(cmb.Name.Length - 1);
            ScrollingPresenter scrollingPresenter = GetScrollingPresenterWithSuffix(suffix);
            if (scrollingPresenter != null)
            {
                scrollingPresenter.VerticalScrollRailingMode = (RailingMode)cmb.SelectedIndex;
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

        private ScrollingPresenter GetScrollingPresenterWithSuffix(string suffix)
        {
            switch (suffix)
            {
                case "0":
                    return scrollingPresenter0;
                case "1":
                    return scrollingPresenter1;
                case "3":
                    return scrollingPresenter3;
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

        private void InitializeUIFromScrollingPresenter(string suffix)
        {
            ScrollingPresenter scrollingPresenter = FindName("scrollingPresenter" + suffix) as ScrollingPresenter;
            if (scrollingPresenter == null)
                return;

            ComboBox cmb = FindName("cmbHorizontalScrollChainingMode" + suffix) as ComboBox;
            if (cmb != null)
                cmb.SelectedIndex = (int)scrollingPresenter.HorizontalScrollChainingMode;

            cmb = FindName("cmbVerticalScrollChainingMode" + suffix) as ComboBox;
            if (cmb != null)
                cmb.SelectedIndex = (int)scrollingPresenter.VerticalScrollChainingMode;

            cmb = FindName("cmbZoomChainingMode" + suffix) as ComboBox;
            if (cmb != null)
                cmb.SelectedIndex = (int)scrollingPresenter.ZoomChainingMode;

            cmb = FindName("cmbHorizontalScrollRailingMode" + suffix) as ComboBox;
            if (cmb != null)
                cmb.SelectedIndex = (int)scrollingPresenter.HorizontalScrollRailingMode;

            cmb = FindName("cmbVerticalScrollRailingMode" + suffix) as ComboBox;
            if (cmb != null)
                cmb.SelectedIndex = (int)scrollingPresenter.VerticalScrollRailingMode;
        }

        private void InitializeUIFromScrollViewer(string suffix)
        {
            ScrollViewer scrollViewer = FindName("scrollViewer" + suffix) as ScrollViewer;
            if (scrollViewer == null)
                return;

            ComboBox cmb = FindName("cmbHorizontalScrollChainingMode" + suffix) as ComboBox;
            if (cmb != null)
                cmb.SelectedIndex = scrollViewer.IsHorizontalScrollChainingEnabled ? 0 : 1;

            cmb = FindName("cmbVerticalScrollChainingMode" + suffix) as ComboBox;
            if (cmb != null)
                cmb.SelectedIndex = scrollViewer.IsVerticalScrollChainingEnabled ? 0 : 1;

            cmb = FindName("cmbZoomChainingMode" + suffix) as ComboBox;
            if (cmb != null)
                cmb.SelectedIndex = scrollViewer.IsZoomChainingEnabled ? 0 : 1;

            cmb = FindName("cmbHorizontalScrollRailingMode" + suffix) as ComboBox;
            if (cmb != null)
                cmb.SelectedIndex = scrollViewer.IsHorizontalRailEnabled ? 0 : 1;

            cmb = FindName("cmbVerticalScrollRailingMode" + suffix) as ComboBox;
            if (cmb != null)
                cmb.SelectedIndex = scrollViewer.IsVerticalRailEnabled ? 0 : 1;
        }

        private void btnResetViews_Click(object sender, RoutedEventArgs e)
        {
            this.txtResetStatus.Text = "Resetting views ...";
            ResetView(this.scrollViewer2);
            ResetView(this.scrollingPresenter0);
            ResetView(this.scrollingPresenter1);
            ResetView(this.scrollingPresenter3);
        }

        private void ResetView(UIElement element)
        {
            ScrollingPresenter scrollingPresenter = element as ScrollingPresenter;

            if (scrollingPresenter != null)
            {
                scrollingPresenter.ScrollTo(
                    0.0,
                    0.0,
                    new ScrollingScrollOptions(AnimationMode.Disabled, SnapPointsMode.Ignore));

                int viewChangeId = scrollingPresenter.ZoomTo(
                    1.0f,
                    System.Numerics.Vector2.Zero,
                    new ScrollingZoomOptions(AnimationMode.Disabled, SnapPointsMode.Ignore)).ZoomFactorChangeId;

                if (this.scrollingPresenter3 == scrollingPresenter)
                    scrollingPresenter3ZoomFactorChangeId = viewChangeId;
            }
            else
            {
                ScrollViewer scrollViewer = element as ScrollViewer;
                scrollViewer.ChangeView(0, 0, 1.0f, true /*disableAnimation*/);
            }
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
