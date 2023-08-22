// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "AcrylicTestApi.h"

//
// The MaterialHelper class embodies the common policy that all the RS2 materials need to pay attention to.
// In particular if any of the following are on, lighting and effects should be disabled:
// * PowerManager.EnergySaverStatus == On
// * !CompositionCapabilities.AreEffectsFast
// * !UISettings.AdvancedEffectsEnabled
//
// This helper listens for all of those changes and calls back to the owner via OnMaterialPolicyStatusChanged().
// The owner can call IsDisabledByMaterialPolicy() to see whether the material should be off according to this policy.
//
// NOTE: To make AreEffectsFast return true in a VM, set HKLM\Software\Microsoft\Windows\Dwm, value ForceEffectMode = dword:2
//
//
// In RS5, as part of work to support Materials in XamlIslands, the responsibility of managing and broadcasting fallback policy
// was moved to Windows.UI.MaterialProperties (owned by Shell). Below is a comparison of legacy vs new MaterialHelper role:
//
// Legacy (MUX, pre-RS5):
// (1) Notify brushes and lights of changes to fallback policy
// (2) Provide single noise texture to brushes.
// (3) Serve as host of assorted statics/helpers used by Fluent brushes & lights (example: hold AcrylicEffectFactory cache)
// (4) Support RS2 workarounds where noise LIS becomes invalid and needs to be recreated
//
// New (WUXC, RS5+):
// (1) Provide DPI-Scaled noise texture to brushes (two islands are on same thread, but can be on different monitor & DPI)
// (2) Serve as host of assorted statics/helpers used by Fluent brushes & lights
//
// To support this, the class was factored into MaterialHelperBase (shared functionality), and conditionally compiled
// derived MaterialHelper (New vs legacy functionality).

class MaterialHelperBase :
    public winrt::implements<MaterialHelperBase, winrt::IInspectable>
{
public:
    static void SimulateDisabledByPolicy(bool value);
    static bool SimulateDisabledByPolicy();

    static void IgnoreAreEffectsFast(bool value);
    static bool IgnoreAreEffectsFast();

    static void OnRevealBrushConnected();
    static void OnRevealBrushDisconnected();

    static void TrackRevealLightsToRemove(const winrt::IVector<winrt::XamlLight>& lights, const std::vector<winrt::XamlLight>& revealLightsToRemove);

    static winrt::CompositionEffectFactory GetOrCreateAcrylicBrushCompositionEffectFactoryFromCache(
        const winrt::Compositor& compositor,
        bool shouldBrushBeOpaque,
        bool useWindowAcrylic,
        bool useCrossFadeEffect,
        bool useCache,
        std::function<winrt::CompositionEffectFactory()> cacheMissingCallback);

    static winrt::CompositionEffectFactory GetOrCreateRevealBrushCompositionEffectFactoryFromCache(
        bool isBorder,
        bool isInverted,
        bool hasBaseColor,
        std::function<winrt::CompositionEffectFactory()> cacheMissingCallback);

    template <typename T> static void LightPolicyChangedHelper(T* instance, bool isDisabledByMaterialPolicy);

    // Number of connected RevealBrushes in the tree (i.e. # of brushes that need lights)
    int m_revealBrushConnectedCount{};
    std::vector<std::pair<winrt::IVector<winrt::XamlLight>, std::vector<winrt::XamlLight>>> m_revealLightsToRemove;


    enum AcrylicBrushCacheHelperParam
    {
        ShouldBrushBeOpaque = 1,
        UseWindowAcrylic = 2,
        UseCrossFadeEffect = 4,
        // If you add more value in, please update MaxCacheSize too
        MaxCacheSize = 8
    };

    enum class RevealBrushCacheFlags
    {
        IsBorder = 1,
        IsInverted = 2,
        HasBaseColor = 4,
        MaxCacheSize = 8,
    };

    // Acrylic Brush
    static int constexpr BuildAcrylicBrushCompositionEffectFactoryKey(bool shouldBrushBeOpaque, bool useWindowAcrylic, bool useCrossFadeEffect);
    void AssertUniqueCompositorOrUpdate(const winrt::Compositor& compositor);

    // cache storage for AcrylicBrushEffectory
    std::array<winrt::ICompositionEffectFactory, AcrylicBrushCacheHelperParam::MaxCacheSize>
        m_acrylicBrushCompositionEffectFactoryCache;

    // This is only a defensive assert to check that Compositor should be the same in the same thread
    // m_acrylicCompositor is used keep a copy of it and then assert in each following query.
    // If Compositor is not the same, current implementation would return wrong EffectFactory
    winrt::Compositor m_acrylicCompositor{ nullptr };

    // Reveal
    std::array<winrt::ICompositionEffectFactory, (size_t)RevealBrushCacheFlags::MaxCacheSize>
        m_revealBrushCompositionEffectFactoryCache;

    winrt::CompositionSurfaceBrush CreateScaledBrush(int dpiScale);

protected:
    bool m_simulateDisabledByPolicy{};   // Test use only: Simulate that material is disabled by policy - for test use only
    bool m_ignoreAreEffectsFast{};       // Test use only: Ignore CompositionCapabilities.AreEffectFasts so tests can get Neon on VMs

};

