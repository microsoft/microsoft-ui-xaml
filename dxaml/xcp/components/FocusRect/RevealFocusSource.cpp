// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "RevealFocusSource.h"
#include "corep.h"
#include "WindowRenderTarget.h"
#include "DCompTreeHost.h"
#include "DoPointerCast.h"
#include "LoadedImageSurface.h"
#include <Pathcch.h>
#include <wil\resource.h>
#include <windows.ui.composition.interop.h>
#include "DCompSurface.h"
#include <hw/effects.h>
#include "D2DUtils.h"
#include "focusRect\inc\RevealFocusAnimator.h"
#include "framework.h"
#include "imaging\inc\RawData.h"
#include "mediaresource.h"
#include "DXamlServices.h"
#include "ColorUtil.h"
#include "KnownColors.h"
#include "host.h"
#include "DoubleUtil.h"
#include "RevealFocusDefaultValue.h"
#include "RootScale.h"
#include <FxCallbacks.h>

using namespace DirectUI;
using namespace RevealFocus;

using wrl_wrappers::HStringReference;

namespace FocusRect {

template<typename LightInterface>
Microsoft::WRL::ComPtr<WUComp::IVisualUnorderedCollection> GetLightTargets(
    _In_ LightInterface* light)
{
    wrl::ComPtr<WUComp::ICompositionLight> compLight;
    IFCFAILFAST(light->QueryInterface(IID_PPV_ARGS(&compLight)));

    wrl::ComPtr<WUComp::IVisualUnorderedCollection> targets;
    IFCFAILFAST(compLight->get_Targets(&targets));
    return targets;
}

Microsoft::WRL::ComPtr<WUComp::ISpriteVisual> RevealFocusSource::BuildRevealFocusVisual(
    _In_opt_ WUComp::IVisual* existingVisual,
    _In_ CUIElement* target,
    _In_ uint32_t primaryColor,
    _In_ const XTHICKNESS& primaryThickness,
    _In_ bool transparentBorder)
{
    m_target = xref::get_weakref(target);

    auto coreServices = target->GetContext();
    WUComp::ICompositor* compositor = coreServices->GetCompositor();

    // Color has changed, create new effect brush. If velocity is enabled, then rebuild
    // the brush with whatever parameters may have been set via reg keys
    uint32_t revealColor = ColorUtils::ApplyAlphaScale(primaryColor, GetDefaultValue(DefaultValue::AlphaMask));
    const XTHICKNESS insets = GetInsetsFromThickness(primaryThickness);

    // When the border is transparent, we want the glow to hug the element, so subtract it.
    const XTHICKNESS newAdjustments = transparentBorder ? SubtractThickness(insets, primaryThickness) : insets;

    const bool thicknessChanged = newAdjustments != m_adjustments;
    const bool colorChanged = m_rgb != revealColor;

    // Only recreate the surface if the color or thickness has changed, otherwise we can reuse what we currently have
    if (colorChanged || thicknessChanged)
    {
        // If we have an existing brush, then get the existing source of the effect from that and we'll reuse it.
        wrl::ComPtr<WUComp::ICompositionEffectBrush> existingEffectBrush;
        if (m_compBrush)
        {
            IFCFAILFAST(m_compBrush.As(&existingEffectBrush));
        }

        auto newEffectBrush = CreateRevealFocusEffectBrush(
            compositor,
            coreServices,
            revealColor,
            colorChanged,
            insets,
            thicknessChanged,
            existingEffectBrush.Get());

        IFCFAILFAST(newEffectBrush.As(&m_compBrush));

        m_rgb = revealColor;
        m_adjustments = newAdjustments;
    }

    wrl::ComPtr<WUComp::ISpriteVisual> spriteVisual;
    if (existingVisual)
    {
        IFCFAILFAST(existingVisual->QueryInterface(IID_PPV_ARGS(&spriteVisual)));
    }
    else
    {
        IFCFAILFAST(compositor->CreateSpriteVisual(&spriteVisual));

        // Remove existing targets from the lights
        for (const auto& light : m_lights.m_vector)
        {
            auto targets = GetLightTargets(light.Get());
            IFCFAILFAST(targets->RemoveAll());
        }

        if (UseLightingEffect())
        {
            wrl::ComPtr<WUComp::IVisual> targetVisual;
            IFCFAILFAST(spriteVisual.As(&targetVisual));
            InitializeLightingEffects(compositor, targetVisual.Get());
        }
    }

    IFCFAILFAST(spriteVisual->put_Brush(m_compBrush.Get()));

    return spriteVisual;
}

Microsoft::WRL::ComPtr<WUComp::ICompositionEffectBrush> RevealFocusSource::CreateRevealFocusEffectBrush(
    _In_ WUComp::ICompositor* compositor,
    _In_ CCoreServices* coreServices,
    _In_ const uint32_t rgb,
    _In_ const bool colorChanged,
    _In_ const XTHICKNESS& insets,
    _In_ const bool sizeChanged,
    _In_opt_ WUComp::ICompositionEffectBrush* existingEffectBrush)
{
    DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_sourceParam, L"nineGridBrush");

