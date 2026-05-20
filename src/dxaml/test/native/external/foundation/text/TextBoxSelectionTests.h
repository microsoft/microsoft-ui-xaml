// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Text {
        class TextBoxSelectionTests : public WEX::TestClass < TextBoxSelectionTests >
        {
        public:
            BEGIN_TEST_CLASS(TextBoxSelectionTests)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"32301317-5c46-4350-8af6-a06552076e89;3192b2bd-30c5-4c19-a6c1-9856b940df63")
                TEST_CLASS_PROPERTY(L"HelixWorkItemCreation", L"CreateWorkItemPerTestClass")
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_CLASS_CLEANUP(ClassCleanup)
            TEST_METHOD_SETUP(TestSetup)
            TEST_METHOD_CLEANUP(TestCleanup)

            BEGIN_TEST_METHOD(TextBoxSelection)
                TEST_METHOD_PROPERTY(L"Description", L"Validates touch selection behavior for TextBox")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TextBoxThemeChangeWUC)
                TEST_METHOD_PROPERTY(L"Description", L"Validates textbox theme change")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(NewTextBoxGripperTest)
                TEST_METHOD_PROPERTY(L"Description", L"Validates gripper showing and hiding after switching focus newly created textbox")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TextBoxBringIntoViewByAPP)
                TEST_METHOD_PROPERTY(L"Description", L"Validates textbox bring into view behavior when APP is responsible for bring into view on SIP Showing.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"OneCore")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TextBoxBringIntoViewByXAML)
                TEST_METHOD_PROPERTY(L"Description", L"Validates textbox bring into view behavior when XAML is responsible for bring into view on SIP Showing.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"OneCore")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TextBoxBringIntoViewPivot)
                TEST_METHOD_PROPERTY(L"Description", L"Validates textbox in pivot header bring into view.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"OneCore")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TextBoxBringIntoViewMultiLine)
                TEST_METHOD_PROPERTY(L"Description", L"Validates caret scrolling into view for small multiline textbox on phone.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TextBoxMouseSelection)
                TEST_METHOD_PROPERTY(L"Description", L"Validates textbox text selection using mouse.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                END_TEST_METHOD()

            BEGIN_TEST_METHOD(TextBoxSelectionHighlightColor)
                TEST_METHOD_PROPERTY(L"Description", L"Validates SelectionHighlightColor and SelectionHighlightColorWhenNotFocused.")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(RichEditBoxSelectionHighlightColor)
                TEST_METHOD_PROPERTY(L"Description", L"Validates SelectionHighlightColor and SelectionHighlightColorWhenNotFocused on RichEditBox.")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyTextBoxSelectionGrippers)
                TEST_METHOD_PROPERTY(L"Description", L"Validates selection grippers.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyTextBlockSelectionGrippers)
                TEST_METHOD_PROPERTY(L"Description", L"Validates TextBlock selection grippers.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyTextBoxSelectionGrippersAreInteractibleWithTextCommandBarFlyout)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that we can interact with grippers when the TextCommandBarFlyout is showing.")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP") // stress mode crash
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyTextBoxFocusAndSelectAll)
                TEST_METHOD_PROPERTY(L"Description", L"Validates scroll into view for calling SelectAll inside the GotFocus handler.")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
                TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // TODO 36060166: Re-enable after fixing unreliability.
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyTextBoxCanPasteClipboardContent)
                TEST_METHOD_PROPERTY(L"Description", L"Validates TextBox CanPasteClipboardContent API.")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyTextBoxPasteFromClipboard)
                TEST_METHOD_PROPERTY(L"Description", L"Validates TextBox clipboard PasteFromClipboard API.")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyTextBoxCopySelectionToClipboard)
                TEST_METHOD_PROPERTY(L"Description", L"Validates TextBox clipboard CopySelectionToClipboard API.")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyTextBoxCutSelectionToClipboard)
                TEST_METHOD_PROPERTY(L"Description", L"Validates TextBox clipboard CutSelectionToClipboard API.")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyTextBoxUndo)
                TEST_METHOD_PROPERTY(L"Description", L"Validates TextBox Undo API.")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyTextBoxRedo)
                TEST_METHOD_PROPERTY(L"Description", L"Validates TextBox Redo API.")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyTextBoxClearUndoRedoHistory)
                TEST_METHOD_PROPERTY(L"Description", L"Validates TextBox ClearUndoRedoHistory API.")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(KeyboardSelectionChangingEvent)
                TEST_METHOD_PROPERTY(L"Description", L"Validates TextBox SelectionChanging event handling by keyboard initiated selection change.")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ProgrammaticSelectionChangingEvent)
                TEST_METHOD_PROPERTY(L"Description", L"Validates TextBox SelectionChanging event handling by programmatic selection change.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(UndoSelectionChangingEvent)
                TEST_METHOD_PROPERTY(L"Description", L"Validates TextBox SelectionChanging event handling when selection is changed by undo.")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(MouseSelectionChangingEvent)
                TEST_METHOD_PROPERTY(L"Description", L"Validates TextBox SelectionChanging event handling by mouse selection change.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TouchSelectionChangingEvent)
                TEST_METHOD_PROPERTY(L"Description", L"Validates TextBox SelectionChanging event handling by touch selection change.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(AppendOnlyTextBoxUsingSelectionChangingEvent)
                TEST_METHOD_PROPERTY(L"Description", L"Validates typical implementation of a append only textbox using SelectionChanging event.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(AppendOnlyRichEditBoxUsingSelectionChangingEvent)
                TEST_METHOD_PROPERTY(L"Description", L"Validates typical implementation of a append only RichEditBox using SelectionChanging event.")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            END_TEST_METHOD()
        private:
            void TextBoxThemeChangeInternal();
            void TextBoxBringIntoView(bool appHandleSIPShowing);

            xaml::Shapes::Rectangle^ GetGripperRect(UIElement^ textElement);
        };
    } }
} } } }

