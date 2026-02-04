// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <XamlLogging.h>

#include <KeyboardUtility.h>
#include <KeyboardUtilityUnitTests.h>

#include <RuntimeEnabledFeatures.h>
#include <DependencyLocator.h>

using namespace Microsoft::WRL;
using namespace WEX::TestExecution;
using namespace InputUtility::Keyboard;

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace Input { namespace Keyboard {

MockKeyEventArgs::MockKeyEventArgs(wsy::VirtualKey key, mui::PhysicalKeyStatus keyStatus)
    : m_virtualKey(key), m_keyStatus(keyStatus) {}

HRESULT MockKeyEventArgs::get_KeyStatus(_Out_ mui::PhysicalKeyStatus* keyStatus)
{
    *keyStatus = m_keyStatus;
    return S_OK;
}

HRESULT MockKeyEventArgs::get_VirtualKey(_Out_ wsy::VirtualKey* key)
{
    *key = m_virtualKey;
    return S_OK;
}

UINT32 KeyboardUtilityUnitTests::LParamPack(
    _In_ int repeatCount,
    _In_ int scanCode,
    _In_ bool isExtendedKey,
    _In_ bool isMenuKeyDown,
    _In_ bool wasKeyDown,
    _In_ bool isKeyReleased,
    _In_ bool isSysKey)
{
    UINT32 lParam = 0;

    lParam |= (repeatCount & 0x0000FFFF);      // bits 0-15
    lParam |= ((scanCode & 0x000000FF) << 16); // bits 16-23

    if (isExtendedKey)
    {
        lParam |= ((LPARAM)1 << 24);
    }
    if (isSysKey && isMenuKeyDown)
    {
        lParam |= ((LPARAM)1 << 29);
    }
    if (wasKeyDown)
    {
        lParam |= ((LPARAM)1 << 30);
    }
    if (isKeyReleased)
    {
        lParam |= ((LPARAM)1 << 31);
    }

    return lParam;
}

void KeyboardUtilityUnitTests::VerifyEditKeyMappings()
{
    VERIFY_ARE_EQUAL(TranslateEditKey(0, wsy::VirtualKey::VirtualKey_Hanja), XEDITKEY_HANJA); // modifiers are irrelevant
    VERIFY_ARE_EQUAL(TranslateEditKey(0, wsy::VirtualKey::VirtualKey_A), XEDITKEY_NONE);

    XUINT32 modifierKeys = KEY_MODIFIER_CTRL | KEY_MODIFIER_ALT; //ctrl + alt
    VERIFY_ARE_EQUAL(TranslateEditKey(modifierKeys, wsy::VirtualKey::VirtualKey_A), XEDITKEY_NONE);

    modifierKeys = KEY_MODIFIER_SHIFT | KEY_MODIFIER_ALT; //shift + alt
    VERIFY_ARE_EQUAL(TranslateEditKey(modifierKeys, wsy::VirtualKey::VirtualKey_A), XEDITKEY_NONE);

    modifierKeys = KEY_MODIFIER_CTRL; // Only Control
    VERIFY_ARE_EQUAL(TranslateEditKey(modifierKeys, wsy::VirtualKey::VirtualKey_A), XEDITKEY_SELECT_ALL);
    VERIFY_ARE_EQUAL(TranslateEditKey(modifierKeys, wsy::VirtualKey::VirtualKey_Z), XEDITKEY_UNDO);
    VERIFY_ARE_EQUAL(TranslateEditKey(modifierKeys, wsy::VirtualKey::VirtualKey_Y), XEDITKEY_REDO);
    VERIFY_ARE_EQUAL(TranslateEditKey(modifierKeys, wsy::VirtualKey::VirtualKey_C), XEDITKEY_COPY);
    VERIFY_ARE_EQUAL(TranslateEditKey(modifierKeys, wsy::VirtualKey::VirtualKey_Insert), XEDITKEY_COPY);
    VERIFY_ARE_EQUAL(TranslateEditKey(modifierKeys, wsy::VirtualKey::VirtualKey_X), XEDITKEY_CUT);
    VERIFY_ARE_EQUAL(TranslateEditKey(modifierKeys, wsy::VirtualKey::VirtualKey_Back), XEDITKEY_WORD_DELETE_PREV);
    VERIFY_ARE_EQUAL(TranslateEditKey(modifierKeys, wsy::VirtualKey::VirtualKey_Delete), XEDITKEY_WORD_DELETE_NEXT);

    modifierKeys = KEY_MODIFIER_SHIFT; // Only Shift
    VERIFY_ARE_EQUAL(TranslateEditKey(modifierKeys, wsy::VirtualKey::VirtualKey_Delete), XEDITKEY_CUT);
    VERIFY_ARE_EQUAL(TranslateEditKey(modifierKeys, wsy::VirtualKey::VirtualKey_Insert), XEDITKEY_PASTE);

    modifierKeys = KEY_MODIFIER_CTRL | KEY_MODIFIER_SHIFT; // Control and No Alt
    VERIFY_ARE_EQUAL(TranslateEditKey(modifierKeys, wsy::VirtualKey::VirtualKey_V), XEDITKEY_PASTE);
    VERIFY_ARE_EQUAL(TranslateEditKey(modifierKeys, wsy::VirtualKey::VirtualKey_Left), XEDITKEY_WORD_LEFT);
    VERIFY_ARE_EQUAL(TranslateEditKey(modifierKeys, wsy::VirtualKey::VirtualKey_Right), XEDITKEY_WORD_RIGHT);
    VERIFY_ARE_EQUAL(TranslateEditKey(modifierKeys, wsy::VirtualKey::VirtualKey_Home), XEDITKEY_DOCUMENT_START);
    VERIFY_ARE_EQUAL(TranslateEditKey(modifierKeys, wsy::VirtualKey::VirtualKey_End), XEDITKEY_DOCUMENT_END);

    modifierKeys = KEY_MODIFIER_SHIFT | KEY_MODIFIER_WINDOWS | KEY_MODIFIER_MOUSEDOWN; // Not Control or Alt
    VERIFY_ARE_EQUAL(TranslateEditKey(modifierKeys, wsy::VirtualKey::VirtualKey_Home), XEDITKEY_LINE_START);
    VERIFY_ARE_EQUAL(TranslateEditKey(modifierKeys, wsy::VirtualKey::VirtualKey_End), XEDITKEY_LINE_END);
    VERIFY_ARE_EQUAL(TranslateEditKey(modifierKeys, wsy::VirtualKey::VirtualKey_Up), XEDITKEY_LINE_UP);
    VERIFY_ARE_EQUAL(TranslateEditKey(modifierKeys, wsy::VirtualKey::VirtualKey_Down), XEDITKEY_LINE_DOWN);
    VERIFY_ARE_EQUAL(TranslateEditKey(modifierKeys, wsy::VirtualKey::VirtualKey_Left), XEDITKEY_CHAR_LEFT);
    VERIFY_ARE_EQUAL(TranslateEditKey(modifierKeys, wsy::VirtualKey::VirtualKey_Right), XEDITKEY_CHAR_RIGHT);
    VERIFY_ARE_EQUAL(TranslateEditKey(modifierKeys, wsy::VirtualKey::VirtualKey_Back), XEDITKEY_CHAR_DELETE_PREV);
    VERIFY_ARE_EQUAL(TranslateEditKey(modifierKeys, wsy::VirtualKey::VirtualKey_PageUp), XEDITKEY_PAGE_PREV);
    VERIFY_ARE_EQUAL(TranslateEditKey(modifierKeys, wsy::VirtualKey::VirtualKey_PageDown), XEDITKEY_PAGE_NEXT);

    modifierKeys = KEY_MODIFIER_WINDOWS | KEY_MODIFIER_MOUSEDOWN; // Not Control, Shift, or Alt
    VERIFY_ARE_EQUAL(TranslateEditKey(modifierKeys, wsy::VirtualKey::VirtualKey_Delete), XEDITKEY_CHAR_DELETE_NEXT);
}


void KeyboardUtilityUnitTests::VerifyCorrectPackingOfKeyEventArgs()
{
    wsy::VirtualKey virtualKey = wsy::VirtualKey::VirtualKey_A;
    mui::PhysicalKeyStatus keyStatus;

    keyStatus.RepeatCount = 1;
    keyStatus.ScanCode = 30;
    keyStatus.IsExtendedKey = true;
    keyStatus.IsMenuKeyDown = true;
    keyStatus.WasKeyDown = false;
    keyStatus.IsKeyReleased = false;

    MockKeyEventArgs args(virtualKey, keyStatus);

    UINT32 expectedParam = LParamPack(
        keyStatus.RepeatCount,
        keyStatus.ScanCode,
        keyStatus.IsExtendedKey ? 1 : 0,
        keyStatus.IsMenuKeyDown ? 1 : 0,
        keyStatus.WasKeyDown ? 1 : 0,
        keyStatus.IsKeyReleased ? 1 : 0,
        true);

    UINT32 lParam = 0;
    UINT32 wParam = 0;

    VERIFY_IS_TRUE(PackIntoWin32StyleKeyArgs(&args, WM_SYSKEYDOWN, &wParam, &lParam) == S_OK);
    VERIFY_ARE_EQUAL(wParam, virtualKey);
    VERIFY_ARE_EQUAL(lParam, expectedParam);

    expectedParam = LParamPack(
        keyStatus.RepeatCount,
        keyStatus.ScanCode,
        keyStatus.IsExtendedKey ? 1 : 0,
        keyStatus.IsMenuKeyDown ? 1 : 0,
        keyStatus.WasKeyDown ? 1 : 0,
        keyStatus.IsKeyReleased ? 1 : 0,
        false);

    MockKeyEventArgs args2(virtualKey, keyStatus);

    lParam = 0;
    wParam = 0;

    VERIFY_IS_TRUE(PackIntoWin32StyleKeyArgs(&args2, WM_KEYDOWN, &wParam, &lParam) == S_OK);
    VERIFY_ARE_EQUAL(wParam, virtualKey);
    VERIFY_ARE_EQUAL(lParam, expectedParam);

    keyStatus.RepeatCount = 1;
    keyStatus.ScanCode = 30;
    keyStatus.IsExtendedKey = true;
    keyStatus.IsMenuKeyDown = true;
    keyStatus.WasKeyDown = true;
    keyStatus.IsKeyReleased = true;

    MockKeyEventArgs args3(virtualKey, keyStatus);

    expectedParam = LParamPack(
        keyStatus.RepeatCount,
        keyStatus.ScanCode,
        keyStatus.IsExtendedKey ? 1 : 0,
        keyStatus.IsMenuKeyDown ? 1 : 0,
        keyStatus.WasKeyDown ? 1 : 0,
        keyStatus.IsKeyReleased ? 1 : 0,
        true);

    lParam = 0;
    wParam = 0;

    //Everything enabled
    VERIFY_IS_TRUE(PackIntoWin32StyleKeyArgs(&args3, WM_SYSKEYDOWN, &wParam, &lParam) == S_OK);
    VERIFY_ARE_EQUAL(wParam, virtualKey);
    VERIFY_ARE_EQUAL(lParam, expectedParam);
}

void KeyboardUtilityUnitTests::VerifyDManipForwarding()
{
    wsy::VirtualKey doNotCare = wsy::VirtualKey::VirtualKey_None;

    VERIFY_IS_TRUE(InputUtility::Keyboard::ShouldForwardToDirectManipulation(wsy::VirtualKey::VirtualKey_Escape, doNotCare));
    VERIFY_IS_TRUE(InputUtility::Keyboard::ShouldForwardToDirectManipulation(wsy::VirtualKey::VirtualKey_Up, doNotCare));
    VERIFY_IS_TRUE(InputUtility::Keyboard::ShouldForwardToDirectManipulation(wsy::VirtualKey::VirtualKey_Down, doNotCare));
    VERIFY_IS_TRUE(InputUtility::Keyboard::ShouldForwardToDirectManipulation(wsy::VirtualKey::VirtualKey_Left, doNotCare));
    VERIFY_IS_TRUE(InputUtility::Keyboard::ShouldForwardToDirectManipulation(wsy::VirtualKey::VirtualKey_Right, doNotCare));
    VERIFY_IS_TRUE(InputUtility::Keyboard::ShouldForwardToDirectManipulation(wsy::VirtualKey::VirtualKey_PageUp, doNotCare));
    VERIFY_IS_TRUE(InputUtility::Keyboard::ShouldForwardToDirectManipulation(wsy::VirtualKey::VirtualKey_PageDown, doNotCare));
    VERIFY_IS_TRUE(InputUtility::Keyboard::ShouldForwardToDirectManipulation(wsy::VirtualKey::VirtualKey_Home, doNotCare));
    VERIFY_IS_TRUE(InputUtility::Keyboard::ShouldForwardToDirectManipulation(wsy::VirtualKey::VirtualKey_End, doNotCare));
    VERIFY_IS_TRUE(InputUtility::Keyboard::ShouldForwardToDirectManipulation(wsy::VirtualKey::VirtualKey_Add, doNotCare));
    VERIFY_IS_TRUE(InputUtility::Keyboard::ShouldForwardToDirectManipulation(wsy::VirtualKey::VirtualKey_Subtract, doNotCare));

    VERIFY_IS_TRUE(InputUtility::Keyboard::ShouldForwardToDirectManipulation(wsy::VirtualKey::VirtualKey_None, static_cast<wsy::VirtualKey>(VK_OEM_PLUS)/*OEM_PLUS and MINUS are not defined wsy::VirtualKeys yet*/));
    VERIFY_IS_TRUE(InputUtility::Keyboard::ShouldForwardToDirectManipulation(wsy::VirtualKey::VirtualKey_None, static_cast<wsy::VirtualKey>(VK_OEM_MINUS)));

    // Gamepad Navigation
    VERIFY_IS_TRUE(InputUtility::Keyboard::ShouldForwardToDirectManipulation(doNotCare, wsy::VirtualKey::VirtualKey_GamepadLeftThumbstickRight));
    VERIFY_IS_TRUE(InputUtility::Keyboard::ShouldForwardToDirectManipulation(doNotCare, wsy::VirtualKey::VirtualKey_GamepadDPadRight));
    VERIFY_IS_TRUE(InputUtility::Keyboard::ShouldForwardToDirectManipulation(doNotCare, wsy::VirtualKey::VirtualKey_GamepadLeftThumbstickLeft));
    VERIFY_IS_TRUE(InputUtility::Keyboard::ShouldForwardToDirectManipulation(doNotCare, wsy::VirtualKey::VirtualKey_GamepadDPadLeft));
    VERIFY_IS_TRUE(InputUtility::Keyboard::ShouldForwardToDirectManipulation(doNotCare, wsy::VirtualKey::VirtualKey_GamepadLeftThumbstickUp));
    VERIFY_IS_TRUE(InputUtility::Keyboard::ShouldForwardToDirectManipulation(doNotCare, wsy::VirtualKey::VirtualKey_GamepadDPadUp));
    VERIFY_IS_TRUE(InputUtility::Keyboard::ShouldForwardToDirectManipulation(doNotCare, wsy::VirtualKey::VirtualKey_GamepadLeftThumbstickDown));
    VERIFY_IS_TRUE(InputUtility::Keyboard::ShouldForwardToDirectManipulation(doNotCare, wsy::VirtualKey::VirtualKey_GamepadDPadDown));

    // Some keycodes that shouldn't be forwarded

    for (int count = 0; count < 26; count++) // Letters shouldn't be forwarded
    {
        VERIFY_IS_FALSE(InputUtility::Keyboard::ShouldForwardToDirectManipulation(static_cast<wsy::VirtualKey>(wsy::VirtualKey::VirtualKey_A + count), doNotCare));
    }

    // Non gamepad navigation input should not be forwarded
    VERIFY_IS_FALSE(InputUtility::Keyboard::ShouldForwardToDirectManipulation(wsy::VirtualKey::VirtualKey_GamepadA, doNotCare));
    VERIFY_IS_FALSE(InputUtility::Keyboard::ShouldForwardToDirectManipulation(wsy::VirtualKey::VirtualKey_GamepadB, doNotCare));
    VERIFY_IS_FALSE(InputUtility::Keyboard::ShouldForwardToDirectManipulation(wsy::VirtualKey::VirtualKey_GamepadX, doNotCare));
    VERIFY_IS_FALSE(InputUtility::Keyboard::ShouldForwardToDirectManipulation(wsy::VirtualKey::VirtualKey_GamepadY, doNotCare));
}

} } } } } }
