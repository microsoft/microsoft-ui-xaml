// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;

using Scroller = Microsoft.UI.Xaml.Controls.Primitives.Scroller;
using ChainingMode = Microsoft.UI.Xaml.Controls.ChainingMode;
using RailingMode = Microsoft.UI.Xaml.Controls.RailingMode;
using AnimationMode = Microsoft.UI.Xaml.Controls.AnimationMode;
using SnapPointsMode = Microsoft.UI.Xaml.Controls.SnapPointsMode;
using ScrollOptions = Microsoft.UI.Xaml.Controls.ScrollOptions;
using ZoomOptions = Microsoft.UI.Xaml.Controls.ZoomOptions;
using ZoomCompletedEventArgs = Microsoft.UI.Xaml.Controls.ZoomCompletedEventArgs;
using MUXControlsTestHooks = Microsoft.UI.Private.Controls.MUXControlsTestHooks;
using MUXControlsTestHooksLoggingMessageEventArgs = Microsoft.UI.Private.Controls.MUXControlsTestHooksLoggingMessageEventArgs;

namespace MUXControlsTestApp
{
    public sealed partial class ScrollerChainingAndRailingPage : TestPage
    {
        private List<string> fullLogs = new List<string>();
        private int scroller3ZoomFactorChangeId = -1;

        public ScrollerChainingAndRailingPage()
        {
            InitializeComponent();
            InitializeUIFromScroller("0");
            InitializeUIFromScroller("1");
            InitializeUIFromScroller("3");
            InitializeUIFromScrollViewer("2");

            scroller0.StateChanged += Scroller_StateChanged;
            scroller1.StateChanged += Scroller_StateChanged;
            scroller3.StateChanged += Scroller_StateChanged;

            scroller3.ZoomCompleted += Scroller_ZoomCompleted;

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

        private void Scroller_StateChanged(Scroller sender, object args)
        {
            if (sender == scroller0)
            {
                txtScroller0State.Text = "scroller0 " + scroller0.State.ToString();
                txtScroller0HSP.Text = String.Format("{0,5:N1}%", sender.HorizontalOffset / 3.0);
                txtScroller0VSP.Text = String.Format("{0,5:N1}%", sender.VerticalOffset / 3.0);
            }
            else if (sender == scroller1)
            {
                txtScroller1State.Text = "scroller1 " + scroller1.State.ToString();
                txtScroller1HSP.Text = String.Format("{0,5:N1}%", sender.HorizontalOffset / 3.0);
                txtScroller1VSP.Text = String.Format("{0,5:N1}%", sender.VerticalOffset / 3.0);
            }
            else if (sender == scroller3)
            {
                txtScroller3State.Text = "scroller3 " + scroller3.State.ToString();
                txtScroller3HSP.Text = String.Format("{0,5:N1}%", sender.HorizontalOffset / 3.0);
                txtScroller3VSP.Text = String.Format("{0,5:N1}%", sender.VerticalOffset / 3.0);
            }
        }

        private void Scroller_ZoomCompleted(Scroller sender, ZoomCompletedEventArgs args)
        {
            if (args.ZoomInfo.ZoomFactorChangeId == scroller3ZoomFactorChangeId)
                this.txtResetStatus.Text = "Views reset";
        }

        private void CmbHorizontalScrollChainingMode_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            ComboBox cmb = sender as ComboBox;
            string suffix = cmb.Name.Substring(cmb.Name.Length - 1);
            Scroller scroller = GetScrollerWithSuffix(suffix);
            if (scroller != null)
            {
                scroller.HorizontalScrollChainingMode = (ChainingMode)cmb.SelectedIndex;
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
            Scroller scroller = GetScrollerWithSuffix(suffix);
            if (scroller != null)
            {
                scroller.VerticalScrollChainingMode = (ChainingMode)cmb.SelectedIndex;
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
            Scroller scroller = GetScrollerWithSuffix(suffix);
            if (scroller != null)
            {
                scroller.ZoomChainingMode = (ChainingMode)cmb.SelectedIndex;
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
            Scroller scroller = GetScrollerWithSuffix(suffix);
            if (scroller != null)
            {
                scroller.HorizontalScrollRailingMode = (RailingMode)cmb.SelectedIndex;
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
            Scroller scroller = GetScrollerWithSuffix(suffix);
            if (scroller != null)
            {
                scroller.VerticalScrollRailingMode = (RailingMode)cmb.SelectedIndex;
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

        private Scroller GetScrollerWithSuffix(string suffix)
        {
            switch (suffix)
            {
                case "0":
                    return scroller0;
                case "1":
                    return scroller1;
                case "3":
                    return scroller3;
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

        private void InitializeUIFromScroller(string suffix)
        {
            Scroller scroller = FindName("scroller" + suffix) as Scroller;
            if (scroller == null)
                return;

            ComboBox cmb = FindName("cmbHorizontalScrollChainingMode" + suffix) as ComboBox;
            if (cmb != null)
                cmb.SelectedIndex = (int)scroller.HorizontalScrollChainingMode;

            cmb = FindName("cmbVerticalScrollChainingMode" + suffix) as ComboBox;
            if (cmb != null)
                cmb.SelectedIndex = (int)scroller.VerticalScrollChainingMode;

            cmb = FindName("cmbZoomChainingMode" + suffix) as ComboBox;
            if (cmb != null)
                cmb.SelectedIndex = (int)scroller.ZoomChainingMode;

            cmb = FindName("cmbHorizontalScrollRailingMode" + suffix) as ComboBox;
            if (cmb != null)
                cmb.SelectedIndex = (int)scroller.HorizontalScrollRailingMode;

            cmb = FindName("cmbVerticalScrollRailingMode" + suffix) as ComboBox;
            if (cmb != null)
                cmb.SelectedIndex = (int)scroller.VerticalScrollRailingMode;
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
            ResetView(this.scroller0);
            ResetView(this.scroller1);
            ResetView(this.scroller3);
        }

        private void ResetView(UIElement element)
        {
            Scroller scroller = element as Scroller;

            if (scroller != null)
            {
                scroller.ScrollTo(
                    0.0,
                    0.0,
                    new ScrollOptions(AnimationMode.Disabled, SnapPointsMode.Ignore));

                int viewChangeId = scroller.ZoomTo(
                    1.0f,
                    System.Numerics.Vector2.Zero,
                    new ZoomOptions(AnimationMode.Disabled, SnapPointsMode.Ignore)).ZoomFactorChangeId;

                if (this.scroller3 == scroller)
                    scroller3ZoomFactorChangeId = viewChangeId;
            }
            else
            {
                ScrollViewer scrollViewer = element as ScrollViewer;
                scrollViewer.ChangeView(0, 0, 1.0f, true /*disableAnimation*/);
            }
        }

        private void ChkLogScrollerMessages_Checked(object sender, RoutedEventArgs e)
        {
            //Turn on info and verbose logging for the Scroller type:
            MUXControlsTestHooks.SetLoggingLevelForType("Scroller", isLoggingInfoLevel: true, isLoggingVerboseLevel: true);

            MUXControlsTestHooks.LoggingMessage += MUXControlsTestHooks_LoggingMessage;
        }

        private void ChkLogScrollerMessages_Unchecked(object sender, RoutedEventArgs e)
        {
            //Turn off info and verbose logging for the Scroller type:
            MUXControlsTestHooks.SetLoggingLevelForType("Scroller", isLoggingInfoLevel: false, isLoggingVerboseLevel: false);

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
