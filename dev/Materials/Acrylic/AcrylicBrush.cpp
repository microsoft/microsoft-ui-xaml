// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "AcrylicBrush.h"
#include "AcrylicBrushFactory.h"
#include "ColorConversion.h"
#include "ResourceAccessor.h"
#include "SharedHelpers.h"
#include "Vector.h"
#include "RuntimeProfiler.h"
#if BUILD_WINDOWS
#include <FeatureStaging-ShellViewManagement.h>
#endif

#include "AcrylicBrush.g.cpp"

AcrylicBrush::AcrylicBrush()
{
    //  Logging usage telemetry
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_Acrylic);
}

AcrylicBrush::~AcrylicBrush()
{
#ifdef BUILD_WINDOWS
    if (m_islandTransformChangedToken.value)
    {
        m_associatedCompositionIsland.StateChanged(m_islandTransformChangedToken);
        m_islandTransformChangedToken.value = 0;
    }
#else
    if (m_noiseChangedToken.value)
    {
        MaterialHelper::NoiseChanged(m_noiseChangedToken);
        m_noiseChangedToken.value = 0;
    }
#endif
}

#if BUILD_WINDOWS
void AcrylicBrush::OnElementConnected(winrt::DependencyObject element) noexcept
{
    // XCBB will use Fallback rendering, so do not run derived Brush code.
    if (SharedHelpers::IsInDesignMode()) { return; }

    bool hasNewMaterialPolicy = false;
    winrt::XamlIsland xamlIsland = winrt::XamlIsland::GetIslandFromElement(element.try_as<winrt::UIElement>());

    if (xamlIsland)
    {
        if (xamlIsland != m_associatedIsland)
        {
            if (m_associatedIsland)
            {
                m_isInterIsland = true;
            }
            else
            {
                // First time getting MP for island
                m_associatedIsland = xamlIsland;
                winrt::IInspectable materialPropertiesInsp = xamlIsland.MaterialProperties();
                m_materialProperties = materialPropertiesInsp.try_as<winrt::MaterialProperties>();
                hasNewMaterialPolicy = true;
            }
        }

        if (m_islandTransformChangedToken.value == 0)
        {
            MaterialHelper::BrushTemplates<AcrylicBrush>::HookupIslandDpiChangedHandler(this);
        }
    }
    else
    {
        if (m_associatedIsland)
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

        MaterialHelper::BrushTemplates<AcrylicBrush>::HookupWindowDpiChangedHandler(this);
    }
    WI_ASSERT_MSG_NOASSUME(m_materialProperties, "No MaterialProperties available in AcrylicBrush::OnElementConnected");


    // DispatcherQueue needed as TransparencyPolicyChanged is raised off thread
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
            // TransparencyPolicyCHanged is called on a worker thread. To ensure correct handler execution, make sure to:
            // (1) Capture weak ref to this to ensure brush still exists when the handler is scheduled to the UI thread.
            // (2) Capture DispatcherQueue so we don't need to access the brush off-thread.
            m_transparencyPolicyChangedRevoker = m_materialProperties.TransparencyPolicyChanged(winrt::auto_revoke, {
                [weakThis = get_weak(), dispatcherQueue = m_dispatcherQueue] (const winrt::IMaterialProperties& sender, const winrt::IInspectable& args)
                {
                    dispatcherQueue.TryEnqueue(winrt::Windows::System::DispatcherQueueHandler([weakThis]()
                    {
                        auto target = weakThis.get();
                        if (target)
                        {
                            // Note HostBackdropTransparencyPolicy also incorporates the IsFullScreenOrTabletMode status tracked by legacy MaterialHelper implementation
                            target->PolicyStatusChangedHelper(
                                MaterialHelper::BrushTemplates<AcrylicBrush>::IsDisabledByInAppTransparencyPolicy(target.get()),
                                MaterialHelper::BrushTemplates<AcrylicBrush>::IsDisabledByHostBackdropTransparencyPolicy(target.get())
                                );
                        }
                    }));
                }
                });
        }

        m_additionalMaterialPolicyChangedToken = MaterialHelper::AdditionalPolicyChanged([this](auto sender) { OnAdditionalMaterialPolicyChanged(sender); });
    }

    m_isConnected = true;

    // Apply initial policy state
    PolicyStatusChangedHelper(
        MaterialHelper::BrushTemplates<AcrylicBrush>::IsDisabledByInAppTransparencyPolicy(this),
        MaterialHelper::BrushTemplates<AcrylicBrush>::IsDisabledByHostBackdropTransparencyPolicy(this)
    );
}
#endif

void AcrylicBrush::OnConnected()
{
    // XCBB will use Fallback rendering, so do not run derived Brush code.
    if (SharedHelpers::IsInDesignMode()) { return; }

    m_fallbackColorChangedToken.value = RegisterPropertyChangedCallback(
        winrt::XamlCompositionBrushBase::FallbackColorProperty(), { this, &AcrylicBrush::OnFallbackColorChanged });

#ifndef BUILD_WINDOWS
    // NOTE: This will call back the status changed callback so do it before setting isConnected = true so we don't run UpdateAcrylicStatus again.
    m_materialPolicyChangedToken = MaterialHelper::PolicyChanged([this](auto sender, auto args) { OnMaterialPolicyStatusChanged(sender, args); });

    // Stay subscribed to NoiseChanged events when brush disconnected so that it gets marked to pick up new noise when (and if) it gets reconnected.
    if (!m_noiseChangedToken.value)
    {
        m_noiseChangedToken = MaterialHelper::NoiseChanged([this](auto sender) { OnNoiseChanged(sender); });
    }

    auto currentWindow = winrt::Window::Current();
    m_currentWindow.set(currentWindow);
    m_windowActivatedToken = currentWindow.Activated({ this, &AcrylicBrush::OnCurrentWindowActivated });

    m_windowSizeChangedToken = MaterialHelper::WindowSizeChanged([this](auto sender, auto args) { OnWindowSizeChanged(sender, args); });
    UpdateWindowActivationStatus();

    m_isConnected = true;
    UpdateAcrylicStatus();
#endif
}

