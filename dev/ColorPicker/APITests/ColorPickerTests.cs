// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using MUXControlsTestApp;
using MUXControlsTestApp.Utilities;
using System;
using System.Numerics;
using System.Collections;
using System.Linq;
using System.Threading;
using Windows.Foundation;
using Windows.UI;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Shapes;
using Windows.UI.Xaml.Media;
using Common;

#if USING_TAEF
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;
#else
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.VisualStudio.TestTools.UnitTesting.Logging;
#endif

using ColorSpectrumShape = Microsoft.UI.Xaml.Controls.ColorSpectrumShape;
using ColorSpectrumComponents = Microsoft.UI.Xaml.Controls.ColorSpectrumComponents;
using ColorPicker = Microsoft.UI.Xaml.Controls.ColorPicker;
using ColorChangedEventArgs = Microsoft.UI.Xaml.Controls.ColorChangedEventArgs;
using ColorSpectrum = Microsoft.UI.Xaml.Controls.Primitives.ColorSpectrum;
using XamlControlsXamlMetaDataProvider = Microsoft.UI.Xaml.XamlTypeInfo.XamlControlsXamlMetaDataProvider;

namespace Windows.UI.Xaml.Tests.MUXControls.ApiTests
{
    [TestClass]
    public class ColorPickerTests : ApiTestBase
    {
        [TestMethod]
        public void ColorPickerTest()
        {
            RunOnUIThread.Execute(() =>
            {
                ColorPicker colorPicker = new ColorPicker();
                Verify.IsNotNull(colorPicker);

                Verify.AreEqual(Colors.White, colorPicker.Color);
                Verify.IsNull(colorPicker.PreviousColor);
                Verify.IsFalse(colorPicker.IsAlphaEnabled);
                Verify.IsTrue(colorPicker.IsColorSpectrumVisible);
                Verify.IsTrue(colorPicker.IsColorPreviewVisible);
                Verify.IsTrue(colorPicker.IsColorSliderVisible);
                Verify.IsTrue(colorPicker.IsAlphaSliderVisible);
                Verify.IsFalse(colorPicker.IsMoreButtonVisible);
                Verify.IsTrue(colorPicker.IsColorChannelTextInputVisible);
                Verify.IsTrue(colorPicker.IsAlphaTextInputVisible);
                Verify.IsTrue(colorPicker.IsHexInputVisible);
                Verify.AreEqual(0, colorPicker.MinHue);
                Verify.AreEqual(359, colorPicker.MaxHue);
                Verify.AreEqual(0, colorPicker.MinSaturation);
                Verify.AreEqual(100, colorPicker.MaxSaturation);
                Verify.AreEqual(0, colorPicker.MinValue);
                Verify.AreEqual(100, colorPicker.MaxValue);
                Verify.AreEqual(ColorSpectrumShape.Box, colorPicker.ColorSpectrumShape);
                Verify.AreEqual(ColorSpectrumComponents.HueSaturation, colorPicker.ColorSpectrumComponents);

                // Clamping the min and max properties changes the color value,
                // so let's test this new value before we change those.
                colorPicker.Color = Colors.Green;
                Verify.AreEqual(Colors.Green, colorPicker.Color);

                colorPicker.PreviousColor = Colors.Red;
                colorPicker.IsAlphaEnabled = true;
                colorPicker.IsColorSpectrumVisible = false;
                colorPicker.IsColorPreviewVisible = false;
                colorPicker.IsColorSliderVisible = false;
                colorPicker.IsAlphaSliderVisible = false;
                colorPicker.IsMoreButtonVisible = true;
                colorPicker.IsColorChannelTextInputVisible = false;
                colorPicker.IsAlphaTextInputVisible = false;
                colorPicker.IsHexInputVisible = false;
                colorPicker.MinHue = 10;
                colorPicker.MaxHue = 300;
                colorPicker.MinSaturation = 10;
                colorPicker.MaxSaturation = 90;
                colorPicker.MinValue = 10;
                colorPicker.MaxValue = 90;
                colorPicker.ColorSpectrumShape = ColorSpectrumShape.Ring;
                colorPicker.ColorSpectrumComponents = ColorSpectrumComponents.HueValue;

                Verify.AreNotEqual(Colors.Green, colorPicker.Color);
                Verify.AreEqual(Colors.Red, colorPicker.PreviousColor);
                Verify.IsTrue(colorPicker.IsAlphaEnabled);
                Verify.IsFalse(colorPicker.IsColorSpectrumVisible);
                Verify.IsFalse(colorPicker.IsColorPreviewVisible);
                Verify.IsFalse(colorPicker.IsColorSliderVisible);
                Verify.IsFalse(colorPicker.IsAlphaSliderVisible);
                Verify.IsTrue(colorPicker.IsMoreButtonVisible);
                Verify.IsFalse(colorPicker.IsColorChannelTextInputVisible);
                Verify.IsFalse(colorPicker.IsAlphaTextInputVisible);
                Verify.IsFalse(colorPicker.IsHexInputVisible);
                Verify.AreEqual(10, colorPicker.MinHue);
                Verify.AreEqual(300, colorPicker.MaxHue);
                Verify.AreEqual(10, colorPicker.MinSaturation);
                Verify.AreEqual(90, colorPicker.MaxSaturation);
                Verify.AreEqual(10, colorPicker.MinValue);
                Verify.AreEqual(90, colorPicker.MaxValue);
                Verify.AreEqual(ColorSpectrumShape.Ring, colorPicker.ColorSpectrumShape);
                Verify.AreEqual(ColorSpectrumComponents.HueValue, colorPicker.ColorSpectrumComponents);
            });
        }

