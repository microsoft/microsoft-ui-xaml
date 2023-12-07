// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "NamespaceAliases.h"
#include "palnetwork.h"
#include <windows.foundation.h>
#include <microsoft.ui.composition.interop.h>

class CCoreServices;

interface IEffectPolicyHelperCallback
{
    virtual void OnPolicyChanged() = 0;
};

// Wraps windows components and decides policy on whether shadows should be enabled or not.
// This object's lifetime is meant to be controlled by the ProjectedShadowManager, and it cannot be transferred between
// different ProjectedShadowManagers.
class EffectPolicyHelper
    : public CXcpObjectBase<IPALExecuteOnUIThread>
{
public:
    EffectPolicyHelper(_In_ CCoreServices* coreServices, _In_ IEffectPolicyHelperCallback* callback);
    ~EffectPolicyHelper() override;

    // Called when the ProjectedShadowManager is deleted. This object might be kept alive temporarily by a ExecuteOnUIThread
    // call. Disconnect the callback so this object doesn't call into a deleted ProjectedShadowManager.
    void ClearCallback(_In_ IEffectPolicyHelperCallback* callback);

    const bool AreShadowsEnabled() { return m_areShadowsEnabled; }

    HRESULT Execute() override;     // IPALExecuteOnUIThread - for reevaluating the shadow policy on the UI thread

    // Test override methods
    void ForceShadowsPolicy(const bool forceShadowsOn);
    void ClearShadowPolicyOverrides();

private:
    _Check_return_ HRESULT OnSettingChanged(_In_ IInspectable*, _In_ IInspectable*);
    _Check_return_ HRESULT ReevaluateShadowPolicy();

    wrl::ComPtr<WUComp::ICompositionCapabilities> m_compositionCapabilities;
    EventRegistrationToken m_compositionCapabilitiesChangedToken{};

    // For get_EnergySaverStatus
    wrl::ComPtr<wsyp::IPowerManagerStatics> m_powerManager;
    EventRegistrationToken m_energySaverStatusChangedToken{};

    // For get_AdvancedEffectsEnabled
    wrl::ComPtr<wuv::IUISettings4> m_uiSettings;
    EventRegistrationToken m_advancedEffectsEnabledChangedToken{};

    // For get_HighContrast
    wrl::ComPtr<wuv::IAccessibilitySettings> m_accessibilitySettings;
    EventRegistrationToken m_highContrastChangedToken{};

    // The various settings changed event handlers can be called off-thread. CCoreServices::ExecuteOnUIThread
    // lets us reevaluate the shadow policy back on the UI thread.
    CCoreServices* m_coreServicesNoRef{nullptr};

    // The callback object must outlive the EffectPolicyHelper, except during teardown when the EffectPolicyHelper
    // might be kept alive temporarily by a ExecuteOnUIThread.
    IEffectPolicyHelperCallback* m_effectPolicyHelperCallbackNoRef{nullptr};

    // Test overrides
    // Note: These are static. Xaml's tests force shadows on once during TestServicesStatics::RuntimeClassInitialize.
    static bool m_forceShadowsOff;
    static bool m_forceShadowsOn;

    bool m_areShadowsEnabled {true};
};
