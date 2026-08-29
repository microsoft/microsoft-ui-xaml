// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>
#include <WUCRenderingScopeGuard.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Graphics {

        class CaretBrowsingTests : public WEX::TestClass<CaretBrowsingTests>
        {
        public:
            BEGIN_TEST_CLASS(CaretBrowsingTests)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"Ignore", L"TRUE")     // Some graphics tests are failing
                TEST_CLASS_PROPERTY(L"HelixWorkItemCreation", L"CreateWorkItemPerTestClass")
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_METHOD_SETUP(TestSetup)

            TEST_METHOD_CLEANUP(TestCleanup)

            BEGIN_TEST_METHOD(TurnCaretBrowsingOffAndClicking)
                TEST_METHOD_PROPERTY(L"Description", L"CaretBrowsingOff")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TurnCaretBrowsingOffAndTabbing)
                TEST_METHOD_PROPERTY(L"Description", L"CaretBrowsingOffTabbing")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Catastrophic failure
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TurnCaretBrowsingOnAndClicking)
                TEST_METHOD_PROPERTY(L"Description", L"CaretBrowsingOn")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TurnCaretBrowsingOnAndTabbing)
                TEST_METHOD_PROPERTY(L"Description", L"CaretBrowsingOnTabbing")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Catastrophic failure
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TurnCaretBrowsingOnAndClickingNonSelectable)
                TEST_METHOD_PROPERTY(L"Description", L"CaretBrowsingOn click the non selectable text")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TurnCaretBrowsingOnAndTabbingNonSelectable)
                TEST_METHOD_PROPERTY(L"Description", L"CaretBrowsingOnTabbing tabbing the non selectable text")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Catastrophic failure
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ShowCaretBrowsingDialogViaF7)
                TEST_METHOD_PROPERTY(L"Description", L"CaretBrowsingOn via F7")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CaretBrowsingPermDisableViaF7Checkbox)
                TEST_METHOD_PROPERTY(L"Description", L"Verify F7, Don't Ask Again, plus No permanently disables F7")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CaretMovingByChar)
                TEST_METHOD_PROPERTY(L"Description", L"Moving Caret by arrow key")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Catastrophic failure
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(SelectionMovingByChar)
                TEST_METHOD_PROPERTY(L"Description", L"Making selection by shift + arrow key")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Catastrophic failure
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CaretMovingByWord)
                TEST_METHOD_PROPERTY(L"Description", L"Moving Caret by control + arrow key")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Catastrophic failure
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(SelectionMovingByWord)
                TEST_METHOD_PROPERTY(L"Description", L"Making selection by shift + control + arrow key")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Catastrophic failure
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CaretMovingByLine)
                TEST_METHOD_PROPERTY(L"Description", L"Moving Caret by control + Home/End key")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Catastrophic failure
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(SelectionMovingByLine)
                TEST_METHOD_PROPERTY(L"Description", L"Making selection by shift + control + Home/End key")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Catastrophic failure
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CaretMovingByContent)
                TEST_METHOD_PROPERTY(L"Description", L"Moving Caret by Home/End key")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Catastrophic failure
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(SelectionMovingByContent)
                TEST_METHOD_PROPERTY(L"Description", L"Making selection by shift + Home/End key")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Catastrophic failure
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CaretMovingUpAndDown)
                TEST_METHOD_PROPERTY(L"Description", L"Moving Caret by Up/Down key")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Catastrophic failure
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(SelectionMovingUpAndDown)
                TEST_METHOD_PROPERTY(L"Description", L"Making selection by shift + Up/Down key")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Catastrophic failure
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(RichTextBlockCaretMovingUpAndDown)
                TEST_METHOD_PROPERTY(L"Description", L"Moving Caret by Up/Down key in RichTextBlock and its Overflows")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Catastrophic failure
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(RichTextBlockCaretMoving)
                TEST_METHOD_PROPERTY(L"Description", L"Moving Caret in RichTextBlock with multiple line and InlineUIContainer")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Catastrophic failure
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(RemoveSelectionShowCaret)
                TEST_METHOD_PROPERTY(L"Description", L"When there is selection, press arrow key to clear selection and show caret. create selection ranged two character move to start or end of seleciton")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CaretMovingInForceWrappedTextBlock)
                TEST_METHOD_PROPERTY(L"Description", L"Moving Caret around in a TextBlock which is force-wrapping a long word")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Catastrophic failure
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CaretMovingInTextBlockHyperLink)
                TEST_METHOD_PROPERTY(L"Description", L"Navigating and tabbing around in HyperLink")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(PopContentDialogTurnOnCaretBrowsing)
                TEST_METHOD_PROPERTY(L"Description", L"Pop the contentdialog and turn on the caretbrowsing mode")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Catastrophic failure
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(PopContentDialogTurnOffCaretBrowsing)
                TEST_METHOD_PROPERTY(L"Description", L"Pop the contentdialog and turn off the caretbrowsing mode")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Catastrophic failure
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(PopContentDialogTurnOnCaretBrowsingWithCheckBox)
                TEST_METHOD_PROPERTY(L"Description", L"Pop the contentdialog and turn on the caretbrowsing mode with checkbox checked")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")   // Catastrophic failure
            END_TEST_METHOD()

        private:
            void CreateSingleLineTextBlock(xaml_controls::TextBlock^& textBlock);
            void CreateMultiLineTextBlock(xaml_controls::TextBlock^& textBlock);
            void CreatWrappingTextBlock(xaml_controls::TextBlock^& textBlock);
            void CreatForceWrappedTextBlock(xaml_controls::TextBlock^& textBlock);
            void CreateRichTextBlock(xaml_controls::RichTextBlock^& richTextBlock);
            void CreateRichTextBlockWithOverflows(xaml_controls::RichTextBlock^& richTextBlock, xaml_controls::RichTextBlockOverflow^& rtbo, xaml_controls::RichTextBlockOverflow^& rtbo2);
            void CreateTextBlockAndRichTextBlock(xaml_controls::RichTextBlock^& richTextBlock, xaml_controls::TextBlock^& textBlock);
            void CreateNonSelectableTBAndSelectableRTB(xaml_controls::RichTextBlock^& richTextBlock, xaml_controls::TextBlock^& textBlock);
            void VerifyTextBlockCaretPosition(xaml_controls::TextBlock^ textBlock, int offset);
            void VerifyRichTextBlockCaretPosition(xaml_controls::RichTextBlock^ richTextBlock, int offset);
            void VerifyRichTextBlockCaretPosition(xaml_controls::RichTextBlockOverflow^ richTextBlockOverflow, int offset);
            void VerifyTextBlockSelection(xaml_controls::TextBlock^ textBlock, int startOffset, int endOffset);
            void VerifyRichTextBlockSelection(xaml_controls::RichTextBlock^ richTextBlock, int startOffset, int endOffset);
        };
    } }
} } } }

