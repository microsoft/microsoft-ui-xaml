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

        BEGIN_TEST_METHOD(ValidateSelectedRangesAfterItemsChange)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that SelectedRanges follows the selected items when items are inserted or removed before or between them")
        END_TEST_METHOD()
        BEGIN_TEST_METHOD(ValidateKeyboardReorderDownKeepsSelection)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that selected items moved down with the keyboard keep their order and selection")
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

        BEGIN_TEST_METHOD(ValidateKeyboardReorderOfUnselectedFocusedItem)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that reordering an unselected focused item moves only that item and keeps the selection of the others")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateKeyboardReorderWithDuplicateItems)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that a reorder selects the moved occurrence of an item that has an equal duplicate")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateReorderWhenReselectHandlerRemovesItemBefore)
            TEST_METHOD_PROPERTY(L"Description", L"Validates a reorder when a SelectionChanged handler removes an item before the moved items while they are re-selected")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateReorderWhenReselectHandlerRemovesItemAfter)
            TEST_METHOD_PROPERTY(L"Description", L"Validates a reorder when a SelectionChanged handler removes an item after the moved items while they are re-selected")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateReorderWhenReselectHandlerRemovesMovedItem)
            TEST_METHOD_PROPERTY(L"Description", L"Validates a reorder when a SelectionChanged handler removes a moved item before it is re-selected")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateReorderWhenReselectHandlerInsertsItem)
            TEST_METHOD_PROPERTY(L"Description", L"Validates a reorder when a SelectionChanged handler inserts an item while the moved items are re-selected")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateReorderWhenReselectHandlerInsertsItemWithDuplicates)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the moved occurrence of a duplicate item is re-selected when a SelectionChanged handler inserts an item")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateReorderWhenReselectHandlerReplacesItemsSource)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that a reorder stops re-selecting when a SelectionChanged handler replaces ItemsSource")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateReorderWhenReselectHandlerClearsItems)
            TEST_METHOD_PROPERTY(L"Description", L"Validates a reorder when a SelectionChanged handler clears the items while the moved items are re-selected")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateReorderWhenReselectHandlerSwitchesToSingleSelectionMode)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that a reorder keeps the selection made by a SelectionChanged handler that switches to Single selection mode")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateReorderWhenReselectHandlerSwitchesToNoneSelectionMode)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that a reorder selects nothing after a SelectionChanged handler switches to None selection mode")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()


        BEGIN_TEST_METHOD(ValidateReorderWhenReselectHandlerSelectsAnotherItem)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that a reorder keeps an item selected by a SelectionChanged handler while the moved items are re-selected")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateReorderWhenReselectHandlerClearsSelection)
            TEST_METHOD_PROPERTY(L"Description", L"Validates a reorder when a SelectionChanged handler clears the selection while the moved items are re-selected")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateReorderWhenMoveHandlerSwitchesToSingleSelectionMode)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that a reorder selects one moved item after a SelectionChanged handler switches to Single selection mode while the items are moved")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateReorderWhenMoveHandlerSwitchesToNoneSelectionMode)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that a reorder selects nothing after a SelectionChanged handler switches to None selection mode while the items are moved")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateReorderWhenMoveHandlerClearsSelection)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that a reorder restores the selection of the moved items after a SelectionChanged handler clears it while they are moved")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateReorderWhenMoveHandlerSwitchesToMultipleSelectionMode)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that a reorder re-selects all moved items after a SelectionChanged handler switches to Multiple selection mode while they are moved")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()
        BEGIN_TEST_METHOD(ValidateReorderWhenMoveHandlerSelectsAnotherItem)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that a reorder keeps an item selected by a SelectionChanged handler while the items are moved")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateDragReorderWhenReselectHandlerRemovesItem)
            TEST_METHOD_PROPERTY(L"Description", L"Validates a drag and drop reorder when a SelectionChanged handler removes an item while the moved items are re-selected")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateDragReorderWhenReselectHandlerReplacesItemsSource)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that a drag and drop reorder stops re-selecting when a SelectionChanged handler replaces ItemsSource")
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
