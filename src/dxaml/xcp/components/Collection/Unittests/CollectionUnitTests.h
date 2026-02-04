// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace Collection {

    class CollectionUnitTests
    {
    public:
        BEGIN_TEST_CLASS(CollectionUnitTests)
        END_TEST_CLASS()

        BEGIN_TEST_METHOD(ValidateCCollectionReserve)
            TEST_METHOD_PROPERTY(L"Classification", L"Integration")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Description", L"Validates that CCollection::Reserve works as expected.")
        END_TEST_METHOD()


    };

} } } } }
