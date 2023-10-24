// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ManipulationCompletedRoutedEventArgs.g.h"
#include "ManipulationDelta.h"
#include "ManipulationVelocities.h"
#include "CoreEventArgsGroup.h"

_Check_return_ HRESULT DirectUI::ManipulationCompletedRoutedEventArgs::get_CumulativeImpl(
    _Out_ ixp::ManipulationDelta* pValue)
{
    HRESULT hr = S_OK;
    CManipulationDelta* pCumulative = nullptr;

    CEventArgs* const pCoreArgs = GetCorePeer();
    IFC(static_cast<CManipulationCompletedEventArgs*>(pCoreArgs)->GetCumulative(&pCumulative));
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

_Check_return_ HRESULT DirectUI::ManipulationCompletedRoutedEventArgs::get_VelocitiesImpl(
    _Out_ ixp::ManipulationVelocities* pValue)
{
    IFCPTR_RETURN(pValue);

    xref_ptr<CEventArgs> coreArgs;
    coreArgs.attach(GetCorePeer());

    xref_ptr<CManipulationVelocities> velocities;
    IFC_RETURN(static_cast<CManipulationCompletedEventArgs*>(coreArgs.get())->GetVelocities(velocities.ReleaseAndGetAddressOf()));

    if (velocities)
    {
        pValue->Linear.X = velocities->m_ptLinear.x;
        pValue->Linear.Y = velocities->m_ptLinear.y;
        pValue->Angular = velocities->m_fAngular;
        pValue->Expansion = velocities->m_fExpansion;
    }
    else
    {
        pValue->Linear.X = 0.0f;
        pValue->Linear.Y = 0.0f;
        pValue->Angular = 0.0f;
        pValue->Expansion = 0.0f;
    }

    return S_OK;
}
