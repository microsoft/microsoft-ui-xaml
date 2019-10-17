// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.Generic;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;

using InputKind = Microsoft.UI.Xaml.Controls.InputKind;
using ScrollingPresenter = Microsoft.UI.Xaml.Controls.Primitives.ScrollingPresenter;
using ScrollingView = Microsoft.UI.Xaml.Controls.ScrollingView;
using ScrollCompletedEventArgs = Microsoft.UI.Xaml.Controls.ScrollCompletedEventArgs;
using ZoomCompletedEventArgs = Microsoft.UI.Xaml.Controls.ZoomCompletedEventArgs;
using ScrollOptions = Microsoft.UI.Xaml.Controls.ScrollOptions;
using ZoomOptions = Microsoft.UI.Xaml.Controls.ZoomOptions;
using AnimationMode = Microsoft.UI.Xaml.Controls.AnimationMode;
using SnapPointsMode = Microsoft.UI.Xaml.Controls.SnapPointsMode;

using ScrollingPresenterTestHooks = Microsoft.UI.Private.Controls.ScrollingPresenterTestHooks;
using ScrollingPresenterViewChangeResult = Microsoft.UI.Private.Controls.ScrollingPresenterViewChangeResult;
using ScrollingViewTestHooks = Microsoft.UI.Private.Controls.ScrollingViewTestHooks;
using MUXControlsTestHooks = Microsoft.UI.Private.Controls.MUXControlsTestHooks;
using MUXControlsTestHooksLoggingMessageEventArgs = Microsoft.UI.Private.Controls.MUXControlsTestHooksLoggingMessageEventArgs;

namespace MUXControlsTestApp
{
    public sealed partial class ScrollingViewsWithSimpleContentsPage : TestPage
    {
        private List<string> fullLogs = new List<string>();
        private int scrollingView52ZoomFactorChangeId = -1;

        public ScrollingViewsWithSimpleContentsPage()
        {
            this.InitializeComponent();

            this.scrollingView51.XYFocusKeyboardNavigation = XYFocusKeyboardNavigationMode.Enabled;

            Loaded += ScrollingViewsWithSimpleContentsPage_Loaded;
            KeyDown += ScrollingViewsWithSimpleContentsPage_KeyDown;
        }

