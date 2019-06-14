// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.Generic;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;

using InputKind = Microsoft.UI.Xaml.Controls.InputKind;
using Scroller = Microsoft.UI.Xaml.Controls.Primitives.Scroller;
using ScrollViewer = Microsoft.UI.Xaml.Controls.ScrollViewer;
using ScrollCompletedEventArgs = Microsoft.UI.Xaml.Controls.ScrollCompletedEventArgs;
using ZoomCompletedEventArgs = Microsoft.UI.Xaml.Controls.ZoomCompletedEventArgs;
using ScrollOptions = Microsoft.UI.Xaml.Controls.ScrollOptions;
using ZoomOptions = Microsoft.UI.Xaml.Controls.ZoomOptions;
using AnimationMode = Microsoft.UI.Xaml.Controls.AnimationMode;
using SnapPointsMode = Microsoft.UI.Xaml.Controls.SnapPointsMode;

using ScrollerTestHooks = Microsoft.UI.Private.Controls.ScrollerTestHooks;
using ScrollerViewChangeResult = Microsoft.UI.Private.Controls.ScrollerViewChangeResult;
using ScrollViewerTestHooks = Microsoft.UI.Private.Controls.ScrollViewerTestHooks;
using MUXControlsTestHooks = Microsoft.UI.Private.Controls.MUXControlsTestHooks;
using MUXControlsTestHooksLoggingMessageEventArgs = Microsoft.UI.Private.Controls.MUXControlsTestHooksLoggingMessageEventArgs;

namespace MUXControlsTestApp
{
    public sealed partial class ScrollViewersWithSimpleContentsPage : TestPage
    {
        private List<string> fullLogs = new List<string>();
        private int scrollViewer52ZoomFactorChangeId = -1;

        public ScrollViewersWithSimpleContentsPage()
        {
            this.InitializeComponent();

            this.scrollViewer51.XYFocusKeyboardNavigation = XYFocusKeyboardNavigationMode.Enabled;

            Loaded += ScrollViewersWithSimpleContentsPage_Loaded;
            KeyDown += ScrollViewersWithSimpleContentsPage_KeyDown;
        }

