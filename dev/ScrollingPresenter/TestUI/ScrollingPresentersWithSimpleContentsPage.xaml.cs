// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.Generic;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;

using ScrollingPresenter = Microsoft.UI.Xaml.Controls.Primitives.ScrollingPresenter;
using InteractionState = Microsoft.UI.Xaml.Controls.ScrollingInteractionState;
using AnimationMode = Microsoft.UI.Xaml.Controls.ScrollingAnimationMode;
using SnapPointsMode = Microsoft.UI.Xaml.Controls.ScrollingSnapPointsMode;
using ScrollOptions = Microsoft.UI.Xaml.Controls.ScrollOptions;
using ZoomOptions = Microsoft.UI.Xaml.Controls.ZoomOptions;
using ScrollCompletedEventArgs = Microsoft.UI.Xaml.Controls.ScrollCompletedEventArgs;
using ZoomCompletedEventArgs = Microsoft.UI.Xaml.Controls.ZoomCompletedEventArgs;

using ScrollingPresenterTestHooks = Microsoft.UI.Private.Controls.ScrollingPresenterTestHooks;
using ScrollingPresenterViewChangeResult = Microsoft.UI.Private.Controls.ScrollingPresenterViewChangeResult;
using MUXControlsTestHooks = Microsoft.UI.Private.Controls.MUXControlsTestHooks;
using MUXControlsTestHooksLoggingMessageEventArgs = Microsoft.UI.Private.Controls.MUXControlsTestHooksLoggingMessageEventArgs;

namespace MUXControlsTestApp
{
    public sealed partial class ScrollingPresentersWithSimpleContentsPage : TestPage
    {
        private List<string> fullLogs = new List<string>();
        private int scrollingPresenter52ZoomFactorChangeId = -1;
        private bool canScrollingPresenter51ContentShrink = true;

        public ScrollingPresentersWithSimpleContentsPage()
        {
            this.InitializeComponent();

            this.scrollingPresenter11.StateChanged += ScrollingPresenter_StateChanged;
            this.scrollingPresenter21.StateChanged += ScrollingPresenter_StateChanged;
            this.scrollingPresenter31.StateChanged += ScrollingPresenter_StateChanged;
            this.scrollingPresenter41.StateChanged += ScrollingPresenter_StateChanged;
            this.scrollingPresenter51.StateChanged += ScrollingPresenter_StateChanged;
            this.scrollingPresenter11.ViewChanged += ScrollingPresenter_ViewChanged;
            this.scrollingPresenter21.ViewChanged += ScrollingPresenter_ViewChanged;
            this.scrollingPresenter31.ViewChanged += ScrollingPresenter_ViewChanged;
            this.scrollingPresenter41.ViewChanged += ScrollingPresenter_ViewChanged;
            this.scrollingPresenter51.ViewChanged += ScrollingPresenter_ViewChanged;
            this.scrollingPresenter11.ScrollCompleted += ScrollingPresenter_ScrollCompleted;
            this.scrollingPresenter21.ScrollCompleted += ScrollingPresenter_ScrollCompleted;
            this.scrollingPresenter31.ScrollCompleted += ScrollingPresenter_ScrollCompleted;
            this.scrollingPresenter41.ScrollCompleted += ScrollingPresenter_ScrollCompleted;
            this.scrollingPresenter51.ScrollCompleted += ScrollingPresenter_ScrollCompleted;
            this.scrollingPresenter11.ZoomCompleted += ScrollingPresenter_ZoomCompleted;
            this.scrollingPresenter21.ZoomCompleted += ScrollingPresenter_ZoomCompleted;
            this.scrollingPresenter31.ZoomCompleted += ScrollingPresenter_ZoomCompleted;
            this.scrollingPresenter41.ZoomCompleted += ScrollingPresenter_ZoomCompleted;
            this.scrollingPresenter51.ZoomCompleted += ScrollingPresenter_ZoomCompleted;

            this.scrollingPresenter12.StateChanged += ScrollingPresenter_StateChanged;
            this.scrollingPresenter22.StateChanged += ScrollingPresenter_StateChanged;
            this.scrollingPresenter32.StateChanged += ScrollingPresenter_StateChanged;
            this.scrollingPresenter42.StateChanged += ScrollingPresenter_StateChanged;
            this.scrollingPresenter52.StateChanged += ScrollingPresenter_StateChanged;
            this.scrollingPresenter12.ViewChanged += ScrollingPresenter_ViewChanged;
            this.scrollingPresenter22.ViewChanged += ScrollingPresenter_ViewChanged;
            this.scrollingPresenter32.ViewChanged += ScrollingPresenter_ViewChanged;
            this.scrollingPresenter42.ViewChanged += ScrollingPresenter_ViewChanged;
            this.scrollingPresenter52.ViewChanged += ScrollingPresenter_ViewChanged;
            this.scrollingPresenter12.ScrollCompleted += ScrollingPresenter_ScrollCompleted;
            this.scrollingPresenter22.ScrollCompleted += ScrollingPresenter_ScrollCompleted;
            this.scrollingPresenter32.ScrollCompleted += ScrollingPresenter_ScrollCompleted;
            this.scrollingPresenter42.ScrollCompleted += ScrollingPresenter_ScrollCompleted;
            this.scrollingPresenter52.ScrollCompleted += ScrollingPresenter_ScrollCompleted;
            this.scrollingPresenter11.ZoomCompleted += ScrollingPresenter_ZoomCompleted;
            this.scrollingPresenter21.ZoomCompleted += ScrollingPresenter_ZoomCompleted;
            this.scrollingPresenter31.ZoomCompleted += ScrollingPresenter_ZoomCompleted;
            this.scrollingPresenter41.ZoomCompleted += ScrollingPresenter_ZoomCompleted;
            this.scrollingPresenter51.ZoomCompleted += ScrollingPresenter_ZoomCompleted;

            ScrollingPresenterTestHooks.ContentLayoutOffsetXChanged += ScrollingPresenterTestHooks_ContentLayoutOffsetXChanged;
            ScrollingPresenterTestHooks.ContentLayoutOffsetYChanged += ScrollingPresenterTestHooks_ContentLayoutOffsetYChanged;
        }

