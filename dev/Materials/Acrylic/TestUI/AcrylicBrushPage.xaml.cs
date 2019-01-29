// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Common;
using System;
using System.Threading;
using System.Threading.Tasks;
using Windows.Foundation.Metadata;
using Windows.UI;
using Windows.UI.Composition;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Automation;
using Windows.UI.Xaml.Automation.Peers;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Hosting;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Media.Animation;
using Windows.UI.Xaml.Navigation;
using MUXControlsTestApp.Utilities;

#if !BUILD_WINDOWS
#if !BUILD_LEAN_MUX_FOR_THE_STORE_APP
using ColorPicker = Microsoft.UI.Xaml.Controls.ColorPicker;
using ColorChangedEventArgs = Microsoft.UI.Xaml.Controls.ColorChangedEventArgs;
#endif
using AcrylicBackgroundSource = Microsoft.UI.Xaml.Media.AcrylicBackgroundSource;
using AcrylicBrush = Microsoft.UI.Xaml.Media.AcrylicBrush;
using AcrylicTestApi = Microsoft.UI.Private.Media.AcrylicTestApi;
using IAcrylicBrushStaticsPrivate = Microsoft.UI.Private.Media.IAcrylicBrushStaticsPrivate;
using MaterialHelperTestApi = Microsoft.UI.Private.Media.MaterialHelperTestApi;
#endif

namespace MUXControlsTestApp
{
    public sealed partial class AcrylicBrushPage : TestPage
    {
        AcrylicBrush _acrylicBrush;
        AcrylicTestApi _acrylicTestApi;
        int _iteration_RunHideAndShowWindow;
        int _iteration_TintTransitionDuration;
        CompositionBrush _previousCompositionBrush;          // Used by test helpers that get called multiple times

        public AcrylicBrushPage()
        {
            this.InitializeComponent();

            if (ApiInformation.IsTypePresent("Windows.UI.Xaml.Media.XamlCompositionBrushBase"))
            {
                AutomationProperties.SetName(this, "AcrylicBrushPage");
                AutomationProperties.SetAutomationId(this, "AcrylicBrushPage");

                _acrylicBrush = new AcrylicBrush
                {
                    BackgroundSource = AcrylicBackgroundSource.HostBackdrop,
                    FallbackColor = Color.FromArgb(0xFF, 0x0, 0x0, 0xFF),
                    TintOpacity = TintOpacity.Value
                };

                _acrylicTestApi = new AcrylicTestApi();

                _iteration_RunHideAndShowWindow = 0;
                _iteration_TintTransitionDuration = 0;
            }

        }

        protected override void OnNavigatedTo(NavigationEventArgs e)
        {
            base.OnNavigatedTo(e);

            // The app will notify the InteractionTest (AcrylicBrushTests.HideAndShowWindow) when 
            // Xaml window visibility changes since these events are not available through UIA / Mita.
            Window.Current.VisibilityChanged += XamlWindow_VisibilityChanged;
        }

        protected override void OnNavigatedFrom(NavigationEventArgs e)
        {
            base.OnNavigatedFrom(e);

            Window.Current.VisibilityChanged -= XamlWindow_VisibilityChanged;
        }

        protected override AutomationPeer OnCreateAutomationPeer()
        {
            return new FrameworkElementAutomationPeer(this);
        }

        private void BackgroundSourceButton_Checked(object sender, RoutedEventArgs e)
        {
            _acrylicBrush.BackgroundSource = AcrylicBackgroundSource.HostBackdrop;
        }

        private void BackgroundSourceButton_Unchecked(object sender, RoutedEventArgs e)
        {
            _acrylicBrush.BackgroundSource = AcrylicBackgroundSource.Backdrop;
        }

        private void TintColorButton_Checked(object sender, RoutedEventArgs e)
        {
#if !BUILD_LEAN_MUX_FOR_THE_STORE_APP
            var colorPicker = new ColorPicker();
            colorPicker.ColorChanged += ColorPicker_ColorChanged;
            colorPicker.Color = _acrylicBrush.TintColor;
            Viewbox.Child = colorPicker;
#endif
        }

        private void TintColorButton_Unchecked(object sender, RoutedEventArgs e)
        {
            Viewbox.Child = null;
        }

        private void FallbackColorButton_Checked(object sender, RoutedEventArgs e)
        {
            _acrylicBrush.FallbackColor = ColorHelper.FromArgb(255, 255, 100, 0);
        }

        private void FallbackColorButton_Unchecked(object sender, RoutedEventArgs e)
        {
            _acrylicBrush.FallbackColor = Color.FromArgb(0xFF, 0xE6, 0xE6, 0xE6); // Default
        }