    // If we have an existing effect brush, grab the source brush of the effect
    wrl::ComPtr<WUComp::ICompositionBrush> existingEffectSourceBrush;
    if (existingEffectBrush)
    {
        IFCFAILFAST(existingEffectBrush->GetSourceParameter(
            HStringReference(c_sourceParam.GetBuffer(), c_sourceParam.GetCount()).Get(),
            &existingEffectSourceBrush));
    }

    // The overall reveal focus effect looks like this:
    //  (1) <CompositeEffect>
    //  (2)     <ColorEffect />
    //  (3)     <CompositionNineGridBrush>
    //  (4)       <CompositionSurfaceBrush with glow image surface />
    //          <CompositionNineGridBrush>
    //      </CompositeEffect>

    wrl::ComPtr<WUComp::ICompositionSurfaceBrush> revealSurfaceBrush;

    // If we have an existing effect source brush, and the size of the glow hasn't changed, then we can reuse what
    // we have. Otherwise, we'll recreate the surface.
    if (existingEffectSourceBrush && !sizeChanged)
    {
        wrl::ComPtr<WUComp::ICompositionNineGridBrush> existingEffectSourceBrushAsNineGrid;
        IFCFAILFAST(existingEffectSourceBrush->QueryInterface(IID_PPV_ARGS(&existingEffectSourceBrushAsNineGrid)));

        wrl::ComPtr<WUComp::ICompositionBrush> existingSurfaceBrush;
        IFCFAILFAST(existingEffectSourceBrushAsNineGrid->get_Source(&existingSurfaceBrush));
        IFCFAILFAST(existingSurfaceBrush.As(&revealSurfaceBrush));
    }
    else
    {
        revealSurfaceBrush = CreateRevealSurfaceBrush(compositor, coreServices, insets);
    }

    wrl::ComPtr<WUComp::ICompositionEffectBrush> compEffectBrush(existingEffectBrush);

    // If the color changed, then we need to recreate the effect
    if (colorChanged)
    {
        auto compositeColorEffect = CreateColorEffect(rgb, c_sourceParam);
        compEffectBrush = CreateEffectBrush(compositor, compositeColorEffect.Get());
    }

    if (compEffectBrush)
    {
        const auto contentRootCoordinator = coreServices->GetContentRootCoordinator();
        // Bug 19562831: [AppWindow HighDPI]: RevealFocusSource uses incorrect scale
        const auto root = contentRootCoordinator->Unsafe_IslandsIncompatible_CoreWindowContentRoot();
        const auto scale = RootScale::GetRasterizationScaleForContentRoot(root);
        auto effectSourceBrush = CreateEffectSourceBrush(
            compositor,
            existingEffectSourceBrush.Get(),
            revealSurfaceBrush.Get(),
            insets,
            scale);

        IFCFAILFAST(compEffectBrush->SetSourceParameter(
            HStringReference(c_sourceParam.GetBuffer(), c_sourceParam.GetCount()).Get(),
            effectSourceBrush.Get()));
    }

    return compEffectBrush;
}

