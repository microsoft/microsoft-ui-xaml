﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "MaterialHelper.h"
#include "AcrylicBrush.h"
#include "RevealBrush.h"
#include "XamlAmbientLight.h"
#include "RevealBorderLight.h"
#include "RevealHoverLight.h"
#include "ResourceAccessor.h"
#include "LifetimeHandler.h"
#include <FrameworkUdk/Containment.h>

// Bug 50308952: [1.5 servicing] Add workaround to WinUI for UISettings RPC_E_WRONG_THREAD issue that is being hit by Photos app
#define WINAPPSDK_CHANGEID_50308952 50308952

/* static */
bool MaterialHelperBase::SimulateDisabledByPolicy()
{
    auto instance = LifetimeHandler::GetMaterialHelperInstance();
    return instance ? instance->m_simulateDisabledByPolicy : false;
}

/* static */
void MaterialHelperBase::SimulateDisabledByPolicy(bool value)
{
    const auto instance = LifetimeHandler::GetMaterialHelperInstance();
    const bool oldValue = instance->m_simulateDisabledByPolicy;
    if (oldValue != value)
    {
        instance->m_simulateDisabledByPolicy = value;
        instance->UpdatePolicyStatus();
    }
}

/* static */
bool MaterialHelperBase::IgnoreAreEffectsFast()
{
    auto instance = LifetimeHandler::GetMaterialHelperInstance();
    return instance ? instance->m_ignoreAreEffectsFast : false;
}

/* static */
void MaterialHelperBase::IgnoreAreEffectsFast(bool value)
{
    const auto instance = LifetimeHandler::GetMaterialHelperInstance();
    const bool oldValue = instance->m_ignoreAreEffectsFast;
    if (oldValue != value)
    {
        instance->m_ignoreAreEffectsFast = value;
        instance->UpdatePolicyStatus();
    }
}

/* static */
void MaterialHelperBase::OnRevealBrushConnected()
{
    auto instance = LifetimeHandler::GetMaterialHelperInstance();
    instance->m_revealBrushConnectedCount++;

    if (instance->m_revealBrushConnectedCount == 1)
    {
        // If this is the first connected brush, attach lights.
        if (instance->m_revealLightsToRemove.size() == 0)
        {
            RevealBrush::AttachLights();
        }
    }
}

/* static */
void MaterialHelperBase::OnRevealBrushDisconnected()
{
    auto instance = LifetimeHandler::GetMaterialHelperInstance();

    MUX_ASSERT(instance->m_revealBrushConnectedCount > 0);
    instance->m_revealBrushConnectedCount--;

    if (instance->m_revealBrushConnectedCount == 0)
    {
        // Remove all the lights we created/attached previously now that there's no RevealBrushes active.
        for (auto pair : instance->m_revealLightsToRemove)
        {
            for (auto lightToRemove : pair.second)
            {
                uint32_t index{};
                if (pair.first.IndexOf(lightToRemove, index))
                {
                    pair.first.RemoveAt(index);
                }
            }
        }

        instance->m_revealLightsToRemove.clear();

        // We just removed all lights from the tree. If we were still in the middle of multi-step process of attaching lights,
        // then don't finish attaching the rest. Otherwise we'll leave the tree in a partially-lit state.
        RevealBrush::StopAttachingLights();
    }
}

/* static */
void MaterialHelperBase::TrackRevealLightsToRemove(const winrt::IVector<winrt::XamlLight>& lights, const std::vector<winrt::XamlLight>& revealLightsToRemove)
{
    auto instance = LifetimeHandler::GetMaterialHelperInstance();
    instance->m_revealLightsToRemove.emplace_back(lights, revealLightsToRemove);
}

/* static */
winrt::CompositionEffectFactory MaterialHelperBase::GetOrCreateAcrylicBrushCompositionEffectFactoryFromCache(
    const winrt::Compositor& compositor,
    bool shouldBrushBeOpaque,
    bool useCrossFadeEffect,
    bool useCache,
    std::function<winrt::CompositionEffectFactory()> cacheMissingCallback)
{
    winrt::CompositionEffectFactory factory{ nullptr };

    if (useCache)
    {
        auto instance = LifetimeHandler::GetMaterialHelperInstance();
        instance->AssertUniqueCompositorOrUpdate(compositor);

        const auto key = BuildAcrylicBrushCompositionEffectFactoryKey(shouldBrushBeOpaque, useCrossFadeEffect);
        auto value = instance->m_acrylicBrushCompositionEffectFactoryCache[key];
        if (value)
        {
            // hit cache
            factory = static_cast<winrt::CompositionEffectFactory&>(value);
        }
        else
        {
            // miss, request to create new one and update cache
            factory = cacheMissingCallback();
            instance->m_acrylicBrushCompositionEffectFactoryCache[key] = factory;
        }
    }
    else
    {
        // always create a new one
        factory = cacheMissingCallback();
    }
    return factory;
}

