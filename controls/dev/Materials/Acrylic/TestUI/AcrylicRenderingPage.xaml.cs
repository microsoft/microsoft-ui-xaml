// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Common;
using System;
using System.Runtime.InteropServices.WindowsRuntime;
using System.Threading.Tasks;
using Windows.Foundation;
using Windows.UI;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Media.Imaging;
using Microsoft.UI.Xaml.Navigation;
using Microsoft.UI.Private.Media;

namespace MUXControlsTestApp
{
    public sealed partial class AcrylicRenderingPage : TestPage
    {
        struct SampledColors
        {
            public Color c1;
            public Color c2;
            public Color c3;
            public Color c4;
        }

        bool[] _luminosityBlendResults = { false, false };

        public AcrylicRenderingPage()
        {
            this.InitializeComponent();

            MaterialHelperTestApi.IgnoreAreEffectsFast = true;
            MaterialHelperTestApi.SimulateDisabledByPolicy = false;
        }

        protected override void OnNavigatedFrom(NavigationEventArgs e)
        {
            // Unset all override flags to avoid impacting subsequent tests
            MaterialHelperTestApi.IgnoreAreEffectsFast = false;
            MaterialHelperTestApi.SimulateDisabledByPolicy = false;
            base.OnNavigatedFrom(e);
        }

        //          We use RenderTargetBitmap to sample some pixels from the scene below:
        //
        //          Test SetUp:
        //
        //          + Consider two adjacent rectangles R1, filled with Red and R2 filled with Blue, each of size 200 x 200
        //          + And the following sample points: 
        //                 P1: in middle of R1
        //                 P2: in middle of R2
        //                 P3: near right edge of R1
        //                 P4: near left edge of R2
        //          + Place R3 filled with myAcrylicBrush (in-app acylic) of size 200 x 400 snugly above R1 and R2
        //               >> R3 should render as acrylic over half-red, half-blue background
        //          + Take a RTB of ContainerGrid, which includes R1, R2 and R3.
        //          + Now we can inspect the colors at P1...P4 to confirm that what's rendered looks like acrylic
        //
        //                   R1                 R2
        //          -------------------+-------------------
        //          |                  |                  |
        //          |                  |                  |
        //          |                  |                  |
        //          |       (P1)       |       (P2)       |
        //          |                  |                  |
        //          |              (P3)|(P4)              |
        //          |                  |                  |
        //          -------------------+-------------------
        async Task<SampledColors> SamplePointColors(Image showRtb)
        {
            Point p1 = new Point(100, 100);             // Point over middle of Rectangle1
            Point p2 = new Point(300, 100);             // Point over middle of Rectangle2
            Point p3 = new Point(197, 100);             // Point near right edge of Rectangle 1
            Point p4 = new Point(203, 100);             // Point near left eedge or Rectangle 2

            RenderTargetBitmap rtb = new RenderTargetBitmap();
            await rtb.RenderAsync(ContainerGrid);
            showRtb.Source = rtb;

            var pixelBuffer = await rtb.GetPixelsAsync();
            byte[] pixelArray = pixelBuffer.ToArray();       // In BGRA8 format

            SampledColors sampledColors = new SampledColors();

            sampledColors.c1 = GetPixelAtPoint(p1, rtb, pixelArray);
            sampledColors.c2 = GetPixelAtPoint(p2, rtb, pixelArray);
            sampledColors.c3 = GetPixelAtPoint(p3, rtb, pixelArray);
            sampledColors.c4 = GetPixelAtPoint(p4, rtb, pixelArray);

            return sampledColors;
        }

        private void RunTestButton_Clicked(object sender, RoutedEventArgs e)
        {
            if (cbComplexBackground.IsChecked == true)
            {
                RunTestWithComplexBackground();
            }
            else
            {
                RunTestWithRedBlueBackground();
            }
        }

