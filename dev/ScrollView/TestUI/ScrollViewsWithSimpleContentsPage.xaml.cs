// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.Generic;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;

using InputKind = Microsoft.UI.Xaml.Controls.InputKind;
using ScrollPresenter = Microsoft.UI.Xaml.Controls.Primitives.ScrollPresenter;
using ScrollView = Microsoft.UI.Xaml.Controls.ScrollView;
using ScrollingScrollCompletedEventArgs = Microsoft.UI.Xaml.Controls.ScrollingScrollCompletedEventArgs;
using ScrollingZoomCompletedEventArgs = Microsoft.UI.Xaml.Controls.ScrollingZoomCompletedEventArgs;
using ScrollingScrollOptions = Microsoft.UI.Xaml.Controls.ScrollingScrollOptions;
using ScrollingZoomOptions = Microsoft.UI.Xaml.Controls.ScrollingZoomOptions;
using AnimationMode = Microsoft.UI.Xaml.Controls.AnimationMode;
using SnapPointsMode = Microsoft.UI.Xaml.Controls.SnapPointsMode;

using ScrollPresenterTestHooks = Microsoft.UI.Private.Controls.ScrollPresenterTestHooks;
using ScrollPresenterViewChangeResult = Microsoft.UI.Private.Controls.ScrollPresenterViewChangeResult;
using ScrollViewTestHooks = Microsoft.UI.Private.Controls.ScrollViewTestHooks;
using MUXControlsTestHooks = Microsoft.UI.Private.Controls.MUXControlsTestHooks;
using MUXControlsTestHooksLoggingMessageEventArgs = Microsoft.UI.Private.Controls.MUXControlsTestHooksLoggingMessageEventArgs;

namespace MUXControlsTestApp
{
    public sealed partial class ScrollViewsWithSimpleContentsPage : TestPage
    {
        private List<string> fullLogs = new List<string>();
        private int scrollView52ZoomFactorChangeId = -1;

        public ScrollViewsWithSimpleContentsPage()
        {
            this.InitializeComponent();

            this.scrollView51.XYFocusKeyboardNavigation = XYFocusKeyboardNavigationMode.Enabled;

            Loaded += ScrollViewsWithSimpleContentsPage_Loaded;
            KeyDown += ScrollViewsWithSimpleContentsPage_KeyDown;
        }

