// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Linq;
using Windows.UI;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Navigation;
using System.Collections.Specialized;
using Windows.UI.Xaml.Tests.MUXControls.ApiTests.RepeaterTests.Common;

using ScrollPresenter = Microsoft.UI.Xaml.Controls.Primitives.ScrollPresenter;
using ScrollingAnchorRequestedEventArgs = Microsoft.UI.Xaml.Controls.ScrollingAnchorRequestedEventArgs;
using ItemsSourceView = Microsoft.UI.Xaml.Controls.ItemsSourceView;
using MUXControlsTestHooks = Microsoft.UI.Private.Controls.MUXControlsTestHooks;
using MUXControlsTestHooksLoggingMessageEventArgs = Microsoft.UI.Private.Controls.MUXControlsTestHooksLoggingMessageEventArgs;
using ScrollPresenterTestHooks = Microsoft.UI.Private.Controls.ScrollPresenterTestHooks;
using ScrollPresenterTestHooksAnchorEvaluatedEventArgs = Microsoft.UI.Private.Controls.ScrollPresenterTestHooksAnchorEvaluatedEventArgs;

namespace MUXControlsTestApp
{
    public sealed partial class ScrollPresenterRepeaterAnchoringPage : TestPage
    {
        private SampleDataSource dataSource = null;
        private DispatcherTimer timer = new DispatcherTimer();
        private Object asyncEventReportingLock = new Object();
        private List<string> lstAsyncEventMessage = new List<string>();
        private List<QueuedOperation> lstQueuedOperations = new List<QueuedOperation>();
        private List<QueuedOperation> lstTriggeredOperations = new List<QueuedOperation>();
        private Border currentAnchor = null;
        private UIElement anchorElement = null;
        private double lastScrollPresenterOffset = 0.0;

        public ScrollPresenterRepeaterAnchoringPage()
        {
            InitializeComponent();

            Loaded += ScrollPresenterRepeaterAnchoringPage_Loaded;

            timer.Interval = new TimeSpan(0, 0, 2 /*sec*/);
            timer.Tick += Timer_Tick;

            cnsAnchorPoint.Width = scrollPresenter.Width;
            cnsAnchorPoint.Height = scrollPresenter.Height;
            repeater.ItemTemplate = elementFactory;
        }

        protected override void OnNavigatedFrom(NavigationEventArgs e)
        {
            MUXControlsTestHooks.SetLoggingLevelForType("ScrollPresenter", isLoggingInfoLevel: false, isLoggingVerboseLevel: false);
            MUXControlsTestHooks.LoggingMessage -= MUXControlsTestHooks_LoggingMessage;

            UpdateRaiseAnchorNotifications(false /*raiseAnchorNotifications*/);

            base.OnNavigatedFrom(e);
        }

