// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Sample {
        class SampleCustomTypeTests : public WEX::TestClass<SampleCustomTypeTests>
        {
        public:
            BEGIN_TEST_CLASS(SampleCustomTypeTests)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")

                TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"dad50b0b-4b6e-4b44-ae60-31e745fd8f74;2848f544-9be5-4b98-bd69-bf3f2a4a04f3")
            END_TEST_CLASS()
            
            TEST_METHOD_SETUP(TestSetup)
            TEST_METHOD_CLEANUP(TestCleanup)
            TEST_CLASS_SETUP(ClassSetup)

            BEGIN_TEST_METHOD(CreateCustomButtonFromCodeBehind)
                TEST_METHOD_PROPERTY(L"Description", L"Creates a custom button from code behind.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CreateCustomButtonFromXaml)
                TEST_METHOD_PROPERTY(L"Description", L"Creates a custom button from XAML.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CreateCustomPage)
                TEST_METHOD_PROPERTY(L"Description", L"Instantiates our custom page object.")
            END_TEST_METHOD()

        };
    }
} } } }
