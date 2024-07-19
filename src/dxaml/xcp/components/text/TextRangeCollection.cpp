// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <DOCollection.h>
#include <DOPointerCast.h>
#include <TextHighlighter.h>
#include <TextRange.h>
#include <wil/resource.h>
#include "TextRangeCollection.h"

#pragma warning(disable:4267) // 'var' : conversion from 'size_t' to 'type', possible loss of data
#pragma warning(disable:4244) // 'argument' : conversion from 'type1' to 'type2', possible loss of data

_Check_return_ HRESULT
CTextRangeCollection::Append(
    _In_ CValue& value,
    _Out_opt_ uint32_t* indexOut
    )
{
    CValue coercedValue;
    IFC_RETURN(EnsureValue(value, coercedValue));

    ASSERT(coercedValue.GetType() == valueTextRange);
    m_items.push_back(coercedValue.As<valueTextRange>());

    if (indexOut)
    {
        *indexOut = m_items.size();
    }

    IFC_RETURN(OnAddToCollection(coercedValue));

    return S_OK;
}

_Check_return_ HRESULT
CTextRangeCollection::Insert(
    uint32_t index,
    _In_ CValue& value
    )
{
    CValue coercedValue;
    IFC_RETURN(EnsureValue(value, coercedValue));

    // Redirect calls that would generate an append operation
    if (index >= m_items.size())
    {
        return Append(coercedValue);
    }

    // Put the new object in its slot.
    ASSERT(coercedValue.GetType() == valueTextRange);
    m_items.insert(m_items.begin() + index, coercedValue.As<valueTextRange>());

    IFC_RETURN(OnAddToCollection(coercedValue));

    return S_OK;
}

_Check_return_ void*
CTextRangeCollection::RemoveAt(
    uint32_t index
    )
{
    // Bail out quickly if the index is out of range
    if (index >= m_items.size())
    {
        return nullptr;
    }

    //allocate memory for the return value;
    auto removedData = wil::make_unique_failfast<TextRangeData>();
    *removedData = m_items[index];
    m_items.erase(m_items.begin() + index);

    CValue valueRemoved;
    valueRemoved.Set<valueTextRange>(*removedData);
    if (FAILED(OnRemoveFromCollection(valueRemoved, index)))
    {
        return nullptr;
    }

    return removedData.release();
}

_Check_return_ void*
CTextRangeCollection::GetItemWithAddRef(
    uint32_t index)
{
    // Bail out quickly if the index is out of range
    if (index >= m_items.size())
    {
        return nullptr;
    }

    // Like the insertion operation the GetItem operation is best handled in the
    // flattened form.  We waste some memory for performance reasons.
    auto value = new TextRangeData(m_items[index]);

    return value;
}

_Check_return_ HRESULT
CTextRangeCollection::IndexOf(
    _In_ CValue& value,
    _Out_ int32_t* indexOut)
{
    CValue coercedValue;
    IFC_RETURN(EnsureValue(value, coercedValue));

    ASSERT(coercedValue.GetType() == valueTextRange);
    auto textRange = coercedValue.As<valueTextRange>();

    auto found = std::find(
        m_items.begin(),
        m_items.end(),
        textRange);

    if (found != m_items.end())
    {
        *indexOut = found - m_items.begin();
        return S_OK;
    }
    else
    {
        // this item is not in the collection
        return E_INVALIDARG;
    }
}

// Helper for functions which accept parameters of type CValue.
//
// Ensures that a parameter is of the expected type either as a CValue type
// or a DO that is the "boxed" representation of that type.  If not, then the
// function will fail with E_INVALIDARG
//
// ***n.b.: The coerced CValue is for use on the stack and no private
// copy is made.  If the target valueType requires that the m_peValue be set,
// for example, the coercedValue may point into the originalValue.
_Check_return_ HRESULT
CTextRangeCollection::EnsureValue(
    _In_ const CValue& originalValue,
    _Outref_ CValue& coercedValue)
{
    ASSERT(&originalValue != &coercedValue);

    if (originalValue.GetType() == valueTextRange)
    {
        coercedValue.CopyConverted(originalValue);
    }
    else
    {
        auto textRange = do_pointer_cast<CTextRange>(originalValue);

        if (textRange)
        {
            coercedValue.Set<valueTextRange>(textRange->m_range);
        }
        else
        {
            return E_INVALIDARG;
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
CTextRangeCollection::OnAddToCollection(
    _In_ const CValue& value
    )
{
    OnCollectionChanged();
    return S_OK;
}

_Check_return_ HRESULT
CTextRangeCollection::OnRemoveFromCollection(
    _In_ const CValue& value,
    int32_t previousIndex
    )
{
    OnCollectionChanged();
    return S_OK;
}

_Check_return_ HRESULT
CTextRangeCollection::OnClear()
{
    OnCollectionChanged();
    return S_OK;
}

void
CTextRangeCollection::OnCollectionChanged()
{
    auto owner = GetOwner();

    if (owner != nullptr)
    {
        if (auto textHighlighterOwner = do_pointer_cast<CTextHighlighter>(owner))
        {
            textHighlighterOwner->InvalidateTextRanges();
        }
        else
        {
            // CTextRangeCollection has unsupported owner.  If CTextRangeCollection is added
            // to new types in the future, it must be appropriate cast here to notify the parent type
            // of collection changes.
            ASSERT(FALSE);
        }
    }
}