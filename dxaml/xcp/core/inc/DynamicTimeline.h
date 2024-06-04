// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ParallelTimeline.h"

enum class DynamicTimelineGenerationMode {
    SteadyState,
    Transition
};

class CDynamicTimeline
    : public CParallelTimeline
{
protected:
    CDynamicTimeline(_In_ CCoreServices *pCore)
        : CParallelTimeline(pCore)
        , m_generationMode(DynamicTimelineGenerationMode::Transition)
    {
    }

public:
    DECLARE_CREATE(CDynamicTimeline);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CDynamicTimeline>::Index;
    }

    _Check_return_ HRESULT OnBegin() override;
    _Check_return_ HRESULT FinalizeIteration() override;

    void AnimationTrackingCollectInfoNoRef(
        _Inout_ CDependencyObject** ppTarget,
        _Inout_ CDependencyObject** ppDynamicTimeline
        ) override;

    void SetGenerationMode(DynamicTimelineGenerationMode mode) { m_generationMode = mode; }
    _Check_return_ DynamicTimelineGenerationMode GetGenerationMode() const { return m_generationMode; }

    // This method generates what usually would be set into the internal TimelineCollection into an external one.
    // It's a questionable design used by the VSM's dynamic storyboard generation code. Consider refactoring
    // in the future.
    _Check_return_ HRESULT GenerateChildren(DynamicTimelineGenerationMode mode, _Outptr_ CTimelineCollection** ppTimelineCollection);
    _Check_return_ HRESULT GenerateChildren();

private:
    _Check_return_ HRESULT ExpandChildren();
    _Check_return_ HRESULT GenerateChildrenInternal(DynamicTimelineGenerationMode mode, bool internalChildren,
        _Outptr_opt_ CTimelineCollection** ppTimelineCollection = nullptr);

    // VSM often is looking for two separate version of a dynamic timeline: the one with
    // all the animations and transitions and the steady-state version for when the state
    // is fully transitioned into. For example with a FadeOutAnimation we care about both
    // the transition and the final appearance of the target. This bool tracks whether at
    // begin time we should generate a steady-state version or the version of the timelines
    // with all the durations filled it.
    DynamicTimelineGenerationMode m_generationMode;
};

