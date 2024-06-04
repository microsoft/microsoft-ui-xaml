// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ThemeGenerator.h"
#include "SplineDoubleKeyFrame.g.h"
#include "LinearDoubleKeyFrame.g.h"
#include "DiscreteObjectKeyFrame.g.h"
#include "ObjectAnimationUsingKeyFrames.g.h"
#include "DoubleAnimationUsingKeyFrames.g.h"
#include "PointerKeyFrame.g.h"
#include "PVLStaggerFunction.g.h"
#include "PointerAnimationUsingKeyFrames.g.h"
#include "StoryBoard.g.h"
#include "KeySpline.g.h"
#include "PointAnimation.g.h"
#include "DiscreteDoubleKeyFrame.g.h"
#include "PointerKeyFrameCollection.g.h"
#include "PointerAnimationUsingKeyFrames.h"

#include "uxtheme.h"
#include "vsanimation.h"

// Uncomment to get ThemeGenerator debug traces
// #define TG_DBG

using namespace DirectUI;
using namespace DirectUISynonyms;

ThemeGeneratorHelper::~ThemeGeneratorHelper()
{
    std::map<HSTRING, DoubleAnimationUsingKeyFrames*>::iterator  itrKeyframes;

    for (itrKeyframes = m_doubleAnimationsMap.begin();
            itrKeyframes != m_doubleAnimationsMap.end();
            ++itrKeyframes)
    {
        ctl::release_interface(itrKeyframes->second);
    }
    m_doubleAnimationsMap.clear();

    std::map<HSTRING, PointerAnimationUsingKeyFrames*>::iterator  itrPointerKeyframes;

    for (itrPointerKeyframes = m_pointerAnimationsMap.begin();
            itrPointerKeyframes != m_pointerAnimationsMap.end();
            ++itrPointerKeyframes)
    {
        ctl::release_interface(itrPointerKeyframes->second);
    }
    m_pointerAnimationsMap.clear();

    ReleaseInterface(m_pStoryboardStatics);
    ReleaseInterface(m_pActivationFactory);
    ReleaseInterface(m_target);
}

