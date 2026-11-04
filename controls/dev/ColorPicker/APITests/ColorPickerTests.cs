// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using MUXControlsTestApp;
using MUXControlsTestApp.Utilities;
using System;
using System.Numerics;
using System.Collections;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using System.Threading;
using Windows.Foundation;
using Windows.UI;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Shapes;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Media.Imaging;
using Microsoft.UI.Xaml.XamlTypeInfo;
using Common;
using SpectrumBrush = Microsoft.UI.Private.Controls.SpectrumBrush;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    [TestClass]
    public class ColorPickerTests : ApiTestBase
    {
        // Scenario: create a ColorPicker, check defaults, set and read back every property; also check that each DP
        //     identifier (incl. ColorPickerSlider.ColorChannel) is non-null, distinct and round-trips.
        // Expected: documented defaults (White, hue 0-359, sat/value 0-100, Box, HueSaturation); set values read back;
        //     Color leaves Green once the ranges clamp it; Get/Set/ClearValue agree with the CLR properties.
        // Failure means: a ColorPicker/ColorPickerSlider DP registration or default changed, a DP identifier is null or
        //     shared, or ColorPicker::OnMinMaxHue/Saturation/ValueChanged no longer clamps Color into the new ranges.
        [TestMethod]
        public void ColorPickerTest()
        {
            RunOnUIThread.Execute(() =>
            {
                ColorPicker colorPicker = new ColorPicker();
                Verify.IsNotNull(colorPicker);

                Verify.AreEqual(Microsoft.UI.Colors.White, colorPicker.Color);
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
                colorPicker.Color = Microsoft.UI.Colors.Green;
                Verify.AreEqual(Microsoft.UI.Colors.Green, colorPicker.Color);

                colorPicker.PreviousColor = Microsoft.UI.Colors.Red;
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

                Verify.AreNotEqual(Microsoft.UI.Colors.Green, colorPicker.Color);
                Verify.AreEqual(Microsoft.UI.Colors.Red, colorPicker.PreviousColor);
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

                // Dependency property identifiers: non-null, distinct and stable.
                VerifyDependencyPropertyIdentifiers(
                    ColorPicker.ColorProperty,
                    ColorPicker.PreviousColorProperty,
                    ColorPicker.IsAlphaEnabledProperty,
                    ColorPicker.IsColorSpectrumVisibleProperty,
                    ColorPicker.IsColorPreviewVisibleProperty,
                    ColorPicker.IsColorSliderVisibleProperty,
                    ColorPicker.IsAlphaSliderVisibleProperty,
                    ColorPicker.IsMoreButtonVisibleProperty,
                    ColorPicker.IsColorChannelTextInputVisibleProperty,
                    ColorPicker.IsAlphaTextInputVisibleProperty,
                    ColorPicker.IsHexInputVisibleProperty,
                    ColorPicker.MinHueProperty,
                    ColorPicker.MaxHueProperty,
                    ColorPicker.MinSaturationProperty,
                    ColorPicker.MaxSaturationProperty,
                    ColorPicker.MinValueProperty,
                    ColorPicker.MaxValueProperty,
                    ColorPicker.ColorSpectrumShapeProperty,
                    ColorPicker.ColorSpectrumComponentsProperty,
                    ColorPicker.OrientationProperty,
                    ColorPickerSlider.ColorChannelProperty);
                Verify.IsTrue(ReferenceEquals(ColorPicker.ColorProperty, ColorPicker.ColorProperty));
                Verify.IsTrue(ReferenceEquals(ColorPicker.OrientationProperty, ColorPicker.OrientationProperty));

                // GetValue/SetValue/ClearValue through each identifier agree with the CLR property.
                var picker = new ColorPicker();
                VerifyDependencyPropertyRoundTrip(picker, ColorPicker.ColorProperty, () => picker.Color, v => picker.Color = v, Microsoft.UI.Colors.White, Microsoft.UI.Colors.Green);
                VerifyDependencyPropertyRoundTrip(picker, ColorPicker.PreviousColorProperty, () => picker.PreviousColor, v => picker.PreviousColor = v, null, (Color?)Microsoft.UI.Colors.Red);
                VerifyDependencyPropertyRoundTrip(picker, ColorPicker.IsAlphaEnabledProperty, () => picker.IsAlphaEnabled, v => picker.IsAlphaEnabled = v, false, true);
                VerifyDependencyPropertyRoundTrip(picker, ColorPicker.IsColorSpectrumVisibleProperty, () => picker.IsColorSpectrumVisible, v => picker.IsColorSpectrumVisible = v, true, false);
                VerifyDependencyPropertyRoundTrip(picker, ColorPicker.IsColorPreviewVisibleProperty, () => picker.IsColorPreviewVisible, v => picker.IsColorPreviewVisible = v, true, false);
                VerifyDependencyPropertyRoundTrip(picker, ColorPicker.IsColorSliderVisibleProperty, () => picker.IsColorSliderVisible, v => picker.IsColorSliderVisible = v, true, false);
                VerifyDependencyPropertyRoundTrip(picker, ColorPicker.IsAlphaSliderVisibleProperty, () => picker.IsAlphaSliderVisible, v => picker.IsAlphaSliderVisible = v, true, false);
                VerifyDependencyPropertyRoundTrip(picker, ColorPicker.IsMoreButtonVisibleProperty, () => picker.IsMoreButtonVisible, v => picker.IsMoreButtonVisible = v, false, true);
                VerifyDependencyPropertyRoundTrip(picker, ColorPicker.IsColorChannelTextInputVisibleProperty, () => picker.IsColorChannelTextInputVisible, v => picker.IsColorChannelTextInputVisible = v, true, false);
                VerifyDependencyPropertyRoundTrip(picker, ColorPicker.IsAlphaTextInputVisibleProperty, () => picker.IsAlphaTextInputVisible, v => picker.IsAlphaTextInputVisible = v, true, false);
                VerifyDependencyPropertyRoundTrip(picker, ColorPicker.IsHexInputVisibleProperty, () => picker.IsHexInputVisible, v => picker.IsHexInputVisible = v, true, false);
                VerifyDependencyPropertyRoundTrip(picker, ColorPicker.MinHueProperty, () => picker.MinHue, v => picker.MinHue = v, 0, 10);
                VerifyDependencyPropertyRoundTrip(picker, ColorPicker.MaxHueProperty, () => picker.MaxHue, v => picker.MaxHue = v, 359, 300);
                VerifyDependencyPropertyRoundTrip(picker, ColorPicker.MinSaturationProperty, () => picker.MinSaturation, v => picker.MinSaturation = v, 0, 10);
                VerifyDependencyPropertyRoundTrip(picker, ColorPicker.MaxSaturationProperty, () => picker.MaxSaturation, v => picker.MaxSaturation = v, 100, 90);
                VerifyDependencyPropertyRoundTrip(picker, ColorPicker.MinValueProperty, () => picker.MinValue, v => picker.MinValue = v, 0, 10);
                VerifyDependencyPropertyRoundTrip(picker, ColorPicker.MaxValueProperty, () => picker.MaxValue, v => picker.MaxValue = v, 100, 90);
                VerifyDependencyPropertyRoundTrip(picker, ColorPicker.ColorSpectrumShapeProperty, () => picker.ColorSpectrumShape, v => picker.ColorSpectrumShape = v, ColorSpectrumShape.Box, ColorSpectrumShape.Ring);
                VerifyDependencyPropertyRoundTrip(picker, ColorPicker.ColorSpectrumComponentsProperty, () => picker.ColorSpectrumComponents, v => picker.ColorSpectrumComponents = v, ColorSpectrumComponents.HueSaturation, ColorSpectrumComponents.ValueHue);
                VerifyDependencyPropertyRoundTrip(picker, ColorPicker.OrientationProperty, () => picker.Orientation, v => picker.Orientation = v, Orientation.Vertical, Orientation.Horizontal);

                var slider = new ColorPickerSlider();
                VerifyDependencyPropertyRoundTrip(slider, ColorPickerSlider.ColorChannelProperty, () => slider.ColorChannel, v => slider.ColorChannel = v, ColorPickerHsvChannel.Value, ColorPickerHsvChannel.Hue);
            });
        }

        // Scenario: subscribe to ColorPicker.ColorChanged, set Color to Green, set Green again, then unsubscribe (-=) and
        //     set Red, checking the sender, the unchanged-value case and handler removal.
        // Expected: exactly one event (sender == picker, OldColor White, NewColor Green); none for the unchanged value;
        //     after -= Color becomes Red without calling the handler.
        // Failure means: ColorPicker::OnColorChanged raises ColorChanged with wrong args/sender or for an unchanged
        //     color, or the event source no longer removes handlers, so apps would get duplicate or stale notifications.
        [TestMethod]
        public void ColorPickerEventsTest()
        {
            RunOnUIThread.Execute(() =>
            {
                ColorPicker colorPicker = new ColorPicker();
                var raisedArgs = new global::System.Collections.Generic.List<ColorChangedEventArgs>();
                ColorPicker eventSender = null;

                TypedEventHandler<ColorPicker, ColorChangedEventArgs> handler = (ColorPicker sender, ColorChangedEventArgs args) =>
                {
                    eventSender = sender;
                    raisedArgs.Add(args);
                };
                colorPicker.ColorChanged += handler;

                colorPicker.Color = Microsoft.UI.Colors.Green;

                Verify.AreEqual(1, raisedArgs.Count, "Setting Color should raise ColorChanged exactly once.");
                Verify.IsTrue(ReferenceEquals(colorPicker, eventSender));
                Verify.AreEqual(Microsoft.UI.Colors.White, raisedArgs[0].OldColor);
                Verify.AreEqual(Microsoft.UI.Colors.Green, raisedArgs[0].NewColor);

                // Setting the same color again is not a change.
                colorPicker.Color = Microsoft.UI.Colors.Green;
                Verify.AreEqual(1, raisedArgs.Count, "Setting an unchanged Color should not raise ColorChanged.");

                // After unsubscribing, the handler must not be called again.
                colorPicker.ColorChanged -= handler;
                colorPicker.Color = Microsoft.UI.Colors.Red;
                Verify.AreEqual(Microsoft.UI.Colors.Red, colorPicker.Color);
                Verify.AreEqual(1, raisedArgs.Count, "A removed ColorChanged handler should not be called.");
            });
        }

        // Scenario: create a ColorSpectrum, check defaults, set Color/ranges/Shape/Components, then HsvColor; also check
        //     DP identifiers, Get/Set/ClearValue round trips, and setting/clearing Color through its DP.
        // Expected: defaults match (White, HSV (0,0,1,1), full ranges, Box, HueSaturation); Green gives HSV ~(120,1,0.5);
        //     HSV (120,1,1) gives #FF00FF00; DPs round-trip; Color Blue gives hue ~240; clearing it restores HsvColor.
        // Failure means: a ColorSpectrum DP registration or default changed, or ColorSpectrum::OnColorChanged /
        //     OnHsvColorChanged (SetColor) no longer keep Color and HsvColor in sync through the RGB <-> HSV conversion.
        [TestMethod]
        public void ColorSpectrumTest()
        {
            RunOnUIThread.Execute(() =>
            {
                ColorSpectrum colorSpectrum = new ColorSpectrum();
                Verify.IsNotNull(colorSpectrum);

                Verify.AreEqual(Microsoft.UI.Colors.White, colorSpectrum.Color);
                Verify.AreEqual(new Vector4() { X = 0.0f, Y = 0.0f, Z = 1.0f, W = 1.0f}, colorSpectrum.HsvColor);
                Verify.AreEqual(0, colorSpectrum.MinHue);
                Verify.AreEqual(359, colorSpectrum.MaxHue);
                Verify.AreEqual(0, colorSpectrum.MinSaturation);
                Verify.AreEqual(100, colorSpectrum.MaxSaturation);
                Verify.AreEqual(0, colorSpectrum.MinValue);
                Verify.AreEqual(100, colorSpectrum.MaxValue);
                Verify.AreEqual(ColorSpectrumShape.Box, colorSpectrum.Shape);
                Verify.AreEqual(ColorSpectrumComponents.HueSaturation, colorSpectrum.Components);

                colorSpectrum.Color = Microsoft.UI.Colors.Green;
                colorSpectrum.MinHue = 10;
                colorSpectrum.MaxHue = 300;
                colorSpectrum.MinSaturation = 10;
                colorSpectrum.MaxSaturation = 90;
                colorSpectrum.MinValue = 10;
                colorSpectrum.MaxValue = 90;
                colorSpectrum.Shape = ColorSpectrumShape.Ring;
                colorSpectrum.Components = ColorSpectrumComponents.HueValue;

                Verify.AreEqual(Microsoft.UI.Colors.Green, colorSpectrum.Color);

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

                Verify.AreEqual(Microsoft.UI.ColorHelper.FromArgb(255, 0, 255, 0), colorSpectrum.Color);
                Verify.AreEqual(new Vector4() { X = 120.0f, Y = 1.0f, Z = 1.0f, W = 1.0f }, colorSpectrum.HsvColor);

                VerifyDependencyPropertyIdentifiers(
                    ColorSpectrum.ColorProperty,
                    ColorSpectrum.HsvColorProperty,
                    ColorSpectrum.MinHueProperty,
                    ColorSpectrum.MaxHueProperty,
                    ColorSpectrum.MinSaturationProperty,
                    ColorSpectrum.MaxSaturationProperty,
                    ColorSpectrum.MinValueProperty,
                    ColorSpectrum.MaxValueProperty,
                    ColorSpectrum.ShapeProperty,
                    ColorSpectrum.ComponentsProperty);
                Verify.IsTrue(ReferenceEquals(ColorSpectrum.HsvColorProperty, ColorSpectrum.HsvColorProperty));

                var spectrum = new ColorSpectrum();
                VerifyDependencyPropertyRoundTrip(spectrum, ColorSpectrum.ColorProperty, () => spectrum.Color, v => spectrum.Color = v, Microsoft.UI.Colors.White, Microsoft.UI.Colors.Blue);
                VerifyDependencyPropertyRoundTrip(spectrum, ColorSpectrum.HsvColorProperty, () => spectrum.HsvColor, v => spectrum.HsvColor = v,
                    new Vector4() { X = 0.0f, Y = 0.0f, Z = 1.0f, W = 1.0f }, new Vector4() { X = 240.0f, Y = 1.0f, Z = 1.0f, W = 1.0f });
                VerifyDependencyPropertyRoundTrip(spectrum, ColorSpectrum.MinHueProperty, () => spectrum.MinHue, v => spectrum.MinHue = v, 0, 10);
                VerifyDependencyPropertyRoundTrip(spectrum, ColorSpectrum.MaxHueProperty, () => spectrum.MaxHue, v => spectrum.MaxHue = v, 359, 300);
                VerifyDependencyPropertyRoundTrip(spectrum, ColorSpectrum.MinSaturationProperty, () => spectrum.MinSaturation, v => spectrum.MinSaturation = v, 0, 10);
                VerifyDependencyPropertyRoundTrip(spectrum, ColorSpectrum.MaxSaturationProperty, () => spectrum.MaxSaturation, v => spectrum.MaxSaturation = v, 100, 90);
                VerifyDependencyPropertyRoundTrip(spectrum, ColorSpectrum.MinValueProperty, () => spectrum.MinValue, v => spectrum.MinValue = v, 0, 10);
                VerifyDependencyPropertyRoundTrip(spectrum, ColorSpectrum.MaxValueProperty, () => spectrum.MaxValue, v => spectrum.MaxValue = v, 100, 90);
                VerifyDependencyPropertyRoundTrip(spectrum, ColorSpectrum.ShapeProperty, () => spectrum.Shape, v => spectrum.Shape = v, ColorSpectrumShape.Box, ColorSpectrumShape.Ring);
                VerifyDependencyPropertyRoundTrip(spectrum, ColorSpectrum.ComponentsProperty, () => spectrum.Components, v => spectrum.Components = v, ColorSpectrumComponents.HueSaturation, ColorSpectrumComponents.SaturationValue);

                // Setting Color through its identifier keeps HsvColor in sync, and clearing it restores both defaults.
                spectrum.SetValue(ColorSpectrum.ColorProperty, Microsoft.UI.Colors.Blue);
                Verify.IsLessThan(Math.Abs(spectrum.HsvColor.X - 240.0), 0.5);
                spectrum.ClearValue(ColorSpectrum.ColorProperty);
                Verify.AreEqual(new Vector4() { X = 0.0f, Y = 0.0f, Z = 1.0f, W = 1.0f }, spectrum.HsvColor);
            });
        }

        // Scenario: subscribe to ColorSpectrum.ColorChanged, set HsvColor to pure green, unsubscribe (-=) and set
        //     HsvColor to blue; then only log (no assert) whether setting Color directly raises the event.
        // Expected: exactly one event (sender == spectrum, OldColor White, NewColor #FF00FF00), Color is #FF00FF00, and
        //     no further event after -=.
        // Failure means: ColorSpectrum::SetColor / RaiseColorChanged stopped raising ColorChanged (or sends wrong
        //     colors/sender) for HSV changes, which is the user-input path, or -= no longer removes the handler.
        [TestMethod]
        public void ColorSpectrumEventsTest()
        {
            RunOnUIThread.Execute(() =>
            {
                ColorSpectrum colorSpectrum = new ColorSpectrum();
                var raisedArgs = new global::System.Collections.Generic.List<ColorChangedEventArgs>();
                var raisedSenders = new global::System.Collections.Generic.List<ColorSpectrum>();

                TypedEventHandler<ColorSpectrum, ColorChangedEventArgs> handler = (ColorSpectrum sender, ColorChangedEventArgs args) =>
                {
                    raisedSenders.Add(sender);
                    raisedArgs.Add(args);
                };
                colorSpectrum.ColorChanged += handler;

                // A ColorSpectrum reports color changes that come through its HSV color (which is also what user input updates).
                // The default color is white.
                colorSpectrum.HsvColor = new Vector4() { X = 120.0f, Y = 1.0f, Z = 1.0f, W = 1.0f };

                Verify.AreEqual(1, raisedArgs.Count, "Changing HsvColor should raise ColorChanged exactly once.");
                Verify.IsTrue(ReferenceEquals(colorSpectrum, raisedSenders[0]));
                Verify.AreEqual(Microsoft.UI.Colors.White, raisedArgs[0].OldColor);
                Verify.AreEqual(Microsoft.UI.ColorHelper.FromArgb(255, 0, 255, 0), raisedArgs[0].NewColor);
                Verify.AreEqual(Microsoft.UI.ColorHelper.FromArgb(255, 0, 255, 0), colorSpectrum.Color);

                // After unsubscribing, the handler must not be called again.
                colorSpectrum.ColorChanged -= handler;
                colorSpectrum.HsvColor = new Vector4() { X = 240.0f, Y = 1.0f, Z = 1.0f, W = 1.0f };
                Verify.AreEqual(Microsoft.UI.ColorHelper.FromArgb(255, 0, 0, 255), colorSpectrum.Color);
                Verify.AreEqual(1, raisedArgs.Count, "A removed ColorChanged handler should not be called.");

                // Observation only (see the product-concern record): setting the Color property directly does not raise ColorChanged.
                int countAfterColorSet = 0;
                colorSpectrum.ColorChanged += (s, a) => countAfterColorSet++;
                colorSpectrum.Color = Microsoft.UI.Colors.Red;
                Log.Comment("ColorSpectrum.ColorChanged raised {0} time(s) after setting Color directly.", countAfterColorSet);
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
                catch (ArgumentException /*e*/)
                {
                    // Due to a bug in C#/WinRT (https://github.com/microsoft/CsWinRT/issues/430),
                    // we don't get a specific exception message. We should re-enable this check once that bug is fixed.
                    //Verify.IsTrue(e.Message.Contains("MinHue must be between 0 and 359."));
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

        [TestMethod]
        public void VerifyClearingHexInputFieldDoesNotCrash()
        {
            ColorPicker colorPicker = null;

            RunOnUIThread.Execute(() =>
            {
                colorPicker = new ColorPicker();
                colorPicker.IsHexInputVisible = true;

                Content = colorPicker;
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var hexTextBox = VisualTreeUtils.FindVisualChildByName(colorPicker, "HexTextBox") as TextBox;

                Verify.IsGreaterThan(hexTextBox.Text.Length, 0, "Hex TextBox should have not been empty.");

                // Clearing the hex input field should not crash the app.
                hexTextBox.Text = "";
            });
        }

        [TestMethod]
        public void VerifyClearingAlphaChannelInputFieldDoesNotCrash()
        {
            ColorPicker colorPicker = null;

            RunOnUIThread.Execute(() =>
            {
                colorPicker = new ColorPicker();
                colorPicker.IsAlphaEnabled = true;

                Content = colorPicker;
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var alphaChannelTextBox = VisualTreeUtils.FindVisualChildByName(colorPicker, "AlphaTextBox") as TextBox;

                Verify.IsGreaterThan(alphaChannelTextBox.Text.Length, 0, "Alpha channel TextBox should have not been empty.");

                // Clearing the alpha channel input field should not crash the app.
                alphaChannelTextBox.Text = "";
            });
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
        [TestProperty("Ignore", "True")] // https://github.com/microsoft/microsoft-ui-xaml/issues/3982
        public void VerifyVisualTree()
        {
            ColorPicker colorPicker = null;
            RunOnUIThread.Execute(() =>
            {
                colorPicker = new ColorPicker { IsAlphaEnabled = true, Width=300, Height=600 };
            });
            TestUtilities.SetAsVisualTreeRoot(colorPicker);

            VisualTreeTestHelper.VerifyVisualTree(root: colorPicker, verificationFileNamePrefix: "ColorPicker");
        }

        // Scenario: for each of the six ColorSpectrumComponents orderings, load a 200x200 Ring ColorSpectrum set to Red
        //     and wait (up to 30 s) for SpectrumRectangle.Fill to be assigned.
        // Expected: the Fill arrives for every ordering; Shape, Components and Color are unchanged and the
        //     SpectrumEllipse part exists.
        // Failure means: ColorSpectrum::CreateBitmapsAndColorMap / FillPixelForRing failed or hung for an ordering, so
        //     UpdateBitmapSources never set a brush and the ring spectrum would render blank.
        [TestMethod]
        public void VerifyRingSpectrumFillsForAllComponents()
        {
            foreach (var components in new[]
            {
                ColorSpectrumComponents.HueSaturation,
                ColorSpectrumComponents.SaturationHue,
                ColorSpectrumComponents.HueValue,
                ColorSpectrumComponents.ValueHue,
                ColorSpectrumComponents.SaturationValue,
                ColorSpectrumComponents.ValueSaturation,
            })
            {
                ColorSpectrum colorSpectrum = null;

                RunOnUIThread.Execute(() =>
                {
                    colorSpectrum = new ColorSpectrum
                    {
                        Width = 200,
                        Height = 200,
                        Shape = ColorSpectrumShape.Ring,
                        Components = components,
                        Color = Microsoft.UI.Colors.Red
                    };
                });

                // Exercises the ring pixel-fill path for each component ordering.
                SetAsRootAndWaitForColorSpectrumFill(colorSpectrum);

                RunOnUIThread.Execute(() =>
                {
                    Verify.AreEqual(ColorSpectrumShape.Ring, colorSpectrum.Shape);
                    Verify.AreEqual(components, colorSpectrum.Components);
                    Verify.AreEqual(Microsoft.UI.Colors.Red, colorSpectrum.Color);
                    Verify.IsNotNull(VisualTreeUtils.FindVisualChildByName(colorSpectrum, "SpectrumEllipse"));
                });
            }
        }

        // Scenario: for each of the six ColorSpectrumComponents orderings, load a 200x200 Box ColorSpectrum set to Blue
        //     and wait (up to 30 s) for SpectrumRectangle.Fill to be assigned.
        // Expected: the Fill arrives for every ordering, and Components and Color are unchanged afterwards.
        // Failure means: ColorSpectrum::CreateBitmapsAndColorMap / FillPixelForBox failed or hung for an ordering, so
        //     UpdateBitmapSources never set a brush and the box spectrum would render blank.
        [TestMethod]
        public void VerifyBoxSpectrumFillsForAllComponents()
        {
            foreach (var components in new[]
            {
                ColorSpectrumComponents.HueSaturation,
                ColorSpectrumComponents.SaturationHue,
                ColorSpectrumComponents.HueValue,
                ColorSpectrumComponents.ValueHue,
                ColorSpectrumComponents.SaturationValue,
                ColorSpectrumComponents.ValueSaturation,
            })
            {
                ColorSpectrum colorSpectrum = null;

                RunOnUIThread.Execute(() =>
                {
                    colorSpectrum = new ColorSpectrum
                    {
                        Width = 200,
                        Height = 200,
                        Shape = ColorSpectrumShape.Box,
                        Components = components,
                        Color = Microsoft.UI.Colors.Blue
                    };
                });

                SetAsRootAndWaitForColorSpectrumFill(colorSpectrum);

                RunOnUIThread.Execute(() =>
                {
                    Verify.AreEqual(components, colorSpectrum.Components);
                    Verify.AreEqual(Microsoft.UI.Colors.Blue, colorSpectrum.Color);
                });
            }
        }

        // Scenario: on a loaded Ring ColorSpectrum with (Hue, Saturation) components, set HsvColor to (240, 1, 1, 1),
        //     then switch Shape to Box.
        // Expected: Color becomes opaque pure blue (#FF0000FF) and stays blue after the shape change, with HsvColor hue
        //     still ~240.
        // Failure means: ColorSpectrum::SetColor's HSV -> RGB conversion is wrong, or OnShapeChanged /
        //     CreateBitmapsAndColorMap rewrote Color or HsvColor while rebuilding the spectrum for the new shape.
        [TestMethod]
        public void VerifyRingSpectrumColorRoundTripsThroughHsvColor()
        {
            ColorSpectrum colorSpectrum = null;

            RunOnUIThread.Execute(() =>
            {
                colorSpectrum = new ColorSpectrum
                {
                    Width = 200,
                    Height = 200,
                    Shape = ColorSpectrumShape.Ring,
                    Components = ColorSpectrumComponents.HueSaturation
                };
            });

            SetAsRootAndWaitForColorSpectrumFill(colorSpectrum);

            RunOnUIThread.Execute(() =>
            {
                colorSpectrum.HsvColor = new Vector4() { X = 240.0f, Y = 1.0f, Z = 1.0f, W = 1.0f };
                Verify.AreEqual(Microsoft.UI.ColorHelper.FromArgb(255, 0, 0, 255), colorSpectrum.Color);

                colorSpectrum.Shape = ColorSpectrumShape.Box;
                Verify.AreEqual(Microsoft.UI.ColorHelper.FromArgb(255, 0, 0, 255), colorSpectrum.Color);
                Verify.IsLessThan(Math.Abs(colorSpectrum.HsvColor.X - 240.0), 0.5);
            });
        }

        // Scenario: load a ColorPicker (alpha enabled, Color Red), get the automation peers of its ThirdDimensionSlider
        //     (value channel) and AlphaSlider, and set the third-dimension slider's Value to 50.
        // Expected: the third-dimension peer exposes a Value pattern whose string contains 100, then 50 (IsReadOnly is
        //     only logged, because SetValue is unimplemented); the alpha slider's peer returns no Value pattern.
        // Failure means: ColorPickerSliderAutomationPeer::GetPatternCore no longer separates alpha from HSV channels, or
        //     Value / GetValueString stopped reporting the slider's current value to screen readers.
        [TestMethod]
        public void VerifyColorPickerSliderValuePattern()
        {
            ColorPicker colorPicker = null;

            RunOnUIThread.Execute(() =>
            {
                colorPicker = new ColorPicker { IsAlphaEnabled = true, Color = Microsoft.UI.Colors.Red };
                Content = colorPicker;
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var thirdDimensionSlider = VisualTreeUtils.FindVisualChildByName(colorPicker, "ThirdDimensionSlider") as ColorPickerSlider;
                var alphaSlider = VisualTreeUtils.FindVisualChildByName(colorPicker, "AlphaSlider") as ColorPickerSlider;
                Verify.IsNotNull(thirdDimensionSlider);
                Verify.IsNotNull(alphaSlider);

                var peer = Microsoft.UI.Xaml.Automation.Peers.FrameworkElementAutomationPeer.FromElement(thirdDimensionSlider);
                Verify.IsNotNull(peer);

                var valueProvider = peer.GetPattern(Microsoft.UI.Xaml.Automation.Peers.PatternInterface.Value)
                    as Microsoft.UI.Xaml.Automation.Provider.IValueProvider;
                Verify.IsNotNull(valueProvider, "Non-alpha sliders expose the Value pattern.");
                // IsReadOnly is only logged: the peer reports writable while SetValue is unimplemented, and a fix may either
                // implement SetValue or report read-only, so the test shouldn't pin either answer.
                Log.Comment("ColorPickerSliderAutomationPeer IsReadOnly: {0}", valueProvider.IsReadOnly);
                Verify.IsFalse(string.IsNullOrEmpty(valueProvider.Value));
                Verify.IsTrue(valueProvider.Value.Contains("100"), "Red has value 100%.");

                // Changing the slider updates the automation value string.
                thirdDimensionSlider.Value = 50;
                Verify.IsTrue(valueProvider.Value.Contains("50"));

                // The alpha slider falls back to the base slider peer, which has no Value pattern.
                var alphaPeer = Microsoft.UI.Xaml.Automation.Peers.FrameworkElementAutomationPeer.FromElement(alphaSlider);
                Verify.IsNotNull(alphaPeer);
                Verify.IsNull(alphaPeer.GetPattern(Microsoft.UI.Xaml.Automation.Peers.PatternInterface.Value));
            });
        }

        // Scenario: load a ColorPicker with alpha enabled; for ThirdDimensionSlider and AlphaSlider, focus the slider,
        //     get the ToolTip attached to its HorizontalThumb, and set Value to 40 and then 73.
        // Expected: each thumb has a ToolTip whose string content contains 40, then contains 73 and no longer 40.
        // Failure means: ColorPickerSlider::OnApplyTemplate / OnValueChangedEvent no longer refresh the ToolTip part's
        //     content from GetToolTipString, so the value tooltip would show a stale number while the slider moves.
        [TestMethod]
        public void VerifyColorPickerSliderToolTipUpdatesWithValue()
        {
            ColorPicker colorPicker = null;

            RunOnUIThread.Execute(() =>
            {
                colorPicker = new ColorPicker { IsAlphaEnabled = true };
                Content = colorPicker;
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                foreach (var name in new[] { "ThirdDimensionSlider", "AlphaSlider" })
                {
                    var slider = VisualTreeUtils.FindVisualChildByName(colorPicker, name) as ColorPickerSlider;
                    Verify.IsNotNull(slider);
                    Verify.IsTrue(slider.Focus(FocusState.Programmatic));
                    // The ToolTip is the value of ToolTipService.ToolTip on the template's HorizontalThumb (not a visual child).
                    var thumb = VisualTreeUtils.FindVisualChildByName(slider, "HorizontalThumb");
                    Verify.IsNotNull(thumb, name + " template should contain HorizontalThumb.");
                    var toolTip = ToolTipService.GetToolTip(thumb) as ToolTip;
                    Verify.IsNotNull(toolTip, name + " thumb should have a ToolTip.");

                    slider.Value = 40;
                    var text40 = toolTip.Content as string;
                    Verify.IsFalse(string.IsNullOrEmpty(text40), name + " tooltip content should be a non-empty string.");
                    Verify.IsTrue(text40.Contains("40"), name + " tooltip should reflect value 40: " + text40);

                    slider.Value = 73;
                    var text73 = toolTip.Content as string;
                    Verify.IsTrue(text73.Contains("73"), name + " tooltip should reflect value 73: " + text73);
                    Verify.IsFalse(text73.Contains("40"), name + " tooltip should no longer reflect 40: " + text73);
                }
            });
        }

        // Scenario: set each range property of ColorPicker and ColorSpectrum to its inclusive bounds, and one past each
        //     bound.
        // Expected: bounds are accepted and read back; values outside [0, 359] (hue) or [0, 100] (saturation/value) throw
        //     ArgumentException. Every case uses a fresh instance so earlier failures can't influence it.
        // Failure means: the range checks in ColorPicker/ColorSpectrum::OnMinMaxHue/Saturation/ValueChanged were removed
        //     or are off by one, so invalid ranges are silently accepted or valid bounds are rejected.
        [TestMethod]
        public void ValidateRangePropertiesThrowForOutOfRangeValues()
        {
            RunOnUIThread.Execute(() =>
            {
                var ranges = new[]
                {
                    new { Name = "MinHue", Picker = ColorPicker.MinHueProperty, Spectrum = ColorSpectrum.MinHueProperty, Lowest = 0, Highest = 359 },
                    new { Name = "MaxHue", Picker = ColorPicker.MaxHueProperty, Spectrum = ColorSpectrum.MaxHueProperty, Lowest = 0, Highest = 359 },
                    new { Name = "MinSaturation", Picker = ColorPicker.MinSaturationProperty, Spectrum = ColorSpectrum.MinSaturationProperty, Lowest = 0, Highest = 100 },
                    new { Name = "MaxSaturation", Picker = ColorPicker.MaxSaturationProperty, Spectrum = ColorSpectrum.MaxSaturationProperty, Lowest = 0, Highest = 100 },
                    new { Name = "MinValue", Picker = ColorPicker.MinValueProperty, Spectrum = ColorSpectrum.MinValueProperty, Lowest = 0, Highest = 100 },
                    new { Name = "MaxValue", Picker = ColorPicker.MaxValueProperty, Spectrum = ColorSpectrum.MaxValueProperty, Lowest = 0, Highest = 100 },
                };

                foreach (var range in ranges)
                {
                    foreach (bool isPicker in new[] { true, false })
                    {
                        DependencyProperty property = isPicker ? range.Picker : range.Spectrum;
                        string name = (isPicker ? "ColorPicker." : "ColorSpectrum.") + range.Name;
                        Func<DependencyObject> create = () => isPicker ? (DependencyObject)new ColorPicker() : new ColorSpectrum();
                        int middle = (range.Lowest + range.Highest) / 2;

                        foreach (int bound in new[] { range.Lowest, range.Highest })
                        {
                            var element = create();
                            // Move away from the default first so that setting the bound is a real change.
                            element.SetValue(property, middle);
                            element.SetValue(property, bound);
                            Verify.AreEqual(bound, (int)element.GetValue(property), name + " should accept the inclusive bound " + bound);
                        }

                        foreach (int outOfRange in new[] { range.Lowest - 1, range.Highest + 1 })
                        {
                            var element = create();
                            // Due to a bug in C#/WinRT (https://github.com/microsoft/CsWinRT/issues/430) the message isn't checked.
                            Verify.Throws<ArgumentException>(() => element.SetValue(property, outOfRange), name + " = " + outOfRange + " should throw");
                            Log.Comment("{0} after the rejected value {1} reads {2}.", name, outOfRange, element.GetValue(property));
                        }
                    }
                }
            });
        }

        // Scenario: change ColorSpectrumComponents on a loaded ColorPicker so the third-dimension slider controls value,
        //     hue and saturation; move the slider; constrain the hue/saturation/value ranges (including min >= max).
        // Expected: slider channel, range, value, automation name and gradient follow the components and ranges; moving
        //     it changes that HSV channel of Color; min >= max clamps slider and gradient to the minimum.
        // Failure means: ColorPicker::SetThirdDimensionSliderChannel / UpdateThirdDimensionSlider (range, gradient stops)
        //     or OnThirdDimensionSliderValueChanged use the wrong HSV channel or range for the third-dimension slider.
        [TestMethod]
        public void VerifyThirdDimensionSliderFollowsSpectrumComponents()
        {
            ColorPicker colorPicker = null;

            RunOnUIThread.Execute(() =>
            {
                colorPicker = new ColorPicker { Color = Microsoft.UI.Colors.Blue };
                Content = colorPicker;
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var slider = (ColorPickerSlider)VisualTreeUtils.FindVisualChildByName(colorPicker, "ThirdDimensionSlider");
                var backgroundRectangle = (Rectangle)VisualTreeUtils.FindVisualChildByName(colorPicker, "ThirdDimensionBackgroundRectangle");
                var gradient = (LinearGradientBrush)backgroundRectangle.Fill;
                var valueProvider = (Microsoft.UI.Xaml.Automation.Provider.IValueProvider)Microsoft.UI.Xaml.Automation.Peers.FrameworkElementAutomationPeer
                    .CreatePeerForElement(slider).GetPattern(Microsoft.UI.Xaml.Automation.Peers.PatternInterface.Value);

                Log.Comment("Default (Hue, Saturation) spectrum: the slider controls value.");
                Verify.AreEqual(ColorPickerHsvChannel.Value, slider.ColorChannel);
                VerifySliderRange(slider, 0, 100, 100);
                VerifyGradientStops(gradient, (0.0, Microsoft.UI.Colors.Black), (1.0, Microsoft.UI.Colors.Blue));
                string valueSliderName = Microsoft.UI.Xaml.Automation.AutomationProperties.GetName(slider);
                Verify.IsFalse(string.IsNullOrEmpty(valueSliderName));

                Log.Comment("(Saturation, Value) spectrum: the slider controls hue and shows the full hue gradient.");
                colorPicker.ColorSpectrumComponents = ColorSpectrumComponents.SaturationValue;
                Verify.AreEqual(ColorPickerHsvChannel.Hue, slider.ColorChannel);
                VerifySliderRange(slider, 0, 359, 240);
                Verify.AreEqual(Microsoft.UI.Colors.Blue, colorPicker.Color, "Changing the components must not change the color.");
                VerifyGradientStops(gradient,
                    (0.0, Microsoft.UI.ColorHelper.FromArgb(255, 255, 0, 0)),
                    (1.0 / 6, Microsoft.UI.ColorHelper.FromArgb(255, 255, 255, 0)),
                    (2.0 / 6, Microsoft.UI.ColorHelper.FromArgb(255, 0, 255, 0)),
                    (3.0 / 6, Microsoft.UI.ColorHelper.FromArgb(255, 0, 255, 255)),
                    (4.0 / 6, Microsoft.UI.ColorHelper.FromArgb(255, 0, 0, 255)),
                    (5.0 / 6, Microsoft.UI.ColorHelper.FromArgb(255, 255, 0, 255)),
                    (1.0, Microsoft.UI.ColorHelper.FromArgb(255, 255, 0, 4)));
                string hueSliderName = Microsoft.UI.Xaml.Automation.AutomationProperties.GetName(slider);
                Verify.IsFalse(string.IsNullOrEmpty(hueSliderName));
                Verify.AreNotEqual(valueSliderName, hueSliderName);
                Verify.IsTrue(valueProvider.Value.Contains("240"), "Hue slider automation value: " + valueProvider.Value);

                Log.Comment("Moving the hue slider changes only the hue.");
                slider.Value = 200;
                Verify.AreEqual(Microsoft.UI.ColorHelper.FromArgb(255, 0, 170, 255), colorPicker.Color);
                Verify.IsTrue(valueProvider.Value.Contains("200"), "Hue slider automation value: " + valueProvider.Value);
                var toolTip = (ToolTip)ToolTipService.GetToolTip(VisualTreeUtils.FindVisualChildByName(slider, "HorizontalThumb"));
                Verify.IsTrue(((string)toolTip.Content).Contains("200"), "Hue slider tooltip: " + toolTip.Content);

                Log.Comment("A hue range of [100, 200] keeps only the pure-color stops inside the range.");
                colorPicker.MinHue = 100;
                colorPicker.MaxHue = 200;
                VerifySliderRange(slider, 100, 200, 200);
                VerifyGradientStops(gradient,
                    (0.0, Microsoft.UI.ColorHelper.FromArgb(255, 85, 255, 0)),
                    ((359.0 / 3 - 100) / 100, Microsoft.UI.ColorHelper.FromArgb(255, 0, 255, 0)),
                    ((359.0 / 2 - 100) / 100, Microsoft.UI.ColorHelper.FromArgb(255, 0, 255, 255)),
                    (1.0, Microsoft.UI.ColorHelper.FromArgb(255, 0, 170, 255)));

                Log.Comment("MaxHue == MinHue: hue is clamped to MinHue and the gradient collapses to that single hue.");
                colorPicker.MaxHue = 100;
                Verify.AreEqual(Microsoft.UI.ColorHelper.FromArgb(255, 85, 255, 0), colorPicker.Color);
                VerifyGradientStops(gradient, (0.0, Microsoft.UI.ColorHelper.FromArgb(255, 85, 255, 0)), (1.0, Microsoft.UI.ColorHelper.FromArgb(255, 85, 255, 0)));

                Log.Comment("(Hue, Value) spectrum: the slider controls saturation.");
                colorPicker.ColorSpectrumComponents = ColorSpectrumComponents.HueValue;
                Verify.AreEqual(ColorPickerHsvChannel.Saturation, slider.ColorChannel);
                VerifySliderRange(slider, 0, 100, 100);
                VerifyGradientStops(gradient, (0.0, Microsoft.UI.Colors.White), (1.0, Microsoft.UI.ColorHelper.FromArgb(255, 85, 255, 0)));
                string saturationSliderName = Microsoft.UI.Xaml.Automation.AutomationProperties.GetName(slider);
                Verify.IsFalse(string.IsNullOrEmpty(saturationSliderName));
                Verify.AreNotEqual(valueSliderName, saturationSliderName);
                Verify.AreNotEqual(hueSliderName, saturationSliderName);

                slider.Value = 50;
                Verify.AreEqual(Microsoft.UI.ColorHelper.FromArgb(255, 170, 255, 128), colorPicker.Color);

                Log.Comment("MinSaturation > MaxSaturation: saturation is clamped to MinSaturation and both stops are equal.");
                colorPicker.MinSaturation = 60;
                colorPicker.MaxSaturation = 40;
                Verify.AreEqual(60.0, slider.Value);
                VerifyEqualEndStops(gradient);

                Log.Comment("MinValue > MaxValue with a (Saturation, Hue) spectrum: value is clamped to MinValue and both stops are equal.");
                colorPicker.ColorSpectrumComponents = ColorSpectrumComponents.SaturationHue;
                Verify.AreEqual(ColorPickerHsvChannel.Value, slider.ColorChannel);
                colorPicker.MinValue = 70;
                colorPicker.MaxValue = 30;
                Verify.AreEqual(70.0, slider.Value);
                VerifyEqualEndStops(gradient);

                Log.Comment("The remaining component orderings map to the same slider channels.");
                colorPicker.ColorSpectrumComponents = ColorSpectrumComponents.ValueSaturation;
                Verify.AreEqual(ColorPickerHsvChannel.Hue, slider.ColorChannel);
                Verify.AreEqual(hueSliderName, Microsoft.UI.Xaml.Automation.AutomationProperties.GetName(slider));
                colorPicker.ColorSpectrumComponents = ColorSpectrumComponents.ValueHue;
                Verify.AreEqual(ColorPickerHsvChannel.Saturation, slider.ColorChannel);
                Verify.AreEqual(saturationSliderName, Microsoft.UI.Xaml.Automation.AutomationProperties.GetName(slider));
                colorPicker.ColorSpectrumComponents = ColorSpectrumComponents.HueSaturation;
                Verify.AreEqual(ColorPickerHsvChannel.Value, slider.ColorChannel);
                Verify.AreEqual(valueSliderName, Microsoft.UI.Xaml.Automation.AutomationProperties.GetName(slider));

                // Observation only (see the product-concern record): switching from a hue slider to a saturation slider
                // while the hue is above 100 lets the slider's new Maximum coerce its value.
                colorPicker.MinHue = 0;
                colorPicker.MaxHue = 359;
                colorPicker.MinSaturation = 0;
                colorPicker.MaxSaturation = 100;
                colorPicker.MinValue = 0;
                colorPicker.MaxValue = 100;
                colorPicker.ColorSpectrumComponents = ColorSpectrumComponents.SaturationValue;
                colorPicker.Color = Microsoft.UI.ColorHelper.FromArgb(255, 128, 128, 255);
                colorPicker.ColorSpectrumComponents = ColorSpectrumComponents.HueValue;
                Log.Comment("Color before switching (Saturation, Value) -> (Hue, Value): #FF8080FF; after: {0}.", colorPicker.Color);
            });
        }

        // Scenario: on a loaded ColorPicker with the More button, toggle it, switch RGB/HSV and type valid/invalid hex
        //     (unfocused, then in two separate focus sessions, alpha on/off); also re-template a picker with a plain Button
        //     as MoreButton.
        // Expected: More label/name and TextEntryGrid follow the toggle (or each click); the hex box name follows the
        //     model; valid hex sets Color and is kept on focus loss; invalid hex keeps Color and is replaced by the last
        //     valid value on focus loss.
        // Failure means: ColorPicker::UpdateMoreButton / OnMoreButton*, OnColorRepresentationComboBoxSelectionChanged,
        //     OnHexTextChanging validation or the OnTextBoxLostFocus rollback regressed.
        [TestMethod]
        public void VerifyTextEntryMoreButtonColorModelAndHexValidation()
        {
            ColorPicker colorPicker = null;
            ColorPicker customColorPicker = null;
            ToggleButton moreButton = null;
            TextBlock moreButtonLabel = null;
            FrameworkElement textEntryGrid = null;
            ComboBox colorModelComboBox = null;
            TextBox hexTextBox = null;
            string collapsedName = null;
            string collapsedLabel = null;
            string rgbHexName = null;

            RunOnUIThread.Execute(() =>
            {
                colorPicker = new ColorPicker
                {
                    IsMoreButtonVisible = true,
                    IsAlphaEnabled = true,
                    Color = Microsoft.UI.ColorHelper.FromArgb(255, 0x12, 0x34, 0x56)
                };

                var customTemplate = (ControlTemplate)Microsoft.UI.Xaml.Markup.XamlReader.Load(TestUtilities.ProcessTestXamlForRepo(
                    @"<ControlTemplate
                        xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
                        xmlns:controls='using:Microsoft.UI.Xaml.Controls'
                        TargetType='controls:ColorPicker'>
                        <Button x:Name='MoreButton'>
                            <TextBlock x:Name='MoreButtonLabel' />
                        </Button>
                    </ControlTemplate>"));
                customColorPicker = new ColorPicker { Template = customTemplate, IsMoreButtonVisible = true };

                var panel = new StackPanel();
                panel.Children.Add(colorPicker);
                panel.Children.Add(customColorPicker);
                Content = panel;
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                moreButton = (ToggleButton)VisualTreeUtils.FindVisualChildByName(colorPicker, "MoreButton");
                moreButtonLabel = (TextBlock)VisualTreeUtils.FindVisualChildByName(colorPicker, "MoreButtonLabel");
                textEntryGrid = (FrameworkElement)VisualTreeUtils.FindVisualChildByName(colorPicker, "TextEntryGrid");
                colorModelComboBox = (ComboBox)VisualTreeUtils.FindVisualChildByName(colorPicker, "ColorRepresentationComboBox");
                hexTextBox = (TextBox)VisualTreeUtils.FindVisualChildByName(colorPicker, "HexTextBox");

                collapsedName = Microsoft.UI.Xaml.Automation.AutomationProperties.GetName(moreButton);
                collapsedLabel = moreButtonLabel.Text;
                Verify.IsFalse(string.IsNullOrEmpty(collapsedName));
                Verify.IsFalse(string.IsNullOrEmpty(collapsedLabel));
                Verify.AreEqual(Visibility.Collapsed, textEntryGrid.Visibility);

                moreButton.IsChecked = true;
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(Visibility.Visible, textEntryGrid.Visibility);
                Verify.AreNotEqual(collapsedName, Microsoft.UI.Xaml.Automation.AutomationProperties.GetName(moreButton));
                Verify.AreNotEqual(collapsedLabel, moreButtonLabel.Text);

                moreButton.IsChecked = false;
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(Visibility.Collapsed, textEntryGrid.Visibility);
                Verify.AreEqual(collapsedName, Microsoft.UI.Xaml.Automation.AutomationProperties.GetName(moreButton));
                Verify.AreEqual(collapsedLabel, moreButtonLabel.Text);

                moreButton.IsChecked = true;
                rgbHexName = Microsoft.UI.Xaml.Automation.AutomationProperties.GetName(hexTextBox);
                Verify.IsFalse(string.IsNullOrEmpty(rgbHexName));
                colorModelComboBox.SelectedIndex = 1;
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                string hsvHexName = Microsoft.UI.Xaml.Automation.AutomationProperties.GetName(hexTextBox);
                Verify.IsFalse(string.IsNullOrEmpty(hsvHexName));
                Verify.AreNotEqual(rgbHexName, hsvHexName, "The hex box name should reflect the HSV color model.");

                colorModelComboBox.SelectedIndex = 0;
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(rgbHexName, Microsoft.UI.Xaml.Automation.AutomationProperties.GetName(hexTextBox));

                hexTextBox.Text = "#80FF0000";
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(Microsoft.UI.ColorHelper.FromArgb(0x80, 0xFF, 0x00, 0x00), colorPicker.Color);

                hexTextBox.Text = "#xyz";
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(Microsoft.UI.ColorHelper.FromArgb(0x80, 0xFF, 0x00, 0x00), colorPicker.Color, "An unparsable hex string must not change the color.");
                Verify.AreEqual("#xyz", hexTextBox.Text, "Invalid text stays in the box until it loses focus.");

                // Focus session 1: the box gets focus showing #800000FF, a valid edit applies, and focus leaves with valid text.
                colorPicker.Color = Microsoft.UI.ColorHelper.FromArgb(0x80, 0x00, 0x00, 0xFF);
                Verify.AreEqual("#800000FF", hexTextBox.Text);
                Verify.IsTrue(hexTextBox.Focus(FocusState.Programmatic), "The hex box should take focus.");
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreNotEqual(FocusState.Unfocused, hexTextBox.FocusState, "The hex box should have focus.");
                hexTextBox.Text = "#FF00FF00";
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(Microsoft.UI.ColorHelper.FromArgb(0xFF, 0x00, 0xFF, 0x00), colorPicker.Color, "A valid hex edit applies while the box has focus.");
                Verify.IsTrue(moreButton.Focus(FocusState.Programmatic), "The More button should take focus from the hex box.");
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual("#FF00FF00", hexTextBox.Text, "Valid hex text is kept when the box loses focus.");
                Verify.AreEqual(Microsoft.UI.ColorHelper.FromArgb(0xFF, 0x00, 0xFF, 0x00), colorPicker.Color, "Losing focus with valid text keeps the edited color.");

                // Focus session 2 starts from the last valid value, so the rollback target and the current color agree.
                // (Within a single session, a valid edit followed by invalid text rolls back to the text the box had when it
                // got focus on some OS versions, e.g. RS5, but keeps the valid edit on Windows 11; see the PR's product
                // concerns. This test doesn't pin either behavior.)
                Verify.IsTrue(hexTextBox.Focus(FocusState.Programmatic), "The hex box should take focus again.");
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreNotEqual(FocusState.Unfocused, hexTextBox.FocusState, "The hex box should have focus again.");
                hexTextBox.Text = "#xyz";
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(Microsoft.UI.ColorHelper.FromArgb(0xFF, 0x00, 0xFF, 0x00), colorPicker.Color, "Invalid text keeps the last valid color while the box has focus.");
                Verify.IsTrue(moreButton.Focus(FocusState.Programmatic), "The More button should take focus from the hex box.");
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                // Losing focus discards the invalid text: the box shows the last valid value again.
                Verify.AreEqual("#FF00FF00", hexTextBox.Text, "Invalid hex text should be replaced when the box loses focus.");
                Verify.AreEqual(Microsoft.UI.ColorHelper.FromArgb(0xFF, 0x00, 0xFF, 0x00), colorPicker.Color, "Losing focus with invalid text should keep the last valid color.");

                colorPicker.IsAlphaEnabled = false;
                colorPicker.Color = Microsoft.UI.ColorHelper.FromArgb(255, 0x80, 0x40, 0xC0);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                // Observation only (see the product-concern record): with alpha disabled, an unparsable hex string is not
                // reported as invalid.
                hexTextBox.Text = "#xyz";
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("IsAlphaEnabled=false: Color after the unparsable hex '#xyz' (previously #FF8040C0) is {0}.", colorPicker.Color);

                hexTextBox.Text = "#123456";
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(Microsoft.UI.ColorHelper.FromArgb(255, 0x12, 0x34, 0x56), colorPicker.Color, "A valid RGB hex string sets an opaque color when alpha is disabled.");
            });

            Log.Comment("A plain Button used as MoreButton toggles the text entry on each click.");
            Button customMoreButton = null;
            TextBlock customMoreButtonLabel = null;
            string customCollapsedLabel = null;
            string customCollapsedName = null;

            RunOnUIThread.Execute(() =>
            {
                customMoreButton = (Button)VisualTreeUtils.FindVisualChildByName(customColorPicker, "MoreButton");
                customMoreButtonLabel = (TextBlock)VisualTreeUtils.FindVisualChildByName(customColorPicker, "MoreButtonLabel");
                customCollapsedLabel = customMoreButtonLabel.Text;
                customCollapsedName = Microsoft.UI.Xaml.Automation.AutomationProperties.GetName(customMoreButton);
                Verify.AreEqual(collapsedLabel, customCollapsedLabel);
                Verify.AreEqual(collapsedName, customCollapsedName);

                InvokeButton(customMoreButton);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreNotEqual(customCollapsedLabel, customMoreButtonLabel.Text);
                Verify.AreNotEqual(customCollapsedName, Microsoft.UI.Xaml.Automation.AutomationProperties.GetName(customMoreButton));

                InvokeButton(customMoreButton);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(customCollapsedLabel, customMoreButtonLabel.Text);
                Verify.AreEqual(customCollapsedName, Microsoft.UI.Xaml.Automation.AutomationProperties.GetName(customMoreButton));
            });
        }

        // Scenario: a ColorPickerSlider (templated with a ToolTip part) and a ColorSpectrum hosted outside a ColorPicker.
        // Expected: the slider tooltip contains the value, with distinct text per channel, and its Value is empty; the
        //     spectrum peer has a default name, honors AutomationProperties.Name, has Value but not Invoke.
        // Failure means: ColorPickerSlider::GetToolTipString or ColorPickerSliderAutomationPeer::Value mishandle a slider
        //     with no parent ColorPicker, or ColorSpectrumAutomationPeer::GetNameCore / GetPatternCore regressed.
        [TestMethod]
        public void VerifyStandaloneColorPickerSliderAndColorSpectrumAutomation()
        {
            ColorPickerSlider slider = null;
            ColorSpectrum spectrum = null;

            RunOnUIThread.Execute(() =>
            {
                // ColorPickerSlider uses Slider's default style, which has no part named "ToolTip"; give it a template that does.
                slider = (ColorPickerSlider)Microsoft.UI.Xaml.Markup.XamlReader.Load(TestUtilities.ProcessTestXamlForRepo(
                    @"<primitives:ColorPickerSlider
                        xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                        xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
                        xmlns:primitives='using:Microsoft.UI.Xaml.Controls.Primitives'
                        Width='200' Minimum='0' Maximum='359'>
                        <primitives:ColorPickerSlider.Template>
                            <ControlTemplate TargetType='primitives:ColorPickerSlider'>
                                <Border x:Name='ToolTipOwner' Height='20' Background='Transparent'>
                                    <ToolTipService.ToolTip>
                                        <ToolTip x:Name='ToolTip' />
                                    </ToolTipService.ToolTip>
                                </Border>
                            </ControlTemplate>
                        </primitives:ColorPickerSlider.Template>
                    </primitives:ColorPickerSlider>"));
                spectrum = new ColorSpectrum { Width = 50, Height = 50 };

                var panel = new StackPanel();
                panel.Children.Add(slider);
                panel.Children.Add(spectrum);
                Content = panel;
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                var toolTip = ToolTipService.GetToolTip(VisualTreeUtils.FindVisualChildByName(slider, "ToolTipOwner")) as ToolTip;
                Verify.IsNotNull(toolTip, "The template's ToolTip part should be attached to its owner.");

                var toolTipTexts = new global::System.Collections.Generic.List<string>();
                foreach (var channel in new[] { ColorPickerHsvChannel.Hue, ColorPickerHsvChannel.Saturation, ColorPickerHsvChannel.Value })
                {
                    slider.ColorChannel = channel;
                    slider.Value = 0;
                    slider.Value = 40;

                    string text = toolTip.Content as string;
                    Verify.IsFalse(string.IsNullOrEmpty(text), channel + " tooltip should not be empty.");
                    Verify.IsTrue(text.Contains("40"), channel + " tooltip should contain the value: " + text);
                    Verify.IsFalse(toolTipTexts.Contains(text), channel + " tooltip should differ from the other channels: " + text);
                    toolTipTexts.Add(text);
                }

                var sliderPeer = Microsoft.UI.Xaml.Automation.Peers.FrameworkElementAutomationPeer.CreatePeerForElement(slider);
                var sliderValue = sliderPeer.GetPattern(Microsoft.UI.Xaml.Automation.Peers.PatternInterface.Value) as Microsoft.UI.Xaml.Automation.Provider.IValueProvider;
                Verify.IsNotNull(sliderValue);
                Verify.AreEqual(string.Empty, sliderValue.Value, "A slider outside a ColorPicker has no color to describe.");

                var spectrumPeer = Microsoft.UI.Xaml.Automation.Peers.FrameworkElementAutomationPeer.CreatePeerForElement(spectrum);
                string defaultName = spectrumPeer.GetName();
                Verify.IsFalse(string.IsNullOrEmpty(defaultName), "A ColorSpectrum without AutomationProperties.Name should get a default name.");

                const string customName = "Custom spectrum name";
                Microsoft.UI.Xaml.Automation.AutomationProperties.SetName(spectrum, customName);
                Verify.AreEqual(customName, spectrumPeer.GetName());
                Verify.AreNotEqual(defaultName, customName);

                Verify.IsNotNull(spectrumPeer.GetPattern(Microsoft.UI.Xaml.Automation.Peers.PatternInterface.Value));
                Verify.IsNull(spectrumPeer.GetPattern(Microsoft.UI.Xaml.Automation.Peers.PatternInterface.Invoke));
            });
        }

        // Scenario: box ColorSpectrum with (Saturation, Value): set a hue in each sextant; then switch to (Hue, Value),
        //     raise MinValue, switch to (Value, Saturation) and make all three ranges min >= max.
        // Expected: each SpectrumBrush blends the pure-hue surfaces around the hue (wrapping after purple) by the sextant
        //     fraction; (Hue, Value) blends by saturation; ranges rebuild surfaces; MinValue >= MaxValue draws MinValue.
        // Failure means: ColorSpectrum::UpdateBitmapSources picks wrong surfaces/opacity, or OnMinMax*Changed /
        //     CreateBitmapsAndColorMap skip a rebuild or mishandle min >= max, so the spectrum draws wrong colors.
        [TestMethod]
        public void VerifySpectrumBrushAcrossHueSextantsAndRangeChanges()
        {
            ColorSpectrum colorSpectrum = null;

            RunOnUIThread.Execute(() =>
            {
                colorSpectrum = new ColorSpectrum
                {
                    Width = 100,
                    Height = 100,
                    Components = ColorSpectrumComponents.SaturationValue,
                    HsvColor = new Vector4() { X = 30.0f, Y = 1.0f, Z = 1.0f, W = 1.0f }
                };
            });

            SetAsRootAndWaitForColorSpectrumFill(colorSpectrum);

            var hueBrushes = new global::System.Collections.Generic.List<SpectrumBrush>();

            RunOnUIThread.Execute(() =>
            {
                var spectrumRectangle = (Rectangle)VisualTreeUtils.FindVisualChildByName(colorSpectrum, "SpectrumRectangle");
                var hues = new[] { 30.0f, 75.0f, 150.0f, 225.0f, 270.0f, 324.0f };
                var expectedOpacities = new[] { 0.5, 0.25, 0.5, 0.75, 0.5, 0.4 };

                for (int i = 0; i < hues.Length; i++)
                {
                    colorSpectrum.HsvColor = new Vector4() { X = hues[i], Y = 1.0f, Z = 1.0f, W = 1.0f };
                    var brush = spectrumRectangle.Fill as SpectrumBrush;
                    Verify.IsNotNull(brush, "Hue " + hues[i] + " should be drawn with a SpectrumBrush.");
                    Verify.IsNotNull(brush.MinSurface);
                    Verify.IsNotNull(brush.MaxSurface);
                    Verify.IsLessThan(Math.Abs(brush.MaxSurfaceOpacity - expectedOpacities[i]), 0.001, "Hue " + hues[i] + " opacity " + brush.MaxSurfaceOpacity);
                    hueBrushes.Add(brush);
                }

                for (int i = 0; i < hueBrushes.Count; i++)
                {
                    var next = hueBrushes[(i + 1) % hueBrushes.Count];
                    Verify.IsTrue(hueBrushes[i].MaxSurface.Equals(next.MinSurface), "Sextant " + i + " should blend towards the surface sextant " + ((i + 1) % 6) + " starts from.");
                    for (int j = 0; j < i; j++)
                    {
                        Verify.IsFalse(hueBrushes[i].MinSurface.Equals(hueBrushes[j].MinSurface), "Sextants " + j + " and " + i + " should start from different surfaces.");
                    }
                }
            });

            Log.Comment("(Hue, Value): the brush blends the minimum- and maximum-saturation surfaces by the saturation.");
            WaitForSpectrumFill(colorSpectrum,
                () => colorSpectrum.Components = ColorSpectrumComponents.HueValue,
                brush => !hueBrushes.Any(hueBrush => hueBrush.MinSurface.Equals(brush.MinSurface)),
                "Switching to (Hue, Value) should draw the spectrum from new surfaces.");

            SpectrumBrush hueValueBrush = null;
            RunOnUIThread.Execute(() =>
            {
                var spectrumRectangle = (Rectangle)VisualTreeUtils.FindVisualChildByName(colorSpectrum, "SpectrumRectangle");
                Verify.IsLessThan(Math.Abs(((SpectrumBrush)spectrumRectangle.Fill).MaxSurfaceOpacity - 1.0), 0.001);

                colorSpectrum.HsvColor = new Vector4() { X = 200.0f, Y = 0.3f, Z = 1.0f, W = 1.0f };
                hueValueBrush = (SpectrumBrush)spectrumRectangle.Fill;
                Verify.IsLessThan(Math.Abs(hueValueBrush.MaxSurfaceOpacity - 0.3), 0.001);
                Verify.IsFalse(hueValueBrush.MinSurface.Equals(hueValueBrush.MaxSurface));
            });

            Log.Comment("MinValue changes the (Hue, Value) axes, so the surfaces are rebuilt.");
            WaitForSpectrumFill(colorSpectrum,
                () => colorSpectrum.MinValue = 20,
                brush => !brush.MinSurface.Equals(hueValueBrush.MinSurface),
                "Changing MinValue should rebuild the (Hue, Value) surfaces.");

            Log.Comment("Min >= max for hue, saturation and value still produces a spectrum.");
            RunOnUIThread.Execute(() =>
            {
                // Saturation is not a (Hue, Value) axis, so this does not rebuild the surfaces.
                colorSpectrum.MinSaturation = 80;
                colorSpectrum.MaxSaturation = 20;
            });

            SpectrumBrush valueSaturationBrush = null;
            WaitForSpectrumFill(colorSpectrum,
                () => colorSpectrum.Components = ColorSpectrumComponents.ValueSaturation,
                brush => Math.Abs(brush.MaxSurfaceOpacity - 200.0 / 60 % 1) < 0.001,
                "The spectrum should be rebuilt for (Value, Saturation).");

            RunOnUIThread.Execute(() =>
            {
                valueSaturationBrush = (SpectrumBrush)((Rectangle)VisualTreeUtils.FindVisualChildByName(colorSpectrum, "SpectrumRectangle")).Fill;
            });

            WaitForSpectrumFill(colorSpectrum,
                () =>
                {
                    // Hue is not a (Value, Saturation) axis; only the MaxValue change rebuilds, with all three ranges inverted.
                    colorSpectrum.MinHue = 200;
                    colorSpectrum.MaxHue = 100;
                    colorSpectrum.MaxValue = 10;
                },
                brush => !brush.MinSurface.Equals(valueSaturationBrush.MinSurface) && brush.MaxSurface != null,
                "The spectrum should be rebuilt with min >= max ranges.");

            IdleSynchronizer.Wait();

            // MinValue 20 >= MaxValue 10, so by convention every point has value 20%. Value is an axis of this spectrum, so
            // the drawn brightness (the largest channel, ~51) must not change along it. Both blended hue surfaces share
            // that value, so the cross-fade weight doesn't affect it.
            Rectangle pixelSource = null;
            RunOnUIThread.Execute(() =>
            {
                pixelSource = (Rectangle)VisualTreeUtils.FindVisualChildByName(colorSpectrum, "SpectrumRectangle");
            });

            var drawn = RenderDrawnSpectrum(pixelSource);
            foreach (double yFraction in new[] { 0.1, 0.5, 0.9 })
            {
                Color pixel = GetPixel(drawn, 0.5, yFraction);
                int brightness = Math.Max(pixel.R, Math.Max(pixel.G, pixel.B));
                Verify.IsLessThanOrEqual(Math.Abs(brightness - 51), 10, "Brightness at height " + yFraction + " should be value 20% (51): " + pixel);
            }

            // ColorSpectrum always assigns a new SpectrumBrush before it is connected, so changing MaxSurfaceOpacity on the
            // live, connected brush is the only route to the cross-fade weight update. The weight is baked into the
            // composition effect when the brush is created, so this update has no public observable; the step only
            // exercises the path and logs the drawn center pixel before and after for the record.
            RunOnUIThread.Execute(() =>
            {
                ((SpectrumBrush)pixelSource.Fill).MaxSurfaceOpacity = 0.9;
            });

            IdleSynchronizer.Wait();
            Log.Comment("Center pixel with MaxSurfaceOpacity 1/3: {0}; after setting 0.9 on the connected brush: {1}.",
                GetPixel(drawn, 0.5, 0.5), GetPixel(RenderElementPixels(pixelSource), 0.5, 0.5));
        }

        private static void VerifyDependencyPropertyIdentifiers(params DependencyProperty[] properties)
        {
            for (int i = 0; i < properties.Length; i++)
            {
                Verify.IsNotNull(properties[i], "Dependency property identifier #" + i + " should not be null");
                for (int j = 0; j < i; j++)
                {
                    Verify.IsFalse(ReferenceEquals(properties[i], properties[j]), "Dependency property identifiers #" + j + " and #" + i + " should be distinct");
                }
            }
        }

        private static void VerifyDependencyPropertyRoundTrip<T>(DependencyObject owner, DependencyProperty property, Func<T> getClrValue, Action<T> setClrValue, T defaultValue, T newValue)
        {
            Verify.AreEqual(defaultValue, getClrValue());
            Verify.AreEqual(defaultValue, (T)owner.GetValue(property));

            owner.SetValue(property, newValue);
            Verify.AreEqual(newValue, getClrValue(), "Value set through the DP should be visible through the CLR property");

            owner.ClearValue(property);
            Verify.AreEqual(defaultValue, getClrValue(), "ClearValue should restore the default");

            setClrValue(newValue);
            Verify.AreEqual(newValue, (T)owner.GetValue(property), "Value set through the CLR property should be visible through the DP");

            owner.ClearValue(property);
            Verify.AreEqual(defaultValue, getClrValue());
        }

        private static void VerifySliderRange(ColorPickerSlider slider, double minimum, double maximum, double value)
        {
            Verify.AreEqual(minimum, slider.Minimum, "Slider minimum");
            Verify.AreEqual(maximum, slider.Maximum, "Slider maximum");
            Verify.IsLessThan(Math.Abs(slider.Value - value), 0.001, "Slider value " + slider.Value);
        }

        private static void VerifyGradientStops(LinearGradientBrush brush, params (double Offset, Color Color)[] expectedStops)
        {
            Verify.AreEqual(expectedStops.Length, brush.GradientStops.Count, "Gradient stop count");
            for (int i = 0; i < expectedStops.Length; i++)
            {
                Verify.IsLessThan(Math.Abs(brush.GradientStops[i].Offset - expectedStops[i].Offset), 0.001, "Offset of stop " + i + ": " + brush.GradientStops[i].Offset);
                Verify.AreEqual(expectedStops[i].Color, brush.GradientStops[i].Color, "Color of stop " + i);
            }
        }

        private static void VerifyEqualEndStops(LinearGradientBrush brush)
        {
            Verify.AreEqual(2, brush.GradientStops.Count);
            Verify.AreEqual(0.0, brush.GradientStops[0].Offset);
            Verify.AreEqual(1.0, brush.GradientStops[1].Offset);
            Verify.AreEqual(brush.GradientStops[0].Color, brush.GradientStops[1].Color, "With min >= max both gradient stops use the minimum.");
        }

        private static void InvokeButton(Button button)
        {
            var peer = (Microsoft.UI.Xaml.Automation.Peers.ButtonAutomationPeer)Microsoft.UI.Xaml.Automation.Peers.FrameworkElementAutomationPeer.CreatePeerForElement(button);
            ((Microsoft.UI.Xaml.Automation.Provider.IInvokeProvider)peer).Invoke();
        }

        // Runs the action on the UI thread and waits until the spectrum's Fill becomes a SpectrumBrush that matches the predicate.
        private void WaitForSpectrumFill(ColorSpectrum colorSpectrum, Action action, Func<SpectrumBrush, bool> isExpectedFill, string description)
        {
            var fillChangedEvent = new ManualResetEvent(false);
            Rectangle spectrumRectangle = null;
            long callbackToken = 0;

            RunOnUIThread.Execute(() =>
            {
                spectrumRectangle = (Rectangle)VisualTreeUtils.FindVisualChildByName(colorSpectrum, "SpectrumRectangle");
                callbackToken = spectrumRectangle.RegisterPropertyChangedCallback(Shape.FillProperty, (o, dp) =>
                {
                    if (spectrumRectangle.Fill is SpectrumBrush brush && isExpectedFill(brush))
                    {
                        fillChangedEvent.Set();
                    }
                });

                action();
            });

            bool fillChanged = fillChangedEvent.WaitOne(TimeSpan.FromSeconds(30));

            RunOnUIThread.Execute(() =>
            {
                spectrumRectangle.UnregisterPropertyChangedCallback(Shape.FillProperty, callbackToken);
            });

            Verify.IsTrue(fillChanged, description);
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

            Verify.IsTrue(spectrumLoadedEvent.WaitOne(TimeSpan.FromSeconds(30)), "The color spectrum should get its Fill after loading.");
        }

        // Renders the element with RenderTargetBitmap and returns its BGRA8 pixels.
        private (int Width, int Height, byte[] Pixels) RenderElementPixels(UIElement element)
        {
            RenderTargetBitmap rtb = null;
            var rendered = new ManualResetEvent(false);
            var status = AsyncStatus.Started;

            RunOnUIThread.Execute(() =>
            {
                rtb = new RenderTargetBitmap();
                rtb.RenderAsync(element).Completed = (operation, operationStatus) =>
                {
                    status = operationStatus;
                    rendered.Set();
                };
            });

            Verify.IsTrue(rendered.WaitOne(TimeSpan.FromSeconds(10)), "RenderTargetBitmap.RenderAsync should complete.");
            Verify.AreEqual(AsyncStatus.Completed, status, "RenderTargetBitmap.RenderAsync status.");

            int width = 0;
            int height = 0;
            byte[] pixels = null;
            rendered.Reset();

            RunOnUIThread.Execute(() =>
            {
                width = rtb.PixelWidth;
                height = rtb.PixelHeight;
                rtb.GetPixelsAsync().Completed = (operation, operationStatus) =>
                {
                    if (operationStatus == AsyncStatus.Completed)
                    {
                        pixels = operation.GetResults().ToArray();
                    }
                    rendered.Set();
                };
            });

            Verify.IsTrue(rendered.WaitOne(TimeSpan.FromSeconds(10)), "RenderTargetBitmap.GetPixelsAsync should complete.");
            Verify.IsNotNull(pixels, "RenderTargetBitmap.GetPixelsAsync should return pixels.");
            Verify.IsTrue(width > 0 && height > 0 && pixels.Length == width * height * 4, "Rendered size " + width + "x" + height);
            return (width, height, pixels);
        }

        // The spectrum's surfaces are decoded asynchronously; render it once they are decoded and its center is drawn opaque.
        private (int Width, int Height, byte[] Pixels) RenderDrawnSpectrum(Rectangle spectrumRectangle)
        {
            for (int attempt = 0; attempt < 20; attempt++)
            {
                bool decoded = false;
                RunOnUIThread.Execute(() =>
                {
                    var brush = (SpectrumBrush)spectrumRectangle.Fill;
                    decoded = brush.MinSurface.DecodedPhysicalSize.Width > 0 && brush.MaxSurface.DecodedPhysicalSize.Width > 0;
                });

                if (decoded)
                {
                    var drawn = RenderElementPixels(spectrumRectangle);
                    if (GetPixel(drawn, 0.5, 0.5).A == 255)
                    {
                        return drawn;
                    }
                }

                Thread.Sleep(250);
                IdleSynchronizer.Wait();
            }

            Verify.Fail("The spectrum's surfaces were not decoded and drawn within 5 seconds.");
            return (0, 0, null);
        }

        // Returns the pixel at the given fractions of the bitmap's width and height.
        private static Color GetPixel((int Width, int Height, byte[] Pixels) bitmap, double xFraction, double yFraction)
        {
            int x = (int)Math.Round(xFraction * (bitmap.Width - 1));
            int y = (int)Math.Round(yFraction * (bitmap.Height - 1));
            int offset = 4 * (bitmap.Width * y + x);
            return Microsoft.UI.ColorHelper.FromArgb(bitmap.Pixels[offset + 3], bitmap.Pixels[offset + 2], bitmap.Pixels[offset + 1], bitmap.Pixels[offset]);
        }
    }
}