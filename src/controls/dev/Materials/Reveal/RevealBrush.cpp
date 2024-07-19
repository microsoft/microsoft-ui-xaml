﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "RevealBrush.h"

#pragma warning(push)
#pragma warning(disable: 6101)  // Returning uninitialized memory '<value>'.  A successful path through the function does not set the named _Out_ parameter.
#include "Microsoft.UI.Private.Composition.Effects_impl.h"
#pragma warning(pop)

#include "XamlAmbientLight.h"
#include "RevealHoverLight.h"
#include "RevealBorderLight.h"
#include "vector.h"
#include "RuntimeProfiler.h"

#include "RevealBorderBrush.properties.cpp"
#include "RevealBackgroundBrush.properties.cpp"

const winrt::Color RevealBrush::sc_defaultColor{ 0, 255, 255, 255 };

const winrt::Color RevealBrush::sc_ambientContributionColor{ 255, 127, 127, 127 };
const float RevealBrush::sc_diffuseAmount{ 0.2f };
const float RevealBrush::sc_specularAmount{ 0.0f };
const float RevealBrush::sc_specularShine{ 0.0f };

const float RevealBrush::sc_diffuseAmountBorder{ 0.2f };
const float RevealBrush::sc_specularAmountBorder{ 0.0f };
const float RevealBrush::sc_specularShineBorder{ 0.0f };

const winrt::Matrix5x4 RevealBrush::sc_colorToAlphaMatrix =
{ 1.0f, 0.0f, 0.0f, 0.15f,
  0.0f, 1.0f, 0.0f, 0.0f,
  0.0f, 0.0f, 1.0f, 0.0f,
  0.0f, 0.0f, 0.0f, 0.0f,
  0.0f, 0.0f, 0.0f, 0.85f };

const winrt::Matrix5x4 RevealBrush::sc_luminanceToAlphaMatrix
{ 1.0f, 0.0f, 0.0f, 0.2125f,
  0.0f, 1.0f, 0.0f, 0.7154f,
  0.0f, 0.0f, 1.0f, 0.0721f,
  0.0f, 0.0f, 0.0f, 0.0f,
  0.0f, 0.0f, 0.0f, 0.0f };

// Originally the reveal border effect was built as:
// 0) SceneLightingEffect input
// 1) (0) multiplied through Luminance-to-alpha color matrix with ambient contribution of 0.5 in the RGBA offset row.
// 2) ArithmeticCompositeEffect taking (1) and the base color with the multiply factors = 2 and offset = -1.
// 
// In light theme we now want to "invert" that result -- but we also don't want to invert the color part of 
// the equation. In addition, the ArithmeticComposite step needs to work when the base color is transparent,
// so instead we will just assume that the base color is transparent and just do a "SourceOver" of the border
// effect onto the base color if it's provided. This gives consistent results no matter what the color is.
//
// If we assume that then we can simplify 1 & 2 above to be just one color matrix. ArithmeticComposite
// when one of the inputs is transparent (all 0) is just a Multiply and Offset factor. In our case that's *2 and -1.
// Folding this into the matrix from #1 produces the below.
// 
// At the end of the day, it doesn't matter so much how we got here. There's the luminance to alpha operation
// plus a *2 multiplier. The *2 factor on the luminance to alpha matrix can only be explained as "that's what 
// design wants". :)

// The reveal border matrix is the luminance to alpha matrix * 2
const winrt::Matrix5x4 sc_revealBorderColorMatrix
{ 2.0f, 0.0f, 0.0f, 2 * 0.2125f,
    0.0f, 2.0f, 0.0f, 2 * 0.7154f,
    0.0f, 0.0f, 2.0f, 2 * 0.0721f,
    0.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f };

