// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Centennial {
        class WPFHostTests : public WEX::TestClass<WPFHostTests>
        {
        public:
            BEGIN_TEST_CLASS(WPFHostTests)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"IsolationLevel", L"Method")
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
                TEST_CLASS_PROPERTY(L"UAP:Host", L"PackagedCwa")
                TEST_CLASS_PROPERTY(L"UAP:Praid", L"XamlNativeTAEFTests")
                TEST_CLASS_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_CLASS_PROPERTY(L"ThreadingModel", L"STA")
                TEST_CLASS_PROPERTY(L"UAP:AppXManifest", APPXMANIFEST_WINDOWS_VERSION_CURRENT_CENTENNIAL)
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_METHOD_SETUP(TestSetup)
            TEST_METHOD_CLEANUP(TestCleanup)

            BEGIN_TEST_METHOD(Canary)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that we are able to start the CLR")  
            END_TEST_METHOD()
        };
    }
} } } }
