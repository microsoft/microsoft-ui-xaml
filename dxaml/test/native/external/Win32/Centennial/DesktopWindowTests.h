// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace DesktopWindow {
        class DesktopWindowTests : public WEX::TestClass<DesktopWindowTests>
        {
        public:
            BEGIN_TEST_CLASS(DesktopWindowTests)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"IsolationLevel", L"Method")
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
                TEST_CLASS_PROPERTY(L"Hosting:Mode", L"WPF")
                TEST_CLASS_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_CLASS_PROPERTY(L"ThreadingModel", L"STA")
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_METHOD_SETUP(TestSetup)
            TEST_METHOD_CLEANUP(TestCleanup)
           
            BEGIN_TEST_METHOD(ValidateDesktopWindowLifeTime)
                TEST_METHOD_PROPERTY(L"Description", L"Validates DesktopWindow life time.")
            END_TEST_METHOD()
        };
    }
} } } }
