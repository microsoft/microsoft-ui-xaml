// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using Private.Infrastructure;
using System;
using System.IO;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Markup;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Media.Imaging;
using Microsoft.UI.Xaml.Tests.Common;
using Microsoft.UI.Xaml.Tests.Common.EventsListeners;
using WEX.TestExecution;
using WEX.TestExecution.Markup;

namespace Microsoft.UI.Xaml.Tests.Foundation.Image
{
    [TestClass]
    public class ManagedImageTests : XamlTestsBase
    {
        public string NativeImageResourcesPath
        {
            get
            {
                return XamlTestsBase.Context.TestDeploymentDir + @"\resources\native\external\foundation\graphics\image\";
            }
        }

        public string ManagedImageResourcesPath
        {
            get
            {
                return XamlTestsBase.Context.TestDeploymentDir + @"\resources\managed\foundation\image\";
            }
        }

        [ClassInitialize]
        [TestProperty("BinaryUnderTest", "Microsoft.UI.Xaml.dll")]
        [TestProperty("RunAs", "UAP")]
        [TestProperty("UAP:Praid", "XamlManagedTAEFTests")]
        [TestProperty("Classification", "Integration")]
        [TestProperty("VelocityTestPass:OneCoreStrict", "Desktop")]
        public static void Setup(TestContext context)
        {
            // Make sure you always call XamlTestsBase.SetupBase
            // here to ensure the test services are initialized.
            XamlTestsBase.SetupBase(context);
        }

        [ClassCleanup]
        public void ClassCleanup()
        {
            base.CommonClassCleanup();
        }

        [TestMethod]
        [TestProperty("Description", "Validates an image can be loaded without crashing.")]
        [TestProperty("Hosting:Mode", "WPF")]
        public void BasicTest()
        {
            Microsoft.UI.Xaml.Controls.Image image = null;
            BitmapImage bi = null;

            UIExecutor.Execute(() =>
            {
                image = new Microsoft.UI.Xaml.Controls.Image();
                bi = new BitmapImage();
                image.Source = bi;
                
                TestServices.WindowHelper.WindowContent = image;
            });
            TestServices.WindowHelper.WaitForIdle();

            using (var imageOpened = new EventTester<FrameworkElement, RoutedEventArgs>(image, "ImageOpened"))
            {
                UIExecutor.Execute(() =>
                {
                    bi.UriSource = new Uri(NativeImageResourcesPath + "rainier_444_2048x1536.jpg");
                });
                
                imageOpened.Wait();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Verifies BitmapImage can be the implicit content of Image.")]
        public void BitmapImageAsImplicitSource()
        {
            UIExecutor.Execute(() =>
            {
                var image = (Microsoft.UI.Xaml.Controls.Image)XamlReader.Load(
                    "<Image xmlns=\"http://schemas.microsoft.com/winfx/2006/xaml/presentation\">" +
                    "<BitmapImage DecodePixelWidth=\"40\" DecodePixelHeight=\"40\" />" +
                    "</Image>");

                var source = (BitmapImage)image.Source;
                Verify.IsNotNull(source);
                Verify.AreEqual(40, source.DecodePixelWidth);
                Verify.AreEqual(40, source.DecodePixelHeight);
            });
        }

        [TestMethod]
        [TestProperty("Description", "Verifies SvgImageSource can be the implicit content of Image.")]
        public void SvgImageSourceAsImplicitSource()
        {
            UIExecutor.Execute(() =>
            {
                var image = (Microsoft.UI.Xaml.Controls.Image)XamlReader.Load(
                    "<Image xmlns=\"http://schemas.microsoft.com/winfx/2006/xaml/presentation\">" +
                    "<SvgImageSource RasterizePixelWidth=\"40\" RasterizePixelHeight=\"40\" />" +
                    "</Image>");

                var source = (SvgImageSource)image.Source;
                Verify.IsNotNull(source);
                Verify.AreEqual(40.0, source.RasterizePixelWidth);
                Verify.AreEqual(40.0, source.RasterizePixelHeight);
            });
        }

        [TestMethod]
        [TestProperty("Description", "Known to cause issues with managed peer pegging of images in XAML.")]
        public void AppBarBitmapIcon()
        {
            UIExecutor.Execute(() =>
            {
                string xamlText = File.ReadAllText(ManagedImageResourcesPath + @"\AppBarBitmapIcon.xaml");
                Grid objFromFile = (Grid)XamlReader.Load(xamlText);
                Verify.IsNotNull(objFromFile);
                TestServices.WindowHelper.WindowContent = objFromFile;
            });

            TestServices.WindowHelper.WaitForIdle();
        }
    }
}