        private void ScrollingViewsWithSimpleContentsPage_Loaded(object sender, RoutedEventArgs e)
        {
            ScrollingViewTestHooks.GetScrollingPresenterPart(this.scrollingView11).StateChanged += ScrollingPresenter_StateChanged;
            ScrollingViewTestHooks.GetScrollingPresenterPart(this.scrollingView21).StateChanged += ScrollingPresenter_StateChanged;
            ScrollingViewTestHooks.GetScrollingPresenterPart(this.scrollingView31).StateChanged += ScrollingPresenter_StateChanged;
            ScrollingViewTestHooks.GetScrollingPresenterPart(this.scrollingView41).StateChanged += ScrollingPresenter_StateChanged;
            ScrollingViewTestHooks.GetScrollingPresenterPart(this.scrollingView51).StateChanged += ScrollingPresenter_StateChanged;
            ScrollingViewTestHooks.GetScrollingPresenterPart(this.scrollingView11).ViewChanged += ScrollingPresenter_ViewChanged;
            ScrollingViewTestHooks.GetScrollingPresenterPart(this.scrollingView21).ViewChanged += ScrollingPresenter_ViewChanged;
            ScrollingViewTestHooks.GetScrollingPresenterPart(this.scrollingView31).ViewChanged += ScrollingPresenter_ViewChanged;
            ScrollingViewTestHooks.GetScrollingPresenterPart(this.scrollingView41).ViewChanged += ScrollingPresenter_ViewChanged;
            ScrollingViewTestHooks.GetScrollingPresenterPart(this.scrollingView51).ViewChanged += ScrollingPresenter_ViewChanged;
            ScrollingViewTestHooks.GetScrollingPresenterPart(this.scrollingView11).ScrollCompleted += ScrollingPresenter_ScrollCompleted;
            ScrollingViewTestHooks.GetScrollingPresenterPart(this.scrollingView21).ScrollCompleted += ScrollingPresenter_ScrollCompleted;
            ScrollingViewTestHooks.GetScrollingPresenterPart(this.scrollingView31).ScrollCompleted += ScrollingPresenter_ScrollCompleted;
            ScrollingViewTestHooks.GetScrollingPresenterPart(this.scrollingView41).ScrollCompleted += ScrollingPresenter_ScrollCompleted;
            ScrollingViewTestHooks.GetScrollingPresenterPart(this.scrollingView51).ScrollCompleted += ScrollingPresenter_ScrollCompleted;
            ScrollingViewTestHooks.GetScrollingPresenterPart(this.scrollingView11).ZoomCompleted += ScrollingPresenter_ZoomCompleted;
            ScrollingViewTestHooks.GetScrollingPresenterPart(this.scrollingView21).ZoomCompleted += ScrollingPresenter_ZoomCompleted;
            ScrollingViewTestHooks.GetScrollingPresenterPart(this.scrollingView31).ZoomCompleted += ScrollingPresenter_ZoomCompleted;
            ScrollingViewTestHooks.GetScrollingPresenterPart(this.scrollingView41).ZoomCompleted += ScrollingPresenter_ZoomCompleted;
            ScrollingViewTestHooks.GetScrollingPresenterPart(this.scrollingView51).ZoomCompleted += ScrollingPresenter_ZoomCompleted;

            ScrollingViewTestHooks.GetScrollingPresenterPart(this.scrollingView12).StateChanged += ScrollingPresenter_StateChanged;
            ScrollingViewTestHooks.GetScrollingPresenterPart(this.scrollingView22).StateChanged += ScrollingPresenter_StateChanged;
            ScrollingViewTestHooks.GetScrollingPresenterPart(this.scrollingView32).StateChanged += ScrollingPresenter_StateChanged;
            ScrollingViewTestHooks.GetScrollingPresenterPart(this.scrollingView42).StateChanged += ScrollingPresenter_StateChanged;
            ScrollingViewTestHooks.GetScrollingPresenterPart(this.scrollingView52).StateChanged += ScrollingPresenter_StateChanged;
            ScrollingViewTestHooks.GetScrollingPresenterPart(this.scrollingView12).ViewChanged += ScrollingPresenter_ViewChanged;
            ScrollingViewTestHooks.GetScrollingPresenterPart(this.scrollingView22).ViewChanged += ScrollingPresenter_ViewChanged;
            ScrollingViewTestHooks.GetScrollingPresenterPart(this.scrollingView32).ViewChanged += ScrollingPresenter_ViewChanged;
            ScrollingViewTestHooks.GetScrollingPresenterPart(this.scrollingView42).ViewChanged += ScrollingPresenter_ViewChanged;
            ScrollingViewTestHooks.GetScrollingPresenterPart(this.scrollingView52).ViewChanged += ScrollingPresenter_ViewChanged;
            ScrollingViewTestHooks.GetScrollingPresenterPart(this.scrollingView12).ScrollCompleted += ScrollingPresenter_ScrollCompleted;
            ScrollingViewTestHooks.GetScrollingPresenterPart(this.scrollingView22).ScrollCompleted += ScrollingPresenter_ScrollCompleted;
            ScrollingViewTestHooks.GetScrollingPresenterPart(this.scrollingView32).ScrollCompleted += ScrollingPresenter_ScrollCompleted;
            ScrollingViewTestHooks.GetScrollingPresenterPart(this.scrollingView42).ScrollCompleted += ScrollingPresenter_ScrollCompleted;
            ScrollingViewTestHooks.GetScrollingPresenterPart(this.scrollingView52).ScrollCompleted += ScrollingPresenter_ScrollCompleted;
            ScrollingViewTestHooks.GetScrollingPresenterPart(this.scrollingView12).ZoomCompleted += ScrollingPresenter_ZoomCompleted;
            ScrollingViewTestHooks.GetScrollingPresenterPart(this.scrollingView22).ZoomCompleted += ScrollingPresenter_ZoomCompleted;
            ScrollingViewTestHooks.GetScrollingPresenterPart(this.scrollingView32).ZoomCompleted += ScrollingPresenter_ZoomCompleted;
            ScrollingViewTestHooks.GetScrollingPresenterPart(this.scrollingView42).ZoomCompleted += ScrollingPresenter_ZoomCompleted;
            ScrollingViewTestHooks.GetScrollingPresenterPart(this.scrollingView52).ZoomCompleted += ScrollingPresenter_ZoomCompleted;
        }