        private void ScrollingPresenter_StateChanged(ScrollingPresenter sender, object args)
        {
            this.txtScrollingPresenterState.Text = sender.Name + " " + sender.State.ToString();
            this.fullLogs.Add(sender.Name + " StateChanged S=" + sender.State.ToString());

            if (!canScrollingPresenter51ContentShrink && sender == scrollingPresenter51 && scrollingPresenter51.State == InteractionState.Idle)
            {
                canScrollingPresenter51ContentShrink = true;
            }
        }

        private void ScrollingPresenter_ViewChanged(ScrollingPresenter sender, object args)
        {
            this.txtScrollingPresenterHorizontalOffset.Text = sender.HorizontalOffset.ToString();
            this.txtScrollingPresenterVerticalOffset.Text = sender.VerticalOffset.ToString();
            this.txtScrollingPresenterZoomFactor.Text = sender.ZoomFactor.ToString();
            this.fullLogs.Add(sender.Name + " ViewChanged H=" + this.txtScrollingPresenterHorizontalOffset.Text + ", V=" + this.txtScrollingPresenterVerticalOffset.Text + ", S=" + this.txtScrollingPresenterZoomFactor.Text);

            if (canScrollingPresenter51ContentShrink && sender == scrollingPresenter51)
            {
                FrameworkElement content = scrollingPresenter51.Content as FrameworkElement;

                if (scrollingPresenter51.HorizontalOffset > scrollingPresenter51.ScrollableWidth + 20.0)
                {
                    canScrollingPresenter51ContentShrink = false;
                    content.Width -= 30.0;
                }
                if (scrollingPresenter51.VerticalOffset > scrollingPresenter51.ScrollableHeight + 25.0)
                {
                    canScrollingPresenter51ContentShrink = false;
                    content.Height -= 40.0;
                }
            }
        }

