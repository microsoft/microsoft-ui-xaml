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

        BEGIN_TEST_METHOD(MoveViewMapsSingleItems)
            TEST_METHOD_PROPERTY(L"Description", L"Validates every intermediate view for single-item moves in both directions.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(MoveViewMapsRanges)
            TEST_METHOD_PROPERTY(L"Description", L"Validates intermediate range moves, including overlapping ranges and endpoints.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(MoveViewValidatesRanges)
            TEST_METHOD_PROPERTY(L"Description", L"Rejects invalid Move indices, empty ranges, and inconsistent item counts.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(MoveViewHandlesLargeIndices)
            TEST_METHOD_PROPERTY(L"Description", L"Validates Move projection arithmetic near the vector size limit.")
        END_TEST_METHOD()

    };

} } } } }
