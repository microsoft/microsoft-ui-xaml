// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "ContentRootEventListener.h"
#include "ContentRoot.h"

#include "corep.h"
#include "FxCallbacks.h"

#include "TextContextMenu.h"
#include "cvalue.h"

#include "eventmgr.h"
#include "Events.h"

#include "focusmgr.h"
#include "inputservices.h"

#include "KeyboardEventArgs.h"
#include "ContextMenuEventArgs.h"
#include "RightTappedEventArgs.h"

using namespace ContentRootAdapters;

const KnownEventIndex ContentRootEventListener::textControlEventsTypes[] = {
    KnownEventIndex::TextBlock_ContextMenuOpening,
    KnownEventIndex::RichTextBlock_ContextMenuOpening,
    KnownEventIndex::RichEditBox_ContextMenuOpening,
    KnownEventIndex::TextBox_ContextMenuOpening,
    KnownEventIndex::PasswordBox_ContextMenuOpening };

ContentRootEventListener::ContentRootEventListener(_In_ CContentRoot& contentRoot)
    : m_element(contentRoot.GetVisualTreeNoRef()->GetRootElementNoRef())
{
    IFCFAILFAST(RegisterTabProcessEventHandler());
    IFCFAILFAST(RegisterContextMenuOpeningEventHandler());
    IFCFAILFAST(RegisterManipulationInertiaProcessingEventHandler());
    IFCFAILFAST(RegisterRightTappedEventHandler());
}

ContentRootEventListener::~ContentRootEventListener()
{
    IFCFAILFAST(UnregisterTabProcessEventHandler());
    IFCFAILFAST(UnregisterContextMenuOpeningEventHandler());
    IFCFAILFAST(UnregisterManipulationInertiaProcessingEventHandler());
    IFCFAILFAST(UnregisterRightTappedEventHandler());
}

_Check_return_ HRESULT ContentRootEventListener::RegisterTabProcessEventHandler()
{
    CValue handler;

    EventHandle hEventProperty(KnownEventIndex::UIElement_TabProcessing);
    handler.SetInternalHandler(&TabProcessingEventListener);
    IFC_RETURN(CEventManager::AddEventListener(m_element.get(), &m_element->m_pEventList, hEventProperty, &handler, EVENTLISTENER_INTERNAL, nullptr, false, true));

    return S_OK;
}

_Check_return_ HRESULT ContentRootEventListener::UnregisterTabProcessEventHandler()
{
    CValue handler;

    EventHandle hEventProperty(KnownEventIndex::UIElement_TabProcessing);
    handler.SetInternalHandler(&TabProcessingEventListener);
    IFC_RETURN(CEventManager::RemoveEventListener(m_element.get(), m_element->m_pEventList, hEventProperty, &handler));

    return S_OK;
}

_Check_return_ HRESULT ContentRootEventListener::TabProcessingEventListener(_In_ CDependencyObject *pSender, _In_ CEventArgs* pEventArgs)
{
    CKeyEventArgs* pKeyEventArgs = static_cast<CKeyEventArgs*>(pEventArgs);

    if (pKeyEventArgs->m_bHandled)
    {
        return S_OK;
    }

    CFocusManager* focusManager = VisualTree::GetFocusManagerForElement(pSender);

    // For TH2 we shipped without bubling up errors from TAB focus navigation
    VERIFYHR(focusManager->ProcessTabStop(pKeyEventArgs->IsShiftPressed() /*bPressedShift*/, &(pKeyEventArgs->m_bHandled)));

    return S_OK;
}

_Check_return_ HRESULT ContentRootEventListener::RegisterContextMenuOpeningEventHandler()
{
    CValue handler;

    for (int i = 0; i < ARRAY_SIZE(textControlEventsTypes); i++)
    {
        EventHandle hEventProperty(textControlEventsTypes[i]);
        handler.SetInternalHandler(&ContextMenuOpeningEventListener);
        IFC_RETURN(CEventManager::AddEventListener(m_element.get(), &m_element->m_pEventList, hEventProperty, &handler, EVENTLISTENER_INTERNAL, nullptr, false, true));
    }

    return S_OK;
}

_Check_return_ HRESULT ContentRootEventListener::UnregisterContextMenuOpeningEventHandler()
{
    CValue handler;

    for (int i = 0; i < ARRAY_SIZE(textControlEventsTypes); i++)
    {
        EventHandle hEventProperty(textControlEventsTypes[i]);
        handler.SetInternalHandler(&ContextMenuOpeningEventListener);
        IFC_RETURN(CEventManager::RemoveEventListener(m_element.get(), m_element->m_pEventList, hEventProperty, &handler));
    }

    return S_OK;
}

