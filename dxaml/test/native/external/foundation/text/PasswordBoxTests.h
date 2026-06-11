// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Text {
        class PasswordBoxTests : public WEX::TestClass <PasswordBoxTests>
        {
        public:

            enum PasswordBoxTestSetting
            {
                RevealModePeek,
                RevealModeHidden,
                RevealModeVisible,
                ButtonEnabled,
                ButtonDisabled
            };

            BEGIN_TEST_CLASS(PasswordBoxTests)
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

            BEGIN_TEST_METHOD(PasswordBoxTestsPeek)
                TEST_METHOD_PROPERTY(L"Description", L"Validates when PasswordBox's PasswordRevealMode property is set to Peek")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(PasswordBoxTestsHidden)
                TEST_METHOD_PROPERTY(L"Description", L"Validates when PasswordBox's PasswordRevealMode property is set to Hidden")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(PasswordBoxTestsVisible)
                TEST_METHOD_PROPERTY(L"Description", L"Validates when PasswordBox's PasswordRevealMode property is set to Visible")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(PasswordBoxTestsButtonEnabled)
                TEST_METHOD_PROPERTY(L"Description", L"Validates when PasswordBox's IsPasswordRevealButtonEnabled property is set to True")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(PasswordBoxTestsButtonDisabled)
                TEST_METHOD_PROPERTY(L"Description", L"Validates when PasswordBox's IsPasswordRevealButtonEnabled property is set to False")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(PasswordBoxTestsFocusChange)
                TEST_METHOD_PROPERTY(L"Description", L"Validates when PasswordBox's behavior when focus is changed")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(PasswordBoxTestsModeChange)
                TEST_METHOD_PROPERTY(L"Description", L"Validates when PasswordBox's behavior of programmaticly changing the PasswordRevelMode")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")

#ifndef MUX_PRERELEASE
 // Test fails when MUXFinalRelease=true is passed as build parameter 
                TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") 
#endif
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(PasswordBoxTestsInputScope)
                TEST_METHOD_PROPERTY(L"Description", L"Validates PasswordBox's behavior of InputScope property")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(PasswordBoxTestsNoContextMenuOnButtonClick)
                TEST_METHOD_PROPERTY(L"Description", L"Validates context menu does not popup when PasswordBox's reveal button is right clicked")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(EnsureNoRevealButtonOnXBox)
                TEST_METHOD_PROPERTY(L"Description", L"Reveal button should never be visible on xbox")
                TEST_METHOD_PROPERTY(L"IsolationLevel", L"Method")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CheckFiresHoldingEvent)
                TEST_METHOD_PROPERTY(L"Description", L"Validates PasswordBox can fire OnHolding event")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CheckFiresManipulationEvents)
                TEST_METHOD_PROPERTY(L"Description", L"Validates PasswordBox can fire manipulation events")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP") //not stable on WPF
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidatePasswordCharValidation)
                TEST_METHOD_PROPERTY(L"Description", L"Validates PasswordBox correctly validates passwords")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateRevealButtonToggle)
                TEST_METHOD_PROPERTY(L"Description", L"Validates PasswordBox toggle pattern invoke")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateKeyboardShortCutAndValuePattern)
                TEST_METHOD_PROPERTY(L"Description", L"Validates alt-f8 shortcut key toggle behavior and value pattern")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidatePasswordTextPatternRevealed)
                TEST_METHOD_PROPERTY(L"Description", L"Validates PasswordBox support of TextPattern when password is revealed")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidatePasswordTextPatternHidden)
                TEST_METHOD_PROPERTY(L"Description", L"Validates PasswordBox support of TextPattern when password is not revealed")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateNoRevealButton)
                TEST_METHOD_PROPERTY(L"Description", L"Validates PasswordBox templated to remove reveal pattern")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateCorrectAcceleratorKeyMessageForRevealButtonValues)
                TEST_METHOD_PROPERTY(L"Description", L"Validates the correct accelerator key message is delivered by the PasswordBox for different values of PasswordRevealMode.")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateRevealButtonAKMessageDoesNotOverwritePreviousMessage)
                TEST_METHOD_PROPERTY(L"Description", L"Validates PasswordRevealMode AcceleratorKey messages do not overwrite previous messages.")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateCtrlDeleteBehavior)
                TEST_METHOD_PROPERTY(L"Description", L"Validates PasswordBox's behavior on Ctrl+Delete input.")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidatePasswordBinding)
                TEST_METHOD_PROPERTY(L"Description", L"Validates PasswordBox's behavior for binding on password Text.")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidatePasswordBoxPlaceholderVisibility)
                TEST_METHOD_PROPERTY(L"Description", L"Validates PasswordBox's placeholder text visibility.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidatePasswordBoxPlaceholderVisibility_Backspace)
                TEST_METHOD_PROPERTY(L"Description", L"Validates PasswordBox's placeholder text visibility after clearing password through backspace.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidatePasswordBoxPlaceholderVisibility_ClearValue)
                TEST_METHOD_PROPERTY(L"Description", L"Validates PasswordBox's placeholder text visibility after calling ClearValue.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidatePasswordBoxPlaceholderVisibility_EmptyString)
                TEST_METHOD_PROPERTY(L"Description", L"Validates PasswordBox's placeholder text visibility after settings the password to an empty string.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidatePasteFromClipboard)
                TEST_METHOD_PROPERTY(L"Description", L"Validates PasswordBox's PasteFromClipboard API.")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateCanPasteClipboardContent)
                TEST_METHOD_PROPERTY(L"Description", L"Validates PasswordBox's CanPasteClipboardContent API.")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            END_TEST_METHOD()

        private:
            enum class PasswordClearMethod
            {
                Backspace,
                ClearValue,
                EmptyString
            };

            Platform::String^ GetPathToFiles() const;
            void PasswordBoxTestHelper(PasswordBoxTestSetting testSetting, bool peekButtonVisible);
            Platform::String^ GetEtwFilterString( Platform::String^ field, unsigned int inputScope);
            void PasswordBoxTestsFocusChangeHelper();
            void PasswordBoxTestsModeChangeHelper();
            void VerifyPasswordUsingUIAValuePattern(xaml_controls::PasswordBox^ passwordBox, LPCWSTR expectedPasswordValue);
            void ValidatePasswordTextPatternTestHelper(bool passwordRevealed);
            void ValidatePasswordSetAndGet(const WCHAR* setString, xaml_controls::PasswordBox^ passwordBox, xaml_controls::TextBlock^ bindingTextBlock);
            void ValidatePasswordBoxPlaceholderVisibilityHelper(PasswordBoxTests::PasswordClearMethod clearMethod);
        };
    } }
} } } }
