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
using ScrollerViewChangeCompletedEventArgs = Microsoft.UI.Xaml.Controls.ScrollerViewChangeCompletedEventArgs;
using ScrollerChangeOffsetsOptions = Microsoft.UI.Xaml.Controls.ScrollerChangeOffsetsOptions;
using ScrollerViewKind = Microsoft.UI.Xaml.Controls.ScrollerViewKind;
using ScrollerViewChangeKind = Microsoft.UI.Xaml.Controls.ScrollerViewChangeKind;
using ScrollerViewChangeSnapPointRespect = Microsoft.UI.Xaml.Controls.ScrollerViewChangeSnapPointRespect;
using ScrollerChangeZoomFactorOptions = Microsoft.UI.Xaml.Controls.ScrollerChangeZoomFactorOptions;
using ScrollViewerTestHooks = Microsoft.UI.Private.Controls.ScrollViewerTestHooks;

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
            ScrollViewerTestHooks.GetScrollerPart(this.scrollViewer11).ViewChangeCompleted += Scroller_ViewChangeCompleted;
            ScrollViewerTestHooks.GetScrollerPart(this.scrollViewer21).ViewChangeCompleted += Scroller_ViewChangeCompleted;
            ScrollViewerTestHooks.GetScrollerPart(this.scrollViewer31).ViewChangeCompleted += Scroller_ViewChangeCompleted;
            ScrollViewerTestHooks.GetScrollerPart(this.scrollViewer41).ViewChangeCompleted += Scroller_ViewChangeCompleted;
            ScrollViewerTestHooks.GetScrollerPart(this.scrollViewer51).ViewChangeCompleted += Scroller_ViewChangeCompleted;

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
            ScrollViewerTestHooks.GetScrollerPart(this.scrollViewer12).ViewChangeCompleted += Scroller_ViewChangeCompleted;
            ScrollViewerTestHooks.GetScrollerPart(this.scrollViewer22).ViewChangeCompleted += Scroller_ViewChangeCompleted;
            ScrollViewerTestHooks.GetScrollerPart(this.scrollViewer32).ViewChangeCompleted += Scroller_ViewChangeCompleted;
            ScrollViewerTestHooks.GetScrollerPart(this.scrollViewer42).ViewChangeCompleted += Scroller_ViewChangeCompleted;
            ScrollViewerTestHooks.GetScrollerPart(this.scrollViewer52).ViewChangeCompleted += Scroller_ViewChangeCompleted;
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
        }

        private void Scroller_ViewChangeCompleted(Scroller sender, ScrollerViewChangeCompletedEventArgs args)
        {
            string senderId = "." + sender.Name;
            FrameworkElement parent = VisualTreeHelper.GetParent(sender) as FrameworkElement;
            if (parent != null)
            {
                senderId = parent.Name + senderId;
            }

            this.fullLogs.Add(senderId + " View change completed. ViewChangeId=" + args.ViewChangeId + ", Result=" + args.Result);

            if (args.ViewChangeId == scrollViewer52ZoomFactorChangeId)
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

        private void ResetView(ScrollViewer scrollViewer)
        {
            Scroller scroller = ScrollViewerTestHooks.GetScrollerPart(scrollViewer);
            string scrollerId = (VisualTreeHelper.GetParent(scroller) as FrameworkElement).Name + "." + scroller.Name;

            int viewChangeId = scroller.ChangeOffsets(new ScrollerChangeOffsetsOptions(0.0, 0.0, ScrollerViewKind.Absolute, ScrollerViewChangeKind.DisableAnimation, ScrollerViewChangeSnapPointRespect.IgnoreSnapPoints));
            this.fullLogs.Add(scrollerId + " ChangeOffsets requested. Id=" + viewChangeId);

            viewChangeId = scroller.ChangeZoomFactor(new ScrollerChangeZoomFactorOptions(1.0f, ScrollerViewKind.Absolute, System.Numerics.Vector2.Zero, ScrollerViewChangeKind.DisableAnimation, ScrollerViewChangeSnapPointRespect.IgnoreSnapPoints));
            this.fullLogs.Add(scrollerId + " ChangeZoomFactor requested. Id=" + viewChangeId);

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
