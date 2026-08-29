// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>

namespace Windows { namespace UI { namespace Xaml { namespace Tests { 

    namespace Associative {

        class AssociativeUnitTests : public WEX::TestClass<AssociativeUnitTests>
        {
        public:
            BEGIN_TEST_CLASS(AssociativeUnitTests)
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)

            TEST_METHOD(LocalStorage_Allocations)

            TEST_METHOD(LocalStorage_CtorsAndDtors)

            TEST_METHOD(LocalStorage_Preallocation)

            TEST_METHOD(LocalStorage_Exceptions)

            TEST_METHOD(LocalStorageStressTests_8)

            TEST_METHOD(LocalStorageStressTests_16)
        };

    }

} } } }
