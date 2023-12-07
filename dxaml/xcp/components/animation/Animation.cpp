// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "Animation.h"
#include "KeyFrame.h"
#include "KeyTime.h"
#include "Duration.h"
#include "KeyFrameCollection.h"
#include "DCompAnimationConversionContext.h"
#include <RepeatBehavior.h>
#include "real.h"
#include "EasingFunctions.h"
#include "timemgr.h"
#include "TypeTableStructs.h"
#include <FloatUtil.h>
#include <MUX-ETWEvents.h>
#include "TimeSpan.h"
#include "XStringBuilder.h"

using namespace std::placeholders;

// WUC enforces these limits on min and max animation duration (see CAnimation::ApplyTimelinePropertiesPublicWUCAPI)
static const float s_minWUCKeyFrameAnimationDurationInSeconds = 0.001f;             // 1 millisecond
const long long CAnimation::s_minWUCKeyFrameAnimationDurationInTicks = static_cast<long long>(10000000i64 * s_minWUCKeyFrameAnimationDurationInSeconds);
const float CAnimation::s_maxWUCKeyFrameAnimationDurationInSeconds = 60*60*24*24;   // 24 days
const UINT64 CAnimation::s_maxWUCKeyFrameAnimationDurationInTicks = 10000000ui64 * static_cast<UINT64>(CAnimation::s_maxWUCKeyFrameAnimationDurationInSeconds);

CAnimation::CAnimation(_In_ CCoreServices *pCore)
    : CTimeline(pCore)
    , m_fUsesKeyFrames(false)
    , m_hasControlOfTarget(true)
    , m_enableDependentAnimation(false)
    , m_pKeyFrames(NULL)
    , m_pDPTo(NULL)
    , m_pDPFrom(NULL)
    , m_pDPBy(NULL)
    , m_pDPValue(NULL)
    , m_pEasingFunction(NULL)
    , m_fFiredDependentAnimationWarning(false)
    , m_hasHandoff(false)
    , m_isToAnimation(false)
    , m_isZeroDuration(false)
    , m_fAnimationApplied(false)
{
}

bool CAnimation::IsZeroDuration() const
{
    return m_isZeroDuration;
}

// Compute the duration when one is not specified in the markup.
// Animations override this method to provide their own default.
void CAnimation::GetNaturalDuration(
    _Out_ DirectUI::DurationType* pDurationType,
    _Out_ XFLOAT* pDurationValue)
{
    const DurationVO* durationValue = nullptr;

    if (m_duration)
    {
        durationValue = &m_duration->Value();
    }

    if (durationValue &&
        durationValue->GetDurationType() == DirectUI::DurationType::TimeSpan)
    {
        *pDurationType = durationValue->GetDurationType();
        *pDurationValue = static_cast<XFLOAT>(durationValue->GetTimeSpanInSec());
    }
    else if (m_fUsesKeyFrames && m_pKeyFrames != nullptr)
    {
        // Versions of Xaml before Threshold will prioritize key frames ahead of Duration="Forever".
        // If a repeating animation has an explicit Duration="Forever" and key frames lasting 3 seconds, then
        // the animation will repeat every 3 seconds in spite of the explicit duration. If the same repeating
        // animation is done without key frames, then the animation never repeats because its duration is forever.

        GetNaturalDurationFromKeyFrames(
            m_pKeyFrames->GetSortedCollection(),
            pDurationType,
            pDurationValue);
    }
    else if (durationValue &&
             durationValue->GetDurationType() != DirectUI::DurationType::Automatic)
    {
        *pDurationType = durationValue->GetDurationType();
        *pDurationValue = static_cast<XFLOAT>(durationValue->GetTimeSpanInSec());
    }
    else
    {
        *pDurationType = DirectUI::DurationType::TimeSpan;
        *pDurationValue = NULL_DURATION_DEFAULT;
    }
}

