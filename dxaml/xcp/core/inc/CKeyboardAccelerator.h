// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CCoreServices;

class CKeyboardAccelerator final : public CDependencyObject
{
public:
    static _Check_return_ HRESULT Create(
        _Outptr_ CDependencyObject **ppObject,
        _In_     CREATEPARAMETERS   *pCreate
    );

    bool ShouldRaiseEvent(_In_ EventHandle hEvent, _In_ bool fInputEvent = false, _In_opt_ CEventArgs *pArgs = NULL) override
    {
        return m_eventList != nullptr;
    }

    XUINT32 ParticipatesInManagedTreeInternal() override
    {
        return PARTICIPATES_IN_MANAGED_TREE;
    }

    // For Invoked event support.
    _Check_return_ HRESULT AddEventListener(
        _In_ EventHandle hEvent,
        _In_ CValue *pValue,
        _In_ XINT32 iListenerType,
        _In_ bool fHandledEventsToo = false) final;

    _Check_return_ HRESULT RemoveEventListener(
        _In_ EventHandle hEvent,
        _In_ CValue *pValue) final;

    gsl::span<REQUEST> GetEventHandlers() override
    {
        return (m_eventList) ? gsl::span<REQUEST>(*m_eventList) : gsl::span<REQUEST>();
    }

private:
    ~CKeyboardAccelerator() override;

    std::unique_ptr<std::vector<REQUEST>> m_eventList;

public:

    KnownTypeIndex GetTypeIndex() const final
    {
        return DependencyObjectTraits<CKeyboardAccelerator>::Index;
    }

    CKeyboardAccelerator(_In_ CCoreServices *pCore)
        : CDependencyObject(pCore)
    {
    }

    _Check_return_ HRESULT EnterImpl(
        _In_ CDependencyObject *pNamescopeOwner,
        _In_ EnterParams params) final;

    _Check_return_ HRESULT LeaveImpl(
        _In_ CDependencyObject *pNamescopeOwner,
        _In_ LeaveParams params) final;

    CDependencyObject *m_scopeOwner = nullptr;
    DirectUI::VirtualKey m_key = DirectUI::VirtualKey::None;
    DirectUI::VirtualKeyModifiers m_keyModifiers = DirectUI::VirtualKeyModifiers::None;
    bool m_isEnabled = true;
};
