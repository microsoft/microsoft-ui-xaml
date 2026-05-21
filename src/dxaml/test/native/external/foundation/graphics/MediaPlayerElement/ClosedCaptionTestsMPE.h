// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

 
namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Graphics { namespace Media {

        class MediaCCTestsMPE : public WEX::TestClass<MediaCCTestsMPE>
        {
        public:
            BEGIN_TEST_CLASS(MediaCCTestsMPE)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_METHOD_SETUP(TestSetup)
            TEST_METHOD_CLEANUP(TestCleanup)

            BEGIN_TEST_METHOD(MediaPlayerElementWithCustomStyle)
                TEST_METHOD_PROPERTY(L"Description", L"Closed Captions with custom style.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(MediaPlayerElementWithCustomRegion)
                TEST_METHOD_PROPERTY(L"Description", L"Closed Captions with custom region.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(MediaPlayerElementWithLines)
                TEST_METHOD_PROPERTY(L"Description", L"Closed Captions with cues and multiple lines")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(MediaPlayerElementWithBadFile)
                TEST_METHOD_PROPERTY(L"Description", L"Closed Captions with bad ttm")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(MediaPlayerElementWithSubformatting)
                TEST_METHOD_PROPERTY(L"Description", L"Closed Captions with subformatting")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
                TEST_METHOD_PROPERTY(L"TestPass:MinOSVer", WINDOWS_OS_VERSION_19H1) // TODO: [MediaPlayerElement] Some CC tests fail on RS5
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(MediaPlayerElementWithMultipleColors)
                TEST_METHOD_PROPERTY(L"Description", L"Closed Captions with multi-color line")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
                TEST_METHOD_PROPERTY(L"TestPass:MinOSVer", WINDOWS_OS_VERSION_19H1) // TODO: [MediaPlayerElement] Some CC tests fail on RS5
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(MediaPlayerElementWithOutline)
                TEST_METHOD_PROPERTY(L"Description", L"Closed Captions with text outline")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(RemovedWhenDisabled)
                TEST_METHOD_PROPERTY(L"Description", L"Closed Captions are removed when the text track is disabled")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(MediaPlayerElementWithImageCueNoMTCUsePercent)
                TEST_METHOD_PROPERTY(L"Description", L"Closed Captions with image cue based on percentage and MTC off")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(MediaPlayerElementWithImageCueNoMTCUsePixels)
                TEST_METHOD_PROPERTY(L"Description", L"Closed Captions with image cue based on pixels and MTC off")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(MediaPlayerElementWithMultipleImageCues)
                TEST_METHOD_PROPERTY(L"Description", L"Closed Captions with multiple image cues")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()
            
            BEGIN_TEST_METHOD(MediaPlayerElementWithCentering)
                TEST_METHOD_PROPERTY(L"Description", L"Closed Captions with center text alignment")
                TEST_METHOD_PROPERTY(L"Ignore",L"TRUE") // TODO: enable this unreliable test
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

        private:
            inline Platform::String^ GetResourcesPath() const;

            // Internally the CC parser uses a float so the actual values given may be slighty off
            static bool CheckTimeWithError(_In_ INT64 given, _In_ INT64 expected, _In_ const INT64 ErrorMargin = 5)
            {
                return (given >= (expected - ErrorMargin)) && (given <= (expected + ErrorMargin));
            }

            void MediaPlayerElementWithImageCueHelper(bool userMTC, int numberOfCues, bool usePercentage);
            void MediaPlayerElementWithTextCueHelper(bool userMTC, Platform::String^ ccFile, Platform::String^ lang);

        };

    } } }
} } } }

