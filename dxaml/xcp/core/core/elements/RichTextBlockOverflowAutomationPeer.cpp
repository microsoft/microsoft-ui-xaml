// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "RichTextBlockOverflowAutomationPeer.h"
#include "CaretBrowsingGlobal.h"

_Check_return_ HRESULT CRichTextBlockOverflowAutomationPeer::HasKeyboardFocusHelper(_Out_ BOOLEAN* pRetVal)
{
    BOOLEAN hasKeyboardFocus = FALSE;

    IFC_RETURN(ThrowElementNotAvailableError());

    // To properly support caret-browsing on text controls, we report the control has focus
    // only if it has focus, and caret-browsing mode is turned on.
    CUIElement* uielement = static_cast<CUIElement*>(m_pDO);
    CFocusManager* focusManager = VisualTree::GetFocusManagerForElement(uielement);
    if (focusManager->IsPluginFocused() && uielement->IsFocused() && GetCaretBrowsingModeEnable())
    {
        hasKeyboardFocus = TRUE;
    }

    *pRetVal = hasKeyboardFocus;

    return S_OK;
}