        [TestMethod]
        public void ColorPickerEventsTest()
        {
            RunOnUIThread.Execute(() =>
            {
                ColorPicker colorPicker = new ColorPicker();

                colorPicker.ColorChanged += (ColorPicker sender, ColorChangedEventArgs args) =>
                {
                    Verify.AreEqual(args.OldColor, Colors.White);
                    Verify.AreEqual(args.NewColor, Colors.Green);
                };

                colorPicker.Color = Colors.Green;
            });
        }

        [TestMethod]
        public void ColorSpectrumTest()
        {
            RunOnUIThread.Execute(() =>
            {
                ColorSpectrum colorSpectrum = new ColorSpectrum();
                Verify.IsNotNull(colorSpectrum);

                Verify.AreEqual(Colors.White, colorSpectrum.Color);
                Verify.AreEqual(new Vector4() { X = 0.0f, Y = 0.0f, Z = 1.0f, W = 1.0f}, colorSpectrum.HsvColor);
                Verify.AreEqual(0, colorSpectrum.MinHue);
                Verify.AreEqual(359, colorSpectrum.MaxHue);
                Verify.AreEqual(0, colorSpectrum.MinSaturation);
                Verify.AreEqual(100, colorSpectrum.MaxSaturation);
                Verify.AreEqual(0, colorSpectrum.MinValue);
                Verify.AreEqual(100, colorSpectrum.MaxValue);
                Verify.AreEqual(ColorSpectrumShape.Box, colorSpectrum.Shape);
                Verify.AreEqual(ColorSpectrumComponents.HueSaturation, colorSpectrum.Components);

                colorSpectrum.Color = Colors.Green;
                colorSpectrum.MinHue = 10;
                colorSpectrum.MaxHue = 300;
                colorSpectrum.MinSaturation = 10;
                colorSpectrum.MaxSaturation = 90;
                colorSpectrum.MinValue = 10;
                colorSpectrum.MaxValue = 90;
                colorSpectrum.Shape = ColorSpectrumShape.Ring;
                colorSpectrum.Components = ColorSpectrumComponents.HueValue;

                Verify.AreEqual(Colors.Green, colorSpectrum.Color);

                // We'll probably encounter some level of rounding error here,
                // so we want to check that the HSV color is *close* to what's expected,
                // not exactly equal.
                Verify.IsLessThan(Math.Abs(colorSpectrum.HsvColor.X - 120.0), 0.1);
                Verify.IsLessThan(Math.Abs(colorSpectrum.HsvColor.Y - 1.0), 0.1);
                Verify.IsLessThan(Math.Abs(colorSpectrum.HsvColor.Z - 0.5), 0.1);

                Verify.AreEqual(10, colorSpectrum.MinHue);
                Verify.AreEqual(300, colorSpectrum.MaxHue);
                Verify.AreEqual(10, colorSpectrum.MinSaturation);
                Verify.AreEqual(90, colorSpectrum.MaxSaturation);
                Verify.AreEqual(10, colorSpectrum.MinValue);
                Verify.AreEqual(90, colorSpectrum.MaxValue);
                Verify.AreEqual(ColorSpectrumShape.Ring, colorSpectrum.Shape);
                Verify.AreEqual(ColorSpectrumComponents.HueValue, colorSpectrum.Components);

                colorSpectrum.HsvColor = new Vector4() { X = 120.0f, Y = 1.0f, Z = 1.0f, W = 1.0f };

                Verify.AreEqual(Color.FromArgb(255, 0, 255, 0), colorSpectrum.Color);
                Verify.AreEqual(new Vector4() { X = 120.0f, Y = 1.0f, Z = 1.0f, W = 1.0f }, colorSpectrum.HsvColor);
            });
        }