/* static */
winrt::CompositionEffectFactory
MaterialHelperBase::GetOrCreateRevealBrushCompositionEffectFactoryFromCache(
    bool isBorder,
    bool isInverted,
    bool hasBaseColor,
    std::function<winrt::CompositionEffectFactory()> cacheMissingCallback)
{
    auto instance = LifetimeHandler::GetMaterialHelperInstance();

    int key = 0;
    key |= isBorder ? static_cast<int>(RevealBrushCacheFlags::IsBorder) : 0;
    key |= isInverted ? static_cast<int>(RevealBrushCacheFlags::IsInverted) : 0;
    key |= hasBaseColor ? static_cast<int>(RevealBrushCacheFlags::HasBaseColor) : 0;

    winrt::ICompositionEffectFactory& factory =
        instance->m_revealBrushCompositionEffectFactoryCache[key];

    if (!factory)
    {
        // miss cache, write the result back into the field we grabbed above.
        factory = cacheMissingCallback();
    }

    return static_cast<winrt::CompositionEffectFactory&>(factory);
}

/* static */
int constexpr MaterialHelperBase::BuildAcrylicBrushCompositionEffectFactoryKey(
    bool shouldBrushBeOpaque,
    bool useCrossFadeEffect)
{
    int key = 0;
    key |= shouldBrushBeOpaque ? AcrylicBrushCacheHelperParam::ShouldBrushBeOpaque : 0;
    key |= useCrossFadeEffect ? AcrylicBrushCacheHelperParam::UseCrossFadeEffect : 0;
    return key;
}

void MaterialHelperBase::AssertUniqueCompositorOrUpdate(const winrt::Compositor& compositor)
{
    if (m_acrylicCompositor)
    {
        MUX_ASSERT(compositor == m_acrylicCompositor);
    }
    else
    {
        m_acrylicCompositor = compositor;
    }
}

winrt::CompositionSurfaceBrush MaterialHelperBase::CreateScaledBrush(int dpiScale)
{
    winrt::Compositor compositor = winrt::CompositionTarget::GetCompositorForCurrentThread();
    winrt::LoadedImageSurface surface{ ResourceAccessor::GetImageSurface(IR_NoiseAsset_256X256_PNG, { 256, 256 }) };
    winrt::CompositionSurfaceBrush noiseBrush = compositor.CreateSurfaceBrush(surface);

    // Noise should never be stretched (we tile it instead)
    noiseBrush.Stretch(winrt::CompositionStretch::None);
    // Align the noise to the top-left. This way it doesn't move around when the element gets resized to the right or bottom.
    // The default has it centered, which moves the noise texture as elements are resized and makes them shimmer.
    noiseBrush.HorizontalAlignmentRatio(0.0f);
    noiseBrush.VerticalAlignmentRatio(0.0f);

    // Undo dpi scale on the noise (we tile it instead)
    noiseBrush.Scale(winrt::float2(1.0f / (static_cast<float>(dpiScale) / 100.0f)));

    return noiseBrush;
}

template <typename T>
/*static*/ void MaterialHelperBase::LightPolicyChangedHelper(T* instance, bool isDisabledByMaterialPolicy)
{
    if (instance->m_isDisabledByMaterialPolicy != isDisabledByMaterialPolicy)
    {
        instance->m_isDisabledByMaterialPolicy = isDisabledByMaterialPolicy;

        if (instance->m_isDisabledByMaterialPolicy)
        {
            instance->ReleaseCompositionResources();
        }
        else
        {
            instance->EnsureCompositionResources();
        }
    }
}
template void MaterialHelperBase::LightPolicyChangedHelper<XamlAmbientLight>(XamlAmbientLight* instance, bool isDisabledByMaterialPolicy);
template void MaterialHelperBase::LightPolicyChangedHelper<RevealBorderLight>(RevealBorderLight* instance, bool isDisabledByMaterialPolicy);
template void MaterialHelperBase::LightPolicyChangedHelper<RevealHoverLight>(RevealHoverLight* instance, bool isDisabledByMaterialPolicy);

