// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>
#include "XamlMetadataProviderOverrider.h"
namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Tools { namespace AppAnalysis {

        class ImageDecodingRuleIntegrationTests : public WEX::TestClass<ImageDecodingRuleIntegrationTests>
        {
        public:
            BEGIN_TEST_CLASS(ImageDecodingRuleIntegrationTests)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.Diagnostics.AppAnalysis.dll")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
                TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"e22a917c-ad18-4a09-bff9-d3ca3e5ee0b8")
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // All AppAnalysis tests are unreliable or failing.
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_CLASS_CLEANUP(ClassCleanup)
            TEST_METHOD_SETUP(TestSetup)
            TEST_METHOD_CLEANUP(TestCleanup)

            BEGIN_TEST_METHOD(VerifiesCatchesSyncronousDecode)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies the ImageDecodingRule fires when source is set on an image before in live tree.")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
                TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyCatchesImageBrushInEllipse)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies the ImageDecodingRule fires when using an image brush for ellipses.")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
                TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyNoFireImageBrushInEllipseWithDecodeSize)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies the ImageDecodingRule doesn't fire when specifying the decode size "
                                      " when using an image brush for ellipses.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyCatchesRedecodeAfterDecodeSizeSpecified)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies the ImageDecodingRule fires when we redecode an image after DecodePixelWidth/Height set.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
                TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyNoFireAfterPlateauScalingChangedSmaller)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies the ImageDecodingRule doesn't fires when we dont redecode due to scaling change where we scaled smaller.")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyNoFireAfterPlateauScalingChangedLarger)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies the ImageDecodingRule fires doesn't fire when redecode due to scaling change where we scaled larger.")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyCatchesSecondIncorrectDecode)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies the ImageDecodingRule fires when we redecode an image synchronously with a new source after it was first "
                                      "decoded correctly.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
                TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyMultipleImagesDifferentSizeSameUri)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies the ImageDecodingRule doesn't fire for the smaller image when DTRS is used for two images.")
            END_TEST_METHOD()

        private:
            Platform::String^ GetResourcesPath() const;
            Concurrency::task<void> OpenImageSync(
                xaml_media::Imaging::BitmapImage^ bitmapImage,
                Platform::String^ file);

            Concurrency::task<void> OpenImageAsync(
                xaml_media::Imaging::BitmapImage^ bitmapImage,
                Platform::String^ file);

            void MultipleImagesTestHelper(
                Platform::String^ xamlPath,
                Platform::String^ imagePath
            );

            void PlateuScaleTestHelper(
                Platform::String^ xamlPath,
                float scaleFactor,
                bool setDecodePixelSize
            );
        };
       
    } } 
} } } }
