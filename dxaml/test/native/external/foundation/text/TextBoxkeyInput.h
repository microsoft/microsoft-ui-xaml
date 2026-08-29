// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Text {
        class TextBoxKeyInputTests : public WEX::TestClass < TextBoxKeyInputTests >
        {
        public:
            BEGIN_TEST_CLASS(TextBoxKeyInputTests)
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
                TEST_CLASS_PROPERTY(L"IsolationLevel", L"Class") // TODO: remove after the associated issue is fixed
                TEST_CLASS_PROPERTY(L"HelixWorkItemCreation", L"CreateWorkItemPerTestClass")
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_CLASS_CLEANUP(ClassCleanup)
            TEST_METHOD_CLEANUP(TestCleanup)

            BEGIN_TEST_METHOD(CheckTextBoxBasicKeyInput)
                TEST_METHOD_PROPERTY(L"Description", L"Validates textbox control simple keyboard input")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CheckTextBoxBasicKeyInputInWindowedPopup)
                TEST_METHOD_PROPERTY(L"Description", L"Validates textbox control simple keyboard input when it is hosted inside a windowed popup")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CheckTextBoxTextChanging)
                TEST_METHOD_PROPERTY(L"Description", L"Validates textbox control TextChanging event")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            END_TEST_METHOD()
    
            BEGIN_TEST_METHOD(TextBoxTextChangingReentrancyCheck)
                TEST_METHOD_PROPERTY(L"Description", L"Validates textbox control TextChanging event re-entrancy call does not crash")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TextBoxTextChangingEventContentChanged)
                TEST_METHOD_PROPERTY(L"Description", L"Validates textbox control TextChanging event's ContentChanged event arg")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(RichEditBoxTextChangingEventContentChanged)
                TEST_METHOD_PROPERTY(L"Description", L"Validates richEditBox control TextChanging event's ContentChanged event arg")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CheckRichEditBoxTextChanging)
                TEST_METHOD_PROPERTY(L"Description", L"Validates richedit control TextChanging event")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(PasswordBoxPasswordChangingEventContentChanged)
                TEST_METHOD_PROPERTY(L"Description", L"Validates PasswordBox control PasswordChanging event's ContentChanged event arg")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CheckPasswordBoxPasswordChanging)
                TEST_METHOD_PROPERTY(L"Description", L"Validates PasswordBox control PasswordChanging event")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CheckTextBoxControlAltKeyInput)
                TEST_METHOD_PROPERTY(L"Description", L"Validates textbox control's ctrl_Arrow keyboard input")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CheckTextBoxURLTabbing)
                TEST_METHOD_PROPERTY(L"Description", L"Validates textbox control's tabbing behavior for inputscope URL")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CheckTextBoxPasteEvent)
                TEST_METHOD_PROPERTY(L"Description", L"Validates textbox control's paste event fired")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CheckTextBoxCopyEvent)
                TEST_METHOD_PROPERTY(L"Description", L"Validates textbox control's copy event fired")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CheckTextBoxCopyEventCanBeHandled)
                TEST_METHOD_PROPERTY(L"Description", L"Validates textbox control's copy event can be handled to prevent copy.")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CheckTextBoxCuttingToClipboardEventCanBeHandled)
                TEST_METHOD_PROPERTY(L"Description", L"Validates textbox control's cutting event can be handled to prevent copy and delete.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CheckTextBoxMaxLength)
                TEST_METHOD_PROPERTY(L"Description", L"Validates textbox control TextChanging event")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(MultiLineTextInput)
                TEST_METHOD_PROPERTY(L"Description", L"Validates keyboard input for MultiLine TextBox")
            END_TEST_METHOD()
            
            BEGIN_TEST_METHOD(CheckEuroSignInput)
                TEST_CLASS_PROPERTY(L"Ignore", L"True")
                TEST_METHOD_PROPERTY(L"Description", L"Validates ctrl-alt-e for euro sign input")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Phone")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CheckMarlettFont)
                TEST_METHOD_PROPERTY(L"Description", L"Validates TextBox rendering of Marlett font")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CheckCtrlAltV)
                TEST_METHOD_PROPERTY(L"Description", L"Validates Ctrl-Alt-V key does not trigger paste")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(IsReadOnlyCheck)
                TEST_METHOD_PROPERTY(L"Description", L"Validates textbox control's IsReadOnly property")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(UndoRedoTextInput)
                TEST_METHOD_PROPERTY(L"Description", L"Validates undo/redo textbox editing using ctrl-z and ctrl-y")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(UndoRedoGrouping)
                TEST_METHOD_PROPERTY(L"Description", L"Validates undo/redo of RichEditBox with grouping")
            END_TEST_METHOD()
                
            BEGIN_TEST_METHOD(HomeAndEndKeyInput)
                TEST_METHOD_PROPERTY(L"Description", L"Validates navigating using Home and End keys")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(InsertKeyInput)
                TEST_METHOD_PROPERTY(L"Description", L"Validates text input using Insert key")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(BackspaceKeyInput)
                TEST_METHOD_PROPERTY(L"Description", L"Validates text input using Backspace key")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(DeleteKeyInput)
                TEST_METHOD_PROPERTY(L"Description", L"Validates text input using Delete key")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(InsertMode)
                TEST_METHOD_PROPERTY(L"Description", L"Validates insert key to toggle insert mode on and off for text input")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(FamilyEmoji)
                TEST_METHOD_PROPERTY(L"Description", L"Validates rendering and deleting family emoji")
                TEST_METHOD_PROPERTY(L"TestPass:MinOSVer", WINDOWS_OS_VERSION_19H1) // DCPP: RS5 Family emoji render incorrectly (Test failure: 4 TextBoxKeyInputTests::FamilyEmoji)
                TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(DetectiveEmoji)
                TEST_METHOD_PROPERTY(L"Description", L"Validates rendering and deleting female detective emoji and female detective emoji with skintone")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
                TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", WINDOWS_OS_VERSION_22H2) // This test is currently failing on 23h2.
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(DisabledFormattingAccelerators)
                TEST_METHOD_PROPERTY(L"Description", L"Validates DisabledFormattingAccelerators for Bold, Italic and Underline")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyCharacterCasing)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that the character casing property works.")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyCharacterCasingWorksWithPaste)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that the character casing property works when pasting into a textbox.")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyCharacterCasingWorksWithPasteInRightToLeftScenarios)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that the character casing property works when pasting into a textbox with right to left flow direction.")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TextBoxNoTextChangingEventForTyping)
                TEST_METHOD_PROPERTY(L"Description", L"Validates there is no TextChanging event fired for format change for normal typing scenario.")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            END_TEST_METHOD()
                
        private:
            bool IsSameColor(::Windows::UI::Color expected, ::Windows::UI::Color actual)
            {
                return (expected.R == actual.R
                    && expected.G == actual.G
                    && expected.B == actual.B
                    && expected.A == actual.A);
            }
            inline Platform::String^ GetResourcesPath() const;

            void EmojiTestHelper(Platform::String^ emojiXamlFile);
            void CheckTextBoxBasicKeyInputHelper(xaml_controls::TextBox^ textBox);
            void CheckTextBoxBasicKeyInputTestHelper();
        };
    } }
} } } }

