// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

#include <RuntimeEnabledFeatureOverride.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace ListViewBaseHeaderItem {

    class IntegrationTests : public WEX::TestClass < IntegrationTests >
    {
    public:
        BEGIN_TEST_CLASS(IntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")

            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"1bb20c90-a558-491b-b76d-55bdb9a46911;eadeac67-1552-4876-a67e-dc1fcb8a6e25")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_CLASS_CLEANUP(ClassCleanup)
        TEST_METHOD_SETUP(TestSetup)

        TEST_METHOD_CLEANUP(TestCleanup)

        //
        // Platform:Any
        //
        BEGIN_TEST_METHOD(CanInstantiate)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully create a ListViewHeaderItem and GridViewHeaderItem.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanEnterAndLeaveLiveTree)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully add/remove a ListViewHeaderItem and GridViewHeaderItem from the live tree.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VisualStyleListViewHeaderItem)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the ListViewHeaderItem uses the new visual styles correctly.")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VisualStyleGridViewHeaderItem)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the GridViewHeaderItem uses the new visual styles correctly.")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        //
        // Platform:Desktop
        //

        BEGIN_TEST_METHOD(CanGotoPointerOverVisualState)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the ListViewHeaderItem can go to PointerOver visual state.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"Ignore", L"True")
        END_TEST_METHOD()

    };

} } } } } }

