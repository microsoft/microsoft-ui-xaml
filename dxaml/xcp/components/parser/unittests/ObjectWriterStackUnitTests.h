// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>
#include <ObjectWriterFrame.h>

class ParserErrorReporter;  

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Framework {

        class ObjectWriterStackUnitTests : public WEX::TestClass<ObjectWriterStackUnitTests>
        {
        public:
            BEGIN_TEST_CLASS(ObjectWriterStackUnitTests)
                TEST_METHOD_PROPERTY(L"Classification", L"Integration")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_CLASS()

            TEST_METHOD(VerifyPushPopCopy)
            TEST_METHOD(VerifyUpdateIterators)
            TEST_METHOD(VerifyFindNamespaceByPrefix)

        private:
            void VerifyFramesAreEqual(const ObjectWriterFrame& frame1, const ObjectWriterFrame& frame2);
        };
    }
} } } }
