// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>
#include <collection.h>
#include <CommonInputHelper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Platform;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Enterprise { namespace ItemsControl {

    class ItemsControlIntegrationTests : public WEX::TestClass<ItemsControlIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(ItemsControlIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")

            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"0d66400d-d95f-4953-be63-adc1e02548a0;4c9bea11-fb07-433e-bbe1-5b18dafb1b27")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_CLASS_CLEANUP(ClassCleanup)

        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(VerifyTabOrderWithLocalTabNavigation)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that an ItemsControl with TabNavigation=Local tabs through all of the items in sequence and then out.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyTabOrderWithCycleTabNavigation)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that an ItemsControl with TabNavigation=Cycle tabs through all of the items in sequence and then returns to its first item.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyTabOrderWithOnceTabNavigation)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that an ItemsControl with TabNavigation=Once tabs to a single item and then out.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyTabOrderWithLocalTabNavigationHeaderAndFooter)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that an ItemsControl with an ItemsPresenter.Header, ItemsPresenter.Footer and TabNavigation=Local tabs through the header, all of the items in sequence, the footer and then out.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyTabOrderWithCycleTabNavigationHeaderAndFooter)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that an ItemsControl with an ItemsPresenter.Header, ItemsPresenter.Footer and TabNavigation=Cycle tabs through the header, all of the items in sequence, the footer and then returns to the header.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyTabOrderWithOnceTabNavigationHeaderAndFooter)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that an ItemsControl with an ItemsPresenter.Header, ItemsPresenter.Footer and TabNavigation=Once tabs to the header, a single item, the footer and then out.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyTabOrderWithVirtualizedItemsAndLocalTabNavigation)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that an ItemsControl with virtualized items and TabNavigation=Local shift-tabs to its last item and tabs to its first item from the outside.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyTabOrderWithVirtualizedItemsAndCycleTabNavigation)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that an ItemsControl with virtualized items and TabNavigation=Cycle shift-tabs to its last item from the outside and then tabs to its first item.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyTabOrderWithGroupedItemsAndLocalTabNavigation)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that a grouped ItemsControl with TabNavigation=Local tabs through all first group items and then out.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyTabOrderWithGroupedItemsAndCycleTabNavigation)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that a grouped ItemsControl with TabNavigation=Cycle tabs through all first group items and then returns to its first group item.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyTabOrderWithGroupedItemsAndOnceTabNavigation)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that a grouped ItemsControl with TabNavigation=Once tabs to a single item and then out.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyTabOrderWithGroupedItemsLocalTabNavigationHeaderAndFooter)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that a grouped ItemsControl with an ItemsPresenter.Header, ItemsPresenter.Footer and TabNavigation=Local tabs through the header, all first group items and then out.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyTabOrderWithGroupedItemsCycleTabNavigationHeaderAndFooter)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that a grouped ItemsControl with an ItemsPresenter.Header, ItemsPresenter.Footer and TabNavigation=Cycle tabs through the header, all first group items and then returns to its first group item.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyTabOrderWithGroupedItemsOnceTabNavigationHeaderAndFooter)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that a grouped ItemsControl with an ItemsPresenter.Header, ItemsPresenter.Footer and TabNavigation=Once tabs to the header, a single item, the footer and then out.")
        END_TEST_METHOD()

    private:
        enum VerifyTabOrderFlags : UINT
        {
            Default = 0,
            UseVirtualizingStackPanelAsItemsPanel = 1,
            UseStackPanelAsItemsPanel = 2,
            UseHeaderAndFooter = 4,
            UseGroupedItems = 8
        };

        String^ GetResourcesPath() const;
        Platform::Collections::Vector<Platform::Object^>^ GetGroupedData() const;
        void VerifyTabOrder(xaml_input::KeyboardNavigationMode tabNavigation, VerifyTabOrderFlags flags, Platform::String^ expectedFocusSequence);
    };

} } } } } }
