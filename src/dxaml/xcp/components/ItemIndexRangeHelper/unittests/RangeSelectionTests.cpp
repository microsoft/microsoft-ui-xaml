// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ItemIndexRangeHelperUnitTests.h"

#include <XamlLogging.h>

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace Enterprise { namespace ItemIndexRangeHelper {

    //
    // Test Cases
    //
    void ItemIndexRangeHelperUnitTests::ValidateCreate()
    {
        DirectUI::Components::ItemIndexRangeHelper::RangeSelection rangeSelection;

        VERIFY_ARE_EQUAL(rangeSelection.size(), static_cast<unsigned int>(0));
    }

    void ItemIndexRangeHelperUnitTests::ValidateItemInsertedAt()
    {
        ItemInsertedAtHelperBefore();
        ItemInsertedAtHelperAfter();
        ItemInsertedAtHelperInside();
    }

    void ItemIndexRangeHelperUnitTests::ValidateItemRemovedAt()
    {
        ItemRemovedAtHelperBefore();
        ItemRemovedAtHelperAfter();
        ItemRemovedAtHelperInside();
        ItemRemovedAtHelperInBetween();
    }

    void ItemIndexRangeHelperUnitTests::ValidateItemChangedAt()
    {
        ItemChangedAtHelperBefore();
        ItemChangedAtHelperAfter();
        ItemChangedAtHelperInside();
    }

    void ItemIndexRangeHelperUnitTests::ValidateSelectAll()
    {
        SelectAllHelperNoRanges();
        SelectAllHelperWithRanges();
    }

    void ItemIndexRangeHelperUnitTests::ValidateSelectRange()
    {
        DirectUI::Components::ItemIndexRangeHelper::RangeSelection rangeSelection;

        SelectRangeInitialRange(rangeSelection);
        SelectRangeBeforeRange(rangeSelection);
        SelectRangeAfterRange(rangeSelection);
        SelectRangeFrontAdjacentRange(rangeSelection);
        SelectRangeEndAdjacentRange(rangeSelection);
        SelectRangeInsideRange(rangeSelection);
        SelectRangeOverlapRange(rangeSelection);
        SelectRangeFrontIntersectionRange(rangeSelection);
        SelectRangeEndIntersectionRange(rangeSelection);
        SelectRangeMultipleIntersectionRange(rangeSelection);
        SelectRangeMultipleIntersectionOverlapRange(rangeSelection);
    }

    void ItemIndexRangeHelperUnitTests::ValidateDeselectRange()
    {
        DirectUI::Components::ItemIndexRangeHelper::RangeSelection rangeSelection;
        std::vector<DirectUI::Components::ItemIndexRangeHelper::Range> addedRanges;

        rangeSelection.SelectRange(5, 20, addedRanges);

        DeselectRangeBeforeRange(rangeSelection);
        DeselectRangeAfterRange(rangeSelection);
        DeselectRangeFrontAdjacentRange(rangeSelection);
        DeselectRangeEndAdjacentRange(rangeSelection);
        DeselectRangeInsideRange(rangeSelection);
        DeselectRangeOverlapRange(rangeSelection);
        DeselectRangeFrontIntersectionRange(rangeSelection);
        DeselectRangeEndIntersectionRange(rangeSelection);
        DeselectRangeMultipleIntersectionRange(rangeSelection);
        DeselectRangeMultipleIntersectionOverlapRange(rangeSelection);
    }

    void ItemIndexRangeHelperUnitTests::ValidateDeselectAll()
    {
        DeselectAllHelperNoRanges();
        DeselectAllHelperWithRanges();
    }

    //
    // ItemInsertedAt Helpers
    //
    void ItemIndexRangeHelperUnitTests::ItemInsertedAtHelperBefore()
    {
        DirectUI::Components::ItemIndexRangeHelper::RangeSelection rangeSelection;
        std::vector<DirectUI::Components::ItemIndexRangeHelper::Range> addedRanges;

        LOG_OUTPUT(L"***ItemInsertedAt Before***");

        rangeSelection.SelectRange(2, 5, addedRanges);
        rangeSelection.SelectRange(10, 3, addedRanges);

        rangeSelection.ItemInsertedAt(0);

        VERIFY_ARE_EQUAL(rangeSelection.size(), static_cast<unsigned int>(2));

        VerifyRange(*(rangeSelection.begin()), 3, 5);
        VerifyRange(*(rangeSelection.begin() + 1), 11, 3);
    }

    void ItemIndexRangeHelperUnitTests::ItemInsertedAtHelperAfter()
    {
        DirectUI::Components::ItemIndexRangeHelper::RangeSelection rangeSelection;
        std::vector<DirectUI::Components::ItemIndexRangeHelper::Range> addedRanges;

        LOG_OUTPUT(L"***ItemInsertedAt After***");

        rangeSelection.SelectRange(2, 5, addedRanges);
        rangeSelection.SelectRange(10, 3, addedRanges);

        rangeSelection.ItemInsertedAt(7);

        VERIFY_ARE_EQUAL(rangeSelection.size(), static_cast<unsigned int>(2));

        VerifyRange(*(rangeSelection.begin()), 2, 5);
        VerifyRange(*(rangeSelection.begin() + 1), 11, 3);
    }

    void ItemIndexRangeHelperUnitTests::ItemInsertedAtHelperInside()
    {
        DirectUI::Components::ItemIndexRangeHelper::RangeSelection rangeSelection;
        std::vector<DirectUI::Components::ItemIndexRangeHelper::Range> addedRanges;

        LOG_OUTPUT(L"***ItemInsertedAt Inside***");

        rangeSelection.SelectRange(2, 5, addedRanges);
        rangeSelection.SelectRange(10, 3, addedRanges);

        rangeSelection.ItemInsertedAt(4);

        VERIFY_ARE_EQUAL(rangeSelection.size(), static_cast<unsigned int>(3));

        VerifyRange(*(rangeSelection.begin()), 2, 2);
        VerifyRange(*(rangeSelection.begin() + 1), 5, 3);
        VerifyRange(*(rangeSelection.begin() + 2), 11, 3);
    }

    //
    // ItemRemovedAt Helpers
    //
    void ItemIndexRangeHelperUnitTests::ItemRemovedAtHelperBefore()
    {
        DirectUI::Components::ItemIndexRangeHelper::RangeSelection rangeSelection;
        std::vector<DirectUI::Components::ItemIndexRangeHelper::Range> addedRanges;

        LOG_OUTPUT(L"***ItemRemovedAt Before***");

        rangeSelection.SelectRange(2, 5, addedRanges);
        rangeSelection.SelectRange(10, 3, addedRanges);

        rangeSelection.ItemRemovedAt(0);

        VERIFY_ARE_EQUAL(rangeSelection.size(), static_cast<unsigned int>(2));

        VerifyRange(*(rangeSelection.begin()), 1, 5);
        VerifyRange(*(rangeSelection.begin() + 1), 9, 3);
    }

    void ItemIndexRangeHelperUnitTests::ItemRemovedAtHelperAfter()
    {
        DirectUI::Components::ItemIndexRangeHelper::RangeSelection rangeSelection;
        std::vector<DirectUI::Components::ItemIndexRangeHelper::Range> addedRanges;

        LOG_OUTPUT(L"***ItemRemovedAt After***");

        rangeSelection.SelectRange(2, 5, addedRanges);
        rangeSelection.SelectRange(10, 3, addedRanges);

        rangeSelection.ItemRemovedAt(7);

        VERIFY_ARE_EQUAL(rangeSelection.size(), static_cast<unsigned int>(2));

        VerifyRange(*(rangeSelection.begin()), 2, 5);
        VerifyRange(*(rangeSelection.begin() + 1), 9, 3);
    }

    void ItemIndexRangeHelperUnitTests::ItemRemovedAtHelperInside()
    {
        DirectUI::Components::ItemIndexRangeHelper::RangeSelection rangeSelection;
        std::vector<DirectUI::Components::ItemIndexRangeHelper::Range> addedRanges;

        LOG_OUTPUT(L"***ItemRemovedAt Inside***");

        rangeSelection.SelectRange(2, 5, addedRanges);
        rangeSelection.SelectRange(10, 3, addedRanges);

        rangeSelection.ItemRemovedAt(4);

        VERIFY_ARE_EQUAL(rangeSelection.size(), static_cast<unsigned int>(2));

        VerifyRange(*(rangeSelection.begin()), 2, 4);
        VerifyRange(*(rangeSelection.begin() + 1), 9, 3);
    }

    void ItemIndexRangeHelperUnitTests::ItemRemovedAtHelperInBetween()
    {
        DirectUI::Components::ItemIndexRangeHelper::RangeSelection rangeSelection;
        std::vector<DirectUI::Components::ItemIndexRangeHelper::Range> addedRanges;

        LOG_OUTPUT(L"***ItemRemovedAt InBetween***");

        rangeSelection.SelectRange(2, 5, addedRanges);
        rangeSelection.SelectRange(8, 3, addedRanges);

        rangeSelection.ItemRemovedAt(7);

        VERIFY_ARE_EQUAL(rangeSelection.size(), static_cast<unsigned int>(1));

        VerifyRange(*(rangeSelection.begin()), 2, 8);
    }

    //
    // ItemChangedAt Helpers
    //
    void ItemIndexRangeHelperUnitTests::ItemChangedAtHelperBefore()
    {
        DirectUI::Components::ItemIndexRangeHelper::RangeSelection rangeSelection;
        std::vector<DirectUI::Components::ItemIndexRangeHelper::Range> addedRanges;

        LOG_OUTPUT(L"***ItemChangedAt Before***");

        rangeSelection.SelectRange(2, 5, addedRanges);
        rangeSelection.SelectRange(10, 3, addedRanges);

        rangeSelection.ItemChangedAt(0);

        VERIFY_ARE_EQUAL(rangeSelection.size(), static_cast<unsigned int>(2));

        VerifyRange(*(rangeSelection.begin()), 2, 5);
        VerifyRange(*(rangeSelection.begin() + 1), 10, 3);
    }

    void ItemIndexRangeHelperUnitTests::ItemChangedAtHelperAfter()
    {
        DirectUI::Components::ItemIndexRangeHelper::RangeSelection rangeSelection;
        std::vector<DirectUI::Components::ItemIndexRangeHelper::Range> addedRanges;

        LOG_OUTPUT(L"***ItemChangedAt After***");

        rangeSelection.SelectRange(2, 5, addedRanges);
        rangeSelection.SelectRange(10, 3, addedRanges);

        rangeSelection.ItemChangedAt(7);

        VERIFY_ARE_EQUAL(rangeSelection.size(), static_cast<unsigned int>(2));

        VerifyRange(*(rangeSelection.begin()), 2, 5);
        VerifyRange(*(rangeSelection.begin() + 1), 10, 3);
    }

    void ItemIndexRangeHelperUnitTests::ItemChangedAtHelperInside()
    {
        DirectUI::Components::ItemIndexRangeHelper::RangeSelection rangeSelection;
        std::vector<DirectUI::Components::ItemIndexRangeHelper::Range> addedRanges;

        LOG_OUTPUT(L"***ItemChangedAt Inside***");

        rangeSelection.SelectRange(2, 5, addedRanges);
        rangeSelection.SelectRange(10, 3, addedRanges);

        rangeSelection.ItemChangedAt(4);

        VERIFY_ARE_EQUAL(rangeSelection.size(), static_cast<unsigned int>(3));

        VerifyRange(*(rangeSelection.begin()), 2, 2);
        VerifyRange(*(rangeSelection.begin() + 1), 5, 2);
        VerifyRange(*(rangeSelection.begin() + 2), 10, 3);
    }

    //
    // SelectAll Helpers
    //
    void ItemIndexRangeHelperUnitTests::SelectAllHelperNoRanges()
    {
        DirectUI::Components::ItemIndexRangeHelper::RangeSelection rangeSelection;
        std::vector<DirectUI::Components::ItemIndexRangeHelper::Range> addedRanges;

        LOG_OUTPUT(L"***SelectAll NoRanges***");

        rangeSelection.SelectAll(static_cast<unsigned int>(20), addedRanges);

        VERIFY_ARE_EQUAL(rangeSelection.size(), static_cast<unsigned int>(1));

        VerifyRange(*(rangeSelection.begin()), 0, 20);

        VERIFY_ARE_EQUAL(addedRanges.size(), static_cast<unsigned int>(1));

        VerifyRange(*(addedRanges.begin()), 0, 20);
    }

    void ItemIndexRangeHelperUnitTests::SelectAllHelperWithRanges()
    {
        DirectUI::Components::ItemIndexRangeHelper::RangeSelection rangeSelection;
        std::vector<DirectUI::Components::ItemIndexRangeHelper::Range> addedRanges;

        LOG_OUTPUT(L"***SelectAll WithRanges***");

        rangeSelection.SelectRange(2, 5, addedRanges);
        rangeSelection.SelectRange(10, 3, addedRanges);

        rangeSelection.SelectAll(static_cast<unsigned int>(20), addedRanges);

        VERIFY_ARE_EQUAL(rangeSelection.size(), static_cast<unsigned int>(1));

        VerifyRange(*(rangeSelection.begin()), 0, 20);

        VERIFY_ARE_EQUAL(addedRanges.size(), static_cast<unsigned int>(3));

        VerifyRange(*(addedRanges.begin()), 0, 2);
        VerifyRange(*(addedRanges.begin() + 1), 7, 3);
        VerifyRange(*(addedRanges.begin() + 2), 13, 7);
    }

    //
    // SelectRange Helpers
    //
    void ItemIndexRangeHelperUnitTests::SelectRangeInitialRange(DirectUI::Components::ItemIndexRangeHelper::RangeSelection& rangeSelection)
    {
        std::vector<DirectUI::Components::ItemIndexRangeHelper::Range> addedRanges;

        LOG_OUTPUT(L"***SelectRange InitialRange***");

        rangeSelection.SelectRange(8, 2, addedRanges);

        VERIFY_ARE_EQUAL(rangeSelection.size(), static_cast<unsigned int>(1));

        VerifyRange(*(rangeSelection.begin()), 8, 2);

        VERIFY_ARE_EQUAL(addedRanges.size(), static_cast<unsigned int>(1));

        VerifyRange(*(addedRanges.begin()), 8, 2);
    }

    void ItemIndexRangeHelperUnitTests::SelectRangeBeforeRange(DirectUI::Components::ItemIndexRangeHelper::RangeSelection& rangeSelection)
    {
        std::vector<DirectUI::Components::ItemIndexRangeHelper::Range> addedRanges;

        LOG_OUTPUT(L"***SelectRange BeforeRange***");

        rangeSelection.SelectRange(2, 3, addedRanges);

        VERIFY_ARE_EQUAL(rangeSelection.size(), static_cast<unsigned int>(2));

        VerifyRange(*(rangeSelection.begin()), 2, 3);
        VerifyRange(*(rangeSelection.begin() + 1), 8, 2);

        VERIFY_ARE_EQUAL(addedRanges.size(), static_cast<unsigned int>(1));

        VerifyRange(*(addedRanges.begin()), 2, 3);
    }

    void ItemIndexRangeHelperUnitTests::SelectRangeAfterRange(DirectUI::Components::ItemIndexRangeHelper::RangeSelection& rangeSelection)
    {
        std::vector<DirectUI::Components::ItemIndexRangeHelper::Range> addedRanges;

        LOG_OUTPUT(L"***SelectRange AfterRange***");

        rangeSelection.SelectRange(16, 4, addedRanges);

        VERIFY_ARE_EQUAL(rangeSelection.size(), static_cast<unsigned int>(3));

        VerifyRange(*(rangeSelection.begin()), 2, 3);
        VerifyRange(*(rangeSelection.begin() + 1), 8, 2);
        VerifyRange(*(rangeSelection.begin() + 2), 16, 4);

        VERIFY_ARE_EQUAL(addedRanges.size(), static_cast<unsigned int>(1));

        VerifyRange(*(addedRanges.begin()), 16, 4);
    }

    void ItemIndexRangeHelperUnitTests::SelectRangeFrontAdjacentRange(DirectUI::Components::ItemIndexRangeHelper::RangeSelection& rangeSelection)
    {
        std::vector<DirectUI::Components::ItemIndexRangeHelper::Range> addedRanges;

        LOG_OUTPUT(L"***SelectRange FrontAdjacentRange***");

        rangeSelection.SelectRange(6, 2, addedRanges);

        VERIFY_ARE_EQUAL(rangeSelection.size(), static_cast<unsigned int>(3));

        VerifyRange(*(rangeSelection.begin()), 2, 3);
        VerifyRange(*(rangeSelection.begin() + 1), 6, 4);
        VerifyRange(*(rangeSelection.begin() + 2), 16, 4);

        VERIFY_ARE_EQUAL(addedRanges.size(), static_cast<unsigned int>(1));

        VerifyRange(*(addedRanges.begin()), 6, 2);
    }

    void ItemIndexRangeHelperUnitTests::SelectRangeEndAdjacentRange(DirectUI::Components::ItemIndexRangeHelper::RangeSelection& rangeSelection)
    {
        std::vector<DirectUI::Components::ItemIndexRangeHelper::Range> addedRanges;

        LOG_OUTPUT(L"***SelectRange EndAdjacentRange***");

        rangeSelection.SelectRange(10, 1, addedRanges);

        VERIFY_ARE_EQUAL(rangeSelection.size(), static_cast<unsigned int>(3));

        VerifyRange(*(rangeSelection.begin()), 2, 3);
        VerifyRange(*(rangeSelection.begin() + 1), 6, 5);
        VerifyRange(*(rangeSelection.begin() + 2), 16, 4);

        VERIFY_ARE_EQUAL(addedRanges.size(), static_cast<unsigned int>(1));

        VerifyRange(*(addedRanges.begin()), 10, 1);
    }

    void ItemIndexRangeHelperUnitTests::SelectRangeInsideRange(DirectUI::Components::ItemIndexRangeHelper::RangeSelection& rangeSelection)
    {
        std::vector<DirectUI::Components::ItemIndexRangeHelper::Range> addedRanges;

        LOG_OUTPUT(L"***SelectRange InsideRange***");

        rangeSelection.SelectRange(7, 2, addedRanges);

        VERIFY_ARE_EQUAL(rangeSelection.size(), static_cast<unsigned int>(3));

        VerifyRange(*(rangeSelection.begin()), 2, 3);
        VerifyRange(*(rangeSelection.begin() + 1), 6, 5);
        VerifyRange(*(rangeSelection.begin() + 2), 16, 4);

        VERIFY_ARE_EQUAL(addedRanges.size(), static_cast<unsigned int>(0));
    }

    void ItemIndexRangeHelperUnitTests::SelectRangeOverlapRange(DirectUI::Components::ItemIndexRangeHelper::RangeSelection& rangeSelection)
    {
        std::vector<DirectUI::Components::ItemIndexRangeHelper::Range> addedRanges;

        LOG_OUTPUT(L"***SelectRange OverlapRange***");

        rangeSelection.SelectRange(14, 7, addedRanges);

        VERIFY_ARE_EQUAL(rangeSelection.size(), static_cast<unsigned int>(3));

        VerifyRange(*(rangeSelection.begin()), 2, 3);
        VerifyRange(*(rangeSelection.begin() + 1), 6, 5);
        VerifyRange(*(rangeSelection.begin() + 2), 14, 7);

        VERIFY_ARE_EQUAL(addedRanges.size(), static_cast<unsigned int>(2));

        VerifyRange(*(addedRanges.begin()), 14, 2);
        VerifyRange(*(addedRanges.begin() + 1), 20, 1);
    }

    void ItemIndexRangeHelperUnitTests::SelectRangeFrontIntersectionRange(DirectUI::Components::ItemIndexRangeHelper::RangeSelection& rangeSelection)
    {
        std::vector<DirectUI::Components::ItemIndexRangeHelper::Range> addedRanges;

        LOG_OUTPUT(L"***SelectRange FrontIntersectionRange***");

        rangeSelection.SelectRange(13, 5, addedRanges);

        VERIFY_ARE_EQUAL(rangeSelection.size(), static_cast<unsigned int>(3));

        VerifyRange(*(rangeSelection.begin()), 2, 3);
        VerifyRange(*(rangeSelection.begin() + 1), 6, 5);
        VerifyRange(*(rangeSelection.begin() + 2), 13, 8);

        VERIFY_ARE_EQUAL(addedRanges.size(), static_cast<unsigned int>(1));

        VerifyRange(*(addedRanges.begin()), 13, 1);
    }

    void ItemIndexRangeHelperUnitTests::SelectRangeEndIntersectionRange(DirectUI::Components::ItemIndexRangeHelper::RangeSelection& rangeSelection)
    {
        std::vector<DirectUI::Components::ItemIndexRangeHelper::Range> addedRanges;

        LOG_OUTPUT(L"***SelectRange EndIntersectionRange***");

        rangeSelection.SelectRange(18, 5, addedRanges);

        VERIFY_ARE_EQUAL(rangeSelection.size(), static_cast<unsigned int>(3));

        VerifyRange(*(rangeSelection.begin()), 2, 3);
        VerifyRange(*(rangeSelection.begin() + 1), 6, 5);
        VerifyRange(*(rangeSelection.begin() + 2), 13, 10);

        VERIFY_ARE_EQUAL(addedRanges.size(), static_cast<unsigned int>(1));

        VerifyRange(*(addedRanges.begin()), 21, 2);
    }

    void ItemIndexRangeHelperUnitTests::SelectRangeMultipleIntersectionRange(DirectUI::Components::ItemIndexRangeHelper::RangeSelection& rangeSelection)
    {
        std::vector<DirectUI::Components::ItemIndexRangeHelper::Range> addedRanges;

        LOG_OUTPUT(L"***SelectRange MultipleIntersectionRange***");

        rangeSelection.SelectRange(3, 7, addedRanges);

        VERIFY_ARE_EQUAL(rangeSelection.size(), static_cast<unsigned int>(2));

        VerifyRange(*(rangeSelection.begin()), 2, 9);
        VerifyRange(*(rangeSelection.begin() + 1), 13, 10);

        VERIFY_ARE_EQUAL(addedRanges.size(), static_cast<unsigned int>(1));

        VerifyRange(*(addedRanges.begin()), 5, 1);
    }

    void ItemIndexRangeHelperUnitTests::SelectRangeMultipleIntersectionOverlapRange(DirectUI::Components::ItemIndexRangeHelper::RangeSelection& rangeSelection)
    {
        std::vector<DirectUI::Components::ItemIndexRangeHelper::Range> addedRanges;

        LOG_OUTPUT(L"***SelectRange MultipleIntersectionOverlapRange***");

        rangeSelection.SelectRange(1, 25, addedRanges);

        VERIFY_ARE_EQUAL(rangeSelection.size(), static_cast<unsigned int>(1));

        VerifyRange(*(rangeSelection.begin()), 1, 25);

        VERIFY_ARE_EQUAL(addedRanges.size(), static_cast<unsigned int>(3));

        VerifyRange(*(addedRanges.begin()), 1, 1);
        VerifyRange(*(addedRanges.begin() + 1), 11, 2);
        VerifyRange(*(addedRanges.begin() + 2), 23, 3);
    }

    //
    // DeselectRange Helpers
    //
    void ItemIndexRangeHelperUnitTests::DeselectRangeBeforeRange(DirectUI::Components::ItemIndexRangeHelper::RangeSelection& rangeSelection)
    {
        std::vector<DirectUI::Components::ItemIndexRangeHelper::Range> removedRanges;

        LOG_OUTPUT(L"***DeselectRange BeforeRange***");

        rangeSelection.DeselectRange(1, 2, removedRanges);

        VERIFY_ARE_EQUAL(rangeSelection.size(), static_cast<unsigned int>(1));

        VerifyRange(*(rangeSelection.begin()), 5, 20);

        VERIFY_ARE_EQUAL(removedRanges.size(), static_cast<unsigned int>(0));
    }

    void ItemIndexRangeHelperUnitTests::DeselectRangeAfterRange(DirectUI::Components::ItemIndexRangeHelper::RangeSelection& rangeSelection)
    {
        std::vector<DirectUI::Components::ItemIndexRangeHelper::Range> removedRanges;

        LOG_OUTPUT(L"***DeselectRange AfterRange***");

        rangeSelection.DeselectRange(26, 2, removedRanges);

        VERIFY_ARE_EQUAL(rangeSelection.size(), static_cast<unsigned int>(1));

        VerifyRange(*(rangeSelection.begin()), 5, 20);

        VERIFY_ARE_EQUAL(removedRanges.size(), static_cast<unsigned int>(0));
    }

    void ItemIndexRangeHelperUnitTests::DeselectRangeFrontAdjacentRange(DirectUI::Components::ItemIndexRangeHelper::RangeSelection& rangeSelection)
    {
        std::vector<DirectUI::Components::ItemIndexRangeHelper::Range> removedRanges;

        LOG_OUTPUT(L"***DeselectRange FrontAdjacentRange***");

        rangeSelection.DeselectRange(3, 2, removedRanges);

        VERIFY_ARE_EQUAL(rangeSelection.size(), static_cast<unsigned int>(1));

        VerifyRange(*(rangeSelection.begin()), 5, 20);

        VERIFY_ARE_EQUAL(removedRanges.size(), static_cast<unsigned int>(0));
    }

    void ItemIndexRangeHelperUnitTests::DeselectRangeEndAdjacentRange(DirectUI::Components::ItemIndexRangeHelper::RangeSelection& rangeSelection)
    {
        std::vector<DirectUI::Components::ItemIndexRangeHelper::Range> removedRanges;

        LOG_OUTPUT(L"***DeselectRange EndAdjacentRange***");

        rangeSelection.DeselectRange(25, 1, removedRanges);

        VERIFY_ARE_EQUAL(rangeSelection.size(), static_cast<unsigned int>(1));

        VerifyRange(*(rangeSelection.begin()), 5, 20);

        VERIFY_ARE_EQUAL(removedRanges.size(), static_cast<unsigned int>(0));
    }

    void ItemIndexRangeHelperUnitTests::DeselectRangeInsideRange(DirectUI::Components::ItemIndexRangeHelper::RangeSelection& rangeSelection)
    {
        std::vector<DirectUI::Components::ItemIndexRangeHelper::Range> removedRanges;

        LOG_OUTPUT(L"***DeselectRange InsideRange***");

        rangeSelection.DeselectRange(11, 3, removedRanges);

        VERIFY_ARE_EQUAL(rangeSelection.size(), static_cast<unsigned int>(2));

        VerifyRange(*(rangeSelection.begin()), 5, 6);
        VerifyRange(*(rangeSelection.begin() + 1), 14, 11);

        VERIFY_ARE_EQUAL(removedRanges.size(), static_cast<unsigned int>(1));

        VerifyRange(*(removedRanges.begin()), 11, 3);
    }

    void ItemIndexRangeHelperUnitTests::DeselectRangeOverlapRange(DirectUI::Components::ItemIndexRangeHelper::RangeSelection& rangeSelection)
    {
        std::vector<DirectUI::Components::ItemIndexRangeHelper::Range> removedRanges;

        LOG_OUTPUT(L"***DeselectRange OverlapRange***");

        rangeSelection.DeselectRange(3, 9, removedRanges);

        VERIFY_ARE_EQUAL(rangeSelection.size(), static_cast<unsigned int>(1));

        VerifyRange(*(rangeSelection.begin()), 14, 11);

        VERIFY_ARE_EQUAL(removedRanges.size(), static_cast<unsigned int>(1));

        VerifyRange(*(removedRanges.begin()), 5, 6);
    }

    void ItemIndexRangeHelperUnitTests::DeselectRangeFrontIntersectionRange(DirectUI::Components::ItemIndexRangeHelper::RangeSelection& rangeSelection)
    {
        std::vector<DirectUI::Components::ItemIndexRangeHelper::Range> removedRanges;

        LOG_OUTPUT(L"***DeselectRange FrontIntersectionRange***");

        rangeSelection.DeselectRange(12, 5, removedRanges);

        VERIFY_ARE_EQUAL(rangeSelection.size(), static_cast<unsigned int>(1));

        VerifyRange(*(rangeSelection.begin()), 17, 8);

        VERIFY_ARE_EQUAL(removedRanges.size(), static_cast<unsigned int>(1));

        VerifyRange(*(removedRanges.begin()), 14, 3);
    }

    void ItemIndexRangeHelperUnitTests::DeselectRangeEndIntersectionRange(DirectUI::Components::ItemIndexRangeHelper::RangeSelection& rangeSelection)
    {
        std::vector<DirectUI::Components::ItemIndexRangeHelper::Range> removedRanges;

        LOG_OUTPUT(L"***DeselectRange EndIntersectionRange***");

        rangeSelection.DeselectRange(22, 5, removedRanges);

        VERIFY_ARE_EQUAL(rangeSelection.size(), static_cast<unsigned int>(1));

        VerifyRange(*(rangeSelection.begin()), 17, 5);

        VERIFY_ARE_EQUAL(removedRanges.size(), static_cast<unsigned int>(1));

        VerifyRange(*(removedRanges.begin()), 22, 3);
    }

    void ItemIndexRangeHelperUnitTests::DeselectRangeMultipleIntersectionRange(DirectUI::Components::ItemIndexRangeHelper::RangeSelection& rangeSelection)
    {
        std::vector<DirectUI::Components::ItemIndexRangeHelper::Range> addedRanges;
        std::vector<DirectUI::Components::ItemIndexRangeHelper::Range> removedRanges;

        LOG_OUTPUT(L"***DeselectRange MultiIntersectionRange***");

        rangeSelection.SelectRange(2, 3, addedRanges);
        rangeSelection.SelectRange(8, 4, addedRanges);

        rangeSelection.DeselectRange(3, 16, removedRanges);

        VERIFY_ARE_EQUAL(rangeSelection.size(), static_cast<unsigned int>(2));

        VerifyRange(*(rangeSelection.begin()), 2, 1);
        VerifyRange(*(rangeSelection.begin() + 1), 19, 3);

        VERIFY_ARE_EQUAL(removedRanges.size(), static_cast<unsigned int>(3));

        VerifyRange(*(removedRanges.begin()), 3, 2);
        VerifyRange(*(removedRanges.begin() + 1), 8, 4);
        VerifyRange(*(removedRanges.begin() + 2), 17, 2);
    }

    void ItemIndexRangeHelperUnitTests::DeselectRangeMultipleIntersectionOverlapRange(DirectUI::Components::ItemIndexRangeHelper::RangeSelection& rangeSelection)
    {
        std::vector<DirectUI::Components::ItemIndexRangeHelper::Range> addedRanges;
        std::vector<DirectUI::Components::ItemIndexRangeHelper::Range> removedRanges;

        LOG_OUTPUT(L"***DeselectRange MultipleIntersectionOverlapRange***");

        rangeSelection.SelectRange(8, 4, addedRanges);

        rangeSelection.DeselectRange(2, 20, removedRanges);

        VERIFY_ARE_EQUAL(rangeSelection.size(), static_cast<unsigned int>(0));

        VERIFY_ARE_EQUAL(removedRanges.size(), static_cast<unsigned int>(3));

        VerifyRange(*(removedRanges.begin()), 2, 1);
        VerifyRange(*(removedRanges.begin() + 1), 8, 4);
        VerifyRange(*(removedRanges.begin() + 2), 19, 3);
    }

    //
    // DeselectAll Helpers
    //
    void ItemIndexRangeHelperUnitTests::DeselectAllHelperNoRanges()
    {
        DirectUI::Components::ItemIndexRangeHelper::RangeSelection rangeSelection;
        std::vector<DirectUI::Components::ItemIndexRangeHelper::Range> removedRanges;

        LOG_OUTPUT(L"***DeselectAll NoRanges***");

        rangeSelection.DeselectAll(removedRanges);

        VERIFY_ARE_EQUAL(rangeSelection.size(), static_cast<unsigned int>(0));

        VERIFY_ARE_EQUAL(removedRanges.size(), static_cast<unsigned int>(0));
    }

    void ItemIndexRangeHelperUnitTests::DeselectAllHelperWithRanges()
    {
        DirectUI::Components::ItemIndexRangeHelper::RangeSelection rangeSelection;
        std::vector<DirectUI::Components::ItemIndexRangeHelper::Range> addedRanges;
        std::vector<DirectUI::Components::ItemIndexRangeHelper::Range> removedRanges;

        LOG_OUTPUT(L"***DeselectAll WithRanges***");

        rangeSelection.SelectRange(2, 5, addedRanges);
        rangeSelection.SelectRange(10, 3, addedRanges);

        rangeSelection.DeselectAll(removedRanges);

        VERIFY_ARE_EQUAL(rangeSelection.size(), static_cast<unsigned int>(0));

        VERIFY_ARE_EQUAL(removedRanges.size(), static_cast<unsigned int>(2));

        VerifyRange(*(removedRanges.begin()), 2, 5);
        VerifyRange(*(removedRanges.begin() + 1), 10, 3);
    }

} } } } } }