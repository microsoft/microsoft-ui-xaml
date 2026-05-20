// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace ItemsStackPanel {

    class ItemsStackPanelIntegrationTests : public WEX::TestClass<ItemsStackPanelIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(ItemsStackPanelIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")

            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"465cba5c-d9c4-40ac-933a-f238efc26016;e7de4cca-1436-4030-80b9-56ef01aa1cae;bdc5c68b-03c1-4217-832c-4c09f99946f4")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        //
        // Platform:Any
        //
        BEGIN_TEST_METHOD(CanInstantiate)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully create an ItemsStackPanel.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanEnterAndLeaveLiveTree)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully add/remove an ItemsStackPanel from the live tree in an ItemsControl.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanStackItemsHorizontally)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that ItemsStackPanels can stack items in a horizontal orientation.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanStackItemsVertically)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that ItemsStackPanels can stack items in a vertical orientation.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CacheIsUpdatedDuringScrolling)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that ItemsStackPanels properly updated their cached elements during scrolling.")
        END_TEST_METHOD()

        //
        // Platform:Desktop
        //

        //
        // Platform:Phone
        //

    private:
        static const int s_itemCount = 3;
        static const int s_itemCountForVirtualization = 9;

    };

} } } } } }
