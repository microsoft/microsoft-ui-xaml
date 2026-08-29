// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

#include <collection.h>

#include <CommonInputHelper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Platform;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Enterprise { namespace ListView {

    class ListViewIntegrationTests : public WEX::TestClass<ListViewIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(ListViewIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")

            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"0d66400d-d95f-4953-be63-adc1e02548a0;4c9bea11-fb07-433e-bbe1-5b18dafb1b27")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_CLASS_CLEANUP(ClassCleanup)

        TEST_METHOD_CLEANUP(TestCleanup)

        //
        // Platform:Any
        //
        BEGIN_TEST_METHOD(AnimateItemIntoViewWithGamepad)
            TEST_METHOD_PROPERTY(L"Description", L"Navigates through items with the gamepad down and up buttons.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(JumpItemIntoViewWithKeyboard)
            TEST_METHOD_PROPERTY(L"Description", L"Navigates through items with the page down, page up, home and end keyboard keys.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(JumpItemIntoViewWithKeyboardWithRootCanvas)
            TEST_METHOD_PROPERTY(L"Description", L"Navigates through items with the page down, page up, home and end keyboard keys while the root element is a Canvas.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(AnimateZoomableItemIntoView)
            TEST_METHOD_PROPERTY(L"Description", L"Navigates through items with the gamepad down and up buttons, and home and end keyboard keys.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(AnimateItemIntoViewProgrammatically)
            TEST_METHOD_PROPERTY(L"Description", L"Animates through items programmatically using the gamepad to navigate.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(BringItemIntoShrunkViewWithGamepad)
            TEST_METHOD_PROPERTY(L"Description", L"Navigates through items with the gamepad down and up buttons using a shrunk ApplicationView.VisibleBounds rectangle.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(StopInertialManipulationWithGamepad)
            TEST_METHOD_PROPERTY(L"Description", L"Uses the gamepad up button to interrupt and reverse ScrollViewer's inertia.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanOpenEmptyMenuFlyoutWhilePanningListView)
            TEST_METHOD_PROPERTY(L"Description", L"Holds an item to bring up an empty context menu and then tries to pan the list with DManip.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyContainerFromItemAfterScrollIntoView)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that ContainerFromItem works correctly after calling ScrollIntoView for that item.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyTabOrderWithLocalTabNavigation)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that a ListView with TabNavigation=Local tabs through all of the items in sequence and then out.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyTabOrderWithCycleTabNavigation)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that a ListView with TabNavigation=Cycle tabs through all of the items in sequence and then returns to its first item.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyTabOrderWithOnceTabNavigation)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that a ListView with TabNavigation=Once tabs to a single item and then out.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyTabOrderWithNestedButtons)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that a ListView with any TabNavigation tabs and shift-tabs through nested focusable controls in its ItemTemplate.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyTabOrderWithLocalTabNavigationHeaderAndFooter)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that a ListView with a Header, Footer and TabNavigation=Local tabs through the Header, all of the items in sequence, the Footer and then out.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyTabOrderWithCycleTabNavigationHeaderAndFooter)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that a ListView with a Header, Footer and TabNavigation=Cycle tabs through the Header, all of the items in sequence, the Footer and then returns to the Header.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyTabOrderWithOnceTabNavigationHeaderAndFooter)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that a ListView with a Header, Footer and TabNavigation=Once tabs to the Header, a single item, the Footer and then out.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyTabOrderWithVirtualizedItemsAndLocalTabNavigation)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that a ListView with virtualized items and TabNavigation=Local shift-tabs to its last item and tabs to its first item from the outside.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyTabOrderWithVirtualizedItemsAndCycleTabNavigation)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that a ListView with virtualized items and TabNavigation=Cycle shift-tabs to its last item from the outside and then tabs to its first item.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyTabOrderWithGroupedItemsAndLocalTabNavigation)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that a grouped ListView with TabNavigation=Local tabs through all of the group headers and items in sequence and then out.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyTabOrderWithGroupedItemsAndCycleTabNavigation)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that a grouped ListView with TabNavigation=Cycle tabs through all of the group headers and items in sequence and then returns to its first group header.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyTabOrderWithGroupedItemsAndOnceTabNavigation)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that a grouped ListView with TabNavigation=Once tabs to a single item and then out.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyTabOrderWithGroupedItemsLocalTabNavigationHeaderAndFooter)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that a grouped ListView with a Header, Footer and TabNavigation=Local tabs through the Header, all of the group headers and items in sequence, the Footer and then out.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyTabOrderWithGroupedItemsCycleTabNavigationHeaderAndFooter)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that a grouped ListView with a Header, Footer and TabNavigation=Cycle tabs through the Header, all of the group headers and items in sequence, the Footer and then returns to the Header.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyTabOrderWithGroupedItemsOnceTabNavigationHeaderAndFooter)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that a grouped ListView with a Header, Footer and TabNavigation=Once tabs to the Header, a single item, the Footer and then out.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyFocusChangeAfterItemsReset)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that focus cannot be given to a ListView immediately after its Items collection is cleared.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyFocusChangeAfterItemsSourceReset)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that focus cannot be given to a ListView immediately after its ItemsSource collection is cleared.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(BringItemIntoViewWithLargeGlobalScale)
            TEST_METHOD_PROPERTY(L"Description", L"Tabs through ListView items while using a large global scale factor and verifies resulting vertical scroll offset.")
        END_TEST_METHOD()

        //
        // Platform:Desktop
        //

        //
        // Platform:Phone
        //

    private:
        enum VerifyTabOrderFlags : UINT
        {
            Default = 0,
            UseVirtualizingStackPanelAsItemsPanel = 1,
            UseStackPanelAsItemsPanel = 2,
            UseHeaderAndFooter = 4,
            UseGroupedItems = 8,
            UseNestedButtons = 16
        };

        String^ GetResourcesPath() const;
        Platform::Collections::Vector<Platform::Object^>^ GetGroupedData(bool getLargeDataSet = true) const;
        void JumpItemIntoViewWithKeyboard(bool useRootCanvas) const;
        void VerifyTabOrder(xaml_input::KeyboardNavigationMode tabNavigation, VerifyTabOrderFlags flags, Platform::String^ expectedFocusSequence);
        void VerifyFocusChangeAfterListViewReset(bool useItemsSource);
    };

} } } } } }
