﻿// Copyright (c) Microsoft Corporation. All rights reserved.
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
    public ReferenceTracker<AcrylicBrush, winrt::implementation::AcrylicBrushT>,
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
    void OnMaterialPolicyStatusChanged(const com_ptr<MaterialHelperBase>& sender, bool isDisabledByMaterialPolicy);

public:
    // Default value on public DP - ok to be public
    static constexpr winrt::Color sc_defaultTintColor{ 204, 255, 255, 255 };
    static constexpr double sc_defaultTintOpacity{ 1.0 };
    static constexpr winrt::TimeSpan sc_defaultTintTransitionDuration{ 500ms };

    static winrt::CompositionEffectBrush CreateAcrylicBrushWorker(
        const winrt::Compositor& compositor,
        bool useCrossFadeEffect,
        winrt::Color tintColor,
        winrt::Color luminosityColor,
        winrt::Color fallbackColor,
        bool shouldBrushBeOpaque,
        bool useCache = false);

    void CoerceToZeroOneRange(double& value);
    void CoerceToZeroOneRange_Nullable(winrt::IReference<double>& value);

private:
    static constexpr auto TintColorColor{ L"TintColor.Color"sv };
    static constexpr auto LuminosityColorColor{ L"LuminosityColor.Color"sv };
    static constexpr auto FallbackColorColor{ L"FallbackColor.Color"sv };
    static constexpr auto OpaqueFallbackColorColor{ L"OpaqueFallbackColor.Color"sv };

    static constexpr float sc_blurRadius = 30.0f;
    static constexpr float sc_noiseOpacity = 0.02f;
    static constexpr winrt::Color sc_exclusionColor{ 26, 255, 255, 255 };
    static constexpr float sc_saturation = 1.25f;

    void CreateAcrylicBrush(bool useCrossFadeEffect, bool forceCreateAcrylicBrush = false);

    static winrt::IGraphicsEffect CombineNoiseWithTintEffect_Luminosity(
        const winrt::IGraphicsEffectSource& blurredSource,
        const winrt::Microsoft::UI::Private::Composition::Effects::ColorSourceEffect& tintColorEffect,
        const winrt::Color initialLuminosityColor,
        std::vector<winrt::hstring>& animatedProperties);

    static winrt::CompositionEffectFactory CreateAcrylicBrushCompositionEffectFactory(const winrt::Compositor& compositor,
        bool shouldBrushBeOpaque,
        bool useCrossFadeEffect,
        winrt::Color initialTintColor,
        winrt::Color initialLuminosityColor,
        winrt::Color initialFallbackColor);

    static winrt::CompositionEffectFactory GetOrCreateAcrylicBrushCompositionEffectFactory(
        const winrt::Compositor& compositor,
        bool shouldBrushBeOpaque,
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

    void PolicyStatusChangedHelper(bool isDisabledByMaterialPolicy);

    bool m_isConnected{};
    bool m_isUsingAcrylicBrush{};
    bool m_noiseChanged{};
    bool m_isUsingOpaqueBrush{};
    bool m_isWaitingForFallbackAnimationComplete{};
    bool m_isDisabledByMaterialPolicy{};

    winrt::event_token m_fallbackColorChangedToken{};
    winrt::event_token m_waitingForFallbackAnimationCompleteToken{};
    winrt::CompositionScopedBatch m_waitingForFallbackAnimationCompleteBatch{ nullptr };
    winrt::CompositionBrush m_brush{ nullptr };
    winrt::event_token m_materialPolicyChangedToken{};
    winrt::event_token m_noiseChangedToken{};
    winrt::CompositionSurfaceBrush m_noiseBrush{ nullptr };
};