        private void AlwaysUseFallbackButton_Checked(object sender, RoutedEventArgs e)
        {
            _acrylicBrush.AlwaysUseFallback = true;
        }

        private void AlwaysUseFallbackButton_Unchecked(object sender, RoutedEventArgs e)
        {
            _acrylicBrush.AlwaysUseFallback = false;
        }

        private void TintOpacity_ValueChanged(object sender, Windows.UI.Xaml.Controls.Primitives.RangeBaseValueChangedEventArgs e)
        {
            if (_acrylicBrush != null)
            {
                _acrylicBrush.TintOpacity = TintOpacity.Value;
            }
        }

        private void TintLuminosityOpacity_ValueChanged(object sender, Windows.UI.Xaml.Controls.Primitives.RangeBaseValueChangedEventArgs e)
        {
            if (_acrylicBrush != null)
            {
                _acrylicBrush.TintLuminosityOpacity = TintLuminosityOpacity.Value;
            }
        }

        private void AutoTintLuminosityOpacity_IsCheckedChanged(object sender, RoutedEventArgs e)
        {
            TintLuminosityOpacity.IsEnabled = AutoTintLuminosityOpacity.IsChecked != true;

            if (_acrylicBrush != null)
            {
                _acrylicBrush.TintLuminosityOpacity = TintLuminosityOpacity.IsEnabled ? TintLuminosityOpacity.Value : (double?) null;
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

#if !BUILD_LEAN_MUX_FOR_THE_STORE_APP
        private void ColorPicker_ColorChanged(ColorPicker sender, ColorChangedEventArgs args)
        {
            _acrylicBrush.TintColor = args.NewColor;
        }
#endif


        private void RunTestButton_Clicked(object sender, RoutedEventArgs e)
        {
            var selectedText = TestNameComboBox.GetSelectedText();

            // This group of tests is separate as it requires manual control of MaterialSetupHelper flags
            switch (selectedText)
            {
                case "AcrylicAlwaysUseFallback": RunAcrylicAlwaysUseFallback(); return;
                case "HideAndShowWindow": RunHideAndShowWindow(); return;
            }

            using (var setup = new MaterialSetupHelper())
            {
                switch (selectedText)
                {
                    case "BasicAcrylicOnRectangle": RunBasicAcrylicOnRectangle(); break;
                    case "AcrylicPropertyChanges": RunAcrylicPropertyChanges(); break;
                    case "VerifyDisconnectedState": RunVerifyDisconnectedState(); break;
                    case "AcrylicCreatedInFallbackMode": RunAcrylicCreatedInFallbackMode(); break;
                    case "VerifyOpaqueTintOptimization": RunVerifyOpaqueTintOptimization(); break;
                    case "TintTransitionDuration": RunTintTransitionDuration(); break;
                    case "AcrylicNoiseCache": RunAcrylicNoiseCache(); break;
                    case "VerifyAcrylicBrushEffect": RunVerifyAcrylicBrushEffect(); break;
                }
            }
        }

        void RunBasicAcrylicOnRectangle()
        {
            // Now that AB has entered the live tree, we can validate that it's backed by an EffectBrush
            // Unfortunately at this point we lack COMP API's to walk + validate the effect graph.
            SetAcrylicBrush(AcrylicBackgroundSource.Backdrop);
            bool isUsingAcrylicBrush = UpdateIsUsingAcrylicBrush();
            var compositionBrush = UpdateCompositionBrush();
            var noiseBrush = UpdateNoiseBrush();

            bool result = false;
            if (isUsingAcrylicBrush)
            {
                // For AcrylicBrush, validate we're using a WUC effect brush and noise is present
                result = ((compositionBrush != null && compositionBrush is CompositionEffectBrush) &&
                          (noiseBrush != null && noiseBrush is CompositionSurfaceBrush));
            }
            else
            {
                // For Fallback case, validate we have a WUC color brush with the right color
                //TestResult.Text = "FallbackCase: Start";
                CompositionColorBrush myColorBrush = compositionBrush as CompositionColorBrush;
                result = myColorBrush.Color.Equals(_acrylicBrush.FallbackColor);
                //TestResult.Text = "FallbackCase: (" + myColorBrush.Color.A.ToString() + ", " + myColorBrush.Color.R + ", " + myColorBrush.Color.G + ", " + myColorBrush.Color.B + ")";
            }

            TestResult.Text = "BasicAcrylicOnRectangle: " + (result ? "Passed" : "Failed");
        }

        // >> Test changes to TintOpacity, TintColor, AcrylicBackgroundsource and Fallback Color
        //    recreate or reuse exsiting AcrylicBursh as expected.
        // >> Also covers AcrylicBrush sharing and rendering in SW-rasterized element (Ellipse)
        void RunAcrylicPropertyChanges()
        {
            AcrylicBrush acrylicBrush1 = new AcrylicBrush
            {
                BackgroundSource = AcrylicBackgroundSource.Backdrop,
                FallbackColor = Colors.Blue,
                TintColor = Colors.Green,
                TintOpacity = 0.1
            };

            AcrylicBrush acrylicBrush2 = new AcrylicBrush
            {
                BackgroundSource = AcrylicBackgroundSource.HostBackdrop,
                FallbackColor = Colors.Blue,
                TintColor = Colors.Bisque,
                TintOpacity = 0.8
            };

            // Add Ellipse with Acrylic Fill and Stroke
            Ellipse1.Visibility = Visibility.Visible;
            Ellipse1.Fill = acrylicBrush1;
            Ellipse1.Stroke = acrylicBrush2;

            // Reuse Ellipse's brush in Rectangle 
            Rectangle1.Fill = acrylicBrush2;

            _acrylicTestApi.AcrylicBrush = acrylicBrush1;
            if (_acrylicTestApi.IsUsingAcrylicBrush == false)
            {
                TestResult.Text = "VerifyOpaqueTintOptimization: Skipped";
                return;
            }

            bool[] results = { false, false, false, false, false, false, false };

            _acrylicTestApi.AcrylicBrush = acrylicBrush1;
            results[0] = VerifyAcrylic();

            _acrylicTestApi.AcrylicBrush = acrylicBrush2;
            results[1] = VerifyAcrylic();

            // Update Tint - should not recreate brush
            _acrylicTestApi.AcrylicBrush = acrylicBrush1;
            CompositionBrush originalAcrylicBrush = UpdateCompositionBrush();
            acrylicBrush1.TintColor = Colors.DeepSkyBlue;
            CompositionBrush updatedAcrylicBrush1 = UpdateCompositionBrush();
            results[2] = VerifyAcrylic() && Object.ReferenceEquals(originalAcrylicBrush, updatedAcrylicBrush1);

            // Update TintOpacity - should not recreate brush
            acrylicBrush1.TintOpacity = 0.123456789;
            CompositionBrush updatedAcrylicBrush2 = UpdateCompositionBrush();
            results[3] = VerifyAcrylic() && Object.ReferenceEquals(originalAcrylicBrush, updatedAcrylicBrush2);

            // Update BackgroundSource to HostBackdrop - should recreate brush
            acrylicBrush1.BackgroundSource = AcrylicBackgroundSource.HostBackdrop;
            CompositionBrush updatedAcrylicBrush3 = UpdateCompositionBrush();
            results[4] = VerifyAcrylic() && !Object.ReferenceEquals(originalAcrylicBrush, updatedAcrylicBrush3);

            // Update BackgroundSource back to Backdrop - should recreate brush
            acrylicBrush1.BackgroundSource = AcrylicBackgroundSource.Backdrop;
            CompositionBrush updatedAcrylicBrush4 = UpdateCompositionBrush();
            results[5] = VerifyAcrylic() &&
                         !Object.ReferenceEquals(originalAcrylicBrush, updatedAcrylicBrush4) &&
                         !Object.ReferenceEquals(updatedAcrylicBrush3, updatedAcrylicBrush4);

            // Update FallbackColor  - should not recreate brush or trigger fallback
            acrylicBrush1.FallbackColor = Colors.LightGoldenrodYellow;
            CompositionBrush updatedAcrylicBrush5 = UpdateCompositionBrush();
            results[6] = VerifyAcrylic() && !Object.ReferenceEquals(originalAcrylicBrush, updatedAcrylicBrush5);

            bool result = results[0] && results[1] && results[2] && results[3] && results[4] && results[5] && results[6];
            string resultString = System.String.Format("({0},{1},{2},{3},{4},{5},{6})", results[0], results[1], results[2], results[3], results[4], results[5], results[6]);

            TestResult.Text = "AcrylicPropertyChanges: " + (result ? "Passed" : ("Failed " + resultString));
        }

        // Test Acrylic's integrity when window is minimized and restored
        void RunHideAndShowWindow()
        {
            // This helper is called twice from the InteractionTest (AcrylicBrushTests.HideAndShowWindow)
            // Window maniulation must be done on InteractionTest side, since app will get suspended as part of test
            if (_iteration_RunHideAndShowWindow == 0)
            {
                // Need to set override flags here manually since we don't want to 
                // unset them until the second phase of the test is complete
                MaterialHelperTestApi.IgnoreAreEffectsFast = true;
                MaterialHelperTestApi.SimulateDisabledByPolicy = false;

                SetAcrylicBrush(AcrylicBackgroundSource.HostBackdrop);
                UpdateSharedState();
                _previousCompositionBrush = UpdateCompositionBrush();

                if (_acrylicTestApi.IsUsingAcrylicBrush == false)
                {
                    TestResult.Text = "HideAndShowWindow: Skipped";
                }
                else
                {
                    TestResult.Text = "HideAndShowWindow: Continue Test";
                }
            }
            else
            {
                bool result = false;

                if (_iteration_RunHideAndShowWindow == 1)
                {
                    // Validate that on MUX Brush is recreated following Suspend/Resume, while in WUXC it is not.
                    CompositionBrush currentComposotionBrush = UpdateCompositionBrush();
#if BUILD_WINDOWS
                    result = Object.ReferenceEquals(_previousCompositionBrush, currentComposotionBrush);
#else
                    result = !Object.ReferenceEquals(_previousCompositionBrush, currentComposotionBrush);
#endif
                }

                // Unset all override flags to avoid impacting subsequent tests
                MaterialHelperTestApi.IgnoreAreEffectsFast = false;
                MaterialHelperTestApi.SimulateDisabledByPolicy = false;

                TestResult.Text = "HideAndShowWindow: " + (result ? "Passed" : "Failed");
            }

            _iteration_RunHideAndShowWindow++;
        }

        // Test AlwaysUseFallback property
        void RunAcrylicAlwaysUseFallback()
        {
            // Need to set override flags here manually since we don't want to 
            // unset them until worker thread actions are complete
            MaterialHelperTestApi.IgnoreAreEffectsFast = true;
            MaterialHelperTestApi.SimulateDisabledByPolicy = false;

            SetAcrylicBrush(AcrylicBackgroundSource.Backdrop);

            // Run these tests off-thread as they need to poll for brush status changes that are deferred by transition animations.
            Task.Run(async () =>
            {
                // If we are in fallback mode already due to policy, skip the test
                if (await VerifyAcrylicWithPolling())
                {
                    bool[] results = { false, false, false, false };

                    RunOnUIThread.Execute(() =>
                    {
                        // Set AlwaysUseFallback, verify we are using the expected fallback color
                        _acrylicBrush.AlwaysUseFallback = true;
                    });
                    results[0] = await VerifyFallbackColorWithPolling();

                    RunOnUIThread.Execute(() =>
                    {
                        // Change fallback color while in fallback, veryify fallback brush is updated
                        _acrylicBrush.FallbackColor = Colors.Purple;
                    });
                    results[1] = await VerifyFallbackColorWithPolling();

                    RunOnUIThread.Execute(() =>
                    {
                        // Unset AlwaysUseFallback, verify we are using acrylic again
                        _acrylicBrush.AlwaysUseFallback = false;
                    });
                    results[2] = await VerifyAcrylicWithPolling();

                    RunOnUIThread.Execute(() =>
                    {
                        // Change fallback color while not in fallback, then force fallback and verify color
                        _acrylicBrush.FallbackColor = Colors.Orange;
                        _acrylicBrush.AlwaysUseFallback = true;
                    });
                    results[3] = await VerifyFallbackColorWithPolling();

                    bool result = results[0] && results[1] && results[2] && results[3];
                    string resultString = System.String.Format("({0},{1},{2},{3})", results[0], results[1], results[2], results[3]);

                    RunOnUIThread.Execute(() =>
                    {
                        // Unset all override flags to avoid impacting subsequent tests
                        MaterialHelperTestApi.IgnoreAreEffectsFast = false;
                        MaterialHelperTestApi.SimulateDisabledByPolicy = false;

                        TestResult.Text = "AcrylicAlwaysUseFallback: " + (result ? "Passed" : ("Failed " + resultString));
                    });
                }
                else
                {
                    RunOnUIThread.Execute(() =>
                    {
                        // Unset all override flags to avoid impacting subsequent tests
                        MaterialHelperTestApi.IgnoreAreEffectsFast = false;
                        MaterialHelperTestApi.SimulateDisabledByPolicy = false;

                        TestResult.Text = "AcrylicAlwaysUseFallback: Skipped";
                    });
                }
            });
        }

        // In non fallback mode, we have crossfading and non crossfading effects. 
        // crossfading only last for hundren ms in the transition between fallback mode and acrylic mode.
        // 
        void RunVerifyAcrylicBrushEffect()
        {
            bool[] results = { false, false };

            AcrylicBrush acrylicBrush1 = new AcrylicBrush
            {
                BackgroundSource = AcrylicBackgroundSource.Backdrop,
                FallbackColor = Colors.Blue,
                TintColor = Colors.Green,
                TintOpacity = 0.1
            };

            // Add Ellipse with Acrylic Fill and Stroke
            Ellipse1.Visibility = Visibility.Visible;
            Ellipse1.Fill = acrylicBrush1;

            _acrylicTestApi.AcrylicBrush = acrylicBrush1;

            // Force to create a AcrylicBrush which is used for animation. So it's a CrosssFading effect brush
            _acrylicTestApi.ForceCreateAcrylicBrush(true /*useCrossFadeEffect*/);

            var fallbackColor = Colors.Black;
            var tintColor = Colors.Black;

            // Get FallbackColor and TintColor. Only CrossFading effect brush provides animation properties for both and TryGetColor success. 
            CompositionGetValueStatus fallbackColorValueStatus = _acrylicTestApi.CompositionBrush.Properties.TryGetColor("FallbackColor.Color", out fallbackColor);
            CompositionGetValueStatus tintColorValueStatus = _acrylicTestApi.CompositionBrush.Properties.TryGetColor("TintColor.Color", out tintColor);

            if (PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.NineteenH1))
            {
                // On 19H1+, Luminosity impacts the effective tint value, so don't validate it here.
                results[0] = tintColorValueStatus != CompositionGetValueStatus.NotFound &&
                             fallbackColorValueStatus != CompositionGetValueStatus.NotFound &&
                             fallbackColor == acrylicBrush1.FallbackColor;
            }
            else
            {
                var expectIintColor = acrylicBrush1.TintColor;
                expectIintColor.A = (byte)Math.Ceiling(255 * acrylicBrush1.TintOpacity); // alpha channel
                results[0] = fallbackColor == acrylicBrush1.FallbackColor && 
                             expectIintColor == tintColor;
            }

            // Force to create a non crosssFading effect brush
            _acrylicTestApi.ForceCreateAcrylicBrush(false /*useCrossFadeEffect*/);

            fallbackColor = Colors.Black;
            tintColor = Colors.Black;

            // Get FallbackColor and TintColor. Non CrossFading effect brush doesn't provide FallbackColor.Color.
            fallbackColorValueStatus =  _acrylicTestApi.CompositionBrush.Properties.TryGetColor("FallbackColor.Color", out fallbackColor);
            tintColorValueStatus = _acrylicTestApi.CompositionBrush.Properties.TryGetColor("TintColor.Color", out tintColor);


            if (PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.NineteenH1))
            {
                // On 19H1+, Luminosity impacts the effective tint value, so don't validate it here.
                results[1] = fallbackColorValueStatus == CompositionGetValueStatus.NotFound;
            }
            else
            {
                var expectIintColor = acrylicBrush1.TintColor;
                expectIintColor.A = (byte)Math.Ceiling(255 * acrylicBrush1.TintOpacity); // alpha channel

                results[1] = fallbackColorValueStatus == CompositionGetValueStatus.NotFound &&
                             expectIintColor == tintColor;
            }

            bool result = results[0] && results[1];
            string resultString = System.String.Format("({0},{1})", results[0], results[1]);
            TestResult.Text = "VerifyAcrylicBrushEffect: " + (result ? "Passed" : ("Failed " + resultString));
        }