// The inverse border is the above but the colors are negated.
const winrt::Matrix5x4 sc_revealInvertedBorderColorMatrix
{ -2.0f,  0.0f,  0.0f,  2 * .2125f,
    0.0f, -2.0f,  0.0f,  2 * .7154f,
    0.0f,  0.0f, -2.0f,  2 * .0721f,
    0.0f,  0.0f,  0.0f,  0.0f,
    0.0f,  0.0f,  0.0f,  0.0f };


GlobalDependencyProperty RevealBrush::s_IsContainerProperty{ nullptr };

void RevealBrush::ClearProperties()
{
    s_IsContainerProperty = nullptr;
    RevealBrushProperties::ClearProperties();
}

void RevealBrush::EnsureProperties()
{
    RevealBrushProperties::EnsureProperties();
    if (!s_IsContainerProperty)
    {
        s_IsContainerProperty =
            InitializeDependencyProperty(
                L"IsContainer",
                winrt::name_of<bool>(),
                winrt::name_of<winrt::RevealBrush>(),
                true /* isAttached */,
                box_value(false),
                &OnIsContainerPropertyChanged);
    }
}

RevealBrush::RevealBrush()
{
    //  Logging usage telemetry
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_Reveal);

    EnsureProperties();
}

RevealBorderBrush::RevealBorderBrush()
{
    m_isBorder = true;
}

RevealBrush::~RevealBrush()
{
    if (m_noiseChangedToken.value)
    {
        MaterialHelper::NoiseChanged(m_noiseChangedToken);
        m_noiseChangedToken.value = 0;
    }
}

void RevealBrush::OnConnected()
{
    // XCBB will use Fallback rendering, so do not run derived Brush code.
    if (SharedHelpers::IsInDesignMode()) { return; }

    MaterialHelper::OnRevealBrushConnected();

    if (!m_fallbackColorChangedToken.value)
    {
        m_fallbackColorChangedToken.value = RegisterPropertyChangedCallback(
            winrt::XamlCompositionBrushBase::FallbackColorProperty(), { this, &RevealBrush::OnFallbackColorChanged });
    }

    // Stay subscribed to NoiseChanged events when brush disconnected so that it gets marked to pick up new noise when (and if) it gets reconnected.
    if (!m_noiseChangedToken.value)
    {
        m_noiseChangedToken = MaterialHelper::NoiseChanged([this](auto sender) { OnNoiseChanged(sender); });
    }

    m_isConnected = true;
    m_materialPolicyChangedToken = MaterialHelper::PolicyChanged([this](auto sender, auto args) { OnMaterialPolicyStatusChanged(sender, args); });

    UpdateLightTargets(true /* ambientToo */);
    UpdateRevealBrush();
}

void RevealBrush::OnDisconnected()
{
    if (SharedHelpers::IsInDesignMode()) { return; }

    m_isConnected = false;

    UpdateLightTargets(true /* ambientToo */);

    if (m_brush)
    {
        // Try for winrt::IClosable because MockDComp doesn't implement IClosable.
        if (auto closable = m_brush.try_as<winrt::IClosable>())
        {
            closable.Close();
        }
        m_brush = nullptr;
        CompositionBrush(nullptr);
    }

    if (m_fallbackColorChangedToken.value)
    {
        UnregisterPropertyChangedCallback(winrt::XamlCompositionBrushBase::FallbackColorProperty(), m_fallbackColorChangedToken.value);
        m_fallbackColorChangedToken.value = 0;
    }

    MaterialHelper::OnRevealBrushDisconnected();

    MaterialHelper::PolicyChanged(m_materialPolicyChangedToken);
    m_materialPolicyChangedToken.value = 0;
}

void RevealBrush::OnFallbackColorChanged(const winrt::DependencyObject& /*sender*/, const winrt::DependencyProperty& /*args*/)
{
    // FallbackColor is used by RevealBrush to determine the solid color to use when we experience fallback due to policy.
    // If it changes dynamically and we're using the fallback brush, make sure we update it now.
    if (IsInFallbackMode())
    {
        UpdateLightTargets(false /* ambientToo */);
        UpdateRevealBrush();
    }
}

