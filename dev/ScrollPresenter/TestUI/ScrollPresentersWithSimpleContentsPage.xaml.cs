// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.Generic;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;

using ScrollPresenter = Microsoft.UI.Xaml.Controls.Primitives.ScrollPresenter;
using ScrollingInteractionState = Microsoft.UI.Xaml.Controls.ScrollingInteractionState;
using ScrollingAnimationMode = Microsoft.UI.Xaml.Controls.ScrollingAnimationMode;
using ScrollingSnapPointsMode = Microsoft.UI.Xaml.Controls.ScrollingSnapPointsMode;
using ScrollingScrollOptions = Microsoft.UI.Xaml.Controls.ScrollingScrollOptions;
using ScrollingZoomOptions = Microsoft.UI.Xaml.Controls.ScrollingZoomOptions;
using ScrollingScrollCompletedEventArgs = Microsoft.UI.Xaml.Controls.ScrollingScrollCompletedEventArgs;
using ScrollingZoomCompletedEventArgs = Microsoft.UI.Xaml.Controls.ScrollingZoomCompletedEventArgs;

using ScrollPresenterTestHooks = Microsoft.UI.Private.Controls.ScrollPresenterTestHooks;
using ScrollPresenterViewChangeResult = Microsoft.UI.Private.Controls.ScrollPresenterViewChangeResult;
using MUXControlsTestHooks = Microsoft.UI.Private.Controls.MUXControlsTestHooks;
using MUXControlsTestHooksLoggingMessageEventArgs = Microsoft.UI.Private.Controls.MUXControlsTestHooksLoggingMessageEventArgs;

namespace MUXControlsTestApp
{
    public sealed partial class ScrollPresentersWithSimpleContentsPage : TestPage
    {
        private List<string> fullLogs = new List<string>();
        private int scrollPresenter52ZoomFactorChangeCorrelationId = -1;
        private bool canScrollPresenter51ContentShrink = true;

