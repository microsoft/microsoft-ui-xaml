// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>

#include <ItemIndexRangeHelper.h>

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace Enterprise { namespace ItemIndexRangeHelper {

    class ItemIndexRangeHelperUnitTests : public WEX::TestClass<ItemIndexRangeHelperUnitTests>
    {
    public:
        BEGIN_TEST_CLASS(ItemIndexRangeHelperUnitTests)
        END_TEST_CLASS()

        BEGIN_TEST_METHOD(ValidateIndexInRange)
            TEST_METHOD_PROPERTY(L"Classification", L"Integration")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Description", L"Validates the static function IndexInRange is functioning properly.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateAreRangesEqual)
            TEST_METHOD_PROPERTY(L"Classification", L"Integration")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Description", L"Validates the static function AreRangesEqual is functioning properly.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateGetContinousIndicesLengthStartingAtIndex)
            TEST_METHOD_PROPERTY(L"Classification", L"Integration")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Description", L"Validates the static function GetContinousIndicesLengthStartingAtIndex is functioning properly.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateIndexWithRespectToRange)
            TEST_METHOD_PROPERTY(L"Classification", L"Integration")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Description", L"Validates the static function IndexWithRespectToRange is functioning properly.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateIsFirstRangeInsideSecondRange)
            TEST_METHOD_PROPERTY(L"Classification", L"Integration")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Description", L"Validates the static function IsFirstRangeInsideSecondRange is functioning properly.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateIsFirstRangeAdjacentToSecondRange)
            TEST_METHOD_PROPERTY(L"Classification", L"Integration")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Description", L"Validates the static function IsFirstRangeAdjacentToSecondRange is functioning properly.")
        END_TEST_METHOD()
        
        BEGIN_TEST_METHOD(ValidateCreate)
            TEST_METHOD_PROPERTY(L"Classification", L"Integration")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can create an instance of RangeSelection.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateItemInsertedAt)
            TEST_METHOD_PROPERTY(L"Classification", L"Integration")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Description", L"Validates the ItemInsertedAt function works properly.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateItemRemovedAt)
            TEST_METHOD_PROPERTY(L"Classification", L"Integration")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Description", L"Validates the ItemRemovedAt function works properly.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateItemChangedAt)
            TEST_METHOD_PROPERTY(L"Classification", L"Integration")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Description", L"Validates the ItemChangedAt function works properly.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateSelectAll)
            TEST_METHOD_PROPERTY(L"Classification", L"Integration")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Description", L"Validates the SelectAll function works properly.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateSelectRange)
            TEST_METHOD_PROPERTY(L"Classification", L"Integration")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Description", L"Validates the SelectRange function works properly.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateDeselectRange)
            TEST_METHOD_PROPERTY(L"Classification", L"Integration")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Description", L"Validates the DeselectRange function works properly.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateDeselectAll)
            TEST_METHOD_PROPERTY(L"Classification", L"Integration")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Description", L"Validates the DeselectAll function works properly.")
        END_TEST_METHOD()

    private:
        void VerifyRange(DirectUI::Components::ItemIndexRangeHelper::Range& range, int firstIndex, int length)
        {
            VERIFY_ARE_EQUAL(range.firstIndex, firstIndex);
            VERIFY_ARE_EQUAL(range.length, static_cast<unsigned int>(length));
        }

        // ItemInsertedAt Helpers
        void ItemInsertedAtHelperBefore();
        void ItemInsertedAtHelperAfter();
        void ItemInsertedAtHelperInside();

        // ItemRemovedAt Helpers
        void ItemRemovedAtHelperBefore();
        void ItemRemovedAtHelperAfter();
        void ItemRemovedAtHelperInside();
        void ItemRemovedAtHelperInBetween();

        // ItemChangedAt Helpers
        void ItemChangedAtHelperBefore();
        void ItemChangedAtHelperAfter();
        void ItemChangedAtHelperInside();

        // SelectAll Helpers
        void SelectAllHelperNoRanges();
        void SelectAllHelperWithRanges();

        // SelectRange Helpers
        void SelectRangeInitialRange(DirectUI::Components::ItemIndexRangeHelper::RangeSelection& rangeSelection);
        void SelectRangeBeforeRange(DirectUI::Components::ItemIndexRangeHelper::RangeSelection& rangeSelection);
        void SelectRangeAfterRange(DirectUI::Components::ItemIndexRangeHelper::RangeSelection& rangeSelection);
        void SelectRangeFrontAdjacentRange(DirectUI::Components::ItemIndexRangeHelper::RangeSelection& rangeSelection);
        void SelectRangeEndAdjacentRange(DirectUI::Components::ItemIndexRangeHelper::RangeSelection& rangeSelection);
        void SelectRangeInsideRange(DirectUI::Components::ItemIndexRangeHelper::RangeSelection& rangeSelection);
        void SelectRangeOverlapRange(DirectUI::Components::ItemIndexRangeHelper::RangeSelection& rangeSelection);
        void SelectRangeFrontIntersectionRange(DirectUI::Components::ItemIndexRangeHelper::RangeSelection& rangeSelection);
        void SelectRangeEndIntersectionRange(DirectUI::Components::ItemIndexRangeHelper::RangeSelection& rangeSelection);
        void SelectRangeMultipleIntersectionRange(DirectUI::Components::ItemIndexRangeHelper::RangeSelection& rangeSelection);
        void SelectRangeMultipleIntersectionOverlapRange(DirectUI::Components::ItemIndexRangeHelper::RangeSelection& rangeSelection);
    
        // DeselectRange Helpers
        void DeselectRangeBeforeRange(DirectUI::Components::ItemIndexRangeHelper::RangeSelection& rangeSelection);
        void DeselectRangeAfterRange(DirectUI::Components::ItemIndexRangeHelper::RangeSelection& rangeSelection);
        void DeselectRangeFrontAdjacentRange(DirectUI::Components::ItemIndexRangeHelper::RangeSelection& rangeSelection);
        void DeselectRangeEndAdjacentRange(DirectUI::Components::ItemIndexRangeHelper::RangeSelection& rangeSelection);
        void DeselectRangeInsideRange(DirectUI::Components::ItemIndexRangeHelper::RangeSelection& rangeSelection);
        void DeselectRangeOverlapRange(DirectUI::Components::ItemIndexRangeHelper::RangeSelection& rangeSelection);
        void DeselectRangeFrontIntersectionRange(DirectUI::Components::ItemIndexRangeHelper::RangeSelection& rangeSelection);
        void DeselectRangeEndIntersectionRange(DirectUI::Components::ItemIndexRangeHelper::RangeSelection& rangeSelection);
        void DeselectRangeMultipleIntersectionRange(DirectUI::Components::ItemIndexRangeHelper::RangeSelection& rangeSelection);
        void DeselectRangeMultipleIntersectionOverlapRange(DirectUI::Components::ItemIndexRangeHelper::RangeSelection& rangeSelection);

        // DeselectAll Helpers
        void DeselectAllHelperNoRanges();
        void DeselectAllHelperWithRanges();
    };

} } } } } } 
