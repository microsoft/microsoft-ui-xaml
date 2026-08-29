// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "Timeline.h"

class CTimelineCollection;

class CTimelineGroup : public CTimeline
{
protected:
    CTimelineGroup(_In_ CCoreServices *pCore)
        : CTimeline(pCore)
    {}

    ~CTimelineGroup() override;

    bool HasChildren();

    void DetachChildren();

public:
    static _Check_return_ HRESULT Create(
        _Outptr_ CDependencyObject **ppObject,
        _In_ CREATEPARAMETERS *pCreate);

    _Check_return_ HRESULT SetValue(_In_ const SetValueParams& args) final;

    // Parsing needs
    virtual _Check_return_ HRESULT AddChild(_In_ CTimeline *pChild);
    _Check_return_ HRESULT RemoveChild(_In_ CTimeline *pChild);

    _Check_return_ HRESULT ComputeStateImpl(
        _In_ const ComputeStateParams &parentParams,
        _Inout_ ComputeStateParams &myParams,
        _Inout_opt_ bool *pHasNoExternalReferences,
        bool hadIndependentAnimationLastTick,
        _Out_ bool *pHasIndependentAnimation
        ) override;

    // Needed for proper refcounting of targets
    _Check_return_ HRESULT OnAddToTimeManager() final;
    _Check_return_ HRESULT OnRemoveFromTimeManager() final;

    bool IsFinite() final;

protected:
    void InitializeIteration() final;

    _Check_return_ HRESULT FinalizeIteration() override;

public:
    CTimelineCollection *m_pChild = nullptr;
};