        public ScrollPresentersWithSimpleContentsPage()
        {
            this.InitializeComponent();

            this.scrollPresenter11.StateChanged += ScrollPresenter_StateChanged;
            this.scrollPresenter21.StateChanged += ScrollPresenter_StateChanged;
            this.scrollPresenter31.StateChanged += ScrollPresenter_StateChanged;
            this.scrollPresenter41.StateChanged += ScrollPresenter_StateChanged;
            this.scrollPresenter51.StateChanged += ScrollPresenter_StateChanged;
            this.scrollPresenter11.ViewChanged += ScrollPresenter_ViewChanged;
            this.scrollPresenter21.ViewChanged += ScrollPresenter_ViewChanged;
            this.scrollPresenter31.ViewChanged += ScrollPresenter_ViewChanged;
            this.scrollPresenter41.ViewChanged += ScrollPresenter_ViewChanged;
            this.scrollPresenter51.ViewChanged += ScrollPresenter_ViewChanged;
            this.scrollPresenter11.ScrollCompleted += ScrollPresenter_ScrollCompleted;
            this.scrollPresenter21.ScrollCompleted += ScrollPresenter_ScrollCompleted;
            this.scrollPresenter31.ScrollCompleted += ScrollPresenter_ScrollCompleted;
            this.scrollPresenter41.ScrollCompleted += ScrollPresenter_ScrollCompleted;
            this.scrollPresenter51.ScrollCompleted += ScrollPresenter_ScrollCompleted;
            this.scrollPresenter11.ZoomCompleted += ScrollPresenter_ZoomCompleted;
            this.scrollPresenter21.ZoomCompleted += ScrollPresenter_ZoomCompleted;
            this.scrollPresenter31.ZoomCompleted += ScrollPresenter_ZoomCompleted;
            this.scrollPresenter41.ZoomCompleted += ScrollPresenter_ZoomCompleted;
            this.scrollPresenter51.ZoomCompleted += ScrollPresenter_ZoomCompleted;

            this.scrollPresenter12.StateChanged += ScrollPresenter_StateChanged;
            this.scrollPresenter22.StateChanged += ScrollPresenter_StateChanged;
            this.scrollPresenter32.StateChanged += ScrollPresenter_StateChanged;
            this.scrollPresenter42.StateChanged += ScrollPresenter_StateChanged;
            this.scrollPresenter52.StateChanged += ScrollPresenter_StateChanged;
            this.scrollPresenter12.ViewChanged += ScrollPresenter_ViewChanged;
            this.scrollPresenter22.ViewChanged += ScrollPresenter_ViewChanged;
            this.scrollPresenter32.ViewChanged += ScrollPresenter_ViewChanged;
            this.scrollPresenter42.ViewChanged += ScrollPresenter_ViewChanged;
            this.scrollPresenter52.ViewChanged += ScrollPresenter_ViewChanged;
            this.scrollPresenter12.ScrollCompleted += ScrollPresenter_ScrollCompleted;
            this.scrollPresenter22.ScrollCompleted += ScrollPresenter_ScrollCompleted;
            this.scrollPresenter32.ScrollCompleted += ScrollPresenter_ScrollCompleted;
            this.scrollPresenter42.ScrollCompleted += ScrollPresenter_ScrollCompleted;
            this.scrollPresenter52.ScrollCompleted += ScrollPresenter_ScrollCompleted;
            this.scrollPresenter11.ZoomCompleted += ScrollPresenter_ZoomCompleted;
            this.scrollPresenter21.ZoomCompleted += ScrollPresenter_ZoomCompleted;
            this.scrollPresenter31.ZoomCompleted += ScrollPresenter_ZoomCompleted;
            this.scrollPresenter41.ZoomCompleted += ScrollPresenter_ZoomCompleted;
            this.scrollPresenter51.ZoomCompleted += ScrollPresenter_ZoomCompleted;

            ScrollPresenterTestHooks.ContentLayoutOffsetXChanged += ScrollPresenterTestHooks_ContentLayoutOffsetXChanged;
            ScrollPresenterTestHooks.ContentLayoutOffsetYChanged += ScrollPresenterTestHooks_ContentLayoutOffsetYChanged;
        }

        private void ScrollPresenter_StateChanged(ScrollPresenter sender, object args)
        {
            this.txtScrollPresenterState.Text = sender.Name + " " + sender.State.ToString();
            this.fullLogs.Add(sender.Name + " StateChanged S=" + sender.State.ToString());

            if (!canScrollPresenter51ContentShrink && sender == scrollPresenter51 && scrollPresenter51.State == ScrollingInteractionState.Idle)
            {
                canScrollPresenter51ContentShrink = true;
            }
        }

        private void ScrollPresenter_ViewChanged(ScrollPresenter sender, object args)
        {
            this.txtScrollPresenterHorizontalOffset.Text = sender.HorizontalOffset.ToString();
            this.txtScrollPresenterVerticalOffset.Text = sender.VerticalOffset.ToString();
            this.txtScrollPresenterZoomFactor.Text = sender.ZoomFactor.ToString();
            this.fullLogs.Add(sender.Name + " ViewChanged H=" + this.txtScrollPresenterHorizontalOffset.Text + ", V=" + this.txtScrollPresenterVerticalOffset.Text + ", S=" + this.txtScrollPresenterZoomFactor.Text);

            if (canScrollPresenter51ContentShrink && sender == scrollPresenter51)
            {
                FrameworkElement content = scrollPresenter51.Content as FrameworkElement;

                if (scrollPresenter51.HorizontalOffset > scrollPresenter51.ScrollableWidth + 20.0)
                {
                    canScrollPresenter51ContentShrink = false;
                    content.Width -= 30.0;
                }
                if (scrollPresenter51.VerticalOffset > scrollPresenter51.ScrollableHeight + 25.0)
                {
                    canScrollPresenter51ContentShrink = false;
                    content.Height -= 40.0;
                }
            }
        }

