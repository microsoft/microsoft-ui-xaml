// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using Common;
using System.Collections.Generic;
using System.IO;
using System.Threading;
using Microsoft.UI.Xaml.Controls;
using MUXControlsTestApp.Utilities;
using Microsoft.UI.Xaml.Media.Imaging;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Controls;
using Windows.Storage;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
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
                is a bit weird. The colors are also changing, so this is going to fail with those changes
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

                imageIcon.Foreground = new SolidColorBrush(Microsoft.UI.Colors.Red);
                imageIcon.Source = new SvgImageSource(uri);
                Content = imageIcon;
                Content.UpdateLayout();
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.IsTrue(imageIcon.Foreground is SolidColorBrush);
                Verify.AreEqual(Microsoft.UI.Colors.Red, (imageIcon.Foreground as SolidColorBrush).Color);
                Verify.AreEqual(uri, ((SvgImageSource)imageIcon.Source).UriSource);

                var image = (Image)VisualTreeHelper.GetChild(imageIcon, 0);
                Verify.IsTrue(image.IsLoaded);
            });
        }

        [TestMethod]
        public void ImageIconAppDataSourceDoesNotLockFile()
        {
            // An image loaded from ms-appdata must not keep its file locked (GitHub #633).
            const string fileName = "ImageIconAppDataSource.png";
            var asset = StorageFile.GetFileFromApplicationUriAsync(new Uri("ms-appx:///Assets/rating_set.png")).AsTask().Result;
            var file = asset.CopyAsync(ApplicationData.Current.LocalFolder, fileName, NameCollisionOption.ReplaceExisting).AsTask().Result;

            using var opened = new ManualResetEvent(false);
            BitmapImage bitmap = null;
            RunOnUIThread.Execute(() =>
            {
                bitmap = new BitmapImage(new Uri("ms-appdata:///local/" + fileName));
                bitmap.ImageOpened += (s, e) => opened.Set();
                Content = new ImageIcon { Width = 32, Height = 32, Source = bitmap };
                Content.UpdateLayout();
            });
            Verify.IsTrue(opened.WaitOne(DefaultWaitTimeInMS), "The ms-appdata image should open.");

            Log.Comment("Overwrite the file while the image is still alive.");
            File.WriteAllBytes(file.Path, new byte[] { 0 });
            GC.KeepAlive(bitmap);
        }
    }
}
