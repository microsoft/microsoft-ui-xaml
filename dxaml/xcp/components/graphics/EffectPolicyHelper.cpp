// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "EffectPolicyHelper.h"
#include "corep.h"

bool EffectPolicyHelper::m_forceShadowsOff = false;
bool EffectPolicyHelper::m_forceShadowsOn = false;

EffectPolicyHelper::EffectPolicyHelper(_In_ CCoreServices* coreServices, _In_ IEffectPolicyHelperCallback* callback)
    : m_coreServicesNoRef(coreServices)
    , m_effectPolicyHelperCallbackNoRef(callback)
{
    //
    // Note: The app may not have permissions to access the various interfaces that we check as part of our effect
    // policies. If the interface isn't available, we assume default values - see ReevaluateShadowPolicy.
    //

    wrl::ComPtr<IActivationFactory> factory;
    IGNOREHR(wf::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Composition_CompositionCapabilities).Get(),
        &factory));

    if (factory != nullptr)
    {
        wrl::ComPtr<IInspectable> inspectable;
        if (SUCCEEDED(factory->ActivateInstance(&inspectable)))
        {
            IFCFAILFAST(inspectable.As(&m_compositionCapabilities));
        }
    }

    if (m_compositionCapabilities != nullptr)
    {
        // Currently this returns an error - 0x80040155 (Interface not registered). We'll need to pick up the fix for this before enabling it.
        // Task 24400234: Fix CompositionCapabilities in lifted IXP
//            IFCFAILFAST(m_compositionCapabilities->add_Changed(
//                wrl::Callback<wf::ITypedEventHandler<WUComp::CompositionCapabilities*, IInspectable*>>(this, &EffectPolicyHelper::OnSettingChanged).Get(),
//                &m_compositionCapabilitiesChangedToken));
    }

    IGNOREHR(wf::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Windows_System_Power_PowerManager).Get(),
        &m_powerManager));

    if (m_powerManager != nullptr)
    {
        VERIFYHR(m_powerManager->add_EnergySaverStatusChanged(
            wrl::Callback<wf::IEventHandler<IInspectable*>>(this, &EffectPolicyHelper::OnSettingChanged).Get(),
            &m_energySaverStatusChangedToken));
    }

    IGNOREHR(wf::ActivateInstance(
        wrl_wrappers::HStringReference(RuntimeClass_Windows_UI_ViewManagement_UISettings).Get(),
        &m_uiSettings));

    if (m_uiSettings != nullptr)
    {
        VERIFYHR(m_uiSettings->add_AdvancedEffectsEnabledChanged(
            wrl::Callback<wf::ITypedEventHandler<wuv::UISettings*, IInspectable*>>(this, &EffectPolicyHelper::OnSettingChanged).Get(),
            &m_advancedEffectsEnabledChangedToken));
    }

    IGNOREHR(wf::ActivateInstance(
        wrl_wrappers::HStringReference(RuntimeClass_Windows_UI_ViewManagement_AccessibilitySettings).Get(),
        &m_accessibilitySettings));

    if (m_accessibilitySettings)
    {
        // Note: We need a CoreWindow for this call to succeed. We delay the call to Initialize to make sure a CoreWindow
        // is available. When running in an islands, there is no CoreWindow, so we allow the call to fail and ignore changes
        // in high contrast mode.
        // This might not work in islands. We have a dummy CoreWindow there, but it's unclear whether that's good enough. The
        // workaround would be to listen for WM_SETTINGCHANGE.
        // Task 24777629: EffectPolicyHelper's IAccessibilitySettings::add_HighContrastChanged requires a CoreWindow, might not work in islands
        IGNOREHR(m_accessibilitySettings->add_HighContrastChanged(
            wrl::Callback<wf::ITypedEventHandler<wuv::AccessibilitySettings*, IInspectable*>>(this, &EffectPolicyHelper::OnSettingChanged).Get(),
            &m_highContrastChangedToken));
    }

    IFCFAILFAST(ReevaluateShadowPolicy());
}

