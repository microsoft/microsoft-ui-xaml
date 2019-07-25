// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "MaterialHelper.h"
#include "RevealBrushTestApi.h"

#include "RevealBrush.g.h"
#include "RevealBorderBrush.g.h"
#include "RevealBackgroundBrush.g.h"
#include "RevealBrush.properties.h"


class RevealBrush :
    public winrt::implementation::RevealBrushT<RevealBrush>,
    public RevealBrushProperties
{
    friend RevealBrushTestApi;
    friend MaterialHelper;
public:
    RevealBrush();
    virtual ~RevealBrush();

    // IXamlCompositionBrushOverrides
    void OnConnected();
    void OnDisconnected();

    // Property changed handler
    void OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);

    void OnMaterialPolicyStatusChanged(const com_ptr<MaterialHelperBase>& sender, bool isDisabledByMaterialPolicy);
    void OnNoiseChanged(const com_ptr<MaterialHelperBase>& sender);

    // On RS2, attaching lights to the tree is a multi-step process that happens over multiple frames. If lights are removed from the
    // tree before they're fully attached, then we should abort the rest of the process.
    static void StopAttachingLights();

    static void AttachLights();
    static void AttachLightsToAncestor(const winrt::UIElement& root, bool trackRootToDisconnectFrom);

    static void ClearProperties();
    static void EnsureProperties();

    // Properties' statics.
    static GlobalDependencyProperty s_IsContainerProperty;

public:
    // Default value on public DP - ok to be public
    static const winrt::Color sc_defaultColor;

protected:
    // DependencyProperty changed event handlers
    void OnColorChanged();
    void OnTargetThemeChanged();

    void CreateRevealBrush();
    void UpdateRevealBrush();

    // Effect factories:
    static winrt::IGraphicsEffect CreateRevealHoverEffect();
    static winrt::IGraphicsEffect CreateRevealBorderEffect(bool isInverted, bool hasBaseColor);

    void OnFallbackColorChanged(const winrt::DependencyObject& sender, const winrt::DependencyProperty& args);

    void UpdateLightTargets(bool ambientToo);

    bool IsInFallbackMode();

    winrt::event_token m_materialPolicyChangedToken{};

    bool m_isConnected{};
    bool m_isBorder{};
    bool m_isBorderLightSet{};
    bool m_isHoverLightSet{};
    bool m_isAmbientLightSet{};
    bool m_isInFallbackMode{};
    bool m_noiseChanged{};
    bool m_hasBaseColor{};
    bool m_isDisabledByMaterialPolicy{};

    winrt::CompositionBrush m_brush{ nullptr };
    winrt::event_token m_fallbackColorChangedToken{};
    winrt::event_token m_noiseChangedToken{};

    static const winrt::Color sc_ambientContributionColor;
    static const float sc_diffuseAmount;
    static const float sc_specularAmount;
    static const float sc_specularShine;
    static const float sc_diffuseAmountBorder;
    static const float sc_specularAmountBorder;
    static const float sc_specularShineBorder;
    static const winrt::Matrix5x4 sc_colorToAlphaMatrix;    // Converts the RGB in the noise texture into an alpha mask
    static const winrt::Matrix5x4 sc_luminanceToAlphaMatrix;


private:
    // Property changed event handler.
    static void OnIsContainerPropertyChanged(
        const winrt::DependencyObject& sender,
        const winrt::DependencyPropertyChangedEventArgs& args);

public:
    static void OnStatePropertyChanged(
        const winrt::DependencyObject& sender,
        const winrt::DependencyPropertyChangedEventArgs& args);

private:
    void EnsureNoiseBrush();
    winrt::CompositionEffectFactory GetOrCreateRevealBrushCompositionEffectFactory(
        bool isBorder,
        bool isInverted,
        bool hasBaseColor,
        const winrt::Compositor& compositor);

    static bool IsOnXboxAndNotMouseMode();
    void RemoveTargetBrush(const wstring_view& lightID);

    void PolicyStatusChangedHelper(bool isDisabledByMaterialPolicy);

    static bool ValidatePublicRootAncestor();
    static winrt::UIElement GetAncestor(const winrt::UIElement & root);
    static void AttachLightsToElement(const winrt::UIElement & element, bool trackAsRootToDisconnectFrom);
    static void AttachLightsImpl();

    winrt::CompositionSurfaceBrush m_noiseBrush{ nullptr };
};

class RevealBorderBrush : 
    public ReferenceTracker<RevealBorderBrush, winrt::implementation::RevealBorderBrushT, RevealBrush>
{
public:
    RevealBorderBrush();
};

class RevealBackgroundBrush :
    public ReferenceTracker<RevealBackgroundBrush, winrt::implementation::RevealBackgroundBrushT, RevealBrush>
{
public:
    RevealBackgroundBrush();
};
