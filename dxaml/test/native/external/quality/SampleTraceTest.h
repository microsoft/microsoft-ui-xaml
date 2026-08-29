// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Quality  {
        class SampleTraceTest : public WEX::TestClass<SampleTraceTest>
        {
        public:

            BEGIN_TEST_CLASS(SampleTraceTest)
                TEST_CLASS_PROPERTY(L"IsolationLevel", L"Class")
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            
            TEST_METHOD_CLEANUP(TestCleanup)

            BEGIN_TEST_METHOD(TestMethod)
                TEST_METHOD_PROPERTY(L"RunAs", L"UAP")
                TEST_METHOD_PROPERTY(L"Description", L"Validates that we can verify traces.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            END_TEST_METHOD()

        private:
            int testMethodExecutedCount = 0;
        };
    }
} } } }
