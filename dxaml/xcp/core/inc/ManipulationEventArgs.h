// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CManipulationEventArgs : public CInputPointEventArgs
{
public:
    CManipulationEventArgs(_In_ CCoreServices* pCore) : CInputPointEventArgs(pCore)
    {
        m_ptManipulation.x = 0;
        m_ptManipulation.y = 0;

        m_bRequestedComplete = FALSE;

        m_pManipulationContainer = NULL;
    }

    // Destructor - releases the potential references to the manipulation container
    ~CManipulationEventArgs() override;

    // Set manipulation origin position
    _Check_return_ HRESULT SetManipulationOriginPosition(
        _In_ XFLOAT xOrigin,
        _In_ XFLOAT yOrigin);

    _Check_return_ HRESULT get_Position(
        __deref wf::Point* pPoint)
    {
        pPoint->X = m_ptManipulation.x;
        pPoint->Y = m_ptManipulation.y;
        RRETURN(S_OK);
    }

    _Check_return_ HRESULT get_Container(_In_ CUIElement** ppManipulationContainer);
    _Check_return_ HRESULT put_Container(_In_ CUIElement* pManipulationContainer);

public:
    XPOINTF             m_ptManipulation;
    bool               m_bRequestedComplete;
    CUIElement*         m_pManipulationContainer;

    _Check_return_ HRESULT Complete();
};