        // Test in-app and window acrylic created while in fallback mode
        void RunAcrylicCreatedInFallbackMode()
        {
            using (var setup = new MaterialSetupHelper(true /* ignoreAreEffectsFast*/, true /* simulateDisabledByPolicy */ ))
            {
                // Test app acrylic
                _acrylicBrush = new AcrylicBrush
                {
                    BackgroundSource = AcrylicBackgroundSource.Backdrop,
                    FallbackColor = Color.FromArgb(0xFF, 0x0, 0x0, 0xFF),
                    TintOpacity = TintOpacity.Value
                };

                _acrylicTestApi.AcrylicBrush = _acrylicBrush;

                Rectangle1.Fill = _acrylicBrush;
                bool result1 = VerifyFallbackColor();

                // Test window acrylic
                _acrylicBrush = new AcrylicBrush
                {
                    BackgroundSource = AcrylicBackgroundSource.HostBackdrop,
                    FallbackColor = Color.FromArgb(0xFF, 0x0, 0xFF, 0xFF),
                    TintOpacity = TintOpacity.Value
                };
                _acrylicTestApi.AcrylicBrush = _acrylicBrush;

                Rectangle1.Fill = _acrylicBrush;
                bool result2 = VerifyFallbackColor();

                TestResult.Text = "AcrylicCreatedInFallbackMode: " + (result1 && result2 ? "Passed" : "Failed ");
            }
        }

