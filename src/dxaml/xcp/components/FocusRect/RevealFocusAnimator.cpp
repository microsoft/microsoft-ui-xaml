// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "RevealFocusAnimator.h"
#include "wil\resource.h"
#include <string>
#include "RevealFocusSource.h"
#include "corep.h"
#include "WindowRenderTarget.h"
#include "DCompTreeHost.h"
#include "WucBrushManager.h"
#include "DoPointerCast.h"
#include "LoadedImageSurface.h"
#include <Pathcch.h>
#include <wil\resource.h>
#include <windows.ui.composition.interop.h>
#include "DCompSurface.h"
#include <hw/effects.h>
#include "D2DUtils.h"
#include "uielement.h"
#include "XStringBuilder.h"
#include "DoubleUtil.h"
#include "RevealFocusDefaultValue.h"
#include <windows.system.power.h>
#include "RevealMotion.h"
#include "focusmgr.h"
#include "Button.h"
#include "DXamlServices.h"

using namespace DirectUI;
using namespace RevealFocus;
using namespace FocusRect;

using wrl_wrappers::HStringReference;

template <typename AnimationInterface>
void StartAnimation(
    _In_ WUComp::ICompositionObject * compositionObject,
    _In_ AnimationInterface * animation,
    _In_ const xstring_ptr_view& propertyName)
{
    HStringReference property(propertyName.GetBuffer(), propertyName.GetCount());

    wrl::ComPtr<WUComp::ICompositionAnimation2> compAnimation2;
    IFCFAILFAST(animation->QueryInterface(IID_PPV_ARGS(&compAnimation2)));
    IFCFAILFAST(compAnimation2->put_Target(property.Get()));

    wrl::ComPtr<WUComp::ICompositionAnimation> compAnimation;
    IFCFAILFAST(animation->QueryInterface(IID_PPV_ARGS(&compAnimation)));
    IFCFAILFAST(compositionObject->StartAnimation(property.Get(), compAnimation.Get()));
}

RevealFocusAnimator::~RevealFocusAnimator()
{
    wrl::ComPtr<wsyp::IPowerManagerStatics> powerManager;
    IGNOREHR(wf::GetActivationFactory(HStringReference(RuntimeClass_Windows_System_Power_PowerManager).Get(), &powerManager));

    if (powerManager)
    {
        if (m_powerSupplyToken.value != 0)
        {
            IGNOREHR(powerManager->remove_PowerSupplyStatusChanged(m_powerSupplyToken));
        }

        if (m_energySaverToken.value != 0)
        {
            IGNOREHR(powerManager->remove_EnergySaverStatusChanged(m_energySaverToken));
        }
    }
}

void RevealFocusAnimator::UpdateVisualSize(
    _In_ WUComp::ICompositionObject* hostVisual,
    _In_ WUComp::ISpriteVisual* spriteVisual,
    _In_ const RevealFocusSource& source) const
{
    wrl::ComPtr<WUComp::ICompositionObject> spriteVisualAsComp;
    IFCFAILFAST(spriteVisual->QueryInterface(IID_PPV_ARGS(&spriteVisualAsComp)));
    const auto sizeAdjustment = source.GetSizeAdjustment();
    ApplyScalarExpressionAnimation(
        m_compositor.Get(),
        spriteVisualAsComp.Get(),
        XSTRING_PTR_EPHEMERAL(L"hostVisual"),
        hostVisual,
        XSTRING_PTR_EPHEMERAL(L"colorGlowSizeXAdjustment"),
        sizeAdjustment.X,
        XSTRING_PTR_EPHEMERAL(L"Size.x"));

    ApplyScalarExpressionAnimation(
        m_compositor.Get(),
        spriteVisualAsComp.Get(),
        XSTRING_PTR_EPHEMERAL(L"hostVisual"),
        hostVisual,
        XSTRING_PTR_EPHEMERAL(L"colorGlowSizeYAdjustment"),
        sizeAdjustment.Y,
        XSTRING_PTR_EPHEMERAL(L"Size.y"));
}

