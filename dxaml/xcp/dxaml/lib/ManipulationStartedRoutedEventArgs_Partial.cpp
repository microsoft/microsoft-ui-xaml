// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ManipulationStartedRoutedEventArgs.g.h"
#include "ManipulationDelta.h"
#include "CoreEventArgsGroup.h"

_Check_return_ HRESULT DirectUI::ManipulationStartedRoutedEventArgs::get_CumulativeImpl(
    _Out_ ixp::ManipulationDelta* pValue)
{
    HRESULT hr = S_OK;
    CManipulationDelta* pCumulative = nullptr;

    CEventArgs* const pCoreArgs = GetCorePeer();
    IFC(static_cast<CManipulationStartedEventArgs*>(pCoreArgs)->GetCumulative(&pCumulative));
    if (pCumulative)
    {
        pValue->Translation.X = pCumulative->m_ptTranslation.x;
        pValue->Translation.Y = pCumulative->m_ptTranslation.y;
        pValue->Scale = pCumulative->m_fScale;
        pValue->Rotation = pCumulative->m_fRotation;
        pValue->Expansion = pCumulative->m_fExpansion;
    }
    else
    {
        pValue->Translation.X = 0.0f;
        pValue->Translation.Y = 0.0f;
        pValue->Scale = 1.0f;
        pValue->Rotation = 0.0f;
        pValue->Expansion = 0.0f;
    }

Cleanup:
    ReleaseInterfaceNoNULL(pCoreArgs);
    ReleaseInterface(pCumulative);
    RRETURN(hr);
}

