// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "DragStartingEventArgs.g.h"
#include "StartDragAsyncOperation.h"
#include "DragOperationDeferral_partial.h"
#include "PointerPointTransform.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT
DragStartingEventArgs::SetOperationContext(
    _In_ wf::Point position,
    _In_ wf::IAsyncOperation<wadt::DataPackageOperation> *pStartDragOperation,
    _In_ wadt::IDataPackage* pDataPackage,
    _In_ ixp::IPointerPoint* pPointerPoint)
{
    m_spStartDragAsyncOperation = pStartDragOperation;
    m_position = position;
    IFC_RETURN(put_Data(pDataPackage));
    m_pointerPoint = pPointerPoint;

    return S_OK;
}

_Check_return_ HRESULT
DragStartingEventArgs::get_DragUIImpl(_COM_Outptr_ xaml::IDragUI** ppValue)
{
    *ppValue = nullptr;
    if (!m_spDragUI)
    {
        ctl::ComPtr<DragUI> spDragUI;
        IFC_RETURN(ctl::make<DragUI>(&spDragUI));
        IFC_RETURN(spDragUI.As(&m_spDragUI));
        m_spDragUI.Cast<DragUI>()->SetCoreDragOperationStarted(m_wasDeferred);
    }
    return m_spDragUI.CopyTo(ppValue);
}


_Check_return_ HRESULT
DragStartingEventArgs::GetPositionImpl(
    _In_opt_ xaml::IUIElement* pRelativeTo,
    _Out_ wf::Point* pReturnValue)
{
    *pReturnValue = m_position;
    if (pRelativeTo != m_spSource.Get())
    {
        ctl::ComPtr<IGeneralTransform> spTransform;
        IFC_RETURN(m_spSource->TransformToVisual(pRelativeTo, spTransform.GetAddressOf()));
        IFC_RETURN(spTransform->TransformPoint(m_position, pReturnValue));
    }
    return S_OK;
}

_Check_return_ HRESULT
DragStartingEventArgs::DeferralAddedImpl()
{
    ++m_deferralCount;
    return S_OK;
}

_Check_return_ HRESULT
DragStartingEventArgs::DeferralCompletedImpl()
{
    if ((--m_deferralCount == 0) && m_wasDeferred)
    {
        IFC_RETURN(ResumeRaiseEvent(true /* shouldStartOperation */));
    }
    return S_OK;
}

_Check_return_ HRESULT
DragStartingEventArgs::GetDeferralImpl(_COM_Outptr_ xaml::IDragOperationDeferral** ppReturnValue)
{
    *ppReturnValue = nullptr;
    ctl::ComPtr<DragOperationDeferral> spDeferral;
    IFC_RETURN(ctl::make<DragOperationDeferral>(this, &spDeferral));

    *ppReturnValue = spDeferral.Detach();

    return S_OK;
}

_Check_return_ HRESULT DragStartingEventArgs::SetOperationVisual()
{
    ctl::ComPtr<DragVisual> spDragVisual;
    bool hasAnchorPoint = true;
    wf::Point anchorPoint = m_position;

    if (m_spDragUI)
    {
        // Application has accessed Visual Settings, we delegate the creation of the visual to it
        // Note: if application called SetContentFromDataPackage, no visual will be created
        IFC_RETURN(m_spDragUI.Cast<DragUI>()->CreateDragVisual(spDragVisual.ReleaseAndGetAddressOf(), &hasAnchorPoint, &anchorPoint));
    }
    else
    {
        // No custom settings: Dragged UIElement is used
        IFC_RETURN(ctl::make<DragVisual>(m_spSource.Get(), &spDragVisual));
    }

    IFC_RETURN(m_spStartDragAsyncOperation.Cast<StartDragAsyncOperation>()->SetVisual(hasAnchorPoint, anchorPoint, spDragVisual.Get()));
    return S_OK;
}

