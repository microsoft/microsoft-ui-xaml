// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Text {
        class InputScopeTests : public WEX::TestClass < InputScopeTests >
        {
        public:
            struct InputScopeTestStruct
            {
                Xaml::Input::InputScopeNameValue XAML_IS; // XAML enum definition
                Platform::String^ Scopename;
                InputScope IS; // Mapped IS value
            };

            BEGIN_TEST_CLASS(InputScopeTests)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"HelixWorkItemCreation", L"CreateWorkItemPerTestClass")
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_CLASS_CLEANUP(ClassCleanup)
            TEST_METHOD_SETUP(TestSetup)
            TEST_METHOD_CLEANUP(TestCleanup)

            BEGIN_TEST_METHOD(InputScopePropertyTest)
                TEST_METHOD_PROPERTY(L"Description", L"Validates setting/getting the InputScope property for TextBox and RichEditBox control.")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            END_TEST_METHOD()

        private:
            Platform::String^ GetPathToFiles() const;
        };
    } }
} } } }