void RevealBrush::OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    winrt::IDependencyProperty property = args.Property();

    if (property == s_ColorProperty)
    {
        UpdateRevealBrush();
    }
    else if (property == s_TargetThemeProperty)
    {
        UpdateLightTargets(false /* ambientToo */);
        UpdateRevealBrush();
    }
    else if (property == s_AlwaysUseFallbackProperty)
    {
        UpdateLightTargets(false /* ambientToo */);
        UpdateRevealBrush();
    }
}

void RevealBrush::RemoveTargetBrush(const wstring_view& lightID)
{
    try
    {
        winrt::XamlLight::RemoveTargetBrush(lightID, *this);
    }
    catch (const winrt::hresult_error& e)
    {
        // RemoveTargetBrush can fail with RPC_E_WRONG_THREAD if called while the Xaml Core is being shutdown, 
        // and there is evidence from Watson that such calls are made in real apps.
        // In this case, all the relationships between lights and brushes are going to be cleared as part of core shutdown,
        // so effectively all brush targets will be unset, so it's ok to ignore this error.
        if (e.to_abi() != RPC_E_WRONG_THREAD) { throw; }
    }
}

void RevealBrush::UpdateLightTargets(bool ambientToo)
{
    if (!m_isConnected || IsInFallbackMode())
    {
        // Untarget the brush by hover/border/ambient lights
        if (m_isBorder && m_isBorderLightSet)
        {
            auto currentLightId = TargetTheme() == winrt::ApplicationTheme::Light ?
                RevealBorderLight::GetLightThemeIdStatic() : RevealBorderLight::GetDarkThemeIdStatic();
            RemoveTargetBrush(currentLightId);
            m_isBorderLightSet = false;
        }
        else if (m_isHoverLightSet)
        {
            RemoveTargetBrush(RevealHoverLight::GetLightIdStatic());
            m_isHoverLightSet = false;
        }

        if (ambientToo && m_isAmbientLightSet)
        {
            RemoveTargetBrush(XamlAmbientLight::GetLightIdStatic());
            m_isAmbientLightSet = false;
        }
    }
    else
    {
        // Target the brush by hover/border/ambient lights
        if (m_isBorder)         // May need to switch border light so always referesh
        {
            auto oldLightId = RevealBorderLight::GetLightThemeIdStatic();
            auto newLightId = RevealBorderLight::GetDarkThemeIdStatic();
            if (TargetTheme() == winrt::ApplicationTheme::Light)
            {
                std::swap(oldLightId, newLightId);
            }
            if (m_isBorderLightSet)
            {
                RemoveTargetBrush(oldLightId);
                m_isBorderLightSet = false;
            }
            winrt::XamlLight::AddTargetBrush(newLightId, *this);
            m_isBorderLightSet = true;
        }
        else if (!m_isHoverLightSet)
        {
            winrt::XamlLight::AddTargetBrush(RevealHoverLight::GetLightIdStatic(), *this);
            m_isHoverLightSet = true;
        }

        if (ambientToo && !m_isAmbientLightSet)
        {
            winrt::XamlLight::AddTargetBrush(XamlAmbientLight::GetLightIdStatic(), *this);
            m_isAmbientLightSet = true;
        }
    }
}

void RevealBrush::PolicyStatusChangedHelper(bool isDisabledByMaterialPolicy)
{
    m_isDisabledByMaterialPolicy = isDisabledByMaterialPolicy;
    if (m_isConnected)
    {
        UpdateLightTargets(true /* ambientToo */);
        UpdateRevealBrush();
    }
}

void RevealBrush::OnMaterialPolicyStatusChanged(const com_ptr<MaterialHelperBase>& sender, bool isDisabledByMaterialPolicy)
{
    //PolicyStatusChangedHelper(isDisabledByMaterialPolicy); // Bug 24355584: DCPP: Reveal is disabled
    PolicyStatusChangedHelper(true);
}