        private void ScrollingViewsWithSimpleContentsPage_KeyDown(object sender, KeyRoutedEventArgs e)
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

        private void ChkLogScrollingPresenterMessages_Checked(object sender, RoutedEventArgs e)
        {
            MUXControlsTestHooks.SetOutputDebugStringLevelForType(
                type: "ScrollingPresenter",
                isLoggingInfoLevel: true,
                isLoggingVerboseLevel: true);

            MUXControlsTestHooks.SetLoggingLevelForType(
                type: "ScrollingPresenter",
                isLoggingInfoLevel: true,
                isLoggingVerboseLevel: true);

            if (chkLogScrollingViewMessages.IsChecked == false)
                MUXControlsTestHooks.LoggingMessage += MUXControlsTestHooks_LoggingMessage;
        }

        private void ChkLogScrollingPresenterMessages_Unchecked(object sender, RoutedEventArgs e)
        {
            MUXControlsTestHooks.SetOutputDebugStringLevelForType(
                type: "ScrollingPresenter",
                isLoggingInfoLevel: false,
                isLoggingVerboseLevel: false);

            MUXControlsTestHooks.SetLoggingLevelForType(
                type: "ScrollingPresenter",
                isLoggingInfoLevel: false,
                isLoggingVerboseLevel: false);

            if (chkLogScrollingViewMessages.IsChecked == false)
                MUXControlsTestHooks.LoggingMessage -= MUXControlsTestHooks_LoggingMessage;
        }

        private void ChkLogScrollingViewMessages_Checked(object sender, RoutedEventArgs e)
        {
            MUXControlsTestHooks.SetOutputDebugStringLevelForType(
                type: "ScrollingView",
                isLoggingInfoLevel: true,
                isLoggingVerboseLevel: true);

            MUXControlsTestHooks.SetLoggingLevelForType(
                type: "ScrollingView",
                isLoggingInfoLevel: true,
                isLoggingVerboseLevel: true);

            if (chkLogScrollingPresenterMessages.IsChecked == false)
                MUXControlsTestHooks.LoggingMessage += MUXControlsTestHooks_LoggingMessage;
        }

        private void ChkLogScrollingViewMessages_Unchecked(object sender, RoutedEventArgs e)
        {
            MUXControlsTestHooks.SetOutputDebugStringLevelForType(
                type: "ScrollingView",
                isLoggingInfoLevel: false,
                isLoggingVerboseLevel: false);

            MUXControlsTestHooks.SetLoggingLevelForType(
                type: "ScrollingView",
                isLoggingInfoLevel: false,
                isLoggingVerboseLevel: false);

            if (chkLogScrollingPresenterMessages.IsChecked == false)
                MUXControlsTestHooks.LoggingMessage -= MUXControlsTestHooks_LoggingMessage;
        }

        private void ScrollingPresenter_StateChanged(ScrollingPresenter sender, object args)
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
            this.txtScrollingPresenterState.Text = senderId + " " + sender.State.ToString();
            this.fullLogs.Add(senderId + " StateChanged S=" + sender.State.ToString());
            chkLogUpdated.IsChecked = false;
        }

        private void ScrollingPresenter_ViewChanged(ScrollingPresenter sender, object args)
        {
            string senderId = "." + sender.Name;
            FrameworkElement parent = VisualTreeHelper.GetParent(sender) as FrameworkElement;
            if (parent != null)
            {
                senderId = parent.Name + senderId;
            }
            this.txtScrollingPresenterHorizontalOffset.Text = sender.HorizontalOffset.ToString();
            this.txtScrollingPresenterVerticalOffset.Text = sender.VerticalOffset.ToString();
            this.txtScrollingPresenterZoomFactor.Text = sender.ZoomFactor.ToString();
            this.fullLogs.Add(senderId + " ViewChanged H=" + this.txtScrollingPresenterHorizontalOffset.Text + ", V=" + this.txtScrollingPresenterVerticalOffset.Text + ", ZF=" + this.txtScrollingPresenterZoomFactor.Text);
            chkLogUpdated.IsChecked = false;
        }

