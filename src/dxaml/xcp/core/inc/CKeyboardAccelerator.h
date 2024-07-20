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
        return m_pEventList != nullptr;
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
        _Out_opt_ CValue *pResult,
        _In_ bool fHandledEventsToo = false) final;

    _Check_return_ HRESULT RemoveEventListener(
        _In_ EventHandle hEvent,
        _In_ CValue *pValue) final;

private:
    ~CKeyboardAccelerator() override;

    CXcpList<REQUEST> *m_pEventList = nullptr;

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
        EnterParams params) final;

    _Check_return_ HRESULT LeaveImpl(
        _In_ CDependencyObject *pNamescopeOwner,
        LeaveParams params) final;

    CDependencyObject *m_scopeOwner = nullptr;
    DirectUI::VirtualKey m_key = DirectUI::VirtualKey::None;
    DirectUI::VirtualKeyModifiers m_keyModifiers = DirectUI::VirtualKeyModifiers::None;
    bool m_isEnabled = true;
};
