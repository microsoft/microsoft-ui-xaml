// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Test {
        class ErrorHandlingTests : public WEX::TestClass<ErrorHandlingTests>
        {
        public:
            BEGIN_TEST_CLASS(ErrorHandlingTests)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")

                TEST_CLASS_PROPERTY(L"Classification", L"IntentionallyFailing")
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_METHOD_CLEANUP(TestCleanup)

            BEGIN_TEST_METHOD(CreateAnIFCFailureAPICall)
                TEST_METHOD_PROPERTY(L"Ignore", L"True")
                TEST_METHOD_PROPERTY(L"Description", L"Calls an API that results in an IFC failure. A full stack trace should be printed and a minidump should be produced.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CreateANullReferenceException)
                TEST_METHOD_PROPERTY(L"Ignore", L"True")
                TEST_METHOD_PROPERTY(L"Description", L"Dereferences a null pointer.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

        };
    }
} } } }
