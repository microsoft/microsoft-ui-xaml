// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Input { namespace Keyboard {
        class BasicKeyboardTests : public WEX::TestClass<BasicKeyboardTests>
        {
        public:
            BEGIN_TEST_CLASS(BasicKeyboardTests)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")

                TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"32301317-5c46-4350-8af6-a06552076e89;d04573b8-e899-4822-bb72-9f4743c89d36")
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_METHOD_SETUP(TestSetup)
            TEST_METHOD_CLEANUP(TestCleanup)

            BEGIN_TEST_METHOD(BasicKeyDownKeyUp)
                TEST_METHOD_PROPERTY(L"Description", L"Validates KeyDown and KeyUp events.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(KeyEventArgs)
                TEST_METHOD_PROPERTY(L"Description", L"Validates the properties in the KeyRoutedEventArgs and order of KeyDown/KeyUp events.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyDeviceIdEqualsEmptyString)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that DeviceId returns the default value of empty string, while we wait for CoreWindow work to reach our branch.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyKeyStatus)
                TEST_METHOD_PROPERTY(L"Description", L"Validates the properties in the KeyRoutedEventArgs and order of KeyDown/KeyUp events.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyIsMenuKeyDown)
                TEST_METHOD_PROPERTY(L"Ignore", L"True") //Enable when 7209577 is resolved
                TEST_METHOD_PROPERTY(L"Description", L"Validates the IsMenuKeyDown property is turned on correctly")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyCtrlAltKeys)
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP") // ALT key is eaten by WPF
                TEST_METHOD_PROPERTY(L"Description", L"Validates Ctrl-Alt+e and verify keydown-up events correctly")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(EnterKeyDownKeyUpOnButton)
                TEST_METHOD_PROPERTY(L"Description", L"Validates key down and up for enter key press on button")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(EnterKeyDownKeyUpOnTextBox)
                TEST_METHOD_PROPERTY(L"Description", L"Validates key down and up for enter key press on textbox")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(F10KeyDown)
                TEST_METHOD_PROPERTY(L"Description", L"Validates KeyDown and KeyUp events for F10")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_METHOD()

        private:
            void EnterKeyDownKeyUpHelper(xaml_controls::Control^ element);
        };

    } } }
} } } }