void RevealBrush::OnNoiseChanged(const com_ptr<MaterialHelperBase>& sender)
{
    m_noiseChanged = true;
    if (m_isConnected)
    {
        UpdateRevealBrush();
    }
}

void RevealBrush::EnsureNoiseBrush()
{
    auto noiseBrush = MaterialHelper::GetNoiseBrush();
    if (noiseBrush != m_noiseBrush)
    {
        m_noiseBrush = noiseBrush;
    }
    m_noiseChanged = false;
}

winrt::Windows::Graphics::Effects::IGraphicsEffect RevealBrush::CreateRevealHoverEffect()
{
    // We apply noise to the SceneLightingEffect to reduce banding artifacts, but we don't want the noise to show up with just
    // ambient lighting (when the brush doesn't have the pointer near it). Since the ambient contribution is a constant, we factor
    // it out to a separate ColorSourceEffect. We can then mask the SLE with a noise texture, combine that with the ambient color
    // effect, and use that in the rest of the effect graph:
    //
    //  (1) <ArithmeticCompositeEffect>
    //  (2)     <BaseColorEffect />
    //  (3)     <ColorMatrixEffect>
    //  (4)         <CompositeStepEffect>
    //  (5)             <ColorSourceEffect for ambient contribution/>
    //  (6)             <AlphaMaskEffect>
    //  (7)                 <SceneLightingEffect without ambient coefficient/>
    //  (8)                 <ColorMatrixEffect>
    //  (9)                     <BorderEffect>
    //                              <Noise texture in a brush />
    //                          </BorderEffect>
    //                      </ColorMatrixEffect>
    //                  </AlphaMaskEffect>
    //              </CompositeStepEffect>
    //          </ColorMatrixEffect>
    //      </ArithmeticCompositeEffect>
    winrt::Color initBaseColor{ 0, 0, 0, 0 };

    // (9) Noise image BorderEffect (infinitely tiles noise image)
    auto noiseBorderEffect = winrt::make_self<Microsoft::UI::Private::Composition::Effects::BorderEffect>();
    noiseBorderEffect->ExtendX(winrt::CanvasEdgeBehavior::Wrap);
    noiseBorderEffect->ExtendY(winrt::CanvasEdgeBehavior::Wrap);
    winrt::CompositionEffectSourceParameter noiseEffectSourceParameter{ L"Noise" };
    noiseBorderEffect->Source(noiseEffectSourceParameter);

    // (8) ColorMatrixEffect applied to (9) to introduce alpha, to be used to mask the SceneLightingEffect
    auto noiseOpacityEffect = winrt::make_self<Microsoft::UI::Private::Composition::Effects::ColorMatrixEffect>();
    noiseOpacityEffect->ColorMatrix(sc_colorToAlphaMatrix);
    noiseOpacityEffect->AlphaMode(winrt::CanvasAlphaMode::Straight);
    noiseOpacityEffect->Source(*noiseBorderEffect);

    // (7) SceneLightingEffect
    // Note: Unlike the Win2D-based effects, SceneLightingEffect is a public Windows platform type, we can instatiate it directly via winrt.
    // No ambient coefficient. This SLE gets noise applied to it, and we don't want the ambient contribution to be affected by noise. The
    // ambient contribution will be added by a separate ColorSourceEffect.
    winrt::Microsoft::UI::Composition::Effects::SceneLightingEffect sceneLightingEffect;
    sceneLightingEffect.AmbientAmount(0);
    sceneLightingEffect.DiffuseAmount(sc_diffuseAmount);
    sceneLightingEffect.SpecularAmount(sc_specularAmount);
    sceneLightingEffect.SpecularShine(sc_specularShine);

    // (6) Alpha mask of (7) with (8)
    auto blendEffectOuter = winrt::make_self<Microsoft::UI::Private::Composition::Effects::AlphaMaskEffect>();
    blendEffectOuter->Source(sceneLightingEffect);
    blendEffectOuter->Mask(*noiseOpacityEffect);

    // (5) ColorSourceEffect
    auto colorSourceEffect = winrt::make_self<Microsoft::UI::Private::Composition::Effects::ColorSourceEffect>();
    colorSourceEffect->Color(sc_ambientContributionColor);

    // (4) Composite of (5) with (6)
    auto compositeEffect = winrt::make_self<Microsoft::UI::Private::Composition::Effects::CompositeStepEffect>();
    compositeEffect->Mode(winrt::CanvasComposite::Add);
    compositeEffect->Destination(*colorSourceEffect);
    compositeEffect->Source(*blendEffectOuter);

    // (3) ColorMatrixEffect applied to (4)
    auto colorMatrixEffect = winrt::make_self<Microsoft::UI::Private::Composition::Effects::ColorMatrixEffect>();
    colorMatrixEffect->ColorMatrix(sc_luminanceToAlphaMatrix);
    colorMatrixEffect->AlphaMode(winrt::CanvasAlphaMode::Straight);
    colorMatrixEffect->Source(*compositeEffect);

    // (2) Base Color Effect
    auto baseColorEffect = winrt::make_self<Microsoft::UI::Private::Composition::Effects::ColorSourceEffect>();
    baseColorEffect->Name(L"BaseColorEffect");
    baseColorEffect->Color(initBaseColor);

    // (1) Composite of (3) [source] with (2) [destination]
    auto arithmeticCompositeEffect = winrt::make_self<Microsoft::UI::Private::Composition::Effects::ArithmeticCompositeEffect>();
    arithmeticCompositeEffect->Source1Amount(2.0f);
    arithmeticCompositeEffect->Source2Amount(2.0f);
    arithmeticCompositeEffect->MultiplyAmount(-2.0f);
    arithmeticCompositeEffect->Offset(-1.0f);
    arithmeticCompositeEffect->Source1(*colorMatrixEffect);
    arithmeticCompositeEffect->Source2(*baseColorEffect);

    return *arithmeticCompositeEffect;
}