        private void ScrollViewersWithSimpleContentsPage_Loaded(object sender, RoutedEventArgs e)
        {
            ScrollViewerTestHooks.GetScrollerPart(this.scrollViewer11).StateChanged += Scroller_StateChanged;
            ScrollViewerTestHooks.GetScrollerPart(this.scrollViewer21).StateChanged += Scroller_StateChanged;
            ScrollViewerTestHooks.GetScrollerPart(this.scrollViewer31).StateChanged += Scroller_StateChanged;
            ScrollViewerTestHooks.GetScrollerPart(this.scrollViewer41).StateChanged += Scroller_StateChanged;
            ScrollViewerTestHooks.GetScrollerPart(this.scrollViewer51).StateChanged += Scroller_StateChanged;
            ScrollViewerTestHooks.GetScrollerPart(this.scrollViewer11).ViewChanged += Scroller_ViewChanged;
            ScrollViewerTestHooks.GetScrollerPart(this.scrollViewer21).ViewChanged += Scroller_ViewChanged;
            ScrollViewerTestHooks.GetScrollerPart(this.scrollViewer31).ViewChanged += Scroller_ViewChanged;
            ScrollViewerTestHooks.GetScrollerPart(this.scrollViewer41).ViewChanged += Scroller_ViewChanged;
            ScrollViewerTestHooks.GetScrollerPart(this.scrollViewer51).ViewChanged += Scroller_ViewChanged;
            ScrollViewerTestHooks.GetScrollerPart(this.scrollViewer11).ScrollCompleted += Scroller_ScrollCompleted;
            ScrollViewerTestHooks.GetScrollerPart(this.scrollViewer21).ScrollCompleted += Scroller_ScrollCompleted;
            ScrollViewerTestHooks.GetScrollerPart(this.scrollViewer31).ScrollCompleted += Scroller_ScrollCompleted;
            ScrollViewerTestHooks.GetScrollerPart(this.scrollViewer41).ScrollCompleted += Scroller_ScrollCompleted;
            ScrollViewerTestHooks.GetScrollerPart(this.scrollViewer51).ScrollCompleted += Scroller_ScrollCompleted;
            ScrollViewerTestHooks.GetScrollerPart(this.scrollViewer11).ZoomCompleted += Scroller_ZoomCompleted;
            ScrollViewerTestHooks.GetScrollerPart(this.scrollViewer21).ZoomCompleted += Scroller_ZoomCompleted;
            ScrollViewerTestHooks.GetScrollerPart(this.scrollViewer31).ZoomCompleted += Scroller_ZoomCompleted;
            ScrollViewerTestHooks.GetScrollerPart(this.scrollViewer41).ZoomCompleted += Scroller_ZoomCompleted;
            ScrollViewerTestHooks.GetScrollerPart(this.scrollViewer51).ZoomCompleted += Scroller_ZoomCompleted;

            ScrollViewerTestHooks.GetScrollerPart(this.scrollViewer12).StateChanged += Scroller_StateChanged;
            ScrollViewerTestHooks.GetScrollerPart(this.scrollViewer22).StateChanged += Scroller_StateChanged;
            ScrollViewerTestHooks.GetScrollerPart(this.scrollViewer32).StateChanged += Scroller_StateChanged;
            ScrollViewerTestHooks.GetScrollerPart(this.scrollViewer42).StateChanged += Scroller_StateChanged;
            ScrollViewerTestHooks.GetScrollerPart(this.scrollViewer52).StateChanged += Scroller_StateChanged;
            ScrollViewerTestHooks.GetScrollerPart(this.scrollViewer12).ViewChanged += Scroller_ViewChanged;
            ScrollViewerTestHooks.GetScrollerPart(this.scrollViewer22).ViewChanged += Scroller_ViewChanged;
            ScrollViewerTestHooks.GetScrollerPart(this.scrollViewer32).ViewChanged += Scroller_ViewChanged;
            ScrollViewerTestHooks.GetScrollerPart(this.scrollViewer42).ViewChanged += Scroller_ViewChanged;
            ScrollViewerTestHooks.GetScrollerPart(this.scrollViewer52).ViewChanged += Scroller_ViewChanged;
            ScrollViewerTestHooks.GetScrollerPart(this.scrollViewer12).ScrollCompleted += Scroller_ScrollCompleted;
            ScrollViewerTestHooks.GetScrollerPart(this.scrollViewer22).ScrollCompleted += Scroller_ScrollCompleted;
            ScrollViewerTestHooks.GetScrollerPart(this.scrollViewer32).ScrollCompleted += Scroller_ScrollCompleted;
            ScrollViewerTestHooks.GetScrollerPart(this.scrollViewer42).ScrollCompleted += Scroller_ScrollCompleted;
            ScrollViewerTestHooks.GetScrollerPart(this.scrollViewer52).ScrollCompleted += Scroller_ScrollCompleted;
            ScrollViewerTestHooks.GetScrollerPart(this.scrollViewer12).ZoomCompleted += Scroller_ZoomCompleted;
            ScrollViewerTestHooks.GetScrollerPart(this.scrollViewer22).ZoomCompleted += Scroller_ZoomCompleted;
            ScrollViewerTestHooks.GetScrollerPart(this.scrollViewer32).ZoomCompleted += Scroller_ZoomCompleted;
            ScrollViewerTestHooks.GetScrollerPart(this.scrollViewer42).ZoomCompleted += Scroller_ZoomCompleted;
            ScrollViewerTestHooks.GetScrollerPart(this.scrollViewer52).ZoomCompleted += Scroller_ZoomCompleted;
        }

        private void ScrollViewersWithSimpleContentsPage_KeyDown(object sender, KeyRoutedEventArgs e)
        {
            if (e.Key == Windows.System.VirtualKey.G)
            {
                GetFullLog();
            }
            else if (e.Key == Windows.System.VirtualKey.C)
            {
                ClearFullLog();
            }
        }

