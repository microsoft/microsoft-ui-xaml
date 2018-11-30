// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.Generic;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;

#if !BUILD_WINDOWS
using Scroller = Microsoft.UI.Xaml.Controls.Scroller;
using ScrollerChangeOffsetsOptions = Microsoft.UI.Xaml.Controls.ScrollerChangeOffsetsOptions;
using ScrollerChangeZoomFactorOptions = Microsoft.UI.Xaml.Controls.ScrollerChangeZoomFactorOptions;
using ScrollerViewKind = Microsoft.UI.Xaml.Controls.ScrollerViewKind;
using ScrollerViewChangeKind = Microsoft.UI.Xaml.Controls.ScrollerViewChangeKind;
using ScrollerViewChangeSnapPointRespect = Microsoft.UI.Xaml.Controls.ScrollerViewChangeSnapPointRespect;
using ScrollerViewChangeCompletedEventArgs = Microsoft.UI.Xaml.Controls.ScrollerViewChangeCompletedEventArgs;
using MUXControlsTestHooks = Microsoft.UI.Private.Controls.MUXControlsTestHooks;
using MUXControlsTestHooksLoggingMessageEventArgs = Microsoft.UI.Private.Controls.MUXControlsTestHooksLoggingMessageEventArgs;
#endif

namespace MUXControlsTestApp
{
    public sealed partial class ScrollersWithSimpleContentsPage : TestPage
    {
        private List<string> fullLogs = new List<string>();
        private int scroller52ZoomFactorChangeId = -1;

        public ScrollersWithSimpleContentsPage()
        {
            this.InitializeComponent();

            this.scroller11.StateChanged += Scroller_StateChanged;
            this.scroller21.StateChanged += Scroller_StateChanged;
            this.scroller31.StateChanged += Scroller_StateChanged;
            this.scroller41.StateChanged += Scroller_StateChanged;
            this.scroller51.StateChanged += Scroller_StateChanged;
            this.scroller11.ViewChanged += Scroller_ViewChanged;
            this.scroller21.ViewChanged += Scroller_ViewChanged;
            this.scroller31.ViewChanged += Scroller_ViewChanged;
            this.scroller41.ViewChanged += Scroller_ViewChanged;
            this.scroller51.ViewChanged += Scroller_ViewChanged;
            this.scroller11.ViewChangeCompleted += Scroller_ViewChangeCompleted;
            this.scroller21.ViewChangeCompleted += Scroller_ViewChangeCompleted;
            this.scroller31.ViewChangeCompleted += Scroller_ViewChangeCompleted;
            this.scroller41.ViewChangeCompleted += Scroller_ViewChangeCompleted;
            this.scroller51.ViewChangeCompleted += Scroller_ViewChangeCompleted;

            this.scroller12.StateChanged += Scroller_StateChanged;
            this.scroller22.StateChanged += Scroller_StateChanged;
            this.scroller32.StateChanged += Scroller_StateChanged;
            this.scroller42.StateChanged += Scroller_StateChanged;
            this.scroller52.StateChanged += Scroller_StateChanged;
            this.scroller12.ViewChanged += Scroller_ViewChanged;
            this.scroller22.ViewChanged += Scroller_ViewChanged;
            this.scroller32.ViewChanged += Scroller_ViewChanged;
            this.scroller42.ViewChanged += Scroller_ViewChanged;
            this.scroller52.ViewChanged += Scroller_ViewChanged;
            this.scroller12.ViewChangeCompleted += Scroller_ViewChangeCompleted;
            this.scroller22.ViewChangeCompleted += Scroller_ViewChangeCompleted;
            this.scroller32.ViewChangeCompleted += Scroller_ViewChangeCompleted;
            this.scroller42.ViewChangeCompleted += Scroller_ViewChangeCompleted;
            this.scroller52.ViewChangeCompleted += Scroller_ViewChangeCompleted;
        }

        private void Scroller_StateChanged(Scroller sender, object args)
        {
            this.txtScrollerState.Text = sender.Name + " " + sender.State.ToString();
            this.fullLogs.Add(sender.Name + " StateChanged S=" + sender.State.ToString());
        }

        private void Scroller_ViewChanged(Scroller sender, object args)
        {
            this.txtScrollerHorizontalOffset.Text = sender.HorizontalOffset.ToString();
            this.txtScrollerVerticalOffset.Text = sender.VerticalOffset.ToString();
            this.txtScrollerZoomFactor.Text = sender.ZoomFactor.ToString();
            this.fullLogs.Add(sender.Name + " ViewChanged H=" + this.txtScrollerHorizontalOffset.Text + ", V=" + this.txtScrollerVerticalOffset.Text + ", S=" + this.txtScrollerZoomFactor.Text);
        }

        private void Scroller_ViewChangeCompleted(Scroller sender, ScrollerViewChangeCompletedEventArgs args)
        {
            this.fullLogs.Add(sender.Name + " ViewChangeCompleted ViewChangeId=" + args.ViewChangeId + ", Result=" + args.Result);

            if (this.scroller52ZoomFactorChangeId == args.ViewChangeId)
            {
                this.txtResetStatus.Text = "Views reset";
            }
        }

