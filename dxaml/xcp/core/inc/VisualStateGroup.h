// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CVisualTransitionCollection;
class CVisualStateCollection;
class CStoryboard;
class CVisualStateChangedEventArgs;

class CVisualStateGroup final
    : public CDependencyObject
{
private:
    CVisualStateGroup(_In_ CCoreServices *pCore);

protected:
    _Check_return_ HRESULT EnterImpl(_In_ CDependencyObject *pNamescopeOwner, _In_ EnterParams params) override;
    _Check_return_ HRESULT LeaveImpl(_In_ CDependencyObject *pNamescopeOwner, _In_ LeaveParams params) override;

public:
    DECLARE_CREATE(CVisualStateGroup);

    ~CVisualStateGroup() override;

    bool ShouldRaiseEvent(_In_ EventHandle hEvent, _In_ bool fInputEvent = false, _In_opt_ CEventArgs* pArgs = nullptr) override
    {
        return (m_eventList != NULL);
    }

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CVisualStateGroup>::Index;
    }

    _Check_return_ HRESULT AddEventListener(_In_ EventHandle hEvent, _In_ CValue * pValue, _In_ XINT32 iListenerType, _In_ bool fHandledEventsToo) final;
    _Check_return_ HRESULT RemoveEventListener(_In_ EventHandle hEvent, _In_ CValue * pValue) override;

    gsl::span<REQUEST> GetEventHandlers() override
    {
        return (m_eventList) ? gsl::span<REQUEST>(*m_eventList) : gsl::span<REQUEST>();
    }

    _Check_return_ HRESULT GetCurrentVisualState(_Outptr_result_maybenull_ CVisualState **ppVisualState);
    _Check_return_ HRESULT SetCurrentVisualState(_In_opt_ CVisualState *pVisualState);

    uint64_t GetCurrentStateChangedSequence() const { return m_currentStateChangedSequence; }
    uint64_t NextCurrentStateChangedSequence() { return ++m_currentStateChangedSequence; }

    // A failing CurrentStateChanged handler stops a synchronous raise, so later handlers never
    // receive that notification. Listeners that wait for a sequence can treat it as lost.
    void MarkCurrentStateChangedUndelivered(uint64_t sequence)
    {
        // Failures are rare, so keep only the most recent few individually. Older ones are
        // folded into a watermark rather than forgotten, so a listener still waiting for one
        // is not blocked forever.
        if (m_undeliveredCurrentStateChangedSequences.size() == 8)
        {
            const auto evicted = m_undeliveredCurrentStateChangedSequences.front();
            if (evicted > m_undeliveredCurrentStateChangedThrough)
            {
                m_undeliveredCurrentStateChangedThrough = evicted;
            }
            m_undeliveredCurrentStateChangedSequences.erase(m_undeliveredCurrentStateChangedSequences.begin());
        }
        m_undeliveredCurrentStateChangedSequences.push_back(sequence);
    }

    // Sequences at or below the watermark count as lost even if one of them is still queued;
    // a listener then recovers as it does for a missing state instead of waiting.
    bool IsCurrentStateChangedUndelivered(uint64_t sequence) const
    {
        if (sequence <= m_undeliveredCurrentStateChangedThrough)
        {
            return true;
        }
        for (const auto undelivered : m_undeliveredCurrentStateChangedSequences)
        {
            if (undelivered == sequence)
            {
                return true;
            }
        }
        return false;
    }

    enum VisualStateGroupEvent
    {
        VisualStateEventChanging = 0,
        VisualStateEventChanged
    };

    _Check_return_ HRESULT NotifyVisualStateEvent(_In_ CVisualState *pOldState, _In_ CVisualState *pNewState, _In_ VisualStateGroupEvent EventState, _In_ CControl *pControl);

    CVisualStateCollection *m_pVisualStates;
    CVisualTransitionCollection* m_pTransitions;
    std::shared_ptr<StateTriggerVariantMap> m_pStateTriggerVariantMap;

private:
    CVisualState *m_pCurrentState;
    std::unique_ptr<std::vector<REQUEST>> m_eventList;
    uint64_t m_currentStateChangedSequence{ 0 };
    std::vector<uint64_t> m_undeliveredCurrentStateChangedSequences;
    uint64_t m_undeliveredCurrentStateChangedThrough{ 0 };

#pragma region Legacy VSM
public:
    HRESULT _Check_return_ FindStateByName(_In_z_ const WCHAR *pStateName, _Outptr_result_maybenull_ CVisualState **ppVisualState);

    HRESULT _Check_return_ StartNewThenStopOld(
        _In_ CFrameworkElement* pControl,
        _In_ XINT32 cToStart,
        _In_reads_(cToStart) CStoryboard** rgToStart
        );

    std::vector<xref_ptr<CStoryboard>>& GetCurrentlyRunningStoryboards() { return m_CurrentlyRunningStoryboards; }

    static HRESULT _Check_return_ EnsureStoryboardSafeForStart(_In_ CFrameworkElement* pControl, _In_ CStoryboard* pSB);

private:
    _Check_return_ HRESULT CleanupRunningStoryboardsOnLeave();

    std::vector<xref_ptr<CStoryboard>> m_CurrentlyRunningStoryboards;
#pragma endregion
};
