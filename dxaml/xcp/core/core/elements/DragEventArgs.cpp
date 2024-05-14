// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "DragEventArgs.h"
#include "Activators.g.h"

#include <WRLHelper.h>
#include <FrameworkUdk/Containment.h>

// Bug 49668737: [1.4 Servicing] After dragging an FE Home item, all item drops on breadcrumbs don't support Move after first drag
#define LOCAL_WINAPPSDK_CHANGEID_49668737 49668737


// Initialize the DragEventArgs with any framework specific context.
_Check_return_ HRESULT CDragEventArgs::Create(_In_ CCoreServices* pCore, _Outptr_ CDragEventArgs** ppArgs, _In_opt_ IInspectable* pWinRtDragInfo, _In_opt_ IInspectable* pDragDropAsyncOperation)
{
    xref_ptr<CDragEventArgs> spArgs;

    spArgs.attach(new CDragEventArgs(pCore));

    IFC_RETURN(FxCallbacks::DragDrop_PopulateDragEventArgs(spArgs.get()));

    if(pWinRtDragInfo)
    {
        IFC_RETURN(spArgs->m_spWinRtDragInfo.reset(pWinRtDragInfo));
        if (WinAppSdk::Containment::IsChangeEnabled<LOCAL_WINAPPSDK_CHANGEID_49668737>())
        {
            wrl::ComPtr<mui::DragDrop::IDragInfo> dragInfo;
            if (SUCCEEDED(pWinRtDragInfo->QueryInterface(IID_PPV_ARGS(&dragInfo))))
            {
                wadt::DataPackageOperation allowedOperations;
                IFC_RETURN(dragInfo->get_AllowedOperations(&allowedOperations));
                IFC_RETURN(spArgs->put_AllowedOperations(static_cast<DirectUI::DataPackageOperation>(allowedOperations)));
            }
        }
    }

    if(pDragDropAsyncOperation)
    {
        IFC_RETURN(spArgs->m_spDragDropAsyncOperation.reset(pDragDropAsyncOperation));
    }

    *ppArgs = spArgs.detach();

    return S_OK;
}

_Check_return_ HRESULT GetDispatcherQueue(msy::IDispatcherQueue** dispatcherQueue)
{
    wrl::ComPtr<msy::IDispatcherQueueStatics> dispatcherQueueStatics;
    IFC_RETURN(wf::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Dispatching_DispatcherQueue).Get(),
        &dispatcherQueueStatics));
    IFC_RETURN(dispatcherQueueStatics->GetForCurrentThread(dispatcherQueue));
    return S_OK;
}

_Check_return_ HRESULT DelayRelease(IInspectable* inspectable)
{
    wrl::ComPtr<msy::IDispatcherQueue> dispatcherQueue;
    IFC_RETURN(GetDispatcherQueue(&dispatcherQueue));
    if (!dispatcherQueue.Get())
    {
        // We are already releasing off thread, so no need to delay the release.
        return S_OK;
    }

    wrl::ComPtr<IInspectable> inspectableCopy(inspectable);
    const auto releaseCallback = WRLHelper::MakeAgileCallback<msy::IDispatcherQueueHandler>(
        [inspectableCopy] () mutable {
            inspectableCopy = nullptr; // Final release
            return S_OK;
        });

    boolean enqueued = false;
    IFC_RETURN(dispatcherQueue->TryEnqueue(releaseCallback.Get(), &enqueued));
    IFCEXPECT_RETURN(enqueued);

    return S_OK;
}

CDragEventArgs::~CDragEventArgs()
{
    wrl::ComPtr<IInspectable> winrtDragInfoCopy(m_spWinRtDragInfo.get());
    m_spWinRtDragInfo.reset();

    // We need to ensure that the final release of the winrt drag info instance is delayed.
    // If not, the final release could also initiate another drag operation in the
    // same UI thread, but because Xaml is draining the clean up queue,
    // then OnReentrancyProtectedWindowMessage could be trigger and the app would crash.
    IFCFAILFAST(DelayRelease(winrtDragInfoCopy.Get()));
}

// Copy the given object into m_pData (pointer copy only), then
// peg the managed object reference contained within.
_Check_return_ HRESULT CDragEventArgs::put_Data(_In_ IInspectable* pNewData)
{
    return m_spData.reset(pNewData);
}

_Check_return_ HRESULT CDragEventArgs::get_Data(_Outptr_ IInspectable** ppData)
{
    m_spData.CopyTo(ppData);
    return S_OK;
}

_Check_return_ HRESULT CDragEventArgs::get_AcceptedOperation(_Out_ DirectUI::DataPackageOperation* pOperation)
{
    *pOperation = m_acceptedOperation;
    return S_OK;
}

// Internal method to update the accepted operation when InputManager is using different EventArgs
// in the same sequence
void CDragEventArgs::UpdateAcceptedOperation(_In_ DirectUI::DataPackageOperation operation)
{
    m_acceptedOperation = operation;
}

_Check_return_ HRESULT CDragEventArgs::put_AcceptedOperation(_In_ DirectUI::DataPackageOperation operation)
{
    m_acceptedOperation = operation;
    // Memorize the source responsible of setting AcceptedOperation to reset it on DragLeave
    if (m_spDragDropAsyncOperation && FxCallbacks::HasRaiseDragDropEventAsyncOperation_SetAcceptedOperationSetterUIElement())
    {
        IFC_RETURN(FxCallbacks::RaiseDragDropEventAsyncOperation_SetAcceptedOperationSetterUIElement(m_spDragDropAsyncOperation.get(), m_pSource));
    }
    return S_OK;
}

_Check_return_ HRESULT CDragEventArgs::GetWinRtDragInfo(_Outptr_result_maybenull_ IInspectable** ppWinRtDragInfo)
{
    m_spWinRtDragInfo.CopyTo(ppWinRtDragInfo);
    return S_OK;
}

_Check_return_ HRESULT CDragEventArgs::GetDragDropAsyncOperation(_Outptr_result_maybenull_ IInspectable** ppDragDropAsyncOperation)
{
    m_spDragDropAsyncOperation.CopyTo(ppDragDropAsyncOperation);
    return S_OK;
}

_Check_return_ HRESULT CDragEventArgs::GetIsDeferred(_In_ CCoreServices* core, _Out_ bool* isDeferred)
{
    *isDeferred = false;

    if (FxCallbacks::HasDragEventArgs_GetIsDeferred())
    {
        IFC_RETURN(FxCallbacks::DragEventArgs_GetIsDeferred(this, isDeferred));
    }

    return S_OK;
}

_Check_return_ HRESULT CDragEventArgs::CreateFrameworkPeer(_Outptr_ IInspectable** ppPeer)
{
    RRETURN(DirectUI::OnFrameworkCreateDragEventArgs(this, ppPeer));
}
