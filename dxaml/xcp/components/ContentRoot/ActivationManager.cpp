// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ActivationManager.h"
#include "ContentRoot.h"
#include "focusmgr.h"

using namespace ContentRootInput;

ActivationManager::ActivationManager(_In_ CInputManager& inputManager)
    : m_inputManager(inputManager)
{
}

bool ActivationManager::IsActive() const
{
    return m_fWindowActive;
}

_Check_return_ HRESULT ActivationManager::ProcessWindowActivation(_In_ bool shiftPressed, _In_ bool fWndActive)
{
    // If we're already in the activation state we're told we're in,
    // then we have nothing to do.
    if (m_fWindowActive == fWndActive && m_fWasWindowActive)
    {
        return S_OK;
    }

    m_fWindowActive = fWndActive;
    m_fWasWindowActive = m_fWasWindowActive || m_fWindowActive;
    const auto contentRoot = m_inputManager.GetContentRoot();

    // Only set window focus if we're about to set focus state
    // to something other than the current state.
    CFocusManager* focusManager = contentRoot->GetFocusManagerNoRef();
    if (focusManager->IsPluginFocused() != m_fWindowActive)
    {
        IFC_RETURN(focusManager->SetWindowFocus(m_fWindowActive, shiftPressed));
    }

    return S_OK;
}

_Check_return_ HRESULT ActivationManager::ProcessFocusInput(_In_ InputMessage *pMsg, _Out_ XINT32 *handled)
{
    ASSERT(pMsg->m_msgID == XCP_GOTFOCUS || pMsg->m_msgID == XCP_LOSTFOCUS);

    // If we're already in the focus state we're told we're in,
    // then we have nothing to do.
    if (m_fWindowFocused == (pMsg->m_msgID == XCP_GOTFOCUS) && m_fWasWindowFocused)
    {
        return S_OK;
    }

    m_fWindowFocused = (pMsg->m_msgID == XCP_GOTFOCUS);
    m_fWasWindowFocused = m_fWasWindowFocused || m_fWindowFocused;

    const auto contentRoot = m_inputManager.GetContentRoot();
    CFocusManager* focusManager = contentRoot->GetFocusManagerNoRef();

    // Only set window focus if we're about to set focus state
    // to something other than the current state.
    if (focusManager->IsPluginFocused() != m_fWindowFocused)
    {
        // At this point, we have a visual tree, we know that the focus value is changing,
        // and the window is currently active, meaning that we're about to set or lose focus
        // from the window.  As such, we should mark this event as handled, since we're going
        // to be doing something with it.
        *handled = TRUE;

        const bool shiftKeyPressed = (pMsg->m_modifierKeys & KEY_MODIFIER_SHIFT) != 0;
        IFC_RETURN(focusManager->SetWindowFocus(m_fWindowFocused, shiftKeyPressed));
    }

    return S_OK;
}