        private void ScrollPresenterRepeaterAnchoringPage_Loaded(object sender, RoutedEventArgs e)
        {
            try
            {
                if (chkLogScrollPresenterMessages.IsChecked == true)
                {
                    MUXControlsTestHooks.SetLoggingLevelForType("ScrollPresenter", isLoggingInfoLevel: true, isLoggingVerboseLevel: true);
                    MUXControlsTestHooks.LoggingMessage += MUXControlsTestHooks_LoggingMessage;
                }

                UpdateRaiseAnchorNotifications(true /*raiseAnchorNotifications*/);

                UpdateHorizontalAnchorRatio();
                UpdateVerticalAnchorRatio();

                if (chkLogScrollPresenterEvents.IsChecked == true)
                {
                    scrollPresenter.ExtentChanged += ScrollPresenter_ExtentChanged;
                    scrollPresenter.StateChanged += ScrollPresenter_StateChanged;
                }
                scrollPresenter.AnchorRequested += ScrollPresenter_AnchorRequested;
                scrollPresenter.ViewChanged += ScrollPresenter_ViewChanged;
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void CmbItemHeightMode_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (repeater != null)
            {
                switch (cmbItemHeightMode.SelectedIndex)
                {
                    case 0: // Random
                        Random rand = new Random(123);
                        dataSource = new SampleDataSource(
                            this,
                            Enumerable.Range(0, 32).Select(i => new DataItem(string.Format("Item #{0}", i), 100 + rand.Next(0, 200))).ToList());
                        break;
                    case 1: // Increasing
                        dataSource = new SampleDataSource(
                            this,
                            Enumerable.Range(0, 32).Select(i => new DataItem(string.Format("Item #{0}", i), 100 + i * 10)).ToList());
                        break;
                    case 2: // Constant
                        dataSource = new SampleDataSource(
                            this,
                            Enumerable.Range(0, 32).Select(i => new DataItem(string.Format("Item #{0}", i), 100)).ToList());
                        break;
                }
                repeater.ItemsSource = dataSource;
            }
        }

        private void Timer_Tick(object sender, object e)
        {
            timer.Stop();
            ExecuteQueuedOperations();
        }

        private void BtnInsert_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                switch (cmbDelayMode.SelectedIndex)
                {
                    case 0: // No delay
                        dataSource.Insert(int.Parse(txtNewStartIndex.Text), int.Parse(txtNewCount.Text), tglResetMode.IsChecked ?? false);
                        break;
                    case 1: // Timer expiration
                        lstQueuedOperations.Add(new QueuedOperation(QueuedOperationType.Insert, 0, 0, int.Parse(txtNewStartIndex.Text), int.Parse(txtNewCount.Text), tglResetMode.IsChecked ?? false));
                        AppendAsyncEventMessage("Queued Insert NewIndex=" + txtNewStartIndex.Text + ", NewCount=" + txtNewCount.Text);
                        timer.Start();
                        break;
                    case 2: // Offset crosses 350
                        lstTriggeredOperations.Add(new QueuedOperation(QueuedOperationType.Insert, 0, 0, int.Parse(txtNewStartIndex.Text), int.Parse(txtNewCount.Text), tglResetMode.IsChecked ?? false));
                        AppendAsyncEventMessage("Triggerable Insert NewIndex=" + txtNewStartIndex.Text + ", NewCount=" + txtNewCount.Text);
                        break;
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnRemove_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                switch (cmbDelayMode.SelectedIndex)
                {
                    case 0: // No delay
                        dataSource.Remove(int.Parse(txtOldStartIndex.Text), int.Parse(txtOldCount.Text), tglResetMode.IsChecked ?? false);
                        break;
                    case 1: // Timer expiration
                        lstQueuedOperations.Add(new QueuedOperation(QueuedOperationType.Remove, int.Parse(txtOldStartIndex.Text), int.Parse(txtOldCount.Text), 0, 0, tglResetMode.IsChecked ?? false));
                        AppendAsyncEventMessage("Queued Remove OldIndex=" + txtOldStartIndex.Text + ", OldCount=" + txtOldCount.Text);
                        timer.Start();
                        break;
                    case 2: // Offset crosses 350
                        lstTriggeredOperations.Add(new QueuedOperation(QueuedOperationType.Remove, int.Parse(txtOldStartIndex.Text), int.Parse(txtOldCount.Text), 0, 0, tglResetMode.IsChecked ?? false));
                        AppendAsyncEventMessage("Triggerable Remove OldIndex=" + txtOldStartIndex.Text + ", OldCount=" + txtOldCount.Text);
                        break;
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnReplace_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                switch (cmbDelayMode.SelectedIndex)
                {
                    case 0: // No delay
                        dataSource.Replace(int.Parse(txtOldStartIndex.Text), int.Parse(txtOldCount.Text), int.Parse(txtNewCount.Text), tglResetMode.IsChecked ?? false);
                        break;
                    case 1: // Timer expiration
                        lstQueuedOperations.Add(new QueuedOperation(QueuedOperationType.Replace, int.Parse(txtOldStartIndex.Text), int.Parse(txtOldCount.Text), 0, int.Parse(txtNewCount.Text), tglResetMode.IsChecked ?? false));
                        AppendAsyncEventMessage("Queued Replace OldIndex=" + txtOldStartIndex.Text + ", OldCount=" + txtOldCount.Text + ", NewCount=" + txtNewCount.Text);
                        timer.Start();
                        break;
                    case 2: // Offset crosses 350
                        lstTriggeredOperations.Add(new QueuedOperation(QueuedOperationType.Replace, int.Parse(txtOldStartIndex.Text), int.Parse(txtOldCount.Text), 0, int.Parse(txtNewCount.Text), tglResetMode.IsChecked ?? false));
                        AppendAsyncEventMessage("Triggerable Replace OldIndex=" + txtOldStartIndex.Text + ", OldCount=" + txtOldCount.Text + ", NewCount=" + txtNewCount.Text);
                        break;
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnReset_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                switch (cmbDelayMode.SelectedIndex)
                {
                    case 0: // No delay
                        dataSource.Reset();
                        break;
                    case 1: // Timer expiration
                        lstQueuedOperations.Add(new QueuedOperation(QueuedOperationType.Reset, 0, 0, 0, 0, true));
                        AppendAsyncEventMessage("Queued Reset");
                        timer.Start();
                        break;
                    case 2: // Offset crosses 350
                        lstTriggeredOperations.Add(new QueuedOperation(QueuedOperationType.Reset, 0, 0, 0, 0, true));
                        AppendAsyncEventMessage("Triggerable Reset");
                        break;
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void ExecuteQueuedOperations()
        {
            try
            {
                while (lstQueuedOperations.Count > 0)
                {
                    QueuedOperation qo = lstQueuedOperations[0];

                    switch (qo.Type)
                    {
                        case QueuedOperationType.Insert:
                            AppendAsyncEventMessage("Unqueuing Insert NewIndex=" + qo.NewIndex + ", NewCount=" + qo.NewCount);
                            dataSource.Insert(qo.NewIndex, qo.NewCount, qo.Reset);
                            break;
                        case QueuedOperationType.Remove:
                            AppendAsyncEventMessage("Unqueuing Remove OldIndex=" + qo.OldIndex + ", OldCount=" + qo.OldCount);
                            dataSource.Remove(qo.OldIndex, qo.OldCount, qo.Reset);
                            break;
                        case QueuedOperationType.Replace:
                            AppendAsyncEventMessage("Unqueuing Replace OldIndex=" + qo.OldIndex + ", OldCount=" + qo.OldCount + ", NewCount=" + qo.NewCount);
                            dataSource.Replace(qo.OldIndex, qo.OldCount, qo.NewCount, qo.Reset);
                            break;
                        case QueuedOperationType.Reset:
                            AppendAsyncEventMessage("Unqueuing Reset");
                            dataSource.Reset();
                            break;
                    }

                    lstQueuedOperations.RemoveAt(0);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void ExecuteTriggerableOperations()
        {
            try
            {
                while (lstTriggeredOperations.Count > 0)
                {
                    QueuedOperation qo = lstTriggeredOperations[0];

                    switch (qo.Type)
                    {
                        case QueuedOperationType.Insert:
                            AppendAsyncEventMessage("Triggering Insert NewIndex=" + qo.NewIndex + ", NewCount=" + qo.NewCount);
                            dataSource.Insert(qo.NewIndex, qo.NewCount, qo.Reset);
                            break;
                        case QueuedOperationType.Remove:
                            AppendAsyncEventMessage("Triggering Remove OldIndex=" + qo.OldIndex + ", OldCount=" + qo.OldCount);
                            dataSource.Remove(qo.OldIndex, qo.OldCount, qo.Reset);
                            break;
                        case QueuedOperationType.Replace:
                            AppendAsyncEventMessage("Triggering Replace OldIndex=" + qo.OldIndex + ", OldCount=" + qo.OldCount + ", NewCount=" + qo.NewCount);
                            dataSource.Replace(qo.OldIndex, qo.OldCount, qo.NewCount, qo.Reset);
                            break;
                        case QueuedOperationType.Reset:
                            AppendAsyncEventMessage("Triggering Reset");
                            dataSource.Reset();
                            break;
                    }

                    lstTriggeredOperations.RemoveAt(0);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void ScrollPresenter_ExtentChanged(ScrollPresenter sender, object args)
        {
            AppendAsyncEventMessage("ExtentChanged ExtentWidth=" + sender.ExtentWidth.ToString() + ", ExtentHeight=" + sender.ExtentHeight.ToString());
        }

        private void ScrollPresenter_StateChanged(ScrollPresenter sender, object args)
        {
            AppendAsyncEventMessage("StateChanged " + sender.State.ToString());
        }

        private void ScrollPresenter_ViewChanged(ScrollPresenter sender, object args)
        {
            if (chkLogScrollPresenterEvents.IsChecked == true)
            {
                AppendAsyncEventMessage("ViewChanged H=" + sender.HorizontalOffset.ToString() + ", V=" + sender.VerticalOffset.ToString() + ", S=" + sender.ZoomFactor.ToString());
            }

            double newScrollPresenterOffset = scrollPresenter.VerticalOffset;

            if (lstTriggeredOperations.Count > 0 &&
                ((lastScrollPresenterOffset <= 350.0 && newScrollPresenterOffset > 350.0) || (lastScrollPresenterOffset >= 350.0 && newScrollPresenterOffset < 350.0)))
            {
                ExecuteTriggerableOperations();
            }

            lastScrollPresenterOffset = newScrollPresenterOffset;
        }

        private void ScrollPresenter_AnchorRequested(ScrollPresenter sender, ScrollingAnchorRequestedEventArgs args)
        {
            try
            {
                if (chkLogScrollPresenterEvents.IsChecked == true)
                {
                    IList<UIElement> anchorCandidates = args.AnchorCandidates;

                    AppendAsyncEventMessage("AnchorRequested anchor=" + (args.AnchorElement == null ? "null" : "non-null") + ", count=" + anchorCandidates.Count);
                }

                args.AnchorElement = anchorElement;
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnGetHorizontalAnchorRatio_Click(object sender, RoutedEventArgs e)
        {
            UpdateHorizontalAnchorRatio();
        }

        private void BtnSetHorizontalAnchorRatio_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                scrollPresenter.HorizontalAnchorRatio = Convert.ToDouble(txtHorizontalAnchorRatio.Text);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnGetVerticalAnchorRatio_Click(object sender, RoutedEventArgs e)
        {
            UpdateVerticalAnchorRatio();
        }

        private void BtnSetVerticalAnchorRatio_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                scrollPresenter.VerticalAnchorRatio = Convert.ToDouble(txtVerticalAnchorRatio.Text);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnGetAnchorElement_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                string asyncEventMessage = "AnchorElement=";

                if (anchorElement == null)
                {
                    cmbAnchorElement.SelectedIndex = 0;
                    asyncEventMessage += "null";
                }
                else if (anchorElement == scrollPresenter)
                {
                    cmbAnchorElement.SelectedIndex = 1;
                    asyncEventMessage += "scrollPresenter";
                }
                else if (anchorElement == tblCollapsedAnchorElement)
                {
                    cmbAnchorElement.SelectedIndex = 2;
                    asyncEventMessage += "collapsed";
                }
                else if (anchorElement == border)
                {
                    cmbAnchorElement.SelectedIndex = 3;
                    asyncEventMessage += "border";
                }
                else
                {
                    cmbAnchorElement.SelectedIndex = 4;
                    asyncEventMessage += "item";
                }
                AppendAsyncEventMessage(asyncEventMessage);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void BtnSetAnchorElement_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                switch (cmbAnchorElement.SelectedIndex)
                {
                    case 0:
                        anchorElement = null;
                        break;
                    case 1:
                        anchorElement = scrollPresenter;
                        break;
                    case 2:
                        anchorElement = tblCollapsedAnchorElement;
                        break;
                    case 3:
                        anchorElement = border;
                        break;
                    case 4:
                        anchorElement = repeater.TryGetElement(int.Parse(txtItemIndex.Text));
                        break;
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                lstScrollPresenterEvents.Items.Add(ex.ToString());
            }
        }

        private void CmbAnchorElement_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (tblItemIndex != null && txtItemIndex != null && cmbAnchorElement != null)
                tblItemIndex.Visibility = txtItemIndex.Visibility = cmbAnchorElement.SelectedIndex == 4 ? Visibility.Visible : Visibility.Collapsed;
        }

        private void UpdateHorizontalAnchorRatio()
        {
            txtHorizontalAnchorRatio.Text = scrollPresenter.HorizontalAnchorRatio.ToString();
        }

        private void UpdateVerticalAnchorRatio()
        {
            txtVerticalAnchorRatio.Text = scrollPresenter.VerticalAnchorRatio.ToString();
        }

        private void BtnClearScrollPresenterEvents_Click(object sender, RoutedEventArgs e)
        {
            lstScrollPresenterEvents.Items.Clear();
        }

        private void BtnClearExceptionReport_Click(object sender, RoutedEventArgs e)
        {
            txtExceptionReport.Text = string.Empty;
        }

        private void BtnInvalidateArrange_Click(object sender, RoutedEventArgs e)
        {
            scrollPresenter.InvalidateArrange();
        }

        private void ChkUseAnimator_Checked(object sender, RoutedEventArgs e)
        {
            repeater.Animator = new Utils.DefaultElementAnimator();
        }

        private void ChkUseAnimator_Unchecked(object sender, RoutedEventArgs e)
        {
            repeater.Animator = null;
        }

        private void ChkLogScrollPresenterMessages_Checked(object sender, RoutedEventArgs e)
        {
            MUXControlsTestHooks.SetLoggingLevelForType("ScrollPresenter", isLoggingInfoLevel: true, isLoggingVerboseLevel: true);
            MUXControlsTestHooks.LoggingMessage += MUXControlsTestHooks_LoggingMessage;
        }

        private void ChkLogScrollPresenterMessages_Unchecked(object sender, RoutedEventArgs e)
        {
            MUXControlsTestHooks.SetLoggingLevelForType("ScrollPresenter", isLoggingInfoLevel: false, isLoggingVerboseLevel: false);
            MUXControlsTestHooks.LoggingMessage -= MUXControlsTestHooks_LoggingMessage;
        }

        private void UpdateRaiseAnchorNotifications(bool raiseAnchorNotifications)
        {
            if (raiseAnchorNotifications)
            {
                if (!ScrollPresenterTestHooks.AreAnchorNotificationsRaised)
                {
                    ScrollPresenterTestHooks.AreAnchorNotificationsRaised = true;
                    ScrollPresenterTestHooks.AnchorEvaluated += ScrollPresenterTestHooks_AnchorEvaluated;
                }
            }
            else
            {
                if (ScrollPresenterTestHooks.AreAnchorNotificationsRaised)
                {
                    ScrollPresenterTestHooks.AreAnchorNotificationsRaised = false;
                    ScrollPresenterTestHooks.AnchorEvaluated -= ScrollPresenterTestHooks_AnchorEvaluated;
                }

                if (currentAnchor != null)
                {
                    currentAnchor.BorderBrush = new SolidColorBrush(Colors.Chartreuse);
                    currentAnchor = null;
                }
                cnsAnchorPoint.Visibility = Visibility.Collapsed;
            }
        }

        private void ChkLogScrollPresenterEvents_Checked(object sender, RoutedEventArgs e)
        {
            scrollPresenter.ExtentChanged += ScrollPresenter_ExtentChanged;
            scrollPresenter.StateChanged += ScrollPresenter_StateChanged;
        }

        private void ChkLogScrollPresenterEvents_Unchecked(object sender, RoutedEventArgs e)
        {
            scrollPresenter.ExtentChanged -= ScrollPresenter_ExtentChanged;
            scrollPresenter.StateChanged -= ScrollPresenter_StateChanged;
        }

        private void MUXControlsTestHooks_LoggingMessage(object sender, MUXControlsTestHooksLoggingMessageEventArgs args)
        {
            // Cut off the terminating new line.
            string msg = args.Message.Substring(0, args.Message.Length - 1);
            string asyncEventMessage = string.Empty;
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
                asyncEventMessage = "Verbose: " + senderName + "m:" + msg;
            }
            else
            {
                asyncEventMessage = "Info: " + senderName + "m:" + msg;
            }

            AppendAsyncEventMessage(asyncEventMessage);
        }

        private void ScrollPresenterTestHooks_AnchorEvaluated(ScrollPresenter sender, ScrollPresenterTestHooksAnchorEvaluatedEventArgs args)
        {
            string anchorName;
            Border newAnchor = null;

            if (args.AnchorElement == null)
            {
                anchorName = "null";
            }
            else
            {
                if (args.AnchorElement is FrameworkElement)
                {
                    FrameworkElement anchorElementAsFE = args.AnchorElement as FrameworkElement;

                    if (string.IsNullOrWhiteSpace(anchorElementAsFE.Name))
                        anchorName = "unnamed-FE";
                    else
                        anchorName = anchorElementAsFE.Name;

                    newAnchor = anchorElementAsFE as Border;
                }
                else
                {
                    anchorName = "UIE";
                }
            }

            if (newAnchor != currentAnchor)
            {
                if (currentAnchor != null)
                {
                    currentAnchor.BorderBrush = new SolidColorBrush(Colors.Chartreuse);
                }
                if (newAnchor != null)
                {
                    newAnchor.BorderBrush = new SolidColorBrush(Colors.Orange);
                }
                currentAnchor = newAnchor;
            }

            if (double.IsNaN(args.ViewportAnchorPointHorizontalOffset) && double.IsNaN(args.ViewportAnchorPointVerticalOffset))
            {
                cnsAnchorPoint.Visibility = Visibility.Collapsed;
            }
            else
            {
                cnsAnchorPoint.Visibility = Visibility.Visible;

                if (double.IsNaN(args.ViewportAnchorPointHorizontalOffset))
                {
                    rectAnchorPoint.Width = scrollPresenter.Width;
                    Canvas.SetLeft(rectAnchorPoint, 0);
                }
                else
                {
                    rectAnchorPoint.Width = double.IsNaN(args.ViewportAnchorPointVerticalOffset) ? 2 : 4;
                    Canvas.SetLeft(rectAnchorPoint, args.ViewportAnchorPointHorizontalOffset * scrollPresenter.ZoomFactor - scrollPresenter.HorizontalOffset - rectAnchorPoint.Width / 2);
                }

                if (double.IsNaN(args.ViewportAnchorPointVerticalOffset))
                {
                    rectAnchorPoint.Height = scrollPresenter.Height;
                    Canvas.SetTop(rectAnchorPoint, 0);
                }
                else
                {
                    rectAnchorPoint.Height = double.IsNaN(args.ViewportAnchorPointHorizontalOffset) ? 2 : 4;
                    Canvas.SetTop(rectAnchorPoint, args.ViewportAnchorPointVerticalOffset * scrollPresenter.ZoomFactor - scrollPresenter.VerticalOffset - rectAnchorPoint.Height / 2);
                }
            }

            if (chkLogScrollPresenterAnchorNotifications.IsChecked == true)
            {
                AppendAsyncEventMessage("  AnchorEvaluated: s:" + sender.Name + ", a:" + anchorName + ", ap:(" + (int)args.ViewportAnchorPointHorizontalOffset + ", " + (int)args.ViewportAnchorPointVerticalOffset + ")");
            }
        }

        private void AppendAsyncEventMessage(string asyncEventMessage)
        {
            lock (asyncEventReportingLock)
            {
                lstAsyncEventMessage.Add(asyncEventMessage);

                var ignored = this.Dispatcher.RunAsync(Windows.UI.Core.CoreDispatcherPriority.Normal, AppendAsyncEventMessage);
            }
        }

        private void AppendAsyncEventMessage()
        {
            lock (asyncEventReportingLock)
            {
                foreach (string asyncEventMessage in lstAsyncEventMessage)
                {
                    lstScrollPresenterEvents.Items.Add(asyncEventMessage);
                }
                lstAsyncEventMessage.Clear();
            }
        }

        private enum QueuedOperationType
        {
            Insert,
            Remove,
            Replace,
            Reset,
        }

        private class QueuedOperation
        {
            public QueuedOperation(QueuedOperationType type, int oldIndex, int oldCount, int newIndex, int newCount, bool reset)
            {
                this.Type = type;
                this.OldIndex = oldIndex;
                this.OldCount = oldCount;
                this.NewIndex = newIndex;
                this.NewCount = newCount;
            }

            public QueuedOperationType Type { get; set; }
            public int OldIndex { get; set; }
            public int NewIndex { get; set; }
            public int OldCount { get; set; }
            public int NewCount { get; set; }
            public bool Reset { get; set; }
        }

        private class SampleDataSource : CustomItemsSourceViewWithUniqueIdMapping
        {
            List<DataItem> _inner;

            public SampleDataSource(ScrollPresenterRepeaterAnchoringPage owner, List<DataItem> source)
            {
                Owner = owner;
                Inner = source;
            }

            protected override int GetSizeCore()
            {
                return Inner.Count;
            }

            public List<DataItem> Inner
            {
                get
                {
                    return _inner;
                }

                set
                {
                    _inner = value;
                }
            }

            private ScrollPresenterRepeaterAnchoringPage Owner
            {
                get;
                set;
            }

            protected override object GetAtCore(int index)
            {
                return Inner[index];
            }

            protected override string KeyFromIndexCore(int index)
            {
                // data is the same as its unique id
                return Inner[index].ToString();
            }

            public void Insert(int index, int count, bool reset)
            {
                try
                {
                    Owner.AppendAsyncEventMessage("SampleDataSource Insert NewIndex=" + index + ", NewCount=" + count + ", Reset=" + reset);

                    for (int i = 0; i < count; i++)
                    {
                        switch (Owner.cmbItemHeightMode.SelectedIndex)
                        {
                            case 0: // Random
                                Random rand = new Random(123);
                                Inner.Insert(index + i, new DataItem(string.Format("ItemI #{0}", Inner.Count), 100 + rand.Next(0, 200)));
                                break;
                            case 1: // Increasing
                                Inner.Insert(index + i, new DataItem(string.Format("ItemI #{0}", Inner.Count), 100 + i * 10));
                                break;
                            case 2: // Constant
                                Inner.Insert(index + i, new DataItem(string.Format("ItemI #{0}", Inner.Count), 100));
                                break;
                        }
                    }

                    if (reset)
                    {
                        OnItemsSourceChanged(CollectionChangeEventArgsConverters.CreateNotifyArgs(NotifyCollectionChangedAction.Reset, -1, -1, -1, -1));
                    }
                    else
                    {
                        OnItemsSourceChanged(CollectionChangeEventArgsConverters.CreateNotifyArgs(
                            NotifyCollectionChangedAction.Add,
                            oldStartingIndex: -1,
                            oldItemsCount: 0,
                            newStartingIndex: index,
                            newItemsCount: count));
                    }
                }
                catch (Exception ex)
                {
                    Owner.txtExceptionReport.Text = ex.ToString();
                    Owner.lstScrollPresenterEvents.Items.Add(ex.ToString());
                }
            }

            public void Remove(int index, int count, bool reset)
            {
                try
                {
                    Owner.AppendAsyncEventMessage("SampleDataSource Remove OldIndex=" + index + ", OldCount=" + count + ", Reset=" + reset);

                    for (int i = 0; i < count; i++)
                    {
                        Inner.RemoveAt(index);
                    }

                    if (reset)
                    {
                        OnItemsSourceChanged(CollectionChangeEventArgsConverters.CreateNotifyArgs(NotifyCollectionChangedAction.Reset, -1, -1, -1, -1));
                    }
                    else
                    {
                        OnItemsSourceChanged(CollectionChangeEventArgsConverters.CreateNotifyArgs(
                            NotifyCollectionChangedAction.Remove,
                            oldStartingIndex: index,
                            oldItemsCount: count,
                            newStartingIndex: -1,
                            newItemsCount: 0));
                    }
                }
                catch (Exception ex)
                {
                    Owner.txtExceptionReport.Text = ex.ToString();
                    Owner.lstScrollPresenterEvents.Items.Add(ex.ToString());
                }
            }

            public void Replace(int index, int oldCount, int newCount, bool reset)
            {
                try
                {
                    Owner.AppendAsyncEventMessage("SampleDataSource Replace OldIndex=" + index + ", OldCount=" + oldCount + ", NewCount=" + newCount + ", Reset=" + reset);

                    for (int i = 0; i < oldCount; i++)
                    {
                        Inner.RemoveAt(index);
                    }

                    for (int i = 0; i < newCount; i++)
                    {
                        switch (Owner.cmbItemHeightMode.SelectedIndex)
                        {
                            case 0: // Random
                                Random rand = new Random(123);
                                Inner.Insert(index, new DataItem(string.Format("ItemR #{0}", Inner.Count), 100 + rand.Next(0, 200)));
                                break;
                            case 1: // Increasing
                                Inner.Insert(index, new DataItem(string.Format("ItemR #{0}", Inner.Count), 100 + i * 10));
                                break;
                            case 2: // Constant
                                Inner.Insert(index, new DataItem(string.Format("ItemR #{0}", Inner.Count), 100));
                                break;
                        }
                    }

                    if (reset)
                    {
                        OnItemsSourceChanged(CollectionChangeEventArgsConverters.CreateNotifyArgs(NotifyCollectionChangedAction.Reset, -1, -1, -1, -1));
                    }
                    else
                    {
                        OnItemsSourceChanged(CollectionChangeEventArgsConverters.CreateNotifyArgs(
                            NotifyCollectionChangedAction.Replace,
                            oldStartingIndex: index,
                            oldItemsCount: oldCount,
                            newStartingIndex: index,
                            newItemsCount: newCount));
                    }
                }
                catch (Exception ex)
                {
                    Owner.txtExceptionReport.Text = ex.ToString();
                    Owner.lstScrollPresenterEvents.Items.Add(ex.ToString());
                }
            }

            public void Reset()
            {
                try
                {
                    Owner.AppendAsyncEventMessage("SampleDataSource Reset");

                    Random rand = new Random(123);

                    for (int i = 0; i < 10; i++)
                    {
                        int from = rand.Next(0, Inner.Count - 1);
                        var value = Inner[from];
                        Inner.RemoveAt(from);
                        int to = rand.Next(0, Inner.Count - 1);
                        Inner.Insert(to, value);
                    }

                    OnItemsSourceChanged(CollectionChangeEventArgsConverters.CreateNotifyArgs(NotifyCollectionChangedAction.Reset, -1, -1, -1, -1));
                }
                catch (Exception ex)
                {
                    Owner.txtExceptionReport.Text = ex.ToString();
                    Owner.lstScrollPresenterEvents.Items.Add(ex.ToString());
                }
            }
        }
    }

    public class DataItem
    {
        public DataItem(string text, int number)
        {
            Text = text;
            Number = number;
        }

        public string Text
        {
            get;
            set;
        }

        public int Number
        {
            get;
            set;
        }
    }
}