void AcrylicBrush::OnDisconnected()
{
    if (SharedHelpers::IsInDesignMode()) { return; }

    m_isConnected = false;
    m_isUsingAcrylicBrush = false;
    CancelFallbackAnimationCompleteWait();

    if (m_brush)
    {
        m_brush.Close();
        m_brush = nullptr;
        CompositionBrush(nullptr);
    }

#ifndef BUILD_WINDOWS
    // Release our reference on the noise, but don't close - it's managed by MaterialHelper
    if (m_noiseBrush)
    {
        m_noiseBrush = nullptr;
    }

    if (auto currentWindow = m_currentWindow.get())
    {
        currentWindow.Activated(m_windowActivatedToken);
        m_currentWindow.set(nullptr);
    }

    MaterialHelper::PolicyChanged(m_materialPolicyChangedToken);
    m_materialPolicyChangedToken.value = 0;

    MaterialHelper::WindowSizeChanged(m_windowSizeChangedToken);
    m_windowSizeChangedToken.value = 0;
#else
    // Release our reference on the DPI-scaled noise, but don't close - it's managed by MaterialHelper
    if (m_dpiScaledNoiseBrush)
    {
        m_dpiScaledNoiseBrush = nullptr;
    }

    // Brushes can't be sared between islands, and their MaterialProperties has affinity to a given island.
    // Drop MaterialProperties when Brush leaves the live tree in case it is moved to a different island.
    _ASSERT(m_materialProperties);
    m_materialProperties = nullptr;
    m_associatedIsland = nullptr;
    m_isInterIsland = false;

    m_transparencyPolicyChangedRevoker.revoke();

    MaterialHelper::AdditionalPolicyChanged(m_additionalMaterialPolicyChangedToken);
    m_additionalMaterialPolicyChangedToken.value = 0;

    MaterialHelper::BrushTemplates<AcrylicBrush>::UnhookWindowDpiChangedHandler(this);
    MaterialHelper::BrushTemplates<AcrylicBrush>::UnhookIslandDpiChangedHandler(this);
#endif

    UnregisterPropertyChangedCallback(winrt::XamlCompositionBrushBase::FallbackColorProperty(), m_fallbackColorChangedToken.value);
    m_fallbackColorChangedToken.value = 0;
}

winrt::CompositionAnimation AcrylicBrush::MakeColorAnimation(const winrt::Color& color, const winrt::TimeSpan& duration, const winrt::Compositor& compositor)
{
    auto animation = compositor.CreateColorKeyFrameAnimation();
    animation.InsertKeyFrame(1.0, color);
    animation.Duration(duration.count() == 0 ? winrt::TimeSpan::duration(1 * 10000) : duration);     // Zero duration KeyFrameAnimations not supported, use 1ms duration in that case.
    return animation;
}

winrt::CompositionAnimation AcrylicBrush::MakeFloatAnimation(
    float fromValue,
    float toValue,
    const winrt::TimeSpan& duration,
    const winrt::CompositionEasingFunction& easing,
    const winrt::Compositor& compositor)
{
    auto animation = compositor.CreateScalarKeyFrameAnimation();
    animation.InsertKeyFrame(0.0, fromValue);
    animation.InsertKeyFrame(1.0, toValue, easing);
    animation.Duration(duration);
    return animation;
}

void AcrylicBrush::PlayCrossFadeAnimation(const winrt::CompositionBrush& brush, float acrylicStart, float acrylicEnd)
{
    const auto switchDuration = 167ms;
    auto compositor = brush.Compositor();
    auto easing = compositor.CreateCubicBezierEasingFunction({ 0.5f, 0.0f }, { 0.0f, 0.9f });

    brush.StartAnimation(L"FadeInOut.Weight", MakeFloatAnimation(acrylicStart, acrylicEnd, switchDuration, easing, compositor));
}

void AcrylicBrush::OnFallbackColorChanged(const winrt::DependencyObject& /*sender*/, const winrt::DependencyProperty& /*args*/)
{
    // FallbackColor is used by AcrylicBrush to determine the solid color to use when we experience fallback due to policy.
    // If it changes dynamically and we're using the fallback brush, make sure we update it now.
    if (m_brush)
    {
        if (m_isUsingAcrylicBrush && !m_isWaitingForFallbackAnimationComplete)
        {
            // It's AcrylicBrush but not crossfading effect, then do nothing because effect doesn't includes FallbackColor.Color 
        }
        else
        {
            m_brush.StartAnimation(
                (m_isUsingAcrylicBrush || m_isWaitingForFallbackAnimationComplete) ? FallbackColorColor : L"Color",
                MakeColorAnimation(FallbackColor(), TintTransitionDuration(), m_brush.Compositor()));
        }
    }
}

