// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "DropOperationTarget.h"
#include "DragDropInternal.h"
#include "ListViewBase.g.h"
#include "RaiseDragDropEventAsyncOperation.h"
#include "microsoft.ui.input.dragdrop.h"

using namespace DirectUI;

extern __declspec(selectany) const WCHAR SynchronousDragDropActionName[] = L"Windows.Foundation.IAsyncAction DropOperationTarget.RaiseDragDropEventActionAsync";
typedef DXamlAsyncActionBase<Microsoft::WRL::AsyncCausalityOptions<SynchronousDragDropActionName>> SynchronousDragDropAction;

_Check_return_ HRESULT
DropOperationTarget::QueryInterfaceImpl(
    _In_ REFIID iid,
    _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(mui::DragDrop::IDropOperationTarget)))
    {
        *ppObject = static_cast<mui::DragDrop::IDropOperationTarget*>(this);
    }
    else
    {
        return ComBase::QueryInterfaceImpl(iid, ppObject);
    }

    AddRefOuter();
    RRETURN(S_OK);
}

HRESULT DropOperationTarget::Initialize(_In_ CXamlIslandRoot* hitTestRootIsland)
{
    m_hitTestRootIslandWeak = xref::get_weakref(hitTestRootIsland);
    return S_OK;
}

//--------------------------------- ----------------------------------------
//
//  Function:   OnDragEnter
//
//  Synopsis:   Called when a drag operation enters the current view.
//
//-------------------------------------------------------------------------
IFACEMETHODIMP
DropOperationTarget::EnterAsync(
    _In_ mui::DragDrop::IDragInfo* pDragInfo,
    _In_ mui::DragDrop::IDragUIOverride* pDragUIOverride,
    _Deref_out_ wf::IAsyncOperation<wadt::DataPackageOperation>** ppReturnValue)
{
    IFCPTR_RETURN(pDragInfo);
    IFCPTR_RETURN(pDragUIOverride);

    m_shouldResetAcceptedOperation = false;
    m_wrAcceptedOperationSetterIInspectable  = nullptr;
    m_acceptedOperation = DirectUI::DataPackageOperation::DataPackageOperation_None;

    IFC_RETURN(RaiseDragDropEventOperationAsync(DragDropMessageType::DragEnter, pDragInfo, pDragUIOverride, ppReturnValue));

    return S_OK;
}


//-------------------------------------------------------------------------
//
//  Function:   OnDragOver
//
//  Synopsis:   Called when a drag operation moves over the current view.
//
//-------------------------------------------------------------------------
IFACEMETHODIMP
DropOperationTarget::OverAsync(
    _In_ mui::DragDrop::IDragInfo* pDragInfo,
    _In_ mui::DragDrop::IDragUIOverride* pDragUIOverride,
    _Deref_out_ wf::IAsyncOperation<wadt::DataPackageOperation>** ppReturnValue)
{
    HRESULT hr = S_OK;

    IFCPTR(pDragInfo);
    IFCPTR(pDragUIOverride);

    IFC(RaiseDragDropEventOperationAsync(DragDropMessageType::DragOver, pDragInfo, pDragUIOverride, ppReturnValue));

Cleanup:
    return hr;
}

//--------------------------------- ----------------------------------------
//
//  Function:   OnDragLeave
//
//  Synopsis:   Called when a drag operation leaves the current view
//
//-------------------------------------------------------------------------
IFACEMETHODIMP
DropOperationTarget::LeaveAsync(
    _In_ mui::DragDrop::IDragInfo* pDragInfo,
    _Outptr_ wf::IAsyncAction** ppReturnValue)
{
    HRESULT hr = S_OK;

    IFCPTR(pDragInfo);

    IFC(RaiseDragDropEventActionAsync(DragDropMessageType::DragLeave, pDragInfo, NULL /* pRaiseDragDropEventAsyncOperation */, ppReturnValue));

Cleanup:
    return hr;
}


//--------------------------------- ----------------------------------------
//
//  Function:   OnDrop
//
//  Synopsis:   Called when a drop occurs on the current view
//
//-------------------------------------------------------------------------
IFACEMETHODIMP
DropOperationTarget::DropAsync(
    _In_ mui::DragDrop::IDragInfo* pDragInfo,
    _Outptr_ wf::IAsyncOperation<wadt::DataPackageOperation>** ppReturnValue)
{
    HRESULT hr = S_OK;

    IFCPTR(pDragInfo);

    IFC(RaiseDragDropEventOperationAsync(DragDropMessageType::Drop, pDragInfo, NULL /* pDragUIOverride */, ppReturnValue));

Cleanup:
    return hr;
}

// This is called by EnterAsync, OverAsync and DropAsync
_Check_return_ HRESULT
DropOperationTarget::RaiseDragDropEventOperationAsync(
    _In_ DragDropMessageType type,
    _In_ mui::DragDrop::IDragInfo* pDragInfo,
    _In_opt_ mui::DragDrop::IDragUIOverride* pDragUIOverride,
    _Deref_out_ wf::IAsyncOperation<wadt::DataPackageOperation>** ppReturnValue)
{
    HRESULT hr = S_OK;

    wrl::ComPtr<RaiseDragDropEventAsyncOperation> spRaiseDragDropEventAsyncOperation;

    IFCPTR(pDragInfo);

    IFC(wrl::MakeAndInitialize<RaiseDragDropEventAsyncOperation>(
        &spRaiseDragDropEventAsyncOperation,
        type,
        this,
        pDragInfo,
        pDragUIOverride));

    IFC(spRaiseDragDropEventAsyncOperation->StartOperation());
    *ppReturnValue = spRaiseDragDropEventAsyncOperation.Detach();

Cleanup:
    return hr;
}

