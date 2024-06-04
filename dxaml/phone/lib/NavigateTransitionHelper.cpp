// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

XAML_ABI_NAMESPACE_BEGIN namespace Microsoft { namespace UI { namespace Xaml { namespace Media { namespace Animation
{
    _Check_return_ HRESULT NavigateTransitionHelper::AddTimelineToStoryboard(
        _In_ ITimeline* timeline,
        _In_ IStoryboard* storyboard)
    {
        HRESULT hr = S_OK;
        wrl::ComPtr<wfc::IVector<Timeline*>> spTimelines;

        // Get timelines from storyboard and add timeline.
        IFC(storyboard->get_Children(&spTimelines));
        IFC(spTimelines->Append(timeline));

Cleanup:
        RRETURN(hr);
    }

    _Check_return_ HRESULT NavigateTransitionHelper::CreateObjectStoryboardForNavigationAnimations(
        _In_ std::vector<ObjectAnimationDescription>* animations,
        _Outptr_ IStoryboard** ppStoryboard)
    {
        HRESULT hr = S_OK;

        ARG_VALIDRETURNPOINTER(ppStoryboard);

        wrl::ComPtr<IStoryboardStatics> spStoryboardStatics;
        wrl::ComPtr<wfc::IVector<Timeline*>> spTimelines;
        wrl::ComPtr<IStoryboard> spStoryboard;

        // Create storyboard statics.
        IFC(wf::GetActivationFactory(wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_Storyboard).Get(), &spStoryboardStatics));

        // Make storyboard.
        IFC(wf::ActivateInstance(wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_Storyboard).Get(), &spStoryboard));

        // Get timelines from storyboard and add timeline.
        IFC(spStoryboard->get_Children(&spTimelines));

        for (auto it = animations->begin(); it != animations->end(); ++it)
        {
            wrl::ComPtr<ITimeline> spTimeline;
            wrl::ComPtr<IObjectAnimationUsingKeyFrames> spAnimation;

            IFC(GetObjectAnimation(spStoryboardStatics, *it, &spAnimation));

            IFC(spAnimation.As(&spTimeline));

            IFC(spTimelines->Append(spTimeline.Get()));
        }

        IFC(spStoryboard.CopyTo(ppStoryboard));

Cleanup:
        RRETURN(hr);
    }

    _Check_return_ HRESULT NavigateTransitionHelper::GetObjectAnimation(
        _In_ wrl::ComPtr<IStoryboardStatics> spStoryboardStatics,
        _In_ ObjectAnimationDescription desc,
        _Outptr_ IObjectAnimationUsingKeyFrames** ppAnimation)
    {
        HRESULT hr = S_OK;

        ARG_VALIDRETURNPOINTER(ppAnimation);

        wrl::ComPtr<xaml::IUIElement> spElement (desc.m_element);
        wrl::ComPtr<IObjectAnimationUsingKeyFrames> spAnimation;
        wrl::ComPtr<ITimeline> spTimeline;
        wrl::ComPtr<xaml::IDependencyObject> spElementAsDO;
        wrl::ComPtr<wfc::IVector<ObjectKeyFrame*>> spKeyframeCollection;

        IFC(wf::ActivateInstance(wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_ObjectAnimationUsingKeyFrames).Get(), &spAnimation));

        // Set targets for the timeline.
        IFC(spAnimation.As(&spTimeline));
        IFC(spElement.As(&spElementAsDO));
        IFC(spStoryboardStatics->SetTarget(spTimeline.Get(), spElementAsDO.Get()));
        IFC(spStoryboardStatics->SetTargetProperty(spTimeline.Get(), *(desc.m_prop)));

        // Get key frames from animation and add key frames.
        IFC(spAnimation->get_KeyFrames(&spKeyframeCollection));

        for (auto it = (desc.m_pKeyFrameDescs)->begin(); it != (desc.m_pKeyFrameDescs)->end(); ++it)
        {
            wrl::ComPtr<IObjectKeyFrame> spKeyFrame;

            IFC(GetObjectKeyFrame(*it, &spKeyFrame));

            IFC(spKeyframeCollection->Append(spKeyFrame.Get()));
        }

        IFC(spAnimation.CopyTo(ppAnimation));

Cleanup:
        RRETURN(hr);
    }

    _Check_return_ HRESULT NavigateTransitionHelper::GetObjectKeyFrame(
        _In_ ObjectKeyFrameDescription desc,
        _Outptr_ IObjectKeyFrame** ppKeyframe)
    {
        HRESULT hr = S_OK;

        ARG_VALIDRETURNPOINTER(ppKeyframe);

        const INT64 ticksPerMillisecond = 10000;

        wrl::ComPtr<IInspectable> spValueInspectable;
        wrl::ComPtr<IObjectKeyFrame> spKeyFrame;
        wf::TimeSpan timeSpan = {};
        KeyTime keyTime = {};

        // Make keyframe.
        IFC(wf::ActivateInstance(wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_DiscreteObjectKeyFrame).Get(), &spKeyFrame));

        IFC(Private::ValueBoxer::CreateBoolean(desc.m_value, &spValueInspectable));

        // Add the key time and key value to the key frame.
        timeSpan.Duration = desc.m_time * ticksPerMillisecond;
        keyTime.TimeSpan = timeSpan;
        IFC(spKeyFrame->put_KeyTime(keyTime));
        IFC(spKeyFrame->put_Value(spValueInspectable.Get()));

        IFC(spKeyFrame.CopyTo(ppKeyframe));

    Cleanup:
        RRETURN(hr);
    }

    _Check_return_
    HRESULT
    NavigateTransitionHelper::CreateStoryboardForNavigationAnimations(
        _In_ std::vector<DoubleAnimationDescription>* animations,
        _Outptr_ IStoryboard** ppStoryboard)
    {
        HRESULT hr = S_OK;

        ARG_VALIDRETURNPOINTER(ppStoryboard);

        wrl::ComPtr<IStoryboardStatics> spStoryboardStatics;
        wrl::ComPtr<wfc::IVector<Timeline*>> spTimelines;
        wrl::ComPtr<IStoryboard> spStoryboard;

        // Create storyboard statics.
        IFC(wf::GetActivationFactory(wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_Storyboard).Get(), &spStoryboardStatics));

        // Make storyboard.
        IFC(wf::ActivateInstance(wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_Storyboard).Get(), &spStoryboard));

        // Get timelines from storyboard and add timeline.
        IFC(spStoryboard->get_Children(&spTimelines));

        for (auto it = animations->begin(); it != animations->end(); ++it)
        {
            wrl::ComPtr<ITimeline> spTimeline;
            wrl::ComPtr<IDoubleAnimationUsingKeyFrames> spAnimation;

            IFC(GetAnimation(spStoryboardStatics, *it, &spAnimation));

            IFC(spAnimation.As(&spTimeline));

            IFC(spTimelines->Append(spTimeline.Get()));
        }

        IFC(spStoryboard.CopyTo(ppStoryboard));

