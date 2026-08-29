// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "RoutedEventArgs.h"

class CContextMenuEventArgs final : public CRoutedEventArgs
{
public:
    CContextMenuEventArgs()
    {
        m_pUIElement = NULL;
        m_cursorLeft = 0;
        m_cursorTop = 0;
        m_showCut = false;
        m_showCopy = false;
        m_showPaste = false;
        m_showUndo = false;
        m_showRedo = false;
        m_showSelectAll = false;
    }

    _Check_return_ HRESULT CreateFrameworkPeer(_Outptr_ IInspectable** ppPeer) override;

    _Check_return_ HRESULT get_CursorLeft(_Out_ DOUBLE* pnValue)
    {
        *pnValue = m_cursorLeft;
        RRETURN(S_OK);
    }

    _Check_return_ HRESULT put_CursorLeft(_In_ DOUBLE nValue)
    {
        m_cursorLeft = nValue;
        RRETURN(S_OK);
    }

    _Check_return_ HRESULT get_CursorTop(_Out_ DOUBLE* pnValue)
    {
        *pnValue = m_cursorTop;
        RRETURN(S_OK);
    }

    _Check_return_ HRESULT put_CursorTop(_In_ DOUBLE nValue)
    {
        m_cursorTop = nValue;
        RRETURN(S_OK);
    }

    CUIElement *m_pUIElement;
    DOUBLE      m_cursorLeft;
    DOUBLE      m_cursorTop;
    bool        m_showCut       : 1;
    bool        m_showCopy      : 1;
    bool        m_showPaste     : 1;
    bool        m_showUndo      : 1;
    bool        m_showRedo      : 1;
    bool        m_showSelectAll : 1;
};

