// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>
#include <CommonInputHelper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace AccessKeys {
        class BasicAccessKeysTests : public WEX::TestClass<BasicAccessKeysTests>
        {
        public:
            BEGIN_TEST_CLASS(BasicAccessKeysTests)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"1bb20c90-a558-491b-b76d-55bdb9a46911;57e0de30-efb3-4001-9ccc-b38032fd1974;bdc5c68b-03c1-4217-832c-4c09f99946f4")
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_METHOD_SETUP(TestSetup)
            TEST_METHOD_CLEANUP(TestCleanup)

            BEGIN_TEST_METHOD(AccessKeyNavigation)
                TEST_METHOD_PROPERTY(L"Description", L"Validates enter and exit of access key mode.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(EnterExitAccessKeyModeUsingKeyInput)
                TEST_METHOD_PROPERTY(L"Description", L"Validates the entering, exiting and invoking access key mode behavior.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(EnterAccessKeyModeUsingEnterDisplayModeForXamlRoot)
                TEST_METHOD_PROPERTY(L"Description", L"Validates the entering access key mode behavior via EnterDisplayMode")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF") 
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(AccessKeyInvokedEvent)
                TEST_METHOD_PROPERTY(L"Description", L"Validates basic AccessKeyInvoked event handling.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TextElementAccessKeyInvokedEvent)
                TEST_METHOD_PROPERTY(L"Description", L"Validates basic AccessKeyInvoked event handling for Text Element.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TextElementInRichTextBlockAccessKeyInvokedEvent)
                TEST_METHOD_PROPERTY(L"Description", L"Validates basic AccessKeyInvoked event handling for Text Elements in RichTextBlocks and Paragraphs.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TextElementInSpanAccessKeyInvokedEvent)
                TEST_METHOD_PROPERTY(L"Description", L"Validates basic AccessKeyInvoked event handling for Text Elements in Spans.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(AccessKeyShownHiddenEvent)
                TEST_METHOD_PROPERTY(L"Description", L"Validates basic AccessKeyShown and AccessKeyHidden event handling.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TextElementAccessKeyShownHiddenEvent)
                TEST_METHOD_PROPERTY(L"Description", L"Validates basic AccessKeyShown and AccessKeyHidden event handling for Text Element.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(RichTextBlockTextElementAccessKeyShownHiddenEvent)
                TEST_METHOD_PROPERTY(L"Description", L"Validates basic AccessKeyShown and AccessKeyHidden event handling for Text Elements in RichTextBlocks.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(AccessKeyPressed)
                TEST_METHOD_PROPERTY(L"Description", L"Validates basic pressing access key invocation scenario.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(AccessKeyPressedOnHiddenOrDeletedControl)
                TEST_METHOD_PROPERTY(L"Description", L"Validates basic pressing access key invocation on collapsed or deleted control scenario.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(MultipleAccessKeysPressed)
                TEST_METHOD_PROPERTY(L"Description", L"Validates multiple key stroks for access key invocation scenario.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(LowerCaseAccessKeyPressed)
                TEST_METHOD_PROPERTY(L"Description", L"Validates lower case access key invocation scenario.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(UpperCaseAccessKeyPressed)
                TEST_METHOD_PROPERTY(L"Description", L"Validates upper case access key invocation scenario.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TwoCharAccessKeysPressed)
                TEST_METHOD_PROPERTY(L"Description", L"Validates two character access keys invocation scenario.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ThreeCharAccessKeysPressed)
                TEST_METHOD_PROPERTY(L"Description", L"Validates three character access keys invocation scenario.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(UnicodeCharAccessKey)
                TEST_METHOD_PROPERTY(L"Description", L"Validates unicode access key invocation scenario.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(NullAccessKeyNotMatching)
                TEST_METHOD_PROPERTY(L"Description", L"Validates null access key scenario.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(DuplicateKeys)
                TEST_METHOD_PROPERTY(L"Description", L"Validates same access key specified for two elements.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ScopeOwners)
                TEST_METHOD_PROPERTY(L"Description", L"Test AccessKeys aren't invoked if they're not part of the root scope")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(BasicHotKey)
                TEST_METHOD_PROPERTY(L"Description", L"Validates basic hot key invocation.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(SettingAccessKeyOverridesFEAPAccessKey)
                TEST_METHOD_PROPERTY(L"Description", L"When we set FrameworkElement.AccessKey and FEAP.AccessKeys does not have a value, ensure that the value is mapped")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(SettingAccessKeyDoesNotOverrideWhenSetOnFEAP)
                TEST_METHOD_PROPERTY(L"Description", L"When we set FrameworkElement.AccessKey and FEAP.AccessKeys does has a value, use the value on AP")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(WindowMoveEndsAKSequence)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that Access Key sequence terminates on Window move")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP") // crash due to window move
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(WindowResizeEndsAKSequence)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that Access Key sequence terminates on Window resize")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP") // crash due to window move
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(AltKeyCodesDoNotFireAccessKeys)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that Alt-Numeric codes do not invoke access keys")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop") //Or OneCore
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

        private:
            void BasicAccessKeysTests::VerifyEnterExitAccessKeyModeUsingKeyInput();
        };
    }
} } } }