Cleanup:
        RRETURN(hr);
    }

    _Check_return_
    HRESULT
    NavigateTransitionHelper::GetAnimation(
        _In_ wrl::ComPtr<IStoryboardStatics> spStoryboardStatics,
        _In_ DoubleAnimationDescription desc,
        _Outptr_ IDoubleAnimationUsingKeyFrames** ppAnimation)
    {
        HRESULT hr = S_OK;

        ARG_VALIDRETURNPOINTER(ppAnimation);

        wrl::ComPtr<xaml::IUIElement> spElement (desc.m_element);
        wrl::ComPtr<IDoubleAnimationUsingKeyFrames> spAnimation;
        wrl::ComPtr<ITimeline> spTimeline;
        wrl::ComPtr<xaml::IDependencyObject> spElementAsDO;
        wrl::ComPtr<wfc::IVector<DoubleKeyFrame*>> spKeyframeCollection;

        IFC(wf::ActivateInstance(wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_DoubleAnimationUsingKeyFrames).Get(), &spAnimation));

        // Set targets for the timeline.
        IFC(spAnimation.As(&spTimeline));
        IFC(spElement.As(&spElementAsDO));
        IFC(spStoryboardStatics->SetTarget(spTimeline.Get(), spElementAsDO.Get()));
        IFC(spStoryboardStatics->SetTargetProperty(spTimeline.Get(), *(desc.m_prop)));

        // Get key frames from animation and add key frames.
        IFC(spAnimation->get_KeyFrames(&spKeyframeCollection));

        for (auto it = (desc.m_pKeyFrameDescs)->begin(); it != (desc.m_pKeyFrameDescs)->end(); ++it)
        {
            wrl::ComPtr<IDoubleKeyFrame> spKeyFrame;

            IFC(GetDoubleKeyFrame(*it, &spKeyFrame));

            IFC(spKeyframeCollection->Append(spKeyFrame.Get()));
        }

        IFC(spAnimation.CopyTo(ppAnimation));

