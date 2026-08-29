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

// When we communicate with DataExchangeHost via cross process calls (e.g. to get drag arg values), COM will use an inner message
// loop and pump messages.  If DataExchangeHost hands off too many drag event callbacks to the UI thread too fast, this inner
// message loop can process additional drag events reentrantly, which will/can corrupt our drag data.  When this occurs, we need to defer 
// the processing of the reentrant event until after the current event handling has completed.  This structure is used to hold
// on to the event data until it can be processed.
struct DirectUI::PendingDragDropActionData
{
    DragDropMessageType type;
    Microsoft::WRL::ComPtr<SynchronousDragDropAction> spDragDropAction;
    ctl::ComPtr<DropOperationTarget> spDropOperationTarget;
    ctl::ComPtr<mui::DragDrop::IDragInfo> spDragInfo;
    Microsoft::WRL::ComPtr<RaiseDragDropEventAsyncOperation> spRaiseDragDropEventAsyncOperation;

    PendingDragDropActionData(
        SynchronousDragDropAction* dragDropAction,
        DragDropMessageType messageType,
        DropOperationTarget* dropOperationTarget,
        mui::DragDrop::IDragInfo* dragInfo,
        RaiseDragDropEventAsyncOperation* raiseDragDropEventAsyncOperation) :
        spDragDropAction(dragDropAction),
        type(messageType),
        spDropOperationTarget(dropOperationTarget),
        spDragInfo(dragInfo),
        spRaiseDragDropEventAsyncOperation(raiseDragDropEventAsyncOperation)
    {
    }

    _Check_return_ HRESULT Invoke()
    {
        IFC_RETURN(spDragDropAction->StartOperation());
        IFC_RETURN(spDropOperationTarget->ProcessDragDropEventAction(type, spDragInfo.Get(), spRaiseDragDropEventAsyncOperation.Get()));
        spDragDropAction->CoreFireCompletion();
        return S_OK;
    }
};

// Thread local data used to manage a quue of pending drag events due to reentrancy.
thread_local bool DropOperationTarget::tls_actionInProgress = false;
thread_local std::unique_ptr<PendingDragDropActionQueue> DropOperationTarget::tls_pendingActions;

// DataExchangeHost does not guarentee that leave event for one drop target will be ahead of
// the enter event for another drop target on the same thread.  If the come in the wrong order
// it will/can corrupt our drag data.  This thread local variable, allows us to keep track
// of which drop target we currently think we are inside of so we can make sure we process these
// events in the right order.
thread_local ctl::ComPtr<DropOperationTarget> DropOperationTarget::tls_activeDropOperationTarget;

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
    if (!tls_pendingActions)
    {
        tls_pendingActions = std::make_unique<PendingDragDropActionQueue>();
    }

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
    mui::DragDrop::IDragInfo* pDragInfo,
    mui::DragDrop::IDragUIOverride* pDragUIOverride,
    wf::IAsyncOperation<wadt::DataPackageOperation>** ppReturnValue)
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
    mui::DragDrop::IDragInfo* pDragInfo,
    mui::DragDrop::IDragUIOverride* pDragUIOverride,
    wf::IAsyncOperation<wadt::DataPackageOperation>** ppReturnValue)
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
    mui::DragDrop::IDragInfo* pDragInfo,
    wf::IAsyncAction** ppReturnValue)
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
    mui::DragDrop::IDragInfo* pDragInfo,
    wf::IAsyncOperation<wadt::DataPackageOperation>** ppReturnValue)
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
    // We always create the SynchronousDragDropAction as we need to return that (even from the nested call), but if we are
    // already in the process of processing one of them (or one is in flight through the dispatcher), then we will will use
    // the dispatcher to schedule it to be processed when the current item(s) is complete.
    static unsigned actionId = 0;
    Microsoft::WRL::ComPtr<SynchronousDragDropAction> syncDragDropAction;
    IFC_RETURN(Microsoft::WRL::MakeAndInitialize<SynchronousDragDropAction>(&syncDragDropAction, ++actionId, nullptr /* dispatcher */));
    
    // When starting an action, we will end up accessing properties on the DragInfo object.  This becomes problematic because
    // this can cause a cross procss COM call which will pump messages and we could get a second drag/drop event while we
    // are still processing this one.  when this happens we will queue up the second (or third) async action and start/process
    // it when we call ourselves back via the dispacher.  This allows us to return the current "completed" action to the data transfer
    // code before we start (and complete) the second (or third) actions.
    if (tls_actionInProgress || !tls_pendingActions->empty())
    {
        // This will be the first pending entry so dispatch the handler for it.
        if (tls_pendingActions->empty())
        {
            IFC_RETURN(QueuePendingActionCallback());
        }

        // Create a pending item and add it to the queue
        tls_pendingActions->emplace_front(std::make_unique<PendingDragDropActionData>(syncDragDropAction.Get(), type, this, pDragInfo, pRaiseDragDropEventAsyncOperation));
    }
    else
    {
        // There were no other actions running (or dispatched) so we can just start this one.
        IFC_RETURN(syncDragDropAction->StartOperation());
        IFC_RETURN(ProcessDragDropEventAction( type, pDragInfo, pRaiseDragDropEventAsyncOperation));
        syncDragDropAction->CoreFireCompletion();
    }

    *ppReturnValue = syncDragDropAction.Detach();
    return S_OK;
}

