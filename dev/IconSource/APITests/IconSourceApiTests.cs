﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;

using MUXControlsTestApp.Utilities;

using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Media;
using Windows.UI.Text;
using Windows.Foundation.Metadata;
using Common;

#if USING_TAEF
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;
#else
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.VisualStudio.TestTools.UnitTesting.Logging;
#endif

using SymbolIconSource = Microsoft.UI.Xaml.Controls.SymbolIconSource;
using FontIconSource = Microsoft.UI.Xaml.Controls.FontIconSource;
using BitmapIconSource = Microsoft.UI.Xaml.Controls.BitmapIconSource;
using ImageIconSource = Microsoft.UI.Xaml.Controls.ImageIconSource;
using PathIconSource = Microsoft.UI.Xaml.Controls.PathIconSource;
using XamlControlsXamlMetaDataProvider = Microsoft.UI.Xaml.XamlTypeInfo.XamlControlsXamlMetaDataProvider;
using Microsoft.UI.Xaml.Controls;
using Windows.UI.Xaml.Media.Imaging;
using Microsoft.UI.Xaml.Controls.AnimatedVisuals;

namespace Windows.UI.Xaml.Tests.MUXControls.ApiTests
{
    [TestClass]
    public class IconSourceApiTests : ApiTestBase
    {
        [TestMethod]
        public void SymbolIconSourceTest()
        {
            SymbolIconSource iconSource = null;
            SymbolIcon symbolIcon = null;

            RunOnUIThread.Execute(() =>
            {
                iconSource = new SymbolIconSource();
                symbolIcon = iconSource.CreateIconElement() as SymbolIcon;

                // IconSource.Foreground should be null to allow foreground inheritance from
                // the parent to work.
                Verify.AreEqual(iconSource.Foreground, null);
                //Verify.AreEqual(symbolIcon.Foreground, null);

                Log.Comment("Validate the defaults match SymbolIcon.");

                var icon = new SymbolIcon();
                Verify.AreEqual(icon.Symbol, iconSource.Symbol);
                Verify.AreEqual(symbolIcon.Symbol, iconSource.Symbol);

                Log.Comment("Validate that you can change the properties.");

                iconSource.Foreground = new SolidColorBrush(Windows.UI.Colors.Red);
                iconSource.Symbol = Symbol.HangUp;
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(iconSource.Foreground is SolidColorBrush);
                Verify.AreEqual(Windows.UI.Colors.Red, (iconSource.Foreground as SolidColorBrush).Color);
                Verify.AreEqual(Windows.UI.Colors.Red, (symbolIcon.Foreground as SolidColorBrush).Color);
                Verify.AreEqual(Symbol.HangUp, iconSource.Symbol);
                Verify.AreEqual(Symbol.HangUp, symbolIcon.Symbol);
            });
        }

        [TestMethod]
        public void FontIconSourceTest()
        {
            FontIconSource iconSource = null;
            FontIcon fontIcon = null;

            RunOnUIThread.Execute(() =>
            {
                iconSource = new FontIconSource();
                fontIcon = iconSource.CreateIconElement() as FontIcon;

                // IconSource.Foreground should be null to allow foreground inheritance from
                // the parent to work.
                Verify.AreEqual(iconSource.Foreground, null);
                //Verify.AreEqual(fontIcon.Foreground, null);

                Log.Comment("Validate the defaults match FontIcon.");

                var icon = new FontIcon();
                Verify.AreEqual(icon.Glyph, iconSource.Glyph);
                Verify.AreEqual(fontIcon.Glyph, iconSource.Glyph);
                Verify.AreEqual(icon.FontSize, iconSource.FontSize);
                Verify.AreEqual(fontIcon.FontSize, iconSource.FontSize);
                Verify.AreEqual(icon.FontStyle, iconSource.FontStyle);
                Verify.AreEqual(fontIcon.FontStyle, iconSource.FontStyle);
                Verify.AreEqual(icon.FontWeight.Weight, iconSource.FontWeight.Weight);
                Verify.AreEqual(fontIcon.FontWeight.Weight, iconSource.FontWeight.Weight);
                Verify.AreEqual(icon.FontFamily.Source, iconSource.FontFamily.Source);
                Verify.AreEqual(fontIcon.FontFamily.Source, iconSource.FontFamily.Source);
                Verify.AreEqual(icon.IsTextScaleFactorEnabled, iconSource.IsTextScaleFactorEnabled);
                Verify.AreEqual(fontIcon.IsTextScaleFactorEnabled, iconSource.IsTextScaleFactorEnabled);
                Verify.AreEqual(icon.MirroredWhenRightToLeft, iconSource.MirroredWhenRightToLeft);
                Verify.AreEqual(fontIcon.MirroredWhenRightToLeft, iconSource.MirroredWhenRightToLeft);

                Log.Comment("Validate that you can change the properties.");

                iconSource.Foreground = new SolidColorBrush(Windows.UI.Colors.Red);
                iconSource.Glyph = "&#xE114;";
                iconSource.FontSize = 25;
                iconSource.FontStyle = FontStyle.Oblique;
                iconSource.FontWeight = new FontWeight() { Weight = 250 };
                iconSource.FontFamily = new FontFamily("Segoe UI Symbol");
                iconSource.IsTextScaleFactorEnabled = true;
                iconSource.MirroredWhenRightToLeft = true;
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(iconSource.Foreground is SolidColorBrush);
                Verify.IsTrue(fontIcon.Foreground is SolidColorBrush);
                Verify.AreEqual(Windows.UI.Colors.Red, (iconSource.Foreground as SolidColorBrush).Color);
                Verify.AreEqual(Windows.UI.Colors.Red, (fontIcon.Foreground as SolidColorBrush).Color);
                Verify.AreEqual("&#xE114;", iconSource.Glyph);
                Verify.AreEqual("&#xE114;", fontIcon.Glyph);
                Verify.AreEqual(25, iconSource.FontSize);
                Verify.AreEqual(25, fontIcon.FontSize);
                Verify.AreEqual(FontStyle.Oblique, iconSource.FontStyle);
                Verify.AreEqual(FontStyle.Oblique, fontIcon.FontStyle);
                Verify.AreEqual(250, iconSource.FontWeight.Weight);
                Verify.AreEqual(250, fontIcon.FontWeight.Weight);
                Verify.AreEqual("Segoe UI Symbol", iconSource.FontFamily.Source);
                Verify.AreEqual("Segoe UI Symbol", fontIcon.FontFamily.Source);
                Verify.AreEqual(true, iconSource.IsTextScaleFactorEnabled);
                Verify.AreEqual(true, fontIcon.IsTextScaleFactorEnabled);
                Verify.AreEqual(true, iconSource.MirroredWhenRightToLeft);
                Verify.AreEqual(true, fontIcon.MirroredWhenRightToLeft);
            });
        }

