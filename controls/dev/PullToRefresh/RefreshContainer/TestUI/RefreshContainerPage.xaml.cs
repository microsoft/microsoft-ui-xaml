// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Navigation;
using Microsoft.UI.Private.Controls;
using Windows.UI.Core;
using Microsoft.UI.Xaml.Tests.MUXControls.ApiTests;
using Common;

using MUXControlsTestApp.Utilities;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

namespace MUXControlsTestApp
{
    public sealed partial class RefreshContainerPage : TestPage
    {
        private DispatcherTimer containerTimer = new DispatcherTimer();
        private DispatcherTimer visualizerTimer = new DispatcherTimer();
        private bool delayRefresh = true;
        private bool containerHasHandler = true;
        private int refreshCount = 0;

        public RefreshContainerPage()
        {
            this.InitializeComponent();
            this.Loaded += OnMainPageLoaded;

            containerTimer.Interval = new TimeSpan(0, 0, 0, 0, 800);
            containerTimer.Tick += containerTimer_Tick;
            visualizerTimer.Interval = new TimeSpan(0, 0, 0, 0, 800);
            visualizerTimer.Tick += visualizerTimer_Tick;
            LogController.InitializeLogging();
        }

        protected override void OnNavigatedFrom(NavigationEventArgs e)
        {
            base.OnNavigatedFrom(e);
            containerTimer.Stop();
            visualizerTimer.Stop();
            if (scrollAwayRenderingHooked)
            {
                CompositionTarget.Rendering -= ScrollAway_Rendering;
                scrollAwayRenderingHooked = false;
            }
        }

        private void OnMainPageLoaded(object sender, RoutedEventArgs e)
        {
            this.RefreshContainer.Visualizer.RefreshStateChanged += RefreshVisualizer_RefreshStateChanged;
            ResetStatesComboBox();
            this.RefreshOnContainerButton.Click += RefreshOnContainerButton_Click;
            this.RefreshOnVisualizerButton.Click += RefreshOnVisualizerButton_Click;
            this.ResetStates.Click += ResetStates_Click;
            this.AdaptButton.Click += AdaptButton_Click;
            this.RotateButton.Click += RotateButton_Click;
            this.ChangeAlignment.Click += ChangeAlignmentButton_Click;
            this.AddOrRemoveRefreshDelay.Click += AddOrRemoveRefreshDelayButton_Click;

            this.RCRefreshRequestedComboBox.Items.Add("Off");
            this.RCRefreshRequestedComboBox.Items.Add("On");
            this.RCRefreshRequestedComboBox.SelectionChanged += RCRefreshRequestedComboBox_SelectionChanged;
            this.RCRefreshRequestedComboBox.SelectedIndex = 1;
            this.RCRefreshRequestedComboBoxSwitcher.Click += RCRefreshRequestedComboBoxSwitcher_Click;
            this.RVRefreshRequestedComboBox.Items.Add("Off");
            this.RVRefreshRequestedComboBox.Items.Add("On");
            this.RVRefreshRequestedComboBox.SelectionChanged += RVRefreshRequestedComboBox_SelectionChanged;
            this.RVRefreshRequestedComboBox.SelectedIndex = 0;
            this.RVRefreshRequestedComboBoxSwitcher.Click += RVRefreshRequestedComboBoxSwitcher_Click;

            ((IRefreshVisualizerPrivate)this.RefreshContainer.Visualizer).InfoProvider.InteractionRatioChanged += RefreshInfoProvider_InteractionRatioChanged;
            var boarderChild = VisualTreeHelper.GetChild(listView, 0);
            var sv = VisualTreeHelper.GetChild(boarderChild, 0);
            var sbas = (ScrollViewer)sv;
            sbas.ViewChanging += Sv_ViewChanging;
            listScrollViewer = sbas;
            sbas.ViewChanged += Sv_ViewChanged;
            this.ScrollAwayDuringPullCheckBox.Checked += ScrollAwayDuringPullCheckBox_Toggled;
            this.ScrollAwayDuringPullCheckBox.Unchecked += ScrollAwayDuringPullCheckBox_Toggled;
        }

        private ScrollViewer listScrollViewer;
        private bool scrollAwayStarted;
        private bool scrollAwayFinished;

