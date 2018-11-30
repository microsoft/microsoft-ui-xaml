// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.Generic;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;

#if !BUILD_WINDOWS
using Scroller = Microsoft.UI.Xaml.Controls.Scroller;
using ScrollerView = Microsoft.UI.Xaml.Controls.ScrollerView;
using ScrollerViewChangeCompletedEventArgs = Microsoft.UI.Xaml.Controls.ScrollerViewChangeCompletedEventArgs;
using ScrollerChangeOffsetsOptions = Microsoft.UI.Xaml.Controls.ScrollerChangeOffsetsOptions;
using ScrollerViewKind = Microsoft.UI.Xaml.Controls.ScrollerViewKind;
using ScrollerViewChangeKind = Microsoft.UI.Xaml.Controls.ScrollerViewChangeKind;
using ScrollerViewChangeSnapPointRespect = Microsoft.UI.Xaml.Controls.ScrollerViewChangeSnapPointRespect;
using ScrollerChangeZoomFactorOptions = Microsoft.UI.Xaml.Controls.ScrollerChangeZoomFactorOptions;
using ScrollerViewTestHooks = Microsoft.UI.Private.Controls.ScrollerViewTestHooks;
#endif

namespace MUXControlsTestApp
{
    public sealed partial class ScrollerViewsWithSimpleContentsPage : TestPage
    {
        private List<string> fullLogs = new List<string>();
        private int scrollerView52ZoomFactorChangeId = -1;

        public ScrollerViewsWithSimpleContentsPage()
        {
            this.InitializeComponent();

            this.scrollerView51.XYFocusKeyboardNavigation = XYFocusKeyboardNavigationMode.Enabled;

            Loaded += ScrollerViewsWithSimpleContentsPage_Loaded;
        }

        private void ScrollerViewsWithSimpleContentsPage_Loaded(object sender, RoutedEventArgs e)
        {
            ScrollerViewTestHooks.GetScrollerPart(this.scrollerView11).StateChanged += Scroller_StateChanged;
            ScrollerViewTestHooks.GetScrollerPart(this.scrollerView21).StateChanged += Scroller_StateChanged;
            ScrollerViewTestHooks.GetScrollerPart(this.scrollerView31).StateChanged += Scroller_StateChanged;
            ScrollerViewTestHooks.GetScrollerPart(this.scrollerView41).StateChanged += Scroller_StateChanged;
            ScrollerViewTestHooks.GetScrollerPart(this.scrollerView51).StateChanged += Scroller_StateChanged;
            ScrollerViewTestHooks.GetScrollerPart(this.scrollerView11).ViewChanged += Scroller_ViewChanged;
            ScrollerViewTestHooks.GetScrollerPart(this.scrollerView21).ViewChanged += Scroller_ViewChanged;
            ScrollerViewTestHooks.GetScrollerPart(this.scrollerView31).ViewChanged += Scroller_ViewChanged;
            ScrollerViewTestHooks.GetScrollerPart(this.scrollerView41).ViewChanged += Scroller_ViewChanged;
            ScrollerViewTestHooks.GetScrollerPart(this.scrollerView51).ViewChanged += Scroller_ViewChanged;
            ScrollerViewTestHooks.GetScrollerPart(this.scrollerView11).ViewChangeCompleted += Scroller_ViewChangeCompleted;
            ScrollerViewTestHooks.GetScrollerPart(this.scrollerView21).ViewChangeCompleted += Scroller_ViewChangeCompleted;
            ScrollerViewTestHooks.GetScrollerPart(this.scrollerView31).ViewChangeCompleted += Scroller_ViewChangeCompleted;
            ScrollerViewTestHooks.GetScrollerPart(this.scrollerView41).ViewChangeCompleted += Scroller_ViewChangeCompleted;
            ScrollerViewTestHooks.GetScrollerPart(this.scrollerView51).ViewChangeCompleted += Scroller_ViewChangeCompleted;

            ScrollerViewTestHooks.GetScrollerPart(this.scrollerView12).StateChanged += Scroller_StateChanged;
            ScrollerViewTestHooks.GetScrollerPart(this.scrollerView22).StateChanged += Scroller_StateChanged;
            ScrollerViewTestHooks.GetScrollerPart(this.scrollerView32).StateChanged += Scroller_StateChanged;
            ScrollerViewTestHooks.GetScrollerPart(this.scrollerView42).StateChanged += Scroller_StateChanged;
            ScrollerViewTestHooks.GetScrollerPart(this.scrollerView52).StateChanged += Scroller_StateChanged;
            ScrollerViewTestHooks.GetScrollerPart(this.scrollerView12).ViewChanged += Scroller_ViewChanged;
            ScrollerViewTestHooks.GetScrollerPart(this.scrollerView22).ViewChanged += Scroller_ViewChanged;
            ScrollerViewTestHooks.GetScrollerPart(this.scrollerView32).ViewChanged += Scroller_ViewChanged;
            ScrollerViewTestHooks.GetScrollerPart(this.scrollerView42).ViewChanged += Scroller_ViewChanged;
            ScrollerViewTestHooks.GetScrollerPart(this.scrollerView52).ViewChanged += Scroller_ViewChanged;
            ScrollerViewTestHooks.GetScrollerPart(this.scrollerView12).ViewChangeCompleted += Scroller_ViewChangeCompleted;
            ScrollerViewTestHooks.GetScrollerPart(this.scrollerView22).ViewChangeCompleted += Scroller_ViewChangeCompleted;
            ScrollerViewTestHooks.GetScrollerPart(this.scrollerView32).ViewChangeCompleted += Scroller_ViewChangeCompleted;
            ScrollerViewTestHooks.GetScrollerPart(this.scrollerView42).ViewChangeCompleted += Scroller_ViewChangeCompleted;
            ScrollerViewTestHooks.GetScrollerPart(this.scrollerView52).ViewChangeCompleted += Scroller_ViewChangeCompleted;
        }

