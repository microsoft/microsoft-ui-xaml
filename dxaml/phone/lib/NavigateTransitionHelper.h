// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

XAML_ABI_NAMESPACE_BEGIN namespace Microsoft { namespace UI { namespace Xaml { namespace Media { namespace Animation
{
    struct ObjectKeyFrameDescription
    {
        INT64 m_time;
        BOOLEAN m_value;
        IEasingFunctionBase* m_easingFunction;

        ObjectKeyFrameDescription()
        {
            m_time = 0;
            m_value = 0;
            m_easingFunction = NULL;
        }

        ObjectKeyFrameDescription(_In_ INT64 time, _In_ BOOLEAN value)
        {
            m_time = time;
            m_value = value;
            m_easingFunction = NULL;
        }

        ObjectKeyFrameDescription(_In_ INT64 time, _In_ BOOLEAN value, _In_ IEasingFunctionBase* easingFunction)
        {
            m_time = time;
            m_value = value;
            m_easingFunction = easingFunction;
        }
    };

    struct ObjectAnimationDescription
    {
        xaml::IUIElement* m_element;
        wrl_wrappers::HString* m_prop;
        std::vector<ObjectKeyFrameDescription>* m_pKeyFrameDescs;

        ObjectAnimationDescription()
        {
            m_element = NULL;
            m_prop = NULL;
            m_pKeyFrameDescs = NULL;
        }

        ObjectAnimationDescription(_In_ xaml::IUIElement* element, _In_ wrl_wrappers::HString* prop, _In_ std::vector<ObjectKeyFrameDescription>* keyFrames)
        {
            m_element = element;
            m_prop = prop;
            m_pKeyFrameDescs = keyFrames;
        }
    };

    struct DoubleKeyFrameDescription
    {
        INT64 m_time;
        DOUBLE m_value;
        IEasingFunctionBase* m_easingFunction;

        DoubleKeyFrameDescription()
        {
            m_time = 0;
            m_value = 0;
            m_easingFunction = NULL;
        }

        DoubleKeyFrameDescription(_In_ INT64 time, _In_ DOUBLE value)
        {
            m_time = time;
            m_value = value;
            m_easingFunction = NULL;
        }

        DoubleKeyFrameDescription(_In_ INT64 time, _In_ DOUBLE value, _In_ IEasingFunctionBase* easingFunction)
        {
            m_time = time;
            m_value = value;
            m_easingFunction = easingFunction;
        }
    };

    struct DoubleAnimationDescription
    {
        xaml::IUIElement* m_element;
        wrl_wrappers::HString* m_prop;
        std::vector<DoubleKeyFrameDescription>* m_pKeyFrameDescs;

        DoubleAnimationDescription()
        {
            m_element = NULL;
            m_prop = NULL;
            m_pKeyFrameDescs = NULL;
        }

        DoubleAnimationDescription(_In_ xaml::IUIElement* element, _In_ wrl_wrappers::HString* prop, _In_ std::vector<DoubleKeyFrameDescription>* keyFrames)
        {
            m_element = element;
            m_prop = prop;
            m_pKeyFrameDescs = keyFrames;
        }
    };

    class NavigateTransitionHelper
    {
        public:

            // General Values.

            static const INT WARP_FACTOR = 1;
            static const INT NO_OFFSET = 0;
            static const INT TIME_ZERO = 0;

            // Turnstile Values.

            static const INT TURNSTILE_EASE = 6;

            static const INT TURNSTILE_OFFSET_IN = 80;
            static const INT TURNSTILE_OFFSET_OUT = 50;
            static const DOUBLE TURNSTILE_AXIS_X;
            static const DOUBLE TURNSTILE_AXIS_Z;

            static const INT64 TURNSTILE_START_TIME = 0;
            static const INT64 TURNSTILE_MID_TIME = WARP_FACTOR * 128;
            static const INT64 TURNSTILE_END_TIME = TURNSTILE_MID_TIME + WARP_FACTOR * 428;

            static const INT TURNSTILE_FEATHER_OFFSET_IN = WARP_FACTOR * 33;
            static const INT TURNSTILE_FEATHER_OFFSET_OUT = WARP_FACTOR * 33;

            static const DOUBLE TURNSTILE_FEATHER_SCALE_IN;
            static const DOUBLE TURNSTILE_FEATHER_SCALE_OUT;

            // Slide Values.

            static const INT SLIDE_EASE = 6;

            static const INT SLIDE_OFFSET_IN = 200;
            static const INT SLIDE_OFFSET_OUT = 200;

            static const INT64 SLIDE_START_TIME = 0;
            static const INT64 SLIDE_MID_TIME = WARP_FACTOR * 250;
            static const INT64 SLIDE_END_TIME = SLIDE_MID_TIME + WARP_FACTOR * 350;

            // Opacity Values.

            static const INT NO_OPACITY = 0;
            static const INT FULL_OPACITY = 1;

            // Scale Values.

            static const INT NO_SCALE = 1;

            NavigateTransitionHelper()
            {

            }

            _Check_return_ static HRESULT AddTimelineToStoryboard(
                _In_ ITimeline* timeline,
                _In_ IStoryboard* storyboard);

            _Check_return_ static HRESULT CreateObjectStoryboardForNavigationAnimations(
                _In_ std::vector<ObjectAnimationDescription>* animations,
                _Outptr_ IStoryboard** ppStoryboard);

            _Check_return_ static HRESULT GetObjectAnimation(
                _In_ wrl::ComPtr<IStoryboardStatics> spStoryboardStatics,
                _In_ ObjectAnimationDescription desc,
                _Outptr_ IObjectAnimationUsingKeyFrames** ppAnimation);

            _Check_return_ static HRESULT GetObjectKeyFrame(
                _In_ ObjectKeyFrameDescription desc,
                _Outptr_ IObjectKeyFrame** ppKeyframe);

            _Check_return_ static HRESULT CreateStoryboardForNavigationAnimations(
                _In_ std::vector<DoubleAnimationDescription>* animations,
                _Outptr_ IStoryboard** ppStoryboard);

            _Check_return_ static HRESULT GetAnimation(
                _In_ wrl::ComPtr<IStoryboardStatics> spStoryboardStatics,
                _In_ DoubleAnimationDescription desc,
                _Outptr_ IDoubleAnimationUsingKeyFrames** ppAnimation);

            _Check_return_ static HRESULT GetDoubleKeyFrame(
                _In_ DoubleKeyFrameDescription desc,
                _Outptr_ IDoubleKeyFrame** ppKeyframe);

            _Check_return_ static HRESULT GetExponentialEase(
                _In_ DOUBLE exponent,
                _In_ EasingMode mode,
                _Outptr_ IEasingFunctionBase** ppExpEase);

            _Check_return_ static HRESULT GetPowerEase(
                _In_ DOUBLE power,
                _In_ EasingMode mode,
                _Outptr_ IEasingFunctionBase** ppPowerEase);

            _Check_return_ static HRESULT GetCircleEase(
                _In_ EasingMode mode,
                _Outptr_ IEasingFunctionBase** ppCircleEase);

            _Check_return_ static HRESULT IsAncestor(
                _In_ xaml::IFrameworkElement* ancestor,
                _In_ xaml::IFrameworkElement* possibleChild,
                _Out_ BOOLEAN* isAncestor);

            // Adds a point animation to the storyboard that sets the
            // transform origin of the element's transition target
            // for the duration of the storyboard.
            _Check_return_ static HRESULT SetTransformOrigin(
                _In_ xaml::IUIElement* element,
                _In_ wf::Point origin,
                _In_ IStoryboard* storyboard);

            _Check_return_ static HRESULT RegisterSplineKeyFrame(
                _In_ IDoubleAnimationUsingKeyFrames* animation,
                _In_ DOUBLE value,
                _In_ INT64 keyTimeInMilliseconds,
                _In_opt_ wf::Point controlPoint1,
                _In_opt_ wf::Point controlPoint2);

            _Check_return_ static HRESULT RegisterEasingKeyFrame(
                _In_ IDoubleAnimationUsingKeyFrames* animation,
                _In_ DOUBLE value,
                _In_ INT64 keyTimeInMilliseconds,
                _In_ IEasingFunctionBase* easingFunction);

            _Check_return_ static HRESULT RegisterDiscreteKeyFrame(
                _In_ IDoubleAnimationUsingKeyFrames* animation,
                _In_ DOUBLE value,
                _In_ INT64 keyTimeInMilliseconds);

            _Check_return_ static HRESULT RemoveHitTestVisbility(
                _In_ xaml_animation::IStoryboardStatics* storyboardStatics,
                _In_ wfc::IVector<Timeline*>* timelines,
                _In_ xaml::IDependencyObject* element);

    };

} } } } } XAML_ABI_NAMESPACE_END
