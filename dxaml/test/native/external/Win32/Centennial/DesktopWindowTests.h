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

            BEGIN_TEST_METHOD(ValidateDefaultRedirectionSurface)
                TEST_METHOD_PROPERTY(L"Description", L"Validates the default HWND style and background erasure on either compositor, including after the optional-change state changes.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateOptedInRedirectionSurface)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that the opt-in skips redirection only on the system compositor and preserves creation-time background erasure.")
                TEST_METHOD_PROPERTY(L"Data:XamlOptionalChanges", L"{SkipWindowRedirectionSurface:true}")
            END_TEST_METHOD()
        };
    }
} } } }
