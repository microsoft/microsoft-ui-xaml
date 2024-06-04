// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ManipulationDelta.h"
#include "ManipulationVelocities.h"


class CManipulationCompletedEventArgs final : public CManipulationEventArgs
{
public:
    CManipulationCompletedEventArgs(_In_ CCoreServices* pCore) : CManipulationEventArgs(pCore)
    {
        m_bInertial = FALSE;
        m_pManipulationTotal = NULL;
        m_pVelocitiesFinal = NULL;
    }

    // Destructor
    ~CManipulationCompletedEventArgs() override
    {
        ReleaseInterface(m_pManipulationTotal);
        ReleaseInterface(m_pVelocitiesFinal);
    }

    _Check_return_ HRESULT CreateFrameworkPeer(_Outptr_ IInspectable** ppPeer) override;

    // IsInertial property.
    _Check_return_ HRESULT get_IsInertial(_Out_ BOOLEAN* pbValue)
    {
        *pbValue = (BOOLEAN)m_bInertial;
        RRETURN(S_OK);
    }
    _Check_return_ HRESULT put_IsInertial(_In_ BOOLEAN value)
    {
        m_bInertial = !!value;
        RRETURN(S_OK);
    }

    _Check_return_ HRESULT GetCumulative(
        _Out_ CManipulationDelta** ppCumulative)
    {
        IFCPTR_RETURN(ppCumulative);
        if (m_pManipulationTotal)
        {
            *ppCumulative = m_pManipulationTotal;
            AddRefInterface(*ppCumulative);
        }
        return S_OK;
    }

    _Check_return_ HRESULT GetVelocities(
        _Out_ CManipulationVelocities** ppVelocities)
    {
        IFCPTR_RETURN(ppVelocities);
        if (m_pVelocitiesFinal)
        {
            *ppVelocities = m_pVelocitiesFinal;
            AddRefInterface(*ppVelocities);
        }

        return S_OK;
    }

public:
    bool                       m_bInertial;
    CManipulationDelta         *m_pManipulationTotal;
    CManipulationVelocities    *m_pVelocitiesFinal;

private:
};

