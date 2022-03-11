﻿// Copyright (c) Microsoft Corporation. All rights reserved.
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
                
                /*
                #3949 is created to re-enable this part
                This is an unparented ImageIcon, so looking up the default foreground and verifying
                is a bit wierd. The colors are also chaning, so this is going to fail with those changes
                So commenting this check out for now to make the test more resilient.

                var theme = Application.Current.RequestedTheme;
                if (theme == ApplicationTheme.Dark)
                {
                    Verify.AreEqual(((SolidColorBrush)imageIcon.Foreground).Color, Colors.White);
                }
                else
                {
                    Verify.AreEqual(((SolidColorBrush)imageIcon.Foreground).Color, Colors.Black);
                }
                */

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
                if (PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone5))
                {
                    var image = ((Image)VisualTreeHelper.GetChild(VisualTreeHelper.GetChild(imageIcon, 0), 0));
                    Verify.IsTrue(image.IsLoaded);
                }
            });
        }
    }
}
