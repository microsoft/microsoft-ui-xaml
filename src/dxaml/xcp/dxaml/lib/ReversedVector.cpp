// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ReversedVector.h"

// Helper class for representing vectorchanged event args
class CVectorChangedEventArgs :
    public wrl::RuntimeClass<wfc::IVectorChangedEventArgs>
{
    InspectableClass(nullptr, BaseTrust);

public:
    HRESULT RuntimeClassInitialize(_In_ wfc::CollectionChange change, _In_ unsigned int index)
    {
        _change = change;
        _index = index;
        return S_OK;
    }

    // IVectorChangedEventArgs
    IFACEMETHOD(get_CollectionChange)(_In_ wfc::CollectionChange *pChange) override
    {
        *pChange = _change;
        return S_OK;
    }
    IFACEMETHOD(get_Index)(_Out_ unsigned int *pIndex) override
    {
        *pIndex = _index;
        return S_OK;
    }

private:
    wfc::CollectionChange _change;
    unsigned int _index;
};



ReversedVector::ReversedVector()
    : m_vectorChangedToken()
{ }

ReversedVector::~ReversedVector()
{
    if (m_vectorChangedToken.value != 0)
    {
        VERIFYHR(m_observableVector->remove_VectorChanged(m_vectorChangedToken));
        m_vectorChangedToken.value = 0;
    }
}

_Check_return_ HRESULT
ReversedVector::SetSource(_In_ wfc::IObservableVector<IInspectable*>* source)
{
    // Can only bind to one source
    FAIL_FAST_ASSERT(!m_source);
    FAIL_FAST_ASSERT(!m_observableVector);

    IFC_RETURN(source->QueryInterface(IID_PPV_ARGS(&m_source)));
    IFC_RETURN(source->QueryInterface(IID_PPV_ARGS(&m_observableVector)));

    return S_OK;
}

bool
ReversedVector::IsBoundTo(_In_ wfc::IObservableVector<IInspectable*>* other) const
{
    wrl::ComPtr<IUnknown> sourceUnknown;
    wrl::ComPtr<IUnknown> otherUnknown;

    if (other)
    {
        if (FAILED(other->QueryInterface(IID_PPV_ARGS(&otherUnknown))))
        {
            return false;
        }
    }
    if (m_source)
    {
        if (FAILED(m_source.As(&sourceUnknown)))
        {
            return false;
        }
    }

    return otherUnknown == sourceUnknown;
}

_Check_return_ HRESULT
ReversedVector::GetAt(_In_ unsigned int index, _Out_ IInspectable** item)
{
    return m_source->GetAt(TransformIndex(index), item);
}

_Check_return_ HRESULT
ReversedVector::get_Size(_Out_ unsigned int *size)
{
    return m_source->get_Size(size);
}

_Check_return_ HRESULT
ReversedVector::IndexOf(_In_ IInspectable* value, _Out_ unsigned int *index, _Out_ boolean *found)
{
    HRESULT result = m_source->IndexOf(value, index, found);
    if (SUCCEEDED(result))
    {
        *index = TransformIndex(*index);
    }
    return result;
}

_Check_return_ HRESULT
ReversedVector::add_VectorChanged(_In_ wfc::VectorChangedEventHandler<IInspectable*>* handler, _Out_ EventRegistrationToken* token)
{
    IFC_RETURN(m_eventSource.Add(handler, token));

    if (m_eventSource.GetSize() != 0 && m_vectorChangedToken.value == 0)
    {
        IFC_RETURN(m_observableVector->add_VectorChanged(
                Microsoft::WRL::Callback<wfc::VectorChangedEventHandler<IInspectable*>>(
                    this,
                    &ReversedVector::OnInnerVectorChanged).Get(),
                &m_vectorChangedToken));
    }

    return S_OK;
}

_Check_return_ HRESULT
ReversedVector::remove_VectorChanged(_In_ EventRegistrationToken token)
{
    IFC_RETURN(m_eventSource.Remove(token));

    if (m_eventSource.GetSize() == 0 && m_vectorChangedToken.value != 0)
    {
        IFC_RETURN(m_observableVector->remove_VectorChanged(m_vectorChangedToken));
        m_vectorChangedToken.value = 0;
    }

    return S_OK;
}

_Check_return_ HRESULT
ReversedVector::OnInnerVectorChanged(
    _In_ wfc::IObservableVector<IInspectable*>* pSender,
    _In_ wfc::IVectorChangedEventArgs* pArgs)
{
    if (m_eventSource.GetSize() == 0)
    {
        return S_OK;
    }

    unsigned int size;
    IFC_RETURN(m_source->get_Size(&size));

    wrl::ComPtr<wfc::IVectorChangedEventArgs> outerArgs;
    wfc::CollectionChange innerChange;
    unsigned int innerIndex;
    unsigned int outerIndex;

    IFC_RETURN(pArgs->get_CollectionChange(&innerChange));
    IFC_RETURN(pArgs->get_Index(&innerIndex));

    switch (innerChange)
    {
        case wfc::CollectionChange_ItemInserted:
            {
                const auto sizeBeforeInsertion = size - 1u;
                // Insertion displaces element n to n+1, and element n-1 doesn't get displaced at all.
                // But here we're looking at it backward, so n moves to n-1 and n+1 doesn't move at all.
                // So, the reverse of an insertion is an insertion at n+1, which is why the index is one
                // greater for insertion.
                outerIndex = sizeBeforeInsertion - innerIndex;
                break;
            }
        case wfc::CollectionChange_ItemRemoved:
            {
                const auto sizeBeforeRemoval = size + 1u;
                outerIndex = sizeBeforeRemoval - 1u - innerIndex;
                break;
            }
        case wfc::CollectionChange_ItemChanged:
            outerIndex = size - 1u - innerIndex;
            break;
        case wfc::CollectionChange_Reset:
            outerIndex = 0u;
            break;
        default:
            ASSERT(false);
    }

    wrl::MakeAndInitialize<CVectorChangedEventArgs>(
        &outerArgs,
        innerChange,
        outerIndex);

    return m_eventSource.InvokeAll(this, outerArgs.Get());
}

unsigned int ReversedVector::TransformIndex(unsigned int index)
{
    unsigned int size;
    IFCFAILFAST(m_source->get_Size(&size));
    return (size - 1) - index;
}

