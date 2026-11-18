// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Enterprise { namespace RangeSelection {

    class RangeSelectionIntegrationTests : public WEX::TestClass < RangeSelectionIntegrationTests >
    {
    public:
        BEGIN_TEST_CLASS(RangeSelectionIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")

            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"309fb554-f012-4ac9-bcf1-833e4a372493;375cd7bd-e448-4315-b2a1-bc02d75b0c4f")

            TEST_METHOD_PROPERTY(L"Classification", L"Integration")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        //
        // Platform:Any
        //
        BEGIN_TEST_METHOD(ValidateSelectedRangesAfterSelectedIndexChanged)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that SelectedRanges functions properly after the SelectedIndex property has changed")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateSelectedRangesAfterSelectedItemReorder)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that SelectedRanges functions properly after a selected item is clicked and dragged into a different location")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateSelectionAfterMultipleSelectedItemsReorder)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that all selected items stay selected after they are dragged together into a different location")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateKeyboardReorderUpKeepsSelection)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that selected items moved up with the keyboard keep their order and selection")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateKeyboardReorderOfNonContiguousSelection)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that non-contiguous selected items are moved together and stay selected")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateKeyboardReorderInMultipleSelectionMode)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that a multi-item reorder keeps the selection in Multiple selection mode")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateKeyboardReorderInSingleSelectionMode)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that a reorder keeps the selected item selected in Single selection mode")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateKeyboardReorderWithDuplicateItems)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that a reorder selects the moved occurrence of an item that has an equal duplicate")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateReorderWhenReselectHandlerRemovesItemBefore)
            TEST_METHOD_PROPERTY(L"Description", L"Validates selection and focus when a SelectionChanged handler removes an item before the moved items")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateReorderWhenReselectHandlerReplacesItemsSource)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that a reorder does not select old items in a replacement ItemsSource")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateReorderWhenReselectHandlerClearsItems)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that clearing the items during re-selection does not apply stale indexes")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateReorderWhenReselectHandlerSwitchesToSingleSelectionMode)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that re-selection respects a handler switching to Single and choosing another item")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateReorderWhenReselectHandlerSwitchesToNoneSelectionMode)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that re-selection stops when a handler switches to None")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateSelectAll)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that SelectAll functions properly")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateSelectRange)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that SelectRange functions properly")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateDeselectRange)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that DeselectRange functions properly")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CanRangeSelectUnrealizedItemsWithShift)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can select a range with shift for which some items might not be realized.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        //
        // Platform:Desktop
        //

        //
        // Platform:Phone
        //

    private:
        xaml_controls::ListView^ SetupEnvironment(const unsigned int size);
    };

} } } } } }
