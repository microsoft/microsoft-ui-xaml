// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "enumdefs.g.h"

class CInputManager;

namespace ContentRootInput
{
    class InputDeviceCache
    {
    public:
        InputDeviceCache(_In_ CInputManager& inputManager);
        void Init();

        DirectUI::FocusInputDeviceKind GetLastFocusInputDeviceKind() const;
        DirectUI::InputDeviceType GetLastInputDeviceType() const;

        void SetLastInputDeviceType(DirectUI::InputDeviceType deviceType, bool keepUIAFocusState = false);

        bool LastInputWasNonFocusNavigationKeyFromSIP() const;
        void SetLastInputWasNonFocusNavigationKeyFromSIP(_In_ bool value);

        void OnSetFocusFromUIA();
        bool GetWasUIAFocusSetSinceLastInput() const;

    private:
        CInputManager& m_inputManager;

        DirectUI::InputDeviceType m_eLastInputDeviceType = DirectUI::InputDeviceType::None;
        bool m_uiaFocusSinceLastInput = false;
        bool m_lastInputWasNonFocusNavigationKeyFromSIP = false;
    };
}