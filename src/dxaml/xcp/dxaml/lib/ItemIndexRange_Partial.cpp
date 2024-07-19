// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ItemIndexRange_Partial.h"
#include "ItemIndexRangeHelper.h"

#pragma warning(disable:4267) //'var' : conversion from 'size_t' to 'type', possible loss of data

using namespace DirectUI;
using namespace DirectUI::Components;

_Check_return_ HRESULT ItemIndexRange::get_LastIndexImpl(
    _Out_ int* pValue)
{
    HRESULT hr = S_OK;

    int firstIndex = 0;
    unsigned int length = 0;

    IFC(get_FirstIndex(&firstIndex));
    IFC(get_Length(&length));

    *pValue = firstIndex + length - 1;

Cleanup:
    return hr;
}

_Check_return_ HRESULT ItemIndexRangeFactory::CreateInstanceImpl(
    _In_ int firstIndex,
    _In_ unsigned int length,
    _In_opt_ IInspectable* pOuter,
    _Outptr_ IInspectable** ppInner,
    _Outptr_ xaml_data::IItemIndexRange** ppInstance)
{
    HRESULT hr = S_OK;

    IInspectable* pInner = nullptr;
    xaml_data::IItemIndexRange* pRangeAsI = nullptr;
    ItemIndexRange* pRange = nullptr;

    ARG_VALIDRETURNPOINTER(ppInstance);
    IFCEXPECT(pOuter == nullptr || ppInner != nullptr);

    IFC(ctl::AggregableActivationFactory<ItemIndexRange>::ActivateInstance(pOuter, &pInner));
    IFC(ctl::do_query_interface(pRangeAsI, pInner));

    pRange = static_cast<ItemIndexRange*>(pRangeAsI);
    IFC(pRange->put_FirstIndex(firstIndex));
    IFC(pRange->put_Length(length));

    if (ppInner)
    {
        *ppInner = pInner;
        pInner = NULL;
    }

    *ppInstance = pRangeAsI;
    pRangeAsI = NULL;

Cleanup:
    ReleaseInterface(pInner);
    ReleaseInterface(pRangeAsI);
    return hr;
}

// returns true if the index is inside the given range
_Check_return_ HRESULT ItemIndexRange::IndexInItemIndexRange(
    _In_ xaml_data::IItemIndexRange* range,
    _In_ int index,
    _Out_ bool *pbReturnValue)
{
    HRESULT hr = S_OK;

    int firstIndex = -1;
    unsigned int length = 0;

    IFC(range->get_FirstIndex(&firstIndex));
    IFC(range->get_Length(&length));

    *pbReturnValue = ItemIndexRangeHelper::IndexInRange(firstIndex, firstIndex + length - 1, index);

Cleanup:
    return hr;
}

// returns true if the index is inside a range inside the given collection
_Check_return_ HRESULT ItemIndexRange::IndexInItemIndexRangeCollection(
    _In_ TrackerCollection<xaml_data::ItemIndexRange*>* pCollection,
    _In_ int index,
    _Out_ bool *pbReturnValue)
{
    HRESULT hr = S_OK;

    unsigned int size = 0;
    ctl::ComPtr<xaml_data::IItemIndexRange> spRange;

    IFC(pCollection->get_Size(&size));

    for (unsigned int i = 0; i < size; ++i)
    {
        IFC(pCollection->GetAt(i, &spRange));

        IFC(ItemIndexRange::IndexInItemIndexRange(spRange.Get(), index, pbReturnValue));

        if (*pbReturnValue)
        {
            break;
        }
    }

Cleanup:
    return hr;
}

// returns true if the two ranges are identical
_Check_return_ HRESULT ItemIndexRange::AreItemIndexRangesEqual(
    _In_ xaml_data::IItemIndexRange* range1,
    _In_ xaml_data::IItemIndexRange* range2,
    _Out_ bool* pbReturnValue)
{
    HRESULT hr = S_OK;

    int firstIndex1 = -1;
    int firstIndex2 = -1;
    unsigned int length1 = 0;
    unsigned int length2 = 0;

    IFC(range1->get_FirstIndex(&firstIndex1));
    IFC(range1->get_Length(&length1));

    IFC(range2->get_FirstIndex(&firstIndex2));
    IFC(range2->get_Length(&length2));

    *pbReturnValue = ItemIndexRangeHelper::AreRangesEqual(firstIndex1, length1, firstIndex2, length2);

Cleanup:
    return hr;
}

