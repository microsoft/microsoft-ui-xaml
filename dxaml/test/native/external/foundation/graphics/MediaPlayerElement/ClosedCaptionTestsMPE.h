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
                TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_24H2)
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(MediaPlayerElementWithCustomRegion)
                TEST_METHOD_PROPERTY(L"Description", L"Closed Captions with custom region.")
                TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_24H2)
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(MediaPlayerElementWithLines)
                TEST_METHOD_PROPERTY(L"Description", L"Closed Captions with cues and multiple lines")
                TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_24H2)
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(MediaPlayerElementWithBadFile)
                TEST_METHOD_PROPERTY(L"Description", L"Closed Captions with bad ttm")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(MediaPlayerElementWithSubformatting)
                TEST_METHOD_PROPERTY(L"Description", L"Closed Captions with subformatting")
                TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_24H2)
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
                TEST_METHOD_PROPERTY(L"TestPass:MinOSVer", WINDOWS_OS_VERSION_19H1) // TODO: [MediaPlayerElement] Some CC tests fail on RS5
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(MediaPlayerElementWithMultipleColors)
                TEST_METHOD_PROPERTY(L"Description", L"Closed Captions with multi-color line")
                TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_24H2)
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
                TEST_METHOD_PROPERTY(L"TestPass:MinOSVer", WINDOWS_OS_VERSION_19H1) // TODO: [MediaPlayerElement] Some CC tests fail on RS5
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(MediaPlayerElementWithOutline)
                TEST_METHOD_PROPERTY(L"Description", L"Closed Captions with text outline")
                TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_24H2)
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(RemovedWhenDisabled)
                TEST_METHOD_PROPERTY(L"Description", L"Closed Captions are removed when the text track is disabled")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TimedTextSourceTeardownDoesNotLeak)
                TEST_METHOD_PROPERTY(L"Description", L"Regression: MediaPlayerElement CC teardown must not leak the TimedMetadataTracksChanged registration (CTimedTextSource::SetMediaPlayer ordering)")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
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

            BEGIN_TEST_METHOD(CaptionCueWithoutStyleOrRegionUsesDefaultPresentation)
                TEST_METHOD_PROPERTY(L"Description", L"A programmatic caption cue without CueStyle/CueRegion renders with the default font, colors, alignment, wrapping and region padding.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CaptionSubformatStylesMapGenericFontFamiliesWeightsAndDecorations)
                TEST_METHOD_PROPERTY(L"Description", L"Each caption subformat becomes a Run that maps the TTML generic font family, weight, font style, decorations and flow direction of its style.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CaptionRegionAlignmentExtentPositionAndPaddingUnits)
                TEST_METHOD_PROPERTY(L"Description", L"Caption regions honor display alignment, pixel/percentage extent, position and padding, z-index, line alignment and wrapping; an image cue without extent gets the default padding.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CaptionSubformatGapsAndPercentOutlineThickness)
                TEST_METHOD_PROPERTY(L"Description", L"Text between subformats uses the cue style and a percentage outline thickness is scaled by the font size.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(UserCaptionColorsAndOpacityOverrideCueStyle)
                TEST_METHOD_PROPERTY(L"Description", L"Windows caption color/opacity settings override the cue style text, background and region colors and the text opacity.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(UserCaptionColorsAndOpacityOverrideCueStyleAlternatePalette)
                TEST_METHOD_PROPERTY(L"Description", L"Windows caption settings map the remaining caption colors and opacities to the rendered caption.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(UserCaptionFontStyleAndSizeScaleCaptionText)
                TEST_METHOD_PROPERTY(L"Description", L"Windows caption font style and size settings override the cue font family and scale the font size and text margin.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(UserCaptionRaisedAndDepressedEdgeEffects)
                TEST_METHOD_PROPERTY(L"Description", L"Raised and depressed caption edge effects render offset copies of the caption text under the TextBlock for every caption size.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(UserCaptionUniformEdgeEffect)
                TEST_METHOD_PROPERTY(L"Description", L"The uniform caption edge effect renders a ring of offset copies of the caption text for every caption size.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(UserCaptionDropShadowEdgeEffect)
                TEST_METHOD_PROPERTY(L"Description", L"The drop shadow caption edge effect applies a size dependent drop shadow to the caption text instead of offset copies.")
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

