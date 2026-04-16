// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>
#include <CommonInputHelper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Text {

        class TextBoxPointerTests : public WEX::TestClass < TextBoxPointerTests >
        {
        public:
            BEGIN_TEST_CLASS(TextBoxPointerTests)
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

            BEGIN_TEST_METHOD(CheckTextBoxPointerOver)
                TEST_METHOD_PROPERTY(L"Description", L"Validates the textbox control visual state on pointer over.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
                TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")    // DCPP: Unreliable test: Foundation::Text::TextBoxPointerTests::CheckTextBoxPointerOver
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CheckTextBoxFocusWUC)
                TEST_METHOD_PROPERTY(L"Description", L"Validates the textbox control's focus behavior on pointer release and press")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")    // DCPP: TextBoxPointerTests::CheckTextBoxFocusWUC fails even though output shows TextBox got focus
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CheckGamepadInputOnDesktop)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that gamepad input in a textbox won't give bad behavior on desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(PreventKeyboardDisplayOnProgrammaticFocus)
                TEST_METHOD_PROPERTY(L"Description", L"Validates PreventKeyboardDisplayOnProgammaticFocus property")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Phone")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(PreventEditFocusLoss)
                TEST_METHOD_PROPERTY(L"Description", L"Validates PreventEditFocusLoss property")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Phone")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CheckFiresManipulationEvents)
                TEST_METHOD_PROPERTY(L"Description", L"Validates TextBox can fire manipulation events")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP") //not stable on WPF
                TEST_METHOD_PROPERTY(L"Ignore", L"True")
            END_TEST_METHOD()

        private:
            Platform::String^ GetPathToFiles() const;
            void CheckTextBoxFocusInternal();
            void TextBoxPointerTests::CheckInputOnDesktop(InputDevice device);
        };
    } }
} } } }
