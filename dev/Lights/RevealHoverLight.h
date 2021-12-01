// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "MaterialHelper.h"
#include "SpotLightStateHelper.h"
#include "RevealTestApi.h"

#include "RevealHoverLight.g.h"

class PointerHoverElementHelper;
class SoftLightSwitch;

class RevealHoverLight :
    public ReferenceTracker<RevealHoverLight, winrt::implementation::RevealHoverLightT>
{
    friend RevealTestApi;
    friend MaterialHelperBase;
    friend MaterialHelper;
public:
    static winrt::hstring& GetLightIdStatic();

    // IXamlLightOverrides
    winrt::hstring GetId();
    void OnConnected(winrt::UIElement const& newElement);
    void OnDisconnected(winrt::UIElement const& oldElement);

#if BUILD_WINDOWS
    void OnAdditionalMaterialPolicyChanged(const com_ptr<MaterialHelperBase>& sender);
#else
    void OnMaterialPolicyStatusChanged(const com_ptr<MaterialHelperBase>& sender, bool isDisabledByMaterialPolicy);
#endif

    void GoToState(_In_ winrt::RevealBrushState newState);

    void SetIsPressLight(bool isPressLight) { m_isPressLight = isPressLight; }
    bool GetIsPressLight() { return m_isPressLight; }
    winrt::SpotLight GetLight() { return m_compositionSpotLight; } // For test APIs

public:
    // These can be adjusted for tweaking/development on DBG builds
#if DBG
    static float s_lightMinSize;
    static float s_lightMaxSize;
    static float s_sizeAdjustment;
    static float s_pressOuterSize;
    static float s_innerConeAngleInDegrees;
    static float s_outerConeAngleInDegrees;
    static float s_spotlightHeight;
    static float s_constantAttenuation;
    static float s_linearAttenuation;

#else
    static const float s_constantAttenuation;
    static const float s_linearAttenuation;
#endif

    static std::array<RevealHoverSpotlightStateDesc, RevealHoverSpotlightState_StateCount> s_revealHoverSpotlightStates;
    static std::array<RevealHoverSpotlightStateDesc, RevealHoverSpotlightState_StateCount> s_pressSpotLightStates;

    // For AggregableComObject
protected:
    void InitializeImpl(_In_opt_ IInspectable* outer);

private:
    enum class LightStates { Off, AnimToHover, AnimToOff, Hover, Pressing, FastRelease, SlowRelease };
    enum class LightEvents { GotoNormal, GotoPointerOver, GotoPressed, AnimationComplete };
    LightStates m_currentLightState{ LightStates::Off };

    void GotoLightState(LightEvents e);
    void GotoLightStateHelper(LightStates target, bool skipAnimation = false);

    void EnableHoverAnimation();
    void DisableHoverAnimation();

    winrt::ApplicationTheme GetCurrentTheme(const winrt::UIElement& target);
    void CancelCurrentPressStateContinuation();

    void OnPointerPressed(const winrt::IInspectable& sender, const winrt::PointerRoutedEventArgs& args);
    void SwitchLight(bool turnOn);

    void EnsureCompositionResources();
    void ReleaseCompositionResources();

    winrt::weak_ref<winrt::UIElement> m_targetElement{ nullptr };
    winrt::ExpressionAnimation m_offsetAnimation{ nullptr };
    winrt::ExpressionAnimation m_outerAngleAnimation{ nullptr };
    bool m_isHoverAnimationActive{ false };
    winrt::CompositionPropertySet m_pointer{ nullptr };
    winrt::CompositionPropertySet m_offsetProps{ nullptr };
    winrt::SpotLight m_compositionSpotLight{ nullptr };
    winrt::CompositionPropertySet m_colorsProxy{ nullptr };
    bool m_isDisabledByMaterialPolicy{};

#if BUILD_WINDOWS
    winrt::DispatcherQueue m_dispatcherQueue{ nullptr };
    winrt::MaterialProperties m_materialProperties{ nullptr };
    winrt::MaterialProperties::TransparencyPolicyChanged_revoker m_transparencyPolicyChangedRevoker{};
    winrt::event_token m_additionalMaterialPolicyChangedToken{};
#else
    winrt::event_token m_materialPolicyChangedToken{};
#endif

    std::function<void()> m_cancelCurrentPressStateContinuation;
    bool m_isPressed{};
    bool m_isPointerOver{};
    bool m_shouldLightBeOn{};

    bool m_isPressLight{};

    // We need to be able to center the hover light when a reveal enabled object is programmatically invoked.
    // For mouse and touch we will set this to false and have our light follow the pointer position.
    bool m_centerLight{ true };

    const RevealHoverSpotlightStateDesc* m_spotLightStates{};
    winrt::IInspectable m_elementPointerPressedEventHandler{};
};