        private void ScrollViewsWithSimpleContentsPage_Loaded(object sender, RoutedEventArgs e)
        {
            ScrollViewTestHooks.GetScrollPresenterPart(this.scrollView11).StateChanged += ScrollPresenter_StateChanged;
            ScrollViewTestHooks.GetScrollPresenterPart(this.scrollView21).StateChanged += ScrollPresenter_StateChanged;
            ScrollViewTestHooks.GetScrollPresenterPart(this.scrollView31).StateChanged += ScrollPresenter_StateChanged;
            ScrollViewTestHooks.GetScrollPresenterPart(this.scrollView41).StateChanged += ScrollPresenter_StateChanged;
            ScrollViewTestHooks.GetScrollPresenterPart(this.scrollView51).StateChanged += ScrollPresenter_StateChanged;
            ScrollViewTestHooks.GetScrollPresenterPart(this.scrollView11).ViewChanged += ScrollPresenter_ViewChanged;
            ScrollViewTestHooks.GetScrollPresenterPart(this.scrollView21).ViewChanged += ScrollPresenter_ViewChanged;
            ScrollViewTestHooks.GetScrollPresenterPart(this.scrollView31).ViewChanged += ScrollPresenter_ViewChanged;
            ScrollViewTestHooks.GetScrollPresenterPart(this.scrollView41).ViewChanged += ScrollPresenter_ViewChanged;
            ScrollViewTestHooks.GetScrollPresenterPart(this.scrollView51).ViewChanged += ScrollPresenter_ViewChanged;
            ScrollViewTestHooks.GetScrollPresenterPart(this.scrollView11).ScrollCompleted += ScrollPresenter_ScrollCompleted;
            ScrollViewTestHooks.GetScrollPresenterPart(this.scrollView21).ScrollCompleted += ScrollPresenter_ScrollCompleted;
            ScrollViewTestHooks.GetScrollPresenterPart(this.scrollView31).ScrollCompleted += ScrollPresenter_ScrollCompleted;
            ScrollViewTestHooks.GetScrollPresenterPart(this.scrollView41).ScrollCompleted += ScrollPresenter_ScrollCompleted;
            ScrollViewTestHooks.GetScrollPresenterPart(this.scrollView51).ScrollCompleted += ScrollPresenter_ScrollCompleted;
            ScrollViewTestHooks.GetScrollPresenterPart(this.scrollView11).ZoomCompleted += ScrollPresenter_ZoomCompleted;
            ScrollViewTestHooks.GetScrollPresenterPart(this.scrollView21).ZoomCompleted += ScrollPresenter_ZoomCompleted;
            ScrollViewTestHooks.GetScrollPresenterPart(this.scrollView31).ZoomCompleted += ScrollPresenter_ZoomCompleted;
            ScrollViewTestHooks.GetScrollPresenterPart(this.scrollView41).ZoomCompleted += ScrollPresenter_ZoomCompleted;
            ScrollViewTestHooks.GetScrollPresenterPart(this.scrollView51).ZoomCompleted += ScrollPresenter_ZoomCompleted;

            ScrollViewTestHooks.GetScrollPresenterPart(this.scrollView12).StateChanged += ScrollPresenter_StateChanged;
            ScrollViewTestHooks.GetScrollPresenterPart(this.scrollView22).StateChanged += ScrollPresenter_StateChanged;
            ScrollViewTestHooks.GetScrollPresenterPart(this.scrollView32).StateChanged += ScrollPresenter_StateChanged;
            ScrollViewTestHooks.GetScrollPresenterPart(this.scrollView42).StateChanged += ScrollPresenter_StateChanged;
            ScrollViewTestHooks.GetScrollPresenterPart(this.scrollView52).StateChanged += ScrollPresenter_StateChanged;
            ScrollViewTestHooks.GetScrollPresenterPart(this.scrollView12).ViewChanged += ScrollPresenter_ViewChanged;
            ScrollViewTestHooks.GetScrollPresenterPart(this.scrollView22).ViewChanged += ScrollPresenter_ViewChanged;
            ScrollViewTestHooks.GetScrollPresenterPart(this.scrollView32).ViewChanged += ScrollPresenter_ViewChanged;
            ScrollViewTestHooks.GetScrollPresenterPart(this.scrollView42).ViewChanged += ScrollPresenter_ViewChanged;
            ScrollViewTestHooks.GetScrollPresenterPart(this.scrollView52).ViewChanged += ScrollPresenter_ViewChanged;
            ScrollViewTestHooks.GetScrollPresenterPart(this.scrollView12).ScrollCompleted += ScrollPresenter_ScrollCompleted;
            ScrollViewTestHooks.GetScrollPresenterPart(this.scrollView22).ScrollCompleted += ScrollPresenter_ScrollCompleted;
            ScrollViewTestHooks.GetScrollPresenterPart(this.scrollView32).ScrollCompleted += ScrollPresenter_ScrollCompleted;
            ScrollViewTestHooks.GetScrollPresenterPart(this.scrollView42).ScrollCompleted += ScrollPresenter_ScrollCompleted;
            ScrollViewTestHooks.GetScrollPresenterPart(this.scrollView52).ScrollCompleted += ScrollPresenter_ScrollCompleted;
            ScrollViewTestHooks.GetScrollPresenterPart(this.scrollView12).ZoomCompleted += ScrollPresenter_ZoomCompleted;
            ScrollViewTestHooks.GetScrollPresenterPart(this.scrollView22).ZoomCompleted += ScrollPresenter_ZoomCompleted;
            ScrollViewTestHooks.GetScrollPresenterPart(this.scrollView32).ZoomCompleted += ScrollPresenter_ZoomCompleted;
            ScrollViewTestHooks.GetScrollPresenterPart(this.scrollView42).ZoomCompleted += ScrollPresenter_ZoomCompleted;
            ScrollViewTestHooks.GetScrollPresenterPart(this.scrollView52).ZoomCompleted += ScrollPresenter_ZoomCompleted;
        }

