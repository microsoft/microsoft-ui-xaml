// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "FocusObserver.h"

class CoreWindowFocusObserver final : public FocusObserver
{
public:
    CoreWindowFocusObserver(_In_ CCoreServices *pCoreService, _In_ CContentRoot* contentRoot);
    ~CoreWindowFocusObserver() override;

    _Check_return_ HRESULT Init(_In_ wuc::ICoreWindow* const pCoreWindow) override;

    bool IsActivated() const override;

    wuc::CoreWindowActivationMode GetActivationMode() const override;

private:
 _Check_return_ HRESULT OnActivated(
        _In_ wuc::ICoreWindow* pSender,
        _In_ wuc::IWindowActivatedEventArgs * pArgs);

    _Check_return_ HRESULT DeInit();

    ctl::ComPtr<wuc::ICoreWindow> m_spCoreWindow;
    EventRegistrationToken m_windowActivatedEventCookie = {};
    wuc::CoreWindowActivationState m_activeState = wuc::CoreWindowActivationState::CoreWindowActivationState_Deactivated;
};

