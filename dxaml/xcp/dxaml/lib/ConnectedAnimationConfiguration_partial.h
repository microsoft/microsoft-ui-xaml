// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "ConnectedAnimationConfiguration.g.h"
#include "ConnectedAnimationCoreConfiguration.h"


namespace DirectUI
{
    PARTIAL_CLASS(ConnectedAnimationConfiguration), public IConnectedAnimationCoreConfiguration
    {
    public:
        ConnectedAnimationConfiguration()
        {
            // We use a duration of zero to indicate that no duration is set and that we
            // should pick up the default from the Connected Animation Service.  This is to
            // maintain compatability as to when the default duration can be set on the
            // Service and what animations pick it up.
            m_duration.Duration = 0;
        }

        // IConnectedAnimationCoreConfiguration;
        _Check_return_ HRESULT GetEffectiveDuration(_Out_ wf::TimeSpan* value) override;
        _Check_return_ HRESULT GetEffectiveEasingFunction(_Outptr_ WUComp::ICompositionEasingFunction** value) override;
        _Check_return_ HRESULT GetEffectPropertySet(_In_ wfn::Vector3 scaleFactors, _Out_ WUComp::ICompositionPropertySet** effectPropertySet) override { return S_OK; };
        bool ShouldShadowBeCast() override { return false; }

        // Supports the IConnectedAnimationCoreConfiguration interface.
        _Check_return_ HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

        virtual _Check_return_ HRESULT GetDefaultEasingFunction(_Outptr_ WUComp::ICompositionEasingFunction** value);
        virtual const wf::TimeSpan GetDefaultDuration();

    protected:
        wf::TimeSpan m_duration;
        ctl::ComPtr<WUComp::ICompositionEasingFunction> m_easingFunction;
    };
}