        private void Scroller_StateChanged(Scroller sender, object args)
        {
            FrameworkElement parent = VisualTreeHelper.GetParent(sender) as FrameworkElement;
            FrameworkElement grandParent = VisualTreeHelper.GetParent(parent) as FrameworkElement;
            string senderId = grandParent.Name + "." + parent.Name + "." + sender.Name;
            this.txtScrollerState.Text = senderId + " " + sender.State.ToString();
            this.fullLogs.Add(senderId + " StateChanged S=" + sender.State.ToString());
        }

        private void Scroller_ViewChanged(Scroller sender, object args)
        {
            string senderId = (VisualTreeHelper.GetParent(sender) as FrameworkElement).Name + "." + sender.Name;
            this.txtScrollerHorizontalOffset.Text = sender.HorizontalOffset.ToString();
            this.txtScrollerVerticalOffset.Text = sender.VerticalOffset.ToString();
            this.txtScrollerZoomFactor.Text = sender.ZoomFactor.ToString();
            this.fullLogs.Add(senderId + " ViewChanged H=" + this.txtScrollerHorizontalOffset.Text + ", V=" + this.txtScrollerVerticalOffset.Text + ", ZF=" + this.txtScrollerZoomFactor.Text);
        }

        private void Scroller_ViewChangeCompleted(Scroller sender, ScrollerViewChangeCompletedEventArgs args)
        {
            string senderId = (VisualTreeHelper.GetParent(sender) as FrameworkElement).Name + "." + sender.Name;

            this.fullLogs.Add(senderId + " View change completed. ViewChangeId=" + args.ViewChangeId + ", Result=" + args.Result);

            if (args.ViewChangeId == scrollerView52ZoomFactorChangeId)
            {
                this.txtResetStatus.Text = "Views reset";
                scrollerView52ZoomFactorChangeId = -1;
            }
        }