// *****************************************
// ***** MUX version of MaterialHelper *****
// *****************************************
/* static */
winrt::event_token MaterialHelper::PolicyChanged(const std::function<void(const com_ptr<MaterialHelperBase>&, bool)>& handler)
{
    auto instance = LifetimeHandler::GetMaterialHelperInstance();
    handler(instance, instance->m_isDisabledByMaterialPolicy);
    return instance->m_policyChangedListeners.add(handler);
}

/* static */
void MaterialHelper::PolicyChanged(winrt::event_token removeToken)
{
    if (auto instance = LifetimeHandler::TryGetMaterialHelperInstance())
    {
        instance->m_policyChangedListeners.remove(removeToken);
    }
}

/* static */
winrt::event_token MaterialHelper::NoiseChanged(const std::function<void(const com_ptr<MaterialHelperBase>&)>& handler)
{
    auto instance = LifetimeHandler::GetMaterialHelperInstance();
    handler(instance);
    return instance->m_noiseChangedListeners.add(handler);
}

/* static */
void MaterialHelper::NoiseChanged(winrt::event_token removeToken)
{
    if (auto instance = LifetimeHandler::TryGetMaterialHelperInstance())
    {
        instance->m_noiseChangedListeners.remove(removeToken);
    }
}

MaterialHelper::MaterialHelper()
{
    try
    {
        m_energySaverStatusChangedRevoker = winrt::PowerManager::EnergySaverStatusChanged(winrt::auto_revoke, { this, &MaterialHelper::OnEnergySaverStatusChanged });
        m_energySaverStatusChangedRevokerValid = true;
    }
    catch (winrt::hresult_error)
    {
        // Some processes like EdgeCP don't have access to activate PowerManager. In those cases, just
        // always run in fallback mode.
    }

    // Sometimes we are called so early that we can't access the CoreWindow yet. In these cases we know that
    // the CompositionCapabilities API will also fail. So don't try to do that until CompositionTarget::Rendering.
    // Same for accessing DisplayInformation and obtainting CoreWindow for this view.
    m_dispatcherQueue = winrt::DispatcherQueue::GetForCurrentThread();
    const auto coreWindow = winrt::CoreWindow::GetForCurrentThread();
    if (coreWindow)
    {
        EnsureCompositionCapabilities();
        HookupDpiChangedHandler();
    }
    else
    {
        auto strongThis = get_strong();
        SharedHelpers::QueueCallbackForCompositionRendering([strongThis]() {
            // Try again to evaluate composition capabilities.
            strongThis->EnsureCompositionCapabilities();
            strongThis->UpdatePolicyStatus(true /* onUIThread */);

            // ... and DisplayInformation to sign up for DpiChanged
            strongThis->HookupDpiChangedHandler();
        });
    }

    winrt::UISettings uiSettings; // Make an instance to be able to listen for changes.
                                  // Convert to IUISettings4 because (a) we're only using the APIs off that interface and (b) some platforms don't implement this.
    m_uiSettings = uiSettings.try_as<winrt::IUISettings4>();
    if (m_uiSettings)
    {
        // If we failed to attach to this event then just assume that the running OS (like OneCore) doesn't have this
        // implemented yet. Can be removed once MSFT#11982393 is fixed.
        try
        {
            m_advancedEffectsEnabledChangedToken = m_uiSettings.AdvancedEffectsEnabledChanged({ this, &MaterialHelper::OnUISettingsChanged });
        }
        catch (winrt::hresult_error)
        {
            m_uiSettings = nullptr;
        }
    }

    UpdatePolicyStatus(true /* onUIThread */);
}

void MaterialHelper::HookupDpiChangedHandler()
{
    if (auto currentView = SharedHelpers::TryGetCurrentCoreApplicationView())
    {
        winrt::DisplayInformation displayInformation = winrt::DisplayInformation::GetForCurrentView();
        m_dpiChangedRevoker = displayInformation.DpiChanged(winrt::auto_revoke, { this, &MaterialHelper::OnDpiChanged });
        m_logicalDpi = displayInformation.LogicalDpi();
    }
    else
    {
        // Calling GetForCurrentView on threads without a CoreWindow throws an error. This comes up in places like LogonUI.
        // Ignore the error and assume a DPI.

        m_logicalDpi = 96;  // This isn't correct. Xaml has internal code that handles XamlPresenter scenarios, but that isn't available through public APIs. We can fix this for WUXC but not MUX.
    }
}