        private void ScrollingPresenter_ScrollCompleted(ScrollingPresenter sender, ScrollCompletedEventArgs args)
        {
            string senderId = "." + sender.Name;
            FrameworkElement parent = VisualTreeHelper.GetParent(sender) as FrameworkElement;
            if (parent != null)
            {
                senderId = parent.Name + senderId;
            }

            ScrollingPresenterViewChangeResult result = ScrollingPresenterTestHooks.GetScrollCompletedResult(args);

            this.fullLogs.Add(senderId + " ScrollCompleted. OffsetsChangeId=" + args.ScrollInfo.OffsetsChangeId + ", Result=" + result);
            chkLogUpdated.IsChecked = false;
        }

        private void ScrollingPresenter_ZoomCompleted(ScrollingPresenter sender, ZoomCompletedEventArgs args)
        {
            string senderId = "." + sender.Name;
            FrameworkElement parent = VisualTreeHelper.GetParent(sender) as FrameworkElement;
            if (parent != null)
            {
                senderId = parent.Name + senderId;
            }

            ScrollingPresenterViewChangeResult result = ScrollingPresenterTestHooks.GetZoomCompletedResult(args);

            this.fullLogs.Add(senderId + " ZoomCompleted. ZoomFactorChangeId=" + args.ZoomInfo.ZoomFactorChangeId + ", Result=" + result);
            chkLogUpdated.IsChecked = false;

            if (args.ZoomInfo.ZoomFactorChangeId == scrollingView52ZoomFactorChangeId)
            {
                this.txtResetStatus.Text = "Views reset";
                scrollingView52ZoomFactorChangeId = -1;
            }
        }

