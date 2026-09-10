// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "WindowInitialShowOptions.g.h"

using namespace DirectUI;

// The setters deliberately do not validate. An out-of-range enum value is rejected with
// E_INVALIDARG by the Show or Activate call that snapshots these options, which leaves the
// window's placement attempt unconsumed so the app can correct the value and try again.

_Check_return_ HRESULT WindowInitialShowOptions::get_ReasonImpl(_Out_ ABI::Microsoft::UI::Xaml::WindowShowReason* value)
{
    *value = m_reason;
    return S_OK;
}

_Check_return_ HRESULT WindowInitialShowOptions::put_ReasonImpl(ABI::Microsoft::UI::Xaml::WindowShowReason value)
{
    m_reason = value;
    return S_OK;
}

_Check_return_ HRESULT WindowInitialShowOptions::get_ActivationBehaviorImpl(_Out_ ABI::Microsoft::UI::Xaml::WindowActivationBehavior* value)
{
    *value = m_activationBehavior;
    return S_OK;
}

_Check_return_ HRESULT WindowInitialShowOptions::put_ActivationBehaviorImpl(ABI::Microsoft::UI::Xaml::WindowActivationBehavior value)
{
    m_activationBehavior = value;
    return S_OK;
}

_Check_return_ HRESULT WindowInitialShowOptions::get_KeepHiddenImpl(_Out_ BOOLEAN* value)
{
    *value = m_keepHidden;
    return S_OK;
}

_Check_return_ HRESULT WindowInitialShowOptions::put_KeepHiddenImpl(BOOLEAN value)
{
    m_keepHidden = value;
    return S_OK;
}