        [TestMethod]
        public void ColorSpectrumEventsTest()
        {
            RunOnUIThread.Execute(() =>
            {
                ColorSpectrum colorSpectrum = new ColorSpectrum();

                colorSpectrum.ColorChanged += (ColorSpectrum sender, ColorChangedEventArgs args) =>
                {
                    Verify.AreEqual(args.OldColor, Colors.Red);
                    Verify.AreEqual(args.NewColor, Colors.Green);
                };

                colorSpectrum.Color = Colors.Green;
            });
        }

        [TestMethod]
        public void ColorPickerDerivationTest()
        {
            RunOnUIThread.Execute(() =>
            {
                // Just create the derived type -- all we're exercising here is the initialization path.
                // (ColorSpectrum does things in its initialization path that would QueryInterface the outer).
                MyColorSpectrum colorSpectrum = new MyColorSpectrum();
            });

            IdleSynchronizer.Wait();
        }

        [TestMethod]
        public void ValidateHueRange()
        {
            RunOnUIThread.Execute(() =>
            {
                var colorPicker = new ColorPicker();
                try
                {
                    colorPicker.MinHue = -1;
                    Verify.Fail("Invalid argument exception wasn't raised.");
                }
                catch(Exception e)
                {
                    Verify.IsTrue(e.Message.Contains("MinHue must be between 0 and 359."));
                }
            });

            IdleSynchronizer.Wait();
        }

        [TestMethod]
        public void ValidateFractionalWidthDoesNotCrash()
        {
            ColorSpectrum colorSpectrum = null;

            RunOnUIThread.Execute(() =>
            {
                colorSpectrum = new ColorSpectrum();

                // 332.75 is the fractional value that caused a crash in Settings when DPI was set to 200%
                // and text scaling was set to >160%.  It ensures that we exercise all of the round() fixes
                // that we made in ColorSpectrum to ensure we always round fractional values instead of
                // truncating them.
                colorSpectrum.Width = 332.75;
                colorSpectrum.Height = 332.75;
            });

            SetAsRootAndWaitForColorSpectrumFill(colorSpectrum);
        }

        // XamlControlsXamlMetaDataProvider does not exist in the OS repo,
        // so we can't execute this test as authored there.
        [TestMethod]
        public void VerifyColorPropertyMetadata()
        {
            RunOnUIThread.Execute(() =>
            {
                XamlControlsXamlMetaDataProvider provider = new XamlControlsXamlMetaDataProvider();
                var colorPickerType = provider.GetXamlType(typeof(ColorPicker).FullName);
                var colorMember = colorPickerType.GetMember("Color");
                var memberType = colorMember.Type;
                Verify.AreEqual(memberType.BaseType.FullName, "ValueType");
            });
        }

        [TestMethod]
        public void VerifyVisualTree()
        {
            ColorPicker colorPicker = null;
            RunOnUIThread.Execute(() =>
            {
                colorPicker = new ColorPicker { IsAlphaEnabled = true, Width=300, Height=600 };
            });
            TestUtilities.SetAsVisualTreeRoot(colorPicker);

            VisualTreeTestHelper.VerifyVisualTree(root: colorPicker, masterFilePrefix: "ColorPicker");
        }

        // This takes a FrameworkElement parameter so you can pass in either a ColorPicker or a ColorSpectrum.
        private void SetAsRootAndWaitForColorSpectrumFill(FrameworkElement element)
        {
            ManualResetEvent spectrumLoadedEvent = new ManualResetEvent(false);

            RunOnUIThread.Execute(() =>
            {
                element.Loaded += (sender, args) =>
                {
                    var spectrumRectangle = VisualTreeUtils.FindVisualChildByName(element, "SpectrumRectangle") as Rectangle;
                    Verify.IsNotNull(spectrumRectangle);

                    spectrumRectangle.RegisterPropertyChangedCallback(Shape.FillProperty, (o, dp) =>
                    {
                        spectrumLoadedEvent.Set();
                    });
                };

                Content = element;
                Content.UpdateLayout();
            });

            spectrumLoadedEvent.WaitOne();
        }
    }
}