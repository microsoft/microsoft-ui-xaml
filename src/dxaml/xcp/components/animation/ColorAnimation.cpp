// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ColorAnimation.h"
#include "Duration.h"
#include "CValue.h"
#include "TypeTableStructs.h"
#include <RuntimeEnabledFeatures.h>
#include <DependencyLocator.h>
#include "EasingFunctions.h"
#include "TimeMgr.h"
#include "ColorUtil.h"
#include "SolidColorBrush.h"
#include <DependencyObjectDCompRegistry.h>
#include <XamlBehaviorMode.h>
#include <ColorUtil.h>
#include "DCompAnimationConversionContext.h"
#include <DCompTreeHost.h>
#include <DoubleUtil.h>

CColorAnimation::~CColorAnimation()
{
    if (GetDCompObjectRegistry() != nullptr)
    {
        GetDCompObjectRegistry()->UnregisterObject(this);
    }
}

void CColorAnimation::AttachDCompAnimations()
{
    // m_hasIndependentAnimation implies m_hasControlOfTarget, due to how it's calculated in CAnimation::UpdateAnimation. We
    // check both as a formality. m_hasIndependentAnimation needs to be checked to ensure that the animation is actually
    // active. Otherwise, an animation can reattach itself immediately after detaching itself earlier in the tick.
    if (m_hasControlOfTarget && HasIndependentAnimation() && m_shouldAttachDCompAnimationInstance)
    {
        AttachDCompAnimationInstancesToTarget();
        m_shouldAttachDCompAnimationInstance = false;
    }
}

void CColorAnimation::ApplyDCompAnimationInstanceToTarget(_In_opt_ IUnknown* dcompAnimation)
{
    if (GetTargetObjectWeakRef() && m_pTargetDependencyProperty != nullptr)
    {
        CDependencyObject* pDO = GetTargetObjectWeakRef().lock();

        if (pDO != nullptr)
        {
            if (dcompAnimation == nullptr)
            {
                m_fAnimationApplied = false;
            }
            else
            {
                // A DComp animation instance that has handoff already evaluated is not allowed to be set on anything, even the same
                // property on the same object. Therefore, check if the animation instance is already set on the target object/property
                // and no-op.
                if (m_fAnimationApplied)
                {
                    IUnknown* currentDCompAnimationNoRef = pDO->GetDCompAnimation(KnownPropertyIndex::SolidColorBrush_ColorAnimation);

                    // DComp objects have tear-off interfaces, so merely casting a pointer isn't enough. Actually QI to IUnknown.
                    xref_ptr<IUnknown> dcompAnimationUnknown;
                    VERIFYHR(dcompAnimation->QueryInterface(IID_PPV_ARGS(dcompAnimationUnknown.ReleaseAndGetAddressOf())));
                    if (currentDCompAnimationNoRef == dcompAnimationUnknown.get())
                    {
                        return;
                    }
                }
                m_fAnimationApplied = true;
            }

            // Record the target dependency object. Normally the render walk will walk to it and update its WUC expression, which
            // will connect and start the WUC animation that we set here. However, if the DO is in a collapsed part of the tree, it
            // won't get walked. We still want to start the animation, so we'll need to walk to it explicitly via the list of
            // registered targets.
            //
            // In the case where pDCompAnimation is null, we also want to record the target dependency object. We want to visit it
            // later to explicitly disconnect the old WUC animation from the WUC expression, otherwise infinite WUC animations can
            // keep ticking. Note that the WUC expression itself will also be disconnected from the WUC visual if the Xaml element
            // is no longer animating.
            CTimeManager* timeManagerNoRef = GetTimeManager();
            if (timeManagerNoRef != nullptr)    // Test stubs return null
            {
                timeManagerNoRef->AddTargetDO(pDO);
            }

            switch (m_pTargetDependencyProperty->GetIndex())
            {
                case KnownPropertyIndex::SolidColorBrush_Color:
                {
                    CSolidColorBrush* pBrush = static_cast<CSolidColorBrush*>(pDO);
                    pBrush->SetDCompAnimation(dcompAnimation, KnownPropertyIndex::SolidColorBrush_ColorAnimation);
                    pBrush->SetIsColorAnimationDirty(true);
                    CSolidColorBrush::NWSetRenderDirty(pBrush, DirtyFlags::Render);
                    break;
                }
            }
        }
    }
}

