// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>
#include <WUCRenderingScopeGuard.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Graphics {

        class TextBlockTests : public WEX::TestClass<TextBlockTests>
        {
        public:
            BEGIN_TEST_CLASS(TextBlockTests)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"1d91ef47-c885-45e2-a578-7aaf1a1b1296;df11dd90-2e1d-45ff-93cb-cd6c0b87e24d")
                TEST_CLASS_PROPERTY(L"HelixWorkItemCreation", L"CreateWorkItemPerTestClass")
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_METHOD_SETUP(TestSetup)

            TEST_METHOD_CLEANUP(TestCleanup)

            BEGIN_TEST_METHOD(RenderFastPathTextBlocksWUCFull)
                TEST_METHOD_PROPERTY(L"Description", L"Renders a few fast path TextBlocks that are using DWriteTextLayout for layout.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"TestPass:MinOSVer", WINDOWS_OS_VERSION_19H1) // RS5 has different DWrite behavior that breaks the text into different visuals
                TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(RenderFastPathTextBlocksWUCFullWithDebugDevice)
                TEST_METHOD_PROPERTY(L"Description", L"Exercise RenderFastPathTextBlocks with the D3D Debug Device enabled.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Ignore", L"True") // TODO: Re-enable D3D Debug Layer tests
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestTextBlockFlyWeightInlineCollection)
                TEST_METHOD_PROPERTY(L"Description", L"TextBlock's Inline collection is not required for the DWriteLayout mode, we lazily create them when user wants to read it or text mode fallback to Normal")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestTextBlockLineStackingStrategies)
                TEST_METHOD_PROPERTY(L"Description", L"Test LineStackingStrategies with single string scenario, they should behave identical.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"TestPass:MinOSVer", WINDOWS_OS_VERSION_19H1) // RS5 has different DWrite behavior that breaks the text into different visuals
                TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.                
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TextPointerTest)
                TEST_METHOD_PROPERTY(L"Description", L"Test basic TextPointer functionalities.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestTextLineBounds)
                TEST_METHOD_PROPERTY(L"Description", L"Test TextLineBounds on the fast path")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(FastPathTextBlockPadding)
                TEST_METHOD_PROPERTY(L"Description", L"Test TextBlock Padding property on the fast path")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(FastPathTextBlockPaddingWUCFull)
                TEST_METHOD_PROPERTY(L"Description", L"Test TextBlock Padding property on the fast path")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(NumberSubstitutionTest)
                TEST_METHOD_PROPERTY(L"Description", L"For languages that could require number substitution, do not use the fast path.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(PlateauScaleTestWUCFull)
                TEST_METHOD_PROPERTY(L"Description", L"Test fast path rendering with high dpi.")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Zoom scale not applied
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ActualSizeIncludePadding)
                TEST_METHOD_PROPERTY(L"Description", L"Verify that the ActualWidth and ActualHeight properties include Padding")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(EmptyTextBlockMeasure)
                TEST_METHOD_PROPERTY(L"Description", L"Validate empty TextBlock meausre.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(HitTestEmptyTextBlock)
                TEST_METHOD_PROPERTY(L"Description", L"Validate empty TextBlock hittesting.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(HitTestTightBoundTextBlock)
                TEST_METHOD_PROPERTY(L"Description", L"Validate TextLineBounds = Tight hittesting.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(GetUIAAttributesFromEmptyTextRange)
                TEST_METHOD_PROPERTY(L"Description", L"Validate gettting attributes form empty text range.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Null UIA element
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(FindAttributeInTextRange)
                TEST_METHOD_PROPERTY(L"Description", L"Validate FindAttribute does not crash when FailFast is enabled.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(FindTextInTextRange)
                TEST_METHOD_PROPERTY(L"Description", L"Validate FindText does not crash when FailFast is enabled.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(UIATextChangedEvent)
                TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // Firing UIA TextChanged for TextBlock/RichTextBlock has been removed.
                                                         // Keep the test case in case we re-attempt adding these events in RS3+
                TEST_METHOD_PROPERTY(L"Description", L"Verify UIA text changed events are correctly fired.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(GridLayoutTest)
                TEST_METHOD_PROPERTY(L"Description", L"Test fast path layout in a grid.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(MaxLineLongWords)
                TEST_METHOD_PROPERTY(L"Description", L"Verify Maxline behavior on the fast path, when the word is long")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(IsTextPerformanceVisualizationEnabled)
                TEST_METHOD_PROPERTY(L"Description", L"Test debugger settings for the fast path")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TextDecorations)
                TEST_METHOD_PROPERTY(L"Description", L"Verify fast path supports underlines")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ChangeTextDecorations)
                TEST_METHOD_PROPERTY(L"Description", L"Verify textblocks redraw when the TextDecorations property changes.")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(SelectionChangedEvent)
                TEST_METHOD_PROPERTY(L"Description", L"Verify that the SelectionChanged event fires")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.                
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(SelectionTextUpdate)
                TEST_METHOD_PROPERTY(L"Description", L"Updating text when current selection is active")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(DisableTextSelection)
                TEST_METHOD_PROPERTY(L"Description", L"Disable IsTextSelectionEnabled property when current selection is active")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // DCPP: RichTextBlockTests::SelectionChangedEvent isn't loading content on WPF
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TextUpdatesWithFocus)
                TEST_METHOD_PROPERTY(L"Description", L"Verify that text changes render correctly when we have the focus")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(UpdateFontScale)
                TEST_METHOD_PROPERTY(L"Description", L"Validate font scale change.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(LineHeightScaleWithFontScale)
                TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") //Enable when line height is scaled with font size.
                TEST_METHOD_PROPERTY(L"Description", L"Validate line height will scale propotionally with font scale.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(MixedFontFamily)
                TEST_METHOD_PROPERTY(L"Description", L"Validate font fallback happens correctly for mixed font family names.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(Typography)
                TEST_METHOD_PROPERTY(L"Description", L"Verify fast path supports Typography")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TextBlockSelectionWUCFull)
                TEST_METHOD_PROPERTY(L"Description", L"Verify fast path text selection")
                TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") //BUG:
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(HyperlinkSelection)
                TEST_METHOD_PROPERTY(L"Description", L"Verify single hyperlink selection")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // TODO 36060166: Re-enable after fixing unreliability.
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ImageFontsWUCFull)
                TEST_METHOD_PROPERTY(L"Description", L"Validate hw rendering for bitmap font and svg font.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TilingWUCFull)
                TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // No Dcomp/MockDComp virtual surface support yet
                TEST_METHOD_PROPERTY(L"Description", L"Validates that TextBlock is correctly rendered when tiled due to large texture limitations.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(BatchingWUCFull)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that TextBlock is correctly rendered when batching multiple glyph runs.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(InvalidWSS)
                TEST_METHOD_PROPERTY(L"Description", L"Verify fast path text block can handle invalid WSS")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(IsTextTrimmedFastPath)
                TEST_METHOD_PROPERTY(L"Description", L"Verify fast path text block can return IsTextTrimmed, the property can be bound to, and an event is raised when changed")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(IsTextTrimmedSlowPath)
                TEST_METHOD_PROPERTY(L"Description", L"Verify slow path text block can return IsTextTrimmed, the property can be bound to, and an event is raised when changed")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(StartEndHorizontalTextAlignment)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies new TextAlignment enums and new HorizontalTextAlignment property")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(NoContextFlyoutWhenSelectionIsDisabled)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies no context flyout can be shown when selection is not enabled on TextBlock")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(WordBreakerSelection)
                TEST_METHOD_PROPERTY(L"Description", L"Ensure text in a TextBlock is properly broken up for double tap selection")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")    // DCPP: TextBlockTests::WordBreakerSelection failing because tapping isn't selecting a word
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TextBlockWordBreakerNavigation)
                TEST_METHOD_PROPERTY(L"Description", L"Ensure text in a TextBlock is properly broken up for navigation")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(RasterizationScale)
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

        private:
            void TextBlockSelectionInternal(
                Microsoft::UI::Xaml::Tests::Common::DCompRendering rendering);

            inline Platform::String^ GetResourcesPath() const;

            void DCompValidationHelper(
                Platform::String^ filename,
                float scale,
                MockDComp::SurfaceComparison comparisonMode,
                float fontScale = 1.0f,
                Microsoft::UI::Xaml::Tests::Common::DCompRendering dcompRendering = Microsoft::UI::Xaml::Tests::Common::DCompRendering::WUCCompleteSynchronousCompTree
            );

            void IsTextTrimmedPropertyAndEventHelper(Platform::String^ textBlockName);
            void IsTextTrimmedBindingHelper(Platform::String^ textBlockName);

            void TextBlockSelection(Microsoft::UI::Xaml::Tests::Common::DCompRendering dcompRendering);
        };

    } }
} } } }