        private void ScrollPresenter_ScrollCompleted(ScrollPresenter sender, ScrollingScrollCompletedEventArgs args)
        {
            ScrollPresenterViewChangeResult result = ScrollPresenterTestHooks.GetScrollCompletedResult(args);

            this.fullLogs.Add(sender.Name + " ScrollCompleted OffsetsChangeCorrelationId=" + args.CorrelationId + ", Result=" + result);
        }

        private void ScrollPresenter_ZoomCompleted(ScrollPresenter sender, ScrollingZoomCompletedEventArgs args)
        {
            ScrollPresenterViewChangeResult result = ScrollPresenterTestHooks.GetZoomCompletedResult(args);

            this.fullLogs.Add(sender.Name + " ZoomCompleted ZoomFactorChangeCorrelationId=" + args.CorrelationId + ", Result=" + result);

            if (this.scrollPresenter52ZoomFactorChangeCorrelationId == args.CorrelationId)
            {
                this.txtResetStatus.Text = "Views reset";
            }
        }

        private void CmbShowScrollPresenter_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (this.scrollPresenter11 != null)
            {
                if (cmbShowScrollPresenter.SelectedIndex == 0)
                {
                    this.scrollPresenter11.Visibility = Visibility.Visible;
                    this.scrollPresenter21.Visibility = Visibility.Visible;
                    this.scrollPresenter31.Visibility = Visibility.Visible;
                    this.scrollPresenter41.Visibility = Visibility.Visible;
                    this.scrollPresenter51.Visibility = Visibility.Visible;
                    this.scrollPresenter12.Visibility = Visibility.Visible;
                    this.scrollPresenter22.Visibility = Visibility.Visible;
                    this.scrollPresenter32.Visibility = Visibility.Visible;
                    this.scrollPresenter42.Visibility = Visibility.Visible;
                    this.scrollPresenter52.Visibility = Visibility.Visible;

                    this.scrollPresenter11.Width = double.NaN;
                    this.scrollPresenter21.Width = double.NaN;
                    this.scrollPresenter31.Width = double.NaN;
                    this.scrollPresenter41.Width = double.NaN;
                    this.scrollPresenter51.Width = double.NaN;
                    this.scrollPresenter12.Width = double.NaN;
                    this.scrollPresenter22.Width = double.NaN;
                    this.scrollPresenter32.Width = double.NaN;
                    this.scrollPresenter42.Width = double.NaN;
                    this.scrollPresenter52.Width = double.NaN;
                    this.scrollPresenter11.Height = double.NaN;
                    this.scrollPresenter21.Height = double.NaN;
                    this.scrollPresenter31.Height = double.NaN;
                    this.scrollPresenter41.Height = double.NaN;
                    this.scrollPresenter51.Height = double.NaN;
                    this.scrollPresenter12.Height = double.NaN;
                    this.scrollPresenter22.Height = double.NaN;
                    this.scrollPresenter32.Height = double.NaN;
                    this.scrollPresenter42.Height = double.NaN;
                    this.scrollPresenter52.Height = double.NaN;

                    for (int rowIndex = 2; rowIndex < 4; rowIndex++)
                        this.rootGrid.RowDefinitions[rowIndex].Height = new GridLength(1, GridUnitType.Star);

                    for (int columnIndex = 0; columnIndex < 5; columnIndex++)
                        this.rootGrid.ColumnDefinitions[columnIndex].Width = new GridLength(1, GridUnitType.Star);
                }
                else
                {
                    this.scrollPresenter11.Visibility = Visibility.Collapsed;
                    this.scrollPresenter21.Visibility = Visibility.Collapsed;
                    this.scrollPresenter31.Visibility = Visibility.Collapsed;
                    this.scrollPresenter41.Visibility = Visibility.Collapsed;
                    this.scrollPresenter51.Visibility = Visibility.Collapsed;
                    this.scrollPresenter12.Visibility = Visibility.Collapsed;
                    this.scrollPresenter22.Visibility = Visibility.Collapsed;
                    this.scrollPresenter32.Visibility = Visibility.Collapsed;
                    this.scrollPresenter42.Visibility = Visibility.Collapsed;
                    this.scrollPresenter52.Visibility = Visibility.Collapsed;

                    for (int rowIndex = 2; rowIndex < 4; rowIndex++)
                        this.rootGrid.RowDefinitions[rowIndex].Height = GridLength.Auto;

                    for (int columnIndex = 0; columnIndex < 5; columnIndex++)
                        this.rootGrid.ColumnDefinitions[columnIndex].Width = GridLength.Auto;

                    ScrollPresenter scrollPresenter = null;

                    switch (cmbShowScrollPresenter.SelectedIndex)
                    {
                        case 1:
                            scrollPresenter = this.scrollPresenter11;
                            break;
                        case 2:
                            scrollPresenter = this.scrollPresenter21;
                            break;
                        case 3:
                            scrollPresenter = this.scrollPresenter31;
                            break;
                        case 4:
                            scrollPresenter = this.scrollPresenter41;
                            break;
                        case 5:
                            scrollPresenter = this.scrollPresenter51;
                            break;
                        case 6:
                            scrollPresenter = this.scrollPresenter12;
                            break;
                        case 7:
                            scrollPresenter = this.scrollPresenter22;
                            break;
                        case 8:
                            scrollPresenter = this.scrollPresenter32;
                            break;
                        case 9:
                            scrollPresenter = this.scrollPresenter42;
                            break;
                        case 10:
                            scrollPresenter = this.scrollPresenter52;
                            break;
                    }

                    scrollPresenter.Visibility = Visibility.Visible;
                    scrollPresenter.Width = 300;
                    scrollPresenter.Height = 400;
                }
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

        private void ScrollPresenterTestHooks_ContentLayoutOffsetXChanged(ScrollPresenter sender, object args)
        {
            float contentLayoutOffsetX = 0.0f;

            ScrollPresenterTestHooks.GetContentLayoutOffsetX(sender, out contentLayoutOffsetX);

            txtScrollPresenterContentLayoutOffsetX.Text = contentLayoutOffsetX.ToString();
        }

        private void ScrollPresenterTestHooks_ContentLayoutOffsetYChanged(ScrollPresenter sender, object args)
        {
            float contentLayoutOffsetY = 0.0f;

            ScrollPresenterTestHooks.GetContentLayoutOffsetY(sender, out contentLayoutOffsetY);

            txtScrollPresenterContentLayoutOffsetY.Text = contentLayoutOffsetY.ToString();
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
            ResetView(this.scrollPresenter11);
            ResetView(this.scrollPresenter21);
            ResetView(this.scrollPresenter31);
            ResetView(this.scrollPresenter41);
            ResetView(this.scrollPresenter51);
            ResetView(this.scrollPresenter12);
            ResetView(this.scrollPresenter22);
            ResetView(this.scrollPresenter32);
            ResetView(this.scrollPresenter42);
            ResetView(this.scrollPresenter52);
        }

        private void ResetView(ScrollPresenter scrollPresenter)
        {
            int viewChangeCorrelationId = scrollPresenter.ScrollTo(
                0.0,
                0.0,
                new ScrollingScrollOptions(ScrollingAnimationMode.Disabled, ScrollingSnapPointsMode.Ignore));
            this.fullLogs.Add(scrollPresenter.Name + " ScrollTo requested. Id=" + viewChangeCorrelationId);

            viewChangeCorrelationId = scrollPresenter.ZoomTo(
                1.0f,
                System.Numerics.Vector2.Zero,
                new ScrollingZoomOptions(ScrollingAnimationMode.Disabled, ScrollingSnapPointsMode.Ignore));
            this.fullLogs.Add(scrollPresenter.Name + " ZoomTo requested. Id=" + viewChangeCorrelationId);
            if (this.scrollPresenter52 == scrollPresenter)
            {
                scrollPresenter52ZoomFactorChangeCorrelationId = viewChangeCorrelationId;
            }
        }
    }
}