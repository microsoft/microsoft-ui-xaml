// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <xref_ptr.h>
#include "EventArgs.h"

class CVisualStateChangedEventArgs final : public CEventArgs
{
public:
    CVisualStateChangedEventArgs() = default;
    CVisualStateChangedEventArgs(_In_opt_ CVisualState* pOldState, _In_opt_ CVisualState* pNewState, _In_opt_ CControl* pControl)
        : m_spOldState(pOldState)
        , m_spNewState(pNewState)
        , m_spControl(pControl)
    {

    }

    _Check_return_ HRESULT CreateFrameworkPeer(_Outptr_ IInspectable** ppPeer) override;

    // OldState property.
    _Check_return_ HRESULT get_OldState(_Outptr_result_maybenull_ CVisualState** ppValue)
    {
        m_spOldState.CopyTo(ppValue);
        return S_OK;
    }
    _Check_return_ HRESULT put_OldState(_In_opt_ CVisualState* pValue)
    {
        m_spOldState = pValue;
        return S_OK;
    }

    // New property.
    _Check_return_ HRESULT get_NewState(_Outptr_result_maybenull_ CVisualState** ppValue)
    {
        m_spNewState.CopyTo(ppValue);
        return S_OK;
    }
    _Check_return_ HRESULT put_NewState(_In_opt_ CVisualState* pValue)
    {
        m_spNewState = pValue;
        return S_OK;
    }

    // Control property.
    _Check_return_ HRESULT get_Control(_Outptr_result_maybenull_ CControl** ppValue)
    {
        m_spControl.CopyTo(ppValue);
        return S_OK;
    }
    _Check_return_ HRESULT put_Control(_In_opt_ CControl* pValue)
    {
        m_spControl = pValue;
        return S_OK;
    }

private:
    xref_ptr<CVisualState> m_spOldState;
    xref_ptr<CVisualState> m_spNewState;
    xref_ptr<CControl> m_spControl;

};

