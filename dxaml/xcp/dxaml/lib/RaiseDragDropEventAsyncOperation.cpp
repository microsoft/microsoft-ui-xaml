// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "RaiseDragDropEventAsyncOperation.h"
#include "DragDropInternal.h"
#include "ListViewBase.g.h"
#include "DragVisual_partial.h"

using namespace DirectUI;

ULONG RaiseDragDropEventAsyncOperation::z_ulUniqueAsyncActionId = 1;

_Check_return_ HRESULT DelayRelease(IInspectable* inspectable);

RaiseDragDropEventAsyncOperation::~RaiseDragDropEventAsyncOperation()
{
    // We need to ensure that the final release of the winrt drag info instance is delayed.
    // If not, the final release could also initiante another drag operation in the
    // same UI thread, but becuase Xaml is draining the clean up queue,
    // then OnReentrancyProtectedWindowMessage could be trigger and the app would crash.
    IFCFAILFAST(DelayRelease(m_spDragInfo.Get()));
    IFCFAILFAST(DelayRelease(m_spDragUIOverride.Get()));
}

_Check_return_ HRESULT
RaiseDragDropEventAsyncOperation::OnStart()
{
    wrl::ComPtr<wf::IAsyncAction> spAction;
    auto completionCallback = wrl::Callback<wf::IAsyncActionCompletedHandler>(this, &RaiseDragDropEventAsyncOperation::OnRaiseDragDropCompleted);

    IFC_RETURN(m_spDropTarget->RaiseDragDropEventActionAsync(
        m_type,
        m_spDragInfo.Get(),
        this,
        &spAction));

    m_spThis = this;

    const HRESULT hr = spAction->put_Completed(completionCallback.Get());
    if (FAILED(hr))
    {
        // We need to handle errors in case spAction completed synchronously.
        m_spThis = nullptr;
        IFC_RETURN(hr);
    }

    return S_OK;
}


_Check_return_ HRESULT
RaiseDragDropEventAsyncOperation::OnRaiseDragDropCompleted(_In_opt_ wf::IAsyncAction*, wf::AsyncStatus status)
{
    m_spThis = nullptr;

    if (status == wf::AsyncStatus::Completed)
    {
        if (m_deferralCount == 0)
        {
            IFC_RETURN(SetupCustomDragVisual());
        }
        else
        {
            m_wasDeferred = true;
        }
    }
    else
    {
        switch (status)
        {
        case wf::AsyncStatus::Canceled:
            Cancel();
            break;
        case wf::AsyncStatus::Error:
            TryTransitionToError(E_FAIL);
            break;
        }

        IFC_RETURN(AsyncBase::FireCompletion());
    }

    return S_OK;
}


_Check_return_ HRESULT
RaiseDragDropEventAsyncOperation::SetupCustomDragVisual()
{
    HRESULT hr = S_OK;

    if ((m_spDragVisual != nullptr) && !m_spDragVisual->IsRendered())
    {
        wrl::ComPtr<wf::IAsyncAction> spAction;
        auto completionCallback = wrl::Callback<wf::IAsyncActionCompletedHandler>(this, &RaiseDragDropEventAsyncOperation::OnCustomVisualSetUp);

        IFC(m_spDragVisual->RenderAsync(
            spAction.ReleaseAndGetAddressOf()));

        IFC(spAction->put_Completed(completionCallback.Get()));

        m_spThis = this;
    }
    else
    {
        // Call OnCustomVisualSetup synchronously
        IFC(OnCustomVisualSetUp(nullptr, wf::AsyncStatus::Completed));
    }

Cleanup:
    return hr;
}