_Check_return_ HRESULT DropOperationTarget::ProcessDragDropEventAction(
    _In_ DragDropMessageType type,
    _In_ mui::DragDrop::IDragInfo* pDragInfo,
    _In_opt_ RaiseDragDropEventAsyncOperation* pRaiseDragDropEventAsyncOperation
    )
{
    IFCPTR_RETURN(pDragInfo);

    // IF we don't have a core, then this async action has come in after shutdown, so don't do anything
    auto dxamlCore = DXamlCore::GetCurrent();
    if (!dxamlCore) return S_OK;

    // Indicate that we are processing an action so we can catch reentrancy
    tls_actionInProgress = true;
    auto resetInProgress = wil::scope_exit([]()
        {
            tls_actionInProgress = false;
        });

    // It does not appear that drag/drop guarentees the order in when we get events when multiple content islands are involved.  We
    // will sometimes get the leave for one island before the enter for the next, but sometimes we will get the enter for new island
    // and even an over, before we get the leave for the old one.  This latter scenario can corrupt our drag/drop state and prevent 
    // us from properly raising events.  So to combat this, we will track our current drop operation target and if we get and enter for
    // a new one without having gotten a leave for the current one, we will artificially generate the leave before processing the enter.
    // Conversly, if we get a leave for a drop operation target that isn't current, we will ignore it.

    if (tls_activeDropOperationTarget.Get() != this)
    {
        if (tls_activeDropOperationTarget)
        {
            if (type == DragDropMessageType::DragLeave)
            {
                // We must have generated a leave event so ignore this one.
                return S_OK;
            }
            // We must have a enter, but we haven't left the previous island yet, so generate a leave.
            ASSERT(type == DragDropMessageType::DragEnter);
            IFC_RETURN(tls_activeDropOperationTarget->ProcessDragDropEventAction(DragDropMessageType::DragLeave, pDragInfo, nullptr));
        }
        tls_activeDropOperationTarget = this;
    }
    if (type == DragDropMessageType::DragLeave || type == DragDropMessageType::Drop) 
    {
        tls_activeDropOperationTarget = nullptr;
    }

    wf::Point point = {};
    XPOINTF pointF = {};
    ctl::ComPtr<xaml_controls::IListViewBase> spCapturedListViewBase;
    DirectUI::DataPackageOperation acceptedOperation = m_acceptedOperation;

    xref_ptr<CXamlIslandRoot> hitTestRootIsland = m_hitTestRootIslandWeak.lock();

    IFC_RETURN(pDragInfo->get_Position(&point));

    // If we are in a popup or some other secondary windows, then adjust our point to be relative to our content root.
    if (m_dragDropPointTransform)
    {
        auto rawPoint = point;
        BOOLEAN result;
        IFC_RETURN(m_dragDropPointTransform->TryTransform(rawPoint, &point, &result));
        if (!result) IFC_RETURN(E_FAIL);
    }

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
        contentRoot = dxamlCore->GetHandle()->GetContentRootCoordinator()->Unsafe_IslandsIncompatible_CoreWindowContentRoot();
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

    return S_OK;
}

HRESULT DropOperationTarget::QueuePendingActionCallback()
{
    ctl::ComPtr<DirectUI::IDispatcher> spDispatcher;
    if (SUCCEEDED(DXamlCore::GetCurrent()->GetXamlDispatcher(&spDispatcher)) && spDispatcher)
    {
        IFC_RETURN(spDispatcher->RunAsync(MakeCallback(this, &DropOperationTarget::RaisePendingDragDropEventAction)));
    }
    return S_OK;
}

HRESULT DropOperationTarget::RaisePendingDragDropEventAction()
{
    // This shouldn't ever happen, but if we don't have any pending actions, then we don't 
    // need to do anything.
    ASSERT(!tls_pendingActions->empty());
    if (tls_pendingActions->empty())
    {
        return S_OK;
    }

    // If we are still processing the previous action, then reschedule.
    if (tls_actionInProgress)
    {
        IFC_RETURN(QueuePendingActionCallback());
        return S_OK;
    }

    // Get the next action
    std::unique_ptr<PendingDragDropActionData> pendingAction;
    pendingAction.swap(tls_pendingActions->front());
    tls_pendingActions->pop_front();

    // If there are still more actions to process, then reschedule to handle the next one.
    // We do this, rather than processing the all, becuase thereis an additional re-entrancy
    // problem with non drag drop messages, so this gives them a chance to execute between
    // drag/drop actions reducing the likelihood they will occur during the action via
    // reentrancy.  Note, this doesn't not elivate the secondary reentrancy issue, but instead
    // just attempts to reduce the number of times it is encountered.
    if (!tls_pendingActions->empty())
    {
        IFC_RETURN(QueuePendingActionCallback());
    }

    // Process the action
    IFC_RETURN(pendingAction->Invoke());

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