        private void ScrollingPresenter_ScrollCompleted(ScrollingPresenter sender, ScrollCompletedEventArgs args)
        {
            ScrollingPresenterViewChangeResult result = ScrollingPresenterTestHooks.GetScrollCompletedResult(args);

            this.fullLogs.Add(sender.Name + " ScrollCompleted OffsetsChangeId=" + args.ScrollInfo.OffsetsChangeId + ", Result=" + result);
        }

        private void ScrollingPresenter_ZoomCompleted(ScrollingPresenter sender, ZoomCompletedEventArgs args)
        {
            ScrollingPresenterViewChangeResult result = ScrollingPresenterTestHooks.GetZoomCompletedResult(args);

            this.fullLogs.Add(sender.Name + " ZoomCompleted ZoomFactorChangeId=" + args.ZoomInfo.ZoomFactorChangeId + ", Result=" + result);

            if (this.scrollingPresenter52ZoomFactorChangeId == args.ZoomInfo.ZoomFactorChangeId)
            {
                this.txtResetStatus.Text = "Views reset";
            }
        }

        private void CmbShowScrollingPresenter_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (this.scrollingPresenter11 != null)
            {
                if (cmbShowScrollingPresenter.SelectedIndex == 0)
                {
                    this.scrollingPresenter11.Visibility = Visibility.Visible;
                    this.scrollingPresenter21.Visibility = Visibility.Visible;
                    this.scrollingPresenter31.Visibility = Visibility.Visible;
                    this.scrollingPresenter41.Visibility = Visibility.Visible;
                    this.scrollingPresenter51.Visibility = Visibility.Visible;
                    this.scrollingPresenter12.Visibility = Visibility.Visible;
                    this.scrollingPresenter22.Visibility = Visibility.Visible;
                    this.scrollingPresenter32.Visibility = Visibility.Visible;
                    this.scrollingPresenter42.Visibility = Visibility.Visible;
                    this.scrollingPresenter52.Visibility = Visibility.Visible;

                    this.scrollingPresenter11.Width = double.NaN;
                    this.scrollingPresenter21.Width = double.NaN;
                    this.scrollingPresenter31.Width = double.NaN;
                    this.scrollingPresenter41.Width = double.NaN;
                    this.scrollingPresenter51.Width = double.NaN;
                    this.scrollingPresenter12.Width = double.NaN;
                    this.scrollingPresenter22.Width = double.NaN;
                    this.scrollingPresenter32.Width = double.NaN;
                    this.scrollingPresenter42.Width = double.NaN;
                    this.scrollingPresenter52.Width = double.NaN;
                    this.scrollingPresenter11.Height = double.NaN;
                    this.scrollingPresenter21.Height = double.NaN;
                    this.scrollingPresenter31.Height = double.NaN;
                    this.scrollingPresenter41.Height = double.NaN;
                    this.scrollingPresenter51.Height = double.NaN;
                    this.scrollingPresenter12.Height = double.NaN;
                    this.scrollingPresenter22.Height = double.NaN;
                    this.scrollingPresenter32.Height = double.NaN;
                    this.scrollingPresenter42.Height = double.NaN;
                    this.scrollingPresenter52.Height = double.NaN;

                    for (int rowIndex = 2; rowIndex < 4; rowIndex++)
                        this.rootGrid.RowDefinitions[rowIndex].Height = new GridLength(1, GridUnitType.Star);

                    for (int columnIndex = 0; columnIndex < 5; columnIndex++)
                        this.rootGrid.ColumnDefinitions[columnIndex].Width = new GridLength(1, GridUnitType.Star);
                }
                else
                {
                    this.scrollingPresenter11.Visibility = Visibility.Collapsed;
                    this.scrollingPresenter21.Visibility = Visibility.Collapsed;
                    this.scrollingPresenter31.Visibility = Visibility.Collapsed;
                    this.scrollingPresenter41.Visibility = Visibility.Collapsed;
                    this.scrollingPresenter51.Visibility = Visibility.Collapsed;
                    this.scrollingPresenter12.Visibility = Visibility.Collapsed;
                    this.scrollingPresenter22.Visibility = Visibility.Collapsed;
                    this.scrollingPresenter32.Visibility = Visibility.Collapsed;
                    this.scrollingPresenter42.Visibility = Visibility.Collapsed;
                    this.scrollingPresenter52.Visibility = Visibility.Collapsed;

                    for (int rowIndex = 2; rowIndex < 4; rowIndex++)
                        this.rootGrid.RowDefinitions[rowIndex].Height = GridLength.Auto;

                    for (int columnIndex = 0; columnIndex < 5; columnIndex++)
                        this.rootGrid.ColumnDefinitions[columnIndex].Width = GridLength.Auto;

                    ScrollingPresenter scrollingPresenter = null;

                    switch (cmbShowScrollingPresenter.SelectedIndex)
                    {
                        case 1:
                            scrollingPresenter = this.scrollingPresenter11;
                            break;
                        case 2:
                            scrollingPresenter = this.scrollingPresenter21;
                            break;
                        case 3:
                            scrollingPresenter = this.scrollingPresenter31;
                            break;
                        case 4:
                            scrollingPresenter = this.scrollingPresenter41;
                            break;
                        case 5:
                            scrollingPresenter = this.scrollingPresenter51;
                            break;
                        case 6:
                            scrollingPresenter = this.scrollingPresenter12;
                            break;
                        case 7:
                            scrollingPresenter = this.scrollingPresenter22;
                            break;
                        case 8:
                            scrollingPresenter = this.scrollingPresenter32;
                            break;
                        case 9:
                            scrollingPresenter = this.scrollingPresenter42;
                            break;
                        case 10:
                            scrollingPresenter = this.scrollingPresenter52;
                            break;
                    }

                    scrollingPresenter.Visibility = Visibility.Visible;
                    scrollingPresenter.Width = 300;
                    scrollingPresenter.Height = 400;
                }
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

        private void ScrollingPresenterTestHooks_ContentLayoutOffsetXChanged(ScrollingPresenter sender, object args)
        {
            float contentLayoutOffsetX = 0.0f;

            ScrollingPresenterTestHooks.GetContentLayoutOffsetX(sender, out contentLayoutOffsetX);

            txtScrollingPresenterContentLayoutOffsetX.Text = contentLayoutOffsetX.ToString();
        }

        private void ScrollingPresenterTestHooks_ContentLayoutOffsetYChanged(ScrollingPresenter sender, object args)
        {
            float contentLayoutOffsetY = 0.0f;

            ScrollingPresenterTestHooks.GetContentLayoutOffsetY(sender, out contentLayoutOffsetY);

            txtScrollingPresenterContentLayoutOffsetY.Text = contentLayoutOffsetY.ToString();
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
            ResetView(this.scrollingPresenter11);
            ResetView(this.scrollingPresenter21);
            ResetView(this.scrollingPresenter31);
            ResetView(this.scrollingPresenter41);
            ResetView(this.scrollingPresenter51);
            ResetView(this.scrollingPresenter12);
            ResetView(this.scrollingPresenter22);
            ResetView(this.scrollingPresenter32);
            ResetView(this.scrollingPresenter42);
            ResetView(this.scrollingPresenter52);
        }

        private void ResetView(ScrollingPresenter scrollingPresenter)
        {
            int viewChangeId = scrollingPresenter.ScrollTo(
                0.0,
                0.0,
                new ScrollOptions(AnimationMode.Disabled, SnapPointsMode.Ignore)).OffsetsChangeId;
            this.fullLogs.Add(scrollingPresenter.Name + " ScrollTo requested. Id=" + viewChangeId);

            viewChangeId = scrollingPresenter.ZoomTo(
                1.0f,
                System.Numerics.Vector2.Zero,
                new ZoomOptions(AnimationMode.Disabled, SnapPointsMode.Ignore)).ZoomFactorChangeId;
            this.fullLogs.Add(scrollingPresenter.Name + " ZoomTo requested. Id=" + viewChangeId);
            if (this.scrollingPresenter52 == scrollingPresenter)
            {
                scrollingPresenter52ZoomFactorChangeId = viewChangeId;
            }
        }
    }
}