        private void ChkLogScrollerMessages_Checked(object sender, RoutedEventArgs e)
        {
            MUXControlsTestHooks.SetOutputDebugStringLevelForType(
                type: "Scroller",
                isLoggingInfoLevel: true,
                isLoggingVerboseLevel: true);

            MUXControlsTestHooks.SetLoggingLevelForType(
                type: "Scroller",
                isLoggingInfoLevel: true,
                isLoggingVerboseLevel: true);

            if (chkLogScrollViewerMessages.IsChecked == false)
                MUXControlsTestHooks.LoggingMessage += MUXControlsTestHooks_LoggingMessage;
        }

        private void ChkLogScrollerMessages_Unchecked(object sender, RoutedEventArgs e)
        {
            MUXControlsTestHooks.SetOutputDebugStringLevelForType(
                type: "Scroller",
                isLoggingInfoLevel: false,
                isLoggingVerboseLevel: false);

            MUXControlsTestHooks.SetLoggingLevelForType(
                type: "Scroller",
                isLoggingInfoLevel: false,
                isLoggingVerboseLevel: false);

            if (chkLogScrollViewerMessages.IsChecked == false)
                MUXControlsTestHooks.LoggingMessage -= MUXControlsTestHooks_LoggingMessage;
        }

        private void ChkLogScrollViewerMessages_Checked(object sender, RoutedEventArgs e)
        {
            MUXControlsTestHooks.SetOutputDebugStringLevelForType(
                type: "ScrollViewer",
                isLoggingInfoLevel: true,
                isLoggingVerboseLevel: true);

            MUXControlsTestHooks.SetLoggingLevelForType(
                type: "ScrollViewer",
                isLoggingInfoLevel: true,
                isLoggingVerboseLevel: true);

            if (chkLogScrollerMessages.IsChecked == false)
                MUXControlsTestHooks.LoggingMessage += MUXControlsTestHooks_LoggingMessage;
        }

        private void ChkLogScrollViewerMessages_Unchecked(object sender, RoutedEventArgs e)
        {
            MUXControlsTestHooks.SetOutputDebugStringLevelForType(
                type: "ScrollViewer",
                isLoggingInfoLevel: false,
                isLoggingVerboseLevel: false);

            MUXControlsTestHooks.SetLoggingLevelForType(
                type: "ScrollViewer",
                isLoggingInfoLevel: false,
                isLoggingVerboseLevel: false);

            if (chkLogScrollerMessages.IsChecked == false)
                MUXControlsTestHooks.LoggingMessage -= MUXControlsTestHooks_LoggingMessage;
        }

        private void Scroller_StateChanged(Scroller sender, object args)
        {
            string senderId = "." + sender.Name;
            FrameworkElement parent = VisualTreeHelper.GetParent(sender) as FrameworkElement;
            if (parent != null)
            {
                senderId = "." + parent.Name + senderId;
                FrameworkElement grandParent = VisualTreeHelper.GetParent(parent) as FrameworkElement;
                if (grandParent != null)
                {
                    senderId = grandParent.Name + senderId;
                }
            }
            this.txtScrollerState.Text = senderId + " " + sender.State.ToString();
            this.fullLogs.Add(senderId + " StateChanged S=" + sender.State.ToString());
            chkLogUpdated.IsChecked = false;
        }

        private void Scroller_ViewChanged(Scroller sender, object args)
        {
            string senderId = "." + sender.Name;
            FrameworkElement parent = VisualTreeHelper.GetParent(sender) as FrameworkElement;
            if (parent != null)
            {
                senderId = parent.Name + senderId;
            }
            this.txtScrollerHorizontalOffset.Text = sender.HorizontalOffset.ToString();
            this.txtScrollerVerticalOffset.Text = sender.VerticalOffset.ToString();
            this.txtScrollerZoomFactor.Text = sender.ZoomFactor.ToString();
            this.fullLogs.Add(senderId + " ViewChanged H=" + this.txtScrollerHorizontalOffset.Text + ", V=" + this.txtScrollerVerticalOffset.Text + ", ZF=" + this.txtScrollerZoomFactor.Text);
            chkLogUpdated.IsChecked = false;
        }