void CAnimation::GetNaturalDurationFromKeyFrames(
    const CDOCollection::storage_type& sortedKeyFrames,
    _Out_ DirectUI::DurationType* pDurationType,
    _Out_ XFLOAT* pDurationValue
    )
{
    // Duration defaults to 1 second
    *pDurationType = DirectUI::DurationType::TimeSpan;
    *pDurationValue = 1.0;

    const int keyFrameCount = static_cast<const int>(sortedKeyFrames.size());

    if (keyFrameCount > 0)
    {
        // Find the largest TimeSpan duration in our keyframes
        // loop backwards since the last keyframe should be the one
        XFLOAT localDuration = 0;
        for (int i = keyFrameCount; i > 0; i--)
        {
            const KeyTimeVO::Wrapper* pCurrent = static_cast<CKeyFrame*>(sortedKeyFrames[i - 1])->m_keyTime;

            // In WinBlue, having a no key time would generate an E_UNEXPECTED that's not actually bubbled up the stack,
            // so it's an error case that's actually tolerated. We keep the same behavior here. In WinBlue this would not
            // set the natural duration at all when returning, which left it as 0 in two places that initialize it to 0
            // and leaves random memory in the third place. We'll ignore the key frame in Threshold. If it's the only
            // key frame, then the duration will be reported as 0.
            if (pCurrent != nullptr)
            {
                const KeyTimeVO& value = pCurrent->Value();

                if (localDuration < static_cast<XFLOAT>(value.GetTimeSpanInSec()))
                {
                    localDuration = static_cast<XFLOAT>(value.GetTimeSpanInSec());
                    break;
                }
            }
        }
        *pDurationValue = localDuration;
    }
}

bool CAnimation::ShouldDiscardDCompAnimationOnDetach()
{
    // Xaml animations can read their initial/final values from the animation target if they're left unset. These values
    // are then baked into the DComp animation that is generated. If the animation is stopped and later restarted, it will
    // read the initial/final values again, rather than reuse the value from before. This new value must be pushed to the
    // DComp animation, which we do by discarding the old DComp animation whenever the Xaml animation is detached from the
    // tree.
    //
    // Continuous handoff will allow the DComp animation to read its initial value, but not its final value. Continuous
    // handoff also only applies if the property is already being animated, which is something that is not necessarily true.
    // So we can't rely on continuous handoffs.
    //
    // Finally, key frame animations all implicitly start from the animation target's current value, so they should all
    // discard their DComp animations when detached.
    //
    return m_fUsesKeyFrames
        // Unit tests won't have m_pDPFrom/To. Treat those as having a non-default value so they don't cause the DComp
        // animation to be discarded.
        || (m_pDPFrom != nullptr && IsPropertyDefault(m_pDPFrom))
        || (m_pDPTo != nullptr && IsPropertyDefault(m_pDPTo));
}