#if BUILD_WINDOWS
// ************************************ WUXC version of MaterialHelper *****************************************

struct IslandBorderLightInfo
{
    int m_revealBrushConnectedCount;
    std::pair<winrt::IVector<winrt::XamlLight>, std::vector<winrt::XamlLight>> m_revealLightsToRemove;
};

class MaterialHelper : public winrt::implements<MaterialHelper, MaterialHelperBase>
{
public:
    template <typename T>
    class LightTemplates
    {
    public:
        static void MaterialHelper::LightTemplates<T>::OnLightTransparencyPolicyChanged(
            const winrt::weak_ref<T> weakInstance,
            const winrt::IMaterialProperties& materialProperties,
            const winrt::DispatcherQueue& dispatcherQueue,
            bool onUIThread);
    };

    template <typename T>
    class BrushTemplates
    {
    public:
        static void HookupWindowDpiChangedHandler(T* instance);
        static void UnhookWindowDpiChangedHandler(T* instance);
        static void HookupIslandDpiChangedHandler(T* instance);
        static void UnhookIslandDpiChangedHandler(T* instance);
        static void OnIslandTransformChanged(T* instance);
        static void UpdateDpiScaledNoiseBrush(T* instance);
        static int GetEffectiveDpi(T* instance);
        static bool IsDisabledByInAppTransparencyPolicy(T* instance);
        static bool IsDisabledByHostBackdropTransparencyPolicy(T* instance);
    };

    virtual ~MaterialHelper() {};

    static winrt::CompositionSurfaceBrush GetNoiseBrush(int dpiScale);

    static void RevealBorderLightUnavailable(bool value);
    static bool RevealBorderLightUnavailable();

    static void OnRevealBrushConnectedIsland(const winrt::XamlIsland& island);
    static void OnRevealBrushDisconnectedIsland(const winrt::XamlIsland& island);
    static void TrackRevealLightsToRemoveIsland(const winrt::XamlIsland& island, const winrt::IVector<winrt::XamlLight>& lights, const std::vector<winrt::XamlLight>& revealLightsToRemove);

    // Notifies listeners of policy changes due to factors outside of materialProperties such, including test overrides or failure to create lights.
    static winrt::event_token AdditionalPolicyChanged(const std::function<void(const com_ptr<MaterialHelperBase>&)>& handler);
    static void  AdditionalPolicyChanged(winrt::event_token removeToken);
    void NotifyAdditionalPolicyChangedListeners();

protected:
    bool m_revealBorderLightUnavailable;

private:
    winrt::CompositionSurfaceBrush GetNoiseBrushImpl(int dpiScale);

private:
    std::unordered_map<int, winrt::CompositionSurfaceBrush> m_dpiScaledNoiseBrushes;
    // Number of connected RevealBrushes in the tree (i.e. # of brushes that need lights) and associated lights foreach XamlIsland.
    std::map<winrt::XamlIsland, IslandBorderLightInfo> m_islandBorderLights;

    event<std::function<void(com_ptr<MaterialHelperBase>)>> m_additionalPolicyChangedListeners;
};

#else
// **************************************** MUX version of MaterialHelper *****************************************
class MaterialHelper : public winrt::implements<MaterialHelper, MaterialHelperBase>
{
public:
    MaterialHelper();
    virtual ~MaterialHelper();

    // Notifies listeners of all policy changes (including those due to test overrides).
    static winrt::event_token PolicyChanged(const std::function<void(const com_ptr<MaterialHelperBase>&, bool)>& handler);
    static void PolicyChanged(winrt::event_token removeToken);

    static winrt::event_token WindowSizeChanged(const std::function<void(const com_ptr<MaterialHelperBase>&, bool)>& handler);
    static void  WindowSizeChanged(winrt::event_token removeToken);

    static winrt::event_token NoiseChanged(const std::function<void(const com_ptr<MaterialHelperBase>&)>& handler);
    static void  NoiseChanged(winrt::event_token removeToken);

    static winrt::CompositionSurfaceBrush GetNoiseBrush();
    static bool RS2IsSafeToCreateNoise();
    static bool IsFullScreenOrTabletMode();

    void UpdatePolicyStatus(bool onUIThread = false);

