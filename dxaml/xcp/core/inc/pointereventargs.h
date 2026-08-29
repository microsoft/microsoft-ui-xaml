// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "Pointer.h"
#include "PointerCollection.h"
#include "InputPointEventArgs.h"

class CPointerEventArgs : public CInputPointEventArgs
{
    // We need the InputManager to be our friend so it can selectively
    // set fullscreen permissions
    friend class CInputServices;
public:
    CPointerEventArgs(_In_ CCoreServices* pCore)
        : CInputPointEventArgs(pCore)
    { }

    // Destructor
    ~CPointerEventArgs() override
    {
        ReleaseInterface(m_pPointer);
    }

    _Check_return_ HRESULT CreateFrameworkPeer(_Outptr_ IInspectable** ppPeer) override;

    _Check_return_ HRESULT get_KeyModifiers(_Out_ DirectUI::VirtualKeyModifiers* pValue)
    {
        *pValue = m_keyModifiers;
        RRETURN(S_OK);
    }

    _Check_return_ HRESULT get_GestureFollowing(_Out_ DirectUI::GestureModes* pValue)
    {
        *pValue = m_uiGestureFollowing;
        RRETURN(S_OK);
    }

    _Check_return_ HRESULT get_Pointer(_Outptr_ CPointer** ppPointer)
    {
        SetInterface(*ppPointer, m_pPointer);
        RRETURN(S_OK);
    }

    _Check_return_ HRESULT get_IsGenerated(_Out_ BOOLEAN* pValue)
    {
        *pValue = (BOOLEAN)m_isGenerated;
        RRETURN(S_OK);
    }

    CPointer*                       m_pPointer = nullptr;
    DirectUI::VirtualKeyModifiers   m_keyModifiers = DirectUI::VirtualKeyModifiers::None;
    DirectUI::GestureModes          m_uiGestureFollowing = DirectUI::GestureModes::None;
    bool                            m_bFiredDelayedPointerUp = false;
    bool                            m_isGenerated = false;

    // Note: Most Pointer events are synchronous, which could safely use NoRef versions of
    //       these PointerPoint/PointerEventArgs variables. But the PointerExited event
    //       is async, so we need to hold strong references for that. Also, apps can hold
    //       onto this CPointerEventArgs, and we need strong refs for that as well.
    xref_ptr<ixp::IPointerPoint>     m_pPointerPoint;
    xref_ptr<ixp::IPointerEventArgs> m_pPointerEventArgs;
};