winrt::Windows::Graphics::Effects::IGraphicsEffect RevealBrush::CreateRevealBorderEffect(bool isInverted, bool hasBaseColor)
{
    // (1) SceneLightingEffect
    // Note: Unlike the Win2D-based effects, SceneLightingEffect is a public Windows platform type, we can instatiate it directly via winrt.
    winrt::Microsoft::UI::Composition::Effects::SceneLightingEffect sceneLightingEffect;
    sceneLightingEffect.AmbientAmount(0.0f);
    sceneLightingEffect.DiffuseAmount(sc_diffuseAmountBorder);
    sceneLightingEffect.SpecularAmount(sc_specularAmountBorder);
    sceneLightingEffect.SpecularShine(sc_specularShineBorder);

    // (2) ColorMatrixEffect applied to 1
    auto colorMatrixEffect = winrt::make_self<Microsoft::UI::Private::Composition::Effects::ColorMatrixEffect>();
    colorMatrixEffect->ColorMatrix(isInverted ? sc_revealInvertedBorderColorMatrix : sc_revealBorderColorMatrix);
    colorMatrixEffect->AlphaMode(winrt::CanvasAlphaMode::Straight);
    colorMatrixEffect->Source(sceneLightingEffect);

    if (hasBaseColor)
    {
        // (3) Base Color Effect
        auto  baseColorEffect = winrt::make_self<Microsoft::UI::Private::Composition::Effects::ColorSourceEffect>();
        baseColorEffect->Name(L"BaseColorEffect");
        // Set base color to 0 because we want to build one EffectFactory and cache it regardless of the different colors that
        // the app may choose. So make the factory have it as 0,0,0,0 and then we'll make it an animatable property and set it
        // immediately after we create it.
        baseColorEffect->Color({ 0,0,0,0 });

        // (4) Do a source over blend of (2) the color matrix with (3) the base color.
        auto compositeEffect = winrt::make_self<Microsoft::UI::Private::Composition::Effects::CompositeStepEffect>();
        compositeEffect->Mode(winrt::CanvasComposite::SourceOver);
        compositeEffect->Source(*colorMatrixEffect);
        compositeEffect->Destination(*baseColorEffect);

        return *compositeEffect;
    }
    else
    {
        // No base color, just return the color matrix effect.
        return *colorMatrixEffect;
    }
}

