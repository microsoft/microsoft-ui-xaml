// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ManipulationDelta.h"
#include "ManipulationVelocities.h"


class CManipulationDeltaEventArgs final : public CManipulationEventArgs
{
public:
    CManipulationDeltaEventArgs(_In_ CCoreServices* pCore) : CManipulationEventArgs(pCore)
    {
        m_bInertial = FALSE;
        m_pDelta = NULL;
        m_pCumulative = NULL;
        m_pVelocities = NULL;
    }

    // Destructor
    ~CManipulationDeltaEventArgs() override
    {
        ReleaseInterface(m_pDelta);
        ReleaseInterface(m_pCumulative);
        ReleaseInterface(m_pVelocities);
    }

    _Check_return_ HRESULT CreateFrameworkPeer(_Outptr_ IInspectable** ppPeer) override;

    _Check_return_ HRESULT GetDelta(
        _Out_ CManipulationDelta** ppDelta)
    {
        IFCPTR_RETURN(ppDelta);
        IFCPTR_RETURN(m_pDelta);

        *ppDelta = m_pDelta;
        AddRefInterface(*ppDelta);

        return S_OK;
    }

    _Check_return_ HRESULT GetCumulative(
        _Out_ CManipulationDelta** ppCumulative)
    {
        IFCPTR_RETURN(ppCumulative);
        IFCPTR_RETURN(m_pCumulative);

        *ppCumulative = m_pCumulative;
        AddRefInterface(*ppCumulative);

        return S_OK;
    }

    _Check_return_ HRESULT GetVelocities(
        _Inout_ CManipulationVelocities** ppVelocities)
    {
        IFCPTR_RETURN(ppVelocities);

        if (m_pVelocities)
        {
            *ppVelocities = m_pVelocities;
            AddRefInterface(*ppVelocities);
        }

        return S_OK;
    }

    _Check_return_ HRESULT get_IsInertial(__deref BOOLEAN* pbValue)
    {
        *pbValue = (BOOLEAN)m_bInertial;
        RRETURN(S_OK);
    }

public:
    bool                       m_bInertial;
    CManipulationDelta         *m_pDelta;
    CManipulationDelta         *m_pCumulative;
    CManipulationVelocities    *m_pVelocities;

private:
};
