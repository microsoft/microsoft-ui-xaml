// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CUIElement;
class xstring_ptr_view;

#include "RevealFocusSource.h"
#include <fwd/windows.ui.composition.h>
#include <fwd/windows.system.power.h>
#include "Microsoft.UI.Composition.h"

namespace DirectUI {
    enum class FocusNavigationDirection : uint8_t;
}

namespace FocusRect {

class RevealFocusAnimator final
{
public:

    RevealFocusAnimator(
        _In_ ixp::ICompositionEasingFunctionStatics* easingFunctionStatics,
        _In_ WUComp::ICompositor* compositor)
        : m_easingFunctionStatics(easingFunctionStatics)
        , m_compositor(compositor)
        , m_easingFunction(GetDefaultEasingFunction(easingFunctionStatics, compositor))
    {
    }

    ~RevealFocusAnimator();

    void UpdateVisualSize(
        _In_ WUComp::ICompositionObject* hostVisual,
        _In_ WUComp::ISpriteVisual* spriteVisual,
        _In_ const RevealFocusSource& source) const;

    void UpdateVisualOffset(
        _In_ WUComp::ICompositionObject* hostVisual,
        _In_ WUComp::ISpriteVisual* spriteVisual,
        _In_ const RevealFocusSource& source) const;

    void AnimateLights(
        _In_ WUComp::ISpriteVisual* spriteVisual,
        _In_ const RevealFocusSource& source,
        _In_ DirectUI::FocusNavigationDirection direction);

    void StopLightAnimation(
        _In_ const RevealFocusSource& source);

    void OnFocusedElementKeyPressed(_In_ const RevealFocusSource& source);
    void OnFocusedElementKeyReleased(_In_ const RevealFocusSource& source);

    HRESULT OnPowerStatusChanged(_In_ IInspectable* sender, _In_ IInspectable* args);

private:
    wrl::ComPtr<ixp::ICompositionEasingFunctionStatics> m_easingFunctionStatics;
    Microsoft::WRL::ComPtr<WUComp::ICompositor>                   m_compositor;
    Microsoft::WRL::ComPtr<WUComp::ICompositionEasingFunction>    m_easingFunction;
    Microsoft::WRL::ComPtr<WUComp::IAmbientLight>                 m_pulsingLight;
    EventRegistrationToken                                                          m_powerSupplyToken{};
    EventRegistrationToken                                                          m_energySaverToken{};
    bool                                                                            m_breathingEnabled = false;
    bool                                                                            m_elementPressed = false;

private:
    // Returns whether or not we should apply the breathing animation to the ambient light. When first called, this will register for the power
    // notifications and call AreLongRunningAnimationsEnabled. After that, the control of whether or not we should breathe is based on the notifications
    // we receive from the system so we'll just return m_breathingEnabled.
    bool ShouldBreathe(_In_ const RevealFocusSource& source);

    void AnimatePulsingLight(_In_ const Microsoft::WRL::ComPtr<WUComp::IAmbientLight>& pulsingLight) const;
    void AnimatePressLight(_In_ const Microsoft::WRL::ComPtr<WUComp::IAmbientLight>& pressLight, _In_ bool turnOn) const;
    void AnimateTravelingLight(
        _In_ WUComp::ICompositionObject* spriteVisual,
        _In_ const RevealFocusSource& source,
        _In_ DirectUI::FocusNavigationDirection direction) const;
    void AnimateDefaultLight(
        _In_ WUComp::ICompositionObject* spriteVisual,
        _In_ const RevealFocusSource& source,
        _In_ bool travelingGlowEnabled);

private:
    // Static methods
    static void ApplyScalarExpressionAnimation(
        _In_ WUComp::ICompositor* compositor,
        _In_ WUComp::ICompositionObject* animated,
        _In_ const xstring_ptr_view& referenceParamName,
        _In_ WUComp::ICompositionObject* referenceParameter,
        _In_ const xstring_ptr_view& scalarParameter,
        _In_ float scalarAdjustment,
        _In_ const xstring_ptr_view& referencePropertyName,
        _In_ const xstring_ptr_view& animatedPropertyName = xstring_ptr::NullString(),
        _In_ const xstring_ptr_view& scalarModifier = XSTRING_PTR_EPHEMERAL(L" + "));

    static Microsoft::WRL::ComPtr<WUComp::ICompositionEasingFunction> GetDefaultEasingFunction(
        _In_ ixp::ICompositionEasingFunctionStatics* easingFunctionStatics,
        _In_ ixp::ICompositor* compositor);

    template <typename Type>
    struct KeyFrame
    {
        float Progress;
        Type Value;
        Microsoft::WRL::ComPtr<WUComp::ICompositionEasingFunction> EasingFunction;

        explicit KeyFrame(float progress, Type value, _In_opt_ WUComp::ICompositionEasingFunction* func = nullptr)
            : Progress(progress)
            , Value(value)
            , EasingFunction(func)
        {
        }
    };

    using ScalarKeyFrame = KeyFrame<float>;
    using Vector3KeyFrame= KeyFrame<wfn::Vector3>;

    struct AnimationBehavior
    {
        explicit AnimationBehavior(_In_ WUComp::AnimationDelayBehavior delayBehavior, _In_ wf::TimeSpan delay)
            : HasDelayBehavior(true)
            , DelayBehavior(delayBehavior)
            , DelayTime({delay})
        {
        }

        explicit AnimationBehavior(_In_ WUComp::AnimationIterationBehavior iterationBehavior, _In_ int count = 0)
            : HasIterationBehavior(true)
            , IterationBehavior(iterationBehavior)
            , IterationCount(count)
        {
        }