        private void Scroller_ScrollCompleted(Scroller sender, ScrollCompletedEventArgs args)
        {
            string senderId = "." + sender.Name;
            FrameworkElement parent = VisualTreeHelper.GetParent(sender) as FrameworkElement;
            if (parent != null)
            {
                senderId = parent.Name + senderId;
            }

            ScrollerViewChangeResult result = ScrollerTestHooks.GetScrollCompletedResult(args);

            this.fullLogs.Add(senderId + " ScrollCompleted. OffsetsChangeId=" + args.ScrollInfo.OffsetsChangeId + ", Result=" + result);
            chkLogUpdated.IsChecked = false;
        }

        private void Scroller_ZoomCompleted(Scroller sender, ZoomCompletedEventArgs args)
        {
            string senderId = "." + sender.Name;
            FrameworkElement parent = VisualTreeHelper.GetParent(sender) as FrameworkElement;
            if (parent != null)
            {
                senderId = parent.Name + senderId;
            }

            ScrollerViewChangeResult result = ScrollerTestHooks.GetZoomCompletedResult(args);

            this.fullLogs.Add(senderId + " ZoomCompleted. ZoomFactorChangeId=" + args.ZoomInfo.ZoomFactorChangeId + ", Result=" + result);
            chkLogUpdated.IsChecked = false;

            if (args.ZoomInfo.ZoomFactorChangeId == scrollViewer52ZoomFactorChangeId)
            {
                this.txtResetStatus.Text = "Views reset";
                scrollViewer52ZoomFactorChangeId = -1;
            }
        }

