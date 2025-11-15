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
        private MicaBackdrop micaBackdrop;
        private DesktopAcrylicBackdrop acrylicBackdrop;

        public SystemBackdropHostPage()
        {
            this.InitializeComponent();

            // Create backdrop instances
            micaBackdrop = new MicaBackdrop();
            acrylicBackdrop = new DesktopAcrylicBackdrop();
        }

        private void MicaButton_Click(object sender, RoutedEventArgs e)
        {
            // Change SystemBackdrop property to MicaBackdrop
            TestBackdropHost.SystemBackdrop = micaBackdrop;
            StatusTextBlock.Text = "Current: Mica";
            BackdropTitleTextBlock.Text = "Mica Backdrop";
        }

        private void AcrylicButton_Click(object sender, RoutedEventArgs e)
        {
            // Change SystemBackdrop property to DesktopAcrylicBackdrop
            TestBackdropHost.SystemBackdrop = acrylicBackdrop;
            StatusTextBlock.Text = "Current: Acrylic";
            BackdropTitleTextBlock.Text = "Desktop Acrylic Backdrop";
        }

        private void CornerRadiusSlider_ValueChanged(object sender, RangeBaseValueChangedEventArgs e)
        {
            if (TestBackdropHost != null)
            {
                double cornerRadiusValue = e.NewValue;

                // Update corner radius for the backdrop host
                TestBackdropHost.CornerRadius = new CornerRadius(cornerRadiusValue);
            }
        }

        private void OpacitySlider_ValueChanged(object sender, RangeBaseValueChangedEventArgs e)
        {
            if (TestBackdropHost != null)
            {
                double opacityValue = e.NewValue;

                // Update opacity for the backdrop host
                TestBackdropHost.Opacity = opacityValue;

                // Update the display text as percentage
                if (OpacityValueTextBlock != null)
                {
                    int percentageValue = (int)(opacityValue * 100);
                    OpacityValueTextBlock.Text = $"{percentageValue} %";
                }
            }
        }

        // Click handlers for test automation buttons
        private void StoreCornerRadiusForTest_Click(object sender, RoutedEventArgs e)
        {
            StoreCornerRadiusForTest();
        }

        private void StoreBackdropTypeForTest_Click(object sender, RoutedEventArgs e)
        {
            StoreBackdropTypeForTest();
        }

        // Helper method to get the SystemBackdrop type name for a given SystemBackdropHost
        // This can be called from automation tests to verify the correct backdrop is set
        public string GetBackdropTypeName()
        {
            if (TestBackdropHost?.SystemBackdrop != null)
            {
                return TestBackdropHost.SystemBackdrop.GetType().Name;
            }
            return "None";
        }

        // Helper method to get the actual CornerRadius value from SystemBackdropHost
        // This can be called from automation tests to verify the CornerRadius property
        public double GetCornerRadiusValue()
        {
            if (TestBackdropHost != null)
            {
                return TestBackdropHost.CornerRadius.TopLeft;
            }
            return -1;
        }

        // Helper method to store the CornerRadius value in a TextBlock for test validation
        // This is called when tests need to validate the property value
        public void StoreCornerRadiusForTest()
        {
            if (TestBackdropHost != null && Results != null)
            {
                double cornerRadius = TestBackdropHost.CornerRadius.TopLeft;
                Results.Text = $"CornerRadius: {cornerRadius}";
            }
        }

        // Helper method to store the SystemBackdrop type name in a TextBlock for test validation
        public void StoreBackdropTypeForTest()
        {
            if (TestBackdropHost != null && Results != null)
            {
                string backdropType = GetBackdropTypeName();
                Results.Text = $"BackdropType: {backdropType}";
            }
        }
    }
}