        private async void RunTestWithComplexBackground()
        {
            System.Diagnostics.Debug.WriteLine("------------------------------");
            System.Diagnostics.Debug.WriteLine("RunTestWithComplexBackground results:");

            RenderTargetBitmap rtb = new RenderTargetBitmap();
            await rtb.RenderAsync(ContainerGrid);
            ShowRtb1.Source = rtb;

            bool[] results = { false, false, false, false, false };

            var pixelBuffer = await rtb.GetPixelsAsync();
            byte[] pixelArray = pixelBuffer.ToArray();       // In BGRA8 format

            // Test to see if the text is blurred
            var textSample1 = GetPixelAtPoint(new Point(143,10), rtb, pixelArray);
            var textSample2 = GetPixelAtPoint(new Point(144,10), rtb, pixelArray);
            var textSample3 = GetPixelAtPoint(new Point(145,10), rtb, pixelArray);
            results[0] = AreClose(textSample1, textSample2) && AreClose(textSample2, textSample3);
            System.Diagnostics.Debug.WriteLine(System.String.Format("textSamples: {0} {1} {2}", textSample1, textSample2, textSample3));

            // Test to see if all colors are fully opaque
            results[1] = textSample1.A == 0xFF && textSample2.A == 0xFF && textSample3.A == 0xFF;

            // Test the left edge red rectangle
            var redRectSample1 = GetPixelAtPoint(new Point(241,60), rtb, pixelArray);
            var redRectSample2 = GetPixelAtPoint(new Point(242,60), rtb, pixelArray);
            var redRectSample3 = GetPixelAtPoint(new Point(243,60), rtb, pixelArray);
            results[2] = AreClose(redRectSample1, redRectSample2) && AreClose(redRectSample2, redRectSample3);
            System.Diagnostics.Debug.WriteLine(System.String.Format("redRectSamples: {0} {1} {2}", redRectSample1, redRectSample2, redRectSample3));

            // Test to see if all colors are fully opaque
            results[3] = redRectSample1.A == 0xFF && redRectSample2.A == 0xFF && redRectSample3.A == 0xFF;

            // Test if the Green FallbackColor is noticeable
            var greenSample = GetPixelAtPoint(new Point(140,100), rtb, pixelArray);
            results[4] = AreClose(greenSample, new Color() { A = 0xFF, R = 0x4D, G = 0x87, B = 0x4D });
            System.Diagnostics.Debug.WriteLine(System.String.Format("greenSample: {0}", greenSample));

            bool result = results[0] && results[1] && results[2] && results[3] && results[4];
            string resultString = System.String.Format("({0},{1},{2},{3},{4})", results[0], results[1], results[2], results[3], results[4]);
            TestResult.Text = "AcrylicOnComplexTransparentBG: " + (result ? "Passed" : ("Failed " + resultString));
        }

        private async void RunTestWithRedBlueBackground()
        {
            bool[] results = { false, false, false, false};

            ShowRtb1_Text.Text = "RenderTargetBitmap: In-app Acylic";
            SampledColors inAppAcrylicColors = await SamplePointColors(ShowRtb1);

            // Test 0: We are not rendering fallback color [for in-app acrylic]
            results[0] = !inAppAcrylicColors.c1.Equals(Microsoft.UI.Colors.Green);

            // Test 1: Transparency is present (acylic over red BG and blue BG comes out different) ...
            results[1] = !inAppAcrylicColors.c1.Equals(inAppAcrylicColors.c2);

            // Test 2: ... but not fully transparent (we don't just evaluate to BG color)
            results[2] = !inAppAcrylicColors.c1.Equals(Microsoft.UI.Colors.Red) && !inAppAcrylicColors.c2.Equals(Microsoft.UI.Colors.Blue);

            // Test 3: Gaussian blur is present  (point at center of one color BG different from point at edge of different color BG)
            results[3] = !inAppAcrylicColors.c1.Equals(inAppAcrylicColors.c3) && !inAppAcrylicColors.c2.Equals(inAppAcrylicColors.c4);

            bool result = results[0] && results[1] && results[2] && results[3];
            string resultString = System.String.Format("({0},{1},{2},{3})", results[0], results[1], results[2], results[3]);
            TestResult.Text = "AcrylicRendering: " + (result ? "Passed" : ("Failed " + resultString));
        }