void MaterialHelper::EnsureCompositionCapabilities()
{
    if (!m_compositionCapabilities)
    {
        try
        {
            m_compositionCapabilities = winrt::CompositionCapabilities();
            m_compositionCapabilitiesChangedToken = m_compositionCapabilities.Changed({ this, &MaterialHelper::OnCompositionCapabilitiesChanged });
        }
        catch (winrt::hresult_error)
        {
            // Some processes don't have access to activate CompositionCapabilities. In those cases, just
            // always run in fallback mode.
        }
    }
}

MaterialHelper::~MaterialHelper()
{
    if (m_compositionCapabilities)
    {
        m_compositionCapabilities.Changed(m_compositionCapabilitiesChangedToken);
    }

    if (m_uiSettings)
    {
        m_uiSettings.AdvancedEffectsEnabledChanged(m_advancedEffectsEnabledChangedToken);
    }
}

void MaterialHelper::OnEnergySaverStatusChanged(const winrt::IInspectable& /*sender*/, const winrt::IInspectable& /*args*/)
{
    UpdatePolicyStatus();
}

void MaterialHelper::OnCompositionCapabilitiesChanged(const winrt::CompositionCapabilities& /*sender*/, const winrt::IInspectable& /*args*/)
{
    UpdatePolicyStatus();
}

void MaterialHelper::OnUISettingsChanged(const winrt::UISettings& /*sender*/, const winrt::IInspectable& /*args*/)
{
    UpdatePolicyStatus();
}

// If plateau scale changed (eg by moving between different res screens in multimon),
// reload the noise surface to prevent noise from being scaled
void MaterialHelper::OnDpiChanged(const winrt::IInspectable& sender, const winrt::IInspectable& /*args*/)
{
    const auto previousLogicalDpi = m_logicalDpi;

    try
    {
        m_logicalDpi = (sender.as<winrt::DisplayInformation>()).LogicalDpi();
    }
    catch (winrt::hresult_error)
    {
        // Watson Bugs 12990478 and 13071055 suggest CDisplayInformation::get_LogicalDpi can fail with
        // 0x80070578 : ERROR_INVALID_WINDOW_HANDLE  if the current view  is somehow not available.
        // This likely means the view has been closed and its logical DPI is no longer relevant.
        // Ignore the error and do not notify subscriber materials in this case.

        // TODO: Consider adding below assert to get data on whether we are swallowing other errors here.
        //MUX_ASSERT(e.to_abi() == ERROR_INVALID_WINDOW_HANDLE);
    }

    // We also get here in case of (logical) Resolution change, ignore that case
    if (previousLogicalDpi != m_logicalDpi)
    {
        ResetNoise();
        auto strongThis = get_strong();
        m_noiseChangedListeners(strongThis);
    }
}

void MaterialHelper::UpdatePolicyStatus(bool onUIThread)
{
    auto strongThis = get_strong();
    auto callback = [strongThis, this]() {
        const bool isEnergySaverMode = m_energySaverStatusChangedRevokerValid ? winrt::PowerManager::EnergySaverStatus() == winrt::EnergySaverStatus::On : true;
        const bool areEffectsFast = m_compositionCapabilities ? (m_compositionCapabilities.AreEffectsFast() || m_ignoreAreEffectsFast) : false;

        bool advancedEffectsEnabled = false;
        if (WinAppSdk::Containment::IsChangeEnabled<WINAPPSDK_CHANGEID_50308952>())
        {
            // We are hitting an issue in Photos where UISettings::AdvancedEffectsEnabled is returning RPC_E_WRONG_THREAD.
            // We work around this issue by;
            //   1. Use a fresh instance of UISettings instead of the cached m_uiSettings.
            //   2. Ignore RPC_E_WRONG_THREAD and use a fallback value.
            try
            {
                winrt::UISettings uiSettings;
                advancedEffectsEnabled = uiSettings.AdvancedEffectsEnabled();
            }
            catch (winrt::hresult_error e)
            {
                if (e.to_abi() != RPC_E_WRONG_THREAD) { throw; }
            }
        }
        else
        {
            advancedEffectsEnabled = m_uiSettings ? m_uiSettings.AdvancedEffectsEnabled() : true;
        }

        bool isDisabledByPolicy = m_simulateDisabledByPolicy || (isEnergySaverMode || !areEffectsFast || !advancedEffectsEnabled);

        if (m_isDisabledByMaterialPolicy != isDisabledByPolicy)
        {
            m_isDisabledByMaterialPolicy = isDisabledByPolicy;
            m_policyChangedListeners(strongThis, m_isDisabledByMaterialPolicy);
        }
    };

    if (onUIThread)
    {
        callback();
    }
    else
    {
        if (m_dispatcherQueue) // We might have no dispatcher in XamlPresenter scenarios, in this case we will always be in the disabled state.
        {
            if (m_dispatcherQueue.HasThreadAccess())
            {
                callback();
            }
            else
            {
                m_dispatcherQueue.TryEnqueue(callback);
            }
        }
    }
}