// There are 4 different lights that can target the reveal visual:
// 1. Default Ambient Light. - This ambient light is always targeting the visual. The intensity is briefly intensified as soon as the element gets focus
// 2. Traveling SpotLight - This spotlight moves across the element when it gets focus. If the element is too small, it won't be animated
// 3. Pulsing Ambient Light - Slowly pulses while element has focus. Requires AC power to be enabled so it doesn't drain the battery.
// 4. Press Ambient Light - When the focused element is pressed, this light grows brighter and stays intense while the key is being held. Once released,
//                  it goes back down to 0.
//  We reuse the lights if we have them, otherwise if they've been discarded we'll recreated them. Lights are only discarded during a DComp cleanup,
//  which will only happen during test code.
void RevealFocusSource::InitializeLightingEffects(
    _In_ WUComp::ICompositor* compositor,
    _In_ WUComp::IVisual* targetVisual)
{
    wrl::ComPtr<WUComp::ICompositor2> compositor2;
    IFCFAILFAST(compositor->QueryInterface(IID_PPV_ARGS(&compositor2)));

    const auto lightColor = ColorUtils::GetWUColor(static_cast<uint32_t>(KnownColors::White));

    wrl::ComPtr<WUComp::IAmbientLight> ambientLight;
    if (!TryGetLight(RevealFocusLight::DefaultAmbient, &ambientLight))
    {
        IFCFAILFAST(compositor2->CreateAmbientLight(&ambientLight));
        AddLight(ambientLight.Get(), RevealFocusLight::DefaultAmbient);
    }
    SetupAmbientLight(ambientLight.Get(), lightColor, targetVisual, GetDefaultValue(DefaultValue::DefaultLightIntensity));

    if (!TryGetLight(RevealFocusLight::PulsingAmbient, &ambientLight))
    {
        IFCFAILFAST(compositor2->CreateAmbientLight(&ambientLight));
        AddLight(ambientLight.Get(), RevealFocusLight::PulsingAmbient);
    }
    SetupAmbientLight(ambientLight.Get(), lightColor, targetVisual, GetDefaultValue(DefaultValue::PulsingLightIntensityMid));

    wrl::ComPtr<WUComp::ISpotLight> spotLight;
    if (!TryGetLight(RevealFocusLight::TravelingSpotlight, &spotLight))
    {
        IFCFAILFAST(compositor2->CreateSpotLight(&spotLight));
        AddLight(spotLight.Get(), RevealFocusLight::TravelingSpotlight);
    }
    SetupSpotLight(spotLight.Get(), lightColor, targetVisual);

    // If animations are disabled, then we won't ever turn this light on
    if (IsAnimationEnabled())
    {
        if (!TryGetLight(RevealFocusLight::PressAmbient, &ambientLight))
        {
            IFCFAILFAST(compositor2->CreateAmbientLight(&ambientLight));
            AddLight(ambientLight.Get(), RevealFocusLight::PressAmbient);
        }
        auto targets = GetLightTargets(ambientLight.Get());
        IFCFAILFAST(targets->Add(targetVisual));

        wrl::ComPtr<WUComp::ICompositionLight3> compLight3;
        IFCFAILFAST(ambientLight.As(&compLight3));
        IFCFAILFAST(compLight3->put_IsEnabled(false));
    }
}

xref_ptr<CUIElement> RevealFocusSource::GetTarget() const
{
    return m_target.lock();
}

wfn::Vector2 RevealFocusSource::GetSizeAdjustment() const
{
    return wfn::Vector2{m_adjustments.left + m_adjustments.right, m_adjustments.top + m_adjustments.bottom};
}

wfn::Vector3 RevealFocusSource::GetOffsetAdjustment() const
{
    return wfn::Vector3{-m_adjustments.left, -m_adjustments.top, 0.0f};
}

bool RevealFocusSource::HasLight() const
{
    return !m_lights.m_vector.empty();
}

/*static*/
XTHICKNESS RevealFocusSource::GetRevealThickness()
{
    // This is an arbitrary thickness value. The actual glow varies based on how thick the primary border is.
    // The idea is that we need to change the FocusRectHost so that the glow doesn't get inappropriately clipped.
    return Thickness(6.0);
}