void AcrylicBrush::OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    auto property = args.Property();

    if (property == s_BackgroundSourceProperty)
    {
        UpdateAcrylicBrush();
    }
    else if (property == s_TintColorProperty ||
        property == s_TintOpacityProperty ||
        property == s_TintLuminosityOpacityProperty)
    {
        const bool shouldUseOpaqueBrush = GetEffectiveTintColor().A == 255;

        // If tint transparency status changed, we need to update the effect chain
        if (m_isUsingOpaqueBrush != shouldUseOpaqueBrush)
        {
            UpdateAcrylicBrush();
        }

        if (m_brush && (m_isUsingAcrylicBrush || m_isWaitingForFallbackAnimationComplete))
        {
            if (property != s_TintLuminosityOpacityProperty)
            {
                // This only needs to update if TintColor or TintOpacity changed
                const auto newColor = GetEffectiveTintColor();
                m_brush.StartAnimation(TintColorColor, MakeColorAnimation(newColor, TintTransitionDuration(), m_brush.Compositor()));
            }

            if (SharedHelpers::Is19H1OrHigher() && !shouldUseOpaqueBrush)
            {
                // Update luminosity any time any of the three properties change
                const auto newLuminosityColor = GetEffectiveLuminosityColor();
                m_brush.StartAnimation(LuminosityColorColor, MakeColorAnimation(newLuminosityColor, TintTransitionDuration(), m_brush.Compositor()));
            }
        }

        if (property == s_TintLuminosityOpacityProperty)
        {
            //  Logging usage telemetry
            __RP_Marker_ClassMemberById(RuntimeProfiler::ProfId_Acrylic, RuntimeProfiler::ProfMemberId_Acrylic_TintLuminosityOpacity_Changed);
        }
    }
    else if (property == s_AlwaysUseFallbackProperty)
    {
        UpdateAcrylicBrush();
    }
}

winrt::Color AcrylicBrush::GetEffectiveTintColor()
{
    winrt::Color tintColor = TintColor();
    const double tintOpacity = TintOpacity();

    // Update tintColor's alpha with the combined opacity value
    // If LuminosityOpacity was specified, we don't intervene into users parameters
    if (TintLuminosityOpacity() != nullptr)
    {
        tintColor.A = static_cast<uint8_t>(round(tintColor.A * tintOpacity));
    }
    else
    {
        const double tintOpacityModifier = GetTintOpacityModifier(tintColor);
        tintColor.A = static_cast<uint8_t>(round(tintColor.A * tintOpacity * tintOpacityModifier));
    }

    return tintColor;
}

double GetTintOpacityModifier(winrt::Color tintColor)
{
    // TintOpacityModifier affects the 19H1+ Luminosity-based recipe only
    if (!SharedHelpers::Is19H1OrHigher())
    {
        return 1.0;
    }

    // This method supresses the maximum allowable tint opacity depending on the luminosity and saturation of a color by 
    // compressing the range of allowable values - for example, a user-defined value of 100% will be mapped to 45% for pure 
    // white (100% luminosity), 85% for pure black (0% luminosity), and 90% for pure gray (50% luminosity).  The intensity of 
    // the effect increases linearly as luminosity deviates from 50%.  After this effect is calculated, we cancel it out
    // linearly as saturation increases from zero.

    const double midPoint = 0.50; // Mid point of HsvV range that these calculations are based on. This is here for easy tuning.

    const double whiteMaxOpacity = 0.45; // 100% luminosity
    const double midPointMaxOpacity = 0.90; // 50% luminosity
    const double blackMaxOpacity = 0.85; // 0% luminosity

    const Rgb rgb = RgbFromColor(tintColor);
    const Hsv hsv = RgbToHsv(rgb);

    double opacityModifier = midPointMaxOpacity;

    if (hsv.v != midPoint)
    {
        // Determine maximum suppression amount
        double lowestMaxOpacity = midPointMaxOpacity;
        double maxDeviation = midPoint;

        if (hsv.v > midPoint)
        {
            lowestMaxOpacity = whiteMaxOpacity; // At white (100% hsvV)
            maxDeviation = 1 - maxDeviation;
        }
        else if (hsv.v < midPoint)
        {
            lowestMaxOpacity = blackMaxOpacity; // At black (0% hsvV)
        }

        double maxOpacitySuppression = midPointMaxOpacity - lowestMaxOpacity;

        // Determine normalized deviation from the midpoint
        const double deviation = abs(hsv.v - midPoint);
        const double normalizedDeviation = deviation / maxDeviation;

        // If we have saturation, reduce opacity suppression to allow that color to come through more
        if (hsv.s > 0)
        {
            // Dampen opacity suppression based on how much saturation there is
            maxOpacitySuppression *= std::max(1 - (hsv.s * 2), 0.0);
        }

        const double opacitySuppression = maxOpacitySuppression * normalizedDeviation;

        opacityModifier = midPointMaxOpacity - opacitySuppression;
    }

    return opacityModifier;
}

winrt::Color AcrylicBrush::GetEffectiveLuminosityColor()
{
    winrt::Color tintColor = TintColor();
    const double tintOpacity = TintOpacity();

    // Purposely leaving out tint opacity modifier here because GetLuminosityColor needs the *original* tint opacity set by the user.
    tintColor.A = static_cast<uint8_t>(round(tintColor.A * tintOpacity));

    winrt::IReference<double> luminosityOpacity = TintLuminosityOpacity();

    return GetLuminosityColor(tintColor, luminosityOpacity);
}

// The tintColor passed into this method should be the original, unmodified color created using user values for TintColor + TintOpacity
winrt::Color GetLuminosityColor(winrt::Color tintColor, winrt::IReference<double> luminosityOpacity)
{
    const Rgb rgbTintColor = RgbFromColor(tintColor);

    // If luminosity opacity is specified, just use the values as is
    if (luminosityOpacity != nullptr)
    {
        return ColorFromRgba(rgbTintColor, std::clamp(luminosityOpacity.GetDouble(), 0.0, 1.0));
    }
    else
    {
        // To create the Luminosity blend input color without luminosity opacity,
        // we're taking the TintColor input, converting to HSV, and clamping the V between these values
        const double minHsvV = 0.125;
        const double maxHsvV = 0.965;

        const Hsv hsvTintColor = RgbToHsv(rgbTintColor);

        const auto clampedHsvV = std::clamp(hsvTintColor.v, minHsvV, maxHsvV);

        const Hsv hsvLuminosityColor = Hsv(hsvTintColor.h, hsvTintColor.s, clampedHsvV);
        const Rgb rgbLuminosityColor = HsvToRgb(hsvLuminosityColor);

        // Now figure out luminosity opacity
        // Map original *tint* opacity to this range
        const double minLuminosityOpacity = 0.15;
        const double maxLuminosityOpacity = 1.03;

        const double luminosityOpacityRangeMax = maxLuminosityOpacity - minLuminosityOpacity;
        const double mappedTintOpacity = ((tintColor.A / 255.0) * luminosityOpacityRangeMax) + minLuminosityOpacity;

        // Finally, combine the luminosity opacity and the HsvV-clamped tint color
        return ColorFromRgba(rgbLuminosityColor, std::min(mappedTintOpacity, 1.0));
    }

}