_Check_return_ HRESULT ThemeGeneratorHelper::Initialize()
{
    HRESULT hr = S_OK;
    m_pActivationFactory = ctl::ActivationFactoryCreator<DirectUI::StoryboardFactory>::CreateActivationFactory();
    IFC(ctl::do_query_interface(m_pStoryboardStatics, m_pActivationFactory));
    IFC(m_strTranslateXPropertyName.Set(STR_LEN_PAIR(L"(UIElement.TransitionTarget).(TransitionTarget.CompositeTransform).TranslateX")));
    IFC(m_strTranslateYPropertyName.Set(STR_LEN_PAIR(L"(UIElement.TransitionTarget).(TransitionTarget.CompositeTransform).TranslateY")));
    IFC(m_strOpacityPropertyName.Set(STR_LEN_PAIR(L"(UIElement.TransitionTarget).Opacity")));
    IFC(m_strCenterYPropertyName.Set(STR_LEN_PAIR(L"(UIElement.TransitionTarget).(TransitionTarget.CompositeTransform).CenterY")));
    IFC(m_strScaleXPropertyName.Set(STR_LEN_PAIR(L"(UIElement.TransitionTarget).(TransitionTarget.CompositeTransform).ScaleX")));
    IFC(m_strScaleYPropertyName.Set(STR_LEN_PAIR(L"(UIElement.TransitionTarget).(TransitionTarget.CompositeTransform).ScaleY")));
    IFC(m_strClipScaleXPropertyName.Set(STR_LEN_PAIR(L"(UIElement.TransitionTarget).(TransitionTarget.ClipTransform).(CompositeTransform.ScaleX)")));
    IFC(m_strClipScaleYPropertyName.Set(STR_LEN_PAIR(L"(UIElement.TransitionTarget).(TransitionTarget.ClipTransform).(CompositeTransform.ScaleY)")));
    IFC(m_strClipTranslateXPropertyName.Set(STR_LEN_PAIR(L"(UIElement.TransitionTarget).(TransitionTarget.ClipTransform).(CompositeTransform.TranslateX)")));
    IFC(m_strClipTranslateYPropertyName.Set(STR_LEN_PAIR(L"(UIElement.TransitionTarget).(TransitionTarget.ClipTransform).(CompositeTransform.TranslateY)")));

    #ifdef DBG
    {
        INT64 dbgSlowDownFactor = 0;

        DXamlCore* pCore = DXamlCore::GetCurrent();

        CCoreServices *pCoreServices = pCore->GetHandle();

        dbgSlowDownFactor = pCoreServices->GetAnimationSlowFactor();

        m_dbgSlowDownFactor = 1;

        if (dbgSlowDownFactor > 0)
        {
            m_dbgSlowDownFactor = dbgSlowDownFactor;
        }
        else
        {
            HKEY hKey;

            if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\DirectUI", 0, KEY_QUERY_VALUE|KEY_WOW64_64KEY, &hKey))
            {
                DWORD cbSize = sizeof(DWORD);
                DWORD dwExtendAnimations = 0;

                if (ERROR_SUCCESS == RegQueryValueEx(hKey, L"ThemeAnimationSlowDownFactor", NULL, NULL, (LPBYTE)&dwExtendAnimations, &cbSize))
                {
                    m_dbgSlowDownFactor = static_cast<INT64>(dwExtendAnimations);
                }

                RegCloseKey(hKey);
            }
        }
    }
    #endif
Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ThemeGeneratorHelper::RegisterKeyFrame(
            _In_ HSTRING targetPropertyName,
            _In_ DOUBLE value,
            _In_ INT64 begintime,
            _In_ INT64 duration,
            _In_opt_ TimingFunctionDescription* pEasing)
{
    HRESULT hr = S_OK;

    wf::TimeSpan keyTimeTimeSpan = {};
    xaml_animation::IDoubleKeyFrame* pKeyFrame = NULL;
    xaml_animation::KeyTime keyTime = {};
    xaml_animation::IKeySpline* pKeySpline = NULL;
    ctl::ComPtr<wfc::IVector<xaml_animation::DoubleKeyFrame*>> spKeyframeCollection;
    UINT keyframeCollectionSize = 0;
    IInspectable* pBeginTimeInsp = NULL;
    wf::IReference<wf::TimeSpan>* pBeginTime = NULL;
    DoubleAnimationUsingKeyFrames* pKeyframes = NULL;

    INT64 time = 0;

    IFCPTR(targetPropertyName);

    if (!pEasing)
    {
        ctl::ComPtr<DiscreteDoubleKeyFrame> spKeyFrame;
        IFC(ctl::make(&spKeyFrame));
        pKeyFrame = spKeyFrame.Detach();
    }
    else if (pEasing->IsLinear() || m_onlyGenerateSteadyState)
    {
        ctl::ComPtr<LinearDoubleKeyFrame> spKeyFrame;
        IFC(ctl::make(&spKeyFrame));
        pKeyFrame = spKeyFrame.Detach();
    }
    else
    {
        ctl::ComPtr<SplineDoubleKeyFrame> spKeyFrame;
        ctl::ComPtr<KeySpline> spKeySpline;
        IFC(ctl::make(&spKeyFrame));
        IFC(ctl::make(&spKeySpline));
        IFC(spKeySpline->put_ControlPoint1(pEasing->cp2));
        IFC(spKeySpline->put_ControlPoint2(pEasing->cp3));
        IFC(spKeyFrame->put_KeySpline(spKeySpline.Get()));
        pKeyFrame = spKeyFrame.Detach();
    }

    // a keyframe needs to be registered inside a timeline. These are cached in the map, keyed on the property.
    pKeyframes = m_doubleAnimationsMap[targetPropertyName];
    if (!pKeyframes)
    {
        ctl::ComPtr<DoubleAnimationUsingKeyFrames> spKeyFrames;
        IFC(ctl::make(&spKeyFrames));
        pKeyframes = spKeyFrames.Detach();
        m_doubleAnimationsMap[targetPropertyName] = pKeyframes;

        IFC(m_pTimelines->Append(pKeyframes));    // this will not addref since pKeyframes does not have state

        if (m_target)
        {
            IFC(m_pStoryboardStatics->SetTarget(static_cast<Timeline*>(pKeyframes), m_target));
        }
        else if (m_targetName)
        {
            IFC(m_pStoryboardStatics->SetTargetName(static_cast<Timeline*>(pKeyframes), m_targetName));
        }
        IFC(m_pStoryboardStatics->SetTargetProperty(static_cast<Timeline*>(pKeyframes), targetPropertyName));

        // with the first keyframe to be created, we'll take its time and actually start the timeline at that point
        if (begintime > 0 && !m_onlyGenerateSteadyState)
        {
            m_begintime = begintime;    // cache for later consumption
            wf::TimeSpan beginTimespan = { };
            beginTimespan.Duration = m_begintime * 10000;
            #ifdef DBG
            beginTimespan.Duration *= m_dbgSlowDownFactor;
            #endif
            IFC(PropertyValue::CreateFromTimeSpan(beginTimespan, &pBeginTimeInsp));
            IFC(ctl::do_query_interface(pBeginTime, pBeginTimeInsp));
            IFC(pKeyframes->put_BeginTime(pBeginTime));
        }
    }

    IFC(pKeyframes->get_KeyFrames(&spKeyframeCollection));
    if (m_onlyGenerateSteadyState)
    {
        // steady state animation is created by only taking the last keyframe and setting its keytime to 0.
        IFC(spKeyframeCollection->get_Size(&keyframeCollectionSize));
        ASSERT(keyframeCollectionSize < 2); // would expect one at the very most
        if (keyframeCollectionSize > 0)
        {
            IFC(spKeyframeCollection->Clear());
        }
    }
    IFC(spKeyframeCollection->Append(pKeyFrame));

    // correct for the begintime set on the timeline
    time = m_onlyGenerateSteadyState ? 0 : begintime + duration - m_begintime;

    // initialize with keytime from transform
    keyTimeTimeSpan.Duration = 10000 * time;  // keyframe keytime is where a value needs to be at a certain time
    #ifdef DBG
    keyTimeTimeSpan.Duration *= m_dbgSlowDownFactor;
    #endif

    keyTime.TimeSpan = keyTimeTimeSpan;
    IFC(pKeyFrame->put_KeyTime(keyTime));
    IFC(pKeyFrame->put_Value(value));

Cleanup:
    ReleaseInterface(pBeginTime);
    ReleaseInterface(pBeginTimeInsp);
    ReleaseInterface(pKeySpline);
    ReleaseInterface(pKeyFrame);
    // note that pKeyframesAsI is not released. It is created in this method and put into a map that does not addref it
    RRETURN(hr);
}

_Check_return_ HRESULT ThemeGeneratorHelper::RegisterPointerKeyFrame(
            _In_ HSTRING targetPropertyName,
            _In_ PointerDirection sourceDirection,
            _In_ DOUBLE pointerValue,
            _In_ DOUBLE value,
            _In_ INT64 begintime)
{
    HRESULT hr = S_OK;

    wf::TimeSpan beginTimespan = { };
    ctl::ComPtr<IInspectable> spBeginTimeInsp;
    ctl::ComPtr<wf::IReference<wf::TimeSpan>> spBeginTime;

    ctl::ComPtr<PointerKeyFrame> spKeyFrame;
    ctl::ComPtr<PointerAnimationUsingKeyFrames> spKeyframes;
    ctl::ComPtr<PointerKeyFrameCollection> spKeyframeCollection;

    IFCPTR(targetPropertyName);

    // a keyframe needs to be registered inside a timeline. These are cached in the map, keyed on the property.
    spKeyframes = m_pointerAnimationsMap[targetPropertyName];
    if (spKeyframes == nullptr)
    {
        IFC(ctl::make(&spKeyframes));

        m_pointerAnimationsMap[targetPropertyName] = spKeyframes.Get();

        // Add a reference to be held by the map.
        ctl::addref_interface(spKeyframes.Get());

        IFC(m_pTimelines->Append(spKeyframes.Get()));

        if (m_target)
        {
            IFC(m_pStoryboardStatics->SetTarget(spKeyframes.Get(), m_target));
            IFC(spKeyframes->SetRelativeToObject(m_target));
        }
        else if (m_targetName)
        {
            IFC(m_pStoryboardStatics->SetTargetName(spKeyframes.Get(), m_targetName));
            IFC(spKeyframes->SetRelativeToObjectName(m_targetName));
        }
        IFC(m_pStoryboardStatics->SetTargetProperty(spKeyframes.Get(), targetPropertyName));
        IFC(spKeyframes->put_PointerSource(sourceDirection));
    }

    IFC(spKeyframes->get_KeyFrames(&spKeyframeCollection));

    IFC(ctl::make(&spKeyFrame));
    IFC(spKeyFrame->put_PointerValue(pointerValue));
    IFC(spKeyFrame->put_TargetValue(value));
    IFC(spKeyframeCollection->Append(spKeyFrame.Get()));

    if (begintime > 0)
    {
        m_begintime = begintime;
        beginTimespan.Duration = m_begintime * 10000;
        IFC(PropertyValue::CreateFromTimeSpan(beginTimespan, &spBeginTimeInsp));
        IFC(spBeginTimeInsp.As(&spBeginTime));
        IFC(spKeyframes->put_BeginTime(spBeginTime.Get()));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ThemeGeneratorHelper::Set2DTransformOriginValues(_In_ wf::Point originPoint)
{
    HRESULT hr = S_OK;

    if (!m_originValuesSet)   // only support one set of origin. Consider throwing if incoming is different
    {
        IFC(SetOriginForProperty(originPoint, wrl_wrappers::HStringReference(STR_LEN_PAIR(L"(UIElement.TransitionTarget).TransformOrigin")).Get(), m_originValuesSet));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ThemeGeneratorHelper::SetClipOriginValues(_In_ wf::Point originPoint)
{
    HRESULT hr = S_OK;

    if (!m_clipValuesSet)   // only support one set of origin. Consider throwing if incoming is different
    {
        IFC(SetOriginForProperty(originPoint, wrl_wrappers::HStringReference(STR_LEN_PAIR(L"(UIElement.TransitionTarget).ClipTransformOrigin")).Get(), m_clipValuesSet));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ThemeGeneratorHelper::PreventHitTestingWhileAnimating(_In_ INT64 durationInMilliseconds)
{
    HRESULT hr = S_OK;
    const INT64 millisecondsTo100Nanoseconds = 10000; // 10^6 / 100
    wrl_wrappers::HString isHitTestVisiblePropertyName;
    xaml_animation::KeyTime keyTime1 = {};
    xaml_animation::KeyTime keyTime2 = {};
    xaml::Duration duration = {};
    ctl::ComPtr<DiscreteObjectKeyFrame> spKeyFrame1;
    ctl::ComPtr<DiscreteObjectKeyFrame> spKeyFrame2;
    ctl::ComPtr<ObjectAnimationUsingKeyFrames> spHitTestVisibleAnimation;
    ctl::ComPtr<wfc::IVector<xaml_animation::ObjectKeyFrame*>> spKeyframeCollection;
    ctl::ComPtr<IInspectable> spBoolInsp1;
    ctl::ComPtr<IInspectable> spBoolInsp2;

    // We will prevent hit-testing by setting the IsHitTestVisible property of
    // the element to FALSE starting from the beginning of animation up to the
    // specified time value.
    if (!m_isHitTestVisibleValueSet) // Only support one set of IsHitTestVisible.
    {
        m_isHitTestVisibleValueSet = TRUE;
        wf::TimeSpan timeSpanForKeyTime1 = {};
        wf::TimeSpan timeSpanForKeyTime2 = {};
        wf::TimeSpan timeSpanForDuration = {};

        IFC(isHitTestVisiblePropertyName.Set(STR_LEN_PAIR(L"UIElement.IsHitTestVisible")));
        IFC(ctl::make(&spKeyFrame1));
        IFC(ctl::make(&spKeyFrame2));
        IFC(ctl::make(&spHitTestVisibleAnimation));

        keyTime1.TimeSpan = timeSpanForKeyTime1;
        IFC(DirectUI::PropertyValue::CreateFromBoolean(FALSE, &spBoolInsp1));
        IFC(spKeyFrame1->put_KeyTime(keyTime1));
        IFC(spKeyFrame1->put_Value(spBoolInsp1.Get()));

        timeSpanForKeyTime2.Duration = durationInMilliseconds * millisecondsTo100Nanoseconds;
        keyTime2.TimeSpan = timeSpanForKeyTime2;
        IFC(DirectUI::PropertyValue::CreateFromBoolean(TRUE, &spBoolInsp2));
        IFC(spKeyFrame2->put_KeyTime(keyTime2));
        IFC(spKeyFrame2->put_Value(spBoolInsp2.Get()));

        IFC(spHitTestVisibleAnimation->get_KeyFrames(&spKeyframeCollection));
        IFC(spKeyframeCollection->Append(spKeyFrame1.Get()));
        IFC(spKeyframeCollection->Append(spKeyFrame2.Get()));

        duration.TimeSpan = timeSpanForDuration;
        duration.Type = xaml::DurationType_TimeSpan;
        IFC(spHitTestVisibleAnimation->put_Duration(duration));

        IFC(m_pTimelines->Append(spHitTestVisibleAnimation.Get()));

        if (m_target)
        {
            IFC(m_pStoryboardStatics->SetTarget(spHitTestVisibleAnimation.Get(), m_target));
        }
        else if (m_targetName)
        {
            IFC(m_pStoryboardStatics->SetTargetName(spHitTestVisibleAnimation.Get(), m_targetName));
        }
        IFC(m_pStoryboardStatics->SetTargetProperty(spHitTestVisibleAnimation.Get(), isHitTestVisiblePropertyName.Get()));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ThemeGeneratorHelper::SetOverrideTarget(_In_ xaml::IDependencyObject* pTarget)
{
    HRESULT hr = S_OK;
    ReplaceInterface(m_target, pTarget);

    RRETURN(hr);
}

_Check_return_ HRESULT ThemeGeneratorHelper::SetOriginForProperty(_In_ wf::Point originPoint, _In_ HSTRING propertyName, _In_ BOOLEAN& valueSet)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<PointAnimation> spOriginTimeline;
    xaml::Duration duration = {};
    IInspectable* pPointInsp = NULL;
    wf::IReference<wf::Point>* pPoint = NULL;

    if (!valueSet)   // only support one set of origin. Consider throwing if incoming is different
    {
        valueSet = TRUE;
        wf::TimeSpan timeSpan = {};

        IFC(ctl::make(&spOriginTimeline));

        IFC(DirectUI::PropertyValue::CreateFromPoint(originPoint, &pPointInsp));
        IFC(ctl::do_query_interface(pPoint, pPointInsp));
        IFC(spOriginTimeline->put_To(pPoint));

        duration.TimeSpan = timeSpan;
        duration.Type = xaml::DurationType_TimeSpan;
        IFC(spOriginTimeline->put_Duration(duration));

        IFC(m_pTimelines->Append(spOriginTimeline.Get()));

        if (m_target)
        {
            IFC(m_pStoryboardStatics->SetTarget(spOriginTimeline.Get(), m_target));
        }
        else if (m_targetName)
        {
            IFC(m_pStoryboardStatics->SetTargetName(spOriginTimeline.Get(), m_targetName));
        }
        IFC(m_pStoryboardStatics->SetTargetProperty(spOriginTimeline.Get(), propertyName));
    }

Cleanup:
    ReleaseInterface(pPoint);
    ReleaseInterface(pPointInsp);
    RRETURN(hr);
}

//------------------------------------------------------------------------
// Translation logic. Responsibility: understanding pvl definition of TRANSLATE2D
//------------------------------------------------------------------------
_Check_return_ HRESULT CreateTranslate2DTimelines(
    _In_ TA_TRANSFORM* pTransform,
    _In_ TimingFunctionDescription* pEasing,
    _In_ ThemeGeneratorHelper* pHelper)
{
    HRESULT hr = S_OK;

    INT64 startTime = (INT64)pTransform->dwStartTime + pHelper->GetAdditionalTime();
    INT64 duration = (INT64)pTransform->dwDurationTime;

    // this is a transform, so we need a doubleanimationusingkeyframes for both x and y

    // ------------ origin ------------
    // we do not support origin on translate
    if (pTransform->eFlags & TATF_HASORIGINVALUES)
    {
        IFC(E_INVALIDARG);
    }

    // ------------ initial value -------
    if (pTransform->eFlags & TATF_HASINITIALVALUES)
    {
        IFC(E_INVALIDARG);
    }

    // ------------ expecting user values ---
    if (pTransform->eFlags ^ TATF_TARGETVALUES_USER)
    {
        IFC(E_INVALIDARG);
    }

    if (startTime > 0)
    {
        // lock the transition to the current value between 0 and startTime
        IFC(pHelper->RegisterKeyFrame(pHelper->GetTranslateXPropertyName(), pHelper->GetStartOffset().X, 0, 0, pEasing));
        IFC(pHelper->RegisterKeyFrame(pHelper->GetTranslateYPropertyName(), pHelper->GetStartOffset().Y, 0, 0, pEasing));

    }

    // ------------ X
    IFC(pHelper->RegisterKeyFrame(pHelper->GetTranslateXPropertyName(), pHelper->GetStartOffset().X, startTime, 0, pEasing));
    IFC(pHelper->RegisterKeyFrame(pHelper->GetTranslateXPropertyName(), pHelper->GetDestinationOffset().X, startTime, duration, pEasing));

    // ------------ Y
    IFC(pHelper->RegisterKeyFrame(pHelper->GetTranslateYPropertyName(), pHelper->GetStartOffset().Y, startTime, 0, pEasing));
    IFC(pHelper->RegisterKeyFrame(pHelper->GetTranslateYPropertyName(), pHelper->GetDestinationOffset().Y, startTime, duration, pEasing));


Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
// Scale logic. Responsibility: understanding pvl definition of SCALE2D
//------------------------------------------------------------------------
_Check_return_ HRESULT CreateScale2DTimelines(
    _In_ TA_TRANSFORM* pTransform,
    _In_ TimingFunctionDescription* pEasing,
    _In_ ThemeGeneratorHelper* pHelper)
{
    HRESULT hr = S_OK;

    TA_TRANSFORM_2D* pTransform2D = reinterpret_cast<TA_TRANSFORM_2D*>(pTransform);
    INT64 startTime = (INT64)pTransform->dwStartTime + pHelper->GetAdditionalTime();

    // ------------ origin ------------
    // we support origin values on a scale
    if (pTransform->eFlags & TATF_HASORIGINVALUES)
    {
        wf::Point point = {pTransform2D->rOriginX, pTransform2D->rOriginY};
        IFC(pHelper->Set2DTransformOriginValues(point));
    }

    // ------------ initial value -------
    if (pTransform->eFlags & TATF_HASINITIALVALUES)
    {
        // additionaltime means we have to lock the scale to the initial value
        if (startTime > 0)
        {
            IFC(pHelper->RegisterKeyFrame(pHelper->GetScaleXPropertyName(), (DOUBLE)pTransform2D->rInitialX, 0, 0, pEasing));
            IFC(pHelper->RegisterKeyFrame(pHelper->GetScaleYPropertyName(), (DOUBLE)pTransform2D->rInitialY, 0, 0, pEasing));
        }

        IFC(pHelper->RegisterKeyFrame(pHelper->GetScaleXPropertyName(), (DOUBLE)pTransform2D->rInitialX, startTime, 0, pEasing));
        IFC(pHelper->RegisterKeyFrame(pHelper->GetScaleYPropertyName(), (DOUBLE)pTransform2D->rInitialY, startTime, 0, pEasing));
    }

    // ------------ actual keyframe ------
    IFC(pHelper->RegisterKeyFrame(pHelper->GetScaleXPropertyName(), (DOUBLE)pTransform2D->rX, startTime, (INT64)pTransform->dwDurationTime, pEasing));
    IFC(pHelper->RegisterKeyFrame(pHelper->GetScaleYPropertyName(), (DOUBLE)pTransform2D->rY, startTime, (INT64)pTransform->dwDurationTime, pEasing));

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
// Opacity logic. Responsibility: understanding pvl definition of OPACITY
//------------------------------------------------------------------------
_Check_return_ HRESULT CreateOpacityTimelines(
    _In_ TA_TRANSFORM* pTransform,
    _In_ TimingFunctionDescription* pEasing,
    _In_ ThemeGeneratorHelper* pHelper)
{
    HRESULT hr = S_OK;

    TA_TRANSFORM_OPACITY* pOpacityTransform = reinterpret_cast<TA_TRANSFORM_OPACITY*>(pTransform);
    INT64 startTime = (INT64)pTransform->dwStartTime + pHelper->GetAdditionalTime();
    INT64 duration = (INT64)pTransform->dwDurationTime;

    // ------------ initial value -------
    DOUBLE initialOpacity = (DOUBLE)pOpacityTransform->rInitialOpacity;
    if (pHelper->GetInitialOpacity(initialOpacity) || pTransform->eFlags & TATF_HASINITIALVALUES)
    {
        if (startTime > 0)
        {
            IFC(pHelper->RegisterKeyFrame(pHelper->GetOpacityPropertyName(), (DOUBLE)pOpacityTransform->rInitialOpacity, 0, 0, pEasing));
        }
        IFC(pHelper->RegisterKeyFrame(pHelper->GetOpacityPropertyName(), (DOUBLE)pOpacityTransform->rInitialOpacity, startTime, 0, pEasing));
    }

    // ------------ actual keyframe ------
    IFC(pHelper->RegisterKeyFrame(pHelper->GetOpacityPropertyName(), (DOUBLE)pOpacityTransform->rOpacity, startTime, duration, pEasing));

Cleanup:
    RRETURN(hr);
}


//------------------------------------------------------------------------
// Clip logic. Responsibility: understanding pvl definition of Clip
//------------------------------------------------------------------------
_Check_return_ HRESULT CreateClipTimelines(
    _In_ TA_TRANSFORM* pTransform,
    _In_ TimingFunctionDescription* pEasing,
    _In_ ThemeGeneratorHelper* pHelper)
{
    HRESULT hr = S_OK;

    TA_TRANSFORM_2D* pTransform2D = reinterpret_cast<TA_TRANSFORM_2D*>(pTransform);
    INT64 startTime = (INT64)pTransform->dwStartTime + pHelper->GetAdditionalTime();

    // ------------ origin ------------
    // we expect origin values on a clip
    if (pTransform->eFlags & TATF_HASORIGINVALUES)
    {
        // we set the center once per animation.
        wf::Point point = {pTransform2D->rOriginX, pTransform2D->rOriginY};
        IFC(pHelper->SetClipOriginValues(point));
    }
    else
    {
        IFC(E_INVALIDARG);
    }

    if (pTransform->eFlags & TATF_HASINITIALVALUES)
    {
        // split out creating X and Y versions to support PVL definitions with different X and Y clips

        if (pTransform2D->rInitialX != pTransform2D->rX)
        {
            // additionaltime means we have to lock to the initial value
            if (startTime > 0)
            {
                IFC(pHelper->RegisterKeyFrame(pHelper->GetClipScaleXPropertyName(), (DOUBLE)pTransform2D->rInitialX, 0, 0, pEasing));
            }
            IFC(pHelper->RegisterKeyFrame(pHelper->GetClipScaleXPropertyName(), (DOUBLE)pTransform2D->rInitialX, startTime, 0, pEasing));
            IFC(pHelper->RegisterKeyFrame(pHelper->GetClipScaleXPropertyName(), (DOUBLE)pTransform2D->rX, startTime, (INT64)pTransform->dwDurationTime, pEasing));
        }

        if (pTransform2D->rInitialY != pTransform2D->rY)
        {
            // additionaltime means we have to lock to the initial value
            if (startTime > 0)
            {
                IFC(pHelper->RegisterKeyFrame(pHelper->GetClipScaleXPropertyName(), (DOUBLE)pTransform2D->rInitialY, 0, 0, pEasing));
            }
            IFC(pHelper->RegisterKeyFrame(pHelper->GetClipScaleYPropertyName(), (DOUBLE)pTransform2D->rInitialY, startTime, 0, pEasing));
            IFC(pHelper->RegisterKeyFrame(pHelper->GetClipScaleYPropertyName(), (DOUBLE)pTransform2D->rY, startTime, (INT64)pTransform->dwDurationTime, pEasing));
        }
    }
    else
    {
        IFC(E_INVALIDARG);
    }

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
// Helper. Responsibility: understanding uxtheme api for retrieving timingfunctions.
//------------------------------------------------------------------------
HRESULT InitializeTimingCurve(
    _In_ HTHEME hTheme,
    _In_ DWORD dwTimingFunctionId,
    _In_ TimingFunctionDescription* pTimingDescription)
{
    HRESULT hr = S_OK;
    DWORD cbSizeTiming = 0;
    DWORD cbSizeOut = 0;
    BYTE* pBufferTiming = NULL;
    IFCPTR(pTimingDescription);

    hr = GetThemeTimingFunction(hTheme, dwTimingFunctionId, NULL, 0, &cbSizeTiming);

    if (HRESULT_FROM_WIN32(ERROR_MORE_DATA) == hr)
    {
        hr = S_OK;  // this was expected
        pBufferTiming = new BYTE[cbSizeTiming];
        TA_TIMINGFUNCTION *pTimingFunction = reinterpret_cast<TA_TIMINGFUNCTION *>(pBufferTiming);

        // Retrieve the timing function for the transform
        IFC(GetThemeTimingFunction(hTheme, dwTimingFunctionId, pTimingFunction, cbSizeTiming, &cbSizeOut));

        switch (pTimingFunction->eTimingFunctionType)
        {
            case TTFT_CUBIC_BEZIER:
            {
                TA_CUBIC_BEZIER *pCubicBezier = reinterpret_cast<TA_CUBIC_BEZIER*>(pTimingFunction);
                pTimingDescription->cp2.X = pCubicBezier->rX0;
                pTimingDescription->cp2.Y = pCubicBezier->rY0;
                pTimingDescription->cp3.X = pCubicBezier->rX1;
                pTimingDescription->cp3.Y = pCubicBezier->rY1;
            }
            break;

            default:
            {
                hr = E_NOTIMPL;
            }
            break;
        }
    }

Cleanup:
    delete[] pBufferTiming;
    RRETURN(hr);
}

//------------------------------------------------------------------------
// Helper. Responsibility: understanding uxtheme api for retrieving pvl definition.
//------------------------------------------------------------------------
HRESULT RetrieveTransform(
    _In_ HTHEME hTheme,
    _In_ INT storyboardID,
    _In_ INT targetID,
    _In_ INT index,
    _Outptr_ TA_TRANSFORM **ppTransform)
{
    DWORD cbSizeTransform = 0;
    DWORD cbSizeOut = 0;
    BYTE* pBuffer = NULL;

    HRESULT hr = GetThemeAnimationTransform(hTheme, storyboardID, targetID, index, NULL, 0, &cbSizeTransform);

    if (HRESULT_FROM_WIN32(ERROR_MORE_DATA) == hr)
    {
        hr = S_OK;  // this was expected

        pBuffer = new BYTE[cbSizeTransform];
        *ppTransform = reinterpret_cast<TA_TRANSFORM *>(pBuffer);

        // Call the function again with the size that is returned from the previous call
        IFC(GetThemeAnimationTransform(hTheme, storyboardID, targetID, index, *ppTransform, cbSizeTransform, &cbSizeOut));
    }
    else
    {
        IFC(E_UNEXPECTED);
    }

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
// Main logic. Responsibility: dissecting PVL specifics.
//------------------------------------------------------------------------
_Check_return_ HRESULT AddTimelines(
    _In_ INT storyboardID,
    _In_ INT targetID,
    _In_ ThemeGeneratorHelper* pHelper)
{
    HRESULT hr = S_OK;
    TA_TRANSFORM* pTransform = NULL;
    TimingFunctionDescription* pTimingFunctionDescription = NULL;

    HTHEME hThemeAnimation = OpenThemeData(NULL, L"Animations"); // todo: apparently this works, why don't I need the window? Should look at dui implementation here
    HTHEME hThemeTiming = OpenThemeData(NULL, L"timingfunction"); // todo: apparently this works, why don't I need the window? Should look at dui implementation here

    #ifdef TG_DBG
    WCHAR szTrace[256];
    IFCEXPECT(swprintf_s(szTrace, 256, L"Adding timeline, storyboardID %d, targetID %d", (int)storyboardID, (int)targetID) >= 0);
    Trace(szTrace);
    #endif

    if (hThemeAnimation)
    {
        DWORD dwTransforms = 0;
        DWORD cbSize = sizeof(dwTransforms);
        DWORD cbSizeOut = 0;

        IFC(GetThemeAnimationProperty(hThemeAnimation, storyboardID, targetID, TAP_TRANSFORMCOUNT, &dwTransforms, cbSize, &cbSizeOut));

        for (DWORD index = 0; index < dwTransforms; ++index)
        {
            pTimingFunctionDescription = new TimingFunctionDescription();   // perf is just not an issue here, so not reusing current description

            IFC(RetrieveTransform(hThemeAnimation, storyboardID, targetID, index, &pTransform));

            IFC(InitializeTimingCurve(hThemeTiming, pTransform->dwTimingFunctionId, pTimingFunctionDescription));

            switch (pTransform->eTransformType)
            {
            case TATT_TRANSLATE_2D:
                IFC(CreateTranslate2DTimelines(pTransform, pTimingFunctionDescription, pHelper));
                break;

            case TATT_SCALE_2D:
                IFC(CreateScale2DTimelines(pTransform, pTimingFunctionDescription, pHelper));
                break;

            case TATT_CLIP:
                IFC(CreateClipTimelines(pTransform, pTimingFunctionDescription, pHelper));
                break;

            case TATT_OPACITY:
                IFC(CreateOpacityTimelines(pTransform, pTimingFunctionDescription, pHelper));
                break;

            default:
                {
                }
                break;
            }

            delete[] reinterpret_cast<BYTE**>(pTransform);
            pTransform = NULL;
            delete pTimingFunctionDescription;
            pTimingFunctionDescription = NULL;
        }

    }

Cleanup:
    CloseThemeData(hThemeAnimation);
    CloseThemeData(hThemeTiming);
    delete pTimingFunctionDescription;
    delete[] reinterpret_cast<BYTE**>(pTransform);
    RRETURN(hr);
}

//------------------------------------------------------------------------
// Adds timelines as defined by pvl
//------------------------------------------------------------------------
_Check_return_ HRESULT ThemeGenerator::AddTimelinesForThemeAnimation(
    _In_ INT storyboardID,
    _In_ INT targetID,
    _In_ ThemeGeneratorHelper* pHelper)
{
    HRESULT hr = S_OK;

    IFC(AddTimelines(storyboardID, targetID, pHelper));

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
// Adds timelines as defined by pvl
//------------------------------------------------------------------------
_Check_return_ HRESULT ThemeGenerator::AddTimelinesForThemeAnimation(
    _In_ INT storyboardID,
    _In_ INT targetID,
    _In_ HSTRING targetName,
    _In_opt_ xaml::IDependencyObject* pTarget,
    _In_ BOOLEAN onlyGenerateSteadyState,
    _In_ wf::Point startOffset,
    _In_ wf::Point destinationOffset,
    _In_ wfc::IVector<xaml_animation::Timeline*>* timelineCollection)
{
    HRESULT hr = S_OK;

    ThemeGeneratorHelper* pSupplier = NULL;
    pSupplier = new ThemeGeneratorHelper(startOffset, destinationOffset, targetName, pTarget, onlyGenerateSteadyState, timelineCollection);
    IFC(pSupplier->Initialize());

    IFC(AddTimelinesForThemeAnimation(storyboardID, targetID, pSupplier));

Cleanup:
    delete pSupplier;
    RRETURN(hr);
}

_Check_return_ HRESULT ThemeGenerator::AddTimelinesForThemeAnimation(
    _In_ INT storyboardID,
    _In_ INT targetID,
    _In_ BOOLEAN onlyGenerateSteadyState,
    _In_ wf::Point startOffset,
    _In_ wf::Point destinationOffset,
    _In_ INT64 additionalTime,
    _In_ wfc::IVector<xaml_animation::Timeline*>* timelineCollection)
{
    HRESULT hr = S_OK;

    ThemeGeneratorHelper* pSupplier = NULL;
    pSupplier = new ThemeGeneratorHelper(startOffset, destinationOffset, NULL, NULL, onlyGenerateSteadyState, timelineCollection);
    IFC(pSupplier->Initialize());
    pSupplier->SetAdditionalTime(additionalTime);

    IFC(AddTimelinesForThemeAnimation(storyboardID, targetID, pSupplier));

Cleanup:
    delete pSupplier;
    RRETURN(hr);
}

//------------------------------------------------------------------------
// Creates a configured stagger function according to pvl
//------------------------------------------------------------------------
_Check_return_ HRESULT ThemeGenerator::GetStaggerFunction(
    _In_ INT storyboardID,
    _In_ INT targetID,
    _Outptr_ StaggerFunctionBase** ppInstance)
{
    HRESULT hr = S_OK;

    DWORD dwFlags = 0;
    DWORD dwStaggerDelay = 0;
    DWORD dwStaggerDelayCap = 0;
    FLOAT rStaggerDelayReduce = 0;
    DWORD cbSizeOut = 0;

    ctl::ComPtr<DirectUI::PVLStaggerFunction> spStagger;
    HTHEME hThemeAnimation = OpenThemeData(NULL, L"Animations");

    if (SUCCEEDED(GetThemeAnimationProperty(hThemeAnimation, storyboardID, targetID, TAP_FLAGS, &dwFlags, sizeof(dwFlags), &cbSizeOut)))
    {
        if ((dwFlags & TAPF_HASSTAGGER) && (dwFlags & TAPF_ALLOWCOLLECTION))
        {
            if (SUCCEEDED(GetThemeAnimationProperty(hThemeAnimation, storyboardID, targetID, TAP_STAGGERDELAY, (void *)&dwStaggerDelay, sizeof(dwStaggerDelay), &cbSizeOut)))
            {
                if (SUCCEEDED(GetThemeAnimationProperty(hThemeAnimation, storyboardID, targetID, TAP_STAGGERDELAYCAP, (void *)&dwStaggerDelayCap, sizeof(dwStaggerDelayCap), &cbSizeOut)))
                {
                    IFC(GetThemeAnimationProperty(hThemeAnimation, storyboardID, targetID, TAP_STAGGERDELAYFACTOR, (void *)&rStaggerDelayReduce, sizeof(rStaggerDelayReduce), &cbSizeOut));

                    IFC(ctl::make(&spStagger));
                    IFC(spStagger->put_Delay((DOUBLE)dwStaggerDelay));
                    IFC(spStagger->put_Maximum((DOUBLE)dwStaggerDelayCap));
                    IFC(spStagger->put_DelayReduce((DOUBLE)rStaggerDelayReduce));    // todo: float?
                }
            }
        }
    }

    *ppInstance = spStagger.Detach();

Cleanup:
    CloseThemeData(hThemeAnimation);
    RRETURN(hr);
}
