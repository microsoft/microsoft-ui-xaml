// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "Timeline.h"

class CDispatcherTimer final : public CTimeline
{
private:
    CDispatcherTimer(_In_ CCoreServices *pCore)
        : CTimeline(pCore)
    {}

protected:
    ~CDispatcherTimer() override;

public:
    static _Check_return_ HRESULT Create(
        _Outptr_ CDependencyObject **ppObject,
        _In_ CREATEPARAMETERS *pCreate);

    _Check_return_ HRESULT Stop();
    _Check_return_ HRESULT Start();
    _Check_return_ HRESULT WorkComplete();

    void FireTickEvent();

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CDispatcherTimer>::Index;
    }

    _Check_return_ HRESULT AddEventListener(
        _In_ EventHandle hEvent,
        _In_ CValue *pValue,
        _In_ XINT32 iListenerType,
        _Out_opt_ CValue *pResult,
        _In_ bool fHandledEventsToo = false) final;

    _Check_return_ HRESULT RemoveEventListener(
        _In_ EventHandle hEvent,
        _In_ CValue *pValue) override;

    _Check_return_ HRESULT ComputeStateImpl(
        _In_ const ComputeStateParams &parentParams,
        _Inout_ ComputeStateParams &myParams,
        _Inout_opt_ bool *pHasNoExternalReferences,
        bool hadIndependentAnimationLastTick,
        _Out_ bool *pHasIndependentAnimation
        ) override;

    bool IsInActiveState() const override { return !!m_fRunning; }

    bool ControlsManagedPeerLifetime() override
    {
        // When there's a native ref on this object, strengthen the reference on the
        // managed peer so that it's not collected.  (A running timer shouldn't
        // be collected.)

        return true;
    }

    XUINT32 ParticipatesInManagedTreeInternal() override
    {
        // Don't use the participation logic, since we're using
        // ControlsManagedPeerLiftime instead.

        return DOESNT_PARTICIPATE_IN_MANAGED_TREE;
    }

public:
    CTimeSpan*  m_pInterval     = nullptr;
    XDOUBLE m_rLastTickTime     = 0.0;
    XUINT8 m_fAddedToManager    = FALSE;
    XUINT8 m_fRunning           = FALSE;
    XUINT8 m_fWorkPending       = FALSE;
};
