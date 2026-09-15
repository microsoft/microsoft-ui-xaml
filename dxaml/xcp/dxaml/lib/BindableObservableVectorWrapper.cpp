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
#include "ErrorInfo.h"
#include "TrackerCollections.h"
#include "VectorChangedEventArgs.g.h"
#include <OptionalChangeState.h>

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

IFACEMETHODIMP BindableObservableVectorWrapper::GetAt(unsigned index, _Out_ IInspectable **item)
{
    if (!m_moveView)
    {
        return BindableVectorWrapper::GetAt(index, item);
    }

    IFCPTR_RETURN(item);
    *item = nullptr;
    IFC_RETURN(CheckMoveSourceUnchanged());
    if (index >= m_moveView->GetSize())
    {
        IFC_RETURN(E_BOUNDS);
    }

    ctl::ComPtr<IInspectable> value;
    IFC_RETURN(BindableVectorWrapper::GetAt(m_moveView->GetSourceIndex(index), &value));
    IFC_RETURN(CheckMoveSourceUnchanged());
    return value.CopyTo(item);
}

IFACEMETHODIMP BindableObservableVectorWrapper::get_Size(_Out_ unsigned *size)
{
    if (!m_moveView)
    {
        return BindableVectorWrapper::get_Size(size);
    }

    IFCPTR_RETURN(size);
    IFC_RETURN(CheckMoveSourceUnchanged());
    *size = m_moveView->GetSize();
    return S_OK;
}

IFACEMETHODIMP BindableObservableVectorWrapper::GetView(
    _Outptr_result_maybenull_ wfc::IVectorView<IInspectable *> **view)
{
    if (!m_tpINCC || !OptionalChangeState::IsCollectionMoveNotificationsEnabled())
    {
        return BindableVectorWrapper::GetView(view);
    }

    IFCPTR_RETURN(view);
    *view = nullptr;
    IFC_RETURN(CheckMoveSourceUnchanged());
    ctl::ComPtr<wfc::IVectorView<IInspectable *>> result;
    IFC_RETURN(ctl::ComObject<TrackerView<IInspectable *>>::CreateInstance(result.ReleaseAndGetAddressOf()));
    result.Cast<TrackerView<IInspectable *>>()->SetCollection(this);
    *view = result.Detach();
    return S_OK;
}

IFACEMETHODIMP BindableObservableVectorWrapper::IndexOf(
    _In_opt_ IInspectable *value,
    _Out_ unsigned *index,
    _Out_ boolean *found)
{
    if (!m_moveView)
    {
        return BindableVectorWrapper::IndexOf(value, index, found);
    }

    IFCPTR_RETURN(index);
    IFCPTR_RETURN(found);
    *index = 0;
    *found = false;
    IFC_RETURN(CheckMoveSourceUnchanged());

    for (unsigned current = 0; current < m_moveView->GetSize(); ++current)
    {
        ctl::ComPtr<IInspectable> item;
        bool equal = false;
        IFC_RETURN(BindableVectorWrapper::GetAt(m_moveView->GetSourceIndex(current), &item));
        IFC_RETURN(CheckMoveSourceUnchanged());
        IFC_RETURN(PropertyValue::AreEqual(value, item.Get(), &equal));
        IFC_RETURN(CheckMoveSourceUnchanged());
        if (equal)
        {
            *index = current;
            *found = true;
            break;
        }
    }

    return S_OK;
}

IFACEMETHODIMP BindableObservableVectorWrapper::First(_Outptr_ wfc::IIterator<IInspectable *> **value)
{
    if (!m_tpINCC || !OptionalChangeState::IsCollectionMoveNotificationsEnabled())
    {
        return BindableVectorWrapper::First(value);
    }

    IFCPTR_RETURN(value);
    *value = nullptr;
    IFC_RETURN(CheckMoveSourceUnchanged());
    ctl::ComPtr<wfc::IIterator<IInspectable *>> result;
    IFC_RETURN(ctl::ComObject<TrackerIterator<IInspectable *>>::CreateInstance(result.ReleaseAndGetAddressOf()));
    result.Cast<TrackerIterator<IInspectable *>>()->SetCollection(this);
    *value = result.Detach();
    return S_OK;
}

IFACEMETHODIMP BindableObservableVectorWrapper::SetAt(unsigned index, _In_opt_ IInspectable *item)
{
    IFC_RETURN(CheckMoveReentrancy());
    return BindableVectorWrapper::SetAt(index, item);
}

IFACEMETHODIMP BindableObservableVectorWrapper::InsertAt(unsigned index, _In_ IInspectable *item)
{
    IFC_RETURN(CheckMoveReentrancy());
    return BindableVectorWrapper::InsertAt(index, item);
}

IFACEMETHODIMP BindableObservableVectorWrapper::RemoveAt(unsigned index)
{
    IFC_RETURN(CheckMoveReentrancy());
    return BindableVectorWrapper::RemoveAt(index);
}

IFACEMETHODIMP BindableObservableVectorWrapper::Append(_In_opt_ IInspectable *item)
{
    IFC_RETURN(CheckMoveReentrancy());
    return BindableVectorWrapper::Append(item);
}

IFACEMETHODIMP BindableObservableVectorWrapper::RemoveAtEnd()
{
    IFC_RETURN(CheckMoveReentrancy());
    return BindableVectorWrapper::RemoveAtEnd();
}

