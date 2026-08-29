// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <KeyboardUtility.h>

#include <RuntimeEnabledFeatures.h>
#include <DependencyLocator.h>

#include <XboxUtility.h>

#define TRANSLATE_MESSAGE_BUFFER 8

// Helper to populate Win32-style LPARAM and WPARAM args for key events
// from a WinRT KeyEventArgs object (used in Xaml input handling codepath for legacy reasons).
// TODO: Task 23284769: Remove unneeded use of win32 input functions and structures
_Check_return_ HRESULT InputUtility::Keyboard::PackIntoWin32StyleKeyArgs(
    _In_ mui::IKeyEventArgs* pArgs,
    _In_ UINT32 uMsg,
    _Out_ UINT32* wParam,
    _Out_ UINT32* lParam)
{
    wsy::VirtualKey virtualKey;
    mui::PhysicalKeyStatus keyStatus;
    const BOOL isSysKey = (uMsg == WM_SYSKEYDOWN || uMsg == WM_SYSKEYUP);

    IFC_RETURN(pArgs->get_VirtualKey(&virtualKey));
    IFC_RETURN(pArgs->get_KeyStatus(&keyStatus));

    *wParam = static_cast<WPARAM>(virtualKey);

    *lParam |= (keyStatus.RepeatCount & 0x0000FFFF);      // bits 0-15
    *lParam |= ((keyStatus.ScanCode & 0x000000FF) << 16); // bits 16-23

    if (keyStatus.IsExtendedKey)
    {
        *lParam |= ((LPARAM)1 << 24);
    }
    if (isSysKey && keyStatus.IsMenuKeyDown)
    {
        *lParam |= ((LPARAM)1 << 29);
    }
    if (keyStatus.WasKeyDown)
    {
        *lParam |= ((LPARAM)1 << 30);
    }
    if (keyStatus.IsKeyReleased)
    {
        *lParam |= ((LPARAM)1 << 31);
    }

    return S_OK;
}

WPARAM InputUtility::Keyboard::GetVirtualKeyFromPacketInput(_In_ int scanCode)
{
    XBYTE bKeyState[256];
    WCHAR pString[TRANSLATE_MESSAGE_BUFFER];
    XINT32 iWritten = 0;

    // Get the keyboard state
    if (!GetKeyboardState(bKeyState))
    {
        XAML_FAIL_FAST();
    }

    // Get the unicode character code from the key press
    iWritten = ToUnicodeEx(
        static_cast<UINT>(VK_PACKET),
        scanCode,
        bKeyState,
        pString,
        TRANSLATE_MESSAGE_BUFFER,
        0,
        (HKL)NULL);

    // Translates a character to the corresponding virtual-key code.
    if (iWritten > 0)
    {
        return LOBYTE(VkKeyScanEx(pString[0], GetKeyboardLayout(0)));
    }

    return VK_PACKET;
}

_Check_return_ XEDITKEY InputUtility::Keyboard::TranslateEditKey(_In_ const XUINT32 modifierKeys, _In_ const wsy::VirtualKey keyCode)
{
    XEDITKEY result = XEDITKEY_NONE; // Default answer

    const bool bAlt = (modifierKeys & KEY_MODIFIER_ALT) == KEY_MODIFIER_ALT;
    const bool bCtrl = (modifierKeys & KEY_MODIFIER_CTRL) == KEY_MODIFIER_CTRL;
    const bool bShift = (modifierKeys & KEY_MODIFIER_SHIFT) == KEY_MODIFIER_SHIFT;
    const bool bCtrlOnly = bCtrl && !bAlt && !bShift;
    const bool bShiftOnly = bShift && !bCtrl && !bAlt;
    const bool bCtrlNoAlt = bCtrl && !bAlt;

    // Ordering of clauses in this chain of if/else can be important.  Be sure to
    //  have the most-specific checks first.
    // Example: Home for start of line, and Control+Home for start of document.
    //  Control+Home needs to be checked earlier than Home, else we'll fire the
    //  Home clause all the time and never get to Control+Home.

    if (keyCode == wsy::VirtualKey::VirtualKey_Hanja)
    {
        result = XEDITKEY_HANJA;
    }
    else if (bCtrlOnly && keyCode == wsy::VirtualKey::VirtualKey_A)
    {
        result = XEDITKEY_SELECT_ALL;
    }
    else if (bCtrlOnly && keyCode == wsy::VirtualKey::VirtualKey_Z)
    {
        result = XEDITKEY_UNDO;
    }
    else if (bCtrlOnly && keyCode == wsy::VirtualKey::VirtualKey_Y)
    {
        result = XEDITKEY_REDO;
    }
    else if (bCtrlOnly && keyCode == wsy::VirtualKey::VirtualKey_C ||
        bCtrlOnly && keyCode == wsy::VirtualKey::VirtualKey_Insert)
    {
        result = XEDITKEY_COPY;
    }
    else if (bCtrlOnly && keyCode == wsy::VirtualKey::VirtualKey_X ||
        bShiftOnly && keyCode == wsy::VirtualKey::VirtualKey_Delete)
    {
        result = XEDITKEY_CUT;
    }
    else if (bCtrlNoAlt && keyCode == wsy::VirtualKey::VirtualKey_V ||
        bShiftOnly && keyCode == wsy::VirtualKey::VirtualKey_Insert)
    {
        result = XEDITKEY_PASTE;
    }
    else if (bCtrlNoAlt && keyCode == wsy::VirtualKey::VirtualKey_Left)
    {
        result = XEDITKEY_WORD_LEFT;
    }
    else if (bCtrlNoAlt && keyCode == wsy::VirtualKey::VirtualKey_Right)
    {
        result = XEDITKEY_WORD_RIGHT;
    }
    else if (bCtrlOnly && keyCode == wsy::VirtualKey::VirtualKey_Back)
    {
        result = XEDITKEY_WORD_DELETE_PREV;
    }
    else if (bCtrlOnly && keyCode == wsy::VirtualKey::VirtualKey_Delete)
    {
        result = XEDITKEY_WORD_DELETE_NEXT;
    }
    else if (bCtrlNoAlt && keyCode == wsy::VirtualKey::VirtualKey_Home)
    {
        result = XEDITKEY_DOCUMENT_START;
    }
    else if (bCtrlNoAlt && keyCode == wsy::VirtualKey::VirtualKey_End)
    {
        result = XEDITKEY_DOCUMENT_END;
    }
    // Combinations of the keys below should've been handled above.
    // So we explicitly exclude modifiers (except shift, which is allowed)
    else if (!bCtrl && !bAlt)
    {
        switch (keyCode)
        {
        case wsy::VirtualKey::VirtualKey_Home:
            result = XEDITKEY_LINE_START;
            break;
        case wsy::VirtualKey::VirtualKey_End:
            result = XEDITKEY_LINE_END;
            break;
        case wsy::VirtualKey::VirtualKey_Up:
            result = XEDITKEY_LINE_UP;
            break;
        case wsy::VirtualKey::VirtualKey_Down:
            result = XEDITKEY_LINE_DOWN;
            break;
        case wsy::VirtualKey::VirtualKey_Left:
            result = XEDITKEY_CHAR_LEFT;
            break;
        case wsy::VirtualKey::VirtualKey_Right:
            result = XEDITKEY_CHAR_RIGHT;
            break;
        case wsy::VirtualKey::VirtualKey_Back:
            result = XEDITKEY_CHAR_DELETE_PREV;
            break;
        case wsy::VirtualKey::VirtualKey_Delete:
            result = XEDITKEY_CHAR_DELETE_NEXT;
            break;
        case wsy::VirtualKey::VirtualKey_PageUp:
            result = XEDITKEY_PAGE_PREV;
            break;
        case wsy::VirtualKey::VirtualKey_PageDown:
            result = XEDITKEY_PAGE_NEXT;
            break;
        }
    }

    return result;
}