        private void ScrollViewsWithSimpleContentsPage_KeyDown(object sender, KeyRoutedEventArgs e)
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

        private void ChkLogScrollPresenterMessages_Checked(object sender, RoutedEventArgs e)
        {
            MUXControlsTestHooks.SetOutputDebugStringLevelForType(
                type: "ScrollPresenter",
                isLoggingInfoLevel: true,
                isLoggingVerboseLevel: true);

            MUXControlsTestHooks.SetLoggingLevelForType(
                type: "ScrollPresenter",
                isLoggingInfoLevel: true,
                isLoggingVerboseLevel: true);

            if (chkLogScrollViewMessages.IsChecked == false)
                MUXControlsTestHooks.LoggingMessage += MUXControlsTestHooks_LoggingMessage;
        }

        private void ChkLogScrollPresenterMessages_Unchecked(object sender, RoutedEventArgs e)
        {
            MUXControlsTestHooks.SetOutputDebugStringLevelForType(
                type: "ScrollPresenter",
                isLoggingInfoLevel: false,
                isLoggingVerboseLevel: false);

            MUXControlsTestHooks.SetLoggingLevelForType(
                type: "ScrollPresenter",
                isLoggingInfoLevel: false,
                isLoggingVerboseLevel: false);

            if (chkLogScrollViewMessages.IsChecked == false)
                MUXControlsTestHooks.LoggingMessage -= MUXControlsTestHooks_LoggingMessage;
        }

        private void ChkLogScrollViewMessages_Checked(object sender, RoutedEventArgs e)
        {
            MUXControlsTestHooks.SetOutputDebugStringLevelForType(
                type: "ScrollView",
                isLoggingInfoLevel: true,
                isLoggingVerboseLevel: true);

            MUXControlsTestHooks.SetLoggingLevelForType(
                type: "ScrollView",
                isLoggingInfoLevel: true,
                isLoggingVerboseLevel: true);

            if (chkLogScrollPresenterMessages.IsChecked == false)
                MUXControlsTestHooks.LoggingMessage += MUXControlsTestHooks_LoggingMessage;
        }

        private void ChkLogScrollViewMessages_Unchecked(object sender, RoutedEventArgs e)
        {
            MUXControlsTestHooks.SetOutputDebugStringLevelForType(
                type: "ScrollView",
                isLoggingInfoLevel: false,
                isLoggingVerboseLevel: false);

            MUXControlsTestHooks.SetLoggingLevelForType(
                type: "ScrollView",
                isLoggingInfoLevel: false,
                isLoggingVerboseLevel: false);

            if (chkLogScrollPresenterMessages.IsChecked == false)
                MUXControlsTestHooks.LoggingMessage -= MUXControlsTestHooks_LoggingMessage;
        }

        private void ScrollPresenter_StateChanged(ScrollPresenter sender, object args)
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
            this.txtScrollPresenterState.Text = senderId + " " + sender.State.ToString();
            this.fullLogs.Add(senderId + " StateChanged S=" + sender.State.ToString());
            chkLogUpdated.IsChecked = false;
        }

        private void ScrollPresenter_ViewChanged(ScrollPresenter sender, object args)
        {
            string senderId = "." + sender.Name;
            FrameworkElement parent = VisualTreeHelper.GetParent(sender) as FrameworkElement;
            if (parent != null)
            {
                senderId = parent.Name + senderId;
            }
            this.txtScrollPresenterHorizontalOffset.Text = sender.HorizontalOffset.ToString();
            this.txtScrollPresenterVerticalOffset.Text = sender.VerticalOffset.ToString();
            this.txtScrollPresenterZoomFactor.Text = sender.ZoomFactor.ToString();
            this.fullLogs.Add(senderId + " ViewChanged H=" + this.txtScrollPresenterHorizontalOffset.Text + ", V=" + this.txtScrollPresenterVerticalOffset.Text + ", ZF=" + this.txtScrollPresenterZoomFactor.Text);
            chkLogUpdated.IsChecked = false;
        }

