// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "VectorCollectionView.g.h"
#include "CollectionViewSource.g.h"

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

    DetachSourceList();
    ReleaseShapingState();

    if (m_isDefaultView)
    {
        UnregisterDefaultView(this);
    }
}


// IVector<IInspectable *>
IFACEMETHODIMP VectorCollectionView::GetAt(
    unsigned index,
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
    unsigned index,
    _In_opt_ IInspectable *item)
{
    RRETURN(GetWriteTarget()->SetAt(index, item));
}

IFACEMETHODIMP VectorCollectionView::InsertAt(
    unsigned index,
    _In_ IInspectable *item)
{
    RRETURN(GetWriteTarget()->InsertAt(index, item));
}

IFACEMETHODIMP VectorCollectionView::RemoveAt(unsigned index)
{
    RRETURN(GetWriteTarget()->RemoveAt(index));
}

IFACEMETHODIMP VectorCollectionView::Append(_In_opt_ IInspectable * item)
{
    RRETURN(GetWriteTarget()->Append(item));
}

IFACEMETHODIMP VectorCollectionView::RemoveAtEnd()
{
    RRETURN(GetWriteTarget()->RemoveAtEnd());
}

IFACEMETHODIMP VectorCollectionView::Clear()
{
    RRETURN(GetWriteTarget()->Clear());
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
        if (*value && IsFilterStarvingLoads())
        {
            *value = FALSE;
        }
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

    if (IsShaped())
    {
        UINT size = 0;
        UINT sourceSize = 0;
        IFC(get_Size(&size));
        IFC(m_tpSourceList->get_Size(&sourceSize));
        if (size != m_sizeAtLastLoad)
        {
            m_fruitlessLoadCount = 0;
        }
        else if (sourceSize != m_sourceSizeAtLastLoad)
        {
            ++m_fruitlessLoadCount;
        }
        m_sizeAtLastLoad = size;
        m_sourceSizeAtLastLoad = sourceSize;
    }

    IFC(m_tpSupportIncrementalLoading->LoadMoreItemsAsync(count, operation));

Cleanup:

    RRETURN(hr);
}

// A filter that rejects the items loads bring would make a consumer (ListView) load until the source has no more:
// once c_maxFruitlessLoads loads in a row grew the source but not the view, it reports no more items (unlike the
// source's HasMoreItems, and unlike WPF, which has no incremental loading) until the view grows or its rules, its
// source or a Refresh recompute it. A filter sparser than that many pages per match stops early.
bool VectorCollectionView::IsFilterStarvingLoads()
{
    UINT size = 0;
    return IsShaped() && m_fruitlessLoadCount >= c_maxFruitlessLoads && SUCCEEDED(get_Size(&size)) && size == m_sizeAtLastLoad;
}

void VectorCollectionView::ResetIncrementalLoadState()
{
    m_fruitlessLoadCount = 0;
    m_sizeAtLastLoad = UINT_MAX;
    m_sourceSizeAtLastLoad = UINT_MAX;
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
    ResetIncrementalLoadState();

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

_Check_return_
HRESULT VectorCollectionView::RetargetSource(_In_ wfc::IVector<IInspectable *> *pSource, bool isGroupingChanged)
{
    ctl::ComPtr<wfc::IVectorChangedEventArgs> spArgs;
    VectorChangedEventArgs *pArgs = nullptr;

    if (m_epVectorChangedHandler)
    {
        auto spOldSource = m_tpSource.AsOrNull<wfc::IObservableVector<IInspectable *>>();
        if (spOldSource)
        {
            IFC_RETURN(m_epVectorChangedHandler.DetachEventHandler(spOldSource.Get()));
        }
        else
        {
            m_epVectorChangedHandler = ctl::EventPtr<VectorChangedEventCallback>();
        }
    }

    IFC_RETURN(SetSource(pSource));

    // The view presents its shaped list but loads more of the collection it shapes: new items arrive as
    // source changes.
    if (m_tpSourceList)
    {
        SetPtrValueWithQIOrNull(m_tpSupportIncrementalLoading, m_tpSourceList.Get());
    }

    // Consumers react to CollectionGroups before the Reset, so they read the new items with the new groups.
    if (isGroupingChanged)
    {
        IFC_RETURN(RaisePropertyChanged(STR_LEN_PAIR(L"CollectionGroups")));
    }

    IFC_RETURN(ctl::ComObject<VectorChangedEventArgs>::CreateInstance(spArgs.ReleaseAndGetAddressOf()));
    spArgs.CastTo(&pArgs);
    IFC_RETURN(pArgs->put_CollectionChange(wfc::CollectionChange_Reset));
    IFC_RETURN(pArgs->put_Index(0));
    IFC_RETURN(ProcessCollectionChange(spArgs.Get()));
    return S_OK;
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
