﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
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
using Microsoft.UI.Xaml.Media.Imaging;
using Microsoft.UI.Xaml.Navigation;
using Microsoft.UI.Private.Controls;
using MUXControlsTestApp;
using MUXControlsTestApp.Utilities;

namespace MUXControlsTestApp
{
    public sealed partial class RefreshContainerOnImagePage : TestPage
    {
        public RefreshContainerOnImagePage()
        {
            this.InitializeComponent();
            this.Loaded += OnMainPageLoaded;

            timer.Interval = new TimeSpan(0, 0, 0, 0, 800);
            timer.Tick += Timer_Tick;
        }
        
        private DispatcherTimer timer = new DispatcherTimer();
        bool containerHasHandler = true;
        private bool delayRefresh = true;
        private int refreshCount = 0;

        protected override void OnNavigatedFrom(NavigationEventArgs e)
        {
            base.OnNavigatedFrom(e);
            timer.Stop();
        }

        private void OnMainPageLoaded(object sender, RoutedEventArgs e)
        {
            this.RefreshContainer.Visualizer.SizeChanged += RefreshVisualizer_SizeChanged;
            ((IRefreshContainerPrivate)this.RefreshContainer).RefreshInfoProviderAdapter = new ImageIRefreshInfoProviderAdapter(this.RefreshContainer.PullDirection, new AnimationHandler(RefreshContainer, this.RefreshContainer.PullDirection));
            
            this.RefreshContainer.Visualizer.RefreshStateChanged += RefreshVisualizer_RefreshStateChanged;
            ResetStatesComboBox();
            ResetInteractionRatiosComboBox();
            this.RefreshOnContainerButton.Click += RefreshOnContainerButton_Click;
            this.RefreshOnVisualizerButton.Click += RefreshOnVisualizerButton_Click;
            this.ResetStates.Click += ResetStates_Click;
            this.ResetInteractionRatios.Click += ResetInteractionRatios_Click;
            this.AdaptButton.Click += AdaptButton_Click;
            this.RotateButton.Click += RotateButton_Click;
            this.ChangeAlignment.Click += ChangeAlignmentButton_Click;
            this.AddOrRemoveRefreshDelay.Click += AddOrRemoveRefreshDelayButton_Click;
            this.ChangeRefreshRequested.Click += ChangeRefreshRequestedButton_Click;
        }

        private void RefreshContainer_RefreshRequested(object sender, RefreshRequestedEventArgs e)
        {
            if (delayRefresh)
            {
                this.RefreshCompletionDeferral = e.GetDeferral();
            }
            else
            {
                e.GetDeferral().Complete();
            }
            refreshCount++;
            this.RefreshCount.Text = refreshCount.ToString();
            timer.Start();
        }

        private void RefreshVisualizer_SizeChanged(object sender, SizeChangedEventArgs e)
        {
            ((IRefreshContainerPrivate)this.RefreshContainer).RefreshInfoProviderAdapter = new ImageIRefreshInfoProviderAdapter(this.RefreshContainer.PullDirection, new AnimationHandler(RefreshContainer, this.RefreshContainer.PullDirection));
        }

        private void RefreshInfoProvider_InteractionRatioChanged(IRefreshInfoProvider sender, RefreshInteractionRatioChangedEventArgs args)
        {
            UpdateInteractionRatiosComboBox(args.InteractionRatio);
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
            switch (this.RefreshContainer.PullDirection)
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
            ((IRefreshContainerPrivate)this.RefreshContainer).RefreshInfoProviderAdapter = new ImageIRefreshInfoProviderAdapter(this.RefreshContainer.PullDirection, new AnimationHandler(RefreshContainer, this.RefreshContainer.PullDirection));
        }

        private void ChangeAlignmentButton_Click(object sender, RoutedEventArgs e)
        {
            switch (this.RefreshContainer.VerticalAlignment)
            {
                case (VerticalAlignment.Center):
                    if (this.RefreshContainer.HorizontalAlignment == HorizontalAlignment.Center)
                    {
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
            if (containerHasHandler)
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

            if (refreshVisualizerPrivate.InfoProvider != null)
            {
                refreshVisualizerPrivate.InfoProvider.InteractionRatioChanged -= RefreshInfoProvider_InteractionRatioChanged;
            }

            refreshVisualizerPrivate.InfoProvider = refreshContainerPrivate.RefreshInfoProviderAdapter.AdaptFromTree(this.Image, this.RefreshContainer.Visualizer.RenderSize);

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

        private void ResetInteractionRatios_Click(object sender, RoutedEventArgs e)
        {
            ResetInteractionRatiosComboBox();
        }

        private void UpdateStatesComboBox(RefreshVisualizerState state)
        {
            this.States.Items.Add(state.ToString());
            this.States.SelectedIndex = this.States.Items.Count - 1;
        }

        private void UpdateInteractionRatiosComboBox(double interactionRatio)
        {
            this.InteractionRatios.Items.Add(interactionRatio.ToString());
            this.InteractionRatios.SelectedIndex = this.InteractionRatios.Items.Count - 1;
        }

        private void ResetStatesComboBox()
        {
            this.States.Items.Clear();
            UpdateStatesComboBox(this.RefreshContainer.Visualizer.State);
        }

        private void ResetInteractionRatiosComboBox()
        {
            this.InteractionRatios.Items.Clear();
            UpdateInteractionRatiosComboBox(0.0);
        }

        private void RefreshVisualizer_RefreshRequested(object sender, RefreshRequestedEventArgs e)
        {
            if (delayRefresh)
            {
                this.RefreshCompletionDeferral = e.GetDeferral();
            }
            else
            {
                e.GetDeferral().Complete();

                BitmapImage bitmapImage = new BitmapImage();
                Image.Width = bitmapImage.DecodePixelWidth = 300;
                bitmapImage.UriSource = new Uri(Image.BaseUri, "Assets/ingredient" + ((refreshCount % 8) + 1).ToString() + ".png");
                Image.Source = bitmapImage;
            }
            refreshCount++;
            this.RefreshCount.Text = refreshCount.ToString();
            timer.Start();
        }

        private Deferral RefreshCompletionDeferral
        {
            get;
            set;
        }

        private void Timer_Tick(object sender, object e)
        {
            timer.Stop();

            if (this.RefreshCompletionDeferral != null)
            {
                BitmapImage bitmapImage = new BitmapImage();
                Image.Width = bitmapImage.DecodePixelWidth = 300;
                bitmapImage.UriSource = new Uri(Image.BaseUri, "Assets/ingredient" + ((refreshCount % 8) + 1).ToString() + ".png");
                Image.Source = bitmapImage;

                this.RefreshCompletionDeferral.Complete();
                this.RefreshCompletionDeferral.Dispose();
                this.RefreshCompletionDeferral = null;
            }
        }
    }
}
