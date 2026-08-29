// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>

#include <LiveReorderHelper.h>

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace LiveReorderHelper {

    class LiveReorderHelperUnitTests : public WEX::TestClass<LiveReorderHelperUnitTests>
    {
    public:
        BEGIN_TEST_CLASS(LiveReorderHelperUnitTests)
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        END_TEST_CLASS()

        BEGIN_TEST_METHOD(ValidateUpdateMovedItemsWithOneElement)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the update function does not crash with item pointing to itself.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateUpdateMovedItemsWithFixedSizedItems)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the update function of the MovedItems with fixed sized items.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateUpdateMovedItemsWithVariableSizedItems)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the update function of the MovedItems with variable sized items.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateUpdateMovedItemsWithGridViewScenario)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the update function of the MovedItems with a GridView scenario.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateUpdateMovedItemsWithOutsideDragWithFixedSizedItems)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the update function of the MovedItems when dragging from outside the List with fixed sized items.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateUpdateMovedItemsWithOutsideDragWithVariableSizedItems)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the update function of the MovedItems when dragging from outside the List with variable sized items.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateGetDragOverIndex)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the GetDragOverIndex function returns the right index.")
        END_TEST_METHOD()

    private:
    };

} } } } }