        // Test adding an AcrylicBrush, animating the affected element, and then removing the brush
        void RunVerifyDisconnectedState()
        {
            _acrylicBrush = new AcrylicBrush
            {
                BackgroundSource = AcrylicBackgroundSource.Backdrop,
                FallbackColor = Color.FromArgb(0xFF, 0x0, 0x0, 0xFF),
                TintOpacity = TintOpacity.Value
            };
            _acrylicTestApi.AcrylicBrush = _acrylicBrush;
            Rectangle1.Fill = _acrylicBrush;

            if (_acrylicTestApi.IsUsingAcrylicBrush == false)
            {
                TestResult.Text = "VerifyDisconnectedState: Skipped";
                return;
            }

            DoubleAnimation positionAnimation = new DoubleAnimation();
            positionAnimation.By = 100;
            positionAnimation.Duration = new TimeSpan(0, 0, 0, 0, 200);
            Storyboard.SetTarget(positionAnimation, MyTranslateTransform);
            Storyboard.SetTargetProperty(positionAnimation, "X");
            Storyboard sb = new Storyboard();
            sb.Children.Add(positionAnimation);
            sb.Completed += RunVerifyDisconnectedState_AnimationCompleted;
            sb.Begin();
        }

        private void RunVerifyDisconnectedState_AnimationCompleted(object sender, object e)
        {
            Rectangle1.Fill = new SolidColorBrush(Colors.Purple);
            bool result = VerifyDisconnected();

            TestResult.Text = "VerifyDisconnectedState: " + (result ? "Passed" : "Failed ");
        }

