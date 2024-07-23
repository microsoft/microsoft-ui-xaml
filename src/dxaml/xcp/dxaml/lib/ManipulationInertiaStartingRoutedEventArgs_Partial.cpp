// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ManipulationInertiaStartingRoutedEventArgs.g.h"
#include "ManipulationDelta.h"
#include "ManipulationVelocities.h"
#include "InertiaExpansionBehavior.g.h"
#include "InertiaRotationBehavior.g.h"
#include "InertiaTranslationBehavior.g.h"
#include "CoreEventArgsGroup.h"

_Check_return_ HRESULT DirectUI::ManipulationInertiaStartingRoutedEventArgs::get_DeltaImpl(
    _Out_ ixp::ManipulationDelta* pValue)
{
    HRESULT hr = S_OK;
    xref_ptr<CEventArgs> spCoreArgs;
    xref_ptr<CManipulationDelta> spDelta;

    spCoreArgs.attach(GetCorePeer());
    IFC(static_cast<CManipulationInertiaStartingEventArgs*>(static_cast<CEventArgs*>(spCoreArgs))->GetDelta(spDelta.ReleaseAndGetAddressOf()));
    if (spDelta)
    {
        pValue->Translation.X = spDelta->m_ptTranslation.x;
        pValue->Translation.Y = spDelta->m_ptTranslation.y;
        pValue->Scale = spDelta->m_fScale;
        pValue->Rotation = spDelta->m_fRotation;
        pValue->Expansion = spDelta->m_fExpansion;
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
    RRETURN(hr);
}

_Check_return_ HRESULT DirectUI::ManipulationInertiaStartingRoutedEventArgs::get_CumulativeImpl(
    _Out_ ixp::ManipulationDelta* pValue)
{
    HRESULT hr = S_OK;
    xref_ptr<CEventArgs> spCoreArgs;
    xref_ptr<CManipulationDelta> spCumulative;

    spCoreArgs.attach(GetCorePeer());
    IFC(static_cast<CManipulationInertiaStartingEventArgs*>(static_cast<CEventArgs*>(spCoreArgs))->GetCumulative(spCumulative.ReleaseAndGetAddressOf()));
    if (spCumulative)
    {
        pValue->Translation.X = spCumulative->m_ptTranslation.x;
        pValue->Translation.Y = spCumulative->m_ptTranslation.y;
        pValue->Scale = spCumulative->m_fScale;
        pValue->Rotation = spCumulative->m_fRotation;
        pValue->Expansion = spCumulative->m_fExpansion;
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
    RRETURN(hr);
}

_Check_return_ HRESULT DirectUI::ManipulationInertiaStartingRoutedEventArgs::get_VelocitiesImpl(
    _Out_ ixp::ManipulationVelocities* pValue)
{
    HRESULT hr = S_OK;
    xref_ptr<CEventArgs> spCoreArgs;
    xref_ptr<CManipulationVelocities> spVelocities;

    IFCPTR(pValue);

    spCoreArgs.attach(GetCorePeer());
    IFC(static_cast<CManipulationInertiaStartingEventArgs*>(static_cast<CEventArgs*>(spCoreArgs))->GetVelocities(spVelocities.ReleaseAndGetAddressOf()));
    if (spVelocities)
    {
        pValue->Linear.X = spVelocities->m_ptLinear.x;
        pValue->Linear.Y = spVelocities->m_ptLinear.y;
        pValue->Angular = spVelocities->m_fAngular;
        pValue->Expansion = spVelocities->m_fExpansion;
    }
    else
    {
        pValue->Linear.X = 0.0f;
        pValue->Linear.Y = 0.0f;
        pValue->Angular = 0.0f;
        pValue->Expansion = 0.0f;
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT DirectUI::ManipulationInertiaStartingRoutedEventArgs::get_ExpansionBehaviorImpl(
    _Outptr_result_maybenull_ xaml_input::IInertiaExpansionBehavior** ppValue)
{
    HRESULT hr = S_OK;
    xref_ptr<CEventArgs> spCoreArgs;
    CInertiaExpansionBehavior* pExpansion = nullptr;

    spCoreArgs.attach(GetCorePeer());
    IFC(static_cast<CManipulationInertiaStartingEventArgs*>(static_cast<CEventArgs*>(spCoreArgs))->get_ExpansionBehavior(&pExpansion));
    IFC(CValueBoxer::ConvertToFramework(pExpansion, ppValue, /* fReleaseCoreValue */ TRUE));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT DirectUI::ManipulationInertiaStartingRoutedEventArgs::put_ExpansionBehaviorImpl(
    _In_opt_ xaml_input::IInertiaExpansionBehavior* pValue)
{
    xref_ptr<CEventArgs> spCoreArgs;

    if (pValue == nullptr)
    {
        IFC_RETURN(E_INVALIDARG);
    }

    CInertiaExpansionBehavior* pValueCore = static_cast<CInertiaExpansionBehavior*>(pValue ? static_cast<DirectUI::InertiaExpansionBehavior*>(pValue)->GetHandle() : nullptr);

    spCoreArgs.attach(GetCorePeer());
    IFC_RETURN(static_cast<CManipulationInertiaStartingEventArgs*>(static_cast<CEventArgs*>(spCoreArgs))->put_ExpansionBehavior(pValueCore));

    return S_OK;
}

_Check_return_ HRESULT DirectUI::ManipulationInertiaStartingRoutedEventArgs::get_RotationBehaviorImpl(
    _Outptr_result_maybenull_ xaml_input::IInertiaRotationBehavior** ppValue)
{
    HRESULT hr = S_OK;
    xref_ptr<CEventArgs> spCoreArgs;
    CInertiaRotationBehavior* pRotation = nullptr;

    spCoreArgs.attach(GetCorePeer());
    IFC(static_cast<CManipulationInertiaStartingEventArgs*>(static_cast<CEventArgs*>(spCoreArgs))->get_RotationBehavior(&pRotation));
    IFC(CValueBoxer::ConvertToFramework(pRotation, ppValue, /* fReleaseCoreValue */ TRUE));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT DirectUI::ManipulationInertiaStartingRoutedEventArgs::put_RotationBehaviorImpl(
    _In_opt_ xaml_input::IInertiaRotationBehavior* pValue)
{
    xref_ptr<CEventArgs> spCoreArgs;

    if (pValue == nullptr)
    {
        IFC_RETURN(E_INVALIDARG);
    }

    CInertiaRotationBehavior* pValueCore = static_cast<CInertiaRotationBehavior*>(pValue ? static_cast<DirectUI::InertiaRotationBehavior*>(pValue)->GetHandle() : nullptr);

    spCoreArgs.attach(GetCorePeer());
    IFC_RETURN(static_cast<CManipulationInertiaStartingEventArgs*>(static_cast<CEventArgs*>(spCoreArgs))->put_RotationBehavior(pValueCore));

    return S_OK;
}

_Check_return_ HRESULT DirectUI::ManipulationInertiaStartingRoutedEventArgs::get_TranslationBehaviorImpl(
    _Outptr_result_maybenull_ xaml_input::IInertiaTranslationBehavior** ppValue)
{
    HRESULT hr = S_OK;
    xref_ptr<CEventArgs> spCoreArgs;
    CInertiaTranslationBehavior* pTranslation = nullptr;

    spCoreArgs.attach(GetCorePeer());
    IFC(static_cast<CManipulationInertiaStartingEventArgs*>(static_cast<CEventArgs*>(spCoreArgs))->get_TranslationBehavior(&pTranslation));
    IFC(CValueBoxer::ConvertToFramework(pTranslation, ppValue, /* fReleaseCoreValue */ TRUE));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT DirectUI::ManipulationInertiaStartingRoutedEventArgs::put_TranslationBehaviorImpl(
    _In_opt_ xaml_input::IInertiaTranslationBehavior* pValue)
{
    xref_ptr<CEventArgs> spCoreArgs;

    if (pValue == nullptr)
    {
        IFC_RETURN(E_INVALIDARG);
    }

    CInertiaTranslationBehavior* pValueCore = static_cast<CInertiaTranslationBehavior*>(pValue ? static_cast<DirectUI::InertiaTranslationBehavior*>(pValue)->GetHandle() : nullptr);

    spCoreArgs.attach(GetCorePeer());
    IFC_RETURN(static_cast<CManipulationInertiaStartingEventArgs*>(static_cast<CEventArgs*>(spCoreArgs))->put_TranslationBehavior(pValueCore));

    return S_OK;
}
