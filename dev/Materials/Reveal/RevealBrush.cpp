// Copyright (c) Microsoft Corporation. All rights reserved.
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
#ifndef BUILD_WINDOWS
    if (m_noiseChangedToken.value)
    {
        MaterialHelper::NoiseChanged(m_noiseChangedToken);
        m_noiseChangedToken.value = 0;
    }
#endif
}

#if BUILD_WINDOWS
void RevealBrush::OnElementConnected(winrt::DependencyObject element) noexcept
{
    // XCBB will use Fallback rendering, so do not run derived Brush code.
    if (SharedHelpers::IsInDesignMode()) { return; }

    bool hasNewMaterialPolicy = false;
    winrt::XamlIsland xamlIsland = winrt::XamlIsland::GetIslandFromElement(element.try_as<winrt::UIElement>());

    if (xamlIsland)
    {
        if (xamlIsland != m_associatedIsland)
        {
            if (m_associatedIsland) // Brush has already been connected to a different Island
            {
                m_isInterIsland = true;
            }
            else
            {
                // First time getting MP for island
                if (m_materialProperties) // Brush has already been connected under a CoreWindow
                {
                    m_isInterIsland = true;
                }
                m_associatedIsland = xamlIsland;
                winrt::IInspectable materialPropertiesInsp = xamlIsland.MaterialProperties();
                m_materialProperties = materialPropertiesInsp.try_as<winrt::MaterialProperties>();
                hasNewMaterialPolicy = true;
            }
        }

        if (!m_isBorder && m_islandTransformChangedToken.value == 0)
        {
            MaterialHelper::BrushTemplates<RevealBrush>::HookupIslandDpiChangedHandler(this);
        }
    }
    else
    {
        if (m_associatedIsland) // Brush has already been connected under an island
        {
            // Attempt to use brush in both an island and CoreWindow - not supported by AcrylicBrush in RS5.
            // Put the brush in fallback mode.
            m_isInterIsland = true;
        }

        // First time getting MP for CoreWindow
        if (!m_materialProperties)
        {
            m_materialProperties = winrt::MaterialProperties::GetForCurrentView();
            hasNewMaterialPolicy = true;
        }

        if (!m_isBorder)
        {
            MaterialHelper::BrushTemplates<RevealBrush>::HookupWindowDpiChangedHandler(this);
        }
    }
    _ASSERT_EXPR(m_materialProperties, L"No MaterialProperties available in RevealBrush::OnElementConnected");

    // Dispatcher needed as TransparencyPolicyChanged is raised off thread
    if (!m_dispatcherQueue)
    {
        m_dispatcherQueue = winrt::DispatcherQueue::GetForCurrentThread();
    }

    if (hasNewMaterialPolicy)
    {
        // We might have no dispatcher in XamlPresenter scenarios (currenlty LogonUI/CredUI do not appear to use Acrylic).
        // In these cases, we will honor the initial policy state but not get change notifications.
        // This matches the legacy MaterialHelper behavior and should be sufficient for the special case of login screen.
        if (m_dispatcherQueue)
        {
            m_transparencyPolicyChangedRevoker = m_materialProperties.TransparencyPolicyChanged(winrt::auto_revoke, {
                [weakThis = get_weak(), dispatcherQueue = m_dispatcherQueue] (const winrt::IMaterialProperties& sender, const winrt::IInspectable& args)
                {
                    dispatcherQueue.TryEnqueue(winrt::Windows::System::DispatcherQueueHandler([weakThis]()
                    {
                        auto target = weakThis.get();
                        if (target)
                        {
                            target->PolicyStatusChangedHelper(MaterialHelper::BrushTemplates<RevealBrush>::IsDisabledByInAppTransparencyPolicy(target.get()));
                        }
                    }));
                }
                });
        }


        m_additionalMaterialPolicyChangedToken = MaterialHelper::AdditionalPolicyChanged([this](auto sender) { OnAdditionalMaterialPolicyChanged(sender); });
    }

    m_isConnected = true;

    // Apply initial policy state
    PolicyStatusChangedHelper(MaterialHelper::BrushTemplates<RevealBrush>::IsDisabledByInAppTransparencyPolicy(this));
}
#endif