void RevealFocusAnimator::UpdateVisualOffset(
    _In_ WUComp::ICompositionObject* hostVisual,
    _In_ WUComp::ISpriteVisual* spriteVisual,
    _In_ const RevealFocusSource& source) const
{
    wrl::ComPtr<WUComp::ICompositionObject> spriteVisualAsComp;
    IFCFAILFAST(spriteVisual->QueryInterface(IID_PPV_ARGS(&spriteVisualAsComp)));

    const auto offsetAdjustment = source.GetOffsetAdjustment();
    ApplyScalarExpressionAnimation(
        m_compositor.Get(),
        spriteVisualAsComp.Get(),
        XSTRING_PTR_EPHEMERAL(L"hostVisual"),
        hostVisual,
        XSTRING_PTR_EPHEMERAL(L"colorGlowOffsetXAdjustment"),
        offsetAdjustment.X,
        XSTRING_PTR_EPHEMERAL(L"Offset.x"));

    ApplyScalarExpressionAnimation(
        m_compositor.Get(),
        spriteVisualAsComp.Get(),
        XSTRING_PTR_EPHEMERAL(L"hostVisual"),
        hostVisual,
        XSTRING_PTR_EPHEMERAL(L"colorGlowOffsetYAdjustment"),
        offsetAdjustment.Y,
        XSTRING_PTR_EPHEMERAL(L"Offset.y"));
}

// Animate the offset of the spotlight based on the direction focus is traveling. The intensity of the inner/outer cone is also
// animated. This gives the appearance that the light is moving across the element.
void RevealFocusAnimator::AnimateTravelingLight(
    _In_ WUComp::ICompositionObject* spriteVisual,
    _In_ const RevealFocusSource& source,
    _In_ DirectUI::FocusNavigationDirection direction) const
{
    wrl::ComPtr<WUComp::ICompositionObject> spotLight;
    if (source.TryGetLight(RevealFocusLight::TravelingSpotlight, &spotLight))
    {
        const float spotlightSize = source.GetSpotLightSize();

        const auto start = wfn::Vector3{ RevealMotion::GetStartX(direction, source), RevealMotion::GetStartY(direction, source), spotlightSize };
        const auto end = wfn::Vector3{ RevealMotion::GetDistanceX(source), RevealMotion::GetDistanceY(source), spotlightSize};

        wrl::ComPtr<WUComp::ICubicBezierEasingFunction> cubicBezier;
        IFCFAILFAST(m_easingFunctionStatics->CreateCubicBezierEasingFunction(m_compositor.Get(), wfn::Vector2{0.17f, 0.8f}, wfn::Vector2{0.33f, 0.99f}, &cubicBezier));
        wrl::ComPtr<WUComp::ICompositionEasingFunction> easingFunction;
        IFCFAILFAST(cubicBezier.As(&easingFunction));

        const std::array<Vector3KeyFrame, 2> vector3keyFrames = {
            Vector3KeyFrame(0.0f, start, easingFunction.Get()),
            Vector3KeyFrame(1.0f, end, easingFunction.Get())
        };

        auto offsetAnimation = CreateVector3KeyFrameAnimation(
            m_compositor.Get(),
            vector3keyFrames,
            source.GetSpotLightDuration(direction));

        StartAnimation(spotLight.Get(), offsetAnimation.Get(), XSTRING_PTR_EPHEMERAL(L"Offset"));

        // Animate the Inner/Outer cone intensity

         const std::array<ScalarKeyFrame, 3> intensityFrames = {
            ScalarKeyFrame(0.0f, GetDefaultValue(DefaultValue::ConeIntensityMin)),
            ScalarKeyFrame(0.2f, GetDefaultValue(DefaultValue::ConeIntensityMax)),
            ScalarKeyFrame(1.0f, GetDefaultValue(DefaultValue::ConeIntensityMin))
        };

        AnimationBehavior behavior(
            WUComp::AnimationStopBehavior_SetToFinalValue);

        const auto intensityDuration = GetIntensityDuration(source, direction);
        // Create/Start InnerConeIntensity animation
        auto innerConeIntensityAnimation = CreateScalarKeyFrameAnimation(
            m_compositor.Get(),
            intensityFrames,
            intensityDuration,
            behavior);

        StartAnimation(
            spotLight.Get(),
            innerConeIntensityAnimation.Get(),
            XSTRING_PTR_EPHEMERAL(L"InnerConeIntensity"));

        // Create/Start OuterConeIntensity animation
        auto outerConeIntensityAnimation = CreateScalarKeyFrameAnimation(
            m_compositor.Get(),
            intensityFrames,
            intensityDuration,
            behavior);

        StartAnimation(
            spotLight.Get(),
            outerConeIntensityAnimation.Get(),
            XSTRING_PTR_EPHEMERAL(L"OuterConeIntensity"));
    }
}

