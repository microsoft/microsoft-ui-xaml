// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ManipulationDelta.h"

class CManipulationStartedEventArgs final : public CManipulationEventArgs
{
public:
    CManipulationStartedEventArgs(_In_ CCoreServices* pCore) : CManipulationEventArgs(pCore)
    {
        m_pCumulative = NULL;
    }

    // Destructor
    ~CManipulationStartedEventArgs() override
    {
        ReleaseInterface(m_pCumulative);
    }

    _Check_return_ HRESULT CreateFrameworkPeer(_Outptr_ IInspectable** ppPeer) override;

    _Check_return_ HRESULT GetCumulative(
        _Inout_ CManipulationDelta** ppCumulative)
    {
        IFCPTR_RETURN(ppCumulative);
        if (m_pCumulative)
        {
            *ppCumulative = m_pCumulative;
            AddRefInterface(*ppCumulative);
        }

        return S_OK;
    }


public:
    CManipulationDelta* m_pCumulative;
};


