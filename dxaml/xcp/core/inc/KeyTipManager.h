// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <KeyTip.h>

class KeyTipManager final
    : public IPALExecuteOnUIThread
{
public:
    KeyTipManager() = default;
    ~KeyTipManager();

    // IPALExecuteOnUIThread interface
    XUINT32 AddRef() override
    {
        return ++m_refs;
    }
    XUINT32 Release() override
    {
        unsigned int refs = --m_refs;
        if (refs == 0)
        {
            delete this;
        }
        return refs;
    }

    _Check_return_ HRESULT Execute() override;

    void ShowAutoKeyTipForElement(_In_ CDependencyObject* obj, _In_z_ const wchar_t* keysPressed);
    void HideAutoKeyTipForElement(_In_ const CDependencyObject* const obj);

    void AccessKeyModeChanged();

    bool AreKeyTipsEnabled() const;
    void SetAreKeyTipsEnabled(bool enabled);

    void NotifyFiniteAnimationIsRunning(_In_ CCoreServices* core, bool isFiniteAnimationRunning);

    void Reset();

private:

    enum class State : unsigned char
    {
        Idle,                                   // There's no work to do
        WaitingToUpdateKeyTips,                 // We're waiting for a short timer to expire before drawing the KeyTips
        UpdatingKeyTips,                        // We're updating KeyTips on the next callback
        WaitingForAnimationsToComplete,         // We're waiting for animations to finish before showing KeyTips
        ForciblyUpdatingKeyTips                 // We're not waiting for animations anymore, update KeyTips on the next callback
    };

    void GoToState(_In_opt_ CCoreServices* core, State newState);

    void StartStateTimer(_In_ CCoreServices* core, int milliseconds);
    void StopStateTimer();

    void RemoveNullKeyTips();

    static _Check_return_ HRESULT OnStateTimerFiredStatic(_In_ CDependencyObject *pSender, _In_ CEventArgs* pEventArgs);
    void OnStateTimerFired(_In_ CCoreServices* core);

    _Check_return_ HRESULT CreatePopup(
        _Inout_ KeyTip& keyTip,
        _Inout_ KeyTipHelper::VisualProperties& visualProperties,
        _In_ unsigned int keysPressed);

    std::vector<KeyTip> m_keyTips;

    xref_ptr<CDispatcherTimer> m_stateTimer;

    unsigned short m_refs = 1;
    unsigned short m_keysPressed = 0;

    State m_state = State::Idle;

    bool m_areKeyTipsEnabled = true;
    bool m_finiteAnimationIsRunning = false;
};