_Check_return_ HRESULT ContentRootEventListener::ContextMenuOpeningEventListener(_In_ CDependencyObject *pSender, _In_ CEventArgs* pEventArgs)
{
    CContextMenuEventArgs* pContextMenuEventArgs = static_cast<CContextMenuEventArgs*>(pEventArgs);
    if (pContextMenuEventArgs->m_bHandled)
    {
        return S_OK;
    }

    XPOINTF ptGlobal;

    ptGlobal.x = static_cast<XFLOAT>(pContextMenuEventArgs->m_cursorLeft);
    ptGlobal.y = static_cast<XFLOAT>(pContextMenuEventArgs->m_cursorTop);

    // NOTE: Showing context menu is not considered a crucial operation that requires us to fail
    //       and propagate the result, so ignoring HR in case of failure is the right thing to do.
    IGNOREHR(TextContextMenu::Show(
        pContextMenuEventArgs->m_pUIElement,
        ptGlobal,
        pContextMenuEventArgs->m_showCut,
        pContextMenuEventArgs->m_showCopy,
        pContextMenuEventArgs->m_showPaste,
        pContextMenuEventArgs->m_showUndo,
        pContextMenuEventArgs->m_showRedo,
        pContextMenuEventArgs->m_showSelectAll));

    pContextMenuEventArgs->m_bHandled = true;

    return S_OK;
}

_Check_return_ HRESULT ContentRootEventListener::RegisterManipulationInertiaProcessingEventHandler()
{
    CValue handler;

    EventHandle hEventProperty(KnownEventIndex::UIElement_ManipulationInertiaProcessing);
    handler.SetInternalHandler([](_In_ auto pSender, _In_ auto pEventArgs)
    {
        ManipulationInertiaProcessingEventListener(pSender, pEventArgs);
        return S_OK;
    });
    IFC_RETURN(CEventManager::AddEventListener(m_element.get(), &m_element->m_pEventList, hEventProperty, &handler, EVENTLISTENER_INTERNAL, nullptr, false, true));

    return S_OK;
}

_Check_return_ HRESULT ContentRootEventListener::UnregisterManipulationInertiaProcessingEventHandler()
{
    CValue handler;

    EventHandle hEventProperty(KnownEventIndex::UIElement_ManipulationInertiaProcessing);
    handler.SetInternalHandler([](_In_ auto pSender, _In_ auto pEventArgs)
    {
        ManipulationInertiaProcessingEventListener(pSender, pEventArgs);
        return S_OK;
    });
    IFC_RETURN(CEventManager::RemoveEventListener(m_element.get(), m_element->m_pEventList, hEventProperty, &handler));

    return S_OK;
}

void ContentRootEventListener::ManipulationInertiaProcessingEventListener(
    _In_ CDependencyObject *pSender,
    _In_ CEventArgs* pEventArgs)
{
    pSender->GetContext()->GetInputServices()->ProcessManipulationInertiaInteraction();
}

_Check_return_ HRESULT ContentRootEventListener::RegisterRightTappedEventHandler()
{
    CValue handler;

    EventHandle hEventProperty(KnownEventIndex::UIElement_RightTapped);
    handler.SetInternalHandler(&RightTappedEventListener);
    IFC_RETURN(CEventManager::AddEventListener(m_element.get(), &m_element->m_pEventList, hEventProperty, &handler, EVENTLISTENER_INTERNAL, nullptr/*pResult*/, false/*fHandledEventsToo*/, true/*fSkipIsActiveCheck*/));

    return S_OK;
}

_Check_return_ HRESULT ContentRootEventListener::UnregisterRightTappedEventHandler()
{
    CValue handler;

    EventHandle hEventProperty(KnownEventIndex::UIElement_RightTapped);
    handler.SetInternalHandler(&RightTappedEventListener);
    IFC_RETURN(CEventManager::RemoveEventListener(m_element.get(), m_element->m_pEventList, hEventProperty, &handler));

    return S_OK;
}

_Check_return_ HRESULT ContentRootEventListener::RightTappedEventListener(_In_ CDependencyObject *pSender, _In_ CEventArgs* pEventArgs)
{
    CRightTappedEventArgs* pRightTappedEventArgs = static_cast<CRightTappedEventArgs*>(pEventArgs);

    if (pRightTappedEventArgs->m_bHandled ||
        (pRightTappedEventArgs->m_pointerDeviceType != DirectUI::PointerDeviceType::Mouse &&
         pRightTappedEventArgs->m_pointerDeviceType != DirectUI::PointerDeviceType::Pen))
    {
        return S_OK;
    }

    auto contentRoot = VisualTree::GetContentRootForElement(pSender);
    auto xamlRootInspectable = contentRoot->GetOrCreateXamlRootNoRef();
    IFC_RETURN(FxCallbacks::ApplicationBarService_ProcessToggleApplicationBarsFromMouseRightTapped(xamlRootInspectable));
    pRightTappedEventArgs->m_bHandled = true;

    return S_OK;
}