    static void SetShouldBeginAttachingLights(bool shouldBeginAttachingLights);
    static bool ShouldBeginAttachingLights();
    static void SetShouldContinueAttachingLights(bool shouldContinueAttachingLights);
    static bool ShouldContinueAttachingLights();

    // Currently used only to work around Bug 18005612 on MUX + RS4 or earlier
    static bool IncrementAndCheckFailedToAttachLightsCount();
    static void ResetFailedToAttachLightsCount();

private:
    bool IsFullScreenOrTabletModeImpl();
    void EnsureCompositionCapabilities();
    void EnsureSizeChangedHandler();

    void OnEnergySaverStatusChanged(const winrt::IInspectable& /*sender*/, const winrt::IInspectable& /*args*/);
    void OnCompositionCapabilitiesChanged(const winrt::CompositionCapabilities& /*sender*/, const winrt::IInspectable& /*args*/);
    void OnUISettingsChanged(const winrt::UISettings& /*sender*/, const winrt::IInspectable& /*args*/);
    void OnDpiChanged(const winrt::IInspectable& sender, const winrt::IInspectable& args);
    void OnSizeChanged(const winrt::IInspectable& sender, const winrt::IInspectable& args);
    void OnVisibilityChanged(const winrt::CoreWindow&, const winrt::VisibilityChangedEventArgs& args);

    void HookupDpiChangedHandler();
    void HookupVisibilityChangedHandler();

    winrt::CompositionSurfaceBrush GetNoiseBrushImpl();
    void ResetNoise();

    bool FailedToAttachLights();

    bool m_isDisabledByMaterialPolicy{};
    float m_logicalDpi{};

    winrt::PowerManager::EnergySaverStatusChanged_revoker m_energySaverStatusChangedRevoker{};
    winrt::event_token m_compositionCapabilitiesChangedToken{};
    winrt::event_token m_advancedEffectsEnabledChangedToken{};
    winrt::DisplayInformation::DpiChanged_revoker m_dpiChangedRevoker{};
    winrt::CoreWindow::VisibilityChanged_revoker m_visibilityChangedRevoker;
    winrt::event_token m_windowSizeChangedToken{};
    winrt::CompositionCapabilities m_compositionCapabilities{ nullptr };
    winrt::IUISettings4 m_uiSettings{ nullptr };
    winrt::CoreDispatcher m_dispatcher{ nullptr };
    winrt::CompositionSurfaceBrush m_noiseBrush{ nullptr };
    winrt::LoadedImageSurface m_noiseSurface{ nullptr };

    // Cache these objects for the view as they are expensive to query via GetForCurrentView() calls.
    winrt::ViewManagement::ApplicationView m_applicationView{ nullptr };
    winrt::ViewManagement::UIViewSettings m_uiViewSettings{ nullptr };

    bool m_wasWindowHidden{};                           // True if we've received CoreWindow.VisiblityChanged w/ Visibility == false
    bool m_waitingForRenderingAfterBecomingVisible{};   // True if we got a VisibilityChanged(True) and are waiting for a CT.Rendering to complete the RS2 workaround for Bug 11159685
    bool m_energySaverStatusChangedRevokerValid{};
    bool m_isFullScreenModeValid{};
    bool m_isFullScreenMode{};

    event<std::function<void(com_ptr<MaterialHelperBase>, bool)>> m_policyChangedListeners;
    event<std::function<void(com_ptr<MaterialHelperBase>, bool)>> m_sizeChangedListeners;
    event<std::function<void(com_ptr<MaterialHelperBase>)>> m_noiseChangedListeners;

    winrt::Window m_currentWindow{ nullptr };

    // On RS4 and lower, Bug 18005612 is not fixed and we may need to re-try light attachment on CT.Rendering event. 
    // This flag ensures that we only attempt to attach the lights once per tick (in case 1st reveal brush re-enters tree on same tick).
    bool m_shouldBeginAttachingLights{};

    // On RS2 lights are attached in multiple steps. First they're attached to the RootScrollViewer, then on the next
    // CompositionTarget.Rendering event we open a popup, then on that popup's Opened event we attach lights to the popup
    // root. If at any point during this process, all reveal brushes leave the tree, then we should abort the process and
    // stop attaching lights, because we're going to remove all lights from the tree. Otherwise we'll leave the tree in a
    // partially-lit state because we added lights to the popup root after all lights were removed from the RootScrollViewer.
    // After that point, brushes under the RSV will not be lit, and we won't add lights to the RSV because we think lights
    // already exist in the tree.
    bool m_shouldContinueAttachingLights{};

    // For Bug 18005612 workaround, we may ave to defer light attachment by a few ticks.
    // If we retry many more times but are not successful, something else is wrong - give up and log an assert in that case.
    static const unsigned int sc_maxFailedToAttachLightsCount = 100;
    unsigned int m_failedToAttachLightsCount{ 0 };
};
#endif
