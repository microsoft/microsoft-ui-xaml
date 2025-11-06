// Copyright (c) Microsoft Corporation. All rights reserved. Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Media;

namespace MUXControlsTestApp
{
    [TopLevelTestPage(Name = "SystemBackdropHost")]
    public sealed partial class SystemBackdropHostPage : TestPage
    {
        public SystemBackdropHostPage()
        {
      this.InitializeComponent();
  }

      private void MicaButton_Click(object sender, RoutedEventArgs e)
    {
            // Show Mica, hide Acrylic
            MicaPanel.Visibility = Visibility.Visible;
         AcrylicPanel.Visibility = Visibility.Collapsed;
  StatusTextBlock.Text = "Current: Mica";
        }

        private void AcrylicButton_Click(object sender, RoutedEventArgs e)
        {
            // Show Acrylic, hide Mica
 MicaPanel.Visibility = Visibility.Collapsed;
 AcrylicPanel.Visibility = Visibility.Visible;
    StatusTextBlock.Text = "Current: Acrylic";
        }

   private void CornerRadiusSlider_ValueChanged(object sender, RangeBaseValueChangedEventArgs e)
 {
            if (MicaBackdropHost != null && AcrylicBackdropHost != null)
            {
 double cornerRadiusValue = e.NewValue;
             
        // Update corner radius for both backdrops
              MicaBackdropHost.CornerRadius = new CornerRadius(cornerRadiusValue);
   AcrylicBackdropHost.CornerRadius = new CornerRadius(cornerRadiusValue);
           
// Update the display text
      CornerRadiusValueTextBlock.Text = $"{cornerRadiusValue:F0} px";
            }
        }

        private void OpacitySlider_ValueChanged(object sender, RangeBaseValueChangedEventArgs e)
        {
            if (MicaBackdropHost != null && AcrylicBackdropHost != null)
            {
      double opacityValue = e.NewValue;
             
        // Update opacity for both backdrops
       MicaBackdropHost.Opacity = opacityValue;
            AcrylicBackdropHost.Opacity = opacityValue;
       
// Update the display text as percentage
   int percentageValue = (int)(opacityValue * 100);
        OpacityValueTextBlock.Text = $"{percentageValue} %";
     }
  }
    }
}