void MaterialHelper::ResetNoise()
{
    if (m_noiseSurface)
    {
        m_noiseSurface.Close();
        m_noiseSurface = nullptr;
    }

    if (m_noiseBrush)
    {
        m_noiseBrush.Close();
        m_noiseBrush = nullptr;
    }
}

/* static */
winrt::CompositionSurfaceBrush MaterialHelper::GetNoiseBrush()
{
    auto instance = LifetimeHandler::GetMaterialHelperInstance();
    return instance->GetNoiseBrushImpl();
}

winrt::CompositionSurfaceBrush MaterialHelper::GetNoiseBrushImpl()
{
    if (!m_noiseBrush)
    {
        int resScaleInt = 100; // maps to winrt::ResolutionScale::Scale100Percent;

        if (winrt::CoreWindow::GetForCurrentThread())
        {
            try
            {
                winrt::ResolutionScale resScale{ winrt::DisplayInformation::GetForCurrentView().ResolutionScale() };
                if (resScale != winrt::ResolutionScale::Invalid)
                {
                    resScaleInt = static_cast<int>(resScale);
                }
                else
                {
                    MUX_ASSERT_MSG(false, "Could not get ResolutionScale for display.");
                }
            }
            catch (const winrt::hresult_error&)
            {
                // Calling GetForCurrentView on threads without a CoreWindow throws an error. This comes up in places like LogonUI.
                // Assume 1.0 scaling and don't touch the noise brush.
                // Assuming 1.0 scaling isn't correct. Xaml has internal code that handles XamlPresenter scenarios, but that isn't available through public APIs. We can fix this for WUXC but not MUX.
            }
        }

        m_noiseBrush = CreateScaledBrush(resScaleInt);
    }

    return m_noiseBrush;
}

// On RS2, we could be waiting for deferred noise recreation via OnVisibilityChanged.
// In this case, it's not safe yet to recreate the brush.
/* static */
bool MaterialHelper::RS2IsSafeToCreateNoise()
{
    auto instance = LifetimeHandler::GetMaterialHelperInstance();
    return !instance->m_wasWindowHidden && !instance->m_waitingForRenderingAfterBecomingVisible;
}

/* static */
void MaterialHelper::SetShouldBeginAttachingLights(bool shouldBeginAttachingLights)
{
    auto instance = LifetimeHandler::GetMaterialHelperInstance();
    instance->m_shouldBeginAttachingLights = shouldBeginAttachingLights;
}

/* static */
bool MaterialHelper::ShouldBeginAttachingLights()
{
    auto instance = LifetimeHandler::GetMaterialHelperInstance();
    return instance->m_shouldBeginAttachingLights;
}

/* static */
void MaterialHelper::SetShouldContinueAttachingLights(bool shouldContinueAttachingLights)
{
    auto instance = LifetimeHandler::GetMaterialHelperInstance();
    instance->m_shouldContinueAttachingLights = shouldContinueAttachingLights;
}

/* static */
bool MaterialHelper::ShouldContinueAttachingLights()
{
    auto instance = LifetimeHandler::GetMaterialHelperInstance();
    return instance->m_shouldContinueAttachingLights;
}

/* static */
void MaterialHelper::ResetFailedToAttachLightsCount()
{
    auto instance = LifetimeHandler::GetMaterialHelperInstance();
    instance->m_failedToAttachLightsCount = 0;
}

/* static */
bool MaterialHelper::IncrementAndCheckFailedToAttachLightsCount()
{
    auto instance = LifetimeHandler::GetMaterialHelperInstance();
    instance->m_failedToAttachLightsCount++;
    return instance->FailedToAttachLights();
}

bool MaterialHelper::FailedToAttachLights()
{
    return m_failedToAttachLightsCount > sc_maxFailedToAttachLightsCount;
}
