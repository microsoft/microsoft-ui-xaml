// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;

using MUXControlsTestApp.Utilities;

using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.AnimatedVisuals;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Media.Imaging;
using Microsoft.UI.Xaml.XamlTypeInfo;
using Windows.UI.Text;
using Windows.Foundation.Metadata;
using Common;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    [TestClass]
    public class IconSourceApiTests : ApiTestBase
    {
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

                iconSource.Foreground = new SolidColorBrush(Microsoft.UI.Colors.Red);
                iconSource.ImageSource = new SvgImageSource(uri);
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(iconSource.Foreground is SolidColorBrush);
                Verify.IsTrue(imageIcon.Foreground is SolidColorBrush);
                Verify.AreEqual(Microsoft.UI.Colors.Red, (iconSource.Foreground as SolidColorBrush).Color);
                Verify.AreEqual(Microsoft.UI.Colors.Red, (imageIcon.Foreground as SolidColorBrush).Color);
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

                iconSource.Foreground = new SolidColorBrush(Microsoft.UI.Colors.Red);
                iconSource.Source = source;
                iconSource.MirroredWhenRightToLeft = true;
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(iconSource.Foreground is SolidColorBrush);
                Verify.IsTrue(animatedIcon.Foreground is SolidColorBrush);
                Verify.AreEqual(Microsoft.UI.Colors.Red, (iconSource.Foreground as SolidColorBrush).Color);
                Verify.AreEqual(Microsoft.UI.Colors.Red, (animatedIcon.Foreground as SolidColorBrush).Color);
                Verify.AreEqual(source, iconSource.Source);
                Verify.AreEqual(source, animatedIcon.Source);
                Verify.IsTrue(iconSource.MirroredWhenRightToLeft);
                Verify.IsTrue(animatedIcon.MirroredWhenRightToLeft);
            });
        }

        [TestMethod]
        public void CreateIconElementReturnsCorrectTypeTest()
        {
            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Verify BitmapIconSource creates BitmapIcon");
                var bitmapIconSource = new BitmapIconSource();
                var bitmapIcon = bitmapIconSource.CreateIconElement();
                Verify.IsNotNull(bitmapIcon);
                Verify.IsTrue(bitmapIcon is BitmapIcon);

                Log.Comment("Verify FontIconSource creates FontIcon");
                var fontIconSource = new FontIconSource();
                var fontIcon = fontIconSource.CreateIconElement();
                Verify.IsNotNull(fontIcon);
                Verify.IsTrue(fontIcon is FontIcon);

                Log.Comment("Verify SymbolIconSource creates SymbolIcon");
                var symbolIconSource = new SymbolIconSource();
                var symbolIcon = symbolIconSource.CreateIconElement();
                Verify.IsNotNull(symbolIcon);
                Verify.IsTrue(symbolIcon is SymbolIcon);

                Log.Comment("Verify PathIconSource creates PathIcon");
                var pathIconSource = new PathIconSource();
                var pathIcon = pathIconSource.CreateIconElement();
                Verify.IsNotNull(pathIcon);
                Verify.IsTrue(pathIcon is PathIcon);
            });
        }

        [TestMethod]
        public void CreateIconElementForegroundTest()
        {
            FontIconSource iconSource1 = null;
            FontIconSource iconSource2 = null;
            FontIcon icon1 = null;
            FontIcon icon2 = null;

            RunOnUIThread.Execute(() =>
            {
                iconSource1 = new FontIconSource()
                {
                    Foreground = new SolidColorBrush(Microsoft.UI.Colors.Blue)
                };

                iconSource2 = new FontIconSource();
                
                Log.Comment("Create first icon element with foreground already set");
                icon1 = iconSource1.CreateIconElement() as FontIcon;
                Verify.IsNotNull(icon1);
                
                Log.Comment("Create second icon element with foreground not set");
                icon2 = iconSource2.CreateIconElement() as FontIcon;
                Verify.IsNotNull(icon2);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Verify foreground is applied to both icon elements");
                Verify.IsTrue(icon1.Foreground is SolidColorBrush);
                Verify.IsTrue(icon2.Foreground is SolidColorBrush);
                Verify.AreEqual(Microsoft.UI.Colors.Blue, (icon1.Foreground as SolidColorBrush).Color);
            });
        }

        [TestMethod]
        public void PropertyChangePropagationToCreatedElements()
        {
            FontIconSource iconSource = null;
            FontIcon icon = null;

            RunOnUIThread.Execute(() =>
            {
                iconSource = new FontIconSource();
                
                Log.Comment("Create icon element before setting properties");
                icon = iconSource.CreateIconElement() as FontIcon;
                
                Verify.IsNotNull(icon);
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Verify foreground is not null before setting");
                Verify.IsNotNull(icon.Foreground);
                
                Log.Comment("Change foreground on IconSource");
                iconSource.Foreground = new SolidColorBrush(Microsoft.UI.Colors.Red);
            });
            
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Verify foreground propagates to created element");
                Verify.IsTrue(icon.Foreground is SolidColorBrush);
                Verify.AreEqual(Microsoft.UI.Colors.Red, (icon.Foreground as SolidColorBrush).Color);
            });
        }

        [TestMethod]
        public void CreateIconElementPreservesIconSourceProperties()
        {
            FontIconSource fontIconSource = null;
            FontIcon fontIcon = null;

            RunOnUIThread.Execute(() =>
            {
                fontIconSource = new FontIconSource();
                fontIconSource.Glyph = "\uE001";
                fontIconSource.FontSize = 24;
                fontIconSource.FontFamily = new Microsoft.UI.Xaml.Media.FontFamily("Segoe UI Symbol");
                fontIconSource.Foreground = new SolidColorBrush(Microsoft.UI.Colors.Purple);
                
                Log.Comment("Create icon element from configured source");
                fontIcon = fontIconSource.CreateIconElement() as FontIcon;
                Verify.IsNotNull(fontIcon);
            });
            
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Log.Comment("Verify all properties are transferred to the icon element");
                Verify.AreEqual("\uE001", fontIcon.Glyph);
                Verify.AreEqual(24.0, fontIcon.FontSize);
                Verify.AreEqual("Segoe UI Symbol", fontIcon.FontFamily.Source);
                Verify.AreEqual(Microsoft.UI.Colors.Purple, (fontIcon.Foreground as SolidColorBrush).Color);
            });
        }
    }
}
