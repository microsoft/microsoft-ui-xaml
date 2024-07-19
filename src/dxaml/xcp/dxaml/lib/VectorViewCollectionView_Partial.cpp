// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "VectorViewCollectionView.g.h"

using namespace DirectUI;
using namespace xaml_data;

VectorViewCollectionView::VectorViewCollectionView()
{ }

VectorViewCollectionView::~VectorViewCollectionView()
{ }

IFACEMETHODIMP VectorViewCollectionView::GetAt(_In_opt_ unsigned index, _Out_  IInspectable **item)
{
    HRESULT hr = S_OK;

    ARG_VALIDRETURNPOINTER(item);
    IFC(m_tpSource->GetAt(index, item));

Cleanup:

    RRETURN(hr);
}

IFACEMETHODIMP VectorViewCollectionView::get_Size(_Out_ unsigned *size)
{
    HRESULT hr = S_OK;

    ARG_VALIDRETURNPOINTER(size);
    IFC(m_tpSource->get_Size(size));

Cleanup:

    RRETURN(hr);
}

IFACEMETHODIMP VectorViewCollectionView::GetView(_Outptr_result_maybenull_ wfc::IVectorView<IInspectable *>** view)
{
    ARG_VALIDRETURNPOINTER(view);
    *view = m_tpSource.Get();
    AddRefInterface(*view);

    RRETURN(S_OK);
}

IFACEMETHODIMP VectorViewCollectionView::IndexOf(_In_opt_ IInspectable * value, _Out_ unsigned *index, _Out_ BOOLEAN *found)
{
    HRESULT hr = S_OK;

    ARG_VALIDRETURNPOINTER(index);
    ARG_VALIDRETURNPOINTER(found);
    IFC(m_tpSource->IndexOf(value, index, found));

Cleanup:

    RRETURN(hr);
}

// IIterable<IInspectable *>
IFACEMETHODIMP VectorViewCollectionView::First(_Outptr_ wfc::IIterator<IInspectable *> **value)
{
    HRESULT hr = S_OK;
    wfc::IIterable<IInspectable *> *pIterable = NULL;

    ARG_VALIDRETURNPOINTER(value);
    IFC(ctl::do_query_interface(pIterable, m_tpSource.Get()));
    IFC(pIterable->First(value));

Cleanup:

    ReleaseInterface(pIterable);

    RRETURN(hr);
}

// Initialization of the collection view
void
VectorViewCollectionView::SetSource(_In_ wfc::IVectorView<IInspectable *>* const pSource)
{
    SetPtrValue(m_tpSource, pSource);
}

// Factory method
_Check_return_
HRESULT
VectorViewCollectionView::CreateInstance(
    _In_ wfc::IVectorView<IInspectable *> *pSource,
    _Outptr_ ICollectionView  **instance)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<VectorViewCollectionView> spCV;

    IFC(ctl::make(&spCV));
    spCV->SetSource(pSource);

    *instance = spCV.Detach();

Cleanup:
    RRETURN(hr);
}