void AcrylicBrush::EnsureNoiseBrush()
{
#if BUILD_WINDOWS
    if (m_noiseChanged || !m_dpiScaledNoiseBrush)
    {
        int resScaleInt = MaterialHelper::BrushTemplates<AcrylicBrush>::GetEffectiveDpi(this);
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

#if BUILD_WINDOWS
void AcrylicBrush::PolicyStatusChangedHelper(bool isDisabledByBackdropPolicy, bool isDisabledByHostBackdropPolicy)
{
    m_isDisabledByBackdropPolicy = isDisabledByBackdropPolicy;
    m_isDisabledByHostBackdropPolicy = isDisabledByHostBackdropPolicy;

    if (m_isConnected)
    {
        UpdateAcrylicStatus();
    }
}

void AcrylicBrush::OnAdditionalMaterialPolicyChanged(const com_ptr<MaterialHelperBase>& sender)
{
    PolicyStatusChangedHelper(
        MaterialHelper::BrushTemplates<AcrylicBrush>::IsDisabledByInAppTransparencyPolicy(this),
        MaterialHelper::BrushTemplates<AcrylicBrush>::IsDisabledByHostBackdropTransparencyPolicy(this)
    );
}
#else

void AcrylicBrush::PolicyStatusChangedHelper(bool isDisabledByMaterialPolicy)
{
    m_isDisabledByMaterialPolicy = isDisabledByMaterialPolicy;
    if (m_isConnected)
    {
        UpdateAcrylicStatus();
    }
}
void AcrylicBrush::OnMaterialPolicyStatusChanged(const com_ptr<MaterialHelperBase>& sender, bool isDisabledByMaterialPolicy)
{
    PolicyStatusChangedHelper(isDisabledByMaterialPolicy);
}

void AcrylicBrush::OnWindowSizeChanged(const com_ptr<MaterialHelperBase>& sender, bool isFullScreenOrTabletMode)
{
    m_isFullScreenOrTabletMode = isFullScreenOrTabletMode;
    UpdateAcrylicBrush();
}

void AcrylicBrush::OnCurrentWindowActivated(const winrt::IInspectable& sender, const winrt::WindowActivatedEventArgs& args)
{
    bool isActivated = false;

    if (SharedHelpers::IsCoreWindowActivationModeAvailable())
    {
        winrt::CoreWindow coreWindow = sender.try_as<winrt::Window>().CoreWindow();
        isActivated = IsWindowActive(coreWindow);
    }
    else
    {
        isActivated = args.WindowActivationState() != winrt::CoreWindowActivationState::Deactivated;
    }

    if (m_isActivated != isActivated)
    {
        m_isActivated = isActivated;
        UpdateAcrylicBrush();
    }
}

bool AcrylicBrush::IsWindowActive(const winrt::CoreWindow& coreWindow)
{
    const winrt::CoreWindowActivationMode activationMode = coreWindow.ActivationMode();

    return activationMode == winrt::CoreWindowActivationMode::ActivatedNotForeground ||
        activationMode == winrt::CoreWindowActivationMode::ActivatedInForeground;
}

void AcrylicBrush::UpdateWindowActivationStatus()
{
    // With the new RS3 API CoreWindow.ActivationMode we can set the right activation in case brush is connected while inactive (Bug 11657917).
    // Unfortunately we can't fix this for RS2 or older RS3 builds. 
    if (SharedHelpers::IsCoreWindowActivationModeAvailable())
    {
        auto coreWindow = winrt::Window::Current().CoreWindow();
        MUX_ASSERT_MSG(coreWindow, "Expecting to have a CoreWindow instance when AcrylicBrush is connected.");

        if (coreWindow)
        {
            m_isActivated = IsWindowActive(coreWindow);
        }
    }
}
#endif

void AcrylicBrush::OnNoiseChanged(const com_ptr<MaterialHelperBase>& sender)
{
    m_noiseChanged = true;
    if (m_isConnected)
    {
        UpdateAcrylicStatus();
    }
}

void AcrylicBrush::UpdateAcrylicStatus()
{
#ifndef BUILD_WINDOWS
    m_isFullScreenOrTabletMode = MaterialHelper::IsFullScreenOrTabletMode();
#endif
    UpdateAcrylicBrush();
}

winrt::CompositionEffectBrush AcrylicBrush::CreateAcrylicBrushWorker(
    const winrt::Compositor& compositor,
    bool useWindowAcrylic,
    bool useCrossFadeEffect,
    winrt::Color initialTintColor,
    winrt::Color initialLuminosityColor,
    winrt::Color initialFallbackColor,
    bool shouldBrushBeOpaque,
    bool useCache)
{
    auto effectFactory = GetOrCreateAcrylicBrushCompositionEffectFactory(
        compositor, shouldBrushBeOpaque, useWindowAcrylic, useCrossFadeEffect,
        initialTintColor, initialLuminosityColor, initialFallbackColor, useCache);

    // Create the Comp effect Brush
    winrt::CompositionEffectBrush acrylicBrush = effectFactory.CreateBrush();

    // Set the backdrop source
    if (!shouldBrushBeOpaque)
    {
        if (useWindowAcrylic)
        {
            auto hostBackdropBrush = compositor.CreateHostBackdropBrush();
            acrylicBrush.SetSourceParameter(L"Backdrop", hostBackdropBrush);
        }
        else
        {
            auto backdropBrush = compositor.CreateBackdropBrush();
            acrylicBrush.SetSourceParameter(L"Backdrop", backdropBrush);
        }
    }

#ifndef BUILD_WINDOWS
    if (acrylicBrush)
    {
        acrylicBrush.Properties().InsertScalar(L"ShouldRenderAsFallbackInIslands", 1.0f);
    }
#endif

    return acrylicBrush;
}

winrt::IGraphicsEffect AcrylicBrush::CombineNoiseWithTintEffect_Legacy(
    const winrt::IGraphicsEffectSource& blurredSource,
    const winrt::Microsoft::UI::Private::Composition::Effects::ColorSourceEffect& tintColorEffect)
{
    // Apply saturation
    auto saturationEffect = winrt::make_self<Microsoft::UI::Private::Composition::Effects::SaturationEffect>();
    saturationEffect->Name(L"Saturation");
    saturationEffect->Saturation(sc_saturation);
    saturationEffect->Source(blurredSource);

    // Apply exclusion:
    // Exclusion Color
    auto exclusionColorEffect = winrt::make_self<Microsoft::UI::Private::Composition::Effects::ColorSourceEffect>();
    exclusionColorEffect->Name(L"ExclusionColor");
    exclusionColorEffect->Color(sc_exclusionColor);
    // Exclusion blend
    auto blendEffectInner = winrt::make_self<Microsoft::UI::Private::Composition::Effects::BlendEffect>();
    blendEffectInner->Mode(winrt::BlendEffectMode::Exclusion);
    blendEffectInner->Foreground(*exclusionColorEffect);
    blendEffectInner->Background(*saturationEffect);

    // Apply tint
    auto compositeEffect = winrt::make_self<Microsoft::UI::Private::Composition::Effects::CompositeStepEffect>();
    compositeEffect->Mode(winrt::CanvasComposite::SourceOver);
    compositeEffect->Destination(*blendEffectInner);
    compositeEffect->Source(tintColorEffect);

    return *compositeEffect;
}

winrt::IGraphicsEffect AcrylicBrush::CombineNoiseWithTintEffect_Luminosity(
    const winrt::IGraphicsEffectSource& blurredSource,
    const winrt::Microsoft::UI::Private::Composition::Effects::ColorSourceEffect& tintColorEffect,
    const winrt::Color initialLuminosityColor,
    std::vector<winrt::hstring>& animatedProperties
)
{
    animatedProperties.push_back(winrt::hstring(LuminosityColorColor));

    // Apply luminosity:

    // Luminosity Color
    auto luminosityColorEffect = winrt::make_self<Microsoft::UI::Private::Composition::Effects::ColorSourceEffect>();
    luminosityColorEffect->Name(L"LuminosityColor");
    luminosityColorEffect->Color(initialLuminosityColor);

    // Luminosity blend
    auto luminosityBlendEffect = winrt::make_self<Microsoft::UI::Private::Composition::Effects::BlendEffect>();
    // NOTE: There is currently a bug where the names of BlendEffectMode::Luminosity and BlendEffectMode::Color are flipped.
    // This should be changed to Luminosity when/if the bug is fixed.
    luminosityBlendEffect->Mode(winrt::BlendEffectMode::Color);
    luminosityBlendEffect->Background(blurredSource);
    luminosityBlendEffect->Foreground(*luminosityColorEffect);

    // Apply tint:

    // Color blend
    auto colorBlendEffect = winrt::make_self<Microsoft::UI::Private::Composition::Effects::BlendEffect>();
    // NOTE: There is currently a bug where the names of BlendEffectMode::Luminosity and BlendEffectMode::Color are flipped.
    // This should be changed to Color when/if the bug is fixed.
    colorBlendEffect->Mode(winrt::BlendEffectMode::Luminosity);
    colorBlendEffect->Background(*luminosityBlendEffect);
    colorBlendEffect->Foreground(tintColorEffect);

    return *colorBlendEffect;
}

// ******************************About the Luminosity-based Acrylic Recipe *****************************
//
// In 19H1, the Acryic recipe was altered to improve integration of Acrylic surfaces with Shadows using 
// a Luminosity effect. Without this a ThemeShadow cast by an Acrylic surface was visible through it 
// as a dark blur, resulting in a muddied appearence that did not match the Acrylic exppectations.
// A Luminosity effect is now used to reduce contrast in the Acrylic source and minimize the 
// shadow's contribution to Acrylic output. See comment on Luminosity in recipe description below for details.
//
// Since ThemeShadow is only present in 19H1+ OS, the new recipe is only needed when when targeting this.
// In addition, RS2 did not have the needed Luminosity blend mode, so the legacy acrylic path needed to be 
// maintained. For consistency, keep the legacy recipe afor all MUX downlevel configs (i.e. targeting RS5 and lower).
//
// CreateAcrylicBrushCompositionEffectFactory() is structured to avoid duplicated code, however 
// the comment below represents the original and new recipe in full as this is more clear.
//
// For perf reason, CrossFadeEffect is only used in animation transition between Acrylic<->Fallback
//
// *************************** Shadow-friendly Luminosity-based Recipe (19H1+) ***************************
//
//  <CrossFadeEffect>           <!-- Animates between fallback color and acrylic -->
//
//      <ColorSourceEffect />   <!-- Fallback color -->
//
//      <CompositeEffect>           <!-- Provides noise for acrylic -->
//
//          <OpacityEffect>     <!-- Noise texture with wrap and alpha -->
//              <BorderEffect>
//                  <Noise texture in a brush />
//              </BorderEffect>
//          </OpacityEffect>
//
//        <!-- There's an option here. It's either a tinted backdrop: -->
//          <BlendEffect (Color)>       <!-- Tint -->
//
//              <ColorSourceEffect />   <!-- Tint color -->
//
//              <BlendEffect (Luminosity)>               <!-- Luminosity:  The luminosity blend effect takes the color (Hue and Saturation) values from our blurred
//                                                            backdrop, but gets the lightness (Luminosity) value from a solid color layer. The color
//                                                            we use is made up of the RGB values of the Tint color, and the A value is set to "luminosity opacity".
//                                                            The purpose is to reduce contrast - allowing colors to come through, but blocking sharp differences
//                                                            in brightness (such as those caused by shadows).                    
//                                                            More on luminosity blend here:
//                                                            https://www.photoshopessentials.com/photo-editing/layer-blend-modes/luminosity/ -->
//                  <ColorSourceEffect />   <!-- Luminosity color -->
//
//                <!-- There's another option here. It's either a blurred backdrop: -->
//                  <Blur>
//                      <Backdrop in a brush/>
//                  </Blur>
//                <!-- ...or a pre-blurred backdrop was provided by the shell, which we use directly: -->
//                  <Backdrop in a brush/>
//
//              </BlendEffect (Luminosity)>
//
//          </BlendEffect (Color)>
//
//        <!-- ...or it's just a solid tint color: -->
//          <ColorSourceEffect />   <!-- Tint color -->
//
//      </CompositeEffect>
//
//  </CrossFadeEffect>
//
//
//
// ********************************** Legacy Recipe (MUX / RS5 and Lower ) **********************************
//
//  <CrossFadeEffect>           <!-- Animates between fallback color and acrylic -->
//
//      <ColorSourceEffect />   <!-- Fallback color -->
//
//      <BlendEffect>           <!-- Provides noise for acrylic -->
//
//          <OpacityEffect>     <!-- Noise texture with wrap and alpha -->
//              <BorderEffect>
//                  <Noise texture in a brush />
//              </BorderEffect>
//          </OpacityEffect>
//
//        <!-- There's an option here. It's either a tinted backdrop: -->
//          <CompositeStepEffect>       <!-- Tint -->
//              <ColorSourceEffect />   <!-- Tint color -->
//              <BlendEffect>               <!-- Exclusion -->
//                  <ColorSourceEffect />   <!-- Exclusion color -->
//                  <Saturation>
//                    <!-- There's another option here. It's either a blurred backdrop: -->
//                      <Blur>
//                          <Backdrop in a brush/>
//                      </Blur>
//                    <!-- ...or a pre-blurred backdrop was provided by the shell, which we use directly: -->
//                      <Backdrop in a brush/>
//                  </Saturation>
//              </BlendEffect>
//          </CompositeStepEffect>
//
//        <!-- ...or it's just a solid tint color: -->
//          <ColorSourceEffect />   <!-- Tint color -->
//
//      </BlendEffect>
//
//  </CrossFadeEffect>
winrt::CompositionEffectFactory AcrylicBrush::CreateAcrylicBrushCompositionEffectFactory(
    const winrt::Compositor& compositor,
    bool shouldBrushBeOpaque,
    bool useWindowAcrylic,
    bool useCrossFadeEffect,
    winrt::Color initialTintColor,
    winrt::Color initialLuminosityColor,
    winrt::Color initialFallbackColor)
{
    winrt::CompositionEffectFactory effectFactory{ nullptr };

    // The part of the effect graph below the noise layer. This is either a semi-transparent tint (common) or an opaque tint (uncommon).
    // Opaque tint may be used by apps wishing add the complexity of noise to their brand color, for example.
    winrt::IGraphicsEffect tintOutput;

    // Tint Color - either used directly or in a Color blend over a blurred backdrop
    auto tintColorEffect = winrt::make_self<Microsoft::UI::Private::Composition::Effects::ColorSourceEffect>();
    tintColorEffect->Name(L"TintColor");
    tintColorEffect->Color(initialTintColor);

    std::vector<winrt::hstring> animatedProperties;
    animatedProperties.push_back(winrt::hstring{ TintColorColor });

    if (shouldBrushBeOpaque)
    {
        tintOutput = *tintColorEffect;
    }
    else
    {
        // Load the backdrop in a brush
        winrt::CompositionEffectSourceParameter backdropEffectSourceParameter{ L"Backdrop" };

        // Get a blurred backdrop...
        winrt::IGraphicsEffectSource blurredSource;
        if (useWindowAcrylic)
        {
            // ...either the shell baked the blur into the backdrop brush, and we use it directly...
            blurredSource = backdropEffectSourceParameter;
        }
        else
        {
            // ...or we apply the blur ourselves
            auto gaussianBlurEffect = winrt::make_self<Microsoft::UI::Private::Composition::Effects::GaussianBlurEffect>();
            gaussianBlurEffect->Name(L"Blur");
            gaussianBlurEffect->BorderMode(winrt::EffectBorderMode::Hard);
            gaussianBlurEffect->BlurAmount(sc_blurRadius);
            gaussianBlurEffect->Source(backdropEffectSourceParameter);
            blurredSource = *gaussianBlurEffect;
        }

        tintOutput = SharedHelpers::Is19H1OrHigher() ?
            CombineNoiseWithTintEffect_Luminosity(blurredSource, *tintColorEffect, initialLuminosityColor, animatedProperties) :
            CombineNoiseWithTintEffect_Legacy(blurredSource, *tintColorEffect);
    }

    // Create noise with alpha and wrap:
    // Noise image BorderEffect (infinitely tiles noise image)
    auto noiseBorderEffect = winrt::make_self<Microsoft::UI::Private::Composition::Effects::BorderEffect>();
    noiseBorderEffect->ExtendX(winrt::CanvasEdgeBehavior::Wrap);
    noiseBorderEffect->ExtendY(winrt::CanvasEdgeBehavior::Wrap);
    winrt::CompositionEffectSourceParameter noiseEffectSourceParameter{ L"Noise" };
    noiseBorderEffect->Source(noiseEffectSourceParameter);
    // OpacityEffect applied to wrapped noise
    auto noiseOpacityEffect = winrt::make_self<Microsoft::UI::Private::Composition::Effects::OpacityEffect>();
    noiseOpacityEffect->Name(L"NoiseOpacity");
    noiseOpacityEffect->Opacity(sc_noiseOpacity);
    noiseOpacityEffect->Source(*noiseBorderEffect);

    // Blend noise on top of tint
    auto blendEffectOuter = winrt::make_self<Microsoft::UI::Private::Composition::Effects::CompositeStepEffect>();
    blendEffectOuter->Mode(winrt::CanvasComposite::SourceOver);
    blendEffectOuter->Destination(tintOutput);
    blendEffectOuter->Source(*noiseOpacityEffect);

    if (useCrossFadeEffect)
    {
        // Fallback color
        auto fallbackColorEffect = winrt::make_self<Microsoft::UI::Private::Composition::Effects::ColorSourceEffect>();
        fallbackColorEffect->Name(L"FallbackColor");
        fallbackColorEffect->Color(initialFallbackColor);

        // CrossFade with the fallback color. Weight = 0 means full fallback, 1 means full acrylic.
        auto fadeInOutEffect = winrt::make_self<Microsoft::UI::Private::Composition::Effects::CrossFadeEffect>();
        fadeInOutEffect->Name(L"FadeInOut");
        fadeInOutEffect->Source1(*fallbackColorEffect);
        fadeInOutEffect->Source2(*blendEffectOuter);
        fadeInOutEffect->Weight(1.0f);

        animatedProperties.push_back(winrt::hstring{ FallbackColorColor });
        animatedProperties.push_back(L"FadeInOut.Weight");
        effectFactory = compositor.CreateEffectFactory(*fadeInOutEffect, animatedProperties);
    }
    else
    {
        effectFactory = compositor.CreateEffectFactory(*blendEffectOuter, animatedProperties);
    }

    return effectFactory;
}

winrt::CompositionEffectFactory AcrylicBrush::GetOrCreateAcrylicBrushCompositionEffectFactory(
    const winrt::Compositor& compositor,
    bool shouldBrushBeOpaque,
    bool useWindowAcrylic,
    bool useCrossFadeEffect,
    winrt::Color initialTintColor,
    winrt::Color initialLuminosityColor,
    winrt::Color initialFallbackColor,
    bool useCache)
{
    return MaterialHelper::GetOrCreateAcrylicBrushCompositionEffectFactoryFromCache(
        compositor,
        shouldBrushBeOpaque,
        useWindowAcrylic,
        useCrossFadeEffect,
        useCache,
        [&compositor, shouldBrushBeOpaque, useWindowAcrylic,
        useCrossFadeEffect, initialTintColor, initialLuminosityColor,
        initialFallbackColor]() {
            return CreateAcrylicBrushCompositionEffectFactory(
                compositor,
                shouldBrushBeOpaque,
                useWindowAcrylic,
                useCrossFadeEffect,
                initialTintColor,
                initialLuminosityColor,
                initialFallbackColor); }
    );
}

void AcrylicBrush::CreateAcrylicBrush(bool useCrossFadeEffect, bool forceCreateAcrylicBrush)
{
    // Forget about any pending animation state when recreating the brush.
    CancelFallbackAnimationCompleteWait();

    const auto compositor = winrt::Window::Current().Compositor();

    const auto fallbackColor = FallbackColor();
    //if forceCreateAcrylicBrush=true, m_isUsingAcrylicBrush is ignored.
    if (forceCreateAcrylicBrush || m_isUsingAcrylicBrush)
    {
        EnsureNoiseBrush();

        const winrt::Color tintColor = GetEffectiveTintColor();
        const winrt::Color luminosityColor = GetEffectiveLuminosityColor();

        m_isUsingOpaqueBrush = tintColor.A == 255;

        // use cache for AcrylicBrushEffectFactory
        const auto acrylicBrush = CreateAcrylicBrushWorker(
            compositor,
            m_isUsingWindowAcrylic,
            useCrossFadeEffect,
            tintColor,
            luminosityColor,
            fallbackColor,
            m_isUsingOpaqueBrush,
            true /* useCache */);


        // Set noise image source
#if BUILD_WINDOWS
        MUX_ASSERT(m_dpiScaledNoiseBrush);
        acrylicBrush.SetSourceParameter(L"Noise", m_dpiScaledNoiseBrush);
#else
        MUX_ASSERT(m_noiseBrush);
        acrylicBrush.SetSourceParameter(L"Noise", m_noiseBrush);
#endif

        acrylicBrush.Properties().InsertColor(TintColorColor, tintColor);

        if (SharedHelpers::Is19H1OrHigher() && !m_isUsingOpaqueBrush)
        {
            acrylicBrush.Properties().InsertColor(LuminosityColorColor, luminosityColor);
        }

        if (useCrossFadeEffect)
        {
            acrylicBrush.Properties().InsertColor(FallbackColorColor, fallbackColor);
        }

        // Update the AcrylicBrush
        m_brush = acrylicBrush;
    }
    else
    {
        m_brush = compositor.CreateColorBrush(fallbackColor);
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

void AcrylicBrush::UpdateAcrylicBrush()
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
        // 
        // In-app acrylic should be enabled unless materials are disabled.
        // The spec for window acrylic is that it should be disabled on desktop when:
        // * The window is full screen.
        // * The system is in tablet mode.
        // * On Xbox (no special check as IsFullScreen is true in this case)
        // * On Phone primary display (no special check as IsTabletMode is true in this case)

        bool isUsingAcrylicBrush = true;
        const bool alwaysUseFallback = AlwaysUseFallback();
        const bool isUsingWindowAcrylic = BackgroundSource() == winrt::AcrylicBackgroundSource::HostBackdrop;
        const bool shouldUseOpaqueBrush = GetEffectiveTintColor().A == 255;

#if BUILD_WINDOWS
        // TODO_FluentIslands: For now the IgnoreAreEffectsFast test hook will override all MaterialProperties policy 
        //                     and enable fluent effects, since MP aggregates all policy compoenents and does not allow 
        //                     us to ignore AreEffectsFast piece only.
        const bool isDisabledByMaterialPropertiesPolicy = (!isUsingWindowAcrylic && m_isDisabledByBackdropPolicy) ||
            (isUsingWindowAcrylic && m_isDisabledByHostBackdropPolicy);

        if (isDisabledByMaterialPropertiesPolicy || alwaysUseFallback || m_isInterIsland)
#else
        if (m_isDisabledByMaterialPolicy ||
            (isUsingWindowAcrylic && (m_isFullScreenOrTabletMode || !m_isActivated)) ||
            alwaysUseFallback)
#endif
        {
            isUsingAcrylicBrush = false;
        }

        // Covers cases where we need a new brush (no animations)
        if (!m_brush ||                                         // Create brush for the first time
            m_noiseChanged ||                                   // Recreate brush with new noise
            (m_isUsingOpaqueBrush != shouldUseOpaqueBrush) ||   // Recreate the brush with (or without) the opaque tint optimization
            (m_isUsingWindowAcrylic != isUsingWindowAcrylic))   // Recreate brush with new type of transparency (Backdrop vs HostBackdrop)
        {
            m_isUsingWindowAcrylic = isUsingWindowAcrylic;
            m_isUsingAcrylicBrush = isUsingAcrylicBrush;

            CreateAcrylicBrush(false /* useCrossFadeEffect */);
        }
        // Covers cases were we switch between fallback and acrylic (needs animations)
        else
        {
            // Same source type, see if we need to switch to/from acrylic brush.
            if (m_isUsingAcrylicBrush != isUsingAcrylicBrush)
            {
                // When we're all done make sure that we recreate the brush to be just a solid color brush.
                auto strongThis = get_strong();

                if (m_isWaitingForFallbackAnimationComplete)
                {
                    CancelFallbackAnimationCompleteWait();
                }

                // After cancel animation, AcrylicBrush doesn't have crossfading effects
                // So we make a new AcrylicBrush with crosssfading effects anyway.
                CreateAcrylicBrush(true /* useCrossFadeEffect */, true /* forceCreateAcrylicBrush */);

                m_isUsingAcrylicBrush = isUsingAcrylicBrush;

                m_isWaitingForFallbackAnimationComplete = true;

                float acrylicStart = 0.0f;
                float acrylicEnd = 0.0f;

                if (m_isUsingAcrylicBrush)
                {
                    // Fallback -> Acrylic Transition. Create the Acrlyic Brush and animate it from fallback color to acrylic effect.
                    acrylicEnd = 1.0f;
                }
                else
                {
                    // Acrylic -> Fallback transition. Animate acrylic to the fallback color, then create the fallback brush.
                    acrylicStart = 1.0f;
                }

                CreateAnimation(m_brush,
                    m_waitingForFallbackAnimationCompleteBatch,
                    m_waitingForFallbackAnimationCompleteToken,
                    acrylicStart,
                    acrylicEnd,
                    [strongThis](auto sender, auto args)
                    {
                        strongThis->CancelFallbackAnimationCompleteWait();

                        // When the animation completes, go create the non crossfading acrylic brush.
                        strongThis->CreateAcrylicBrush(false /* useCrossFadeEffect */);
                    });
            }
        }
    }
}

void AcrylicBrush::CreateAnimation(
    const winrt::CompositionBrush& brush,
    winrt::CompositionScopedBatch& scopedBatch,
    winrt::event_token& token,
    float acrylicStart,
    float acrylicEnd,
    const winrt::TypedEventHandler<winrt::IInspectable, winrt::CompositionBatchCompletedEventArgs>& handler)
{
    const auto newScopedBatch = brush.Compositor().CreateScopedBatch(winrt::CompositionBatchTypes::Animation);
    PlayCrossFadeAnimation(brush, acrylicStart, acrylicEnd);
    newScopedBatch.End();

    scopedBatch = newScopedBatch;
    token = newScopedBatch.Completed(handler);
}

void AcrylicBrush::CancelFallbackAnimationCompleteWait()
{
    m_isWaitingForFallbackAnimationComplete = false;

    if (m_waitingForFallbackAnimationCompleteBatch)
    {
        m_waitingForFallbackAnimationCompleteBatch.Completed(m_waitingForFallbackAnimationCompleteToken);
        m_waitingForFallbackAnimationCompleteBatch = nullptr;
        m_waitingForFallbackAnimationCompleteToken = {};
    }
}

void AcrylicBrush::CoerceToZeroOneRange(double& value)
{
    if (!isnan(value))
    {
        value = std::clamp(value, 0.0, 1.0);
    }
}

void AcrylicBrush::CoerceToZeroOneRange_Nullable(winrt::IReference<double>& value)
{
    if (value && !isnan(value.Value()))
    {
        value = std::clamp(value.Value(), 0.0, 1.0);
    }
}

#if BUILD_WINDOWS
// If plateau scale changed (eg by moving between different res screens in multimon),
// reload the noise surface to prevent noise from being scaled
void AcrylicBrush::OnIslandTransformChanged(const winrt::CompositionIsland& sender, const winrt::IInspectable& /*args*/)
{
    MaterialHelper::BrushTemplates<AcrylicBrush>::UpdateDpiScaledNoiseBrush(this);
}
#endif
