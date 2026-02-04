// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ItemIndexRangeHelperUnitTests.h"

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace Enterprise { namespace ItemIndexRangeHelper {

    //
    // Test Cases
    //
    void ItemIndexRangeHelperUnitTests::ValidateIndexInRange()
    {
        VERIFY_IS_TRUE(DirectUI::Components::ItemIndexRangeHelper::IndexInRange(1, 5, 1));
        VERIFY_IS_TRUE(DirectUI::Components::ItemIndexRangeHelper::IndexInRange(1, 5, 2));
        VERIFY_IS_TRUE(DirectUI::Components::ItemIndexRangeHelper::IndexInRange(1, 5, 5));

        VERIFY_IS_FALSE(DirectUI::Components::ItemIndexRangeHelper::IndexInRange(1, 5, 6));
        VERIFY_IS_FALSE(DirectUI::Components::ItemIndexRangeHelper::IndexInRange(0, 5, 6));
    }

    void ItemIndexRangeHelperUnitTests::ValidateAreRangesEqual()
    {
        VERIFY_IS_TRUE(DirectUI::Components::ItemIndexRangeHelper::AreRangesEqual(1, 5, 1, 5));

        VERIFY_IS_FALSE(DirectUI::Components::ItemIndexRangeHelper::AreRangesEqual(1, 5, 1, 6));
        VERIFY_IS_FALSE(DirectUI::Components::ItemIndexRangeHelper::AreRangesEqual(2, 5, 1, 5));
    }

    void ItemIndexRangeHelperUnitTests::ValidateGetContinousIndicesLengthStartingAtIndex()
    {
        unsigned int tempIndices[] = { 1, 2, 3, 4, 5, 7, 9, 10 };
        std::vector<unsigned int> indices;
        for (int i = 0; i < 8; ++i)
        {
            indices.push_back(tempIndices[i]);
        }

        VERIFY_ARE_EQUAL(DirectUI::Components::ItemIndexRangeHelper::GetContinousIndicesLengthStartingAtIndex(indices, 0), static_cast<unsigned int>(5));
        VERIFY_ARE_EQUAL(DirectUI::Components::ItemIndexRangeHelper::GetContinousIndicesLengthStartingAtIndex(indices, 1), static_cast<unsigned int>(4));
        VERIFY_ARE_EQUAL(DirectUI::Components::ItemIndexRangeHelper::GetContinousIndicesLengthStartingAtIndex(indices, 2), static_cast<unsigned int>(3));
        VERIFY_ARE_EQUAL(DirectUI::Components::ItemIndexRangeHelper::GetContinousIndicesLengthStartingAtIndex(indices, 3), static_cast<unsigned int>(2));
        VERIFY_ARE_EQUAL(DirectUI::Components::ItemIndexRangeHelper::GetContinousIndicesLengthStartingAtIndex(indices, 4), static_cast<unsigned int>(1));
        VERIFY_ARE_EQUAL(DirectUI::Components::ItemIndexRangeHelper::GetContinousIndicesLengthStartingAtIndex(indices, 5), static_cast<unsigned int>(1));
        VERIFY_ARE_EQUAL(DirectUI::Components::ItemIndexRangeHelper::GetContinousIndicesLengthStartingAtIndex(indices, 6), static_cast<unsigned int>(2));
        VERIFY_ARE_EQUAL(DirectUI::Components::ItemIndexRangeHelper::GetContinousIndicesLengthStartingAtIndex(indices, 7), static_cast<unsigned int>(1));
    }

    void ItemIndexRangeHelperUnitTests::ValidateIndexWithRespectToRange()
    {
        VERIFY_ARE_EQUAL(DirectUI::Components::ItemIndexRangeHelper::IndexWithRespectToRange(1, 5, 1), DirectUI::Components::ItemIndexRangeHelper::IndexLocation::Inside);
        VERIFY_ARE_EQUAL(DirectUI::Components::ItemIndexRangeHelper::IndexWithRespectToRange(1, 5, 2), DirectUI::Components::ItemIndexRangeHelper::IndexLocation::Inside);
        VERIFY_ARE_EQUAL(DirectUI::Components::ItemIndexRangeHelper::IndexWithRespectToRange(1, 5, 5), DirectUI::Components::ItemIndexRangeHelper::IndexLocation::Inside);

        VERIFY_ARE_EQUAL(DirectUI::Components::ItemIndexRangeHelper::IndexWithRespectToRange(1, 5, 0), DirectUI::Components::ItemIndexRangeHelper::IndexLocation::Before);
        VERIFY_ARE_EQUAL(DirectUI::Components::ItemIndexRangeHelper::IndexWithRespectToRange(1, 5, 6), DirectUI::Components::ItemIndexRangeHelper::IndexLocation::After);
    }

    void ItemIndexRangeHelperUnitTests::ValidateIsFirstRangeInsideSecondRange()
    {
        VERIFY_IS_TRUE(DirectUI::Components::ItemIndexRangeHelper::IsFirstRangeInsideSecondRange(1, 5, 1, 5));
        VERIFY_IS_TRUE(DirectUI::Components::ItemIndexRangeHelper::IsFirstRangeInsideSecondRange(1, 5, 0, 6));

        VERIFY_IS_FALSE(DirectUI::Components::ItemIndexRangeHelper::IsFirstRangeInsideSecondRange(1, 5, 2, 6));
        VERIFY_IS_FALSE(DirectUI::Components::ItemIndexRangeHelper::IsFirstRangeInsideSecondRange(1, 5, 7, 6));
        VERIFY_IS_FALSE(DirectUI::Components::ItemIndexRangeHelper::IsFirstRangeInsideSecondRange(2, 6, 1, 5));
        VERIFY_IS_FALSE(DirectUI::Components::ItemIndexRangeHelper::IsFirstRangeInsideSecondRange(7, 6, 1, 5));
        VERIFY_IS_FALSE(DirectUI::Components::ItemIndexRangeHelper::IsFirstRangeInsideSecondRange(1, 5, 2, 1));
    }

    void ItemIndexRangeHelperUnitTests::ValidateIsFirstRangeAdjacentToSecondRange()
    {
        VERIFY_IS_TRUE(DirectUI::Components::ItemIndexRangeHelper::IsFirstRangeAdjacentToSecondRange(1, 5, 6));

        VERIFY_IS_FALSE(DirectUI::Components::ItemIndexRangeHelper::IsFirstRangeAdjacentToSecondRange(1, 5, 0));
        VERIFY_IS_FALSE(DirectUI::Components::ItemIndexRangeHelper::IsFirstRangeAdjacentToSecondRange(1, 5, 1));
        VERIFY_IS_FALSE(DirectUI::Components::ItemIndexRangeHelper::IsFirstRangeAdjacentToSecondRange(1, 5, 2));
        VERIFY_IS_FALSE(DirectUI::Components::ItemIndexRangeHelper::IsFirstRangeAdjacentToSecondRange(1, 5, 7));
    }

} } } } } }