_Check_return_ HRESULT
DragStartingEventArgs::StartOrUpdateOperation()
{
    BOOLEAN cancel;
    IFC_RETURN(get_Cancel(&cancel));

    if (cancel)
    {
        IFC_RETURN(m_spStartDragAsyncOperation.Cast<StartDragAsyncOperation>()->DoCancel());
    }
    else
    {
        if (m_deferralCount == 0)
        {
            IFC_RETURN(SetOperationVisual());
        }

        // m_spSource is set in our caller (StartRaiseEvent), so should never be null, but check it just in case
        ASSERT(m_spSource);

        ctl::ComPtr<mui::IPointerPointTransform> pointerPointTransform;
        IFC_RETURN(PointerPointTransform::CreatePointerPointTransform(m_spSource.Get(), &pointerPointTransform));

        ctl::ComPtr<ixp::IPointerPoint> pointerPointFromTransformedPointerPoint;
        IFC_RETURN(m_pointerPoint->GetTransformedPoint(pointerPointTransform.Get(), &pointerPointFromTransformedPointerPoint));

        // This will only start the operation if it has not yet been started
        IFC_RETURN(m_spStartDragAsyncOperation.Cast<StartDragAsyncOperation>()->StartIfNeeded(static_cast<UIElement *>(m_spSource.Get())->GetHandle(), pointerPointFromTransformedPointerPoint.Get()));

        // If we have already instanciated DragUI helper, let's notify that some properties cannot be changed anymore
        if (m_spDragUI)
        {
            m_spDragUI.Cast<DragUI>()->SetCoreDragOperationStarted(true);
        }
    }
    return S_OK;
}


_Check_return_ HRESULT
DragStartingEventArgs::StartRaiseEvent(
    _In_ UIElementGenerated::DragStartingEventSourceType* pParent,
    _In_ xaml::IUIElement *pSource,
    _In_ const TrackerPtrVector<wf::ITypedEventHandler<xaml::UIElement*, xaml::DragStartingEventArgs*>> &delegates,
    _In_ const containers::bit_vector& handledTooValues,
    bool shouldStartOperation)
{
    m_spSource = pSource;
    m_spParent = pParent;

    if (delegates.Size() == 0)
    {
        if (shouldStartOperation)
        {
            IFC_RETURN(StartOrUpdateOperation());
        }
    }
    else
    {
        size_t itrHandledValue = 0;

        for (auto itrDelegate = delegates.Begin();
            itrDelegate != delegates.End();
            itrDelegate++, itrHandledValue++)
        {
            ASSERT(itrHandledValue < handledTooValues.size());
            UIElementGenerated::DragStartingEventSourceType::HandlerInfo handlerInfo = { (*itrDelegate).Get(), handledTooValues[itrHandledValue] };
            m_handlers.push_back(handlerInfo);

        }
        m_currentHandler = m_handlers.begin();
        IFC_RETURN(ResumeRaiseEvent(shouldStartOperation));
    }
    return S_OK;
}

_Check_return_ HRESULT
DragStartingEventArgs::ResumeRaiseEvent(bool shouldStartOperation)
{
    for (/* m_currentHandler already initialized */;
            m_currentHandler != m_handlers.end();
            ++m_currentHandler)
    {
        auto handlerInfo = *m_currentHandler;
        auto *pHandlerNoRef = handlerInfo.m_spHandler.Get();
        IFCPTR_RETURN(pHandlerNoRef);

        if (!(handlerInfo.m_bHandledToo))
        {
            bool bHandled = false;
            IFC_RETURN(IsHandled(&bHandled));
            if (bHandled)
            {
                // We won't invoke this handler, but we still continue the iteration
                // because other handlers may want to be called even if handled has been set
                continue;
            }
        }

        // SYNC_CALL_TO_APP DIRECT - This next line may directly call out to app code.
        HRESULT invokeHR = pHandlerNoRef->Invoke(m_spSource.Get(), this);

        if ((invokeHR == RPC_E_DISCONNECTED) || (invokeHR == HRESULT_FROM_WIN32(RPC_S_SERVER_UNAVAILABLE)))
        {
            // If the event has been disconnected, we swallow the error and remove the handler.
            // This is consistent with the rest of the WinRT event source implementations.
            IFC_RETURN(ErrorHelper::ClearError());
            IFC_RETURN(m_spParent->RemoveHandler(pHandlerNoRef));
            invokeHR = S_OK;
        }
        IFC_RETURN(invokeHR);

        if (m_deferralCount > 0)
        {
            ++m_currentHandler;
            // Let's suspend the loop, but we'll start the operation anyway
            // i.e. any subsequent "cancel" will cancel the CoreDragOperation
            m_wasDeferred = true;
            break;
        }
    }

    // We have reached the root and have no delegate anymore
    if (shouldStartOperation)
    {
        IFC_RETURN(StartOrUpdateOperation());
    }
    
    return S_OK;
}