void RevealFocusSource::SetupAmbientLight(
    _In_ WUComp::IAmbientLight* ambientLight,
    _In_ wu::Color color,
    _In_ WUComp::IVisual* targetVisual,
    _In_ float intensity)
{
    IFCFAILFAST(ambientLight->put_Color(color));

    wrl::ComPtr<WUComp::IAmbientLight2> ambientLight2;
    IFCFAILFAST(ambientLight->QueryInterface(IID_PPV_ARGS(&ambientLight2)));
    IFCFAILFAST(ambientLight2->put_Intensity(intensity));

    auto targets = GetLightTargets(ambientLight);
    IFCFAILFAST(targets->Add(targetVisual));
}

void RevealFocusSource::SetupSpotLight(
    _In_ WUComp::ISpotLight* spotLight,
    _In_ wu::Color color,
    _In_ WUComp::IVisual* targetVisual)
{
    IFCFAILFAST(spotLight->put_InnerConeColor(color));
    IFCFAILFAST(spotLight->put_CoordinateSpace(targetVisual));

    IFCFAILFAST(spotLight->put_InnerConeAngleInDegrees(GetDefaultValue(DefaultValue::InnerConeAngle)));
    IFCFAILFAST(spotLight->put_OuterConeAngleInDegrees(GetDefaultValue(DefaultValue::OuterConeAngle)));

    IFCFAILFAST(spotLight->put_Offset(wfn::Vector3{0,0,0}));
    auto targets = GetLightTargets(spotLight);
    IFCFAILFAST(targets->Add(targetVisual));
}

bool RevealFocusSource::IsAnimationEnabled() const
{
    bool shouldAnimate = false;
    auto target = GetTarget();
    if (target)
    {
        IFCFAILFAST(FxCallbacks::FrameworkCallbacks_IsAnimationEnabled(&shouldAnimate));
    }

    return !!shouldAnimate;
}

void RevealFocusSource::Reset(_In_ ResetReason reason)
{
    switch (reason)
    {
    case ResetReason::DCompCleanup:
    {
        // Reset everything when cleaning up DComp resources. This is only
        // done via test hooks.
        m_compBrush.Reset();
        m_target.reset();
        m_rgb = 0;
        m_adjustments = {};
        m_lights.m_vector.clear();
        break;
    }
    case ResetReason::DeviceLost:
    {
        // Our brush is holding onto a surface that is device dependent. Throw away
        // the entire brush so we'll rebuild it on next focus. Lights aren't device
        // dependent so we are ok.
        m_compBrush.Reset();
        break;
    }
    case ResetReason::FocusReset:
    {
        // When focus is reset, remove all targets from the lights
        for (const auto& light : m_lights.m_vector)
        {
            auto targets = GetLightTargets(light.Get());
            IFCFAILFAST(targets->RemoveAll());
        }
        break;
    }
    }
}

Microsoft::WRL::ComPtr<wgr::Effects::IGraphicsEffect> RevealFocusSource::CreateColorEffect(
    _In_ uint32_t rgb,
    _In_ const xstring_ptr_view& effectSourceParameter)
{
    auto colorEffect = wrl::Make<ColorSourceEffect>();
    colorEffect->Color = PALToD2DColor(rgb);

    wrl::ComPtr<WUComp::ICompositionEffectSourceParameterFactory> sourceParamFactory;
    IFCFAILFAST(wf::GetActivationFactory(
        HStringReference(RuntimeClass_Microsoft_UI_Composition_CompositionEffectSourceParameter).Get(),
        &sourceParamFactory));

    wrl::ComPtr<WUComp::ICompositionEffectSourceParameter> param;
    IFCFAILFAST(sourceParamFactory->Create(
        HStringReference(effectSourceParameter.GetBuffer(), effectSourceParameter.GetCount()).Get(),
        &param));

    auto compositeEffect = wrl::Make<CompositeEffect>();
    compositeEffect->Mode = D2D1_COMPOSITE_MODE_DESTINATION_IN;
    IFCFAILFAST(param.As(&compositeEffect->Source));
    compositeEffect->Destination = colorEffect;

    return compositeEffect;
}