void RevealFocusAnimator::AnimateLights(
    _In_ WUComp::ISpriteVisual* spriteVisual,
    _In_ const RevealFocusSource& source,
    _In_ DirectUI::FocusNavigationDirection direction)
{
    const bool travelingFocusEnabled = source.IsTravelingFocusEnabled(direction);
    wrl::ComPtr<WUComp::ICompositionObject> spriteVisualAsComp;
    IFCFAILFAST(spriteVisual->QueryInterface(IID_PPV_ARGS(&spriteVisualAsComp)));
    AnimateDefaultLight(spriteVisualAsComp.Get(), source, travelingFocusEnabled);
    if (travelingFocusEnabled)
    {
        AnimateTravelingLight(spriteVisualAsComp.Get(), source, direction);
    }

    // Make sure that we get the ambient light even if we don't want to enable the breathing
    // animation. It's possible that we could get an event notifying us that we can kick off
    // the breathing animation. Keep a weakref in case we outlive the light.
    if (source.TryGetLight(RevealFocusLight::PulsingAmbient, &m_pulsingLight) && ShouldBreathe(source))
    {
        AnimatePulsingLight(m_pulsingLight);
    }
}

// Animate the default ambient light. If traveling focus is enabled, this is a short pulse with only 2 key frames to help
// see the spotlight travel better.
void RevealFocusAnimator::AnimateDefaultLight(
    _In_ WUComp::ICompositionObject* spriteVisual,
    _In_ const RevealFocusSource& source,
    _In_ bool travelingGlowEnabled)
{
    wrl::ComPtr<WUComp::ICompositionObject> defaultLight;
    const auto intensityDuration = GetAmbientLightDuration();
    AnimationBehavior behavior(WUComp::AnimationStopBehavior_LeaveCurrentValue);
    wrl::ComPtr<WUComp::IScalarKeyFrameAnimation> defaultAmbientLightAnimation;
    if (source.TryGetLight(RevealFocusLight::DefaultAmbient, &defaultLight) && travelingGlowEnabled)
    {
        const std::array<ScalarKeyFrame, 2> defaultTravelingIntensityFrames = {
            ScalarKeyFrame(0.0f, GetDefaultValue(DefaultValue::DefaultLightIntensityMax)),
            ScalarKeyFrame(1.0f, GetDefaultValue(DefaultValue::DefaultLightIntensity))
        };
        defaultAmbientLightAnimation = CreateScalarKeyFrameAnimation(
            m_compositor.Get(),
            defaultTravelingIntensityFrames,
            intensityDuration,
            behavior);
    }
    else if (defaultLight && source.IsAnimationEnabled())
    {
        const std::array<ScalarKeyFrame, 3> defaultIntensityFrames = {
            ScalarKeyFrame(0.0f, GetDefaultValue(DefaultValue::DefaultLightIntensityMin)),
            ScalarKeyFrame(0.1f, GetDefaultValue(DefaultValue::DefaultLightIntensityMax)),
            ScalarKeyFrame(1.0f, GetDefaultValue(DefaultValue::DefaultLightIntensity))
        };
        defaultAmbientLightAnimation = CreateScalarKeyFrameAnimation(
            m_compositor.Get(),
            defaultIntensityFrames,
            intensityDuration,
            behavior);
    }

    if (defaultLight && defaultAmbientLightAnimation)
    {
        StartAnimation(defaultLight.Get(), defaultAmbientLightAnimation.Get(), XSTRING_PTR_EPHEMERAL(L"Intensity"));
    }
}

