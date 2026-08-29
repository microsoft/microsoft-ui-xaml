// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "InternalStringCollection.h"

using namespace DirectUI;



HRESULT STDMETHODCALLTYPE HStringIterator::get_Current(_Out_ HSTRING *pCurrent)
{
    if (m_currentIndex >= m_tpCollection->m_strings.size())
    {
        return E_BOUNDS;
    }
    return m_tpCollection->m_strings[m_currentIndex].CopyTo(pCurrent);
}

HRESULT STDMETHODCALLTYPE HStringIterator::get_HasCurrent(_Out_ boolean *hasCurrent)
{
    *hasCurrent = m_currentIndex < m_tpCollection.Get()->m_strings.size();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE HStringIterator::MoveNext(_Out_ boolean *hasCurrent)
{
    if (m_currentIndex < m_tpCollection->m_strings.size())
    {
        ++m_currentIndex;
        return get_HasCurrent(hasCurrent);
    }
    else
    {
        return E_BOUNDS;
    }
}


HRESULT HStringIterator::Initialize(_In_ InternalStringCollection* pCollection)
{
    SetPtrValue(m_tpCollection, pCollection);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE InternalStringCollection::First(_Outptr_result_maybenull_ wfc::IIterator<HSTRING> **ppFirst)
{
    ctl::ComPtr<HStringIterator> spResult;

    IFC_RETURN(ctl::make(&spResult));
    IFC_RETURN(spResult->Initialize(this));

    IFC_RETURN(spResult.MoveTo(ppFirst));

    return S_OK;
}

