// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Derivation of UICommand that sets default property values
//      based on an enum value representing standard commands
//      (cut, copy, select all, etc.).

#pragma once

#include "StandardUICommand.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(StandardUICommand)
    {
        protected:
            ~StandardUICommand() override { }

            _Check_return_ HRESULT PrepareState() override;

            _Check_return_ HRESULT OnPropertyChanged2(_In_ const PropertyChangedParams& args) override;

            _Check_return_ HRESULT EnterImpl(
                    _In_ bool bLive,
                    _In_ bool bSkipNameRegistration,
                    _In_ bool bCoercedIsEnabled,
                    _In_ bool bUseLayoutRounding) override;

        private:
            _Check_return_ HRESULT PopulateForKind(xaml_input::StandardUICommandKind kind);

            _Check_return_ HRESULT PopulateWithProperties(
                int labelResourceId,
                xaml_controls::Symbol symbol,
                int acceleratorKeyResourceId,
                wsy::VirtualKeyModifiers acceleratorModifiers,
                int descriptionResourceId);

            _Check_return_ HRESULT PopulateWithProperties(
                int labelResourceId,
                xaml_controls::Symbol symbol,
                wsy::VirtualKey acceleratorKey,
                wsy::VirtualKeyModifiers acceleratorModifiers,
                int descriptionResourceId);

            _Check_return_ HRESULT SetLabelIfUnset(int labelResourceId);

            _Check_return_ HRESULT SetIconSourceIfUnset(xaml_controls::Symbol symbol);

            _Check_return_ HRESULT SetKeyboardAcceleratorIfUnset(
                wsy::VirtualKey acceleratorKey,
                wsy::VirtualKeyModifiers acceleratorModifiers);

            _Check_return_ HRESULT SetDescriptionIfUnset(int descriptionResourceId);

            bool m_ownsLabel{ true };
            bool m_ownsIconSource{ true };
            bool m_ownsKeyboardAccelerator{ true };
            bool m_ownsDescription{ true };

            wsy::VirtualKey m_previousAcceleratorKey{ wsy::VirtualKey_None };
            wsy::VirtualKeyModifiers m_previousAcceleratorModifiers{ wsy::VirtualKeyModifiers_None };
            bool m_settingPropertyInternally{ false };
    };
}