IFACEMETHODIMP BindableObservableVectorWrapper::Clear()
{
    IFC_RETURN(CheckMoveReentrancy());
    return BindableVectorWrapper::Clear();
}

_Check_return_ HRESULT BindableObservableVectorWrapper::CheckMoveSourceUnchanged() const
{
    if (m_sourceChangedDuringMove)
    {
        IFC_RETURN(E_CHANGED_STATE);
    }
    return S_OK;
}

_Check_return_ HRESULT BindableObservableVectorWrapper::CheckMoveReentrancy() const
{
    if (m_moveView)
    {
        IFC_RETURN(ErrorHelper::OriginateError(
            E_ILLEGAL_METHOD_CALL,
            wrl_wrappers::HStringReference(L"The collection cannot be modified during a Move notification.").Get()));
    }
    return S_OK;
}

// IObservableVector<IInspectable *>
IFACEMETHODIMP BindableObservableVectorWrapper::add_VectorChanged(
    _In_ wfc::VectorChangedEventHandler<IInspectable *> *pHandler,
    _Out_ EventRegistrationToken *token)
{
    HRESULT hr = S_OK;

    ARG_NOTNULL(pHandler, "handler");
    ARG_VALIDRETURNPOINTER(token);

    m_vectorChangedHandlers.AddHandler(pHandler, token);

Cleanup:

    RRETURN(hr);
}

IFACEMETHODIMP BindableObservableVectorWrapper::remove_VectorChanged(
    EventRegistrationToken token)
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
    if (m_moveView)
    {
        // The app changed the original source during a synthetic notification. Invalidate
        // the projection and reconcile with a Reset after the current notification unwinds.
        m_sourceChangedDuringMove = true;
        return S_OK;
    }

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

    case wxaml_interop::NotifyCollectionChangedAction_Move:
        if (OptionalChangeState::IsCollectionMoveNotificationsEnabled())
        {
            IFC(ProcessCollectionMove(pArgs));
        }
        else
        {
            IFC(RaiseVectorChanged(wfc::CollectionChange_Reset, 0));
        }
        break;

    case wxaml_interop::NotifyCollectionChangedAction_Reset:
        IFC(RaiseVectorChanged(wfc::CollectionChange_Reset, 0));
        break;
    }

Cleanup:

    RRETURN(hr);
}

_Check_return_
HRESULT
BindableObservableVectorWrapper::ProcessCollectionMove(_In_ INotifyCollectionChangedEventArgs *pArgs)
{
    // A notification handler can replace ItemsSource and release the owner's adapter.
    const ctl::ComPtr<wfc::IVector<IInspectable *>> keepAlive(this);

    INT oldIndex = 0;
    INT newIndex = 0;
    UINT oldCount = 0;
    UINT newCount = 0;
    UINT sourceSize = 0;
    ctl::ComPtr<IBindableVector> oldItems;
    ctl::ComPtr<IBindableVector> newItems;

    IFC_RETURN(pArgs->get_OldStartingIndex(&oldIndex));
    IFC_RETURN(pArgs->get_NewStartingIndex(&newIndex));
    IFC_RETURN(pArgs->get_OldItems(&oldItems));
    IFC_RETURN(pArgs->get_NewItems(&newItems));
    if (oldItems)
    {
        IFC_RETURN(oldItems->get_Size(&oldCount));
    }
    if (newItems)
    {
        IFC_RETURN(newItems->get_Size(&newCount));
    }
    IFC_RETURN(BindableVectorWrapper::get_Size(&sourceSize));

    if (!Components::CollectionMoveView::IsValid(sourceSize, oldIndex, newIndex, oldCount, newCount))
    {
        IFC_RETURN(ErrorHelper::OriginateError(
            E_INVALIDARG,
            wrl_wrappers::HStringReference(
                L"A Move notification must specify valid old and new indices and equally sized, nonempty item ranges.").Get()));
    }
    if (oldIndex == newIndex)
    {
        return S_OK;
    }

    HRESULT hr = S_OK;
    bool resetRequired = false;
    {
        m_moveView.emplace(sourceSize, oldIndex, newIndex, oldCount);
        auto clearMoveView = wil::scope_exit([this]()
        {
            m_moveView.reset();
            m_sourceChangedDuringMove = false;
        });

        for (UINT removed = 0; removed < oldCount && SUCCEEDED(hr) && !m_sourceChangedDuringMove; ++removed)
        {
            m_moveView->RemoveNext();
            hr = RaiseVectorChanged(wfc::CollectionChange_ItemRemoved, oldIndex);
        }
        for (UINT inserted = 0; inserted < oldCount && SUCCEEDED(hr) && !m_sourceChangedDuringMove; ++inserted)
        {
            m_moveView->InsertNext();
            hr = RaiseVectorChanged(wfc::CollectionChange_ItemInserted, newIndex + inserted);
        }

        resetRequired = FAILED(hr) || m_sourceChangedDuringMove;
    }

    if (resetRequired)
    {
        // Restore the real source before notifying consumers, even if a handler failed.
        // Preserve the original language exception if the recovery notification also fails.
        ErrorInfo errorInfo;
        const bool restoreErrorInfo = FAILED(hr) && SUCCEEDED(errorInfo.GetFromThread());
        const HRESULT resetResult = RaiseVectorChanged(wfc::CollectionChange_Reset, 0);
        if (SUCCEEDED(hr))
        {
            hr = resetResult;
        }
        else if (restoreErrorInfo)
        {
            IGNOREHR(errorInfo.SetOnThread());
        }
    }

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