// return true if the two collection are identical (even in order of items)
// for example, {{5,1},{7,3}} is NOT equal to {{7,3},{5,1}}
_Check_return_ HRESULT ItemIndexRange::AreItemIndexRangeCollectionsEqual(
    _In_ TrackerCollection<xaml_data::ItemIndexRange*>* pCollection1,
    _In_ TrackerCollection<xaml_data::ItemIndexRange*>* pCollection2,
    _Out_ bool* pbReturnValue)
{
    HRESULT hr = S_OK;

    unsigned int size1 = 0;
    unsigned int size2 = 0;
    bool areEqual = false;
    ctl::ComPtr<xaml_data::IItemIndexRange> spRange1;
    ctl::ComPtr<xaml_data::IItemIndexRange> spRange2;

    *pbReturnValue = false;

    IFC(pCollection1->get_Size(&size1));
    IFC(pCollection2->get_Size(&size2));

    // if they are not of the same size, then they're definitely not equal
    if (size1 == size2)
    {
        unsigned int i = 0;

        for (; i < size1; ++i)
        {
            IFC(pCollection1->GetAt(i, &spRange1));
            IFC(pCollection2->GetAt(i, &spRange2));

            IFC(ItemIndexRange::AreItemIndexRangesEqual(spRange1.Get(), spRange2.Get(), &areEqual));

            // if the ranges are not equal, that means the collections are not identical
            // we break, ensuring that the return value will be false
            if (!areEqual)
            {
                break;
            }
        }

        // if we reached this point and i is equal to the size, that means that all ranges are identical
        if (i == size1)
        {
            *pbReturnValue = true;
        }
    }

Cleanup:
    return hr;
}

// goes through the vector and creates ranges from continuous indices
_Check_return_ HRESULT ItemIndexRange::AppendItemIndexRangesFromSortedVectorToItemIndexRangeCollection(
    _In_ const std::vector<unsigned int>& indices,
    _Out_ TrackerCollection<xaml_data::ItemIndexRange*>* pCollection)
{
    HRESULT hr = S_OK;

    unsigned int size = indices.size();
    unsigned int length = 1;

    // grouping continuous ranges together
    for (unsigned int i = 0; i < size; i += length)
    {
        length = ItemIndexRangeHelper::GetContinousIndicesLengthStartingAtIndex(indices, i);

        IFC(ItemIndexRange::AppendItemIndexRangeToItemIndexRangeCollection(indices[i], length, pCollection));
    }

Cleanup:
    return hr;
}

// Used to create and ItemIndexRange and append it to the provided collection
_Check_return_ HRESULT ItemIndexRange::AppendItemIndexRangeToItemIndexRangeCollection(
    _In_ int startIndex,
    _In_ unsigned int length,
    _Inout_ DirectUI::TrackerCollection<xaml_data::ItemIndexRange*>* pCollection)
{
    HRESULT hr = S_OK;

    ctl::ComPtr<ItemIndexRange> spRange;

    IFC(ctl::make(&spRange));

    IFC(spRange->put_FirstIndex(startIndex));
    IFC(spRange->put_Length(length));

    // add the range to the tracked ranges list
    IFC(pCollection->Append(spRange.Get()));

Cleanup:
    return hr;
}

// creates an ItemIndexRange collection from a vector of ranges
_Check_return_ HRESULT ItemIndexRange::GetItemIndexRangeCollectionFromRangesVector(
    _In_ std::vector<DirectUI::Components::ItemIndexRangeHelper::Range>::iterator& rangesBegin,
    _In_ std::vector<DirectUI::Components::ItemIndexRangeHelper::Range>::iterator& rangesEnd,
    _Out_ TrackerCollection<xaml_data::ItemIndexRange*>* pCollection)
{
    HRESULT hr = S_OK;

    for (auto itr = rangesBegin; itr != rangesEnd; ++itr)
    {
        ctl::ComPtr<ItemIndexRange> spRange;
        auto& range = *itr;

        IFC(ctl::make(&spRange));
        IFC(spRange->put_FirstIndex(range.firstIndex));
        IFC(spRange->put_Length(range.length));

        IFC(pCollection->Append(spRange.Get()));
    }

Cleanup:
    return hr;
}