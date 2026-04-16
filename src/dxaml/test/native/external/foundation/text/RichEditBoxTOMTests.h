// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Text {
        class RichEditBoxTOMTests : public WEX::TestClass < RichEditBoxTOMTests >
        {
        public:
            BEGIN_TEST_CLASS(RichEditBoxTOMTests)
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
                TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"32301317-5c46-4350-8af6-a06552076e89")
                TEST_CLASS_PROPERTY(L"HelixWorkItemCreation", L"CreateWorkItemPerTestClass")
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_CLASS_CLEANUP(ClassCleanup)
            TEST_METHOD_SETUP(TestSetup)
            TEST_METHOD_CLEANUP(TestCleanup)

            BEGIN_TEST_METHOD(DocumentDraw)
                TEST_METHOD_PROPERTY(L"Description", L"Testing TOM (Text Object Model API exposed through Windows.UI.Text namesapce by RichEdit) API to include drawing bitmap and lines through RichEditBox's Document property.")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestSelection)
                TEST_METHOD_PROPERTY(L"Description", L"Testing TOM (Text Object Model API exposed through Windows.UI.Text namesapce by RichEdit) Selection API on RichEditBox's Document property.")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestClipboardCopyFormats)
                TEST_METHOD_PROPERTY(L"Description", L"Testing ClipboardCopyFormat property on RichEditBox.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestLinkNavigation)
                TEST_METHOD_PROPERTY(L"Description", L"Testing link navigation on RichEditBox.")
                TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // disable the test while investigating solution for random test failures
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CheckFiresManipulationEvents)
                TEST_METHOD_PROPERTY(L"Description", L"Validates RichEditBox can fire manipulation events.")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(RangeFormat)
                TEST_METHOD_PROPERTY(L"Description", L"Validates updating format for RichEdit's range object.")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(RangeFormatSizeAndSpacing)
                TEST_METHOD_PROPERTY(L"Description", L"Validates updating size and spacing for RichEdit's range objects.")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(SelectionFormat)
                TEST_METHOD_PROPERTY(L"Description", L"Validates updating format for RichEdit's selection object.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"OneCore")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestLongStringPaste)
                TEST_METHOD_PROPERTY(L"Description", L"Validates pasting and rendering of very long text.")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(PasteImage)
                TEST_METHOD_PROPERTY(L"Description", L"Validates pasting bitmap image to RichEditBox and paste event handling.")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(PasteAndSelectImage)
                TEST_METHOD_PROPERTY(L"Description", L"Validates pasting bitmap image into RichEditBox's current selection and bitmap image invertion when selected.")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(RichEditBoxMaxLength)
                TEST_METHOD_PROPERTY(L"Description", L"Verifiy keyboard input when MaxLength property is set")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(SetTextAdheresToMaxLength)
                TEST_METHOD_PROPERTY(L"Ignore", L"True") // Tracked by a known issue
                TEST_METHOD_PROPERTY(L"Description", L"Verify that text cannot exceed MaxLength when set programmatically")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(PasteImageAdheresToMaxLength)
                TEST_METHOD_PROPERTY(L"Description", L"Verify that an image paste cannot exceed MaxLength")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(PasteTextAdheresToMaxLength)
                TEST_METHOD_PROPERTY(L"Description", L"Verify that an text paste cannot exceed MaxLength")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(AlignmentIncludesTrailingWhitespace)
                TEST_METHOD_PROPERTY(L"Description", L"Validates AlignmentIncludesTrailingWhitespace property on ITextDocument2.")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(UseLfAndUseCrLf)
                TEST_METHOD_PROPERTY(L"Description", L"Validates TextGetOptions UseLf and UseCrLf.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(IgnoreTrailingCharacterSpacing)
                TEST_METHOD_PROPERTY(L"Description", L"Validates IgnoreTrailingCharacterSpacing document setting.")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(FormatFontWeight)
                TEST_METHOD_PROPERTY(L"Description", L"Validates Set/Get FontWeight property on ITextCharacterFormat.")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(StartEndHorizontalTextAlignment)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies new TextAlignment enums and new HorizontalTextAlignment property")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyClearUndoRedoHistory)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies ClearUndoRedoHistory clears the undo/redo history.")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyProgrammaticSelectionCutRaisesRichEditBoxEvent)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that calling Cut on the RichEditBox's selection range raises the Cut event.")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyProgrammaticSelectionCopyRaisesRichEditBoxEvent)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that calling Copy on the RichEditBox's selection range raises the Cut event.")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyProgrammaticSelectionPasteRaisesRichEditBoxEvent)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that calling Paste on the RichEditBox's selection range raises the Cut event.")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            END_TEST_METHOD()

        private:
            Platform::String^ GetResourcesPath() const;
            void PrepRichEditBox(xaml_controls::RichEditBox^ rebx, bool ignoreTrailingCharacterSpacing, Xaml::TextAlignment alignment, bool underline, bool strikethrough, bool partial, bool multiline);
        };
    } }
} } } }

