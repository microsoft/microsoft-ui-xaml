// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace ListBox {

    class ListBoxIntegrationTests : public WEX::TestClass<ListBoxIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(ListBoxIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")

            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"465cba5c-d9c4-40ac-933a-f238efc26016;a69ddfa4-5142-4bed-887d-6d0ca14a3473;b34da8d2-333d-40a9-a19c-94b1f9785580")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        //
        // Platform:Any
        //
        BEGIN_TEST_METHOD(CanInstantiate)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully create a ListBox.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanEnterAndLeaveLiveTree)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully add/remove a ListBox from the live tree.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DefaultContainerIsListBoxItem)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that ListBoxes are populated with ListBoxItem elements by default.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VirtualizesWithEnoughItems)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that ListBoxes do UI virtualization after a sufficient number of items added to their collection.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanScrollItemIntoView)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that ScrollIntoView works properly.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ScrollingPreservesVirtualization)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that calling ScrollIntoView to progressively show all of a virtualized list of items causes items no longer in view to be recycled.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateUIElementTree)
            TEST_METHOD_PROPERTY(L"Ignore", L"True")
            TEST_METHOD_PROPERTY(L"Description", L"Validates the UI element tree of ListBox in various visual states.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyListBoxItemFocus)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that tapping on a ListBoxItem gives it Pointer Focus.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateFootprint)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the ActualWidth and ActualHeight of ListBox.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateCtrlAScenariosWithVirtualization)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully invoke Ctrl+A scenarios to SelectAll then DeselectAll with virtualization.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateCtrlAScenariosWithoutVirtualization)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully invoke Ctrl+A scenarios to SelectAll then DeselectAll without virtualization.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateListBoxItemVisibilityChanged)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully change the visibility of the ListBoxItem.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateListBoxItemRightTapped)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully react to a RightTap on the ListBoxItem.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanNavigateToAndRemainOnLastItem)
            TEST_METHOD_PROPERTY(L"Description", L"Validates ability to navigate to last item and remain there with the keyboard.")
        END_TEST_METHOD()

        //
        // Platform:Desktop
        //
        BEGIN_TEST_METHOD(CanSelectSingleItemsOnDesktop)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully select single items in the ListBox on desktop.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanSelectMultipleItemsOnDesktop)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully select multiple items in the ListBox on desktop.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanExtendSelection)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully extend the number of selected items in the ListBox on desktop.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        END_TEST_METHOD()

        //
        // Platform:Phone
        //
        BEGIN_TEST_METHOD(CanSelectSingleItemsOnPhone)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully select single items in the ListBox on phone.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Phone")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanSelectMultipleItemsOnPhone)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully select multiple items in the ListBox on phone.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Phone")
        END_TEST_METHOD()

    private:
        void VerifyCanSelectSingleItems(bool doTap);
        void VerifyCanSelectMultipleItems(bool doTap);

        void SelectItemAtIndex(xaml_controls::ListBox^ listBox, int index, bool doTap);
        void ValidateCtrlAScenarios(const int itemCount);

        // The ListBox by default employs UI virtualization - with a control size of 200x200 and a standard TextBlock size,
        // we can load around 10-20 items into the ListBox before it starts virtualizing and stops loading UI elements into its panel.
        // So, in order to test both the basic case and the case where it starts virtualizing, we'll define item counts that are both
        // significantly smaller and significantly larger than this range.
        static const int s_itemCountWithoutVirtualization = 7;
        static const int s_itemCountWithVirtualization = 50;

    };

} } } } } }

