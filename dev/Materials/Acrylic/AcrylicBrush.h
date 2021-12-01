// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "MaterialHelper.h"
#include "AcrylicTestApi.h"

#include "AcrylicBrush.g.h"
#include "AcrylicBrush.properties.h"

#include <limits>

#pragma warning(push)
#pragma warning(disable: 6101)  // Returning uninitialized memory '<value>'.  A successful path through the function does not set the named _Out_ parameter.
#include "Microsoft.UI.Private.Composition.Effects_impl.h"
#pragma warning(pop)

winrt::Color GetLuminosityColor(winrt::Color tintColor, winrt::IReference<double> luminosityOpacity);
double GetTintOpacityModifier(winrt::Color tintColor);

class AcrylicBrush :
#if BUILD_WINDOWS
    public ReferenceTracker<AcrylicBrush, winrt::implementation::AcrylicBrushT, winrt::IXamlCompositionBrushBaseOverridesPrivate>,
#else
    public ReferenceTracker<AcrylicBrush, winrt::implementation::AcrylicBrushT>,
#endif
    public AcrylicBrushProperties
{
    friend AcrylicTestApi;
    friend MaterialHelper;

public:
    AcrylicBrush();
    virtual ~AcrylicBrush();

    // IXamlCompositionBrushOverrides
    void OnConnected();
    void OnDisconnected();
    void OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    void OnNoiseChanged(const com_ptr<MaterialHelperBase>& sender);

#if BUILD_WINDOWS
    void OnElementConnected(winrt::DependencyObject element) noexcept;
    void OnIslandTransformChanged(const winrt::CompositionIsland& sender, const winrt::IInspectable& /*args*/);
    void OnTransparencyPolicyChanged(const winrt::IMaterialProperties& sender, const winrt::IInspectable& /*args*/);
    void OnAdditionalMaterialPolicyChanged(const com_ptr<MaterialHelperBase>& sender);
#else
    void OnMaterialPolicyStatusChanged(const com_ptr<MaterialHelperBase>& sender, bool isDisabledByMaterialPolicy);
    void OnWindowSizeChanged(const com_ptr<MaterialHelperBase>& sender, bool isFullScreenOrTabletMode);
#endif


public:
    // Default value on public DP - ok to be public
    static constexpr winrt::Color sc_defaultTintColor{ 204, 255, 255, 255 };
    static constexpr double sc_defaultTintOpacity{ 1.0 };
    static constexpr winrt::TimeSpan sc_defaultTintTransitionDuration{ 500ms };

    static winrt::CompositionEffectBrush CreateAcrylicBrushWorker(
        const winrt::Compositor& compositor,
        bool useWindowAcrylic,
        bool useCrossFadeEffect,
        winrt::Color tintColor,
        winrt::Color luminosityColor,
        winrt::Color fallbackColor,
        bool shouldBrushBeOpaque,
        bool useCache = false);

    void CoerceToZeroOneRange(double& value);
    void CoerceToZeroOneRange_Nullable(winrt::IReference<double>& value);

protected: // AcrylicTestApi needs CreateAcrylicBrush be public or protected
    void CreateAcrylicBrush(bool useCrossFadeEffect, bool forceCreateAcrylicBrush = false);

private:
    static constexpr auto TintColorColor{ L"TintColor.Color"sv };
    static constexpr auto LuminosityColorColor{ L"LuminosityColor.Color"sv };
    static constexpr auto FallbackColorColor{ L"FallbackColor.Color"sv };

    static constexpr float sc_blurRadius = 30.0f;
    static constexpr float sc_noiseOpacity = 0.02f;
    static constexpr winrt::Color sc_exclusionColor{ 26, 255, 255, 255 };
    static constexpr float sc_saturation = 1.25f;

    static winrt::IGraphicsEffect CombineNoiseWithTintEffect_Legacy(
        const winrt::IGraphicsEffectSource& blurredSource,
        const winrt::Microsoft::UI::Private::Composition::Effects::ColorSourceEffect& tintColorEffect);

    static winrt::IGraphicsEffect CombineNoiseWithTintEffect_Luminosity(
        const winrt::IGraphicsEffectSource& blurredSource,
        const winrt::Microsoft::UI::Private::Composition::Effects::ColorSourceEffect& tintColorEffect,
        const winrt::Color initialLuminosityColor,
        std::vector<winrt::hstring>& animatedProperties);

    static winrt::CompositionEffectFactory CreateAcrylicBrushCompositionEffectFactory(const winrt::Compositor& compositor,
        bool shouldBrushBeOpaque,
        bool useWindowAcrylic,
        bool useCrossFadeEffect,
        winrt::Color initialTintColor,
        winrt::Color initialLuminosityColor,
        winrt::Color initialFallbackColor);

    static winrt::CompositionEffectFactory GetOrCreateAcrylicBrushCompositionEffectFactory(
        const winrt::Compositor& compositor,
        bool shouldBrushBeOpaque,
        bool useWindowAcrylic,
        bool useCrossFadeEffect,
        winrt::Color initialTintColor,
        winrt::Color initialLuminosityColor,
        winrt::Color initialFallbackColor,
        bool useCache);

    winrt::Color GetEffectiveTintColor();
    winrt::Color GetEffectiveLuminosityColor();

    void EnsureNoiseBrush();
    void UpdateAcrylicBrush();

    // Handle acrylic status changes
    void OnCurrentWindowActivated(const winrt::IInspectable& sender, const winrt::WindowActivatedEventArgs& args);
    void UpdateAcrylicStatus();

    void OnFallbackColorChanged(const winrt::DependencyObject& sender, const winrt::DependencyProperty& args);

    void CreateAnimation(const winrt::CompositionBrush& brush,
        winrt::CompositionScopedBatch& scopedBatch,
        winrt::event_token& token,
        float acrylicStart,
        float acrylicEnd,
        const winrt::TypedEventHandler<winrt::IInspectable, winrt::CompositionBatchCompletedEventArgs>& handler);

    void CancelFallbackAnimationCompleteWait();

    static winrt::CompositionAnimation MakeColorAnimation(const winrt::Color& color, const winrt::TimeSpan& duration, const winrt::Compositor& compositor);
    static winrt::CompositionAnimation MakeFloatAnimation(
        float fromValue,
        float toValue,
        const winrt::TimeSpan& duration,
        const winrt::CompositionEasingFunction& easing,
        const winrt::Compositor& compositor);
    static void PlayCrossFadeAnimation(const winrt::CompositionBrush& brush, float acrylicStart, float acrylicEnd);

    bool IsInFallbackMode() { return !m_isUsingAcrylicBrush; }

#if BUILD_WINDOWS
    void PolicyStatusChangedHelper(bool isDisabledByBackdropPolicy, bool isDisabledByHostBackdropPolicy);
#else
    void PolicyStatusChangedHelper(bool isDisabledByMaterialPolicy);
    bool IsWindowActive(const winrt::CoreWindow& coreWindow);
    void UpdateWindowActivationStatus();
#endif

    bool m_isConnected{};
    bool m_isUsingAcrylicBrush{};
    bool m_isUsingWindowAcrylic{};
    bool m_noiseChanged{};
    bool m_isUsingOpaqueBrush{};
    bool m_isWaitingForFallbackAnimationComplete{};

#if BUILD_WINDOWS
    bool m_isDisabledByBackdropPolicy{};
    bool m_isDisabledByHostBackdropPolicy{};
    bool m_isInterIsland{};
#else
    bool m_isActivated{ true };
    bool m_isFullScreenOrTabletMode{};
    bool m_isDisabledByMaterialPolicy{};
#endif

    winrt::event_token m_fallbackColorChangedToken{};
    winrt::event_token m_waitingForFallbackAnimationCompleteToken{};
    winrt::CompositionScopedBatch m_waitingForFallbackAnimationCompleteBatch{ nullptr };
    winrt::CompositionBrush m_brush{ nullptr };

#if BUILD_WINDOWS
    float m_logicalDpi{};
    winrt::DisplayInformation::DpiChanged_revoker m_dpiChangedRevoker{};
    winrt::MaterialProperties::TransparencyPolicyChanged_revoker m_transparencyPolicyChangedRevoker{};
    winrt::event_token m_islandTransformChangedToken{};
    winrt::CompositionSurfaceBrush m_dpiScaledNoiseBrush{ nullptr };
    winrt::DispatcherQueue m_dispatcherQueue{ nullptr };
    winrt::MaterialProperties m_materialProperties{ nullptr };
    winrt::XamlIsland m_associatedIsland{ nullptr };
    winrt::CompositionIsland m_associatedCompositionIsland{ nullptr };
    winrt::event_token m_additionalMaterialPolicyChangedToken{};
#else
    winrt::event_token m_windowActivatedToken{};
    winrt::event_token m_materialPolicyChangedToken{};
    winrt::event_token m_windowSizeChangedToken{};
    winrt::event_token m_noiseChangedToken{};
    tracker_ref<winrt::Window> m_currentWindow{ this };
    winrt::CompositionSurfaceBrush m_noiseBrush{ nullptr };
#endif
};