void RevealBrush::OnConnected()
{
    // XCBB will use Fallback rendering, so do not run derived Brush code.
    if (SharedHelpers::IsInDesignMode()) { return; }

#if BUILD_WINDOWS

    if (m_associatedIsland)
    {
        MaterialHelper::OnRevealBrushConnectedIsland(m_associatedIsland);
    }
    else
    {
        MaterialHelper::OnRevealBrushConnected();
    }

#else
    MaterialHelper::OnRevealBrushConnected();
#endif

    if (!m_fallbackColorChangedToken.value)
    {
        m_fallbackColorChangedToken.value = RegisterPropertyChangedCallback(
            winrt::XamlCompositionBrushBase::FallbackColorProperty(), { this, &RevealBrush::OnFallbackColorChanged });
    }

#ifndef BUILD_WINDOWS
    // Stay subscribed to NoiseChanged events when brush disconnected so that it gets marked to pick up new noise when (and if) it gets reconnected.
    if (!m_noiseChangedToken.value)
    {
        m_noiseChangedToken = MaterialHelper::NoiseChanged([this](auto sender) { OnNoiseChanged(sender); });
    }

    m_isConnected = true;
    m_materialPolicyChangedToken = MaterialHelper::PolicyChanged([this](auto sender, auto args) { OnMaterialPolicyStatusChanged(sender, args); });

    UpdateLightTargets(true /* ambientToo */);
    UpdateRevealBrush();
#endif
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

#if BUILD_WINDOWS
    if (m_associatedIsland)
    {
        MaterialHelper::OnRevealBrushDisconnectedIsland(m_associatedIsland);
    }
    else
    {
        MaterialHelper::OnRevealBrushDisconnected();
    }
#else
    MaterialHelper::OnRevealBrushDisconnected();
#endif


#if BUILD_WINDOWS
    // Brushes can't be sared between islands, and their MaterialProperties has affinity to a given island.
    // Drop MaterialProperties when Brush leaves the live tree in case it is moved to a different island.
    MUX_ASSERT(m_materialProperties);
    m_materialProperties = nullptr;
    m_associatedIsland = nullptr;
    m_isInterIsland = false;

    m_transparencyPolicyChangedRevoker.revoke();

    if (!m_isBorder)
    {
        MaterialHelper::BrushTemplates<RevealBrush>::UnhookWindowDpiChangedHandler(this);
        MaterialHelper::BrushTemplates<RevealBrush>::UnhookIslandDpiChangedHandler(this);
    }

    MaterialHelper::AdditionalPolicyChanged(m_additionalMaterialPolicyChangedToken);
    m_additionalMaterialPolicyChangedToken.value = 0;
#else
    MaterialHelper::PolicyChanged(m_materialPolicyChangedToken);
    m_materialPolicyChangedToken.value = 0;
#endif
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

#if BUILD_WINDOWS
void RevealBrush::OnAdditionalMaterialPolicyChanged(const com_ptr<MaterialHelperBase>& sender)
{
    PolicyStatusChangedHelper(MaterialHelper::BrushTemplates<RevealBrush>::IsDisabledByInAppTransparencyPolicy(this));
}
#else
void RevealBrush::OnMaterialPolicyStatusChanged(const com_ptr<MaterialHelperBase>& sender, bool isDisabledByMaterialPolicy)
{
    PolicyStatusChangedHelper(isDisabledByMaterialPolicy);
}

void RevealBrush::OnNoiseChanged(const com_ptr<MaterialHelperBase>& sender)
{
    m_noiseChanged = true;
    if (m_isConnected)
    {
        UpdateRevealBrush();
    }
}
#endif

void RevealBrush::EnsureNoiseBrush()
{
#if BUILD_WINDOWS
    if (m_noiseChanged || !m_dpiScaledNoiseBrush)
    {
        int resScaleInt = MaterialHelper::BrushTemplates<RevealBrush>::GetEffectiveDpi(this);
        m_dpiScaledNoiseBrush = MaterialHelper::GetNoiseBrush(resScaleInt);
    }
    m_noiseChanged = false;
#else
    auto noiseBrush = MaterialHelper::GetNoiseBrush();
    if (noiseBrush != m_noiseBrush)
    {
        m_noiseBrush = noiseBrush;
    }
    m_noiseChanged = false;
#endif
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
    winrt::Compositor compositor = winrt::Window::Current().Compositor();
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
#if BUILD_WINDOWS
            MUX_ASSERT(m_dpiScaledNoiseBrush);
            revealBrush.SetSourceParameter(L"Noise", m_dpiScaledNoiseBrush);
#else
            MUX_ASSERT(m_noiseBrush);
            revealBrush.SetSourceParameter(L"Noise", m_noiseBrush);
#endif
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

#ifndef BUILD_WINDOWS
    if (m_brush)
    {
        m_brush.Properties().InsertScalar(L"ShouldRenderAsFallbackInIslands", 1.0f);
    }
#endif
}

void RevealBrush::UpdateRevealBrush()
{
#ifndef BUILD_WINDOWS
    if (!MaterialHelper::RS2IsSafeToCreateNoise())
    {
        // No-op for now, we'll recreate noise (as well as the brush) on VisibilityChanged -> true event.
        MUX_ASSERT(!SharedHelpers::IsRS3OrHigher());
        return;
    }
#endif

    if (m_isConnected)
    {
        CreateRevealBrush();
    }
    CompositionBrush(m_brush);

#if BUILD_WINDOWS
    if (false /*xamlroot*/)
    {
        auto strongThis = get_strong();
        strongThis.as<winrt::IXamlCompositionBrushBasePrivates>().SetBrushForXamlRoot(nullptr /*xamlRoot*/, m_brush);
    }
#endif
}

#ifndef BUILD_WINDOWS
void RevealBrush::StopAttachingLights()
{
    MaterialHelper::SetShouldBeginAttachingLights(false);
    MaterialHelper::SetShouldContinueAttachingLights(false);
    MaterialHelper::ResetFailedToAttachLightsCount();
}
#endif

void RevealBrush::AttachLights()
{
    if (SharedHelpers::IsRS5OrHigher())
    {
        // There are some non-fatal cases where we failed to hook up lights, tracked by:
        // Bug 18893751: WUXC: Investigate test failures when MUX_ASSERT(ValidatePublicRootAncestor()) is enabled
        // Comment this assert for now and skip attaching lights. Remove this when Bug 18893751 is fixed.
        // MUX_ASSERT(ValidatePublicRootAncestor());

        RevealBrush::AttachLightsImpl();
    }
#ifndef BUILD_WINDOWS
    else
    {
        // On MUX + RS4 or lower, Bug 18005612 is present. To work around, we defer hooking up the lights until we can verify the public root is reachable.
        // We expect to reach the public root after a few retries - if we still can't reach it after MaterialHelper::sc_maxAttemptsToAttachLights assert to get data on this situation.
        if (ValidatePublicRootAncestor())
        {
            RevealBrush::AttachLightsImpl();
        }
        else
        {
            MaterialHelper::SetShouldBeginAttachingLights(true);

            // Note that AttachLights is only called by the first RevealBrush::OnConnected. 
            // In case of the brush re-entering on same tick, ShouldBeginAttachingLights ensure we 
            // don't generate multiple subscriptions to CT.Rerndering
            auto renderingEventForWindowRootToken = std::make_shared<winrt::event_token>();
            *renderingEventForWindowRootToken = winrt::Xaml::Media::CompositionTarget::Rendering(
                [renderingEventForWindowRootToken](auto&, auto&) {
                    bool unsubscribeFromRenderingEvent = true;
                    if (MaterialHelper::ShouldBeginAttachingLights())
                    {
                        if (ValidatePublicRootAncestor())
                        {
                            RevealBrush::AttachLightsImpl();
                        }
                        else if (MaterialHelper::IncrementAndCheckFailedToAttachLightsCount())
                        {
                            // We've actually exceeded max failed to attach lights count - not clear if this is a valid scenario, get a record in Watson...
                            MUX_ASSERT(FALSE);
                        }
                        else
                        {
                            // Keep trying...
                            unsubscribeFromRenderingEvent = false;
                        }
                    }

                    if (unsubscribeFromRenderingEvent)
                    {
                        winrt::Xaml::Media::CompositionTarget::Rendering(*renderingEventForWindowRootToken);
                    }
                });
        }
    }
#endif
}

void RevealBrush::AttachLightsImpl()
{
    // Safe to attach RootScrollViewer lights immediately
    auto windowRoot = winrt::Window::Current().Content();
    if (!windowRoot) { return; }

    RevealBrush::AttachLightsToAncestor(windowRoot, true);

#ifndef BUILD_WINDOWS
    if (!SharedHelpers::DoesXamlMoveRSVLightToRootVisual())
    {
        MaterialHelper::SetShouldContinueAttachingLights(true);

        // Defer attaching PopupRoot until next Rendering event since this requires tree manipulations 
        // and will fail if we are called while PopupRoot is undergoing Layout
        auto renderingEventForPopupToken = std::make_shared<winrt::event_token>();
        *renderingEventForPopupToken = winrt::Xaml::Media::CompositionTarget::Rendering(
            [renderingEventForPopupToken](auto&, auto&) {
                // Detach event or Rendering will keep calling us back.
                winrt::Xaml::Media::CompositionTarget::Rendering(*renderingEventForPopupToken);

                // If we've removed all lights from the tree before we finished attaching all lights, then don't attach the rest.
                if (MaterialHelper::ShouldContinueAttachingLights())
                {
                    // Lights have been attached to the visual root, but there are still other places where the app can use
                    // reveal brushes. They need lights as well. One place is the popup root, which is the parent of the child
                    // elements of all open popups.
                    winrt::Popup popup;
                    winrt::Canvas popupChild;
                    popup.Child(popupChild);
                    auto popupOpenedToken = std::make_shared<winrt::event_token>();
                    *popupOpenedToken = popup.Opened([popup, popupChild, popupOpenedToken](auto&, auto&) {
                        // If we've removed all lights from the tree before we finished attaching all lights, then don't attach the rest.
                        if (MaterialHelper::ShouldContinueAttachingLights())
                        {
                            // Attach lights to PopupRoot.
                            RevealBrush::AttachLightsToAncestor(popupChild, true);
                        }
                        popup.IsOpen(false);
                        popup.Opened(*popupOpenedToken);
                        });

                    popup.IsOpen(true);
                }
            });
    }
#endif
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
    auto windowRoot = winrt::Window::Current().Content();

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


#if BUILD_WINDOWS
void RevealBrush::AttachLightsToIsland(const winrt::XamlIsland& island)
{
    auto lights = island.Lights();

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

        MaterialHelper::TrackRevealLightsToRemoveIsland(
            island,
            lights,
            { *borderLightTheme, *wideBorderLightTheme, *borderDarkTheme, *wideBorderDarkTheme, *ambientLight }
        );
    }
}
#endif

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
#if BUILD_WINDOWS
    return m_isInFallbackMode = m_isDisabledByMaterialPolicy || AlwaysUseFallback() || IsOnXboxAndNotMouseMode() || m_isInterIsland;
#else
    return m_isInFallbackMode = m_isDisabledByMaterialPolicy || AlwaysUseFallback() || IsOnXboxAndNotMouseMode();
#endif
}