        [TestMethod]
        public void BitmapIconSourceTest()
        {
            BitmapIconSource iconSource = null;
            BitmapIcon bitmapIcon = null;
            var uri = new Uri("ms-appx:///Assets/ingredient1.png");

            RunOnUIThread.Execute(() =>
            {
                iconSource = new BitmapIconSource();
                bitmapIcon = iconSource.CreateIconElement() as BitmapIcon;

                // IconSource.Foreground should be null to allow foreground inheritance from
                // the parent to work.
                Verify.AreEqual(iconSource.Foreground, null);
                //Verify.AreEqual(bitmapIcon.Foreground, null);

                Log.Comment("Validate the defaults match BitmapIcon.");

                var icon = new BitmapIcon();
                Verify.AreEqual(icon.UriSource, iconSource.UriSource);
                Verify.AreEqual(bitmapIcon.UriSource, iconSource.UriSource);

                if (ApiInformation.IsPropertyPresent("Windows.UI.Xaml.Controls.BitmapIcon", "ShowAsMonochrome"))
                {
                    Verify.AreEqual(icon.ShowAsMonochrome, iconSource.ShowAsMonochrome);
                    Verify.AreEqual(bitmapIcon.ShowAsMonochrome, iconSource.ShowAsMonochrome);
                }

                Log.Comment("Validate that you can change the properties.");

                iconSource.Foreground = new SolidColorBrush(Windows.UI.Colors.Red);
                iconSource.UriSource = uri;
                iconSource.ShowAsMonochrome = false;
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(iconSource.Foreground is SolidColorBrush);
                Verify.IsTrue(bitmapIcon.Foreground is SolidColorBrush);
                Verify.AreEqual(Windows.UI.Colors.Red, (iconSource.Foreground as SolidColorBrush).Color);
                Verify.AreEqual(Windows.UI.Colors.Red, (bitmapIcon.Foreground as SolidColorBrush).Color);
                Verify.AreEqual(uri, iconSource.UriSource);
                Verify.AreEqual(uri, bitmapIcon.UriSource);
                Verify.AreEqual(false, iconSource.ShowAsMonochrome);
                Verify.AreEqual(false, bitmapIcon.ShowAsMonochrome);
            });
        }

        [TestMethod]
        public void ImageIconSourceTest()
        {
            ImageIconSource iconSource = null;
            ImageIcon imageIcon = null;
            var uri = new Uri("ms-appx:///Assets/Nuclear_symbol.svg");

            RunOnUIThread.Execute(() =>
            {
                iconSource = new ImageIconSource();
                imageIcon = iconSource.CreateIconElement() as ImageIcon;

                // IconSource.Foreground should be null to allow foreground inheritance from
                // the parent to work.
                Verify.AreEqual(iconSource.Foreground, null);
                //Verify.AreEqual(imageIcon.Foreground, null);

                Log.Comment("Validate the defaults match BitmapIcon.");

                var icon = new ImageIcon();
                Verify.AreEqual(icon.Source, iconSource.ImageSource);
                Verify.AreEqual(imageIcon.Source, iconSource.ImageSource);

                Log.Comment("Validate that you can change the properties.");

                iconSource.Foreground = new SolidColorBrush(Windows.UI.Colors.Red);
                iconSource.ImageSource = new SvgImageSource(uri);
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(iconSource.Foreground is SolidColorBrush);
                Verify.IsTrue(imageIcon.Foreground is SolidColorBrush);
                Verify.AreEqual(Windows.UI.Colors.Red, (iconSource.Foreground as SolidColorBrush).Color);
                Verify.AreEqual(Windows.UI.Colors.Red, (imageIcon.Foreground as SolidColorBrush).Color);
                Verify.AreEqual(uri, ((SvgImageSource)iconSource.ImageSource).UriSource);
                Verify.AreEqual(uri, ((SvgImageSource)imageIcon.Source).UriSource);
            });
        }