        private void CmbShowScrollViewer_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (this.scrollViewer11 != null)
            {
                if (cmbShowScrollViewer.SelectedIndex == 0)
                {
                    this.scrollViewer11.Visibility = Visibility.Visible;
                    this.scrollViewer21.Visibility = Visibility.Visible;
                    this.scrollViewer31.Visibility = Visibility.Visible;
                    this.scrollViewer41.Visibility = Visibility.Visible;
                    this.scrollViewer51.Visibility = Visibility.Visible;
                    this.scrollViewer12.Visibility = Visibility.Visible;
                    this.scrollViewer22.Visibility = Visibility.Visible;
                    this.scrollViewer32.Visibility = Visibility.Visible;
                    this.scrollViewer42.Visibility = Visibility.Visible;
                    this.scrollViewer52.Visibility = Visibility.Visible;

                    this.scrollViewer11.Width = double.NaN;
                    this.scrollViewer21.Width = double.NaN;
                    this.scrollViewer31.Width = double.NaN;
                    this.scrollViewer41.Width = double.NaN;
                    this.scrollViewer51.Width = double.NaN;
                    this.scrollViewer12.Width = double.NaN;
                    this.scrollViewer22.Width = double.NaN;
                    this.scrollViewer32.Width = double.NaN;
                    this.scrollViewer42.Width = double.NaN;
                    this.scrollViewer52.Width = double.NaN;
                    this.scrollViewer11.Height = double.NaN;
                    this.scrollViewer21.Height = double.NaN;
                    this.scrollViewer31.Height = double.NaN;
                    this.scrollViewer41.Height = double.NaN;
                    this.scrollViewer51.Height = double.NaN;
                    this.scrollViewer12.Height = double.NaN;
                    this.scrollViewer22.Height = double.NaN;
                    this.scrollViewer32.Height = double.NaN;
                    this.scrollViewer42.Height = double.NaN;
                    this.scrollViewer52.Height = double.NaN;

                    for (int rowIndex = 2; rowIndex < 4; rowIndex++)
                        this.rootGrid.RowDefinitions[rowIndex].Height = new GridLength(1, GridUnitType.Star);

                    for (int columnIndex = 0; columnIndex < 5; columnIndex++)
                        this.rootGrid.ColumnDefinitions[columnIndex].Width = new GridLength(1, GridUnitType.Star);

                    cmbIgnoredInputKind.IsEnabled = false;
                    cmbIgnoredInputKind.SelectedIndex = 0;
                }
                else
                {
                    this.scrollViewer11.Visibility = Visibility.Collapsed;
                    this.scrollViewer21.Visibility = Visibility.Collapsed;
                    this.scrollViewer31.Visibility = Visibility.Collapsed;
                    this.scrollViewer41.Visibility = Visibility.Collapsed;
                    this.scrollViewer51.Visibility = Visibility.Collapsed;
                    this.scrollViewer12.Visibility = Visibility.Collapsed;
                    this.scrollViewer22.Visibility = Visibility.Collapsed;
                    this.scrollViewer32.Visibility = Visibility.Collapsed;
                    this.scrollViewer42.Visibility = Visibility.Collapsed;
                    this.scrollViewer52.Visibility = Visibility.Collapsed;

                    for (int rowIndex = 2; rowIndex < 4; rowIndex++)
                        this.rootGrid.RowDefinitions[rowIndex].Height = GridLength.Auto;

                    for (int columnIndex = 0; columnIndex < 5; columnIndex++)
                        this.rootGrid.ColumnDefinitions[columnIndex].Width = GridLength.Auto;

                    cmbIgnoredInputKind.IsEnabled = true;

                    ScrollViewer scrollViewer = SelectedScrollViewer;

                    scrollViewer.Visibility = Visibility.Visible;
                    scrollViewer.Width = 300;
                    scrollViewer.Height = 400;

                    txtScrollerHorizontalOffset.Text = scrollViewer.HorizontalOffset.ToString();
                    txtScrollerVerticalOffset.Text = scrollViewer.VerticalOffset.ToString();
                    txtScrollerZoomFactor.Text = scrollViewer.ZoomFactor.ToString();

                    switch (scrollViewer.IgnoredInputKind)
                    {
                        case InputKind.None:
                            cmbIgnoredInputKind.SelectedIndex = 1;
                            break;
                        case InputKind.Touch:
                            cmbIgnoredInputKind.SelectedIndex = 2;
                            break;
                        case InputKind.Pen:
                            cmbIgnoredInputKind.SelectedIndex = 3;
                            break;
                        case InputKind.MouseWheel:
                            cmbIgnoredInputKind.SelectedIndex = 4;
                            break;
                        case InputKind.Keyboard:
                            cmbIgnoredInputKind.SelectedIndex = 5;
                            break;
                        case InputKind.Gamepad:
                            cmbIgnoredInputKind.SelectedIndex = 6;
                            break;
                        case InputKind.All:
                            cmbIgnoredInputKind.SelectedIndex = 7;
                            break;
                        default:
                            cmbIgnoredInputKind.SelectedIndex = 0;
                            break;
                    }
                }
            }
        }

        private void CmbIgnoredInputKind_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            InputKind ignoredInputKind;
            ScrollViewer scrollViewer = SelectedScrollViewer;

            switch (cmbIgnoredInputKind.SelectedIndex)
            {
                case 0:
                    return;
                case 1:
                    ignoredInputKind = InputKind.None;
                    break;
                case 2:
                    ignoredInputKind = InputKind.Touch;
                    break;
                case 3:
                    ignoredInputKind = InputKind.Pen;
                    break;
                case 4:
                    ignoredInputKind = InputKind.MouseWheel;
                    break;
                case 5:
                    ignoredInputKind = InputKind.Keyboard;
                    break;
                case 6:
                    ignoredInputKind = InputKind.Gamepad;
                    break;
                default:
                    ignoredInputKind = InputKind.All;
                    break;
            }

