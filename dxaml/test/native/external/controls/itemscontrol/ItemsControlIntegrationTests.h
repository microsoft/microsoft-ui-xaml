// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace ItemsControl {

    class ItemsControlIntegrationTests : public WEX::TestClass<ItemsControlIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(ItemsControlIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")

            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"465cba5c-d9c4-40ac-933a-f238efc26016;e7de4cca-1436-4030-80b9-56ef01aa1cae;b34da8d2-333d-40a9-a19c-94b1f9785580")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        //
        // Platform:Any
        //
        BEGIN_TEST_METHOD(CanInstantiate)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully create an ItemsControl.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanEnterAndLeaveLiveTree)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully add/remove an ItemsControl from the live tree.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanAddItemsDirectly)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully add items to the ItemsControl's Items collection.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanAddItemsFromSource)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully add items to the ItemsControl via its ItemsSource property.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanGetContainersFromItems)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully get containers from the items they hold.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanGetContainersFromIndexes)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully add containers from an expected index.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanGetItemsFromContainers)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully get items from the containers that hold them.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanGetIndexesFromContainers)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully get indexes from the containers at those indexes.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanLayoutWithoutItemContainerGenerator)
            TEST_METHOD_PROPERTY(L"Description", L"Validates ability to lay out a GridView control without the presence of an IItemContainerGenerator yet.")
        END_TEST_METHOD()

        //
        // Platform:Desktop
        //

        //
        // Platform:Phone
        //

    private:
        static const int s_itemCount = 5;

    };

} } } } } }