void CColorAnimation::AttachDCompAnimationInstancesToTarget()
{
    if (m_spWUCAnimation != nullptr)
    {
        ApplyDCompAnimationInstanceToTarget(m_spWUCAnimation.Get());
    }
}

void CColorAnimation::DetachDCompAnimationInstancesFromTarget()
{
    if (m_hasControlOfTarget)
    {
        // Note that if this animation no longer controls its target, we don't want to detach the target's animations. Otherwise
        // we could unintentionally remove the animation that has taken over control of the target.
        ApplyDCompAnimationInstanceToTarget(nullptr);   // WUC
    }

    if (ShouldDiscardDCompAnimationOnDetach())
    {
        m_spWUCAnimation.Reset();
        m_wucAnimator.Reset();
        DetachDCompCompletedHandlerOnStop();
        CTimeline::SetDCompAnimationDirty(this, DirtyFlags::None /* flags - ignored */);
    }
}

void CColorAnimation::ResolvePendingDCompAnimationOperations()
{
    if (m_wucAnimator)
    {
        if (m_hasPendingSeekForDComp)
        {
            // Xaml seeks to positive infinity in order to complete storyboards (see CStoryboard::CompleteInternal).
            // This isn't allowed by DComp, which will get us an E_INVALIDARG. No-op this case. The DComp animation
            // will be detached from the tree anyway, since this timeline is at the end and is either Filling or Stopped.
            if (m_pendingDCompSeekTime != XFLOAT_INF)
            {
                // Xaml allows an animation to be seeked back arbitrarily far. DComp does not allow an animation to be
                // seeked back beyond the initial segment. We'll add a segment at time 0, but the app might still request
                // to seek to a negative time. In those cases we'll clamp the seek to 0 to avoid the E_INVALIDARG. The
                // other option is to no-op the seek, which seems worse than clamping.
                SeekDCompAnimationInstances(m_pendingDCompSeekTime < 0 ? 0 : m_pendingDCompSeekTime);
            }
            m_hasPendingSeekForDComp = false;
        }

        if (m_hasPendingPauseForDComp)
        {
            FAIL_FAST_ASSERT(!m_hasPendingResumeForDComp);
            IFCFAILFAST(m_wucAnimator->Pause());
            m_hasPendingPauseForDComp = false;
        }

        if (m_hasPendingResumeForDComp)
        {
            FAIL_FAST_ASSERT(!m_hasPendingPauseForDComp);
            IFCFAILFAST(m_wucAnimator->Start());    // Start is the resume API for WUC.
            m_hasPendingResumeForDComp = false;
        }
    }
}

void CColorAnimation::SeekDCompAnimationInstances(double globalSeekTime)
{
    if (m_wucAnimator)
    {
        // If there is a custom timeline driving the WUC animation, WUC will internally forward this seek to the legacy
        // DComp animation. Legacy DComp animations use signed ints for seeking, so a large UINT will become negative.
        // This then causes DComp to think that the legacy DComp animation is invalid, and to not evaluate it further,
        // which then means the WUC animation stops progressing and ticks forever.
        //
        // WUC limits its animations to a maximum length of 24 days, so we'll clamp the seek offset to 24 days.
        if (DirectUI::DoubleUtil::IsInfinity(globalSeekTime)
            || globalSeekTime > CAnimation::s_maxWUCKeyFrameAnimationDurationInSeconds)
        {
            m_wucAnimator->Seek(CAnimation::s_maxWUCKeyFrameAnimationDurationInTicks);
        }
        else
        {
            UINT64 offset100ns = static_cast<UINT64>(globalSeekTime * 10000000);
            m_wucAnimator->Seek(offset100ns);
        }
    }

    m_shouldSynchronizeDCompAnimationAfterResume = false;
}

