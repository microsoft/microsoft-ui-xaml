// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ManipulationPivot.h"

class CManipulationStartingEventArgs final : public CManipulationEventArgs
{
public:
    CManipulationStartingEventArgs(_In_ CCoreServices* pCore) : CManipulationEventArgs(pCore)
    {
        m_uiManipulationMode = DirectUI::ManipulationModes::System;
        m_pPivot = NULL;
    }

    // Destructor
    ~CManipulationStartingEventArgs() override
    {
        ReleaseInterface(m_pPivot);
    }

    _Check_return_ HRESULT CreateFrameworkPeer(_Outptr_ IInspectable** ppPeer) override;

    _Check_return_ HRESULT get_Mode(_Out_ DirectUI::ManipulationModes* pValue)
    {
        *pValue = m_uiManipulationMode;
        RRETURN(S_OK);
    }

    _Check_return_ HRESULT put_Mode(_In_ DirectUI::ManipulationModes value)
    {
        m_uiManipulationMode = value;
        RRETURN(S_OK);
    }

    _Check_return_ HRESULT get_Pivot(_Outptr_ CManipulationPivot** ppManipulationPivot)
    {
        SetInterface(*ppManipulationPivot, m_pPivot);
        RRETURN(S_OK);
    }

    _Check_return_ HRESULT put_Pivot(_In_ CManipulationPivot* pManipulationPivot)
    {
        ReplaceInterface(m_pPivot, pManipulationPivot);
        RRETURN(S_OK);
    }

public:
    DirectUI::ManipulationModes m_uiManipulationMode;
    CManipulationPivot* m_pPivot;
};