winrt::CompositionEffectFactory
RevealBrush::GetOrCreateRevealBrushCompositionEffectFactory(
    bool isBorder,
    bool isInverted,
    bool hasBaseColor,
    const winrt::Compositor& compositor)
{
    auto effectFactory = MaterialHelper::GetOrCreateRevealBrushCompositionEffectFactoryFromCache(
        isBorder,
        isInverted,
        hasBaseColor,
        [isBorder, isInverted, hasBaseColor, &compositor]() {
            // Set up the effect chain for reveal
            winrt::IGraphicsEffect effect;
            if (isBorder)
            {
                effect = CreateRevealBorderEffect(isInverted, hasBaseColor);
            }
            else
            {
                effect = CreateRevealHoverEffect();
            }

            winrt::IVector<winrt::hstring> animatedProperties;
            if (hasBaseColor)
            {
                animatedProperties = winrt::make<Vector<winrt::hstring, MakeVectorParam<VectorFlag::None>()>>();
                animatedProperties.Append(L"BaseColorEffect.Color");
            }

            // Create the Comp effect Brush
            return compositor.CreateEffectFactory(effect, animatedProperties);
        });
    return effectFactory;
}

void RevealBrush::CreateRevealBrush()
{
    winrt::Compositor compositor = winrt::CompositionTarget::GetCompositorForCurrentThread();
    if (!IsInFallbackMode())
    {
        const auto color = Color();
        // If the color is ever not transparent black then start using the base color version of the effect.
        if (color.A != 0 || color.R != 0 || color.G != 0 || color.B != 0)
        {
            m_hasBaseColor = true;
        }

        EnsureNoiseBrush();

        const bool isInverted = m_isBorder && (TargetTheme() == winrt::ApplicationTheme::Light);
        const auto effectFactory = GetOrCreateRevealBrushCompositionEffectFactory(m_isBorder, isInverted, m_hasBaseColor, compositor);
        const winrt::CompositionEffectBrush revealBrush = effectFactory.CreateBrush();

        if (m_hasBaseColor)
        {
            // effectFactory is only a template and doesn't have correct color setting, change it
            revealBrush.Properties().InsertColor(L"BaseColorEffect.Color", color);
        }

        if (!m_isBorder)
        {
            // Set noise image source. The border brush is only used for a few pixels, and does not show banding, so it doesn't get noise.
            MUX_ASSERT(m_noiseBrush);
            revealBrush.SetSourceParameter(L"Noise", m_noiseBrush);
        }

        m_brush = revealBrush;
    }
    else
    {
        const auto fallbackColor = FallbackColor();

        if (fallbackColor.A == 0)
        {
            // fallback and transparent brush, set to nullptr for performance optimization
            m_brush = nullptr;
        }
        else
        {
            m_brush = compositor.CreateColorBrush(fallbackColor);
        }
    }

}

void RevealBrush::UpdateRevealBrush()
{
    if (m_isConnected)
    {
        CreateRevealBrush();
    }
    CompositionBrush(m_brush);
}

void RevealBrush::StopAttachingLights()
{
    MaterialHelper::SetShouldBeginAttachingLights(false);
    MaterialHelper::SetShouldContinueAttachingLights(false);
    MaterialHelper::ResetFailedToAttachLightsCount();
}

