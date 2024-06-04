// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "GravityConnectedAnimationConfiguration.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(GravityConnectedAnimationConfiguration)
    {
    public:
        GravityConnectedAnimationConfiguration() {};

        _Check_return_ HRESULT GetEffectPropertySet(_In_ wfn::Vector3 scaleFactors, _Out_ WUComp::ICompositionPropertySet** effectPropertySet) override;
        bool ShouldShadowBeCast() override { return m_isShadowEnabled; }

        _Check_return_ HRESULT get_IsShadowEnabledImpl(_Out_ BOOLEAN* value);
        _Check_return_ HRESULT put_IsShadowEnabledImpl(_In_ BOOLEAN value);

    private:
        BOOLEAN m_isShadowEnabled = true;
    };
}