KnownPropertyIndex CAnimation::MapToDCompAnimationProperty(KnownPropertyIndex xamlAnimatedProperty)
{
    switch (xamlAnimatedProperty)
    {
        case KnownPropertyIndex::Canvas_Left: { return KnownPropertyIndex::UIElement_OffsetXAnimation; }
        case KnownPropertyIndex::Canvas_Top:  { return KnownPropertyIndex::UIElement_OffsetYAnimation; }

        case KnownPropertyIndex::TranslateTransform_X: { return KnownPropertyIndex::TranslateTransform_XAnimation; }
        case KnownPropertyIndex::TranslateTransform_Y: { return KnownPropertyIndex::TranslateTransform_YAnimation; }

        case KnownPropertyIndex::ScaleTransform_CenterX: { return KnownPropertyIndex::ScaleTransform_CenterXAnimation; }
        case KnownPropertyIndex::ScaleTransform_CenterY: { return KnownPropertyIndex::ScaleTransform_CenterYAnimation; }
        case KnownPropertyIndex::ScaleTransform_ScaleX:  { return KnownPropertyIndex::ScaleTransform_ScaleXAnimation; }
        case KnownPropertyIndex::ScaleTransform_ScaleY:  { return KnownPropertyIndex::ScaleTransform_ScaleYAnimation; }

        case KnownPropertyIndex::RotateTransform_Angle:   { return KnownPropertyIndex::RotateTransform_AngleAnimation; }
        case KnownPropertyIndex::RotateTransform_CenterX: { return KnownPropertyIndex::RotateTransform_CenterXAnimation; }
        case KnownPropertyIndex::RotateTransform_CenterY: { return KnownPropertyIndex::RotateTransform_CenterYAnimation; }

        case KnownPropertyIndex::SkewTransform_CenterX: { return KnownPropertyIndex::SkewTransform_CenterXAnimation; }
        case KnownPropertyIndex::SkewTransform_CenterY: { return KnownPropertyIndex::SkewTransform_CenterYAnimation; }
        case KnownPropertyIndex::SkewTransform_AngleX:  { return KnownPropertyIndex::SkewTransform_AngleXAnimation; }
        case KnownPropertyIndex::SkewTransform_AngleY:  { return KnownPropertyIndex::SkewTransform_AngleYAnimation; }

        case KnownPropertyIndex::CompositeTransform_CenterX:    { return KnownPropertyIndex::CompositeTransform_CenterXAnimation; }
        case KnownPropertyIndex::CompositeTransform_CenterY:    { return KnownPropertyIndex::CompositeTransform_CenterYAnimation; }
        case KnownPropertyIndex::CompositeTransform_ScaleX:     { return KnownPropertyIndex::CompositeTransform_ScaleXAnimation; }
        case KnownPropertyIndex::CompositeTransform_ScaleY:     { return KnownPropertyIndex::CompositeTransform_ScaleYAnimation; }
        case KnownPropertyIndex::CompositeTransform_SkewX:      { return KnownPropertyIndex::CompositeTransform_SkewXAnimation; }
        case KnownPropertyIndex::CompositeTransform_SkewY:      { return KnownPropertyIndex::CompositeTransform_SkewYAnimation; }
        case KnownPropertyIndex::CompositeTransform_Rotation:   { return KnownPropertyIndex::CompositeTransform_RotateAnimation; }
        case KnownPropertyIndex::CompositeTransform_TranslateX: { return KnownPropertyIndex::CompositeTransform_TranslateXAnimation; }
        case KnownPropertyIndex::CompositeTransform_TranslateY: { return KnownPropertyIndex::CompositeTransform_TranslateYAnimation; }

        case KnownPropertyIndex::UIElement_Opacity: { return KnownPropertyIndex::UIElement_OpacityAnimation; }

        case KnownPropertyIndex::TransitionTarget_Opacity: { return KnownPropertyIndex::TransitionTarget_OpacityAnimation; }

        case KnownPropertyIndex::PlaneProjection_CenterOfRotationX: { return KnownPropertyIndex::PlaneProjection_CenterOfRotationXAnimation; }
        case KnownPropertyIndex::PlaneProjection_CenterOfRotationY: { return KnownPropertyIndex::PlaneProjection_CenterOfRotationYAnimation; }
        case KnownPropertyIndex::PlaneProjection_CenterOfRotationZ: { return KnownPropertyIndex::PlaneProjection_CenterOfRotationZAnimation; }
        case KnownPropertyIndex::PlaneProjection_GlobalOffsetX: { return KnownPropertyIndex::PlaneProjection_GlobalOffsetXAnimation; }
        case KnownPropertyIndex::PlaneProjection_GlobalOffsetY: { return KnownPropertyIndex::PlaneProjection_GlobalOffsetYAnimation; }
        case KnownPropertyIndex::PlaneProjection_GlobalOffsetZ: { return KnownPropertyIndex::PlaneProjection_GlobalOffsetZAnimation; }
        case KnownPropertyIndex::PlaneProjection_LocalOffsetX: { return KnownPropertyIndex::PlaneProjection_LocalOffsetXAnimation; }
        case KnownPropertyIndex::PlaneProjection_LocalOffsetY: { return KnownPropertyIndex::PlaneProjection_LocalOffsetYAnimation; }
        case KnownPropertyIndex::PlaneProjection_LocalOffsetZ: { return KnownPropertyIndex::PlaneProjection_LocalOffsetZAnimation; }
        case KnownPropertyIndex::PlaneProjection_RotationX: { return KnownPropertyIndex::PlaneProjection_RotationXAnimation; }
        case KnownPropertyIndex::PlaneProjection_RotationY: { return KnownPropertyIndex::PlaneProjection_RotationYAnimation; }
        case KnownPropertyIndex::PlaneProjection_RotationZ: { return KnownPropertyIndex::PlaneProjection_RotationZAnimation; }

        case KnownPropertyIndex::CompositeTransform3D_CenterX: { return KnownPropertyIndex::CompositeTransform3D_CenterXAnimation; }
        case KnownPropertyIndex::CompositeTransform3D_CenterY: { return KnownPropertyIndex::CompositeTransform3D_CenterYAnimation; }
        case KnownPropertyIndex::CompositeTransform3D_CenterZ: { return KnownPropertyIndex::CompositeTransform3D_CenterZAnimation; }
        case KnownPropertyIndex::CompositeTransform3D_RotationX: { return KnownPropertyIndex::CompositeTransform3D_RotationXAnimation; }
        case KnownPropertyIndex::CompositeTransform3D_RotationY: { return KnownPropertyIndex::CompositeTransform3D_RotationYAnimation; }
        case KnownPropertyIndex::CompositeTransform3D_RotationZ: { return KnownPropertyIndex::CompositeTransform3D_RotationZAnimation; }
        case KnownPropertyIndex::CompositeTransform3D_ScaleX: { return KnownPropertyIndex::CompositeTransform3D_ScaleXAnimation; }
        case KnownPropertyIndex::CompositeTransform3D_ScaleY: { return KnownPropertyIndex::CompositeTransform3D_ScaleYAnimation; }
        case KnownPropertyIndex::CompositeTransform3D_ScaleZ: { return KnownPropertyIndex::CompositeTransform3D_ScaleZAnimation; }
        case KnownPropertyIndex::CompositeTransform3D_TranslateX: { return KnownPropertyIndex::CompositeTransform3D_TranslateXAnimation; }
        case KnownPropertyIndex::CompositeTransform3D_TranslateY: { return KnownPropertyIndex::CompositeTransform3D_TranslateYAnimation; }
        case KnownPropertyIndex::CompositeTransform3D_TranslateZ: { return KnownPropertyIndex::CompositeTransform3D_TranslateZAnimation; }

        // This maps to all of A/R/G/B, but since either all are animated or none are animated, we can just use the A channel when checking for existing animations.
        case KnownPropertyIndex::SolidColorBrush_Color: { return KnownPropertyIndex::SolidColorBrush_ColorAAnimation; }
    }

    return KnownPropertyIndex::UnknownType_UnknownProperty;
}