        private void ScrollAwayDuringPullCheckBox_Toggled(object sender, RoutedEventArgs e)
        {
            scrollAwayStarted = false;
            scrollAwayFinished = false;
            if (scrollAwayProvider != null)
            {
                scrollAwayProvider.InteractionRatioChanged -= ScrollAwayProvider_InteractionRatioChanged;
                scrollAwayProvider.IsInteractingForRefreshChanged -= ScrollAwayProvider_IsInteractingForRefreshChanged;
                scrollAwayProvider = null;
            }
            if (scrollAwayRenderingHooked)
            {
                CompositionTarget.Rendering -= ScrollAway_Rendering;
                scrollAwayRenderingHooked = false;
            }
            this.StateTransitions.Text = "";
            this.ScrollAwayTrace.Text = "";
        }

        // First ChangeView: the adapter sees ViewChanging while the pull is Interacting but the current offset is still 0.
        // Second ChangeView (once the first one is applied): the current offset is now beyond the adapter's threshold.
        private void Sv_ViewChanged(object sender, ScrollViewerViewChangedEventArgs e)
        {
            if (scrollAwayStarted && !scrollAwayFinished && listScrollViewer.VerticalOffset >= 199.0)
            {
                scrollAwayFinished = true;
                // The first scroll has landed. It started while the offset was still inside the adapter's 1 DIP threshold,
                // so it must not have cancelled the pull; record that before the second scroll, which starts beyond it.
                scrollAwayInteractingAfterFirstScroll = scrollAwayProvider != null && scrollAwayProvider.IsInteractingForRefresh;
                UpdateScrollAwayTrace();
                listScrollViewer.ChangeView(null, 250.0, null, true /*disableAnimation*/);
            }
        }

        private void RCRefreshRequestedComboBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if(this.RCRefreshRequestedComboBox.SelectedIndex == 0)
            {
                this.RefreshContainer.RefreshRequested -= RefreshContainer_RefreshRequested;
            }
            else
            {
                this.RefreshContainer.RefreshRequested += RefreshContainer_RefreshRequested;
            }
        }

        private void RVRefreshRequestedComboBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (this.RVRefreshRequestedComboBox.SelectedIndex == 0)
            {
                this.RefreshContainer.Visualizer.RefreshRequested -= RefreshVisualizer_RefreshRequested;
            }
            else
            {
                this.RefreshContainer.Visualizer.RefreshRequested += RefreshVisualizer_RefreshRequested;
            }
        }

        private void RefreshContainer_RefreshRequested(object sender, RefreshRequestedEventArgs e)
        {
            if (delayRefresh)
            {
                this.ContainerRefreshCompletionDeferral = e.GetDeferral();
            }
            else
            {
                e.GetDeferral().Complete();
            }
            refreshCount++;
            this.RefreshCount.Text = refreshCount.ToString();
            containerTimer.Start();
        }
        private void RefreshVisualizer_RefreshRequested(object sender, RefreshRequestedEventArgs e)
        {
            if (delayRefresh)
            {
                this.VisualizerRefreshCompletionDeferral = e.GetDeferral();
            }
            else
            {
                e.GetDeferral().Complete();
            }
            refreshCount++;
            this.RefreshCount.Text = refreshCount.ToString();
            visualizerTimer.Start();
        }

        private void Sv_ViewChanging(object sender, ScrollViewerViewChangingEventArgs e)
        {
            this.ScrollerOffset.Text = e.NextView.VerticalOffset.ToString();
        }

        private void RefreshInfoProvider_InteractionRatioChanged(IRefreshInfoProvider sender, RefreshInteractionRatioChangedEventArgs args)
        {
            this.InteractionRatio.Text = args.InteractionRatio.ToString();
        }

        private void RCRefreshRequestedComboBoxSwitcher_Click(object sender, RoutedEventArgs e)
        {
            this.RCRefreshRequestedComboBox.SelectedIndex = ((this.RCRefreshRequestedComboBox.SelectedIndex + 1) % 2);
        }
        private void RVRefreshRequestedComboBoxSwitcher_Click(object sender, RoutedEventArgs e)
        {
            this.RVRefreshRequestedComboBox.SelectedIndex = ((this.RVRefreshRequestedComboBox.SelectedIndex + 1) % 2);
        }

        private void RefreshOnContainerButton_Click(object sender, RoutedEventArgs e)
        {
            this.RefreshContainer.RequestRefresh();
        }

        private void RefreshOnVisualizerButton_Click(object sender, RoutedEventArgs e)
        {
            this.RefreshContainer.Visualizer.RequestRefresh();
        }

        private void RotateButton_Click(object sender, RoutedEventArgs e)
        {
            switch(this.RefreshContainer.PullDirection)
            {
                case (RefreshPullDirection.TopToBottom):
                    this.RefreshContainer.PullDirection = RefreshPullDirection.LeftToRight;
                    break;
                case (RefreshPullDirection.LeftToRight):
                    this.RefreshContainer.PullDirection = RefreshPullDirection.BottomToTop;
                    break;
                case (RefreshPullDirection.BottomToTop):
                    this.RefreshContainer.PullDirection = RefreshPullDirection.RightToLeft;
                    break;
                case (RefreshPullDirection.RightToLeft):
                    this.RefreshContainer.PullDirection = RefreshPullDirection.TopToBottom;
                    break;
            }
        }

        private void ChangeAlignmentButton_Click(object sender, RoutedEventArgs e)
        {
            switch(this.RefreshContainer.VerticalAlignment)
            {
                case (VerticalAlignment.Center):
                    if(this.RefreshContainer.HorizontalAlignment == HorizontalAlignment.Center)
                    {
                        this.RefreshContainer.HorizontalAlignment = HorizontalAlignment.Center;
                        this.RefreshContainer.VerticalAlignment = VerticalAlignment.Top;
                    }
                    if (this.RefreshContainer.HorizontalAlignment == HorizontalAlignment.Left)
                    {
                        this.RefreshContainer.HorizontalAlignment = HorizontalAlignment.Center;
                        this.RefreshContainer.VerticalAlignment = VerticalAlignment.Bottom;
                    }
                    if (this.RefreshContainer.HorizontalAlignment == HorizontalAlignment.Right)
                    {
                        this.RefreshContainer.HorizontalAlignment = HorizontalAlignment.Center;
                        this.RefreshContainer.VerticalAlignment = VerticalAlignment.Center;
                    }
                    break;
                case (VerticalAlignment.Top):
                    {
                        this.RefreshContainer.HorizontalAlignment = HorizontalAlignment.Left;
                        this.RefreshContainer.VerticalAlignment = VerticalAlignment.Center;
                        break;
                    }
                case (VerticalAlignment.Bottom):
                    {
                        this.RefreshContainer.HorizontalAlignment = HorizontalAlignment.Right;
                        this.RefreshContainer.VerticalAlignment = VerticalAlignment.Center;
                        break;
                    }
            }
        }

        private void AddOrRemoveRefreshDelayButton_Click(object sender, RoutedEventArgs e)
        {
            delayRefresh = !delayRefresh;
        }

        private void ChangeRefreshRequestedButton_Click(object sender, RoutedEventArgs e)
        {
            if(containerHasHandler)
            {
                this.RefreshContainer.RefreshRequested -= RefreshContainer_RefreshRequested;
                this.RefreshContainer.Visualizer.RefreshRequested += RefreshVisualizer_RefreshRequested;
            }
            else
            {
                this.RefreshContainer.RefreshRequested += RefreshContainer_RefreshRequested;
                this.RefreshContainer.Visualizer.RefreshRequested -= RefreshVisualizer_RefreshRequested;
            }
            containerHasHandler = !containerHasHandler;
        }

        private void AdaptButton_Click(object sender, RoutedEventArgs e)
        {
            IRefreshContainerPrivate refreshContainerPrivate = (IRefreshContainerPrivate)this.RefreshContainer;
            IRefreshVisualizerPrivate refreshVisualizerPrivate = (IRefreshVisualizerPrivate)this.RefreshContainer.Visualizer;

            refreshVisualizerPrivate.InfoProvider = refreshContainerPrivate.RefreshInfoProviderAdapter.AdaptFromTree(this.listView, this.RefreshContainer.Visualizer.RenderSize);

            refreshContainerPrivate.RefreshInfoProviderAdapter.SetAnimations(this.RefreshContainer.Visualizer);
            refreshVisualizerPrivate.InfoProvider.InteractionRatioChanged += RefreshInfoProvider_InteractionRatioChanged;
        }

        private void RefreshVisualizer_RefreshStateChanged(RefreshVisualizer sender, RefreshStateChangedEventArgs args)
        {
            UpdateStatesComboBox(args.NewState);
            this.StateTransitions.Text += args.OldState + "->" + args.NewState + ";";
            if (this.ScrollAwayDuringPullCheckBox.IsChecked == true && !scrollAwayStarted && args.NewState == RefreshVisualizerState.Interacting)
            {
                scrollAwayStarted = true;
                scrollAwayRatioAtCancel = double.NaN;
                scrollAwayMaxRatioAfterCancel = double.NaN;
                scrollAwayOffsetAtCancel = double.NaN;
                scrollAwayInteractingAfterFirstScroll = null;
                scrollAwayProvider = ((IRefreshVisualizerPrivate)sender).InfoProvider;
                scrollAwayProvider.InteractionRatioChanged += ScrollAwayProvider_InteractionRatioChanged;
                scrollAwayProvider.IsInteractingForRefreshChanged += ScrollAwayProvider_IsInteractingForRefreshChanged;
                if (!scrollAwayRenderingHooked)
                {
                    CompositionTarget.Rendering += ScrollAway_Rendering;
                    scrollAwayRenderingHooked = true;
                }
                listScrollViewer.ChangeView(null, 200.0, null, true /*disableAnimation*/);
            }
            else if (scrollAwayStarted && double.IsNaN(scrollAwayRatioAtCancel) && args.OldState == RefreshVisualizerState.Interacting && args.NewState == RefreshVisualizerState.Idle)
            {
                // The provider writes every tracker position to its composition property set before it throttles
                // InteractionRatioChanged, so this is the ratio at the moment of the cancel.
                scrollAwayRatioAtCancel = ReadScrollAwayRatio();
                UpdateScrollAwayTrace();
            }
        }

        // Shows whether the pull was still going on when the interaction was cancelled: a ratio that keeps growing after
        // the cancel means the finger was still dragging, i.e. the cancel did not come from the end of the manipulation.
        // The ratio is sampled from the provider's composition property (updated on every tracker change), not from the
        // throttled InteractionRatioChanged event arguments.
        private void ScrollAwayProvider_InteractionRatioChanged(IRefreshInfoProvider sender, RefreshInteractionRatioChangedEventArgs args)
        {
            SampleScrollAwayRatio();
        }

        private void ScrollAway_Rendering(object sender, object e)
        {
            SampleScrollAwayRatio();
        }

        // Records the ScrollViewer's offset (in DIPs) at the instant the provider stops interacting for refresh, i.e. the
        // offset the adapter's threshold check saw when it cancelled the pull.
        private void ScrollAwayProvider_IsInteractingForRefreshChanged(IRefreshInfoProvider sender, object args)
        {
            if (!sender.IsInteractingForRefresh && double.IsNaN(scrollAwayOffsetAtCancel))
            {
                scrollAwayOffsetAtCancel = listScrollViewer.VerticalOffset;
                UpdateScrollAwayTrace();
            }
        }

        private void SampleScrollAwayRatio()
        {
            if (scrollAwayProvider == null || double.IsNaN(scrollAwayRatioAtCancel))
            {
                return;
            }
            double ratio = ReadScrollAwayRatio();
            if (!double.IsNaN(ratio) && (double.IsNaN(scrollAwayMaxRatioAfterCancel) || ratio > scrollAwayMaxRatioAfterCancel))
            {
                scrollAwayMaxRatioAfterCancel = ratio;
                UpdateScrollAwayTrace();
            }
        }

        private double ReadScrollAwayRatio()
        {
            float ratio;
            if (scrollAwayProvider != null &&
                scrollAwayProvider.CompositionProperties.TryGetScalar(scrollAwayProvider.InteractionRatioCompositionProperty, out ratio) == Microsoft.UI.Composition.CompositionGetValueStatus.Succeeded)
            {
                return ratio;
            }
            return double.NaN;
        }

        private void UpdateScrollAwayTrace()
        {
            this.ScrollAwayTrace.Text = "ratioAtCancel=" + scrollAwayRatioAtCancel.ToString("F2", System.Globalization.CultureInfo.InvariantCulture) +
                ";maxRatioAfterCancel=" + scrollAwayMaxRatioAfterCancel.ToString("F2", System.Globalization.CultureInfo.InvariantCulture) +
                ";offsetAtCancel=" + scrollAwayOffsetAtCancel.ToString("F2", System.Globalization.CultureInfo.InvariantCulture) +
                ";interactingAfterFirstScroll=" + (scrollAwayInteractingAfterFirstScroll.HasValue ? scrollAwayInteractingAfterFirstScroll.Value.ToString() : "NotRecorded");
        }

        private IRefreshInfoProvider scrollAwayProvider;
        private bool scrollAwayRenderingHooked;
        private double scrollAwayRatioAtCancel = double.NaN;
        private double scrollAwayMaxRatioAfterCancel = double.NaN;
        private double scrollAwayOffsetAtCancel = double.NaN;
        private bool? scrollAwayInteractingAfterFirstScroll;

        private void ResetStates_Click(object sender, RoutedEventArgs e)
        {
            ResetStatesComboBox();
        }

        private void UpdateStatesComboBox(RefreshVisualizerState state)
        {
            this.States.Items.Add(state.ToString());
            this.States.SelectedIndex = this.States.Items.Count - 1;
        }

        private void ResetStatesComboBox()
        {
            this.States.Items.Clear();
            UpdateStatesComboBox(this.RefreshContainer.Visualizer.State);
        }

        private Deferral ContainerRefreshCompletionDeferral
        {
            get;
            set;
        }

        private Deferral VisualizerRefreshCompletionDeferral
        {
            get;
            set;
        }

        // DispatcherTimer ticks on the UI thread. (The previous CoreApplication.MainView dispatcher hop is not available in the
        // desktop test app and threw E_ILLEGAL_METHOD_CALL, crashing the app after every refresh.)
        private void containerTimer_Tick(object sender, object e)
        {
            containerTimer_TickImpl();
        }

        private void visualizerTimer_Tick(object sender, object e)
        {
            visualizerTimer_TickImpl();
        }

        private void containerTimer_TickImpl()
        {
            containerTimer.Stop();
            if (this.ContainerRefreshCompletionDeferral != null)
            {
                this.ContainerRefreshCompletionDeferral.Complete();
                this.ContainerRefreshCompletionDeferral.Dispose();
                this.ContainerRefreshCompletionDeferral = null;
            }
        }

        private void visualizerTimer_TickImpl()
        {
            visualizerTimer.Stop();
            if (this.VisualizerRefreshCompletionDeferral != null)
            {
                this.VisualizerRefreshCompletionDeferral.Complete();
                this.VisualizerRefreshCompletionDeferral.Dispose();
                this.VisualizerRefreshCompletionDeferral = null;
            }
        }
    }

    // A Panel that is itself an IRefreshInfoProvider. It is referenced from RefreshContainerPage.xaml so that the app's
    // XAML type information knows it, which lets API tests use it as the "Root" of a custom RefreshContainer template.
    public sealed partial class RefreshInfoProviderPanel : Grid, IRefreshInfoProvider
    {
        private readonly Microsoft.UI.Composition.CompositionPropertySet compositionProperties;

        public RefreshInfoProviderPanel()
        {
            compositionProperties = Microsoft.UI.Xaml.Hosting.ElementCompositionPreview.GetElementVisual(this).Compositor.CreatePropertySet();
            compositionProperties.InsertScalar(InteractionRatioCompositionProperty, 0.0f);
        }

        public void RaiseInteractionRatioChangedEvent(float interactionRatio)
        {
            InteractionRatioChanged?.Invoke(this, PullToRefreshHelperTestApi.CreateRefreshInteractionRatioChangedEventArgsInstance(interactionRatio));
        }

        public double ExecutionRatio => 0.8;
        public Microsoft.UI.Composition.CompositionPropertySet CompositionProperties => compositionProperties;
        public string InteractionRatioCompositionProperty => "InteractionRatio";
        public bool IsInteractingForRefresh => false;

        public event TypedEventHandler<IRefreshInfoProvider, RefreshInteractionRatioChangedEventArgs> InteractionRatioChanged;
#pragma warning disable CS0067 // Required by IRefreshInfoProvider; this provider never interacts.
        public event TypedEventHandler<IRefreshInfoProvider, object> IsInteractingForRefreshChanged;
#pragma warning restore CS0067
        public event TypedEventHandler<IRefreshInfoProvider, object> RefreshCompleted;
        public event TypedEventHandler<IRefreshInfoProvider, object> RefreshStarted;

        public void OnRefreshCompleted()
        {
            RefreshCompleted?.Invoke(this, null);
        }

        public void OnRefreshStarted()
        {
            RefreshStarted?.Invoke(this, null);
        }
    }
}