        // Test adding an AcrylicBrush and switching its tint transparency status 
        // to validate that we use optimized opaque tint effect chain
        void RunVerifyOpaqueTintOptimization()
        {
            // Start out with transparent tint
            _acrylicBrush = new AcrylicBrush
            {
                BackgroundSource = AcrylicBackgroundSource.Backdrop,
                FallbackColor = Colors.Blue,
                TintColor = Colors.Green,
                TintOpacity = 0.1
            };
            _acrylicTestApi.AcrylicBrush = _acrylicBrush;
            Rectangle1.Fill = _acrylicBrush;

            if (_acrylicTestApi.IsUsingAcrylicBrush == false)
            {
                TestResult.Text = "VerifyOpaqueTintOptimization: Skipped";
                return;
            }

            bool[] results = { false, false, false, false };

            CompositionBrush originalTransparentBrush = UpdateCompositionBrush();
            results[0] = VerifyAcrylic();

            // Switch to opaque tint
            _acrylicBrush.TintOpacity = 1.0;
            CompositionBrush updatedOpaqueBrush = UpdateCompositionBrush();
            results[1] = VerifyAcrylic();

            // Switch back to transparent tint
            _acrylicBrush.TintOpacity = 0.1;
            CompositionBrush updatedTransapentBrush = UpdateCompositionBrush();
            results[2] = VerifyAcrylic();

            // Make a new brush got created in each case
            results[3] =
                !Object.ReferenceEquals(originalTransparentBrush, updatedOpaqueBrush) &&
                !Object.ReferenceEquals(originalTransparentBrush, updatedTransapentBrush) &&
                !Object.ReferenceEquals(updatedOpaqueBrush, updatedTransapentBrush);

            bool result = results[0] && results[1] && results[2] && results[3];
            string resultString = System.String.Format("({0},{1},{2},{3})", results[0], results[1], results[2], results[3]);

            TestResult.Text = "VerifyOpaqueTintOptimization: " + (result ? "Passed" : ("Failed " + resultString));
        }

