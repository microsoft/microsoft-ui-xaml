// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ConnectedAnimationConfiguration_partial.h"
#include "GravityConnectedAnimationConfiguration_partial.h"
#include "Corep.h"
#include "EasingFunctionBase.g.h"
#include <RuntimeEnabledFeatures.h>
#include <FeatureFlags.h>

using namespace DirectUI;
using namespace RuntimeFeatureBehavior;

_Check_return_ HRESULT DirectUI::ConnectedAnimationConfiguration::GetEffectiveEasingFunction(_Outptr_result_maybenull_ WUComp::ICompositionEasingFunction** value)
{
    ctl::ComPtr<WUComp::ICompositionEasingFunction> easingFunction = m_easingFunction;
    if (easingFunction == nullptr)
    {
        IFC_RETURN(GetDefaultEasingFunction(&easingFunction));
    }
    *value = easingFunction.Detach();
    return S_OK;
}

_Check_return_ HRESULT DirectUI::ConnectedAnimationConfiguration::GetEffectiveDuration(_Out_ wf::TimeSpan* value)
{
    bool isAnimationEnabled = true;
    IFC_RETURN(FxCallbacks::FrameworkCallbacks_IsAnimationEnabled(&isAnimationEnabled));
    if (!isAnimationEnabled)
    {
        value->Duration = 10000;  // If animations are disabled in Settings, we still need to fire Completed event with no delay, so use a 1ms animation.
    }
    else if (m_duration.Duration != 0)
    {
        *value = m_duration;
    }
    else
    {
        *value = GetDefaultDuration();
    }

    return S_OK;
}

// Supports the IConnectedAnimationCoreConfiguration interface.
_Check_return_ HRESULT DirectUI::ConnectedAnimationConfiguration::QueryInterfaceImpl(
    _In_ REFIID iid,
    _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(IConnectedAnimationCoreConfiguration)))
    {
        *ppObject = static_cast<IConnectedAnimationCoreConfiguration*>(this);
    }
    else
    {
        RRETURN(ConnectedAnimationConfigurationGenerated::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}

_Check_return_ HRESULT DirectUI::ConnectedAnimationConfiguration::GetDefaultEasingFunction(_Outptr_result_maybenull_ WUComp::ICompositionEasingFunction** value)
{
    ctl::ComPtr<WUComp::ICompositionEasingFunction> easingFunction;
    IFC_RETURN(DXamlCore::GetCurrent()->GetHandle()->GetConnectedAnimationServiceNoRef()->GetDefaultEasingFunction(&easingFunction));
    *value = easingFunction.Detach();
    return S_OK;
}

const wf::TimeSpan DirectUI::ConnectedAnimationConfiguration::GetDefaultDuration()
{
    return DXamlCore::GetCurrent()->GetHandle()->GetConnectedAnimationServiceNoRef()->GetDefaultDuration();
}

_Check_return_ HRESULT ConnectedAnimationConfiguration_GetDefault(_Out_ IConnectedAnimationCoreConfiguration** configuration)
{
   *configuration = nullptr;
    if (!GetRuntimeEnabledFeatureDetector()->IsFeatureEnabled(RuntimeEnabledFeature::DisableDefaultConnectedAnimationConfiguration))
    {
        IFC_RETURN(ctl::ComObject<GravityConnectedAnimationConfiguration>::CreateInstance(configuration));
    }

    return S_OK;
}