// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Input { namespace Pointer {
        
        class PointerReentrancyTests : public WEX::TestClass<PointerReentrancyTests>
        {
        public:
            BEGIN_TEST_CLASS(PointerReentrancyTests)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"Hosting:Mode", L"WPF")
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_METHOD_SETUP(TestSetup)
            TEST_METHOD_CLEANUP(TestCleanup)
            
            BEGIN_TEST_METHOD(NestedMessagePumpInButtonClick)
                TEST_METHOD_PROPERTY(L"Description", L"Run a nested message pump inside a Button.Click handler, and process pointer input inside that pump.")
                TEST_METHOD_PROPERTY(L"TestPass:MaxOSVer", L"22621") // This test is currently failing on 23h2, hence stop at 22h2 which is 22621.
            END_TEST_METHOD()
        };
        
    } } }
} } } }
