// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//
//      Implements a wrapper arround an IBindableVector that also
//      implements INCC
//
//  Note:
//      This version of the wrapper does not listen for changes
//      on the source.

#include "precomp.h"
#include "BindableObservableVectorWrapper.h"
#include "VectorChangedEventArgs.g.h"

using namespace DirectUI;
using namespace xaml_data;
using namespace xaml_interop;

BindableObservableVectorWrapper::BindableObservableVectorWrapper()
{
}

BindableObservableVectorWrapper::~BindableObservableVectorWrapper()
{
    if (m_epCollectionChangedHandler)
    {
        auto spINCC = m_tpINCC.GetSafeReference();
        if (spINCC)
        {
            VERIFYHR(m_epCollectionChangedHandler.DetachEventHandler(spINCC.Get()));
        }
    }

    if (m_epBindableVectorChangedHandler)
    {
        auto spObservableVector = m_tpObservableVector.GetSafeReference();
        if (spObservableVector)
        {
            VERIFYHR(m_epBindableVectorChangedHandler.DetachEventHandler(spObservableVector.Get()));
        }
    }
}

HRESULT BindableObservableVectorWrapper::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(wfc::IObservableVector<IInspectable *>)))
    {
        *ppObject = static_cast<wfc::IObservableVector<IInspectable *> *>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(ISupportIncrementalLoading)))
    {
        *ppObject = static_cast<ISupportIncrementalLoading *>(this);
    }
    else
    {
        RRETURN(BindableVectorWrapper::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}

// IObservableVector<IInspectable *>
IFACEMETHODIMP BindableObservableVectorWrapper::add_VectorChanged(
    _In_ wfc::VectorChangedEventHandler<IInspectable *> *pHandler,
    _In_ EventRegistrationToken *token)
{
    HRESULT hr = S_OK;

    ARG_NOTNULL(pHandler, "handler");
    ARG_VALIDRETURNPOINTER(token);

    m_vectorChangedHandlers.AddHandler(pHandler, token);

Cleanup:

    RRETURN(hr);
}

IFACEMETHODIMP BindableObservableVectorWrapper::remove_VectorChanged(
    _In_ EventRegistrationToken token)
{
    RRETURN(m_vectorChangedHandlers.RemoveHandler(token));
}

// ISupportIncrementalLoading
IFACEMETHODIMP BindableObservableVectorWrapper::get_HasMoreItems(_Out_ boolean *value)
{
    HRESULT hr = S_OK;

    ARG_NOTNULL(value, "value");

    if (m_tpSupportIncrementalLoading)
    {
        IFC(m_tpSupportIncrementalLoading->get_HasMoreItems(value));
    }
    else
    {
        *value = false;
    }

Cleanup:

    RRETURN(hr);
}


IFACEMETHODIMP BindableObservableVectorWrapper::LoadMoreItemsAsync(
    _In_ UINT32 count,
    _Outptr_ wf::IAsyncOperation<xaml_data::LoadMoreItemsResult> **operation)
{
    HRESULT hr = S_OK;

    ARG_NOTNULL(operation, "operation");

    if (m_tpSupportIncrementalLoading)
    {
        IFC(m_tpSupportIncrementalLoading->LoadMoreItemsAsync(count, operation));
    }
    else
    {
        // We're not doing incremental loading if the source does not support it
        IFC(E_NOTIMPL);
    }

Cleanup:

    RRETURN(hr);
}


_Check_return_
HRESULT
BindableObservableVectorWrapper::SetINCC(_In_ xaml_interop::INotifyCollectionChanged *pINCC)
{
    HRESULT hr = S_OK;

    IFCEXPECT_ASSERT(!m_tpINCC);
    IFCEXPECT_ASSERT(!m_tpObservableVector);

    // Wrap the pINCC in a TrackerTargetReference.  But whether or not that will actuall perform tracking is optional
    // based on the EnableTracking virtual.
    SetPtrValue(m_tpINCC, pINCC);

    IFC(m_epCollectionChangedHandler.AttachEventHandler(m_tpINCC.Get(),
        [this](IInspectable *sender, INotifyCollectionChangedEventArgs *args)
        {
            return ProcessCollectionChange(args);
        }));

Cleanup:

    RRETURN(hr);
}

_Check_return_
HRESULT
BindableObservableVectorWrapper::SetObservableVector(_In_ xaml_interop::IBindableObservableVector *pObservableVector)
{
    HRESULT hr = S_OK;

    IFCEXPECT_ASSERT(!m_tpINCC);
    IFCEXPECT_ASSERT(!m_tpObservableVector);

    // Wrap the pointer in a TrackerTargetReference.  But whether or not that will actuall perform tracking is optional
    // based on the EnableTracking virtual.
    SetPtrValue(m_tpObservableVector, pObservableVector);

    IFC(m_epBindableVectorChangedHandler.AttachEventHandler(m_tpObservableVector.Get(),
        [this](IBindableObservableVector *pSender, IInspectable *pArgs)
        {
            return ProcessVectorChange(pArgs);
        }));

Cleanup:

    RRETURN(hr);
}

void
BindableObservableVectorWrapper::SetVirtualizingInterface(_In_ IInspectable* const pSource)
{
    SetPtrValueWithQIOrNull(m_tpSupportIncrementalLoading, pSource);
}

void BindableObservableVectorWrapper::OnReferenceTrackerWalk(INT walkType)
{
    m_vectorChangedHandlers.ReferenceTrackerWalk(static_cast<EReferenceTrackerWalkType>(walkType));

    BindableVectorWrapper::OnReferenceTrackerWalk(walkType);
}


_Check_return_
HRESULT
BindableObservableVectorWrapper::ProcessCollectionChange(_In_ INotifyCollectionChangedEventArgs *pArgs)
{
    HRESULT hr = S_OK;
    xaml_interop::NotifyCollectionChangedAction action;
    INT index = 0;

    IFC(pArgs->get_Action(&action));

    switch (action)
    {
    case wxaml_interop::NotifyCollectionChangedAction_Add:
        IFC(pArgs->get_NewStartingIndex(&index));
        IFC(RaiseVectorChanged(wfc::CollectionChange_ItemInserted, index));
        break;

    case wxaml_interop::NotifyCollectionChangedAction_Remove:
        IFC(pArgs->get_OldStartingIndex(&index));
        IFC(RaiseVectorChanged(wfc::CollectionChange_ItemRemoved, index));
        break;

    case wxaml_interop::NotifyCollectionChangedAction_Replace:
        IFC(pArgs->get_NewStartingIndex(&index));
        IFC(RaiseVectorChanged(wfc::CollectionChange_ItemChanged, index));
        break;

    // TODO: Map Move to a remove and add
    case wxaml_interop::NotifyCollectionChangedAction_Move:
    case wxaml_interop::NotifyCollectionChangedAction_Reset:
        IFC(RaiseVectorChanged(wfc::CollectionChange_Reset, 0));
        break;
    }

Cleanup:

    RRETURN(hr);
}

_Check_return_
HRESULT
BindableObservableVectorWrapper::ProcessVectorChange(_In_ IInspectable *pArgs)
{
    HRESULT hr = S_OK;
    wfc::IVectorChangedEventArgs* pVectorChangedEventArgs = NULL;

    IFC(ctl::do_query_interface(pVectorChangedEventArgs, pArgs));

    IFC(RaiseVectorChanged(pVectorChangedEventArgs));

Cleanup:
    ReleaseInterface(pVectorChangedEventArgs);
    RRETURN(hr);
}

_Check_return_
HRESULT
BindableObservableVectorWrapper::RaiseVectorChanged(_In_ wfc::CollectionChange action, UINT index)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<VectorChangedEventArgs> spArgs;
    DXamlCore* pCore = DXamlCore::GetCurrent();

    // May return a recycled instance.
    IFC(pCore->GetVectorChangedEventArgsFromPool(&spArgs));
    IFC(spArgs->put_CollectionChange(action));
    IFC(spArgs->put_Index(index));

    IFC(RaiseVectorChanged(spArgs.Get()));

    IFC(pCore->ReleaseVectorChangedEventArgsToPool(spArgs.Get()));

Cleanup:
    RRETURN(hr);
}