_Check_return_ HRESULT CAnimation::OnRemoveFromTimeManager()
{
    HRESULT hr = __super::OnRemoveFromTimeManager();

    if (GetTimeManager() && GetTargetObjectWeakRef() && GetTargetObjectWeakRef().expired() && GetTargetDependencyProperty())
    {
        if (GetTimeManager()->GetAnimationOnProperty(GetTargetObjectWeakRef(), GetTargetDependencyProperty()->GetIndex()) == this)
        {
            GetTimeManager()->ClearAnimationOnProperty(GetTargetObjectWeakRef(), GetTargetDependencyProperty()->GetIndex());
        }
    }
    IFC_RETURN(hr);
    return S_OK;
}

CompositionAnimationConversionResult CAnimation::MakeCompositionAnimationVirtual(_Inout_ CompositionAnimationConversionContext* myContext)
{
    CompositionAnimationConversionResult result = CompositionAnimationConversionResult::Success;

    if (m_isDCompAnimationDirty || myContext->IsForceCompositionAnimationDirty())
    {
        result = MakeCompositionKeyFrameAnimationVirtual(myContext);

        if (result == CompositionAnimationConversionResult::Success)
        {
            m_wucAnimator.Reset();

            // In order for this Xaml animation to create a WUC animation, the Xaml animation must be dirty. That means either:
            //    - This is the first time that we're making a WUC animation, in which case there should be no scoped batch.
            //    - Or, this Xaml animation has changed somehow, in which case it must have been stopped first, which should
            //      have reset the scoped batch. Active animations are treated as immutable and cannot be changed.
            //    - Or, this animation was restarted while it was already playing, in which case we should detach the old WUC
            //      completed handler because we don't care anymore. We'll attach a new handler for the new animation.
            if (m_wucAnimationCompletedToken.value != 0)
            {
                DetachDCompCompletedHandlerOnStop();
            }

            MarkShouldAttachDCompAnimationInstance();

            m_isDCompAnimationDirty = false;
            m_isDCompAnimationDirtyInSubtree = false;
            m_forceDCompAnimationDirtyInSubtree = false;
        }
    }

    m_conversionResult = result;
    return result;
}

