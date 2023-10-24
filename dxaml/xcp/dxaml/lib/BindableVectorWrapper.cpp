// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//
//      Implements a wrapper arround an IBindableVector to make it
//      look like a simple IVector<IInspectable *>.
//
//  Note:
//      This version of the wrapper does not listen for changes
//      on the source.

#include "precomp.h"
#include "BindableVectorWrapper.h"

using namespace DirectUI;
using namespace xaml_data;
using namespace xaml_interop;

BindableVectorWrapper::BindableVectorWrapper()
{ }

BindableVectorWrapper::~BindableVectorWrapper()
{
}

HRESULT BindableVectorWrapper::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(wfc::IVector<IInspectable *>)))
    {
        *ppObject = static_cast<wfc::IVector<IInspectable *>*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(wfc::IIterable<IInspectable *>)))
    {
        *ppObject = static_cast<wfc::IIterable<IInspectable *>*>(this);
    }
    else
    {
        RRETURN(ctl::WeakReferenceSource::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}

// IVector
IFACEMETHODIMP BindableVectorWrapper::GetAt(_In_opt_ unsigned index, _Out_  IInspectable **item)
{
    RRETURN(m_tpSource->GetAt(index, item));
}

IFACEMETHODIMP BindableVectorWrapper::get_Size(_Out_ unsigned *size)
{
    RRETURN(m_tpSource->get_Size(size));
}

IFACEMETHODIMP BindableVectorWrapper::GetView(_Outptr_result_maybenull_ wfc::IVectorView<IInspectable *>** view)
{
    HRESULT hr = S_OK;
    IBindableVectorView *pView = NULL;

    IFC(m_tpSource->GetView(&pView));

    // IBindableVectorView is designed to be v-table compatible with IVectorView<IInspectable *>
    *view = reinterpret_cast<wfc::IVectorView<IInspectable *>*>(pView);
    pView = NULL;

Cleanup:

    ReleaseInterface(pView);

    RRETURN(hr);
}

IFACEMETHODIMP BindableVectorWrapper::IndexOf(_In_opt_ IInspectable * value, _Out_ unsigned *index, _Out_ BOOLEAN *found)
{
    RRETURN(m_tpSource->IndexOf(value, index, found));
}

IFACEMETHODIMP BindableVectorWrapper::SetAt(_In_ unsigned index, _In_opt_ IInspectable *item)
{
    RRETURN(m_tpSource->SetAt(index, item));
}

IFACEMETHODIMP BindableVectorWrapper::InsertAt(_In_ unsigned index, _In_ IInspectable *item)
{
    RRETURN(m_tpSource->InsertAt(index, item));
}

IFACEMETHODIMP BindableVectorWrapper::RemoveAt(_In_ unsigned index)
{
    RRETURN(m_tpSource->RemoveAt(index));
}

IFACEMETHODIMP BindableVectorWrapper::Append(_In_opt_ IInspectable * item)
{
    RRETURN(m_tpSource->Append(item));
}

IFACEMETHODIMP BindableVectorWrapper::RemoveAtEnd()
{
    RRETURN(m_tpSource->RemoveAtEnd());
}

IFACEMETHODIMP BindableVectorWrapper::Clear()
{
    RRETURN(m_tpSource->Clear());
}

// IIterable
IFACEMETHODIMP BindableVectorWrapper::First(_Outptr_ wfc::IIterator<IInspectable *> **value)
{
    HRESULT hr = S_OK;
    IBindableIterator *pIterator = NULL;
    IBindableIterable *pIterable = NULL;

    IFC(ctl::do_query_interface(pIterable, m_tpSource.Get()));

    IFC(pIterable->First(&pIterator));

    // IBindableIterator is designed to be v-table compatible with IIterator<IInspectable*>
    *value = reinterpret_cast<wfc::IIterator<IInspectable *>*>(pIterator);
    pIterator = NULL;

Cleanup:

    ReleaseInterface(pIterator);
    ReleaseInterface(pIterable);

    RRETURN(hr);
}


_Check_return_
HRESULT
BindableVectorWrapper::SetVector(_In_ IBindableVector *pVector)
{
    HRESULT hr = S_OK;

    IFCEXPECT(!m_tpSource);

    // Wrap the vector in a TrackerTargetReference.  But whether or not that will actuall perform tracking is optional
    // based on the EnableTracking virtual.
    SetPtrValue(m_tpSource, pVector);

Cleanup:

    RRETURN(hr);
}

_Check_return_
HRESULT
BindableVectorWrapper::CreateInstance(
    _In_ IBindableVector *pVector,
    _Outptr_ wfc::IVector<IInspectable *> **ppVector)
{
    HRESULT hr = S_OK;
    BindableVectorWrapper *pResult = NULL;

    IFC(ctl::ComObject<BindableVectorWrapper>::CreateInstance(&pResult));

    IFC(pResult->SetVector(pVector));

    *ppVector = pResult;
    pResult = NULL;

Cleanup:

    ctl::release_interface(pResult);

    RRETURN(hr);
}





