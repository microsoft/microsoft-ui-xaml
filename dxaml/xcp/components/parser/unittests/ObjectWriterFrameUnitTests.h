// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>

class ParserErrorReporter;  

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Framework {

        class ObjectWriterFrameUnitTests : public WEX::TestClass<ObjectWriterFrameUnitTests>
        {
        public:
            BEGIN_TEST_CLASS(ObjectWriterFrameUnitTests)
                TEST_METHOD_PROPERTY(L"Classification", L"Integration")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_CLASS()

            TEST_METHOD(VerifyFindNamespaceByPrefix)
            TEST_METHOD(VerifyDirectives)
            TEST_METHOD(VerifyType)
            TEST_METHOD(VerifyMember)
            TEST_METHOD(VerifyCollection)
            TEST_METHOD(VerifyInstance)
            TEST_METHOD(VerifyValue)
            TEST_METHOD(VerifyWeakRef)
            TEST_METHOD(VerifyAssignedProperties)
            TEST_METHOD(VerifyReplacementPropertyValues)
            TEST_METHOD(VerifyCompressedStack)
        };
    }
} } } }
