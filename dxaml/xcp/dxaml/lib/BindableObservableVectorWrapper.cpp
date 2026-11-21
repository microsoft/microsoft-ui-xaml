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
    if (!IsProjecting())
    {
        return BindableVectorWrapper::GetAt(index, item);
    }

    IFCPTR_RETURN(item);
    *item = nullptr;
    IFC_RETURN(CheckProjectedSourceUnchanged());
    if (index >= GetProjectedSize())
    {
        IFC_RETURN(E_BOUNDS);
    }

    ctl::ComPtr<IInspectable> value;
    IFC_RETURN(GetProjectedItem(index, &value));
    IFC_RETURN(CheckProjectedSourceUnchanged());
    return value.CopyTo(item);
}

IFACEMETHODIMP BindableObservableVectorWrapper::get_Size(_Out_ unsigned *size)
{
    if (!IsProjecting())
    {
        return BindableVectorWrapper::get_Size(size);
    }

    IFCPTR_RETURN(size);
    IFC_RETURN(CheckProjectedSourceUnchanged());
    *size = GetProjectedSize();
    return S_OK;
}

IFACEMETHODIMP BindableObservableVectorWrapper::GetView(
    _Outptr_result_maybenull_ wfc::IVectorView<IInspectable *> **view)
{
    // Only INCC can emit Move; preserve the source's own view for IBindableObservableVector-only sources.
    if (!m_rangeView && (!m_tpINCC || !OptionalChangeState::IsCollectionMoveNotificationsEnabled()))
    {
        return BindableVectorWrapper::GetView(view);
    }

    IFCPTR_RETURN(view);
    *view = nullptr;
    IFC_RETURN(CheckProjectedSourceUnchanged());
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
    if (!IsProjecting())
    {
        return BindableVectorWrapper::IndexOf(value, index, found);
    }

    IFCPTR_RETURN(index);
    IFCPTR_RETURN(found);
    *index = 0;
    *found = false;
    IFC_RETURN(CheckProjectedSourceUnchanged());

    if (m_rangeView)
    {
        // Use the source's own equality, as outside a range change; for example, a .NET source
        // finds an equal object or NaN that the comparison below would miss.
        unsigned sourceIndex = 0;
        boolean sourceFound = false;
        unsigned oldItemIndex = 0;
        boolean oldItemFound = false;
        IFC_RETURN(BindableVectorWrapper::IndexOf(value, &sourceIndex, &sourceFound));
        IFC_RETURN(CheckProjectedSourceUnchanged());
        if (m_rangeOldItems)
        {
            IFC_RETURN(m_rangeOldItems->IndexOf(value, &oldItemIndex, &oldItemFound));
            IFC_RETURN(CheckProjectedSourceUnchanged());
        }

        UINT projectedIndex = 0;
        switch (m_rangeView->ResolveIndexOf(!!sourceFound, sourceIndex, !!oldItemFound, oldItemIndex, &projectedIndex))
        {
        case Components::CollectionRangeChangeView::IndexOfResult::Found:
            *index = projectedIndex;
            *found = true;
            return S_OK;

        case Components::CollectionRangeChangeView::IndexOfResult::NotFound:
            return S_OK;

        case Components::CollectionRangeChangeView::IndexOfResult::Ambiguous:
            break;
        }

        for (unsigned current = 0; current < GetProjectedSize(); ++current)
        {
            const auto location = m_rangeView->GetLocation(current);
            // An equal item cannot precede the first match in its own collection.
            if (location.isOldItem
                    ? (!oldItemFound || location.index < oldItemIndex)
                    : (!sourceFound || location.index < sourceIndex))
            {
                continue;
            }

            ctl::ComPtr<IInspectable> item;
            unsigned firstIndex = 0;
            boolean itemFound = false;
            IFC_RETURN(GetProjectedItem(current, &item));
            IFC_RETURN(CheckProjectedSourceUnchanged());

            // Equal items have the same first match in their originating collection.
            // This preserves that collection's equality even when its first match is hidden.
            if (location.isOldItem)
            {
                IFC_RETURN(m_rangeOldItems->IndexOf(item.Get(), &firstIndex, &itemFound));
            }
            else
            {
                IFC_RETURN(BindableVectorWrapper::IndexOf(item.Get(), &firstIndex, &itemFound));
            }
            IFC_RETURN(CheckProjectedSourceUnchanged());
            if (itemFound && firstIndex == (location.isOldItem ? oldItemIndex : sourceIndex))
            {
                *index = current;
                *found = true;
                break;
            }
        }

        return S_OK;
    }

    for (unsigned current = 0; current < GetProjectedSize(); ++current)
    {
        ctl::ComPtr<IInspectable> item;
        bool equal = false;
        IFC_RETURN(GetProjectedItem(current, &item));
        IFC_RETURN(CheckProjectedSourceUnchanged());
        IFC_RETURN(PropertyValue::AreEqual(value, item.Get(), &equal));
        IFC_RETURN(CheckProjectedSourceUnchanged());
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
    // This wrapper also handles IBindableObservableVector without INCC. Such sources cannot
    // emit Move and must retain their original iterator semantics, even with the opt-in enabled.
    if (!m_rangeView && (!m_tpINCC || !OptionalChangeState::IsCollectionMoveNotificationsEnabled()))
    {
        return BindableVectorWrapper::First(value);
    }

    IFCPTR_RETURN(value);
    *value = nullptr;
    IFC_RETURN(CheckProjectedSourceUnchanged());
    ctl::ComPtr<wfc::IIterator<IInspectable *>> result;
    IFC_RETURN(ctl::ComObject<TrackerIterator<IInspectable *>>::CreateInstance(result.ReleaseAndGetAddressOf()));
    result.Cast<TrackerIterator<IInspectable *>>()->SetCollection(this);
    *value = result.Detach();
    return S_OK;
}

IFACEMETHODIMP BindableObservableVectorWrapper::SetAt(unsigned index, _In_opt_ IInspectable *item)
{
    IFC_RETURN(CheckProjectionReentrancy());
    return BindableVectorWrapper::SetAt(index, item);
}

IFACEMETHODIMP BindableObservableVectorWrapper::InsertAt(unsigned index, _In_ IInspectable *item)
{
    IFC_RETURN(CheckProjectionReentrancy());
    return BindableVectorWrapper::InsertAt(index, item);
}

IFACEMETHODIMP BindableObservableVectorWrapper::RemoveAt(unsigned index)
{
    IFC_RETURN(CheckProjectionReentrancy());
    return BindableVectorWrapper::RemoveAt(index);
}

IFACEMETHODIMP BindableObservableVectorWrapper::Append(_In_opt_ IInspectable *item)
{
    IFC_RETURN(CheckProjectionReentrancy());
    return BindableVectorWrapper::Append(item);
}

IFACEMETHODIMP BindableObservableVectorWrapper::RemoveAtEnd()
{
    IFC_RETURN(CheckProjectionReentrancy());
    return BindableVectorWrapper::RemoveAtEnd();
}

IFACEMETHODIMP BindableObservableVectorWrapper::Clear()
{
    IFC_RETURN(CheckProjectionReentrancy());
    return BindableVectorWrapper::Clear();
}

UINT BindableObservableVectorWrapper::GetProjectedSize() const
{
    return m_moveView ? m_moveView->GetSize() : m_rangeView->GetSize();
}

_Check_return_ HRESULT BindableObservableVectorWrapper::GetProjectedItem(UINT index, _Outptr_ IInspectable **item)
{
    if (m_moveView)
    {
        return BindableVectorWrapper::GetAt(m_moveView->GetSourceIndex(index), item);
    }

    const auto location = m_rangeView->GetLocation(index);
    if (location.isOldItem)
    {
        return m_rangeOldItems->GetAt(location.index, item);
    }
    return BindableVectorWrapper::GetAt(location.index, item);
}

_Check_return_ HRESULT BindableObservableVectorWrapper::CheckProjectedSourceUnchanged() const
{
    if (IsProjecting() && m_sourceChangedDuringProjection)
    {
        IFC_RETURN(E_CHANGED_STATE);
    }
    return S_OK;
}

_Check_return_ HRESULT BindableObservableVectorWrapper::CheckProjectionReentrancy() const
{
    if (IsProjecting())
    {
        IFC_RETURN_ORIGINATE_ERROR(
            E_ILLEGAL_METHOD_CALL,
            wrl_wrappers::HStringReference(L"The collection cannot be modified while a collection change notification is being processed.").Get());
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
    if (m_preparingProjection || IsProjecting())
    {
        // The source changed while preparing or projecting a Move or a multi-item change.
        // Reconcile with a Reset rather than forwarding a nested notification against a stale source state.
        m_sourceChangedDuringProjection = true;
        return S_OK;
    }

    HRESULT hr = S_OK;
    xaml_interop::NotifyCollectionChangedAction action;

    IFC(pArgs->get_Action(&action));

    switch (action)
    {
    case wxaml_interop::NotifyCollectionChangedAction_Add:
    case wxaml_interop::NotifyCollectionChangedAction_Remove:
    case wxaml_interop::NotifyCollectionChangedAction_Replace:
        IFC(ProcessCollectionItemsChange(action, pArgs));
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

    HRESULT hr = S_OK;
    bool resetRequired = false;
    INT oldIndex = 0;
    INT newIndex = 0;
    UINT oldCount = 0;
    UINT newCount = 0;
    UINT sourceSize = 0;
    ctl::ComPtr<IBindableVector> oldItems;
    ctl::ComPtr<IBindableVector> newItems;

    m_preparingProjection = true;

    IFC(pArgs->get_OldStartingIndex(&oldIndex));
    IFC(pArgs->get_NewStartingIndex(&newIndex));
    IFC(pArgs->get_OldItems(&oldItems));
    IFC(pArgs->get_NewItems(&newItems));
    if (oldItems)
    {
        IFC(oldItems->get_Size(&oldCount));
    }
    if (newItems)
    {
        IFC(newItems->get_Size(&newCount));
    }
    IFC(BindableVectorWrapper::get_Size(&sourceSize));

    // A nested change may alter the source size. Preserve intrinsic argument errors,
    // but reconcile instead of validating indices against the changed source.
    if (!Components::CollectionMoveView::HasValidArguments(oldIndex, newIndex, oldCount, newCount) ||
        (!m_sourceChangedDuringProjection &&
            !Components::CollectionMoveView::IsValid(sourceSize, oldIndex, newIndex, oldCount, newCount)))
    {
        IFC_ORIGINATE_ERROR(
            E_INVALIDARG,
            wrl_wrappers::HStringReference(
                L"A Move notification must specify valid old and new indices and equally sized, nonempty item ranges.").Get());
    }
    if (m_sourceChangedDuringProjection)
    {
        resetRequired = true;
        goto Cleanup;
    }
    if (oldIndex == newIndex)
    {
        goto Cleanup;
    }

    m_preparingProjection = false;
    {
        m_moveView.emplace(sourceSize, oldIndex, newIndex, oldCount);
        auto clearMoveView = wil::scope_exit([this]()
        {
            // IFC also leaves this scope before Cleanup raises a Reset.
            m_moveView.reset();
        });

        for (UINT removed = 0; removed < oldCount && !m_sourceChangedDuringProjection; ++removed)
        {
            m_moveView->RemoveNext();
            IFC(RaiseVectorChanged(wfc::CollectionChange_ItemRemoved, oldIndex));
        }
        for (UINT inserted = 0; inserted < oldCount && !m_sourceChangedDuringProjection; ++inserted)
        {
            m_moveView->InsertNext();
            IFC(RaiseVectorChanged(wfc::CollectionChange_ItemInserted, newIndex + inserted));
        }

        resetRequired = m_sourceChangedDuringProjection;
    }

Cleanup:
    m_preparingProjection = false;
    m_sourceChangedDuringProjection = false;
    if (FAILED(hr))
    {
        // Reset is best effort after a failure. Its handlers may replace error info
        // even when they succeed, so always restore the original error info.
        ErrorInfo errorInfo;
        const bool restoreErrorInfo = SUCCEEDED(errorInfo.GetFromThread());
        IGNOREHR(RaiseVectorChanged(wfc::CollectionChange_Reset, 0));
        if (restoreErrorInfo)
        {
            IGNOREHR(errorInfo.SetOnThread());
        }
    }
    else if (resetRequired)
    {
        // No prior error: a failed recovery Reset is the failure to return.
        hr = RaiseVectorChanged(wfc::CollectionChange_Reset, 0);
    }

    RRETURN(hr);
}

_Check_return_
HRESULT
BindableObservableVectorWrapper::ProcessCollectionItemsChange(
    xaml_interop::NotifyCollectionChangedAction action,
    _In_ INotifyCollectionChangedEventArgs *pArgs)
{
    // App code run while reading the payload, or a notification handler, can replace
    // ItemsSource and release the owner's adapter.
    const ctl::ComPtr<wfc::IVector<IInspectable *>> keepAlive(this);

    HRESULT hr = S_OK;
    INT index = 0;
    UINT oldCount = 0;
    UINT newCount = 0;
    ctl::ComPtr<IBindableVector> oldItems;
    ctl::ComPtr<IBindableVector> newItems;
    bool sourceChanged = false;

    {
        // That app code can also change the source. The nested change is not forwarded;
        // this change is reconciled with a Reset instead, as for Move.
        m_preparingProjection = true;
        auto endPreparation = wil::scope_exit([this, &sourceChanged]()
        {
            m_preparingProjection = false;
            sourceChanged = m_sourceChangedDuringProjection;
            m_sourceChangedDuringProjection = false;
        });

        if (action == wxaml_interop::NotifyCollectionChangedAction_Remove)
        {
            IFC(pArgs->get_OldStartingIndex(&index));
        }
        else
        {
            IFC(pArgs->get_NewStartingIndex(&index));
        }
        if (action != wxaml_interop::NotifyCollectionChangedAction_Add)
        {
            IFC(pArgs->get_OldItems(&oldItems));
            if (oldItems)
            {
                IFC(oldItems->get_Size(&oldCount));
            }
        }
        if (action != wxaml_interop::NotifyCollectionChangedAction_Remove)
        {
            IFC(pArgs->get_NewItems(&newItems));
            if (newItems)
            {
                IFC(newItems->get_Size(&newCount));
            }
        }
    }

    if (sourceChanged)
    {
        return RaiseVectorChanged(wfc::CollectionChange_Reset, 0);
    }

    // Single-item changes keep their original one-notification forwarding.
    switch (action)
    {
    case wxaml_interop::NotifyCollectionChangedAction_Add:
        if (!newItems || newCount == 1)
        {
            return RaiseVectorChanged(wfc::CollectionChange_ItemInserted, index);
        }
        break;

    case wxaml_interop::NotifyCollectionChangedAction_Remove:
        if (!oldItems || oldCount == 1)
        {
            return RaiseVectorChanged(wfc::CollectionChange_ItemRemoved, index);
        }
        break;

    default:
        if ((!oldItems && !newItems) || (oldCount == 1 && newCount == 1))
        {
            return RaiseVectorChanged(wfc::CollectionChange_ItemChanged, index);
        }
        break;
    }

    // A multi-item change is expanded into one notification per item, because each vector
    // notification describes exactly one item.
    return ProcessCollectionRangeChange(index, oldItems.Get(), oldCount, newItems.Get(), newCount);

Cleanup:
    // Only failures reach here. The change was not forwarded, so the view must be reconciled
    // whether or not the source also changed while the payload was read.
    // Reset is best effort; keep the payload failure and its error info.
    {
        ErrorInfo errorInfo;
        const bool restoreErrorInfo = SUCCEEDED(errorInfo.GetFromThread());
        IGNOREHR(RaiseVectorChanged(wfc::CollectionChange_Reset, 0));
        if (restoreErrorInfo)
        {
            IGNOREHR(errorInfo.SetOnThread());
        }
    }

    RRETURN(hr);
}

_Check_return_
HRESULT
BindableObservableVectorWrapper::ProcessCollectionRangeChange(
    INT index,
    _In_opt_ IBindableVector *pOldItems,
    UINT oldCount,
    _In_opt_ IBindableVector *pNewItems,
    UINT newCount)
{
    HRESULT hr = S_OK;
    bool resetRequired = false;
    UINT sourceSize = 0;

    m_preparingProjection = true;

    IFC(BindableVectorWrapper::get_Size(&sourceSize));

    // Without a valid range, or after a nested change, the intermediate states cannot be
    // described consistently, so reconcile with a Reset.
    if (m_sourceChangedDuringProjection ||
        !Components::CollectionRangeChangeView::IsValid(sourceSize, index, newCount))
    {
        resetRequired = true;
        goto Cleanup;
    }

    m_preparingProjection = false;
    {
        m_rangeOldItems = pOldItems;
        m_rangeView.emplace(sourceSize, index, oldCount, newCount);
        auto clearRangeView = wil::scope_exit([this]()
        {
            // IFC also leaves this scope before Cleanup raises a Reset.
            m_rangeView.reset();
            m_rangeOldItems.Reset();
        });

        // Old items are replaced in place first; the remaining old items are then removed,
        // or the remaining new items inserted.
        const UINT replacedCount = oldCount < newCount ? oldCount : newCount;
        for (UINT replaced = 0; replaced < replacedCount && !m_sourceChangedDuringProjection; ++replaced)
        {
            m_rangeView->ReplaceNext();
            IFC(RaiseVectorChanged(wfc::CollectionChange_ItemChanged, index + replaced));
        }
        for (UINT removed = replacedCount; removed < oldCount && !m_sourceChangedDuringProjection; ++removed)
        {
            m_rangeView->RemoveNext();
            IFC(RaiseVectorChanged(wfc::CollectionChange_ItemRemoved, index + replacedCount));
        }
        for (UINT inserted = replacedCount; inserted < newCount && !m_sourceChangedDuringProjection; ++inserted)
        {
            m_rangeView->InsertNext();
            IFC(RaiseVectorChanged(wfc::CollectionChange_ItemInserted, index + inserted));
        }

        resetRequired = m_sourceChangedDuringProjection;
    }

Cleanup:
    m_preparingProjection = false;
    m_sourceChangedDuringProjection = false;
    if (FAILED(hr))
    {
        // Reset is best effort after a failure. Its handlers may replace error info
        // even when they succeed, so always restore the original error info.
        ErrorInfo errorInfo;
        const bool restoreErrorInfo = SUCCEEDED(errorInfo.GetFromThread());
        IGNOREHR(RaiseVectorChanged(wfc::CollectionChange_Reset, 0));
        if (restoreErrorInfo)
        {
            IGNOREHR(errorInfo.SetOnThread());
        }
    }
    else if (resetRequired)
    {
        // No prior error: a failed recovery Reset is the failure to return.
        hr = RaiseVectorChanged(wfc::CollectionChange_Reset, 0);
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
