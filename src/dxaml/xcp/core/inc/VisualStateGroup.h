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
        return (m_pEventList != NULL);
    }

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CVisualStateGroup>::Index;
    }

    _Check_return_ HRESULT AddEventListener(_In_ EventHandle hEvent, _In_ CValue * pValue, _In_ XINT32 iListenerType, _Out_opt_ CValue * pResult, _In_ bool fHandledEventsToo) final;
    _Check_return_ HRESULT RemoveEventListener(_In_ EventHandle hEvent, _In_ CValue * pValue) override;

    _Check_return_ HRESULT GetCurrentVisualState(_Outptr_result_maybenull_ CVisualState **ppVisualState);
    _Check_return_ HRESULT SetCurrentVisualState(_In_opt_ CVisualState *pVisualState);

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
    CXcpList<REQUEST> *m_pEventList;

#pragma region Legacy VSM
public:
    HRESULT _Check_return_ FindStateByName(_In_z_ const WCHAR *pStateName, _Out_ CVisualState **ppVisualState);

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