Cleanup:
        RRETURN(hr);
    }

    _Check_return_
    HRESULT
    NavigateTransitionHelper::GetDoubleKeyFrame(
        _In_ DoubleKeyFrameDescription desc,
        _Outptr_ IDoubleKeyFrame** ppKeyframe)
    {
        HRESULT hr = S_OK;

        ARG_VALIDRETURNPOINTER(ppKeyframe);

        const INT64 ticksPerMillisecond = 10000;

        wrl::ComPtr<IDoubleKeyFrame> spKeyFrame;
        wf::TimeSpan timeSpan = {};
        KeyTime keyTime = {};

        // Make keyframe.
        if(desc.m_easingFunction)
        {
            wrl::ComPtr<IEasingDoubleKeyFrame> spEasingKeyFrame;
            IFC(wf::ActivateInstance(wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_EasingDoubleKeyFrame).Get(), &spEasingKeyFrame));
            IFC(spEasingKeyFrame->put_EasingFunction(desc.m_easingFunction));
            IFC(spEasingKeyFrame.As(&spKeyFrame));
        }
        else
        {
            IFC(wf::ActivateInstance(wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_LinearDoubleKeyFrame).Get(), &spKeyFrame));
        }

        // Add the key time and key value to the key frame.
        timeSpan.Duration = desc.m_time * ticksPerMillisecond;
        keyTime.TimeSpan = timeSpan;
        IFC(spKeyFrame->put_KeyTime(keyTime));
        IFC(spKeyFrame->put_Value(desc.m_value));

        IFC(spKeyFrame.CopyTo(ppKeyframe));

Cleanup:
        RRETURN(hr);
    }

    _Check_return_
    HRESULT
    NavigateTransitionHelper::GetExponentialEase(
        _In_ DOUBLE exponent,
        _In_ EasingMode mode,
        _Outptr_ IEasingFunctionBase** ppExpEase)
    {
        HRESULT hr = S_OK;

        ARG_VALIDRETURNPOINTER(ppExpEase);

        wrl::ComPtr<IEasingFunctionBase> spEasingFunction;
        wrl::ComPtr<IExponentialEase> spExponentialEase;

        IFC(wf::ActivateInstance(wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_ExponentialEase).Get(), &spExponentialEase));
        IFC(spExponentialEase->put_Exponent(exponent));

        IFC(spExponentialEase.As(&spEasingFunction));
        IFC(spEasingFunction->put_EasingMode(mode));

        IFC(spEasingFunction.CopyTo(ppExpEase));

Cleanup:
        RRETURN(hr);
    }

    _Check_return_
    HRESULT
    NavigateTransitionHelper::GetPowerEase(
        _In_ DOUBLE power,
        _In_ EasingMode mode,
        _Outptr_ IEasingFunctionBase** ppPowerEase)
    {
        HRESULT hr = S_OK;

        ARG_VALIDRETURNPOINTER(ppPowerEase);

        wrl::ComPtr<IEasingFunctionBase> spEasingFunction;
        wrl::ComPtr<IPowerEase> spPowerEase;

        IFC(wf::ActivateInstance(wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_PowerEase).Get(), &spPowerEase));
        IFC(spPowerEase->put_Power(power));

        IFC(spPowerEase.As(&spEasingFunction));
        IFC(spEasingFunction->put_EasingMode(mode));

        IFC(spEasingFunction.CopyTo(ppPowerEase));

