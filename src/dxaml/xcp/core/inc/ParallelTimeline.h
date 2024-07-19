// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "TimelineGroup.h"
#include <Indexes.g.h>
#include <DependencyObjectTraits.h>
#include <DependencyObjectTraits.g.h>

// Object for <ParallelTimeline> tag
class CParallelTimeline : public CTimelineGroup
{
protected:
    CParallelTimeline(_In_ CCoreServices *pCore)
        : CTimelineGroup(pCore)
    {}

    CompositionAnimationConversionResult MakeCompositionAnimationVirtual(_Inout_ CompositionAnimationConversionContext* myContext) override;

public:
#if defined(__XAML_UNITTESTS__)
    CParallelTimeline()  // !!! FOR UNIT TESTING ONLY !!!
        : CParallelTimeline(nullptr)
    {}
#endif

    ~CParallelTimeline() override;

    DECLARE_CREATE(CParallelTimeline);

    KnownTypeIndex GetTypeIndex() const override;

    void GetNaturalDuration(_Out_ DirectUI::DurationType *pDurationType, _Out_ XFLOAT *prDurationValue) final;

    bool IsInterestingForAnimationTracking() override;

    void AnimationTrackingCollectInfoNoRef(
        _Inout_ CDependencyObject** ppTarget,
        _Inout_ CDependencyObject** ppDynamicTimeline
        ) override;

    _Check_return_ HRESULT UpdateAnimation(
        _In_ const ComputeStateParams &parentParams,
        _Inout_ ComputeStateParams &myParams,
        XDOUBLE beginTime,
        DirectUI::DurationType durationType,
        XFLOAT durationValue,
        _In_ const COptionalDouble &expirationTime,
        _Out_ bool *pIsIndependentAnimation
        ) override;

    HRESULT FireCompletedEvent() final;

    void AttachDCompAnimations() override;

    void AddPendingDCompPause() override;
    void AddPendingDCompResume() override;
    void AddPendingDCompSeek(double globalSeekTime) override;
    void SeekDCompAnimationInstances(double globalSeekTime) override;
    void ClearPendingDCompAnimationOperationsRecursive() override;
    void ResolvePendingDCompAnimationOperationsRecursive() override;

    void PauseDCompAnimationsOnSuspend() override;
    void ResumeDCompAnimationsOnResume() override;

    void ReleaseDCompResources() final;

    void DetachDCompCompletedHandlerOnStop() final;

    void UpdateIsWaitingForDCompAnimationCompleted(bool childIsWaiting) final;
};