        private async void SetLuminosityButton_Clicked(object sender, RoutedEventArgs e)
        {
            bool result = false;

            using (var logger = new ResultsLogger("SetLuminosityBlend", TestResult))
            {
                LogAcrylicRecipeVersion(logger);

                // Capture state when TintLuminosityOpacity is at default value of null
                // On 19H1+, this means auto configure the Luminosity for the given Tint
                // On RS5-, it has no effect (we use the legacy acrylic recipe)

                ShowRtb1_Text.Text = "SetLuminosityBlend: Luminosity = auto";
                SampledColors initialColors = await SamplePointColors(ShowRtb1);

                // Manually set TintLuminosityOpacity
                myAcrylicBrush.TintLuminosityOpacity = 1.0;
                await Task.Delay(2000);

                ShowRtb2_Text.Text = "SetLuminosityBlend: Luminosity = 1.0";
                SampledColors customLuminosityColors = await SamplePointColors(ShowRtb2);

                // Color change expected
                result = !initialColors.Equals(customLuminosityColors);

                logger.Verify(result, "Sampled acrylic pixels did not change as expected");
            }
        }

        private async void UnsetLuminosityButton_Clicked(object sender, RoutedEventArgs e)
        {
            bool result = false;

            using (var logger = new ResultsLogger("ClearLuminosityBlend", TestResult))
            {
                LogAcrylicRecipeVersion(logger);

                // Manually set TintLuminosityOpacity
                myAcrylicBrush.TintLuminosityOpacity = 1.0;
                await Task.Delay(2000);

                ShowRtb1_Text.Text = "ClearLuminosityBlend: Luminosity = 1.0";
                SampledColors customLuminsotyColors = await SamplePointColors(ShowRtb1);

                // Restore TintLuminosityOpacity to default
                myAcrylicBrush.ClearValue(AcrylicBrush.TintLuminosityOpacityProperty);
                await Task.Delay(2000);

                ShowRtb2_Text.Text = "ClearLuminosityBlend: Luminosity = auto";
                SampledColors defaultLuminosityColors = await SamplePointColors(ShowRtb2);

                // Color change expected
                result = !defaultLuminosityColors.Equals(customLuminsotyColors);

                logger.Verify(result, "Sampled acrylic pixels did not change as expected");
            }
        }

        void LogAcrylicRecipeVersion(ResultsLogger logger)
        {
            // Color change expected
            logger.LogMessage("Using Luminosity-based recipe... ");
        }

        Color GetPixelAtPoint(Point p, RenderTargetBitmap rtb, byte[] pixelArray)
        {
            // Test infra normally runs at 100% scale, but account for the scale so this
            // works if the developer is running this at a different display scale.
            double scale = ContainerGrid.XamlRoot.RasterizationScale;
            Color pixelColor = new Color();
            int x = (int)(p.X * scale);
            int y = (int)(p.Y * scale);
            int pointPosition = 4 * (rtb.PixelWidth * y + x);

            pixelColor.B = pixelArray[pointPosition];
            pixelColor.G = pixelArray[pointPosition + 1];
            pixelColor.R = pixelArray[pointPosition + 2];
            pixelColor.A = pixelArray[pointPosition + 3];

            return pixelColor;
        }

        private bool AreClose(Color color1, Color color2)
        {
            // Note: "<= 2" was arbitrarily chosen to be considered close and is currently sufficient.
            //       This needs to be enough to account for both acrylic blur and acrylic noise.
            bool aClose = Math.Abs((int)color1.A - (int)color2.A) <= 2;
            bool rClose = Math.Abs((int)color1.R - (int)color2.R) <= 2;
            bool gClose = Math.Abs((int)color1.G - (int)color2.G) <= 2;
            bool bClose = Math.Abs((int)color1.B - (int)color2.B) <= 2;
            return aClose && rClose && gClose && bClose;
        }

        private void cbComplexBackground_Toggled(object sender, RoutedEventArgs e)
        {
            if (cbComplexBackground.IsChecked == true)
            {
                blueRedBGSP.Visibility = Visibility.Collapsed;
                complexBGGrid.Visibility = Visibility.Visible;
            }
            else
            {
                blueRedBGSP.Visibility = Visibility.Visible;
                complexBGGrid.Visibility = Visibility.Collapsed;
            }
        }

        private void cbToggleFallbackColor_Toggled(object sender, RoutedEventArgs e)
        {
            if (cbToggleFallbackColor.IsChecked == true)
            {
                myAcrylicBrush.FallbackColor = Microsoft.UI.Colors.Blue;
            }
            else
            {
                myAcrylicBrush.FallbackColor = Microsoft.UI.Colors.Green;
            }
        }
    }
}
