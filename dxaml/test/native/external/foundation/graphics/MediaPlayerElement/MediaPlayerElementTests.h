// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Graphics { namespace Media {

        class MediaPlayerElementTests : public WEX::TestClass<MediaPlayerElementTests>
        {
        public:
            BEGIN_TEST_CLASS(MediaPlayerElementTests)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_METHOD_SETUP(TestSetup)
            TEST_METHOD_CLEANUP(TestCleanup)

            BEGIN_TEST_METHOD(SmokeTestApi)
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(SmokeTestPeer)
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CanEnableDisableMTC)
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ParseSourceUri)
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(PeerHasLocalizedControlType)
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(PosterSource)
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(MediaRasterizationScale)
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()
            BEGIN_TEST_METHOD(RootRasterizationScale)
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            inline Platform::String^ GetResourcesPath() const;

        private:
            void CanPlayWithMTCInternal();
            void RasterizationScale(double mediaPlayerRasterizationScale, double rootRasterizationScale);
        };

    } } }
} } } }