        [TestMethod]
        public void AnimatedIconSourceTest()
        {
            AnimatedIconSource iconSource = null;
            IAnimatedVisualSource2 source = null;
            AnimatedIcon animatedIcon = null;

            RunOnUIThread.Execute(() =>
            {
                iconSource = new AnimatedIconSource();
                source = new AnimatedChevronDownSmallVisualSource();
                animatedIcon = iconSource.CreateIconElement() as AnimatedIcon;

                // IconSource.Foreground should be null to allow foreground inheritance from
                // the parent to work.
                Verify.AreEqual(iconSource.Foreground, null);
                //Verify.AreEqual(animatedIcon.Foreground, null);
                Verify.AreEqual(iconSource.MirroredWhenRightToLeft, false);
                Verify.AreEqual(animatedIcon.MirroredWhenRightToLeft, false);

                Log.Comment("Validate the defaults match BitmapIcon.");

                var icon = new AnimatedIcon();
                Verify.AreEqual(icon.Source, iconSource.Source);
                Verify.AreEqual(animatedIcon.Source, iconSource.Source);
                Verify.AreEqual(icon.MirroredWhenRightToLeft, iconSource.MirroredWhenRightToLeft);

                Log.Comment("Validate that you can change the properties.");

                iconSource.Foreground = new SolidColorBrush(Windows.UI.Colors.Red);
                iconSource.Source = source;
                iconSource.MirroredWhenRightToLeft = true;
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(iconSource.Foreground is SolidColorBrush);
                Verify.IsTrue(animatedIcon.Foreground is SolidColorBrush);
                Verify.AreEqual(Windows.UI.Colors.Red, (iconSource.Foreground as SolidColorBrush).Color);
                Verify.AreEqual(Windows.UI.Colors.Red, (animatedIcon.Foreground as SolidColorBrush).Color);
                Verify.AreEqual(source, iconSource.Source);
                Verify.AreEqual(source, animatedIcon.Source);
                Verify.IsTrue(iconSource.MirroredWhenRightToLeft);
                Verify.IsTrue(animatedIcon.MirroredWhenRightToLeft);
            });
        }

        [TestMethod]
        public void PathIconSourceTest()
        {
            PathIconSource iconSource = null;
            RectangleGeometry rectGeometry = null;
            PathIcon pathIcon = null;

            RunOnUIThread.Execute(() =>
            {
                iconSource = new PathIconSource();
                pathIcon = iconSource.CreateIconElement() as PathIcon;

                // IconSource.Foreground should be null to allow foreground inheritance from
                // the parent to work.
                Verify.AreEqual(iconSource.Foreground, null);
                //Verify.AreEqual(pathIcon.Foreground, null);

                Log.Comment("Validate the defaults match PathIcon.");

                var icon = new PathIcon();
                Verify.AreEqual(icon.Data, iconSource.Data);
                Verify.AreEqual(pathIcon.Data, iconSource.Data);

                Log.Comment("Validate that you can change the properties.");

                iconSource.Foreground = new SolidColorBrush(Windows.UI.Colors.Red);
                iconSource.Data = rectGeometry = new RectangleGeometry();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(iconSource.Foreground is SolidColorBrush);
                Verify.IsTrue(pathIcon.Foreground is SolidColorBrush);
                Verify.AreEqual(Windows.UI.Colors.Red, (iconSource.Foreground as SolidColorBrush).Color);
                Verify.AreEqual(Windows.UI.Colors.Red, (pathIcon.Foreground as SolidColorBrush).Color);
                Verify.AreEqual(rectGeometry, iconSource.Data);
                Verify.AreEqual(rectGeometry, pathIcon.Data);
            });
        }

        // XamlControlsXamlMetaDataProvider does not exist in the OS repo,
        // so we can't execute this test as authored there.
        [TestMethod]
        public void VerifyFontWeightPropertyMetadata()
        {
            RunOnUIThread.Execute(() =>
            {
                XamlControlsXamlMetaDataProvider provider = new XamlControlsXamlMetaDataProvider();
                var FontIconSourceType = provider.GetXamlType(typeof(FontIconSource).FullName);
                var fontWeightMember = FontIconSourceType.GetMember("FontWeight");
                Verify.IsNotNull(fontWeightMember);
                var memberType = fontWeightMember.Type;
                Verify.AreEqual(memberType.BaseType.FullName, "ValueType");
            });
        }
    }
}