            scrollViewer.IgnoredInputKind = ignoredInputKind;
        }

        private void btnGetFullLog_Click(object sender, RoutedEventArgs e)
        {
            GetFullLog();
        }

        private void btnClearFullLog_Click(object sender, RoutedEventArgs e)
        {
            ClearFullLog();
        }

        private void btnResetViews_Click(object sender, RoutedEventArgs e)
        {
            this.txtResetStatus.Text = "Resetting views ...";
            ResetView(this.scrollViewer11);
            ResetView(this.scrollViewer21);
            ResetView(this.scrollViewer31);
            ResetView(this.scrollViewer41);
            ResetView(this.scrollViewer51);
            ResetView(this.scrollViewer12);
            ResetView(this.scrollViewer22);
            ResetView(this.scrollViewer32);
            ResetView(this.scrollViewer42);
            ResetView(this.scrollViewer52);
        }

        private void GetFullLog()
        {
            this.txtResetStatus.Text = "GetFullLog. Populating cmbFullLog...";
            chkLogCleared.IsChecked = false;
            foreach (string log in this.fullLogs)
            {
                this.cmbFullLog.Items.Add(log);
            }
            chkLogUpdated.IsChecked = true;
            this.txtResetStatus.Text = "GetFullLog. Done.";
        }

        private void ClearFullLog()
        {
            this.txtResetStatus.Text = "ClearFullLog. Clearing cmbFullLog & fullLogs...";
            chkLogUpdated.IsChecked = false;
            this.fullLogs.Clear();
            this.cmbFullLog.Items.Clear();
            chkLogCleared.IsChecked = true;
            this.txtResetStatus.Text = "ClearFullLog. Done.";
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

            fullLogs.Add((args.IsVerboseLevel ? "Verbose: " : "Info: ") + senderName + "m:" + msg);
        }

        private void ResetView(ScrollViewer scrollViewer)
        {
            Scroller scroller = ScrollViewerTestHooks.GetScrollerPart(scrollViewer);
            string scrollerId = (VisualTreeHelper.GetParent(scroller) as FrameworkElement).Name + "." + scroller.Name;

            int viewChangeId = scroller.ScrollTo(0.0, 0.0, new ScrollOptions(AnimationMode.Disabled, SnapPointsMode.Ignore)).OffsetsChangeId;
            this.fullLogs.Add(scrollerId + " ScrollTo requested. Id=" + viewChangeId);

            viewChangeId = scroller.ZoomTo(1.0f, System.Numerics.Vector2.Zero, new ZoomOptions(AnimationMode.Disabled, SnapPointsMode.Ignore)).ZoomFactorChangeId;
            this.fullLogs.Add(scrollerId + " ZoomTo requested. Id=" + viewChangeId);

            chkLogUpdated.IsChecked = false;

            if (scrollViewer == this.scrollViewer52)
                scrollViewer52ZoomFactorChangeId = viewChangeId;
        }

        private ScrollViewer SelectedScrollViewer
        {
            get
            {
                ScrollViewer scrollViewer = null;

                switch (cmbShowScrollViewer.SelectedIndex)
                {
                    case 1:
                        scrollViewer = this.scrollViewer11;
                        break;
                    case 2:
                        scrollViewer = this.scrollViewer21;
                        break;
                    case 3:
                        scrollViewer = this.scrollViewer31;
                        break;
                    case 4:
                        scrollViewer = this.scrollViewer41;
                        break;
                    case 5:
                        scrollViewer = this.scrollViewer51;
                        break;
                    case 6:
                        scrollViewer = this.scrollViewer12;
                        break;
                    case 7:
                        scrollViewer = this.scrollViewer22;
                        break;
                    case 8:
                        scrollViewer = this.scrollViewer32;
                        break;
                    case 9:
                        scrollViewer = this.scrollViewer42;
                        break;
                    case 10:
                        scrollViewer = this.scrollViewer52;
                        break;
                }

                return scrollViewer;
            }
        }
    }
}