#if BUILD_WINDOWS
void RevealBrush::OnTransparencyPolicyChanged(const winrt::IMaterialProperties& sender, const winrt::IInspectable& /*args*/)
{
    // We might have no dispatcher in XamlPresenter scenarios (currently, LogonUI may use reveal via ListView).
    // In these cases, we will honor the initial policy state but not get change notifications.
    // This matches the legacy MaterialHelper behavior and should be sufficient for the special case of login screen.
    if (m_dispatcherQueue)
    {
        com_ptr<RevealBrush> strongThis = get_strong();
        m_dispatcherQueue.TryEnqueue(
            winrt::Windows::System::DispatcherQueueHandler([strongThis, sender]()
                {
                    strongThis->PolicyStatusChangedHelper(MaterialHelper::BrushTemplates<RevealBrush>::IsDisabledByInAppTransparencyPolicy(strongThis.get()));
                }));
    }
}

void RevealBrush::OnIslandTransformChanged(const winrt::CompositionIsland& sender, const winrt::IInspectable& /*args*/)
{
    MaterialHelper::BrushTemplates<RevealBrush>::UpdateDpiScaledNoiseBrush(this);
}
#endif

void RevealBrush::OnIsContainerPropertyChanged(
    const winrt::DependencyObject& sender,
    const winrt::DependencyPropertyChangedEventArgs& args)
{
    if (auto elementSender = sender.try_as<winrt::IUIElement5>())
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

    // Choose the appropriate element for attaching the RevealHoverLights to.
    // On RS3 and higher we use the revealed element itself, however on RS2 and lower we need
    // to use its parent, to workaround an LTE-specific bug which was fixed in RS3.
    auto elementForHoverLight = SharedHelpers::IsRS3OrHigher() ? sender : winrt::VisualTreeHelper::GetParent(sender);
    if (elementForHoverLight)
    {
        if (auto uiElement = elementForHoverLight.try_as<winrt::IUIElement5>())
        {
            // If we transition to something other than Normal, see if we need to attach the hover light.
            if (targetState != winrt::RevealBrushState::Normal)
            {
                elementForHoverLight.SetValue(s_IsContainerProperty, box_value(true));
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