        void RunTintTransitionDuration()
        {
            int durationMS = 0;

            if (_iteration_TintTransitionDuration == 0) { durationMS = 0; }
            else if (_iteration_TintTransitionDuration == 1) { durationMS = 500; }
            else if (_iteration_TintTransitionDuration == 2) { durationMS = 999; }

            _iteration_TintTransitionDuration++;

            _acrylicBrush = new AcrylicBrush
            {
                BackgroundSource = AcrylicBackgroundSource.Backdrop,
                FallbackColor = Colors.Blue,
                TintColor = Colors.Green,
                TintOpacity = 0.8
            };
            _acrylicTestApi.AcrylicBrush = _acrylicBrush;
            Rectangle1.Fill = _acrylicBrush;

            _acrylicBrush.TintTransitionDuration = new TimeSpan(0, 0, 0, 0, durationMS);
            _acrylicBrush.TintColor = Colors.DarkOrange;
        }

        // Test noise cache is functional:
        // 1. Validate two AcrylicBrushes both use the same noise
        // 2. Validate destroying these AB's and creating a new one still use the same noise.
        void RunAcrylicNoiseCache()
        {
            AcrylicBrush acrylicBrush1 = new AcrylicBrush
            {
                BackgroundSource = AcrylicBackgroundSource.Backdrop,
                FallbackColor = Colors.Blue,
                TintColor = Colors.Green,
                TintOpacity = 0.1
            };

            AcrylicBrush acrylicBrush2 = new AcrylicBrush
            {
                BackgroundSource = AcrylicBackgroundSource.Backdrop,
                FallbackColor = Colors.Blue,
                TintColor = Colors.Bisque,
                TintOpacity = 1
            };

            Rectangle1.Fill = acrylicBrush1;
            _acrylicTestApi.AcrylicBrush = acrylicBrush1;
            CompositionBrush noiseBrush1 = UpdateNoiseBrush();

            if (_acrylicTestApi.IsUsingAcrylicBrush == false)
            {
                TestResult.Text = "AcrylicNoiseCache: Skipped";
                return;
            }

            Rectangle1.Stroke = acrylicBrush2;
            _acrylicTestApi.AcrylicBrush = acrylicBrush2;
            CompositionBrush noiseBrush2 = UpdateNoiseBrush();

            // Make sure two brushes conccurrently in tree use the same noise
            bool result1 = Object.ReferenceEquals(noiseBrush1, noiseBrush2);

            // Destroy existing brushes
            Rectangle1.Fill = new SolidColorBrush(Colors.PaleGoldenrod);
            Rectangle1.Stroke = new SolidColorBrush(Colors.PaleGoldenrod);
            acrylicBrush1 = null;
            acrylicBrush2 = null;
            _acrylicTestApi.AcrylicBrush = null;
            GC.Collect();

            AcrylicBrush acrylicBrush3 = new AcrylicBrush
            {
                BackgroundSource = AcrylicBackgroundSource.Backdrop,
                FallbackColor = Colors.Blue,
                TintColor = Colors.DarkOliveGreen,
                TintOpacity = 0.25
            };

            Rectangle1.Fill = acrylicBrush3;
            _acrylicTestApi.AcrylicBrush = acrylicBrush3;
            CompositionBrush noiseBrush3 = UpdateNoiseBrush();

            // Make sure new brush still uses the same noise
            bool result2 = Object.ReferenceEquals(noiseBrush2, noiseBrush3);

            bool result = result1 && result2;
            string resultString = System.String.Format("({0},{1})", result1, result2);

            TestResult.Text = "AcrylicNoiseCache: " + (result ? "Passed" : ("Failed " + resultString));
        }