void RevealFocusAnimator::StopLightAnimation(
    _In_ const RevealFocusSource& source)
{
    wrl::ComPtr<WUComp::ICompositionObject> spotLight;
    if (source.TryGetLight(RevealFocusLight::TravelingSpotlight, &spotLight))
    {
        IFCFAILFAST(spotLight->StopAnimation(HStringReference(L"Offset").Get()));
        IFCFAILFAST(spotLight->StopAnimation(HStringReference(L"InnerConeIntensity").Get()));
        IFCFAILFAST(spotLight->StopAnimation(HStringReference(L"OuterConeIntensity").Get()));
    }

    if (m_pulsingLight)
    {
        wrl::ComPtr<WUComp::ICompositionObject> ambientLightAsObj;
        IFCFAILFAST(m_pulsingLight.As(&ambientLightAsObj));
        IFCFAILFAST(ambientLightAsObj->StopAnimation(HStringReference(L"Intensity").Get()));
        m_pulsingLight.Reset();
    }

    wrl::ComPtr<WUComp::ICompositionLight3> pressLight;
    if (source.TryGetLight(RevealFocusLight::PressAmbient, &pressLight))
    {
        IFCFAILFAST(pressLight->put_IsEnabled(false));
    }
}

Microsoft::WRL::ComPtr<WUComp::ICompositionEasingFunction> RevealFocusAnimator::GetDefaultEasingFunction(
    _In_ ixp::ICompositionEasingFunctionStatics* easingFunctionStatics,
    _In_ WUComp::ICompositor* compositor)
{
    wrl::ComPtr<WUComp::ILinearEasingFunction> defaultFunction;
    IFCFAILFAST(easingFunctionStatics->CreateLinearEasingFunction(compositor, &defaultFunction));

    wrl::ComPtr<WUComp::ICompositionEasingFunction> returnFunc;
    IFCFAILFAST(defaultFunction.As(&returnFunc));
    return returnFunc;
}

void RevealFocusAnimator::ApplyScalarExpressionAnimation(
    _In_ WUComp::ICompositor* compositor,
    _In_ WUComp::ICompositionObject* animated,
    _In_ const xstring_ptr_view& referenceParamName,
    _In_ WUComp::ICompositionObject* referenceParameter,
    _In_ const xstring_ptr_view& scalarParameter,
    _In_ float scalarAdjustment,
    _In_ const xstring_ptr_view& referencePropertyName,
    _In_ const xstring_ptr_view& animatedPropertyName,
    _In_ const xstring_ptr_view& scalarModifier)
{
    XStringBuilder builder;
    IFCFAILFAST(builder.Initialize(referenceParamName.GetCount() + scalarParameter.GetCount() + referencePropertyName.GetCount() + scalarModifier.GetCount() + 1)); // 1 for '.'
    IFCFAILFAST(builder.Append(referenceParamName));
    IFCFAILFAST(builder.AppendChar(L'.'));
    IFCFAILFAST(builder.Append(referencePropertyName));
    IFCFAILFAST(builder.Append(scalarModifier));
    IFCFAILFAST(builder.Append(scalarParameter));

    xstring_ptr finalString;
    IFCFAILFAST(builder.DetachString(&finalString));

    xruntime_string_ptr finalStringRuntime;
    IFCFAILFAST(finalString.Promote(&finalStringRuntime));

    wrl::ComPtr<WUComp::IExpressionAnimation> colorGlowExpressionAnimation;
    IFCFAILFAST(compositor->CreateExpressionAnimationWithExpression(finalStringRuntime.GetHSTRING(), &colorGlowExpressionAnimation));

    wrl::ComPtr<WUComp::ICompositionAnimation> colorGlowCompositionAnimation;
    IFCFAILFAST(colorGlowExpressionAnimation.As(&colorGlowCompositionAnimation));
    IFCFAILFAST(colorGlowCompositionAnimation->SetReferenceParameter(
        HStringReference(referenceParamName.GetBuffer(), referenceParamName.GetCount()).Get(),
        referenceParameter));
    IFCFAILFAST(colorGlowCompositionAnimation->SetScalarParameter(
        HStringReference(scalarParameter.GetBuffer(), scalarParameter.GetCount()).Get(),
        scalarAdjustment));

    // If no animated property is supplied, then use the same as the host
    if (animatedPropertyName.IsNullOrEmpty())
    {
        IFCFAILFAST(animated->StartAnimation(
            HStringReference(referencePropertyName.GetBuffer(), referencePropertyName.GetCount()).Get(),
            colorGlowCompositionAnimation.Get()));
    }
    else
    {
        IFCFAILFAST(animated->StartAnimation(
            HStringReference(animatedPropertyName.GetBuffer(), animatedPropertyName.GetCount()).Get(),
            colorGlowCompositionAnimation.Get()));
    }
}