void RevealBrush::AttachLights()
{
    // There are some non-fatal cases where we failed to hook up lights, tracked by:
    // Bug 18893751: WUXC: Investigate test failures when MUX_ASSERT(ValidatePublicRootAncestor()) is enabled
    // Comment this assert for now and skip attaching lights. Remove this when Bug 18893751 is fixed.
    // MUX_ASSERT(ValidatePublicRootAncestor());

    RevealBrush::AttachLightsImpl();
}

void RevealBrush::AttachLightsImpl()
{
    // Safe to attach RootScrollViewer lights immediately
    auto windowRoot = winrt::Window::Current() ? winrt::Window::Current().Content() : nullptr;
    if (!windowRoot) { return; }

    RevealBrush::AttachLightsToAncestor(windowRoot, true);
}

winrt::UIElement RevealBrush::GetAncestor(const winrt::UIElement& root)
{
    auto current = root;
    auto parent = current;

    // Walk up to the highest element and hook up lights there.
    // If there is an ambient light anywhere on the path, we must have already 
    // added the global lights required for Reveal, so return immediately.
    do
    {
        current = parent;
        parent = winrt::VisualTreeHelper::GetParent(current).as<winrt::UIElement>();
    } while (parent != nullptr);

    return current;
}

bool RevealBrush::ValidatePublicRootAncestor()
{
    auto windowRoot = winrt::Window::Current() ? winrt::Window::Current().Content() : nullptr;

    // On MUX + RS2, it's possible XCB::OnConnected is called before window has content.
    if (!windowRoot)
    {
        return false;
    }

    // Do some more checks to ensure we didn't put lights where we don't expect.
    // Either Window.Content is a Canvas, in which case it's the immediate child of the root visual and we should set lights
    // on it directly, or it isn't, in which case we should have walked up to the RootScrollViewer and set lights there.
    auto ancestor = GetAncestor(windowRoot);
    bool windowContentIsCanvas = static_cast<bool>(windowRoot.try_as<winrt::Canvas>());
    bool walkedUpToScrollViewer = winrt::VisualTreeHelper::GetParent(windowRoot) &&
        static_cast<bool>(ancestor.try_as<winrt::FxScrollViewer>());

    // On MUX + RS3/RS4, it's possible XCB::OnConnected is called before visual tree is constructed and the ancestor walk returns a false elemenet.
    return windowContentIsCanvas || walkedUpToScrollViewer;
}

void RevealBrush::AttachLightsToElement(const winrt::UIElement& element, bool trackAsRootToDisconnectFrom)
{
    auto lights = element.Lights();

    bool hasRevealBorderLights = false;
    for (auto light : lights)
    {
        if (auto self = light.try_as<winrt::RevealBorderLight>())
        {
            hasRevealBorderLights = true;
            break;
        }
    }

    if (!hasRevealBorderLights)
    {
        //
        // To achieve the reveal border effect we need two lights AND the configuration of those lights needs be different per-theme.
        // In order to do this we attach 4 lights to the root in each combination -- ("normal", "wide") x ("light", "dark").
        //
        // When RevealBorderLight is in light theme it reports a different ID than in dark theme. RevealBorderBrush also has a
        // theme property and it will select the RevealBorderLight "light theme" or "dark theme" ID as appropriate.
        //
        // In this way we can have the desired effect of different light configs in dark vs light theme but also support 
        // subtrees that have different theme via FrameworkElement.RequestedTheme because the ThemeResource references to
        // RevealBorderBrush resources will choose the right brush and then those brushes will choose the right light.
        //
        auto borderLightTheme = winrt::make_self<RevealBorderLight>();
        borderLightTheme->SetIsLightTheme(true);
        auto wideBorderLightTheme = winrt::make_self<RevealBorderLight>();
        wideBorderLightTheme->SetIsWideLight(true);
        wideBorderLightTheme->SetIsLightTheme(true);

        auto borderDarkTheme = winrt::make_self<RevealBorderLight>();
        auto wideBorderDarkTheme = winrt::make_self<RevealBorderLight>();
        wideBorderDarkTheme->SetIsWideLight(true);

        auto ambientLight = winrt::make_self<XamlAmbientLight>();

        lights.Append(*borderLightTheme);
        lights.Append(*wideBorderLightTheme);
        lights.Append(*borderDarkTheme);
        lights.Append(*wideBorderDarkTheme);
        lights.Append(*ambientLight);

        if (trackAsRootToDisconnectFrom)
        {
            MaterialHelper::TrackRevealLightsToRemove(lights,
                { *borderLightTheme, *wideBorderLightTheme, *borderDarkTheme, *wideBorderDarkTheme, *ambientLight });
        }
    }
}