        private void CmbShowScroller_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (this.scroller11 != null)
            {
                if (cmbShowScroller.SelectedIndex == 0)
                {
                    this.scroller11.Visibility = Visibility.Visible;
                    this.scroller21.Visibility = Visibility.Visible;
                    this.scroller31.Visibility = Visibility.Visible;
                    this.scroller41.Visibility = Visibility.Visible;
                    this.scroller51.Visibility = Visibility.Visible;
                    this.scroller12.Visibility = Visibility.Visible;
                    this.scroller22.Visibility = Visibility.Visible;
                    this.scroller32.Visibility = Visibility.Visible;
                    this.scroller42.Visibility = Visibility.Visible;
                    this.scroller52.Visibility = Visibility.Visible;

                    this.scroller11.Width = double.NaN;
                    this.scroller21.Width = double.NaN;
                    this.scroller31.Width = double.NaN;
                    this.scroller41.Width = double.NaN;
                    this.scroller51.Width = double.NaN;
                    this.scroller12.Width = double.NaN;
                    this.scroller22.Width = double.NaN;
                    this.scroller32.Width = double.NaN;
                    this.scroller42.Width = double.NaN;
                    this.scroller52.Width = double.NaN;
                    this.scroller11.Height = double.NaN;
                    this.scroller21.Height = double.NaN;
                    this.scroller31.Height = double.NaN;
                    this.scroller41.Height = double.NaN;
                    this.scroller51.Height = double.NaN;
                    this.scroller12.Height = double.NaN;
                    this.scroller22.Height = double.NaN;
                    this.scroller32.Height = double.NaN;
                    this.scroller42.Height = double.NaN;
                    this.scroller52.Height = double.NaN;

                    for (int rowIndex = 2; rowIndex < 4; rowIndex++)
                        this.rootGrid.RowDefinitions[rowIndex].Height = new GridLength(1, GridUnitType.Star);

                    for (int columnIndex = 0; columnIndex < 5; columnIndex++)
                        this.rootGrid.ColumnDefinitions[columnIndex].Width = new GridLength(1, GridUnitType.Star);
                }
                else
                {
                    this.scroller11.Visibility = Visibility.Collapsed;
                    this.scroller21.Visibility = Visibility.Collapsed;
                    this.scroller31.Visibility = Visibility.Collapsed;
                    this.scroller41.Visibility = Visibility.Collapsed;
                    this.scroller51.Visibility = Visibility.Collapsed;
                    this.scroller12.Visibility = Visibility.Collapsed;
                    this.scroller22.Visibility = Visibility.Collapsed;
                    this.scroller32.Visibility = Visibility.Collapsed;
                    this.scroller42.Visibility = Visibility.Collapsed;
                    this.scroller52.Visibility = Visibility.Collapsed;

                    for (int rowIndex = 2; rowIndex < 4; rowIndex++)
                        this.rootGrid.RowDefinitions[rowIndex].Height = GridLength.Auto;

                    for (int columnIndex = 0; columnIndex < 5; columnIndex++)
                        this.rootGrid.ColumnDefinitions[columnIndex].Width = GridLength.Auto;

                    Scroller scroller = null;

                    switch (cmbShowScroller.SelectedIndex)
                    {
                        case 1:
                            scroller = this.scroller11;
                            break;
                        case 2:
                            scroller = this.scroller21;
                            break;
                        case 3:
                            scroller = this.scroller31;
                            break;
                        case 4:
                            scroller = this.scroller41;
                            break;
                        case 5:
                            scroller = this.scroller51;
                            break;
                        case 6:
                            scroller = this.scroller12;
                            break;
                        case 7:
                            scroller = this.scroller22;
                            break;
                        case 8:
                            scroller = this.scroller32;
                            break;
                        case 9:
                            scroller = this.scroller42;
                            break;
                        case 10:
                            scroller = this.scroller52;
                            break;
                    }

                    scroller.Visibility = Visibility.Visible;
                    scroller.Width = 300;
                    scroller.Height = 400;
                }
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

        private void btnResetViews_Click(object sender, RoutedEventArgs e)
        {
            this.txtResetStatus.Text = "Resetting views ...";
            ResetView(this.scroller11);
            ResetView(this.scroller21);
            ResetView(this.scroller31);
            ResetView(this.scroller41);
            ResetView(this.scroller51);
            ResetView(this.scroller12);
            ResetView(this.scroller22);
            ResetView(this.scroller32);
            ResetView(this.scroller42);
            ResetView(this.scroller52);
        }

        private void ResetView(Scroller scroller)
        {
            int viewChangeId = scroller.ChangeOffsets(new ScrollerChangeOffsetsOptions(0.0, 0.0, ScrollerViewKind.Absolute, ScrollerViewChangeKind.DisableAnimation, ScrollerViewChangeSnapPointRespect.IgnoreSnapPoints));
            this.fullLogs.Add(scroller.Name + " ChangeOffsets requested. Id=" + viewChangeId);

            viewChangeId = scroller.ChangeZoomFactor(new ScrollerChangeZoomFactorOptions(1.0f, ScrollerViewKind.Absolute, System.Numerics.Vector2.Zero, ScrollerViewChangeKind.DisableAnimation, ScrollerViewChangeSnapPointRespect.IgnoreSnapPoints));
            this.fullLogs.Add(scroller.Name + " ChangeZoomFactor requested. Id=" + viewChangeId);
            if (this.scroller52 == scroller)
            {
                scroller52ZoomFactorChangeId = viewChangeId;
            }
        }
    }
}