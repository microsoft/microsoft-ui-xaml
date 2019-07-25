// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Windows.UI.Composition;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Navigation;

using AcrylicBrush = Microsoft.UI.Xaml.Media.AcrylicBrush;
using AcrylicTestApi = Microsoft.UI.Private.Media.AcrylicTestApi;
using MaterialHelperTestApi = Microsoft.UI.Private.Media.MaterialHelperTestApi;

namespace MUXControlsTestApp
{
    public sealed partial class AcrylicMarkupPage : TestPage
    {
        AcrylicTestApi _acrylicTestApi;
        AcrylicBrush _acrylicBrush;

        public AcrylicMarkupPage()
        {
            this.InitializeComponent();

            _acrylicTestApi = new AcrylicTestApi();

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

        private void RunTestButton_Clicked(object sender, RoutedEventArgs e)
        {
            bool[] results = { false, false, false };

            _acrylicTestApi.AcrylicBrush = AcrylicBrush1;
            results[0] = VerifyAcrylic();

            _acrylicTestApi.AcrylicBrush = AcrylicBrush2;
            results[1] = VerifyAcrylic();

            _acrylicTestApi.AcrylicBrush = AcrylicBrush3;
            _acrylicBrush = AcrylicBrush3;
            results[2] = VerifyFallbackColor();

            bool result = results[0] && results[1] && results[2];
            string resultString = System.String.Format("({0},{1},{2})", results[0], results[1], results[2]);

            TestResult.Text = "AcrylicFromMarkup: " + (result ? "Passed" : ("Failed " + resultString));
        }

        bool VerifyFallbackColor()
        {
            bool isUsingAcrylicBrush = UpdateIsUsingAcrylicBrush();
            var compositionBrush = UpdateCompositionBrush();
            return !isUsingAcrylicBrush &&
                   (compositionBrush != null && compositionBrush is CompositionColorBrush) &&
                   (compositionBrush as CompositionColorBrush).Color.Equals(_acrylicBrush.FallbackColor);
        }

        bool VerifyAcrylic()
        {
            bool isUsingAcrylicBrush = UpdateIsUsingAcrylicBrush();
            var compositionBrush = UpdateCompositionBrush();
            var noiseBrush = UpdateNoiseBrush();
            return isUsingAcrylicBrush &&
                   (compositionBrush != null && compositionBrush is CompositionEffectBrush) &&
                   (noiseBrush != null && noiseBrush is CompositionSurfaceBrush);
        }

        private CompositionBrush UpdateCompositionBrush()
        {
            CompositionBrush result = null;
            if (_acrylicTestApi.AcrylicBrush != null)
            {
                result = _acrylicTestApi.CompositionBrush;

                // ToString() here will log the type of the brush - useful to check whether we are in fallback mode
                CompositionBrushPointer.Text = (result == null) ? "null" : result.ToString();
            }
            return result;
        }

        private CompositionBrush UpdateNoiseBrush()
        {
            CompositionBrush result = null;
            if (_acrylicTestApi.AcrylicBrush != null)
            {
                result = _acrylicTestApi.NoiseBrush;
                NoiseBrushPointer.Text = (result == null) ? "null" : result.ToString();
            }
            return result;
        }

        private bool UpdateIsUsingAcrylicBrush()
        {
            bool result = _acrylicTestApi.IsUsingAcrylicBrush;
            IsUsingAcrylicBrush.Text = result.ToString();
            return result;
        }

    }
}