// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ContextMenuProcessor.h"

#include <Indexes.g.h>

#include "corep.h"
#include "eventmgr.h"

#include "ContextRequestedEventArgs.h"
#include "EventArgs.h"

#include "DoPointerCast.h"

#include <FxCallbacks.h>

using namespace ContentRootInput;

#define CONTEXT_REQUEST_ON_HOLD_DELAY_MSEC 500

ContextMenuProcessor::ContextMenuProcessor(_In_ CContentRoot& contentRoot)
    : m_contentRoot(contentRoot)
{
}

_Check_return_ HRESULT ContextMenuProcessor::ProcessContextRequestOnKeyboardInput(
    _In_ CDependencyObject* pSource,
    _In_ wsy::VirtualKey virtualKey,
    _In_ XUINT32 modifierKeys)
{
    if ((virtualKey == wsy::VirtualKey::VirtualKey_F10 && (modifierKeys & KEY_MODIFIER_SHIFT)) ||
        (virtualKey == wsy::VirtualKey::VirtualKey_Application) ||
        (virtualKey == wsy::VirtualKey::VirtualKey_GamepadMenu))
    {
        IFC_RETURN(RaiseContextRequestedEvent(
            pSource,
            { -1, -1 },
            false /* isTouchInput */));
    }

    return S_OK;
}

_Check_return_ HRESULT ContextMenuProcessor::RaiseContextRequestedEvent(
    _In_ CDependencyObject* pSource,
    _In_ wf::Point point,
    _In_ bool isTouchInput)
{
    xref_ptr<CContextRequestedEventArgs> contextRequestedArgs;
    IFC_RETURN(contextRequestedArgs.init(new CContextRequestedEventArgs(&m_contentRoot.GetCoreServices())));

    IFC_RETURN(contextRequestedArgs->put_Source(pSource));
    contextRequestedArgs->SetGlobalPoint(point);

    // This is a synchronous callout to application code that allows
    // the application to re-enter XAML. The application could
    // change state and release objects, so protect against
    // reentrancy by ensuring that objects are alive and state is
    // re-validated after return.
    m_contentRoot.GetCoreServices().GetEventManager()->RaiseRoutedEvent(
        EventHandle(KnownEventIndex::UIElement_ContextRequested),
        static_cast<CDependencyObject *>(pSource),
        contextRequestedArgs.get(),
        FALSE /* bIgnoreVisibility */,
        TRUE /* fRaiseSync */,
        TRUE /* fInputEvent */);

    if (contextRequestedArgs->m_bHandled && isTouchInput)
    {
        m_isContextMenuOnHolding = true;
    }

    return S_OK;
}

_Check_return_ HRESULT ContextMenuProcessor::ProcessContextRequestOnHoldingGesture(_In_ CDependencyObject *pElement)
{
    // check if element is draggable/pannable
    bool isDraggableOrPannable = false;
    FxCallbacks::UIElement_IsDraggableOrPannable(static_cast<CUIElement*>(pElement), &isDraggableOrPannable);

    if (isDraggableOrPannable)
    {
        // Create and start the contextmenu timer, and attach the timeout handler to fire ShowContextMenu
        CREATEPARAMETERS cp(&m_contentRoot.GetCoreServices());
        IFC_RETURN(CreateDO(m_contextMenuTimer.ReleaseAndGetAddressOf(), &cp));

        CValue value;
        value.SetInternalHandler(OnContextRequestOnHoldingTimeout);
        IFC_RETURN(m_contextMenuTimer->AddEventListener(
            EventHandle(KnownEventIndex::DispatcherTimer_Tick),
            &value,
            EVENTLISTENER_INTERNAL, nullptr, FALSE));

        xref_ptr<CTimeSpan> timeSpan;
        IFC_RETURN(CreateDO(timeSpan.ReleaseAndGetAddressOf(), &cp));
        timeSpan->m_rTimeSpan = static_cast<float>(CONTEXT_REQUEST_ON_HOLD_DELAY_MSEC) / 1000.0f;
        IFC_RETURN(m_contextMenuTimer->SetValueByKnownIndex(KnownPropertyIndex::DispatcherTimer_Interval, timeSpan.get()));
        IFC_RETURN(m_contextMenuTimer->SetTargetObject(pElement)); // Make sure we can get "pElement" from the event handler
        IFC_RETURN(m_contextMenuTimer->Start());
    }
    else
    {
        IFC_RETURN(RaiseContextRequestedEvent(
            pElement,
            m_contextMenuOnHoldingTouchPoint,
            true /* isTouchInput */));
    }

    return S_OK;
}

_Check_return_ HRESULT ContextMenuProcessor::OnContextRequestOnHoldingTimeout(
    _In_ CDependencyObject *pSender,
    _In_ CEventArgs* pEventArgs)
{
    xref_ptr<CDispatcherTimer> timer;
    IFC_RETURN(DoPointerCast(timer, pSender));
    IFC_RETURN(timer->WorkComplete());
    IFC_RETURN(timer->Stop());

    // The target of the timer stores the UIElement on which touch timer was initiated
    xref_ptr<CDependencyObject> source = timer->GetTargetObject();

    ContextMenuProcessor& contextMenuProcessor = VisualTree::GetContentRootForElement(source.get())->GetInputManager().GetContextMenuProcessor();

    IFC_RETURN(contextMenuProcessor.RaiseContextRequestedEvent(
        source,
        contextMenuProcessor.m_contextMenuOnHoldingTouchPoint,
        true /* isTouchInput */));

    return S_OK;
}