HRESULT DropOperationTarget::RaiseDragDropEventActionAsync(
    _In_ DragDropMessageType type,
    _In_ mui::DragDrop::IDragInfo* pDragInfo,
    _In_opt_ RaiseDragDropEventAsyncOperation* pRaiseDragDropEventAsyncOperation,
    _Outptr_ wf::IAsyncAction** ppReturnValue)
{
    IFCPTR_RETURN(pDragInfo);

    // IF we don't have a core, then this async action has come in after shutdown, so don't do anything
    auto dxamlCore = DXamlCore::GetCurrent();
    if (!dxamlCore) return S_OK;

    wf::Point point = {};
    XPOINTF pointF = {};
    ctl::ComPtr<xaml_controls::IListViewBase> spCapturedListViewBase;
    DirectUI::DataPackageOperation acceptedOperation = m_acceptedOperation;

    xref_ptr<CXamlIslandRoot> hitTestRootIsland = m_hitTestRootIslandWeak.lock();

    IFC_RETURN(pDragInfo->get_Position(&point));

    // Forward the position to the captured ListViewBase. In M1, XAML is still responsible for rendering the drag visual
    // this will be removed once WinRT API supports setting the DragUI.
    IFC_RETURN(dxamlCore->GetDragDrop()->GetCapturedDragSource(&spCapturedListViewBase));
    if (spCapturedListViewBase.Get())
    {
        IFC_RETURN(spCapturedListViewBase.Cast<ListViewBase>()->UpdateDragPositionOnCapturedTarget(type, point));
    }

    pointF.x = static_cast<XFLOAT>(point.X);
    pointF.y = static_cast<XFLOAT>(point.Y);

    CContentRoot* contentRoot = nullptr;

    if (hitTestRootIsland == nullptr)
    {
        contentRoot = dxamlCore->GetHandle()->GetContentRootCoordinator()->Unsafe_XamlIslandsIncompatible_CoreWindowContentRoot();
    }
    else
    {
        contentRoot = VisualTree::GetContentRootForElement(hitTestRootIsland);
    }

    IFC_RETURN(CoreImports::DragDrop_RaiseEvent(contentRoot, static_cast<CCoreServices*>(dxamlCore->GetHandle()), type, pointF, TRUE /*raiseDragEventsSync*/,
                                     pDragInfo, static_cast<wf::IAsyncOperation<wadt::DataPackageOperation>*>(pRaiseDragDropEventAsyncOperation),
                                     &acceptedOperation, hitTestRootIsland.get()));

    if (pRaiseDragDropEventAsyncOperation != nullptr)
    {
        if (m_shouldResetAcceptedOperation)
        {
            // We have left the UIElement which had set AcceptedOperation and NO other UIElement
            // has set it since (either DragLeave was the last event or nobody was "interested")
            acceptedOperation = DirectUI::DataPackageOperation::DataPackageOperation_None;
        }
        pRaiseDragDropEventAsyncOperation->SetAcceptedOperation(static_cast<wadt::DataPackageOperation>(acceptedOperation));
        m_acceptedOperation = acceptedOperation;
    }


    // We can handle drag events synchronously, but we still need to return an async action.
    // Let's build one that completes synchronously.
    {
        static unsigned actionId = 0;
        Microsoft::WRL::ComPtr<SynchronousDragDropAction> syncDragDropAction;

        IFC_RETURN(Microsoft::WRL::MakeAndInitialize<SynchronousDragDropAction>(&syncDragDropAction, ++actionId, nullptr /* dispatcher */));
        IFC_RETURN(syncDragDropAction->StartOperation());
        syncDragDropAction->CoreFireCompletion();

        *ppReturnValue = syncDragDropAction.Detach();
    }

    return S_OK;
}

void DropOperationTarget::CheckIfAcceptedOperationShouldBeReset(_In_ IInspectable* sourceAsInspectable)
{
    ctl::ComPtr<IInspectable> spAcceptedOperationSetterIInspectable;

    IFCFAILFAST(m_wrAcceptedOperationSetterIInspectable.As(&spAcceptedOperationSetterIInspectable));
    if (!spAcceptedOperationSetterIInspectable || ctl::are_equal(spAcceptedOperationSetterIInspectable.Get(), sourceAsInspectable))
    {
        m_shouldResetAcceptedOperation = true;
        m_wrAcceptedOperationSetterIInspectable = nullptr;
    }
}

void DropOperationTarget::SetAcceptedOperationSetterUIElement(_In_ IInspectable* sourceAsInspectable)
{
    // Set custom visual setter and do not clear override once this is called
    m_shouldResetAcceptedOperation = false;
    IFCFAILFAST(ctl::AsWeak(sourceAsInspectable, &m_wrAcceptedOperationSetterIInspectable));
}

void DropOperationTarget::SetAcceptedOperation(
    _In_ IInspectable* sourceAsInspectable,
    _In_ wadt::DataPackageOperation acceptedOperation)
{
    m_acceptedOperation = static_cast<DirectUI::DataPackageOperation>(acceptedOperation);
    SetAcceptedOperationSetterUIElement(sourceAsInspectable);
}
