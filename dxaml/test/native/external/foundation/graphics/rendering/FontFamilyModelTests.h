// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>
#include <WUCRenderingScopeGuard.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Graphics {

        class FontFamilyModelTests : public WEX::TestClass<FontFamilyModelTests>
        {
        public:
            BEGIN_TEST_CLASS(FontFamilyModelTests)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"TestPass:ExcludeOn", L"Santorini")
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_METHOD_SETUP(TestSetup)

            TEST_METHOD_CLEANUP(TestCleanup)

            BEGIN_TEST_METHOD(ValidateVariableFontUsage)
                TEST_METHOD_PROPERTY(L"Description", L"Validate the use of variable fonts.")
                TEST_METHOD_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
                TEST_METHOD_PROPERTY(L"TestPass:MinOSVer", WINDOWS_OS_VERSION_19H1) // Segoe UI Variable doesn't exist on RS5
            END_TEST_METHOD()

        };

    } }
} } } }