Cleanup:
        RRETURN(hr);
    }

    _Check_return_
    HRESULT
    NavigateTransitionHelper::GetCircleEase(
        _In_ EasingMode mode,
        _Outptr_ IEasingFunctionBase** ppCircleEase)
    {
        HRESULT hr = S_OK;

        ARG_VALIDRETURNPOINTER(ppCircleEase);

        wrl::ComPtr<IEasingFunctionBase> spEasingFunction;
        wrl::ComPtr<ICircleEase> spCircleEase;

        IFC(wf::ActivateInstance(wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_CircleEase).Get(), &spCircleEase));

        IFC(spCircleEase.As(&spEasingFunction));
        IFC(spEasingFunction->put_EasingMode(mode));

        IFC(spEasingFunction.CopyTo(ppCircleEase));

Cleanup:
        RRETURN(hr);
    }

    _Check_return_
    HRESULT
    NavigateTransitionHelper::IsAncestor(
        _In_ xaml::IFrameworkElement* ancestor,
        _In_ xaml::IFrameworkElement* possibleChild,
        _Out_ BOOLEAN* isAncestor)
    {
        HRESULT hr = S_OK;
        *isAncestor = FALSE;

        wrl::ComPtr<xaml::IFrameworkElement> spPossibleChild (possibleChild);
        wrl::ComPtr<xaml::IDependencyObject> spPossibleChildAsDO;
        wrl::ComPtr<xaml::IDependencyObject> spParentOfChildAsDO;
        wrl::ComPtr<xaml::IFrameworkElement> spParentOfChildAsFrameworkElement;
        wrl::ComPtr<IVisualTreeHelperStatics> spTreeHelperStatics;

        IFC(wf::GetActivationFactory(wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_VisualTreeHelper).Get(), &spTreeHelperStatics));

        IFC(spPossibleChild.As(&spPossibleChildAsDO));
        IFC(spTreeHelperStatics->GetParent(spPossibleChildAsDO.Get(), &spParentOfChildAsDO));

        if (!spParentOfChildAsDO)
        {
            // The parent is null, so return false (already set).
            goto Cleanup;
        }

        IFC(spParentOfChildAsDO.As(&spParentOfChildAsFrameworkElement));

        if (spParentOfChildAsFrameworkElement.Get() == ancestor)
        {
            *isAncestor = TRUE;
            goto Cleanup;
        }
        else
        {
            IFC(IsAncestor(ancestor, spParentOfChildAsFrameworkElement.Get(), isAncestor));
        }

Cleanup:
        RRETURN(hr);
    }

    _Check_return_
    HRESULT
    NavigateTransitionHelper::SetTransformOrigin(
        _In_ xaml::IUIElement* element,
        _In_ wf::Point origin,
        _In_ IStoryboard* storyboard)
    {
        HRESULT hr = S_OK;
        xaml::Duration duration = {};
        wf::TimeSpan timeSpan = {};

        wrl_wrappers::HStringReference transformOriginPropertyName(STR_LEN_PAIR(L"(UIElement.TransitionTarget).TransformOrigin"));
        wrl::ComPtr<xaml::IUIElement> spElement (element);
        wrl::ComPtr<xaml::IDependencyObject> spElementAsDO;
        wrl::ComPtr<IPointAnimation> spTransformOriginAnimation;
        wrl::ComPtr<ITimeline> spAnimationAsTimeline;
        wrl::ComPtr<IStoryboardStatics> spStoryboardStatics;
        wrl::ComPtr<IInspectable> spPointAsInspectable;
        wrl::ComPtr<wf::IReference<wf::Point>> spPointAsReference;

        IFC(wf::ActivateInstance(wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_PointAnimation).Get(), &spTransformOriginAnimation));
        IFC(wf::GetActivationFactory(wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_Storyboard).Get(), &spStoryboardStatics));

        // Set the target and target property.
        IFC(spElement.As(&spElementAsDO));
        IFC(spTransformOriginAnimation.As(&spAnimationAsTimeline));
        IFC(spStoryboardStatics->SetTarget(spAnimationAsTimeline.Get(), spElementAsDO.Get()));
        IFC(spStoryboardStatics->SetTargetProperty(spAnimationAsTimeline.Get(), transformOriginPropertyName.Get()));

        // Set destination value.
        IFC(Private::ValueBoxer::CreatePoint(origin, &spPointAsInspectable));
        IFC(spPointAsInspectable.As(&spPointAsReference));
        IFC(spTransformOriginAnimation->put_To(spPointAsReference.Get()));

        // Set the duration of the animation.
        duration.TimeSpan = timeSpan;
        duration.Type = xaml::DurationType_TimeSpan;
        IFC(spAnimationAsTimeline->put_Duration(duration));

        IFC(AddTimelineToStoryboard(spAnimationAsTimeline.Get(), storyboard));

