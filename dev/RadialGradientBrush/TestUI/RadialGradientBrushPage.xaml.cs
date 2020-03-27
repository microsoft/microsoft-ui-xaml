// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Markup;
using Windows.UI;
using System.Windows.Input;
using System.Numerics;
using Windows.UI.Xaml.Media.Imaging;
using Windows.Foundation;
using Windows.Foundation.Metadata;
using Windows.UI.Composition;
using Windows.UI.Xaml.Data;

#if !BUILD_WINDOWS
using RadialGradientBrush = Microsoft.UI.Xaml.Media.RadialGradientBrush;
#endif

namespace MUXControlsTestApp
{
    [TopLevelTestPage(Name = "RadialGradientBrush")]
    public sealed partial class RadialGradientBrushPage : TestPage
    {
        public RadialGradientBrush DynamicGradientBrush;
        private Random _random;

        public RadialGradientBrushPage()
        {
            InitializeComponent();
            _random = new Random();
        }

        public string[] GetColorSpaceValueNames()
        {
            return Enum.GetNames(typeof(CompositionColorSpace));
        }

        public string DynamicGradientBrushInterpolationSpace
        {
            get
            {
                if (DynamicGradientBrush != null)
                {
                    return DynamicGradientBrush.InterpolationSpace.ToString();
                }
                else
                {
                    return null;
                }
            }
            set
            {
                if (DynamicGradientBrush != null && value != null)
                {
                    DynamicGradientBrush.InterpolationSpace = (CompositionColorSpace)Enum.Parse(typeof(CompositionColorSpace), value);
                }
            }
        }

        public string[] GetSpreadMethodValueNames()
        {
            return Enum.GetNames(typeof(GradientSpreadMethod));
        }

        public string DynamicGradientBrushSpreadMethod
        {
            get
            {
                if (DynamicGradientBrush != null)
                {
                    return DynamicGradientBrush.SpreadMethod.ToString();
                }
                else
                {
                    return null;
                }
            }
            set
            {
                if (DynamicGradientBrush != null && value != null)
                {
                    DynamicGradientBrush.SpreadMethod = (GradientSpreadMethod)Enum.Parse(typeof(GradientSpreadMethod), value);
                }
            }
        }

        private void ReplaceGradientButton_Click(object sender, RoutedEventArgs e)
        {
            DynamicGradientBrush = new RadialGradientBrush();
            DynamicGradientBrush.FallbackColor = Color.FromArgb(Byte.MaxValue, (byte)_random.Next(256), (byte)_random.Next(256), (byte)_random.Next(256));

            // Set brush before adding stops
            RectangleWithDynamicGradient.Fill = DynamicGradientBrush;

            DynamicGradientBrush.GradientStops.Clear();
            for (int i = 0; i < _random.Next(2, 5); i++)
            {
                AddRandomGradientStop(DynamicGradientBrush);
            }

            // Set brush after adding stops
            TextBlockWithDynamicGradient.Foreground = DynamicGradientBrush;

            Bindings.Update();
        }

        private void AddGradientStopButton_Click(object sender, RoutedEventArgs e)
        {
            AddRandomGradientStop(DynamicGradientBrush);
        }

        private void RemoveGradientStopButton_Click(object sender, RoutedEventArgs e)
        {
            RemoveRandomGradientStop(DynamicGradientBrush);
        }

        private void RandomizeGradientOriginButton_Click(object sender, RoutedEventArgs e)
        {
            RandomizeGradientOffset(DynamicGradientBrush);
        }

        private void RandomizeEllipseCenterButton_Click(object sender, RoutedEventArgs e)
        {
            RandomizeEllipseCenter(DynamicGradientBrush);
        }

        private void RandomizeEllipseRadiusButton_Click(object sender, RoutedEventArgs e)
        {
            RandomizeEllipseRadius(DynamicGradientBrush);
        }

        private void ToggleMappingModeButton_Click(object sender, RoutedEventArgs e)
        {
            ToggleMappingMode(DynamicGradientBrush);
        }