bool RevealFocusAnimator::ShouldBreathe(_In_ const RevealFocusSource& source)
{
    if (m_powerSupplyToken.value == 0)
    {
        wrl::ComPtr<wsyp::IPowerManagerStatics> powerManager;
        IFCFAILFAST(wf::GetActivationFactory(HStringReference(RuntimeClass_Windows_System_Power_PowerManager).Get(), &powerManager));

        // register PowerSupplyStatus events
        IFCFAILFAST(powerManager->add_PowerSupplyStatusChanged(wrl::Callback<wf::IEventHandler<IInspectable*>>(this,
            &RevealFocusAnimator::OnPowerStatusChanged).Get(), &m_powerSupplyToken));

        IFCFAILFAST(powerManager->add_EnergySaverStatusChanged(wrl::Callback<wf::IEventHandler<IInspectable*>>(this,
            &RevealFocusAnimator::OnPowerStatusChanged).Get(), &m_energySaverToken));

        m_breathingEnabled = source.IsAnimationEnabled() && AreLongRunningAnimationsEnabled(powerManager.Get());
    }

    return m_breathingEnabled;
}

bool RevealFocusAnimator::AreLongRunningAnimationsEnabled(_In_ wsyp::IPowerManagerStatics* powerMan)
{
    auto energySaverStatus = wsyp::EnergySaverStatus_Disabled;
    auto powerSupplyStatus = wsyp::PowerSupplyStatus_NotPresent;

    IFCFAILFAST(powerMan->get_EnergySaverStatus(&energySaverStatus));
    IFCFAILFAST(powerMan->get_PowerSupplyStatus(&powerSupplyStatus));

    // "Adequate" power supply is a funny term, it just means there is AC power and the battery (if present) is charging.
    // Disable if energy saver is on in case user wants the battery to charge faster.
    return energySaverStatus != wsyp::EnergySaverStatus_On && powerSupplyStatus == wsyp::PowerSupplyStatus_Adequate;
}

HRESULT RevealFocusAnimator::OnPowerStatusChanged(_In_ IInspectable* sender, _In_ IInspectable* args)
{
    // Both args and sender are null in this callback so they don't contain much exciting info
    UNREFERENCED_PARAMETER(sender);
    UNREFERENCED_PARAMETER(args);

    wrl::ComPtr<wsyp::IPowerManagerStatics> powerManager;
    IFCFAILFAST(wf::GetActivationFactory(HStringReference(RuntimeClass_Windows_System_Power_PowerManager).Get(), &powerManager));

    m_breathingEnabled = AreLongRunningAnimationsEnabled(powerManager.Get());

    // If animations are enabled and the light is still around, then start the animation. Otherwise, stop the animations.
    if (m_breathingEnabled && m_pulsingLight)
    {
        AnimatePulsingLight(m_pulsingLight);
    }
    else if (m_pulsingLight)
    {
        wrl::ComPtr<WUComp::ICompositionObject> ambientLightAsObj;
        IFCFAILFAST(m_pulsingLight.As(&ambientLightAsObj));
        IFCFAILFAST(ambientLightAsObj->StopAnimation(HStringReference(L"Intensity").Get()));
    }

    return S_OK;
}