Cleanup:
        RRETURN(hr);
    }

    _Check_return_
    HRESULT
    NavigateTransitionHelper::RegisterSplineKeyFrame(
        _In_ IDoubleAnimationUsingKeyFrames* animation,
        _In_ DOUBLE value,
        _In_ INT64 keyTimeInMilliseconds,
        _In_opt_ wf::Point controlPoint1,
        _In_opt_ wf::Point controlPoint2)
    {
        const INT64 millisecondsTo100Nanoseconds = 10000; // 10^6 / 100

        wf::TimeSpan timeSpan = {};
        KeyTime keyTime = {};
        wrl::ComPtr<IKeySpline> keySpline;
        wrl::ComPtr<IDoubleAnimationUsingKeyFrames> doubleAnimationUsingKeyFrames(animation);
        wrl::ComPtr<ISplineDoubleKeyFrame> splineDoubleKeyFrame;
        wrl::ComPtr<IDoubleKeyFrame> doubleKeyFrame;
        wrl::ComPtr<wfc::IVector<DoubleKeyFrame*>> keyFrameCollection;

        IFC_RETURN(wf::ActivateInstance(wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_KeySpline).Get(), &keySpline));
        IFC_RETURN(wf::ActivateInstance(wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_SplineDoubleKeyFrame).Get(), &splineDoubleKeyFrame));

        timeSpan.Duration = keyTimeInMilliseconds * millisecondsTo100Nanoseconds;
        keyTime.TimeSpan = timeSpan;
        IFC_RETURN(keySpline->put_ControlPoint1(controlPoint1));
        IFC_RETURN(keySpline->put_ControlPoint2(controlPoint2));

        IFC_RETURN(splineDoubleKeyFrame->put_KeySpline(keySpline.Get()));
        IFC_RETURN(splineDoubleKeyFrame.As(&doubleKeyFrame));
        IFC_RETURN(doubleKeyFrame->put_KeyTime(keyTime));
        IFC_RETURN(doubleKeyFrame->put_Value(value));

        IFC_RETURN(doubleAnimationUsingKeyFrames->get_KeyFrames(&keyFrameCollection));
        IFC_RETURN(keyFrameCollection->Append(doubleKeyFrame.Get()));

        return S_OK;
    }

    _Check_return_
        HRESULT
        NavigateTransitionHelper::RegisterEasingKeyFrame(
            _In_ IDoubleAnimationUsingKeyFrames* animation,
            _In_ DOUBLE value,
            _In_ INT64 keyTimeInMilliseconds,
            _In_ IEasingFunctionBase* easingFunction)
    {
        const INT64 millisecondsTo100Nanoseconds = 10000; // 10^6 / 100

        wf::TimeSpan timeSpan = {};
        KeyTime keyTime = {};
        wrl::ComPtr<IDoubleAnimationUsingKeyFrames> doubleAnimationUsingKeyFrames(animation);
        wrl::ComPtr<IEasingDoubleKeyFrame> easingDoubleKeyFrame;
        wrl::ComPtr<IDoubleKeyFrame> doubleKeyFrame;
        wrl::ComPtr<wfc::IVector<DoubleKeyFrame*>> keyFrameCollection;

        IFC_RETURN(wf::ActivateInstance(wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_EasingDoubleKeyFrame).Get(), &easingDoubleKeyFrame));

        IFC_RETURN(easingDoubleKeyFrame->put_EasingFunction(easingFunction));

        timeSpan.Duration = keyTimeInMilliseconds * millisecondsTo100Nanoseconds;
        keyTime.TimeSpan = timeSpan;

        IFC_RETURN(easingDoubleKeyFrame.As(&doubleKeyFrame));
        IFC_RETURN(doubleKeyFrame->put_KeyTime(keyTime));
        IFC_RETURN(doubleKeyFrame->put_Value(value));

        IFC_RETURN(doubleAnimationUsingKeyFrames->get_KeyFrames(&keyFrameCollection));
        IFC_RETURN(keyFrameCollection->Append(doubleKeyFrame.Get()));

        return S_OK;
    }

    _Check_return_
        HRESULT
        NavigateTransitionHelper::RegisterDiscreteKeyFrame(
            _In_ IDoubleAnimationUsingKeyFrames* animation,
            _In_ DOUBLE value,
            _In_ INT64 keyTimeInMilliseconds)
    {
        const INT64 millisecondsTo100Nanoseconds = 10000; // 10^6 / 100

        wf::TimeSpan timeSpan = {};
        KeyTime keyTime = {};
        wrl::ComPtr<IDoubleAnimationUsingKeyFrames> doubleAnimationUsingKeyFrames(animation);
        wrl::ComPtr<IDiscreteDoubleKeyFrame> discreteDoubleKeyFrame;
        wrl::ComPtr<IDoubleKeyFrame> doubleKeyFrame;
        wrl::ComPtr<wfc::IVector<DoubleKeyFrame*>> keyFrameCollection;

        IFC_RETURN(wf::ActivateInstance(wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_DiscreteDoubleKeyFrame).Get(), &discreteDoubleKeyFrame));

        timeSpan.Duration = keyTimeInMilliseconds * millisecondsTo100Nanoseconds;
        keyTime.TimeSpan = timeSpan;

        IFC_RETURN(discreteDoubleKeyFrame.As(&doubleKeyFrame));
        IFC_RETURN(doubleKeyFrame->put_KeyTime(keyTime));
        IFC_RETURN(doubleKeyFrame->put_Value(value));

        IFC_RETURN(doubleAnimationUsingKeyFrames->get_KeyFrames(&keyFrameCollection));
        IFC_RETURN(keyFrameCollection->Append(doubleKeyFrame.Get()));

        return S_OK;
    }

    _Check_return_
        HRESULT
        NavigateTransitionHelper::RemoveHitTestVisbility(
            _In_ xaml_animation::IStoryboardStatics* storyboardStatics,
            _In_ wfc::IVector<Timeline*>* timelines,
            _In_ xaml::IDependencyObject* element)
    {
        wrl_wrappers::HString propertyNameIsHitTestVisible;
        IFC_RETURN(propertyNameIsHitTestVisible.Set(STR_LEN_PAIR(L"UIElement.IsHitTestVisible")));

        wrl::ComPtr<IObjectAnimationUsingKeyFrames> objectAnimationUsingKeyFrames;
        IFC_RETURN(wf::ActivateInstance(wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_ObjectAnimationUsingKeyFrames).Get(), &objectAnimationUsingKeyFrames));

        wrl::ComPtr<IDiscreteObjectKeyFrame> discreteObjectKeyFrame;
        IFC_RETURN(wf::ActivateInstance(wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_DiscreteObjectKeyFrame).Get(), &discreteObjectKeyFrame));

        wf::TimeSpan timeSpan = { 0 };
        KeyTime keyTime = {};
        keyTime.TimeSpan = timeSpan;

        wrl::ComPtr<IObjectKeyFrame> objectKeyFrame;
        wrl::ComPtr<wfc::IVector<ObjectKeyFrame*>> keyFrameCollection;
        wrl::ComPtr<IInspectable> value;

        IFC_RETURN(Private::ValueBoxer::CreateBoolean(FALSE, &value));
        IFC_RETURN(discreteObjectKeyFrame.As(&objectKeyFrame));
        IFC_RETURN(objectKeyFrame->put_KeyTime(keyTime));
        IFC_RETURN(objectKeyFrame->put_Value(value.Get()));

        IFC_RETURN(objectAnimationUsingKeyFrames->get_KeyFrames(&keyFrameCollection));
        IFC_RETURN(keyFrameCollection->Append(objectKeyFrame.Get()));

        wrl::ComPtr<ITimeline> timeline;
        IFC_RETURN(objectAnimationUsingKeyFrames.As(&timeline));
        IFC_RETURN(storyboardStatics->SetTarget(timeline.Get(), element));
        IFC_RETURN(storyboardStatics->SetTargetProperty(timeline.Get(), propertyNameIsHitTestVisible.Get()));

        IFC_RETURN(timelines->Append(timeline.Get()));

        return S_OK;
    }
} } } } } XAML_ABI_NAMESPACE_END