void RevealBrush::AttachLightsToAncestor(const winrt::UIElement& root, bool trackAsRootToDisconnectFrom)
{
    auto ancestor = GetAncestor(root);
    AttachLightsToElement(ancestor, trackAsRootToDisconnectFrom);
}

// If Reveal is enabled, GridViewItem/ListViewItem add two additional layers: 
//     RevealBackground and RevealBorderBrush
// It has great performance degradation on Xbox
// The workaround is if it's running on Xbox and it's not in MouseMode,
// then reveal always runs on fallback mode. No additional layers is applied then.
// Please remove this function if performance is improved in future.
bool RevealBrush::IsOnXboxAndNotMouseMode()
{
    return SharedHelpers::IsOnXbox() && !SharedHelpers::IsMouseModeEnabled();
}

bool RevealBrush::IsInFallbackMode()
{
    return m_isInFallbackMode = m_isDisabledByMaterialPolicy || AlwaysUseFallback() || IsOnXboxAndNotMouseMode();
}

void RevealBrush::OnIsContainerPropertyChanged(
    const winrt::DependencyObject& sender,
    const winrt::DependencyPropertyChangedEventArgs& args)
{
    if (auto elementSender = sender.try_as<winrt::IUIElement>())
    {
        bool isAdding = unbox_value<bool>(args.NewValue());

        auto lights = elementSender.Lights();
        int i = 0;

        if (isAdding)
        {
            // add the hover light
            auto hoverLight = winrt::make_self<RevealHoverLight>();
            lights.Append(*hoverLight);

            // add the press light
            auto pressLight = winrt::make_self<RevealHoverLight>();
            pressLight->SetIsPressLight(true);
            lights.Append(*pressLight);
        }
        else
        {
            // Step through all of the lights
            for (auto light : lights)
            {
                if (auto self = light.try_as<winrt::RevealHoverLight>())
                {
                    // if we are in remove mode, remove all hover lights
                    lights.RemoveAt(i);
                }
                i++;
            }
        }
    }
}

void RevealBrush::OnStatePropertyChanged(
    const winrt::DependencyObject& sender,
    const winrt::DependencyPropertyChangedEventArgs& args)
{
    auto currentState = unbox_value<winrt::RevealBrushState>(args.OldValue());
    auto targetState = unbox_value<winrt::RevealBrushState>(args.NewValue());

    if (currentState == targetState)
    {
        return;
    }

    if (sender)
    {
        if (auto uiElement = sender.try_as<winrt::IUIElement>())
        {
            // If we transition to something other than Normal, see if we need to attach the hover light.
            if (targetState != winrt::RevealBrushState::Normal)
            {
                sender.SetValue(s_IsContainerProperty, box_value(true));
            }

            // Transition the Hover Light and the Pressed Light
            auto lights = uiElement.Lights();
            MUX_ASSERT(lights.Size() == 2);

            for (auto light : lights)
            {
                if (auto hoverLight = light.try_as<winrt::RevealHoverLight>())
                {
                    auto internalHoverLight = winrt::get_self<RevealHoverLight>(hoverLight);
                    internalHoverLight->GoToState(targetState);
                }
            }
        }
    }
}
