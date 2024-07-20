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
    }
}
