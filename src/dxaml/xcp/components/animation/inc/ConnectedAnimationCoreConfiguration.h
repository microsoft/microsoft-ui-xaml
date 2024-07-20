// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "windows.ui.composition.h"

interface __declspec(uuid("b47d6d7a-e739-44cf-90cf-b930a7b84d88")) IConnectedAnimationCoreConfiguration : public IUnknown
{
    virtual _Check_return_ HRESULT GetEffectiveEasingFunction(_Outptr_result_maybenull_ WUComp::ICompositionEasingFunction** value) = 0;
    virtual _Check_return_ HRESULT GetEffectiveDuration(_Out_ wf::TimeSpan* value) = 0;
    virtual _Check_return_ HRESULT GetEffectPropertySet(_In_ wfn::Vector3 scaleFactors, _Out_ WUComp::ICompositionPropertySet** effectPropertySet) = 0;
    virtual bool ShouldShadowBeCast() = 0;
};

// Static function to retrieve the default configuration from DXaml.
_Check_return_ HRESULT ConnectedAnimationConfiguration_GetDefault(_Out_ IConnectedAnimationCoreConfiguration** configuration);
