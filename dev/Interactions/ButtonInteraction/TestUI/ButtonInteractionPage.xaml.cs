// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;

using Windows.UI.Xaml;
using Windows.UI.Xaml.Automation;

using Microsoft.UI.Private.Controls;
using Windows.UI.Xaml.Media;
using Windows.UI;

namespace MUXControlsTestApp
{
    public sealed partial class ButtonInteractionPage : TestPage
    {
        private uint _invokeCount = 0;

        public ButtonInteractionPage()
        {
            this.InitializeComponent();

            Interaction = new ButtonInteraction();
            Interaction.Invoked += OnTargetElementInvoked;
            Interaction.PropertyChanged += OnInteractionPropertyChanged;
#if USE_INTERNAL_SDK
            targetElement.Interactions.Add(Interaction);
#endif
        }

        public ButtonInteraction Interaction
        {
            get;
            private set;
        }

        private void OnInvokeModeChanged(object sender, Windows.UI.Xaml.Controls.SelectionChangedEventArgs e)
        {
            if (Interaction != null)
            {
                Interaction.InvokeMode = (ButtonInteractionInvokeMode)invokeModeCB.SelectedIndex;
            }
        }

        private void OnInteractionPropertyChanged(object sender, System.ComponentModel.PropertyChangedEventArgs e)
        {
            if (e.PropertyName == "IsPressing")
            {
                UpdateRectVisualState();
                isPressingTB.Text = Interaction.IsPressing ? "True" : "False";
            }
            else if (e.PropertyName == "IsHovering")
            {
                UpdateRectVisualState();
                isHoveringTB.Text = Interaction.IsHovering ? "True" : "False";
            }
        }

        private void OnResetInvokeCountClick(object sender, RoutedEventArgs e)
        {
            _invokeCount = 0;
            invokeCountTB.Text = "0";
        }

        private void OnTargetElementInvoked(ButtonInteraction sender, ButtonInteractionInvokedEventArgs args)
        {
            invokeCountTB.Text = string.Format("{0}", ++_invokeCount);
        }

        private void UpdateRectVisualState()
        {
            if (Interaction.IsPressing)
            {
                rect.Fill = new SolidColorBrush(Colors.YellowGreen);
            }
            else if (Interaction.IsHovering)
            {
                rect.Fill = new SolidColorBrush(Colors.OrangeRed);
            }
            else
            {
                rect.Fill = new SolidColorBrush(Colors.Orange);
            }
        }
    }
}