void CColorAnimation::PauseDCompAnimationsOnSuspend()
{
    // This timeline may not be active. If it's a child of a storyboard with a longer duration, this timeline could be
    // Filling or Stopped while the parent is Active.

    if (m_wucAnimator)
    {
        IFCFAILFAST(m_wucAnimator->Pause());
    }
}

void CColorAnimation::ResumeDCompAnimationsOnResume()
{
    // This timeline may not be active. If it's a child of a storyboard with a longer duration, this timeline could be
    // Filling or Stopped while the parent is Active.

    if (m_wucAnimator)
    {
        IFCFAILFAST(m_wucAnimator->Start());    // Start is the resume API for WUC.
        m_shouldSynchronizeDCompAnimationAfterResume = true;
    }
}

void CColorAnimation::ReleaseDCompResources()
{
    __super::ReleaseDCompResources();
}

_Check_return_ HRESULT CColorAnimation::ValueAssign(
    AssignmentOperand destOperand,
    AssignmentOperand sourceOperand)
{
    ASSERT(sourceOperand != destOperand);

    XUINT32* sourceValue = GetInternalOperand(sourceOperand);
    XUINT32* destValue = GetInternalOperand(destOperand);
    *destValue = *sourceValue;

    return S_OK;
}

_Check_return_ HRESULT CColorAnimation::ValueAssign(
    _Out_ CValue& destOperand,
    AssignmentOperand sourceOperand)
{
    XUINT32* sourceValue = GetInternalOperand(sourceOperand);
    destOperand.SetColor(*sourceValue);

    return S_OK;
}

_Check_return_ HRESULT CColorAnimation::ValueAssign(
    AssignmentOperand destOperand,
    _In_ const CValue& sourceOperand)
{
    ASSERT(sourceOperand.GetType() == valueColor);

    XUINT32* destValue = GetInternalOperand(destOperand);
    *destValue = sourceOperand.AsColor();

    return S_OK;
}

XUINT32* CColorAnimation::GetInternalOperand(AssignmentOperand operand)
{
    switch (operand)
    {
        case AssignmentOperand::From:
            return &m_vFrom;

        case AssignmentOperand::To:
            return &m_vTo;

        case AssignmentOperand::By:
            return &m_vBy;

        case AssignmentOperand::BaseValue:
            return &m_vBaseValue;

        case AssignmentOperand::NonAnimatedBaseValue:
            return &m_vNonAnimatedBaseValue;

        case AssignmentOperand::CurrentValue:
            return &m_vCurrentValue;

        default:
            XCP_FAULT_ON_FAILURE(false);
            return nullptr;
    }
}

_Check_return_ CompositionAnimationConversionResult CColorAnimation::MakeCompositionKeyFrameAnimationVirtual(_Inout_ CompositionAnimationConversionContext* myContext)
{
    wu::Color from = ColorUtils::GetWUColor(m_vFrom);
    wu::Color to = ColorUtils::GetWUColor(m_vTo);

    // Xaml ColorAnimations support "by" animation, but it doesn't make sense - "make this color 0x20 more opaque and 0x80 redder
    // throughout the duration". We also don't want to build a complicated WUC expression for doing math with colors, so we ignore
    // "by" animations and treat them as from/to animations instead. This means they won't get continuous handoff.
    //
    // These types of animations are rare - there are currently no instances of "by" ColorAnimations in generic.xaml.
    //
    // m_vTo should already be available after going through OnBegin -> ReadBaseValuesFromTargetOrHandoff -> ComputeToUsingFromAndBy.
    const auto& colorAnimation = myContext->CreateColorLinearAnimation(
        from,
        to,
        m_hasHandoff,
        static_cast<CEasingFunctionBase*>(m_pEasingFunction));

    colorAnimation.As(&m_spWUCAnimation);
    IFC_ANIMATION(myContext->ApplyProperties(m_spWUCAnimation.Get()));
    UpdateWUCAnimationTelemetryString(myContext);

    if (GetDCompObjectRegistry() != nullptr)
    {
        GetDCompObjectRegistry()->EnsureObjectWithDCompResourceRegistered(this);
    }

    return CompositionAnimationConversionResult::Success;
}