_Check_return_ HRESULT
RaiseDragDropEventAsyncOperation::OnCustomVisualSetUp(_In_opt_ wf::IAsyncAction*, _In_ wf::AsyncStatus status)
{
    HRESULT hr = S_OK;

    if (status == wf::AsyncStatus::Completed)
    {
        auto currentDragDrop = DXamlCore::GetCurrent()->GetDragDrop();
        if (m_spDragUIOverride != nullptr)
        {
            // Drop target needs to set DragVisual on the eventargs in order to set the custom visual, otherwise, dragged visual
            // will not be changed.
            if (m_spDragVisual != nullptr)
            {
                ctl::ComPtr<wgri::ISoftwareBitmap> spSoftwareBitmap;
                IFC(m_spDragVisual->GetSoftwareBitmap(spSoftwareBitmap.GetAddressOf()));
                if (m_hasAnchorPoint)
                {
                    IFC(m_spDragUIOverride->SetContentFromSoftwareBitmap2(spSoftwareBitmap.Get(), m_anchorPoint));
                }
                else
                {
                    IFC(m_spDragUIOverride->SetContentFromSoftwareBitmap(spSoftwareBitmap.Get()));
                }
            }
            else
            {
                if (currentDragDrop->ShouldClearCustomVisual())
                {
                    IFC(m_spDragUIOverride->Clear());
                }
            }
        }
        else // if (m_spCoreDragUIOverride == nullptr)
        {
            // Clean up on core Drop/Leave
            currentDragDrop->SetCustomVisualSetterUIElement(nullptr);
        }
    }
    else if (status == wf::AsyncStatus::Canceled)
    {
        Cancel();
    }
    else
    {
        //wf::AsyncStatus::Error:
        TryTransitionToError(E_FAIL);
    }


    IFC_RETURN(AsyncBase::FireCompletion());

Cleanup:
    m_spThis = nullptr;
    return S_OK;
}


// This RPInvoke is called in DragLeave to check if the UIElement we are leaving had set the AcceptedOperation
/*static*/ _Check_return_ HRESULT
RaiseDragDropEventAsyncOperation::CheckIfAcceptedOperationShouldBeCleared(_In_ IInspectable* pOperation, _In_opt_ CDependencyObject* pSource)
{
    RaiseDragDropEventAsyncOperation* pRaiseDragDropEventAsyncOperationNoRef = reinterpret_cast<RaiseDragDropEventAsyncOperation*>(pOperation);
    ctl::ComPtr<DependencyObject> spUIElementAsDO;

    if (SUCCEEDED(DXamlCore::GetCurrent()->GetPeer(pSource, &spUIElementAsDO)))
    {
        // Some DO such as RootVisual doesn't have a framework peer, in that case, we will skip the check
        pRaiseDragDropEventAsyncOperationNoRef->m_spDropTarget->CheckIfAcceptedOperationShouldBeReset(ctl::as_iinspectable(spUIElementAsDO.Get()));
    }
    return S_OK;
}


// This RPInvoke is called by DragEventArgs when the accepted operation if set
/*static*/ _Check_return_ HRESULT
RaiseDragDropEventAsyncOperation::SetAcceptedOperationSetterUIElement(_In_ IInspectable* pOperation, _In_opt_ CDependencyObject* pSource)
{
    RaiseDragDropEventAsyncOperation* pRaiseDragDropEventAsyncOperationNoRef = reinterpret_cast<RaiseDragDropEventAsyncOperation*>(pOperation);
    ctl::ComPtr<DependencyObject> spUIElementAsDO;

    if (SUCCEEDED(DXamlCore::GetCurrent()->GetPeer(pSource, &spUIElementAsDO)))
    {
        // Some DO such as RootVisual doesn't have a framework peer, in that case, we will skip the check
        pRaiseDragDropEventAsyncOperationNoRef->m_spDropTarget->SetAcceptedOperationSetterUIElement(ctl::as_iinspectable(spUIElementAsDO.Get()));
    }
    return S_OK;
}

IFACEMETHODIMP
RaiseDragDropEventAsyncOperation::SetAcceptedOperation(
_In_ IInspectable* source,
_In_ wadt::DataPackageOperation acceptedOperation)
{
    m_spDropTarget->SetAcceptedOperation(source, acceptedOperation);
    return S_OK;
}