_Check_return_
HRESULT
BindableObservableVectorWrapper::RaiseVectorChanged(wfc::IVectorChangedEventArgs* pArgs)
{
    RRETURN(m_vectorChangedHandlers.Raise(this, pArgs));
}

_Check_return_
HRESULT BindableObservableVectorWrapper::CreateInstance(
    _In_ IBindableVector *pVector,
    _In_ INotifyCollectionChanged *pINCC,
    _Outptr_ wfc::IVector<IInspectable *> **ppVector)
{
    HRESULT hr = S_OK;
    BindableObservableVectorWrapper *pResult = NULL;

    IFC(ctl::ComObject<BindableObservableVectorWrapper>::CreateInstance(&pResult));
    IFC(pResult->InitializeInstance(pVector, pINCC, pVector));

    *ppVector = pResult;
    pResult = NULL;

Cleanup:

    ctl::release_interface(pResult);

    RRETURN(hr);
}



//
// InitializeInstance for the (IBindableVector,INCC) construction overload
//
_Check_return_
HRESULT BindableObservableVectorWrapper::InitializeInstance(
    _In_ xaml_interop::IBindableVector *pVector,
    _In_ xaml_interop::INotifyCollectionChanged *pINCC,
    _In_ IInspectable *pVirtualizingInterfaces )
{
    HRESULT hr = S_OK;

    IFC(SetVector(pVector));
    IFC(SetINCC(pINCC));
    SetVirtualizingInterface(pVector);

Cleanup:

    RRETURN(hr);
}

_Check_return_
HRESULT BindableObservableVectorWrapper::CreateInstance(
    _In_ IBindableObservableVector *pObservableVector,
    _Outptr_ wfc::IVector<IInspectable *> **ppVector)
{
    HRESULT hr = S_OK;
    BindableObservableVectorWrapper *pResult = NULL;
    IBindableVector *pVector = NULL;

    IFC(ctl::do_query_interface(pVector, pObservableVector));

    IFC(ctl::ComObject<BindableObservableVectorWrapper>::CreateInstance(&pResult));

    IFC( pResult->InitializeInstance( pVector, pObservableVector, pObservableVector ));

    *ppVector = pResult;
    pResult = NULL;

Cleanup:

    ctl::release_interface(pResult);
    ReleaseInterface(pVector);

    RRETURN(hr);
}

//
// InitializeInstance for the (IBindableVector,IBindableObservableVector) construction overload
//

_Check_return_
HRESULT BindableObservableVectorWrapper::InitializeInstance(
    _In_ xaml_interop::IBindableVector *pVector,
    _In_ xaml_interop::IBindableObservableVector *pObservableVector,
    _In_ IInspectable *pVirtualizingInterfaces )
{
    HRESULT hr = S_OK;

    IFC(SetVector(pVector));
    IFC(SetObservableVector(pObservableVector));
    SetVirtualizingInterface(pObservableVector);

Cleanup:

    RRETURN( hr );
}