Microsoft::WRL::ComPtr<WUComp::ICompositionEffectBrush> RevealFocusSource::CreateEffectBrush(
    _In_ WUComp::ICompositor* compositor,
    _In_ wgr::Effects::IGraphicsEffect* effect)
{
    wrl::ComPtr<WUComp::ICompositionEffectFactory> effectFactory;
    IFCFAILFAST(compositor->CreateEffectFactory(effect, &effectFactory));

    wrl::ComPtr<WUComp::ICompositionEffectBrush> compEffectBrush;
    IFCFAILFAST(effectFactory->CreateBrush(&compEffectBrush));

    return compEffectBrush;
}

Microsoft::WRL::ComPtr<WUComp::ICompositionSurfaceBrush> RevealFocusSource::CreateRevealSurfaceBrush(
    _In_ WUComp::ICompositor* compositor,
    _In_ CCoreServices* services,
    _In_ const XTHICKNESS& inset)
{
    const float glowSizeModifier = GetDefaultValue(DefaultValue::GlowSizeModifier);
    const float glowWidth = glowSizeModifier*inset.left + glowSizeModifier*inset.right;
    const float glowHeight = glowSizeModifier*inset.top + glowSizeModifier*inset.bottom;

    CREATEPARAMETERS cp(services);
    xref_ptr<CLoadedImageSurface> imageSurface;
    IFCFAILFAST(CLoadedImageSurface::Create(reinterpret_cast<CDependencyObject**>(imageSurface.ReleaseAndGetAddressOf()), &cp));
    imageSurface->SetDesiredSize(glowWidth, glowHeight);

    RawData rawImageData;
    IFCFAILFAST(services->GetBrowserHost()->GetResourceData(IMAGE_REVEAL_FOCUS, XSTRING_PTR_EPHEMERAL(L"PNG"), &rawImageData));

    IFCFAILFAST(imageSurface->InitFromMemory(wil::make_unique_failfast<RawData>(std::move(rawImageData))));
    wrl::ComPtr<WUComp::ICompositionSurface> surfacePeerAsSurface;
    IFCFAILFAST(DXamlServices::GetPeer(imageSurface, IID_PPV_ARGS(&surfacePeerAsSurface)));

    wrl::ComPtr<WUComp::ICompositionSurfaceBrush> surfaceBrush;

    IFCFAILFAST(compositor->CreateSurfaceBrushWithSurface(surfacePeerAsSurface.Get(), &surfaceBrush));
    // TODO 15394150: is this necessary to stretch?
    IFCFAILFAST(surfaceBrush->put_Stretch(WUComp::CompositionStretch_Fill));
    return surfaceBrush;
}

Microsoft::WRL::ComPtr<WUComp::ICompositionBrush> RevealFocusSource::CreateEffectSourceBrush(
    _In_ WUComp::ICompositor* compositor,
    _In_opt_ WUComp::ICompositionBrush* existingEffectSourceBrush,
    _In_ WUComp::ICompositionSurfaceBrush* surfaceBrush,
    _In_ const XTHICKNESS& inset,
    _In_ const float scale)
{
    wrl::ComPtr<WUComp::ICompositor2> compositor2;
    IFCFAILFAST(compositor->QueryInterface(IID_PPV_ARGS(&compositor2)));

    wrl::ComPtr<WUComp::ICompositionNineGridBrush> nineGrid;
    // If there was an existing one, then reuse that
    if (existingEffectSourceBrush)
    {
        IFCFAILFAST(existingEffectSourceBrush->QueryInterface(IID_PPV_ARGS(&nineGrid)));
    }
    else
    {
        IFCFAILFAST(compositor2->CreateNineGridBrush(&nineGrid));
        IFCFAILFAST(nineGrid->put_IsCenterHollow(true));
    }

    // Ninegrid brush with a surface brush set as its source will treat the insets as pixels in the source surface,
    // and will use the inset scale factor to translate that into Visual-relative values (DIPs) . Because of this, we call
    // CompositionNineGridBrush.SetInsetScales(1/scale), and express the inset thickness in terms of source-surface pixels.
    const float inverseScale = (scale == 0 ? 0 : 1.0f / scale);

    IFCFAILFAST(nineGrid->put_LeftInset(inset.left*scale));
    IFCFAILFAST(nineGrid->put_RightInset(inset.right*scale));
    IFCFAILFAST(nineGrid->put_TopInset(inset.top*scale));
    IFCFAILFAST(nineGrid->put_BottomInset(inset.bottom*scale));

    IFCFAILFAST(nineGrid->SetInsetScales(inverseScale));

    wrl::ComPtr<WUComp::ICompositionBrush> surfaceBrushAsBrush;
    IFCFAILFAST(surfaceBrush->QueryInterface(IID_PPV_ARGS(&surfaceBrushAsBrush)));
    IFCFAILFAST(nineGrid->put_Source(surfaceBrushAsBrush.Get()));

    wrl::ComPtr<WUComp::ICompositionBrush> effectSourceBrush;
    IFCFAILFAST(nineGrid.As(&effectSourceBrush));
    return effectSourceBrush;
}

