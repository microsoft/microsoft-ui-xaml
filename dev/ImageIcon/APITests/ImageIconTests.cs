// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using Common;
using System.Collections.Generic;
using Microsoft.UI.Xaml.Controls;
using MUXControlsTestApp.Utilities;
using Windows.UI.Xaml.Media.Imaging;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Controls;

#if USING_TAEF
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;
#else
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.VisualStudio.TestTools.UnitTesting.Logging;
#endif

namespace Windows.UI.Xaml.Tests.MUXControls.ApiTests
{

    [TestClass]
    public class ImageIconTests : ApiTestBase
    {
        [TestMethod]
        public void ImageIconTest()
        {
            ImageIcon imageIcon = null;
            var uri = new Uri("ms-appx:///Assets/Nuclear_symbol.svg");

            RunOnUIThread.Execute(() =>
            {
                imageIcon = new ImageIcon();
                Verify.AreEqual(((SolidColorBrush)imageIcon.Foreground).Color, Colors.White) ;

                Log.Comment("Validate that you can change the properties.");

                imageIcon.Foreground = new SolidColorBrush(Windows.UI.Colors.Red);
                imageIcon.Source = new SvgImageSource(uri);
                Content = imageIcon;
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(imageIcon.Foreground is SolidColorBrush);
                Verify.AreEqual(Windows.UI.Colors.Red, (imageIcon.Foreground as SolidColorBrush).Color);
                Verify.AreEqual(uri, ((SvgImageSource)imageIcon.Source).UriSource);
                var image = ((Image)VisualTreeHelper.GetChild(VisualTreeHelper.GetChild(imageIcon, 0), 0));
                Verify.IsTrue(image.IsLoaded);
            });
        }
    }
}
