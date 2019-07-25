// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Common;
using System;
using System.Runtime.InteropServices.WindowsRuntime;
using System.Threading.Tasks;
using Windows.Foundation;
using Windows.UI;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Media.Imaging;
using Windows.UI.Xaml.Navigation;

using AcrylicBackgroundSource = Microsoft.UI.Xaml.Media.AcrylicBackgroundSource;
using MaterialHelperTestApi = Microsoft.UI.Private.Media.MaterialHelperTestApi;
using AcrylicBrush = Microsoft.UI.Xaml.Media.AcrylicBrush;

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

        private async void RunTestButton_Clicked(object sender, RoutedEventArgs e)
        {
            bool[] results = { false, false, false, false, false, false };

            ShowRtb1_Text.Text = "RenderTargetBitmap: In-app Acylic";
            SampledColors inAppAcrylicColors = await SamplePointColors(ShowRtb1);

            // Test 0: We are not rendering fallback color [for in-app acrylic]
            results[0] = !inAppAcrylicColors.c1.Equals(Colors.Green);

            // Test 1: Transparency is present (acylic over red BG and blue BG comes out different) ...
            results[1] = !inAppAcrylicColors.c1.Equals(inAppAcrylicColors.c2);

            // Test 2: ... but not fully transparent (we don't just evaluate to BG color)
            results[2] = !inAppAcrylicColors.c1.Equals(Colors.Red) && !inAppAcrylicColors.c2.Equals(Colors.Blue);

            // Test 3: Gaussian blur is present  (point at center of one color BG different from point at edge of different color BG)
            results[3] = !inAppAcrylicColors.c1.Equals(inAppAcrylicColors.c3) && !inAppAcrylicColors.c2.Equals(inAppAcrylicColors.c4);

            // Now switch to HostBackdrop and get an RTB for this rendreing
            // Note: For window acrylic, we don't know what's behind the test app, 
            //       however we can still validate that we don't render fallback or in-app acrylic.
            myAcrylicBrush.BackgroundSource = AcrylicBackgroundSource.HostBackdrop;
            await Task.Delay(2000);

            ShowRtb2_Text.Text = "RenderTargetBitmap: Window Acylic";
            SampledColors windowAcrylicColors = await SamplePointColors(ShowRtb2);

            // Test 4: We are not rendering fallback color [for window acrylic]
            results[4] = !windowAcrylicColors.c1.Equals(Colors.Green);

            // Test 5: We are not still doing in-app acrylic
            results[5] = !windowAcrylicColors.c1.Equals(inAppAcrylicColors.c1);

            bool result = results[0] && results[1] && results[2] && results[3] && results[4] && results[5];
            string resultString = System.String.Format("({0},{1},{2},{3},{4},{5})", results[0], results[1], results[2], results[3], results[4], results[5]);
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

                if (PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.NineteenH1))
                {
                    // Color change expected on 19H1
                    result = !initialColors.Equals(customLuminosityColors);
                }
                else
                {
                    // No color change expected on RS5 and lower
                    result = initialColors.Equals(customLuminosityColors);
                }

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

                if (PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.NineteenH1))
                {
                    // Color change expected on 19H1
                    result = !defaultLuminosityColors.Equals(customLuminsotyColors);
                }
                else
                {
                    // No color change expected on RS5 and lower
                    result = defaultLuminosityColors.Equals(customLuminsotyColors);
                }

                logger.Verify(result, "Sampled acrylic pixels did not change as expected");
            }
        }

        void LogAcrylicRecipeVersion(ResultsLogger logger)
        {
            if (PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.NineteenH1))
            {
                // Color change expected on 19H1
                logger.LogMessage("Using Luminosity-based recipe (19H1)... ");
            }
            else
            {
                // No color change expected on RS5 and lower
                logger.LogMessage("Using legacy Acrylic recipe (RS5 and lower)... ");
            }
        }

        Color GetPixelAtPoint(Point p, RenderTargetBitmap rtb, byte[] pixelArray)
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