        bool VerifyDisconnected()
        {
            bool isUsingAcrylicBrush = UpdateIsUsingAcrylicBrush();
            var compositionBrush = UpdateCompositionBrush();
            var noiseBrush = UpdateNoiseBrush();
            return !isUsingAcrylicBrush &&
                    compositionBrush == null && 
                    noiseBrush == null;     // Noise is cached in MaterialHelper but not in individual brushes
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

        bool VerifyFallbackColor()
        {
            bool isUsingAcrylicBrush = UpdateIsUsingAcrylicBrush();
            var compositionBrush = UpdateCompositionBrush();
            return !isUsingAcrylicBrush &&
                   (compositionBrush != null && compositionBrush is CompositionColorBrush) &&
                   (compositionBrush as CompositionColorBrush).Color.Equals(_acrylicBrush.FallbackColor);
        }

        // Poll for up to 1 sec for underlyig WUC brush to become a CompositionColorBrush using the FallbackColor.
        // This is needed because fallback <-> acrylic changes (and color changes in the fallback brush, if it is in use)
        // are delayed until transition animations are completed.
        // Since XCBB.CompositionBrush is not a DependencyProperty, we can't get a property change notification.
        private async Task PollForFallbackBrush()
        {
            await Task.Run(async () =>
            {
                for (int i = 0; i < 5; i++)
                {
                    bool hasFallbackBrush = false;
                    RunOnUIThread.Execute(() =>
                    {
                        CompositionColorBrush polledBrushAsColorBrush = _acrylicTestApi.CompositionBrush as CompositionColorBrush;
                        hasFallbackBrush = polledBrushAsColorBrush != null && polledBrushAsColorBrush.Color.Equals(_acrylicBrush.FallbackColor);
                    });
                    if (hasFallbackBrush) { break; }

                    await Task.Delay(200);
                }
            });
        }

        // Poll for up to 1 sec for underlyig WUC brush to become a CompositonEffectBrush.
        // This is needed because fallback <-> acrylic changes are delayed until transition animations are completed.
        // Since XCBB.CompositionBrush is not a DependencyProperty, we can't get a property change notification.
        private async Task PollForEffectBrush()
        {
            await Task.Run(async () =>
            {
                for (int i = 0; i < 5; i++)
                {
                    bool hasEffectBrush = false;
                    RunOnUIThread.Execute(() =>
                    {
                        CompositionEffectBrush polledBrushsAEffectBrush = _acrylicTestApi.CompositionBrush as CompositionEffectBrush;
                        hasEffectBrush = polledBrushsAEffectBrush != null;
                    });
                    if (hasEffectBrush) { break; }

                    await Task.Delay(200);
                }
            });
        }
        private async Task<bool> VerifyAcrylicWithPolling()
        {
            await PollForEffectBrush();

            bool result = false;
            RunOnUIThread.Execute(() =>
            {
                result = VerifyAcrylic();
            });

            return result;
        }

        private async Task<bool> VerifyFallbackColorWithPolling()
        {
            await PollForFallbackBrush();

            bool result = false;
            RunOnUIThread.Execute(() =>
            {
                result = VerifyFallbackColor();
            });

            return result;
        }

        // We use UIA AutopmationEvents to send notifications about Window Visisbility changes,
        // since there are no direct events for this exposed directly in UIA / Mita. 
        // Also, here isn't a custom AutomationEvent type, so use an existing event type - AsyncContentLoaded.
        // The InteractionTest expects it to fire in a specific pattern and hence extracts its own meaning 
        // from each firing of the event.
        private void XamlWindow_VisibilityChanged(object sender, Windows.UI.Core.VisibilityChangedEventArgs e)
        {
            var automationPeer = FrameworkElementAutomationPeer.FromElement(this);
            automationPeer.RaiseAutomationEvent(AutomationEvents.AsyncContentLoaded);
        }

        private void SetAcrylicBrush(AcrylicBackgroundSource backgroundSource)
        {
            _acrylicBrush.BackgroundSource = backgroundSource;
            Rectangle1.Fill = _acrylicBrush;
            _acrylicTestApi.AcrylicBrush = _acrylicBrush;
        }

        private void UpdateSharedState()
        {
            UpdateIsUsingAcrylicBrush();
            UpdateCompositionBrush();
            UpdateNoiseBrush();
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

        private void UpdateStateButton_Click(object sender, RoutedEventArgs e)
        {
            UpdateSharedState();
        }

        private class WrapperEffectBrush : XamlCompositionBrushBase
        {
            public WrapperEffectBrush(CompositionEffectBrush brush)
            {
                this.CompositionBrush = brush;
            }
        }

        private void CreateAcrylicEffectBrush_Click(object sender, RoutedEventArgs e)
        {
            if (Overlay.Background == null)
            {
                var compositor = Window.Current.Compositor;
                var noiseSurface = LoadedImageSurface.StartLoadFromUri(new Uri("ms-appx:///Microsoft.UI.Xaml/Assets/NoiseAsset_256X256_PNG.png"));

                var acrylicFactory = System.Runtime.InteropServices.WindowsRuntime.WindowsRuntimeMarshal.GetActivationFactory(typeof(AcrylicBrush));
                var privateApi = (IAcrylicBrushStaticsPrivate)acrylicFactory;
                var acrylicEffectBrush = privateApi.CreateBackdropAcrylicEffectBrush(
                    compositor,
                    Color.FromArgb(0x44, 0x00, 0xFF, 0x00),
                    Colors.Transparent,
                    false);

                var noiseBrush = compositor.CreateSurfaceBrush(noiseSurface);
                noiseBrush.Stretch = CompositionStretch.None;

                acrylicEffectBrush.SetSourceParameter("Noise", noiseBrush);

                Overlay.Background = new WrapperEffectBrush(acrylicEffectBrush);
            }
            else
            {
                Overlay.Background = null;
            }
        }

        private void SetAcrylicBrushButton_Click(object sender, RoutedEventArgs e)
        {
            SetAcrylicBrush(AcrylicBackgroundSource.Backdrop);
        }

        private void Overlay_PointerPressed(object sender, Windows.UI.Xaml.Input.PointerRoutedEventArgs e)
        {
            Overlay.Background = null;
        }
    }
}
