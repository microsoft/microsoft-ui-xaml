// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "WindowInitialShowOptions.g.h"

namespace DirectUI
{
    // One-time options for a window's initial display. Window snapshots these when its
    // first Show or Activate call consumes the placement attempt; later mutations of the
    // same instance have no effect on a window that already consumed it.
    PARTIAL_CLASS(WindowInitialShowOptions)
    {
        private:
            ABI::Microsoft::UI::Xaml::WindowShowReason m_reason
                { ABI::Microsoft::UI::Xaml::WindowShowReason_Default };
            ABI::Microsoft::UI::Xaml::WindowActivationBehavior m_activationBehavior
                { ABI::Microsoft::UI::Xaml::WindowActivationBehavior_Activate };
            BOOLEAN m_keepHidden{ FALSE };

        public:
            _Check_return_ HRESULT get_ReasonImpl(_Out_ ABI::Microsoft::UI::Xaml::WindowShowReason* value);
            _Check_return_ HRESULT put_ReasonImpl(ABI::Microsoft::UI::Xaml::WindowShowReason value);

            _Check_return_ HRESULT get_ActivationBehaviorImpl(_Out_ ABI::Microsoft::UI::Xaml::WindowActivationBehavior* value);
            _Check_return_ HRESULT put_ActivationBehaviorImpl(ABI::Microsoft::UI::Xaml::WindowActivationBehavior value);

            _Check_return_ HRESULT get_KeepHiddenImpl(_Out_ BOOLEAN* value);
            _Check_return_ HRESULT put_KeepHiddenImpl(BOOLEAN value);
    };
}
