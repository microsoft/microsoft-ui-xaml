// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>
#include <WUCRenderingScopeGuard.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Graphics {

        class RichTextBlockTests : public WEX::TestClass<RichTextBlockTests>
        {
        public:
            BEGIN_TEST_CLASS(RichTextBlockTests)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"e6d4a8e5-be97-431f-871b-4937e816c8b3;24aa2bdf-d1ac-40bb-bb77-63c409a5da27")
                TEST_CLASS_PROPERTY(L"HelixWorkItemCreation", L"CreateWorkItemPerTestClass")
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_METHOD_SETUP(TestSetup)

            TEST_METHOD_CLEANUP(TestCleanup)

            BEGIN_TEST_METHOD(EmbeddedInlineElementPlacement)
                TEST_METHOD_PROPERTY(L"Description", L"Test the embedded inline element placement when window size is changed.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Missing comp node
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(EmbeddedInlineElementPlacementBidi)
                TEST_METHOD_PROPERTY(L"Ignore",L"TRUE") // re-enable when bug fix for 038422 is FIed to RSMQ branch.
                TEST_METHOD_PROPERTY(L"Description", L"Test the embedded inline element placement for Bidi scenarios")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(SpanWithEmptyInlineCollection)
                TEST_METHOD_PROPERTY(L"Description", L"Verify we can handle Spans with empty InlineCollections.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ParagraphWithEmptyInlineCollection)
                TEST_METHOD_PROPERTY(L"Description", L"Verify we can handle Paragraphs with empty InlineCollections.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(NWRenderWUCFull)
                TEST_METHOD_PROPERTY(L"Description", L"Test the software render path - BitmapCache")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(DoubleTap)
                TEST_METHOD_PROPERTY(L"Description", L"Double tap RTB to invoke text selection gripper")
                TEST_METHOD_PROPERTY(L"Ignore", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CopySelection_Keyboard)
                TEST_METHOD_PROPERTY(L"Description", L"Test copy selection through Ctrl+C")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CopySelection_API)
                TEST_METHOD_PROPERTY(L"Description", L"Test copy selection through API")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(SelectionChangedEvent)
                TEST_METHOD_PROPERTY(L"Description", L"Verify that the SelectionChanged event fires")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // DCPP: RichTextBlockTests::SelectionChangedEvent isn't loading content on WPF
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TextUpdatesWithFocus)
                TEST_METHOD_PROPERTY(L"Description", L"Verify that text changes render correctly when we have the focus")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ClearEmbeddedElements)
                TEST_METHOD_PROPERTY(L"Description", L"")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(GetArrtributeFromDocumentRange)
                TEST_METHOD_PROPERTY(L"Description", L"")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(StartEndHorizontalTextAlignment)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies new TextAlignment enums and new HorizontalTextAlignment property")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(IsTextTrimmedRichTextBlock)
                TEST_METHOD_PROPERTY(L"Description", L"Verify RichTextBlock can return IsTextTrimmed, the property can be bound to, and an event is raised when changed")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(IsTextTrimmedRichTextBlockOverflow)
                TEST_METHOD_PROPERTY(L"Description", L"Verify RichTextBlockOverflow can return IsTextTrimmed, the property can be bound to, and an event is raised when changed")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(DoNotRenderTooManyDiacritics)
                TEST_METHOD_PROPERTY(L"Description", L"Verify XAML does not crash when trying to render more diacritics than LineServices can handle")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(JupRtblXamlP0)
                TEST_METHOD_PROPERTY(L"Description", L"Jupiter RichTextBlock - XamlP0 tests (Ported from legacy:JupRtblXamlP0)")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Different text offsets
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(EmptyOverflowHasNullTextPatternAndCanNavigatePast)
                TEST_METHOD_PROPERTY(L"Description", L"An empty RichTextBlockOverflow with no baseline RichTextBlock should not return a valid textpattern to Narrator")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(EmptyRichTextBlockWithOverflowHasNoText)
                TEST_METHOD_PROPERTY(L"Description", L"An empty RichTextBlock that has an overflow should return a null text range, not crash")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(InlineUIContainerDirections)
                TEST_METHOD_PROPERTY(L"Description", L"Verify positioning of InlineUIContainer when text reading order is reverse of FlowDirection")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Different text offsets
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyInlineUIContainerIsChildOfCorrectRange)
                TEST_METHOD_PROPERTY(L"Description", L"InlineUIContainers need to be found in the appropriate text ranges for UIA to handle them correctly")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(RichTextBlockWordBreakerNavigation)
                TEST_METHOD_PROPERTY(L"Description", L"Ensure text in an RTB is properly broken up for navigation")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(RichTextBlockOverflowWordBreakerNavigation)
                TEST_METHOD_PROPERTY(L"Description", L"Ensure text in an RTBO is properly broken up for navigation")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyNarratorSelectionOnRichTextBlockOverflow)
                TEST_METHOD_PROPERTY(L"Description", L"Ensure that using TextRangeAdapter APIs to get the selection in an RTBO after selecting inside a RTB doesn't crash.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(RichTextBlockOverflowTapOnHyperlink)
                TEST_METHOD_PROPERTY(L"Description", L"Tap on a Hyperlink in an RTBO")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // TODO 36060166: Re-enable after fixing unreliability.
            END_TEST_METHOD()
        private:
            inline Platform::String^ GetResourcesPath() const;
            void IsTextTrimmedPropertyAndEventHelper(Platform::String^ textBlockName, Platform::String^ textContent);
            void IsTextTrimmedBindingHelper(Platform::String^ textBlockName, Platform::String^ textContent);
        };

    } }
} } } }