// The pulsing light is a slow "pulsing" light that gradually gets more intense and then resets. It continues for as long as the
// element has keyboard focus and we are on AC power.
void RevealFocusAnimator::AnimatePulsingLight(_In_ const Microsoft::WRL::ComPtr<WUComp::IAmbientLight>& pulsingLight) const
{
    const std::array<ScalarKeyFrame, 3> intensityFrames = {
        ScalarKeyFrame(0.0f, GetDefaultValue(DefaultValue::PulsingLightIntensityMid)),
        ScalarKeyFrame(0.5f, GetDefaultValue(DefaultValue::PulsingLightIntensityHigh)),
        ScalarKeyFrame(1.0f, GetDefaultValue(DefaultValue::PulsingLightIntensityLow))
    };

    AnimationBehavior behavior(
        WUComp::AnimationDelayBehavior_SetInitialValueBeforeDelay,
        GetPulsingAnimationDelay(),
        WUComp::AnimationStopBehavior_SetToFinalValue,
        WUComp::AnimationIterationBehavior_Forever);

    auto intensityAnimation = CreateScalarKeyFrameAnimation(
        m_compositor.Get(),
        intensityFrames,
        GetPulsingAnimationDuration(),
        behavior);

    wrl::ComPtr<WUComp::ICompositionObject> pulsingLightAsObj;
    IFCFAILFAST(pulsingLight.As(&pulsingLightAsObj));
    StartAnimation(
        pulsingLightAsObj.Get(),
        intensityAnimation.Get(),
        XSTRING_PTR_EPHEMERAL(L"Intensity"));
}

void RevealFocusAnimator::ApplyAnimationBehavior(
    _In_ WUComp::IKeyFrameAnimation* animation,
    _In_ wf::TimeSpan ts,
    _In_ const AnimationBehavior& behavior)
{
    IFCFAILFAST(animation->put_Duration(ts));
    if (behavior.HasIterationBehavior)
    {
        IFCFAILFAST(animation->put_IterationBehavior(behavior.IterationBehavior));
        if (behavior.IterationBehavior == WUComp::AnimationIterationBehavior_Count)
        {
            IFCFAILFAST(animation->put_IterationCount(behavior.IterationCount));
        }
    }

    if (behavior.HasDelayBehavior)
    {
        wrl::ComPtr<WUComp::IKeyFrameAnimation3> animation3;
        IFCFAILFAST(animation->QueryInterface(IID_PPV_ARGS(&animation3)));
        IFCFAILFAST(animation3->put_DelayBehavior(behavior.DelayBehavior));
        IFCFAILFAST(animation->put_DelayTime(behavior.DelayTime));
    }

    if (behavior.HasStopBehavior)
    {
        IFCFAILFAST(animation->put_StopBehavior(behavior.StopBehavior));
    }
}

Microsoft::WRL::ComPtr<WUComp::IScalarKeyFrameAnimation> RevealFocusAnimator::CreateScalarKeyFrameAnimationWithBehavior(
    _In_ WUComp::ICompositor * compositor,
    _In_ wf::TimeSpan ts,
    _In_ const AnimationBehavior& behavior)
{
    wrl::ComPtr<WUComp::IScalarKeyFrameAnimation> scalarKeyFrame;
    IFCFAILFAST(compositor->CreateScalarKeyFrameAnimation(&scalarKeyFrame));
    wrl::ComPtr<WUComp::IKeyFrameAnimation> scalarAsKeyFrameAnimation;
    IFCFAILFAST(scalarKeyFrame.As(&scalarAsKeyFrameAnimation));
    ApplyAnimationBehavior(scalarAsKeyFrameAnimation.Get(), ts, behavior);
    return scalarKeyFrame;
}

Microsoft::WRL::ComPtr<WUComp::IVector3KeyFrameAnimation> RevealFocusAnimator::CreateVector3KeyFrameAnimationWithBehavior(
    _In_ WUComp::ICompositor * compositor,
    _In_ wf::TimeSpan ts,
    _In_ const AnimationBehavior& behavior)
{
    wrl::ComPtr<WUComp::IVector3KeyFrameAnimation> vector3KeyFrame;
    IFCFAILFAST(compositor->CreateVector3KeyFrameAnimation(&vector3KeyFrame));
    wrl::ComPtr<WUComp::IKeyFrameAnimation> keyFrameAnimation;
    IFCFAILFAST(vector3KeyFrame.As(&keyFrameAnimation));
    ApplyAnimationBehavior(keyFrameAnimation.Get(), ts, behavior);
    return vector3KeyFrame;
}

void RevealFocusAnimator::InsertKeyFrame(
    _In_ WUComp::IScalarKeyFrameAnimation * animation,
    _In_ const ScalarKeyFrame& keyFrame)
{
    IFCFAILFAST(animation->InsertKeyFrameWithEasingFunction(keyFrame.Progress, keyFrame.Value, keyFrame.EasingFunction.Get()));
}

