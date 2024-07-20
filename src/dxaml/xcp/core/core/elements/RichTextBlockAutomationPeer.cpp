// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "RichTextBlockAutomationPeer.h"
#include "CaretBrowsingGlobal.h"

_Check_return_ HRESULT CRichTextBlockAutomationPeer::HasKeyboardFocusHelper(_Out_ BOOLEAN* pRetVal)
{
    BOOLEAN hasKeyboardFocus = FALSE;

    IFC_RETURN(ThrowElementNotAvailableError());

    CUIElement* uielement = static_cast<CUIElement*>(m_pDO);
    CFocusManager* focusManager = VisualTree::GetFocusManagerForElement(uielement);
    if (focusManager->IsPluginFocused() && uielement->IsFocused() && (GetCaretBrowsingModeEnable() || uielement->IsTabStop()))
    {
        hasKeyboardFocus = TRUE;
    }

    *pRetVal = hasKeyboardFocus;

    return S_OK;
}

_Check_return_ HRESULT CRichTextBlockAutomationPeer::IsKeyboardFocusableHelper(_Out_ BOOLEAN* pRetVal)
{
    BOOLEAN isKeyboardFocusable = FALSE;

    IFC_RETURN(ThrowElementNotAvailableError());

    CRichTextBlock* richTextBlock = static_cast<CRichTextBlock*>(m_pDO);
    if (richTextBlock->IsFocusable() && (GetCaretBrowsingModeEnable() || richTextBlock->IsTabStop()))
    {
        isKeyboardFocusable = TRUE;
    }

    *pRetVal = isKeyboardFocusable;

    return S_OK;
}
