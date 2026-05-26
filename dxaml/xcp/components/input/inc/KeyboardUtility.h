// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <paltypes.h>

namespace InputUtility { namespace Keyboard {

    _Check_return_ HRESULT PackIntoWin32StyleKeyArgs(
        _In_ mui::IKeyEventArgs* const pArgs,
        _In_ const UINT32 uMsg,
        _Out_ UINT32* wParam,
        _Out_ UINT32* lParam);

    WPARAM GetVirtualKeyFromPacketInput(_In_ int scanCode);

    _Check_return_ XEDITKEY TranslateEditKey(_In_ const XUINT32 modifierKeys, _In_ const wsy::VirtualKey keyCode);

    bool ShouldForwardToDirectManipulation(
        _In_ const wsy::VirtualKey remappedVirtualKey,
        _In_ const wsy::VirtualKey virtualKey);

    unsigned int GetRepeatCount(_In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam);

    wsy::VirtualKey GetVirtualKeyForETWLogging(_In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam);

    bool ShouldLogVirtualKeyForKeyEvent(
        _In_ wsy::VirtualKey virtualKey);
} }