void RevealFocusAnimator::InsertKeyFrame(
    _In_ WUComp::IVector3KeyFrameAnimation * animation,
    _In_ const Vector3KeyFrame& keyFrame)
{
    IFCFAILFAST(animation->InsertKeyFrameWithEasingFunction(keyFrame.Progress, keyFrame.Value, keyFrame.EasingFunction.Get()));
}

wf::TimeSpan RevealFocusAnimator::GetAmbientLightDuration()
{
    return  wf::TimeSpan{ static_cast<int64_t>(GetDefaultValue(DefaultValue::AmbientLightDuration))};
}

wf::TimeSpan RevealFocusAnimator::GetPulsingAnimationDuration()
{
    return  wf::TimeSpan{ static_cast<int64_t>(GetDefaultValue(DefaultValue::PulsingAnimationDuration))};
}

wf::TimeSpan RevealFocusAnimator::GetPulsingAnimationDelay()
{
    return  wf::TimeSpan{ static_cast<int64_t>(GetDefaultValue(DefaultValue::PulsingAnimationDelay))};
}

wf::TimeSpan RevealFocusAnimator::GetIntensityDuration(const RevealFocusSource& source, DirectUI::FocusNavigationDirection direction)
{
    const auto spotlightDuration = source.GetSpotLightDuration(direction);
    return wf::TimeSpan { static_cast<int64_t>(
        static_cast<float>(spotlightDuration.Duration) * GetDefaultValue(DefaultValue::SpotLightIntensityDurationFactor)
        )};
}

void RevealFocusAnimator::OnFocusedElementKeyPressed(_In_ const RevealFocusSource& source)
{
    wrl::ComPtr<WUComp::IAmbientLight> pressLight;
    if (!m_elementPressed && source.TryGetLight(RevealFocusLight::PressAmbient, &pressLight) && !!GetDefaultValue(DefaultValue::UsePressAnimations))
    {
        m_elementPressed = true;
        AnimatePressLight(pressLight, true);
    }
}

void RevealFocusAnimator::OnFocusedElementKeyReleased(_In_ const RevealFocusSource& source)
{
    wrl::ComPtr<WUComp::IAmbientLight> pressLight;
    if (m_elementPressed && source.TryGetLight(RevealFocusLight::PressAmbient, &pressLight))
    {
        AnimatePressLight(pressLight, false);
        m_elementPressed = false;
    }
}

// When an element is pressed, turn on the light and animate the light to the desired intensity. If the key is released,
// then slowly put the intensity to 0
void RevealFocusAnimator::AnimatePressLight(_In_ const Microsoft::WRL::ComPtr<WUComp::IAmbientLight>& pressLight, _In_ bool turnOn) const
{
    BOOLEAN isEnabled = false;
    wrl::ComPtr<WUComp::ICompositionLight3> compLight3;
    IFCFAILFAST(pressLight.As(&compLight3));
    IFCFAILFAST(compLight3->get_IsEnabled(&isEnabled));

    if (!isEnabled && turnOn)
    {
        // Turn on the light if we need to
        IFCFAILFAST(compLight3->put_IsEnabled(true));
    }

    const float enabledIntensity = GetDefaultValue(DefaultValue::DefaultLightIntensity);
    const float disabledIntensity = 0.0f;

    AnimationBehavior behavior(WUComp::AnimationStopBehavior_SetToFinalValue);
    const std::array<ScalarKeyFrame, 2> intensityFrames = {
        ScalarKeyFrame(0.0f, !turnOn ? enabledIntensity : disabledIntensity),
        ScalarKeyFrame(1.0f, !turnOn ? disabledIntensity : enabledIntensity)
    };

    auto intensityAnimation = CreateScalarKeyFrameAnimation(
        m_compositor.Get(),
        intensityFrames,
        wf::TimeSpan{static_cast<int64_t>(GetDefaultValue(DefaultValue::PressLightDuration))},
        behavior);

    wrl::ComPtr<WUComp::ICompositionObject> ambientLightAsObj;
    IFCFAILFAST(pressLight.As(&ambientLightAsObj));
    StartAnimation(
        ambientLightAsObj.Get(),
        intensityAnimation.Get(),
        XSTRING_PTR_EPHEMERAL(L"Intensity"));
 }

