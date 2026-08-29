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
        }

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

        async private void containerTimer_Tick(object sender, object e)
        {
            CoreDispatcher disp = Windows.ApplicationModel.Core.CoreApplication.MainView.CoreWindow.Dispatcher;
            if (disp.HasThreadAccess)
            {
                containerTimer_TickImpl();
            }
            else
            {
                await disp.RunAsync(CoreDispatcherPriority.Normal, () =>
                 {
                     containerTimer_TickImpl();
                 });
            }
        }
        async private void visualizerTimer_Tick(object sender, object e)
        {
            CoreDispatcher disp = Windows.ApplicationModel.Core.CoreApplication.MainView.CoreWindow.Dispatcher;
            if (disp.HasThreadAccess)
            {
                visualizerTimer_TickImpl();
            }
            else
            {
                await disp.RunAsync(CoreDispatcherPriority.Normal, () =>
                {
                    visualizerTimer_TickImpl();
                });
            }
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
}