        explicit AnimationBehavior(_In_ WUComp::AnimationStopBehavior stopBehavior)
            : HasStopBehavior(true)
            , StopBehavior(stopBehavior)
        {
        }

        explicit AnimationBehavior(
            _In_ WUComp::AnimationDelayBehavior delayBehavior,
            _In_ wf::TimeSpan delay,
            _In_ WUComp::AnimationIterationBehavior iterationBehavior,
            _In_ int count = 0)
            : HasDelayBehavior(true)
            , DelayBehavior(delayBehavior)
            , DelayTime(delay)
            , HasIterationBehavior(true)
            , IterationBehavior(iterationBehavior)
            , IterationCount(count)
        {
        }

        explicit AnimationBehavior(
            _In_ WUComp::AnimationDelayBehavior delayBehavior,
            _In_ wf::TimeSpan delay,
            _In_ WUComp::AnimationStopBehavior stopBehavior,
            _In_ WUComp::AnimationIterationBehavior iterationBehavior,
            _In_ int count = 0)
            : HasDelayBehavior(true)
            , DelayBehavior(delayBehavior)
            , DelayTime(delay)
            , HasIterationBehavior(true)
            , IterationBehavior(iterationBehavior)
            , IterationCount(count)
            , StopBehavior(stopBehavior)
        {
        }

        static AnimationBehavior Default()
        {
            return AnimationBehavior();
        }

        bool HasDelayBehavior = false;
        WUComp::AnimationDelayBehavior DelayBehavior;
        wf::TimeSpan DelayTime = {0};

        bool HasIterationBehavior = false;
        WUComp::AnimationIterationBehavior IterationBehavior;
        int IterationCount = 0;

        bool HasStopBehavior = false;
        WUComp::AnimationStopBehavior StopBehavior{};

    private:
        AnimationBehavior() = default;
    };

    // Methods used for creating Scalar/Vector3 KeyFrameAnimations. The behavior is optional,
    // and will opt-in to the default (which is no-delay, no iteration behavior)

    template <std::size_t Count>
    static Microsoft::WRL::ComPtr<WUComp::IScalarKeyFrameAnimation> CreateScalarKeyFrameAnimation(
        _In_ WUComp::ICompositor * compositor,
        _In_ const std::array<ScalarKeyFrame, Count>& keyFrames,
        _In_ wf::TimeSpan ts,
        _In_ const AnimationBehavior& behavior = AnimationBehavior::Default())
    {
        auto scalarKeyFrame = CreateScalarKeyFrameAnimationWithBehavior(compositor, ts, behavior);

        for (const ScalarKeyFrame& kf : keyFrames)
        {
            InsertKeyFrame(scalarKeyFrame.Get(), kf);
        }

        return scalarKeyFrame;
    }

    template <std::size_t Count>
    static Microsoft::WRL::ComPtr<WUComp::IVector3KeyFrameAnimation> CreateVector3KeyFrameAnimation(
        _In_ WUComp::ICompositor * compositor,
        _In_ const std::array<Vector3KeyFrame, Count>& keyFrames,
        _In_ wf::TimeSpan ts,
        _In_ const AnimationBehavior& behavior = AnimationBehavior::Default())
    {
        auto vector3KeyFrame = CreateVector3KeyFrameAnimationWithBehavior(compositor, ts, behavior);

        for (const Vector3KeyFrame& kf : keyFrames)
        {
            InsertKeyFrame(vector3KeyFrame.Get(), kf);
        }

        return vector3KeyFrame;
    }

    // Helper methods used by the templated CreateScalarKeyFrameAnimation and CreateVector3KeyFrameAnimation
    static Microsoft::WRL::ComPtr<WUComp::IScalarKeyFrameAnimation> CreateScalarKeyFrameAnimationWithBehavior(
        _In_ WUComp::ICompositor * compositor,
        _In_ wf::TimeSpan ts,
        _In_ const AnimationBehavior& behavior);

    static Microsoft::WRL::ComPtr<WUComp::IVector3KeyFrameAnimation> CreateVector3KeyFrameAnimationWithBehavior(
        _In_ WUComp::ICompositor * compositor,
        _In_ wf::TimeSpan ts,
        _In_ const AnimationBehavior& behavior);

    static void InsertKeyFrame(
        _In_ WUComp::IScalarKeyFrameAnimation * animation,
        _In_ const ScalarKeyFrame& keyFrame);

    static void InsertKeyFrame(
        _In_ WUComp::IVector3KeyFrameAnimation * animation,
        _In_ const Vector3KeyFrame& keyFrame);

    // Checks the state of the battery to see if we should enable long running animations. Currently this
    // is only enabled when plugged into AC power and Battery Saver isn't enabled. Battery Saver could
    // be enabled while plugged in so we should still conserve power where possible.
    static bool AreLongRunningAnimationsEnabled(_In_ wsyp::IPowerManagerStatics* powerMan);

    static void ApplyAnimationBehavior(
        _In_ WUComp::IKeyFrameAnimation* animation,
        _In_ wf::TimeSpan ts,
        _In_ const AnimationBehavior& behavior);

    static wf::TimeSpan GetAmbientLightDuration();
    static wf::TimeSpan GetPulsingAnimationDuration();
    static wf::TimeSpan GetPulsingAnimationDelay();
    static wf::TimeSpan GetIntensityDuration(const RevealFocusSource& source, DirectUI::FocusNavigationDirection direction);
};

}
