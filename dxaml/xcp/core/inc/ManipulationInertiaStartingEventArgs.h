// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "InertiaExpansionBehavior.h"
#include "InertiaRotationBehavior.h"
#include "InertiaTranslationBehavior.h"

class CManipulationInertiaStartingEventArgs : public CManipulationEventArgs
{
public:
    CManipulationInertiaStartingEventArgs(_In_ CCoreServices* pCore) : CManipulationEventArgs(pCore)
    {
        m_pExpansion = NULL;
        m_pRotation = NULL;
        m_pTranslation = NULL;
        m_pDelta = NULL;
        m_pCumulative = NULL;
        m_pVelocities = NULL;
    }

    ~CManipulationInertiaStartingEventArgs() override
    {
        ReleaseInterface(m_pExpansion);
        ReleaseInterface(m_pRotation);
        ReleaseInterface(m_pTranslation);
        ReleaseInterface(m_pDelta);
        ReleaseInterface(m_pCumulative);
        ReleaseInterface(m_pVelocities);
    }

    _Check_return_ HRESULT CreateFrameworkPeer(_Outptr_ IInspectable** ppPeer) override;

    _Check_return_ HRESULT GetDelta(
        _Inout_ CManipulationDelta** ppDelta)
    {
        IFCPTR_RETURN(ppDelta);
        if (m_pDelta)
        {
            *ppDelta = m_pDelta;
            AddRefInterface(*ppDelta);
        }

        return S_OK;
    }
    
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

    _Check_return_ HRESULT get_ExpansionBehavior(_Outptr_ CInertiaExpansionBehavior** ppExpansion)
    {
        SetInterface(*ppExpansion, m_pExpansion);
        RRETURN(S_OK);
    }

    _Check_return_ HRESULT put_ExpansionBehavior(_In_ CInertiaExpansionBehavior* pExpansion)
    {
        ReplaceInterface(m_pExpansion, pExpansion);
        RRETURN(S_OK);
    }

    _Check_return_ HRESULT get_RotationBehavior(_Outptr_ CInertiaRotationBehavior** ppRotation)
    {
        SetInterface(*ppRotation, m_pRotation);
        RRETURN(S_OK);
    }

    _Check_return_ HRESULT put_RotationBehavior(_In_ CInertiaRotationBehavior* pRotation)
    {
        ReplaceInterface(m_pRotation, pRotation);
        RRETURN(S_OK);
    }

    _Check_return_ HRESULT get_TranslationBehavior(_Outptr_ CInertiaTranslationBehavior** ppTranslation)
    {
        SetInterface(*ppTranslation, m_pTranslation);
        RRETURN(S_OK);
    }

    _Check_return_ HRESULT put_TranslationBehavior(_In_ CInertiaTranslationBehavior* pTranslation)
    {
        ReplaceInterface(m_pTranslation, pTranslation);
        RRETURN(S_OK);
    }

public:
    CInertiaExpansionBehavior* m_pExpansion;
    CInertiaRotationBehavior* m_pRotation;
    CInertiaTranslationBehavior* m_pTranslation;
    CManipulationDelta         *m_pDelta;
    CManipulationDelta         *m_pCumulative;
    CManipulationVelocities    *m_pVelocities;
};
