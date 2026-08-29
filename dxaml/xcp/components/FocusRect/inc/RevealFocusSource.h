// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <stack_vector.h>
#include <fwd/windows.ui.composition.h>
#include <fwd/windows.graphics.h>

class CUIElement;
class CCoreServices;
class IRawData;
class xstring_ptr_view;

namespace DirectUI {
    enum class FocusNavigationDirection : uint8_t;
}

namespace FocusRect {

    enum class RevealFocusLight
    {
        DefaultAmbient = 0,
        PulsingAmbient,
        TravelingSpotlight,
        PressAmbient,

        Count // This should be last
    };

    class RevealFocusSource final
    {

    public:
        Microsoft::WRL::ComPtr<WUComp::ISpriteVisual> BuildRevealFocusVisual(
            _In_opt_ WUComp::IVisual* existingVisual,
            _In_ CUIElement* target,
            _In_ uint32_t color,
            _In_ const XTHICKNESS& primaryThickness,
            _In_ bool transparentBorder);

        // Returns the element in the tree that has focus
        xref_ptr<CUIElement> GetTarget() const;

        // Returns the size adjustment required for the reveal focus visual
        wfn::Vector2 GetSizeAdjustment() const;

        // Returns the offset adjustment required for the reveal focus visual
        wfn::Vector3 GetOffsetAdjustment() const;

        // Returns true if animations are enabled system wide. If animations are disabled, then the lights shouldn't be animated.
        bool IsAnimationEnabled() const;

        // Returns whether or not the reveal effect is using lights. Currently, this is only configurable via a reg key when we have velocity enabled.
        bool HasLight() const;

        // Returns a static thickness that is used to alter which element in the tree hosts the border so that we properly render the reveal glow.
        static XTHICKNESS GetRevealThickness();

        // Returns the duration of the spotlight animation. This is determined by the size of the element and the direction focus is moving.
        wf::TimeSpan GetSpotLightDuration(_In_ DirectUI::FocusNavigationDirection direction) const;

        // Returns the size of the spotlight used.
        float GetSpotLightSize() const;

        // Returns true if traveling focus is enabled. If the element is too small for the direction focus is moving, we won't use the traveling focus.
        bool IsTravelingFocusEnabled(_In_ DirectUI::FocusNavigationDirection direction) const;

        // Returns the size of the insets to be applied to the nine grid brush based on the FocusVisualPrimaryThickness
        XTHICKNESS GetInsetsFromThickness(_In_ const XTHICKNESS& primaryThickness) const;

#pragma warning(suppress: 6506) // non-pointer when expanding to class actual parameter.
        template <typename InterfaceType>
        bool TryGetLight(RevealFocusLight lightType, _Out_ Microsoft::WRL::Details::ComPtrRef<Microsoft::WRL::ComPtr<InterfaceType>> light) const
        {
            if (m_lights.m_vector.size() > static_cast<size_t>(lightType))
            {
                return SUCCEEDED(m_lights.m_vector[static_cast<size_t>(lightType)].As(light));
            }

            return false;
        }

        enum class ResetReason
        {
            DeviceLost,
            DCompCleanup,
            FocusReset
        };

        // Resets the reveal focus source based on the reset reason.
        void Reset(_In_ ResetReason reason);

    private:

        using RevealLightCollection = Jupiter::stack_vector<Microsoft::WRL::ComPtr<WUComp::ICompositionLight>, static_cast<size_t>(RevealFocusLight::Count)>;

        Microsoft::WRL::ComPtr<WUComp::ICompositionBrush> m_compBrush;
        RevealLightCollection m_lights;
        xref::weakref_ptr<CUIElement> m_target;
        uint32_t m_rgb = 0;
        XTHICKNESS m_adjustments{};

    private:
        template <typename LightType>
        void AddLight(_In_ LightType* light, _In_ RevealFocusLight index)
        {
            Microsoft::WRL::ComPtr<WUComp::ICompositionLight> compLight;
            IFCFAILFAST(light->QueryInterface(IID_PPV_ARGS(&compLight)));
            m_lights.m_vector.emplace(m_lights.m_vector.begin() + static_cast<size_t>(index), std::move(compLight));
        }

        float GetTravelingDistance(_In_ DirectUI::FocusNavigationDirection direction) const;

        static bool UseLightingEffect();
        void InitializeLightingEffects(
            _In_ WUComp::ICompositor* compositor,
            _In_ WUComp::IVisual* targetVisual);

        static void SetupAmbientLight(
            _In_ WUComp::IAmbientLight* ambientLight,
            _In_ wu::Color color,
            _In_ WUComp::IVisual* targetVisual,
            _In_ float intensity);

        static void SetupSpotLight(
            _In_ WUComp::ISpotLight* pointLight,
            _In_ wu::Color color,
            _In_ WUComp::IVisual* targetVisual);

        static Microsoft::WRL::ComPtr<WUComp::ICompositionEffectBrush> CreateRevealFocusEffectBrush(
            _In_ WUComp::ICompositor* compositor,
            _In_ CCoreServices* services,
            _In_ const uint32_t rgb,
            _In_ const bool colorChanged,
            _In_ const XTHICKNESS& insets,
            _In_ const bool sizeChanged,
            _In_opt_ WUComp::ICompositionEffectBrush* existingEffectBrush);

        // Creates the color effect with the source parameter passed in.
        static Microsoft::WRL::ComPtr<wgr::Effects::IGraphicsEffect> CreateColorEffect(
            _In_ uint32_t rgb,
            _In_ const xstring_ptr_view& effectSourceParameter);

        // Creates an effect brush with the supplied graphics effect.
        static Microsoft::WRL::ComPtr<WUComp::ICompositionEffectBrush> CreateEffectBrush(
            _In_ WUComp::ICompositor* compositor,
            _In_ wgr::Effects::IGraphicsEffect* effect);

        // Creates the composition surface used to render the reveal effect.
        static Microsoft::WRL::ComPtr<WUComp::ICompositionSurfaceBrush> CreateRevealSurfaceBrush(
            _In_ WUComp::ICompositor* compositor,
            _In_ CCoreServices* services,
            _In_ const XTHICKNESS& primaryThickness);

        // Creates the brush that is to be used as the source for the effect brush. Takes an existing brush that if
        // isn't null, will be used for the source. The CompositionSurfaceBrush passed in has the reveal effect applied to it.
        // The FocusVisualPrimaryThickness determines the size of the reveal glow, so that is required as well.
        static Microsoft::WRL::ComPtr<WUComp::ICompositionBrush> CreateEffectSourceBrush(
            _In_ WUComp::ICompositor* compositor,
            _In_opt_ WUComp::ICompositionBrush* existingEffectSourceBrush,
            _In_ WUComp::ICompositionSurfaceBrush* surfaceBrush,
            _In_ const XTHICKNESS& primaryThickness,
            _In_ const float scale);

    };
}