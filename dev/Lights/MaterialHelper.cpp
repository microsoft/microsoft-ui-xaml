// Copyright (c) Microsoft Corporation. All rights reserved.
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
#if BUILD_WINDOWS
        instance->NotifyAdditionalPolicyChangedListeners();
#else
        instance->UpdatePolicyStatus();
#endif
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
#if BUILD_WINDOWS
        instance->NotifyAdditionalPolicyChangedListeners();
#else
        instance->UpdatePolicyStatus();
#endif
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
            // Ensure that ambient and border lights needed for reveal effects are set on tree root
            if (SharedHelpers::IsRS2OrHigher())
            {
                RevealBrush::AttachLights();
            }
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

#ifndef BUILD_WINDOWS
        // We just removed all lights from the tree. If we were still in the middle of multi-step process of attaching lights,
        // then don't finish attaching the rest. Otherwise we'll leave the tree in a partially-lit state.
        RevealBrush::StopAttachingLights();
#endif
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
    bool useWindowAcrylic,
    bool useCrossFadeEffect,
    bool useCache,
    std::function<winrt::CompositionEffectFactory()> cacheMissingCallback)
{
    winrt::CompositionEffectFactory factory{ nullptr };

    if (useCache)
    {
        auto instance = LifetimeHandler::GetMaterialHelperInstance();
        instance->AssertUniqueCompositorOrUpdate(compositor);

        const auto key = BuildAcrylicBrushCompositionEffectFactoryKey(shouldBrushBeOpaque, useWindowAcrylic, useCrossFadeEffect);
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
    bool useWindowAcrylic,
    bool useCrossFadeEffect)
{
    int key = 0;
    key |= shouldBrushBeOpaque ? AcrylicBrushCacheHelperParam::ShouldBrushBeOpaque : 0;
    key |= useWindowAcrylic ? AcrylicBrushCacheHelperParam::UseWindowAcrylic : 0;
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
    winrt::Compositor compositor = winrt::Window::Current().Compositor();
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

#if BUILD_WINDOWS
// ****************************************
// **** WUXC version of MaterialHelper ****
// ****************************************
/* static */
winrt::event_token MaterialHelper::AdditionalPolicyChanged(const std::function<void(const com_ptr<MaterialHelperBase>&)>& handler)
{
    auto instance = LifetimeHandler::GetMaterialHelperInstance();
    handler(instance);
    return instance->m_additionalPolicyChangedListeners.add(handler);
}

/* static */
void MaterialHelper::AdditionalPolicyChanged(winrt::event_token removeToken)
{
    if (auto instance = LifetimeHandler::TryGetMaterialHelperInstance())
    {
        instance->m_additionalPolicyChangedListeners.remove(removeToken);
    }
}


/* static */
bool MaterialHelper::RevealBorderLightUnavailable()
{
    auto instance = LifetimeHandler::GetMaterialHelperInstance();
    return instance ? instance->m_revealBorderLightUnavailable : false;
}

/* static */
void MaterialHelper::RevealBorderLightUnavailable(bool value)
{
    auto instance = LifetimeHandler::GetMaterialHelperInstance();
    bool oldValue = instance->m_revealBorderLightUnavailable;
    if (oldValue != value)
    {
        instance->m_revealBorderLightUnavailable = value;
        instance->NotifyAdditionalPolicyChangedListeners();
    }
}

/* static */
winrt::CompositionSurfaceBrush MaterialHelper::GetNoiseBrush(int dpiScale)
{
    auto instance = LifetimeHandler::GetMaterialHelperInstance();
    return instance->GetNoiseBrushImpl(dpiScale);
}

winrt::CompositionSurfaceBrush MaterialHelper::GetNoiseBrushImpl(int dpiScale)
{
    winrt::CompositionSurfaceBrush noiseBrush{ nullptr };

    auto it = m_dpiScaledNoiseBrushes.find(dpiScale);
    if (it != m_dpiScaledNoiseBrushes.end())
    {
        noiseBrush = it->second;
    }
    else
    {
        noiseBrush = CreateScaledBrush(dpiScale);
        m_dpiScaledNoiseBrushes.emplace(dpiScale, noiseBrush);
    }

    return noiseBrush;
}

template <typename T>
/*static*/
void MaterialHelper::LightTemplates<T>::OnLightTransparencyPolicyChanged(
    const winrt::weak_ref<T> weakInstance,
    const winrt::IMaterialProperties& materialProperties,
    const winrt::DispatcherQueue& dispatcherQueue,
    bool onUIThread)
{
    auto callback = [weakInstance, dispatcherQueue, materialProperties]() {
        auto instance = weakInstance.get();
        if (instance)
        {
            bool isDisabledByMaterialPolicy =
                (materialProperties.InAppTransparencyPolicy() == winrt::Windows::UI::TransparencyPolicy::Opaque && !MaterialHelper::IgnoreAreEffectsFast()) || MaterialHelper::SimulateDisabledByPolicy();
            LightPolicyChangedHelper(instance.get(), isDisabledByMaterialPolicy);
        }
    };

    if (onUIThread)
    {
        callback();
    }
    else
    {
        if (dispatcherQueue) // We might have no dispatcher in XamlPresenter scenarios, in this case we will always be in the disabled state.
        {
            dispatcherQueue.TryEnqueue(winrt::Windows::System::DispatcherQueueHandler(callback));
        }
    }
}

template class MaterialHelper::LightTemplates<XamlAmbientLight>;
template class MaterialHelper::LightTemplates<RevealHoverLight>;
template class MaterialHelper::LightTemplates<RevealBorderLight>;

template <typename T>
/*static*/ void MaterialHelper::BrushTemplates<T>::HookupWindowDpiChangedHandler(T* instance)
{
    if (!instance->m_dpiChangedRevoker)
    {
        try
        {
            // TODO_FluentIslands: Can we cache displayInformation? The GetForCurrentView() could be an expensive call...

            // Capture strong instance of brush in the lambda in case we get called after being disconencted (and possibly destructed).
            // This can happen if some other handler of DisplayInformation.DpiChanged gets invoked first and 
            // causes this brush to leave the live tree, get disconnected and unregister from its DPIChanged handler - 
            // that unregistration doesn't affect the current firing event because the DpiChanged event handler list is locked.
            com_ptr<T> strongInstance = instance->get_strong();

            winrt::DisplayInformation displayInformation = winrt::DisplayInformation::GetForCurrentView();
            instance->m_dpiChangedRevoker = displayInformation.DpiChanged(winrt::auto_revoke, {
                [strongInstance](const winrt::DisplayInformation& displayInformation, const winrt::IInspectable&)
                {
                    float previousLogicalDpi = strongInstance->m_logicalDpi;

                    try
                    {
                        strongInstance->m_logicalDpi = displayInformation.LogicalDpi();
                    }
                    catch (winrt::hresult_error)
                    {
                        // Watson Bugs 12990478 and 13071055 suggest CDisplayInformation::get_LogicalDpi can fail with 
                        // 0x80070578 : ERROR_INVALID_WINDOW_HANDLE  if the current view  is somehow not available.
                        // This likely means the view has been closed and its logical DPI is no longer relevant. 
                        // Ignore the error and do not notify subscriber materials in this case. 

                        // TODO: Consider adding below assert to get data on whether we are swallowing other errors here.
                        //_ASSERT(e.to_abi() == ERROR_INVALID_WINDOW_HANDLE);
                    }

                    // We also get here in case of (logical) Resolution change, ignore that case
                    if (previousLogicalDpi != strongInstance->m_logicalDpi)
                    {
                        UpdateDpiScaledNoiseBrush(strongInstance.get());
                    }
                } });

            instance->m_logicalDpi = displayInformation.LogicalDpi();
        }
        catch (winrt::hresult_error)
        {
            // Calling GetForCurrentView on threads without a CoreWindow throws an error. This comes up in places like LogonUI.
            // Ignore the error and assume a DPI.

            instance->m_logicalDpi = 96;  // This isn't correct. Xaml has internal code that handles XamlPresenter scenarios, but that isn't available through public APIs. We can fix this for WUXC but not MUX.
        }
    }
}

template <typename T>
/*static*/ void  MaterialHelper::BrushTemplates<T>::UnhookWindowDpiChangedHandler(T* instance)
{
    instance->m_dpiChangedRevoker.revoke();
}

template <typename T>
/*static*/ void MaterialHelper::BrushTemplates<T>::HookupIslandDpiChangedHandler(T* instance)
{
    if (!instance->m_islandTransformChangedToken.value)
    {
        instance->m_associatedCompositionIsland = (instance->m_associatedIsland.AppContent()).try_as<winrt::UIContentRoot>().Island();
        instance->m_islandTransformChangedToken = instance->m_associatedCompositionIsland.StateChanged({ instance, &T::OnIslandTransformChanged });
    }
}

template <typename T>
/*static*/ void MaterialHelper::BrushTemplates<T>::UnhookIslandDpiChangedHandler(T* instance)
{
    if (instance->m_islandTransformChangedToken.value)
    {
        instance->m_associatedCompositionIsland.StateChanged(instance->m_islandTransformChangedToken);
        instance->m_islandTransformChangedToken.value = 0;
    }
    instance->m_associatedCompositionIsland = nullptr;
}

template <typename T>
/*static*/ void MaterialHelper::BrushTemplates<T>::OnIslandTransformChanged(T* instance)
{
    UpdateDpiScaledNoiseBrush(instance);
}

template <typename T>
/*static*/ void MaterialHelper::BrushTemplates<T>::UpdateDpiScaledNoiseBrush(T* instance)
{
    // If we are using acrylic effect now, now we can refresh the existing one with new noise.
    // Otherwise mark it so that new noise is used when the effect brush is created.

    // Check for m_isConnected in case DpiChanged event unregistration no-oped (see comment in HookupWindowDpiChangedHandler() for details).
    if (!instance->IsInFallbackMode() && instance->m_isConnected)
    {
        int resScaleInt = GetEffectiveDpi(instance);

        if (resScaleInt != 0)
        {
            auto dpiScaledNoiseBrush = GetNoiseBrush(resScaleInt);
            _ASSERT(dpiScaledNoiseBrush != instance->m_dpiScaledNoiseBrush);

            winrt::CompositionEffectBrush effectBrush = instance->m_brush.try_as<winrt::CompositionEffectBrush>();
            effectBrush.SetSourceParameter(L"Noise", dpiScaledNoiseBrush);
            instance->m_dpiScaledNoiseBrush = dpiScaledNoiseBrush;
        }
    }
    else
    {
        instance->m_noiseChanged = true;
    }
}

template <typename T>
/*static*/ int MaterialHelper::BrushTemplates<T>::GetEffectiveDpi(T* instance)
{
    int resolutionScale = 100;           // Use 100% scale if we can't get DisplayInformation.GetForCurrentView (eg XamlPresenter)

    if (instance->m_associatedIsland)
    {
        winrt::CompositionIsland compIsland = (instance->m_associatedIsland.AppContent()).try_as<winrt::UIContentRoot>().Island();
        resolutionScale = static_cast<int>(std::round(compIsland.RasterizationScale() * 100.0f));
    }
    else
    {
        try
        {
            resolutionScale = static_cast<int>(winrt::DisplayInformation::GetForCurrentView().ResolutionScale());
        }
        catch (winrt::hresult_error)
        {
            // Calling GetForCurrentView on threads without a CoreWindow throws an error. This comes up in places like LogonUI.
            // Assume 1.0 scaling and don't touch the noise brush.

            // Assuming 1.0 scaling isn't correct. Xaml has internal code that handles XamlPresenter scenarios, but that isn't available through public APIs. We can fix this for WUXC but not MUX.
        }
    }

    return resolutionScale;
}

template <typename T>
/*static*/ bool MaterialHelper::BrushTemplates<T>::IsDisabledByInAppTransparencyPolicy(T* instance)
{
    bool isDisabledByInAppTransparencyPolicy = false;

    if (instance->m_materialProperties)
    {
        isDisabledByInAppTransparencyPolicy = (instance->m_materialProperties.InAppTransparencyPolicy() == winrt::Windows::UI::TransparencyPolicy::Opaque && !IgnoreAreEffectsFast()) || SimulateDisabledByPolicy();
    }
    else
    {
        isDisabledByInAppTransparencyPolicy = SimulateDisabledByPolicy();
    }

    return isDisabledByInAppTransparencyPolicy;
}

/*static*/ bool MaterialHelper::BrushTemplates<RevealBrush>::IsDisabledByInAppTransparencyPolicy(RevealBrush* instance)
{
    bool isDisabledByInAppTransparencyPolicy = false;

    if (instance->m_materialProperties)
    {
        isDisabledByInAppTransparencyPolicy = (instance->m_materialProperties.InAppTransparencyPolicy() == winrt::Windows::UI::TransparencyPolicy::Opaque && !IgnoreAreEffectsFast()) || SimulateDisabledByPolicy() || RevealBorderLightUnavailable();
    }
    else
    {
        isDisabledByInAppTransparencyPolicy = SimulateDisabledByPolicy() || RevealBorderLightUnavailable();
    }

    return isDisabledByInAppTransparencyPolicy;
}

template <typename T>
/*static*/ bool MaterialHelper::BrushTemplates<T>::IsDisabledByHostBackdropTransparencyPolicy(T* instance)
{
    bool isDisabledByHostBackdropPolicy = false;

    if (instance->m_materialProperties)
    {
        isDisabledByHostBackdropPolicy = (instance->m_materialProperties.HostBackdropTransparencyPolicy() == winrt::Windows::UI::TransparencyPolicy::Opaque && !IgnoreAreEffectsFast()) || SimulateDisabledByPolicy();
    }
    else
    {
        isDisabledByHostBackdropPolicy = SimulateDisabledByPolicy();
    }

    return isDisabledByHostBackdropPolicy;
}

template class MaterialHelper::BrushTemplates<AcrylicBrush>;
template class MaterialHelper::BrushTemplates<RevealBrush>;

/* static */
void MaterialHelper::OnRevealBrushConnectedIsland(const winrt::XamlIsland& island)
{
    auto instance = LifetimeHandler::GetMaterialHelperInstance();
    auto it = instance->m_islandBorderLights.find(island);

    if (it == instance->m_islandBorderLights.end())
    {
        IslandBorderLightInfo newBorderLightInfo;
        newBorderLightInfo.m_revealBrushConnectedCount = 1;

        instance->m_islandBorderLights.emplace(island, newBorderLightInfo);

        // Ensure that ambient and border lights needed for reveal effects are set on IslandRoot
        RevealBrush::AttachLightsToIsland(island);
    }
    else
    {
        it->second.m_revealBrushConnectedCount++;
    }
}

/* static */
void MaterialHelper::OnRevealBrushDisconnectedIsland(const winrt::XamlIsland& island)
{
    auto instance = LifetimeHandler::GetMaterialHelperInstance();
    auto it = instance->m_islandBorderLights.find(island);
    auto& islandBorderLightInfo = it->second;

    _ASSERT(instance->m_revealBrushConnectedCount > 0);
    islandBorderLightInfo.m_revealBrushConnectedCount--;

    if (islandBorderLightInfo.m_revealBrushConnectedCount == 0)
    {
        // Remove all the lights we created/attached previously now that there's no RevealBrushes active.
        auto& pair = islandBorderLightInfo.m_revealLightsToRemove;
        for (auto lightToRemove : pair.second)
        {
            uint32_t index{};
            if (pair.first.IndexOf(lightToRemove, index))
            {
                pair.first.RemoveAt(index);
            }
        }

        pair.first = nullptr;
        pair.second.clear();
        instance->m_islandBorderLights.erase(island);
    }
}

/* static */
void MaterialHelper::TrackRevealLightsToRemoveIsland(const winrt::XamlIsland& island, const winrt::IVector<winrt::XamlLight>& lights, const std::vector<winrt::XamlLight>& revealLightsToRemove)
{
    auto instance = LifetimeHandler::GetMaterialHelperInstance();
    auto it = instance->m_islandBorderLights.find(island);
    it->second.m_revealLightsToRemove = std::make_pair(lights, revealLightsToRemove);
}

void MaterialHelper::NotifyAdditionalPolicyChangedListeners()
{
    auto strongThis = get_strong();
    m_additionalPolicyChangedListeners(strongThis);
}

#else
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
winrt::event_token MaterialHelper::WindowSizeChanged(const std::function<void(const com_ptr<MaterialHelperBase>&, bool)>& handler)
{
    auto instance = LifetimeHandler::GetMaterialHelperInstance();
    instance->EnsureSizeChangedHandler();
    return instance->m_sizeChangedListeners.add(handler);
}

/* static */
void MaterialHelper::WindowSizeChanged(winrt::event_token removeToken)
{
    if (auto instance = LifetimeHandler::TryGetMaterialHelperInstance())
    {
        instance->m_sizeChangedListeners.remove(removeToken);
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

    // Sometimes we are called so early that we can't access the Dispatcher yet. In these cases we know that
    // the CompositionCapabilities API will also fail. So don't try to do that until CompositionTarget::Rendering.
    // Same for accessing DisplayInformation and obtainting CoreWindow for this view.
    m_dispatcher = winrt::Window::Current().Dispatcher();
    if (m_dispatcher)
    {
        EnsureCompositionCapabilities();
        HookupDpiChangedHandler();

        // For RS2 apps, we susbscribe to VisibilityChanged to work around bug 11159685.
        if (!SharedHelpers::IsRS3OrHigher())
        {
            HookupVisibilityChangedHandler();
        }
    }
    else
    {
        auto strongThis = get_strong();
        SharedHelpers::QueueCallbackForCompositionRendering([strongThis]() {
            // Try again to grab the dispatcher and evaluate composition capabilities.
            strongThis->m_dispatcher = winrt::Window::Current().Dispatcher();
            strongThis->EnsureCompositionCapabilities();
            strongThis->UpdatePolicyStatus(true /* onUIThread */);

            // ... and DisplayInformation to sign up for DpiChanged
            strongThis->HookupDpiChangedHandler();

            // ... and get the CoreWindow to sign up for VisibilityChanged
            if (!SharedHelpers::IsRS3OrHigher())
            {
                strongThis->HookupVisibilityChangedHandler();
            }
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
    try
    {
        winrt::DisplayInformation displayInformation = winrt::DisplayInformation::GetForCurrentView();
        m_dpiChangedRevoker = displayInformation.DpiChanged(winrt::auto_revoke, { this, &MaterialHelper::OnDpiChanged });
        m_logicalDpi = displayInformation.LogicalDpi();
    }
    catch (winrt::hresult_error)
    {
        // Calling GetForCurrentView on threads without a CoreWindow throws an error. This comes up in places like LogonUI.
        // Ignore the error and assume a DPI.

        m_logicalDpi = 96;  // This isn't correct. Xaml has internal code that handles XamlPresenter scenarios, but that isn't available through public APIs. We can fix this for WUXC but not MUX.
    }
}

void MaterialHelper::HookupVisibilityChangedHandler()
{
    winrt::CoreWindow coreWindow = winrt::Window::Current().CoreWindow();
    m_visibilityChangedRevoker = coreWindow.VisibilityChanged(winrt::auto_revoke, { this, &MaterialHelper::OnVisibilityChanged });
}

// Only sign up for Window.SizeChanged events if there is a connected AcrylicBrush. In particular, reveal-only apps shouldn't incur the cost here.
void MaterialHelper::EnsureSizeChangedHandler()
{
    if (m_windowSizeChangedToken.value == 0)
    {
        m_currentWindow = winrt::Window::Current();
        m_windowSizeChangedToken = m_currentWindow.SizeChanged({ this, &MaterialHelper::OnSizeChanged });
    }
}

void MaterialHelper::EnsureCompositionCapabilities()
{
    if (!m_compositionCapabilities && SharedHelpers::IsRS2OrHigher())
    {
        try
        {
            m_compositionCapabilities = winrt::CompositionCapabilities::GetForCurrentView();
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

    if (m_windowSizeChangedToken.value && m_currentWindow)
    {
        m_currentWindow.SizeChanged(m_windowSizeChangedToken);
        m_currentWindow = nullptr;
        m_windowSizeChangedToken.value = 0;
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

// Xaml may have offered its DComp resources - particularly the Noise texture - while app window was not visible.
// Due to a bug 11159685 (fixed in RS3), LoadedImageSurface may fail to reclaim an offered and discarded surface, 
// resulitng in "noiseless acrylic". Reload the noise with a new LIS when app regains visibilty as a workaround.
void MaterialHelper::OnVisibilityChanged(const winrt::CoreWindow&, const winrt::VisibilityChangedEventArgs& args)
{
    MUX_ASSERT(!SharedHelpers::IsRS3OrHigher());

    if (args.Visible())
    {
        if (m_wasWindowHidden)
        {
            // We are transitioning to visible state (m_wasWindowHidden helps exclude initial change to Visible on Startup)
            m_wasWindowHidden = false;
            m_waitingForRenderingAfterBecomingVisible = true;

            // Attempting immediately recreate LoadedImageSurface here may result in an AV when this
            // visibility changed event is a part of device lost recovery and/or app resume.
            // Defer creation until next Rendering event when the graphics device is in the better shape.
            SharedHelpers::QueueCallbackForCompositionRendering(
                [strongThis = get_strong()]
            {
                // The window may become invisible again before we process this tick
                if (!strongThis->m_wasWindowHidden)
                {
                    strongThis->m_waitingForRenderingAfterBecomingVisible = false;
                    strongThis->ResetNoise();
                    strongThis->m_noiseChangedListeners(strongThis);
                }
            });
        }
    }
    else
    {
        // We are transitioning to hidden state.
        m_wasWindowHidden = true;

        // Close the LIS when app hides, since it can crash on resume (again due to Bug 11159685).
        ResetNoise();
    }
}

// Evaluating IsFullScreenOrTabletMode() is expensive so do it here in MaterialHelper instead of AcrylicBrush
// so we can do it once per thread instead of once per brush instance. Otherwise, apps which have ~50 AcrylicBrushes 
// experience severe rendering lag in resize scenarios (Bug 13289165).
void MaterialHelper::OnSizeChanged(const winrt::IInspectable& /*sender*/, const winrt::IInspectable& /*args*/)
{
    const bool isFullScreenOrTabletMode = IsFullScreenOrTabletMode();

    const auto strongThis = get_strong();
    m_sizeChangedListeners(strongThis, isFullScreenOrTabletMode);
}

void MaterialHelper::UpdatePolicyStatus(bool onUIThread)
{
    auto strongThis = get_strong();
    auto callback = [strongThis, this]() {
        const bool isEnergySaverMode = m_energySaverStatusChangedRevokerValid ? winrt::PowerManager::EnergySaverStatus() == winrt::EnergySaverStatus::On : true;
        const bool areEffectsFast = m_compositionCapabilities ? (m_compositionCapabilities.AreEffectsFast() || m_ignoreAreEffectsFast) : false;
        const bool advancedEffectsEnabled = m_uiSettings ? m_uiSettings.AdvancedEffectsEnabled() : true;

        const bool isDisabledByPolicy = m_simulateDisabledByPolicy || (isEnergySaverMode || !areEffectsFast || !advancedEffectsEnabled);

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
        if (m_dispatcher) // We might have no dispatcher in XamlPresenter scenarios, in this case we will always be in the disabled state.
        {
            if (m_dispatcher.HasThreadAccess())
            {
                callback();
            }
            else
            {
                auto ignore = m_dispatcher.RunAsync(winrt::CoreDispatcherPriority::Normal, callback);
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
        catch (winrt::hresult_error)
        {
            // Calling GetForCurrentView on threads without a CoreWindow throws an error. This comes up in places like LogonUI.
            // Assume 1.0 scaling and don't touch the noise brush.
            // Assuming 1.0 scaling isn't correct. Xaml has internal code that handles XamlPresenter scenarios, but that isn't available through public APIs. We can fix this for WUXC but not MUX.
        }

        m_noiseBrush = CreateScaledBrush(resScaleInt);

        if (!SharedHelpers::IsRS3OrHigher())
        {
            m_noiseSurface = (m_noiseBrush.Surface()).try_as<winrt::LoadedImageSurface>();
        }
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
bool MaterialHelper::IsFullScreenOrTabletMode()
{
    try
    {
        auto instance = LifetimeHandler::GetMaterialHelperInstance();

        // ApplicationView::GetForCurrentView() is an expensive call - make sure to cache the ApplicationView
        if (!instance->m_applicationView)
        {
            instance->m_applicationView = winrt::ViewManagement::ApplicationView::GetForCurrentView();
        }

        // UIViewSettings::GetForCurrentView() is an expensive call - make sure to cache the UIViewSettings
        if (!instance->m_uiViewSettings)
        {
            instance->m_uiViewSettings = winrt::ViewManagement::UIViewSettings::GetForCurrentView();
        }

        const bool isFullScreenMode = instance->m_applicationView.IsFullScreenMode();
        const bool isTabletMode = instance->m_uiViewSettings.UserInteractionMode() == winrt::ViewManagement::UserInteractionMode::Touch;

        return isFullScreenMode || isTabletMode;
    }
    catch (winrt::hresult_error)
    {
        // Calling GetForCurrentView on threads without a CoreWindow throws an error. This can happen in XamlIsland scenarios.
        // In those cases assume that we are not in full screen or tablet mode for now.
        // Task 19285526: In Islands, IsFullScreenOrTabletMode() can use ApplicationView or UIViewSettings.
        return false;
    }
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
#endif