        private async void GenerateRenderTargetBitmapButton_Click(object sender, RoutedEventArgs e)
        {
            var rtb = new RenderTargetBitmap();
            await rtb.RenderAsync(GradientRectangle);

            RenderTargetBitmapResultRectangle.Fill = new ImageBrush() { ImageSource = rtb };

            var pixelBuffer = await rtb.GetPixelsAsync();
            byte[] pixelArray = pixelBuffer.ToArray();

            // Sample top left and center pixels to verify rendering is correct.
            var centerColor = GetPixelAtPoint(new Point(rtb.PixelWidth / 2, rtb.PixelHeight / 2), rtb, pixelArray);
            var outerColor = GetPixelAtPoint(new Point(0, 0), rtb, pixelArray);

            if (ApiInformation.IsTypePresent("Windows.UI.Composition.CompositionRadialGradientBrush"))
            {
                // If CompositionRadialGradientBrush is available then should be rendering a gradient.
                if (centerColor == Colors.Orange && outerColor == Colors.Green)
                {
                    ColorMatchTestResult.Text = "Passed";
                }
                else
                {
                    ColorMatchTestResult.Text = "Failed";
                }
            }
            else
            {
                if (centerColor == Colors.Red && outerColor == Colors.Red)
                {
                    ColorMatchTestResult.Text = "Passed";
                }
                else
                {
                    ColorMatchTestResult.Text = "Failed";
                }
            }
        }

        private void AddRandomGradientStop(RadialGradientBrush gradientBrush)
        {
            if (gradientBrush != null)
            {
                var stop = new GradientStop();
                stop.Color = Color.FromArgb(Byte.MaxValue, (byte)_random.Next(256), (byte)_random.Next(256), (byte)_random.Next(256));
                stop.Offset = _random.NextDouble();

                gradientBrush.GradientStops.Add(stop);
            }
        }

        private void RemoveRandomGradientStop(RadialGradientBrush gradientBrush)
        {
            if (gradientBrush != null && gradientBrush.GradientStops.Count > 0)
            {
                gradientBrush.GradientStops.RemoveAt(_random.Next(0, gradientBrush.GradientStops.Count - 1));
            }
        }

        private void RandomizeGradientOffset(RadialGradientBrush gradientBrush)
        {
            if (gradientBrush != null)
            {
                if (gradientBrush.MappingMode == BrushMappingMode.Absolute)
                {
                    gradientBrush.GradientOffset = new Point(_random.Next(0, 100), _random.Next(0, 100));
                }
                else
                {
                    gradientBrush.GradientOffset = new Point(_random.Next(-100, 100) / 100f, _random.Next(-100, 100) / 100f);
                }
            }
        }

        private void RandomizeEllipseCenter(RadialGradientBrush gradientBrush)
        {
            if (gradientBrush != null)
            {
                if (gradientBrush.MappingMode == BrushMappingMode.Absolute)
                {
                    gradientBrush.EllipseCenter = new Point(_random.Next(0, 100), _random.Next(0, 100));
                }
                else
                {
                    gradientBrush.EllipseCenter = new Point(_random.NextDouble(), _random.NextDouble());
                }
            }
        }

        private void RandomizeEllipseRadius(RadialGradientBrush gradientBrush)
        {
            if (gradientBrush != null)
            {
                if (gradientBrush.MappingMode == BrushMappingMode.Absolute)
                {
                    gradientBrush.EllipseRadius = new Point(_random.Next(10, 200), _random.Next(10, 200));
                }
                else
                {
                    gradientBrush.EllipseRadius = new Point(_random.NextDouble(), _random.NextDouble());
                }
            }
        }

        private void ToggleMappingMode(RadialGradientBrush gradientBrush)
        {
            if (gradientBrush != null)
            {
                gradientBrush.MappingMode = ((gradientBrush.MappingMode == BrushMappingMode.RelativeToBoundingBox) ? BrushMappingMode.Absolute : BrushMappingMode.RelativeToBoundingBox);

                RandomizeEllipseCenter(gradientBrush);
                RandomizeEllipseRadius(gradientBrush);
                RandomizeGradientOffset(gradientBrush);
            }
        }

        private Color GetPixelAtPoint(Point p, RenderTargetBitmap rtb, byte[] pixelArray)
        {
            Color pixelColor = new Color();
            int x = (int)(p.X);
            int y = (int)(p.Y);
            int pointPosition = 4 * (rtb.PixelWidth * y + x);

            pixelColor.B = pixelArray[pointPosition];
            pixelColor.G = pixelArray[pointPosition + 1];
            pixelColor.R = pixelArray[pointPosition + 2];
            pixelColor.A = pixelArray[pointPosition + 3];

            return pixelColor;
        }
    }
}
