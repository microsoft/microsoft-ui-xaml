// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "VectorCollectionView.g.h"

using namespace DirectUI;
using namespace xaml_data;
using namespace xaml_interop;

VectorCollectionView::VectorCollectionView()
{ }

VectorCollectionView::~VectorCollectionView()
{
    if (m_epVectorChangedHandler)
    {
        auto spSource = m_tpSource.GetSafeReference();
        if (spSource)
        {
            VERIFYHR(m_epVectorChangedHandler.DetachEventHandler(spSource.Get()));
        }
    }
}


// IVector<IInspectable *>
IFACEMETHODIMP VectorCollectionView::GetAt(
    _In_opt_ unsigned index,
    _Out_  IInspectable **item)
{
    HRESULT hr = S_OK;

    ARG_VALIDRETURNPOINTER(item)
    IFC(m_tpSource->GetAt(index, item));

Cleanup:

    RRETURN(hr);
}

IFACEMETHODIMP VectorCollectionView::get_Size(_Out_ unsigned *size)
{
    HRESULT hr = S_OK;

    ARG_VALIDRETURNPOINTER(size);
    IFC(m_tpSource->get_Size(size));

Cleanup:

    RRETURN(hr);
}

IFACEMETHODIMP VectorCollectionView::GetView(
    _Outptr_result_maybenull_ wfc::IVectorView<IInspectable *>** view)
{
    HRESULT hr = S_OK;

    ARG_VALIDRETURNPOINTER(view);
    IFC(m_tpSource->GetView(view));

Cleanup:

    RRETURN(hr);
}

IFACEMETHODIMP VectorCollectionView::IndexOf(
    _In_opt_ IInspectable * value,
    _Out_ unsigned *index,
    _Out_ BOOLEAN *found)
{
    HRESULT hr = S_OK;

    ARG_VALIDRETURNPOINTER(index);
    ARG_VALIDRETURNPOINTER(found);
    IFC(m_tpSource->IndexOf(value, index, found));

Cleanup:

    RRETURN(hr);
}

IFACEMETHODIMP VectorCollectionView::SetAt(
    _In_ unsigned index,
    _In_opt_ IInspectable *item)
{
    RRETURN(m_tpSource->SetAt(index, item));
}

IFACEMETHODIMP VectorCollectionView::InsertAt(
    _In_ unsigned index,
    _In_ IInspectable *item)
{
    RRETURN(m_tpSource->InsertAt(index, item));
}

IFACEMETHODIMP VectorCollectionView::RemoveAt(_In_ unsigned index)
{
    RRETURN(m_tpSource->RemoveAt(index));
}

IFACEMETHODIMP VectorCollectionView::Append(_In_opt_ IInspectable * item)
{
    RRETURN(m_tpSource->Append(item));
}

IFACEMETHODIMP VectorCollectionView::RemoveAtEnd()
{
    RRETURN(m_tpSource->RemoveAtEnd());
}

IFACEMETHODIMP VectorCollectionView::Clear()
{
    RRETURN(m_tpSource->Clear());
}

// IIterable<IInspectable *>
IFACEMETHODIMP VectorCollectionView::First(_Outptr_ wfc::IIterator<IInspectable *> **value)
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

// ISupportIncrementalLoading portion of ICollectionView
_Check_return_
HRESULT
VectorCollectionView::get_HasMoreItemsImpl(_Out_ BOOLEAN *value)
{
    HRESULT hr = S_OK;

    if (!m_tpSupportIncrementalLoading)
    {
        *value = false;
    }
    else
    {
        IFC(m_tpSupportIncrementalLoading->get_HasMoreItems(value));
    }

Cleanup:

    RRETURN(hr);
}

_Check_return_
HRESULT
VectorCollectionView::LoadMoreItemsAsyncImpl(
    _In_ UINT32 count,
    _Outptr_ wf::IAsyncOperation<LoadMoreItemsResult> **operation)
{
    HRESULT hr = S_OK;

    // If we don't have incremental loading capabilities then
    // this method is not implemented
    if (!m_tpSupportIncrementalLoading)
    {
        IFC(E_NOTIMPL);
    }

    IFC(m_tpSupportIncrementalLoading->LoadMoreItemsAsync(count, operation));

Cleanup:

    RRETURN(hr);
}

_Check_return_
HRESULT VectorCollectionView::OnSourceVectorChanged(
    _In_ wfc::IObservableVector<IInspectable *> *pSender,
    _In_ IInspectable *pArgs)
{
    HRESULT hr = S_OK;
    wfc::IVectorChangedEventArgs *pActualArgs = NULL;

    IFC(ctl::do_query_interface(pActualArgs, pArgs));

    IFC(ProcessCollectionChange(pActualArgs));

Cleanup:

    ReleaseInterface(pActualArgs);

    RRETURN(hr);
}

_Check_return_
HRESULT VectorCollectionView::SetSource(_In_ wfc::IVector<IInspectable *> *pSource)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wfc::IObservableVector<IInspectable *>> spObservable;

    SetPtrValue(m_tpSource, pSource);

    // Capture the virtualization interface
    SetPtrValueWithQIOrNull(m_tpSupportIncrementalLoading, m_tpSource.Get());

    // Capture the observable interface.
    //
    // A source that implements IBindableObservableVector, but not IObservableVector<IInspectable *>
    // will be wrapped in a BindableObservableVectorWrapper.
    spObservable = m_tpSource.AsOrNull<wfc::IObservableVector<IInspectable *>>();

    if (spObservable)
    {
        IFC(m_epVectorChangedHandler.AttachEventHandler(spObservable.Get(),
            [this](wfc::IObservableVector<IInspectable *> *pSender, wfc::IVectorChangedEventArgs *pArgs)
            {
                HRESULT hr = S_OK;

                auto pegThis = ctl::try_make_autopeg(this);
                if (pegThis)
                {
                    IFC(OnSourceVectorChanged(pSender, pArgs));
                }

            Cleanup:
                RRETURN(hr);
            }));
    }

Cleanup:

    RRETURN(hr);
}

// Factory static method
_Check_return_
HRESULT VectorCollectionView::CreateInstance(
    _In_ wfc::IVector<IInspectable *> *pSource,
    _Outptr_ xaml_data::ICollectionView  **instance)
{
   HRESULT hr = S_OK;
   ctl::ComPtr<VectorCollectionView> spCV;

   IFCPTR(pSource);

   IFC(ctl::make(&spCV));
   IFC(spCV->SetSource(pSource));

   *instance = spCV.Detach();

Cleanup:
   RRETURN(hr);
}
