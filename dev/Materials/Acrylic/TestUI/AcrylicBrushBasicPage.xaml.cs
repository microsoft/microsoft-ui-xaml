// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Windows.UI;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Hosting;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Controls;
using System;

namespace MUXControlsTestApp
{
    public sealed partial class AcrylicBrushBasicPage : TestPage
    {
        AcrylicBrush myAcrylicBrush;
        //AcrylicBrush sideBarAcrylicBrush;

        public AcrylicBrushBasicPage()
        {
            this.InitializeComponent();

            myAcrylicBrush = new AcrylicBrush
            {
                FallbackColor = Color.FromArgb(0xFF, 0xFF, 0xFF, 0xFF),
                TintOpacity = TintOpacity.Value
            };

            //sideBarAcrylicBrush = new AcrylicBrush
            //{
            //    BackgroundSource = AcrylicBackgroundSource.HostBackdrop,
            //    TintColor = Color.FromArgb(0xCC, 0xFF, 0xFF, 0xFF),
            //    FallbackColor = Color.FromArgb(0xFF, 0xE6, 0xE6, 0xE6),
            //    TintOpacity = 0.25
            //};

            //Sidebar.Background = sideBarAcrylicBrush;
        }

        private void TestPage_Loaded(object sender, RoutedEventArgs e)
        {
            Rectangle1.Fill = myAcrylicBrush;
        }

        private void BackgroundSourceButton_Checked(object sender, RoutedEventArgs e)
        {
            myAcrylicBrush.BackgroundSource = AcrylicBackgroundSource.HostBackdrop;
        }

        private void BackgroundSourceButton_Unchecked(object sender, RoutedEventArgs e)
        {
            myAcrylicBrush.BackgroundSource = AcrylicBackgroundSource.Backdrop;   // De
        }

        private void TintColorButton_Checked(object sender, RoutedEventArgs e)
        {
            var colorPicker = new ColorPicker();
            colorPicker.ColorChanged += ColorPicker_ColorChanged;
            colorPicker.Color = myAcrylicBrush.TintColor;
            Viewbox.Child = colorPicker;
        }

        private void TintColorButton_Unchecked(object sender, RoutedEventArgs e)
        {
            Viewbox.Child = null;
        }

        private void FallbackColorButton_Checked(object sender, RoutedEventArgs e)
        {
            myAcrylicBrush.FallbackColor = ColorHelper.FromArgb(255, 255, 100, 0);
        }

        private void FallbackColorButton_Unchecked(object sender, RoutedEventArgs e)
        {
            myAcrylicBrush.FallbackColor = Color.FromArgb(0xFF, 0xE6, 0xE6, 0xE6); // Default
        }

        private void TintOpacity_ValueChanged(object sender, Windows.UI.Xaml.Controls.Primitives.RangeBaseValueChangedEventArgs e)
        {
            if (myAcrylicBrush != null)
            {
                myAcrylicBrush.TintOpacity = TintOpacity.Value;
            }
        }

        private void AnimateBackgroundButton_Checked(object sender, RoutedEventArgs e)
        {
            var visual = ElementCompositionPreview.GetElementVisual(BackgroundPanel);

            var animation = visual.Compositor.CreateScalarKeyFrameAnimation();
            animation.InsertExpressionKeyFrame(0.25f, "this.StartingValue - 200", visual.Compositor.CreateLinearEasingFunction());
            animation.InsertExpressionKeyFrame(0.75f, "this.StartingValue + 200", visual.Compositor.CreateLinearEasingFunction());
            animation.InsertExpressionKeyFrame(1.0f, "this.StartingValue", visual.Compositor.CreateLinearEasingFunction());
            animation.Duration = TimeSpan.FromSeconds(10);
            animation.IterationBehavior = Windows.UI.Composition.AnimationIterationBehavior.Forever;
            visual.StartAnimation("Offset.X", animation);
        }

        private void AnimateBackgroundButton_Unchecked(object sender, RoutedEventArgs e)
        {
            var visual = ElementCompositionPreview.GetElementVisual(BackgroundPanel);
            visual.Offset = new System.Numerics.Vector3(0, 0, 0);
        }

        private void ColorPicker_ColorChanged(ColorPicker sender, ColorChangedEventArgs args)
        {
            myAcrylicBrush.TintColor = args.NewColor;
        }
    }
}