        private void ScrollPresenter_ScrollCompleted(ScrollPresenter sender, ScrollingScrollCompletedEventArgs args)
        {
            string senderId = "." + sender.Name;
            FrameworkElement parent = VisualTreeHelper.GetParent(sender) as FrameworkElement;
            if (parent != null)
            {
                senderId = parent.Name + senderId;
            }

            ScrollPresenterViewChangeResult result = ScrollPresenterTestHooks.GetScrollCompletedResult(args);

            this.fullLogs.Add(senderId + " ScrollCompleted. OffsetsChangeId=" + args.ScrollInfo.OffsetsChangeId + ", Result=" + result);
            chkLogUpdated.IsChecked = false;
        }

        private void ScrollPresenter_ZoomCompleted(ScrollPresenter sender, ScrollingZoomCompletedEventArgs args)
        {
            string senderId = "." + sender.Name;
            FrameworkElement parent = VisualTreeHelper.GetParent(sender) as FrameworkElement;
            if (parent != null)
            {
                senderId = parent.Name + senderId;
            }

            ScrollPresenterViewChangeResult result = ScrollPresenterTestHooks.GetZoomCompletedResult(args);

            this.fullLogs.Add(senderId + " ZoomCompleted. ZoomFactorChangeId=" + args.ZoomInfo.ZoomFactorChangeId + ", Result=" + result);
            chkLogUpdated.IsChecked = false;

            if (args.ZoomInfo.ZoomFactorChangeId == scrollView52ZoomFactorChangeId)
            {
                this.txtResetStatus.Text = "Views reset";
                scrollView52ZoomFactorChangeId = -1;
            }
        }