bool InputUtility::Keyboard::ShouldForwardToDirectManipulation(
    _In_ const wsy::VirtualKey remappedVirtualKey,
    _In_ const wsy::VirtualKey virtualKey)
{
    return  remappedVirtualKey == wsy::VirtualKey::VirtualKey_Escape ||
            remappedVirtualKey == wsy::VirtualKey::VirtualKey_Up ||
            remappedVirtualKey == wsy::VirtualKey::VirtualKey_Down ||
            remappedVirtualKey == wsy::VirtualKey::VirtualKey_Left ||
            remappedVirtualKey == wsy::VirtualKey::VirtualKey_Right ||
            XboxUtility::IsGamepadNavigationDirection(virtualKey) ||
            (remappedVirtualKey == wsy::VirtualKey::VirtualKey_PageUp) ||
            (remappedVirtualKey == wsy::VirtualKey::VirtualKey_PageDown) ||
            (remappedVirtualKey == wsy::VirtualKey::VirtualKey_Home) ||
            (remappedVirtualKey == wsy::VirtualKey::VirtualKey_End) ||
            (remappedVirtualKey == wsy::VirtualKey::VirtualKey_Add) ||
            (remappedVirtualKey == wsy::VirtualKey::VirtualKey_Subtract) ||
            (virtualKey == VK_OEM_PLUS) ||
            (virtualKey == VK_OEM_MINUS);
}

wsy::VirtualKey InputUtility::Keyboard::GetVirtualKeyForETWLogging(_In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam)
{
    // This is an injected unicode character which is the IHM soft keyboard, pen, or SendInput().
    // We need to get the active keyboard state, then translate it to a corresponding virtual
    // key code from the unicode character.
    if (wParam == VK_PACKET)
    {
        const int scanCode = (lParam & 0x7F0000) >> 16;
        wParam = InputUtility::Keyboard::GetVirtualKeyFromPacketInput(scanCode);
    }

    auto virtualKey = static_cast<wsy::VirtualKey>(wParam);

    if (ShouldLogVirtualKeyForKeyEvent(virtualKey))
    {
        //We only want to log directional and focus changing virtual keys
        return virtualKey;
    }

    return wsy::VirtualKey_None;
}

unsigned int InputUtility::Keyboard::GetRepeatCount(_In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam)
{
    if (wParam == VK_PACKET)
    {
        // This is an injected unicode character which is the IHM soft keyboard, pen, or SendInput()
        return 1;
    }
    // Repeat count is bits 16-23
    return (lParam & 0x00FF0000) >> 16;
}

bool InputUtility::Keyboard::ShouldLogVirtualKeyForKeyEvent(
    _In_ wsy::VirtualKey virtualKey)
{
    //We are are logging keys for deeper investigations into focus movement and performance.
    //However, we want to limit the keys logged to those required for focus changes
    //to avoid accidentally exposing a keylogger. As a result, we should only log
    //tab, arrow keys, and gamepad navigation direction keys.
    return ((virtualKey <= wsy::VirtualKey_Down) && (virtualKey >= wsy::VirtualKey_Left))
        || (virtualKey == wsy::VirtualKey_Tab)
        || XboxUtility::IsGamepadNavigationDirection(virtualKey)
        || XboxUtility::IsGamepadPageNavigationDirection(virtualKey);
}