EffectPolicyHelper::~EffectPolicyHelper()
{
    if (m_compositionCapabilitiesChangedToken.value != 0)
    {
        IGNOREHR(m_compositionCapabilities->remove_Changed(m_compositionCapabilitiesChangedToken));
    }

    if (m_energySaverStatusChangedToken.value != 0)
    {
        IGNOREHR(m_powerManager->remove_EnergySaverStatusChanged(m_energySaverStatusChangedToken));
    }

    if (m_advancedEffectsEnabledChangedToken.value != 0)
    {
        IGNOREHR(m_uiSettings->remove_AdvancedEffectsEnabledChanged(m_advancedEffectsEnabledChangedToken));
    }

    if (m_highContrastChangedToken.value != 0)
    {
        IGNOREHR(m_accessibilitySettings->remove_HighContrastChanged(m_highContrastChangedToken));
    }
}

void EffectPolicyHelper::ClearCallback(_In_ IEffectPolicyHelperCallback* callback)
{
    ASSERT(m_effectPolicyHelperCallbackNoRef == callback);
    m_effectPolicyHelperCallbackNoRef = nullptr;
}

HRESULT EffectPolicyHelper::OnSettingChanged(_In_ IInspectable*, _In_ IInspectable*)
{
    IFC_RETURN(m_coreServicesNoRef->ExecuteOnUIThread(this, ReentrancyBehavior::CrashOnReentrancy));
    return S_OK;
}

HRESULT EffectPolicyHelper::Execute()
{
    // The EffectPolicyHelper might be temporarily kept alive by a ExecuteOnUIThread while the ProjectedShadowManager
    // has already been deleted. In that case it should have called ClearCallback and disconnected itself.
    if (m_effectPolicyHelperCallbackNoRef != nullptr)
    {
        IFC_RETURN(ReevaluateShadowPolicy());
    }

    return S_OK;
}

HRESULT EffectPolicyHelper::ReevaluateShadowPolicy()
{
    bool areShadowsEnabled = m_forceShadowsOn;

    if (m_forceShadowsOff)
    {
        areShadowsEnabled = false;
    }
    else if (!m_forceShadowsOn)
    {
        //
        // Note: The app may not have permissions to access the various interfaces that we check as part of our effect
        // policies. If the interface isn't available, we assume that it passes the policy checks and that it won't disable
        // any effects.
        //

        boolean areEffectsFast = false;
        if (m_compositionCapabilities != nullptr)
        {
            IFC_RETURN(m_compositionCapabilities->AreEffectsFast(&areEffectsFast));
        }

        wsyp::EnergySaverStatus energySaverStatus = wsyp::EnergySaverStatus_On;
        if (m_powerManager != nullptr)
        {
            IFC_RETURN(m_powerManager->get_EnergySaverStatus(&energySaverStatus));
        }
        const bool isEnergySaver = (energySaverStatus == wsyp::EnergySaverStatus_On);

        boolean advancedEffectsEnabled = true;
        if (m_uiSettings != nullptr)
        {
            IFC_RETURN(m_uiSettings->get_AdvancedEffectsEnabled(&advancedEffectsEnabled));
        }

        boolean highContrastEnabled = false;
        if (m_accessibilitySettings != nullptr)
        {
            IFC_RETURN(m_accessibilitySettings->get_HighContrast(&highContrastEnabled));
        }

        areShadowsEnabled = areEffectsFast && !isEnergySaver && advancedEffectsEnabled && !highContrastEnabled;
    }

    if (areShadowsEnabled != m_areShadowsEnabled)
    {
        m_areShadowsEnabled = areShadowsEnabled;

        if (m_effectPolicyHelperCallbackNoRef != nullptr)
        {
            m_effectPolicyHelperCallbackNoRef->OnPolicyChanged();
        }
    }

    return S_OK;
}

void EffectPolicyHelper::ForceShadowsPolicy(const bool forceShadowsOn)
{
    m_forceShadowsOn = forceShadowsOn;
    m_forceShadowsOff = !forceShadowsOn;
    IGNOREHR(ReevaluateShadowPolicy());
}

void EffectPolicyHelper::ClearShadowPolicyOverrides()
{
    m_forceShadowsOff = false;
    m_forceShadowsOn = false;
    IGNOREHR(ReevaluateShadowPolicy());
}