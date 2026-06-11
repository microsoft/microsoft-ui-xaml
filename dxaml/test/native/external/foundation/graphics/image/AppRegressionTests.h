// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Graphics { namespace Image {

        class AppRegressionTests : public WEX::TestClass<AppRegressionTests>
        {
        public:
            BEGIN_TEST_CLASS(AppRegressionTests)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")

                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"9882729e-eea8-4b89-99e7-92145be50e76;bd1463b3-e5f2-4d54-9394-63a431c53a6e;b7d949c9-6779-44c5-b16b-1f51e18f3866")

#if defined(_WIN64) // Disable the test on 64-bit build because of floating point differences between x87 and sse instructions
                TEST_CLASS_PROPERTY(L"Ignore", L"TRUE")
#endif
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_METHOD_SETUP(TestSetup)

            TEST_METHOD_CLEANUP(TestCleanup)

            BEGIN_TEST_METHOD(TFS_895642)
                TEST_METHOD_PROPERTY(L"Description", L"Regression test which verifies for a crash when multiple hubs are instantiated with image backgrounds.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TFS_2144093)
                TEST_METHOD_PROPERTY(L"Description", L"Renders two elements referring the same image URI, where one of the elements requires software surfaces. This may lead to not updating hardware surfaces and missing textures in render.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TFS_2529029)
                TEST_METHOD_PROPERTY(L"Description", L"Regression test: This crashes if an image is opened at one size with BTIL and then changes size and needs software rasterization (which disables BTIL).")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TFS_2187595)
                TEST_METHOD_PROPERTY(L"Description", L"Regression test that verifies if an image that is not in the tree but has another image referencing it's hardware cached surface gets updated properly.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TFS_2524409)
                TEST_METHOD_PROPERTY(L"Description", L"Regression test that verifies changing the plateau scale with certain key properties being used and ensuring the image is rendered correctly.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Zoom scale not applied
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TFS_1715872)
                TEST_METHOD_PROPERTY(L"Description", L"Regression test that verifies decoding a smaller image while an image is being decoded with background thread image loading.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TFS_6697031)
                TEST_METHOD_PROPERTY(L"Description", L"Regression test that verifies setting DecodePixelHeight with DecodePixelType=Logical using a large plateau scale and an image with rotated EXIF data.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Zoom scale not applied
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TFS_7302806)
                TEST_METHOD_PROPERTY(L"Description", L"Regression test that verifies setting an image URI source to a different local resource and immediately attempting to click the image which has a Tapped handler.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TFS_4354828)
                TEST_METHOD_PROPERTY(L"Description", L"Regression test that verifies a discarded hardware surface for an image element that is not walked but is cache referenced by another image.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Illegal to wait on a task in a Windows Runtime STA
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TFS_9797797)
                TEST_METHOD_PROPERTY(L"Description", L"Regression test: Set the URI on the BitmapImage after it has been used with stream.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Illegal to wait on a task in a Windows Runtime STA
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TFS_9742148)
                TEST_METHOD_PROPERTY(L"Description", L"Regression test: Flushes surface updates in-between locks of a surface during asynchronous decoding (BitmapImage version).")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TFS_9062885)
                TEST_METHOD_PROPERTY(L"Description", L"Set the explicit decode size that causes allocation of an upscaled surface")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TFS_12066837)
                TEST_METHOD_PROPERTY(L"Description", L"Replace the image Uri in MeasureOverride")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Crash in test
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

        private:
            inline Platform::String^ GetResourcesPath() const;

            static void SetSourceAsyncFromFile(xaml_imaging::BitmapImage^ bi, Platform::String^ filePath);
        };

    } } }
} } } }