CompositionAnimationConversionResult CAnimation::MakeCompositionKeyFrameAnimationVirtual(_Inout_ CompositionAnimationConversionContext* myContext)
{
    return CompositionAnimationConversionResult::Success;
}

void CAnimation::ReleaseDCompResources()
{
    __super::ReleaseDCompResources();

    m_spWUCAnimation.Reset();
}

void CAnimation::TraceTickPausedAnimationHelper()
{
    if (EventEnabledTickPausedAnimationInfo())
    {
        auto target = GetTargetObjectWeakRef().lock();

        xstring_ptr targetName;
        if (target != nullptr)
        {
            targetName = target->m_strName;
        }

        xstring_ptr propertyName;
        if (m_pTargetDependencyProperty != nullptr)
        {
            propertyName = m_pTargetDependencyProperty->GetName();
        }

        TraceTickPausedAnimationInfo(
            reinterpret_cast<UINT64>(target.get()),
            !targetName.IsNullOrEmpty() ? targetName.GetBuffer() : L"[Unnamed Target]",
            !propertyName.IsNullOrEmpty() ? propertyName.GetBuffer() : L"[Custom Property]");
    }
}

void CAnimation::UpdateWUCAnimationTelemetryString(_In_ CompositionAnimationConversionContext* myContext)
{
    XStringBuilder telemetryBuilder;

    if (!myContext->m_storyboardTelemetryName.IsNull())
    {
        IFCFAILFAST(telemetryBuilder.Append(XSTRING_PTR_EPHEMERAL(L",SB:")));
        IFCFAILFAST(telemetryBuilder.Append(myContext->m_storyboardTelemetryName));
    }

    CDependencyObject* target = GetTargetObjectWeakRef().lock();
    if (target != nullptr)
    {
        if (target->m_strName.IsNull())
        {
            IFCFAILFAST(telemetryBuilder.Append(XSTRING_PTR_EPHEMERAL(L",Unnamed target")));
        }
        else
        {
            IFCFAILFAST(telemetryBuilder.Append(XSTRING_PTR_EPHEMERAL(L",Target:")));
            IFCFAILFAST(telemetryBuilder.Append(target->m_strName));
        }

        xstring_ptr targetPath = target->GetUIPathForTracing(true /* followDOParentChain */);
        IFCFAILFAST(telemetryBuilder.Append(targetPath));
    }

    if (m_pTargetDependencyProperty != nullptr)
    {
        xstring_ptr propertyName = m_pTargetDependencyProperty->GetName();
        IFCFAILFAST(telemetryBuilder.Append(XSTRING_PTR_EPHEMERAL(L",Prop:")));
        IFCFAILFAST(telemetryBuilder.Append(propertyName));
    }

    xstring_ptr telemetry;
    IFCFAILFAST(telemetryBuilder.DetachString(&telemetry));
    if (telemetry.IsNull())
    {
        telemetry = xstring_ptr::EmptyString();
    }

    wrl::ComPtr<WUComp::ICompositionObject2> animationAsCO;
    m_spWUCAnimation.As(&animationAsCO);
    wrl_wrappers::HStringReference telemetryString(telemetry.GetBuffer());
    IFCFAILFAST(animationAsCO->put_Comment(telemetryString.Get()));
}