        private void CmbShowScrollingView_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (this.scrollingView11 != null)
            {
                if (cmbShowScrollingView.SelectedIndex == 0)
                {
                    this.scrollingView11.Visibility = Visibility.Visible;
                    this.scrollingView21.Visibility = Visibility.Visible;
                    this.scrollingView31.Visibility = Visibility.Visible;
                    this.scrollingView41.Visibility = Visibility.Visible;
                    this.scrollingView51.Visibility = Visibility.Visible;
                    this.scrollingView12.Visibility = Visibility.Visible;
                    this.scrollingView22.Visibility = Visibility.Visible;
                    this.scrollingView32.Visibility = Visibility.Visible;
                    this.scrollingView42.Visibility = Visibility.Visible;
                    this.scrollingView52.Visibility = Visibility.Visible;

                    this.scrollingView11.Width = double.NaN;
                    this.scrollingView21.Width = double.NaN;
                    this.scrollingView31.Width = double.NaN;
                    this.scrollingView41.Width = double.NaN;
                    this.scrollingView51.Width = double.NaN;
                    this.scrollingView12.Width = double.NaN;
                    this.scrollingView22.Width = double.NaN;
                    this.scrollingView32.Width = double.NaN;
                    this.scrollingView42.Width = double.NaN;
                    this.scrollingView52.Width = double.NaN;
                    this.scrollingView11.Height = double.NaN;
                    this.scrollingView21.Height = double.NaN;
                    this.scrollingView31.Height = double.NaN;
                    this.scrollingView41.Height = double.NaN;
                    this.scrollingView51.Height = double.NaN;
                    this.scrollingView12.Height = double.NaN;
                    this.scrollingView22.Height = double.NaN;
                    this.scrollingView32.Height = double.NaN;
                    this.scrollingView42.Height = double.NaN;
                    this.scrollingView52.Height = double.NaN;

                    for (int rowIndex = 2; rowIndex < 4; rowIndex++)
                        this.rootGrid.RowDefinitions[rowIndex].Height = new GridLength(1, GridUnitType.Star);

                    for (int columnIndex = 0; columnIndex < 5; columnIndex++)
                        this.rootGrid.ColumnDefinitions[columnIndex].Width = new GridLength(1, GridUnitType.Star);

                    cmbIgnoredInputKind.IsEnabled = false;
                    cmbIgnoredInputKind.SelectedIndex = 0;
                }
                else
                {
                    this.scrollingView11.Visibility = Visibility.Collapsed;
                    this.scrollingView21.Visibility = Visibility.Collapsed;
                    this.scrollingView31.Visibility = Visibility.Collapsed;
                    this.scrollingView41.Visibility = Visibility.Collapsed;
                    this.scrollingView51.Visibility = Visibility.Collapsed;
                    this.scrollingView12.Visibility = Visibility.Collapsed;
                    this.scrollingView22.Visibility = Visibility.Collapsed;
                    this.scrollingView32.Visibility = Visibility.Collapsed;
                    this.scrollingView42.Visibility = Visibility.Collapsed;
                    this.scrollingView52.Visibility = Visibility.Collapsed;

                    for (int rowIndex = 2; rowIndex < 4; rowIndex++)
                        this.rootGrid.RowDefinitions[rowIndex].Height = GridLength.Auto;

                    for (int columnIndex = 0; columnIndex < 5; columnIndex++)
                        this.rootGrid.ColumnDefinitions[columnIndex].Width = GridLength.Auto;

                    cmbIgnoredInputKind.IsEnabled = true;

                    ScrollingView scrollingView = SelectedScrollingView;

                    scrollingView.Visibility = Visibility.Visible;
                    scrollingView.Width = 300;
                    scrollingView.Height = 400;

                    txtScrollingPresenterHorizontalOffset.Text = scrollingView.HorizontalOffset.ToString();
                    txtScrollingPresenterVerticalOffset.Text = scrollingView.VerticalOffset.ToString();
                    txtScrollingPresenterZoomFactor.Text = scrollingView.ZoomFactor.ToString();

                    switch (scrollingView.IgnoredInputKind)
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
            ScrollingView scrollingView = SelectedScrollingView;

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

            scrollingView.IgnoredInputKind = ignoredInputKind;
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
            ResetView(this.scrollingView11);
            ResetView(this.scrollingView21);
            ResetView(this.scrollingView31);
            ResetView(this.scrollingView41);
            ResetView(this.scrollingView51);
            ResetView(this.scrollingView12);
            ResetView(this.scrollingView22);
            ResetView(this.scrollingView32);
            ResetView(this.scrollingView42);
            ResetView(this.scrollingView52);
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

        private void ResetView(ScrollingView scrollingView)
        {
            ScrollingPresenter scrollingPresenter = ScrollingViewTestHooks.GetScrollingPresenterPart(scrollingView);
            string scrollingPresenterId = (VisualTreeHelper.GetParent(scrollingPresenter) as FrameworkElement).Name + "." + scrollingPresenter.Name;

            int viewChangeId = scrollingPresenter.ScrollTo(0.0, 0.0, new ScrollOptions(AnimationMode.Disabled, SnapPointsMode.Ignore)).OffsetsChangeId;
            this.fullLogs.Add(scrollingPresenterId + " ScrollTo requested. Id=" + viewChangeId);

            viewChangeId = scrollingPresenter.ZoomTo(1.0f, System.Numerics.Vector2.Zero, new ZoomOptions(AnimationMode.Disabled, SnapPointsMode.Ignore)).ZoomFactorChangeId;
            this.fullLogs.Add(scrollingPresenterId + " ZoomTo requested. Id=" + viewChangeId);

            chkLogUpdated.IsChecked = false;

            if (scrollingView == this.scrollingView52)
                scrollingView52ZoomFactorChangeId = viewChangeId;
        }

        private ScrollingView SelectedScrollingView
        {
            get
            {
                ScrollingView scrollingView = null;

                switch (cmbShowScrollingView.SelectedIndex)
                {
                    case 1:
                        scrollingView = this.scrollingView11;
                        break;
                    case 2:
                        scrollingView = this.scrollingView21;
                        break;
                    case 3:
                        scrollingView = this.scrollingView31;
                        break;
                    case 4:
                        scrollingView = this.scrollingView41;
                        break;
                    case 5:
                        scrollingView = this.scrollingView51;
                        break;
                    case 6:
                        scrollingView = this.scrollingView12;
                        break;
                    case 7:
                        scrollingView = this.scrollingView22;
                        break;
                    case 8:
                        scrollingView = this.scrollingView32;
                        break;
                    case 9:
                        scrollingView = this.scrollingView42;
                        break;
                    case 10:
                        scrollingView = this.scrollingView52;
                        break;
                }

                return scrollingView;
            }
        }
    }
}
