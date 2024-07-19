﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using MUXControlsTestApp;
using MUXControlsTestApp.Utilities;
using System;
using System.Linq;
using Windows.UI;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Shapes;
using Microsoft.UI.Xaml.Media;
using Windows.Foundation.Metadata;
using System.Collections.Generic;
using System.Text.RegularExpressions;
using Common;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    [TestClass]
    public class AcrylicBrushTests : ApiTestBase
    {
        private Rectangle _rectangle1 = null;
        private StackPanel _rootSP = null;
        private static string pattern = @"SystemControl(.*)(AcrylicWindow|AcrylicElement).*Brush";
        private static Regex rgx = new Regex(pattern, RegexOptions.IgnoreCase);
        private Color unknownColor = Microsoft.UI.ColorHelper.FromArgb(111, 112, 113, 114);

        [TestMethod]
        /// SystemControl <fallback color> Acrylic <background source> <tint color and tint opacity> Brush
        public void VerifyAcrylicBrushNameRule()
        {
            foreach (var brushName in GetAcrylicBrushNames())
            {
                Verify.IsTrue(rgx.Match(brushName).Success, brushName + " match the pattern");
            }
        }

        /// <summary>
        /// SystemControl <fallback color> Acrylic <background source> <tint color and tint opacity> Brush
        /// In practice, we’ll have:
        /// SystemControlAcrylicWindowBrush
        /// SystemControlAcrylicElementMediumHighBrush
        /// SystemControlAccentDark1AcrylicWindowAccentDark1Brush
        /// SystemControlBaseHighAcrylicElementBrush
        /// SystemControlBaseHighAcrylicWindowMediumHighBrush
        /// 
        /// deduce color by name: brush name -> <fallback color> -> System{<fallback color>}Color -> Color
        /// get color by function: brush name -> FallbackColor -> Color
        /// then compare the two Microsoft.UI.Colors.
        /// </summary>
        [TestMethod]
        public void VerifyAcrylicBrushHasCorrectFallbackColor()
        {
            if (PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone2))
            {
                Log.Comment("Using system XAML AcrylicBrushes post-RS2, so nothing to test.");
                return;
            }

            RunOnUIThread.Execute(() =>
            {
                // only select the pair whose <fallback color> is not empty
                var brushAndFallbackColorNamePairs = GetAcrylicBrushNames()
                    .Select(brushName => new KeyValuePair<string, string>(brushName, GetFallBackColorName(brushName)))
                    .Where(pair => !String.IsNullOrEmpty(pair.Value));

                var lightTheme = Application.Current.Resources.MergedDictionaries.First().ThemeDictionaries["Light"] as ResourceDictionary;
                var darkTheme = Application.Current.Resources.MergedDictionaries.First().ThemeDictionaries["Default"] as ResourceDictionary;

                Log.Comment("Verify Dark");
                VerifyAcrylicBrushHasCorrectFallbackColorOnTheme(brushAndFallbackColorNamePairs, darkTheme);
                Log.Comment("Verify Light");
                VerifyAcrylicBrushHasCorrectFallbackColorOnTheme(brushAndFallbackColorNamePairs, lightTheme);
            }
            );
        }

        private void VerifyAcrylicBrushHasCorrectFallbackColorOnTheme(IEnumerable<KeyValuePair<string, string>> brushAndFallbackColorNamePairs, ResourceDictionary theme)
        {
            foreach (var namePair in brushAndFallbackColorNamePairs)
            {
                Color fallbackColor = GetAcrylicBrushFallbackColor(namePair.Key, theme); ;
                Color fallbackExpectedColor = GetColorByName(namePair.Value, theme);
                Verify.AreEqual(fallbackColor, fallbackExpectedColor,
                    string.Format("Fallback Color for AcrylicBrush {0} and Color {1} are the same", namePair.Key, namePair.Value));
            };
        }

        /// <summary>
        /// Verifies the AcrylicBrush default properties.
        /// </summary>
        [TestMethod]
        public void VerifyDefaultPropertyValues()
        {
            RunOnUIThread.Execute(() =>
            {
                AcrylicBrush acrylicBrush = new AcrylicBrush();
                Verify.IsNotNull(acrylicBrush);

                MatrixTransform identityTransform = new MatrixTransform();
                identityTransform.Matrix = Matrix.Identity;

                Log.Comment("Verifying AcrylicBrush default property values");
                Verify.AreEqual(acrylicBrush.FallbackColor, Microsoft.UI.ColorHelper.FromArgb(0, 255, 255, 255));
                Verify.AreEqual(acrylicBrush.AlwaysUseFallback, false);
                Verify.AreEqual(acrylicBrush.TintColor, Microsoft.UI.ColorHelper.FromArgb(204, 255, 255, 255));
                Verify.AreEqual(acrylicBrush.TintOpacity, 1.0);
                Verify.AreEqual(acrylicBrush.Opacity, 1.0);
            });
        }

        private void SetupDefaultUI()
        {
            _rectangle1 = new Rectangle();
            _rectangle1.Width = 200;
            _rectangle1.Height = 100;
            _rectangle1.Fill = new SolidColorBrush(Microsoft.UI.Colors.Purple);

            _rootSP = new StackPanel();
            _rootSP.Children.Add(_rectangle1);

            Log.Comment("Setting window content");
            Content = _rootSP;
        }

        private Color GetColorByName(string name, ResourceDictionary dict)
        {
            object color;
            if (dict.TryGetValue(name, out color))
                return (Color)color;
            return unknownColor;
        }

        private Color GetSolidColorBrushColor(string brushName, ResourceDictionary dict)
        {
            Color color = unknownColor;
            var brush = dict[brushName] as SolidColorBrush;
            if (brush != null)
                color = brush.Color;

            return color;
        }

        private Color GetAcrylicBrushFallbackColor(string brushName, ResourceDictionary dict)
        {
            Color color = unknownColor;

            var brush = dict[brushName] as AcrylicBrush;
            if (brush != null)
                color = brush.FallbackColor;

            return color;
        }
        private IEnumerable<string> GetAcrylicBrushNames()
        {
            return AcrylicColorPage.AcrylicBrushNames;
        }

        /// <summary>
        /// brush name is: SystemControl <fallback color> Acrylic <background source>
        /// <fallback color> should be part of FallbackColor
        /// eg: SystemControlBaseHighAcrylicElementBrush <-> SystemBaseHighColor
        /// BaseHigh is <fallback color> and SystemBaseHighColor is the expected fallback Color name
        /// </summary>
        private string GetFallBackColorName(String brushName)
        {
            string name = "";
            Match m = rgx.Match(brushName);

            if (m.Success)
                name = m.Groups[1].Value;

            if (!String.IsNullOrEmpty(name))
            {
                switch (name)
                {
                    case "AccentDark1": name = "SystemAccentColorDark1"; break;
                    case "AccentDark2": name = "SystemAccentColorDark2"; break;
                    case "AccentDark3": name = "SystemAccentColorDark3"; break;
                    case "Accent": name = "SystemAccentColor"; break;
                    case "AccentLight1": name = "SystemAccentColorLight1"; break;
                    case "AccentLight2": name = "SystemAccentColorLight2"; break;
                    case "AccentLight3": name = "SystemAccentColorLight3"; break;
                    default:
                        name = string.Format("System{0}Color", name); break;
                }
            }

            return name;
        }

    }
}