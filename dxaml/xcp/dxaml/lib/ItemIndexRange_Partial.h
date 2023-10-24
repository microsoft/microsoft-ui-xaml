// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ItemIndexRange.g.h"
#include "ItemIndexRangeHelper.h"

namespace DirectUI
{
    PARTIAL_CLASS(ItemIndexRange)
    {
    public:
        _Check_return_ HRESULT get_LastIndexImpl(
            _Out_ int* pValue);

#pragma region static functions
        // returns true if the index is inside the given range
        static _Check_return_ HRESULT IndexInItemIndexRange(
            _In_ xaml_data::IItemIndexRange* range,
            _In_ int index,
            _Out_ bool *pbReturnValue);

        // returns true if the index is inside a range inside the given collection
        static _Check_return_ HRESULT IndexInItemIndexRangeCollection(
            _In_ TrackerCollection<xaml_data::ItemIndexRange*>* pCollection,
            _In_ int index,
            _Out_ bool *pbReturnValue);

        // returns true if the two ranges are identical
        static _Check_return_ HRESULT AreItemIndexRangesEqual(
            _In_ xaml_data::IItemIndexRange* range1,
            _In_ xaml_data::IItemIndexRange* range2,
            _Out_ bool* pbReturnValue);

        // return true if the two collection are identical (even in order of items)
        static _Check_return_ HRESULT AreItemIndexRangeCollectionsEqual(
            _In_ TrackerCollection<xaml_data::ItemIndexRange*>* pCollection1,
            _In_ TrackerCollection<xaml_data::ItemIndexRange*>* pCollection2,
            _Out_ bool* pbReturnValue);

        // goes through the vector and creates ranges from continuous indices
        static _Check_return_ HRESULT AppendItemIndexRangesFromSortedVectorToItemIndexRangeCollection(
            _In_ const std::vector<unsigned int>& indices,
            _Out_ TrackerCollection<xaml_data::ItemIndexRange*>* pCollection);

        // Used to create and ItemIndexRange and append it to the provided collection
        static _Check_return_ HRESULT AppendItemIndexRangeToItemIndexRangeCollection(
            _In_ int startIndex,
            _In_ unsigned int length,
            _Inout_ DirectUI::TrackerCollection<xaml_data::ItemIndexRange*>* pCollection);

        // creates an ItemIndexRange collection from a vector of ranges
        static _Check_return_ HRESULT GetItemIndexRangeCollectionFromRangesVector(
            _In_ std::vector<DirectUI::Components::ItemIndexRangeHelper::Range>::iterator& rangesBegin,
            _In_ std::vector<DirectUI::Components::ItemIndexRangeHelper::Range>::iterator& rangesEnd,
            _Out_ TrackerCollection<xaml_data::ItemIndexRange*>* pCollection);
#pragma endregion
    };
}