        private void CmbShowScrollView_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (this.scrollView11 != null)
            {
                if (cmbShowScrollView.SelectedIndex == 0)
                {
                    this.scrollView11.Visibility = Visibility.Visible;
                    this.scrollView21.Visibility = Visibility.Visible;
                    this.scrollView31.Visibility = Visibility.Visible;
                    this.scrollView41.Visibility = Visibility.Visible;
                    this.scrollView51.Visibility = Visibility.Visible;
                    this.scrollView12.Visibility = Visibility.Visible;
                    this.scrollView22.Visibility = Visibility.Visible;
                    this.scrollView32.Visibility = Visibility.Visible;
                    this.scrollView42.Visibility = Visibility.Visible;
                    this.scrollView52.Visibility = Visibility.Visible;

                    this.scrollView11.Width = double.NaN;
                    this.scrollView21.Width = double.NaN;
                    this.scrollView31.Width = double.NaN;
                    this.scrollView41.Width = double.NaN;
                    this.scrollView51.Width = double.NaN;
                    this.scrollView12.Width = double.NaN;
                    this.scrollView22.Width = double.NaN;
                    this.scrollView32.Width = double.NaN;
                    this.scrollView42.Width = double.NaN;
                    this.scrollView52.Width = double.NaN;
                    this.scrollView11.Height = double.NaN;
                    this.scrollView21.Height = double.NaN;
                    this.scrollView31.Height = double.NaN;
                    this.scrollView41.Height = double.NaN;
                    this.scrollView51.Height = double.NaN;
                    this.scrollView12.Height = double.NaN;
                    this.scrollView22.Height = double.NaN;
                    this.scrollView32.Height = double.NaN;
                    this.scrollView42.Height = double.NaN;
                    this.scrollView52.Height = double.NaN;

                    for (int rowIndex = 2; rowIndex < 4; rowIndex++)
                        this.rootGrid.RowDefinitions[rowIndex].Height = new GridLength(1, GridUnitType.Star);

                    for (int columnIndex = 0; columnIndex < 5; columnIndex++)
                        this.rootGrid.ColumnDefinitions[columnIndex].Width = new GridLength(1, GridUnitType.Star);

                    cmbIgnoredInputKind.IsEnabled = false;
                    cmbIgnoredInputKind.SelectedIndex = 0;
                }
                else
                {
                    this.scrollView11.Visibility = Visibility.Collapsed;
                    this.scrollView21.Visibility = Visibility.Collapsed;
                    this.scrollView31.Visibility = Visibility.Collapsed;
                    this.scrollView41.Visibility = Visibility.Collapsed;
                    this.scrollView51.Visibility = Visibility.Collapsed;
                    this.scrollView12.Visibility = Visibility.Collapsed;
                    this.scrollView22.Visibility = Visibility.Collapsed;
                    this.scrollView32.Visibility = Visibility.Collapsed;
                    this.scrollView42.Visibility = Visibility.Collapsed;
                    this.scrollView52.Visibility = Visibility.Collapsed;

                    for (int rowIndex = 2; rowIndex < 4; rowIndex++)
                        this.rootGrid.RowDefinitions[rowIndex].Height = GridLength.Auto;

                    for (int columnIndex = 0; columnIndex < 5; columnIndex++)
                        this.rootGrid.ColumnDefinitions[columnIndex].Width = GridLength.Auto;

                    cmbIgnoredInputKind.IsEnabled = true;

                    ScrollView scrollView = SelectedScrollView;

                    scrollView.Visibility = Visibility.Visible;
                    scrollView.Width = 300;
                    scrollView.Height = 400;

                    txtScrollPresenterHorizontalOffset.Text = scrollView.HorizontalOffset.ToString();
                    txtScrollPresenterVerticalOffset.Text = scrollView.VerticalOffset.ToString();
                    txtScrollPresenterZoomFactor.Text = scrollView.ZoomFactor.ToString();

                    switch (scrollView.IgnoredInputKind)
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
            ScrollView scrollView = SelectedScrollView;

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

            scrollView.IgnoredInputKind = ignoredInputKind;
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
            ResetView(this.scrollView11);
            ResetView(this.scrollView21);
            ResetView(this.scrollView31);
            ResetView(this.scrollView41);
            ResetView(this.scrollView51);
            ResetView(this.scrollView12);
            ResetView(this.scrollView22);
            ResetView(this.scrollView32);
            ResetView(this.scrollView42);
            ResetView(this.scrollView52);
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

        private void ResetView(ScrollView scrollView)
        {
            ScrollPresenter scrollPresenter = ScrollViewTestHooks.GetScrollPresenterPart(scrollView);
            string scrollPresenterId = (VisualTreeHelper.GetParent(scrollPresenter) as FrameworkElement).Name + "." + scrollPresenter.Name;

            int viewChangeId = scrollPresenter.ScrollTo(0.0, 0.0, new ScrollingScrollOptions(AnimationMode.Disabled, SnapPointsMode.Ignore)).OffsetsChangeId;
            this.fullLogs.Add(scrollPresenterId + " ScrollTo requested. Id=" + viewChangeId);

            viewChangeId = scrollPresenter.ZoomTo(1.0f, System.Numerics.Vector2.Zero, new ScrollingZoomOptions(AnimationMode.Disabled, SnapPointsMode.Ignore)).ZoomFactorChangeId;
            this.fullLogs.Add(scrollPresenterId + " ZoomTo requested. Id=" + viewChangeId);

            chkLogUpdated.IsChecked = false;

            if (scrollView == this.scrollView52)
                scrollView52ZoomFactorChangeId = viewChangeId;
        }

        private ScrollView SelectedScrollView
        {
            get
            {
                ScrollView scrollView = null;

                switch (cmbShowScrollView.SelectedIndex)
                {
                    case 1:
                        scrollView = this.scrollView11;
                        break;
                    case 2:
                        scrollView = this.scrollView21;
                        break;
                    case 3:
                        scrollView = this.scrollView31;
                        break;
                    case 4:
                        scrollView = this.scrollView41;
                        break;
                    case 5:
                        scrollView = this.scrollView51;
                        break;
                    case 6:
                        scrollView = this.scrollView12;
                        break;
                    case 7:
                        scrollView = this.scrollView22;
                        break;
                    case 8:
                        scrollView = this.scrollView32;
                        break;
                    case 9:
                        scrollView = this.scrollView42;
                        break;
                    case 10:
                        scrollView = this.scrollView52;
                        break;
                }

                return scrollView;
            }
        }
    }
}