bool RevealFocusSource::UseLightingEffect()
{
    return !!GetDefaultValue(DefaultValue::UseLightingEffect);
}

bool RevealFocusSource::IsTravelingFocusEnabled(_In_ DirectUI::FocusNavigationDirection direction) const
{
    // Traveling focus doesn't look good if the element is too small, so disable it if that's the case
    return IsAnimationEnabled() && (GetTravelingDistance(direction) > GetDefaultValue(DefaultValue::SmallTravelingSpotLightThreshold)) && !!GetDefaultValue(DefaultValue::UseTravelingFocus);
}

wf::TimeSpan RevealFocusSource::GetSpotLightDuration(_In_ DirectUI::FocusNavigationDirection direction) const
{
    const float spotLightSpeed = GetTravelingDistance(direction) / GetDefaultValue(DefaultValue::SpotLightSpeed);
    return wf::TimeSpan { HNS_FROM_SECOND(static_cast<int64_t>(spotLightSpeed)) };
}

float RevealFocusSource::GetTravelingDistance(_In_ DirectUI::FocusNavigationDirection direction) const
{
    auto target = do_pointer_cast<CFrameworkElement>(m_target.lock());
    float value = 0.0f;
    switch(direction)
    {
    case DirectUI::FocusNavigationDirection::Up:
    case DirectUI::FocusNavigationDirection::Down:
        if (target)
        {
            value = target->GetActualHeight();
        }
        break;
    case DirectUI::FocusNavigationDirection::Left:
    case DirectUI::FocusNavigationDirection::Right:
        if (target)
        {
            value = target->GetActualWidth();
        }
        break;
    case DirectUI::FocusNavigationDirection::None:
    // No direction is OK, just means we won't use traveling focus
        break;
    case DirectUI::FocusNavigationDirection::Previous:
    case DirectUI::FocusNavigationDirection::Next:
    default:
        ASSERT(false);
    }

    return value;
}

float RevealFocusSource::GetSpotLightSize() const
{
    if (auto target = do_pointer_cast<CFrameworkElement>(m_target.lock()))
    {
        return std::max(target->GetActualWidth(), target->GetActualHeight());
    }

    return 0.0f;
}

XTHICKNESS RevealFocusSource::GetInsetsFromThickness(_In_ const XTHICKNESS& primaryThickness) const
{
    XTHICKNESS inset = {};

    const float scale = GetDefaultValue(DefaultValue::ThicknessToGlowSizeScale);
    const float min = GetDefaultValue(DefaultValue::MinimumGlowSize);
    const auto linearInsetFunc = [scale, min](float x) -> float {
        return  x > 0.0f ? scale*x+min : 0.0f;
    };

    inset.left = linearInsetFunc(primaryThickness.left);
    inset.right = linearInsetFunc(primaryThickness.right);
    inset.top = linearInsetFunc(primaryThickness.top);
    inset.bottom = linearInsetFunc(primaryThickness.bottom);

    return inset;
}

}