        private void CmbShowScrollerView_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (this.scrollerView11 != null)
            {
                if (cmbShowScrollerView.SelectedIndex == 0)
                {
                    this.scrollerView11.Visibility = Visibility.Visible;
                    this.scrollerView21.Visibility = Visibility.Visible;
                    this.scrollerView31.Visibility = Visibility.Visible;
                    this.scrollerView41.Visibility = Visibility.Visible;
                    this.scrollerView51.Visibility = Visibility.Visible;
                    this.scrollerView12.Visibility = Visibility.Visible;
                    this.scrollerView22.Visibility = Visibility.Visible;
                    this.scrollerView32.Visibility = Visibility.Visible;
                    this.scrollerView42.Visibility = Visibility.Visible;
                    this.scrollerView52.Visibility = Visibility.Visible;

                    this.scrollerView11.Width = double.NaN;
                    this.scrollerView21.Width = double.NaN;
                    this.scrollerView31.Width = double.NaN;
                    this.scrollerView41.Width = double.NaN;
                    this.scrollerView51.Width = double.NaN;
                    this.scrollerView12.Width = double.NaN;
                    this.scrollerView22.Width = double.NaN;
                    this.scrollerView32.Width = double.NaN;
                    this.scrollerView42.Width = double.NaN;
                    this.scrollerView52.Width = double.NaN;
                    this.scrollerView11.Height = double.NaN;
                    this.scrollerView21.Height = double.NaN;
                    this.scrollerView31.Height = double.NaN;
                    this.scrollerView41.Height = double.NaN;
                    this.scrollerView51.Height = double.NaN;
                    this.scrollerView12.Height = double.NaN;
                    this.scrollerView22.Height = double.NaN;
                    this.scrollerView32.Height = double.NaN;
                    this.scrollerView42.Height = double.NaN;
                    this.scrollerView52.Height = double.NaN;

                    for (int rowIndex = 2; rowIndex < 4; rowIndex++)
                        this.rootGrid.RowDefinitions[rowIndex].Height = new GridLength(1, GridUnitType.Star);

                    for (int columnIndex = 0; columnIndex < 5; columnIndex++)
                        this.rootGrid.ColumnDefinitions[columnIndex].Width = new GridLength(1, GridUnitType.Star);
                }
                else
                {
                    this.scrollerView11.Visibility = Visibility.Collapsed;
                    this.scrollerView21.Visibility = Visibility.Collapsed;
                    this.scrollerView31.Visibility = Visibility.Collapsed;
                    this.scrollerView41.Visibility = Visibility.Collapsed;
                    this.scrollerView51.Visibility = Visibility.Collapsed;
                    this.scrollerView12.Visibility = Visibility.Collapsed;
                    this.scrollerView22.Visibility = Visibility.Collapsed;
                    this.scrollerView32.Visibility = Visibility.Collapsed;
                    this.scrollerView42.Visibility = Visibility.Collapsed;
                    this.scrollerView52.Visibility = Visibility.Collapsed;

                    for (int rowIndex = 2; rowIndex < 4; rowIndex++)
                        this.rootGrid.RowDefinitions[rowIndex].Height = GridLength.Auto;

                    for (int columnIndex = 0; columnIndex < 5; columnIndex++)
                        this.rootGrid.ColumnDefinitions[columnIndex].Width = GridLength.Auto;

                    ScrollerView scrollerView = null;

                    switch (cmbShowScrollerView.SelectedIndex)
                    {
                        case 1:
                            scrollerView = this.scrollerView11;
                            break;
                        case 2:
                            scrollerView = this.scrollerView21;
                            break;
                        case 3:
                            scrollerView = this.scrollerView31;
                            break;
                        case 4:
                            scrollerView = this.scrollerView41;
                            break;
                        case 5:
                            scrollerView = this.scrollerView51;
                            break;
                        case 6:
                            scrollerView = this.scrollerView12;
                            break;
                        case 7:
                            scrollerView = this.scrollerView22;
                            break;
                        case 8:
                            scrollerView = this.scrollerView32;
                            break;
                        case 9:
                            scrollerView = this.scrollerView42;
                            break;
                        case 10:
                            scrollerView = this.scrollerView52;
                            break;
                    }

                    scrollerView.Visibility = Visibility.Visible;
                    scrollerView.Width = 300;
                    scrollerView.Height = 400;

                    txtScrollerHorizontalOffset.Text = scrollerView.HorizontalOffset.ToString();
                    txtScrollerVerticalOffset.Text = scrollerView.VerticalOffset.ToString();
                    txtScrollerZoomFactor.Text = scrollerView.ZoomFactor.ToString();
                }
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

        private void btnResetViews_Click(object sender, RoutedEventArgs e)
        {
            this.txtResetStatus.Text = "Resetting views ...";
            ResetView(this.scrollerView11);
            ResetView(this.scrollerView21);
            ResetView(this.scrollerView31);
            ResetView(this.scrollerView41);
            ResetView(this.scrollerView51);
            ResetView(this.scrollerView12);
            ResetView(this.scrollerView22);
            ResetView(this.scrollerView32);
            ResetView(this.scrollerView42);
            ResetView(this.scrollerView52);
        }

        private void ResetView(ScrollerView scrollerView)
        {
            Scroller scroller = ScrollerViewTestHooks.GetScrollerPart(scrollerView);
            string scrollerId = (VisualTreeHelper.GetParent(scroller) as FrameworkElement).Name + "." + scroller.Name;

            int viewChangeId = scroller.ChangeOffsets(new ScrollerChangeOffsetsOptions(0.0, 0.0, ScrollerViewKind.Absolute, ScrollerViewChangeKind.DisableAnimation, ScrollerViewChangeSnapPointRespect.IgnoreSnapPoints));
            this.fullLogs.Add(scrollerId + " ChangeOffsets requested. Id=" + viewChangeId);

            viewChangeId = scroller.ChangeZoomFactor(new ScrollerChangeZoomFactorOptions(1.0f, ScrollerViewKind.Absolute, System.Numerics.Vector2.Zero, ScrollerViewChangeKind.DisableAnimation, ScrollerViewChangeSnapPointRespect.IgnoreSnapPoints));
            this.fullLogs.Add(scrollerId + " ChangeZoomFactor requested. Id=" + viewChangeId);

            if (scrollerView == this.scrollerView52)
                scrollerView52ZoomFactorChangeId = viewChangeId;
        }
    }
}
