// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <XamlTailored.h>

#include "InputHelper.h"
#include "KeyboardHelper.h"
#include "RpcClient.h"
#include "WindowHelper.h"
#include "IXamlTestHooks-win.h"
#include "HostingDispatcher.h"
#include "Utilities.h"
#include "Hosting.h"

using namespace WEX::Common;
using namespace Microsoft::UI::Xaml::Tests::Common;

namespace Private { namespace Infrastructure {
    const wchar_t* KeyboardHelper::s_keyboardInputHandleName = L"KeyboardInputReceived";
    test_infra::KeyboardWaitKind KeyboardHelper::s_waitKind = test_infra::KeyboardWaitKind::KeyboardWaitKind_Default;

HRESULT KeyboardHelper::RuntimeClassInitialize()
{
    COM_START
    {
        // Copied from WRInputHelper.cpp, provides a large mapping of
        // unicode characters to their virtual key equivalents.
        m_vKeyMapping[std::wstring(L"cancel")]       = VK_CANCEL;
        m_vKeyMapping[std::wstring(L"backspace")]    = VK_BACK;
        m_vKeyMapping[std::wstring(L"clear")]        = VK_CLEAR;
        m_vKeyMapping[std::wstring(L"enter")]        = VK_RETURN;
        m_vKeyMapping[std::wstring(L"return")]       = VK_RETURN;
        m_vKeyMapping[std::wstring(L"numpadenter")]  = VK_RETURN;
        m_vKeyMapping[std::wstring(L"shift")]        = VK_SHIFT;
        m_vKeyMapping[std::wstring(L"pause")]        = VK_PAUSE;
        m_vKeyMapping[std::wstring(L"capslock")]     = VK_CAPITAL;
        m_vKeyMapping[std::wstring(L"kanamode")]     = VK_KANA;
        m_vKeyMapping[std::wstring(L"hangulmode")]   = VK_HANGUL;
        m_vKeyMapping[std::wstring(L"junjamode")]    = VK_JUNJA;
        m_vKeyMapping[std::wstring(L"finalmode")]    = VK_FINAL;
        m_vKeyMapping[std::wstring(L"hanjamode")]    = VK_HANJA;
        m_vKeyMapping[std::wstring(L"kanjimode")]    = VK_KANJI;
        m_vKeyMapping[std::wstring(L"imeconvert")]   = VK_CONVERT;
        m_vKeyMapping[std::wstring(L"imenonconvert")]= VK_NONCONVERT;
        m_vKeyMapping[std::wstring(L"imeaccept")]    = VK_ACCEPT;
        m_vKeyMapping[std::wstring(L"imemodechange")]= VK_MODECHANGE;
        m_vKeyMapping[std::wstring(L"prior")]        = VK_PRIOR;
        m_vKeyMapping[std::wstring(L"pageup")]       = VK_PRIOR;
        m_vKeyMapping[std::wstring(L"next")]         = VK_NEXT;
        m_vKeyMapping[std::wstring(L"pagedown")]     = VK_NEXT;
        m_vKeyMapping[std::wstring(L"end")]          = VK_END;
        m_vKeyMapping[std::wstring(L"home")]         = VK_HOME;
        m_vKeyMapping[std::wstring(L"select")]       = VK_SELECT;
        m_vKeyMapping[std::wstring(L"print")]        = VK_PRINT;
        m_vKeyMapping[std::wstring(L"execute")]      = VK_EXECUTE;
        m_vKeyMapping[std::wstring(L"printscreen")]  = VK_SNAPSHOT;
        m_vKeyMapping[std::wstring(L"snapshot")]     = VK_SNAPSHOT;
        m_vKeyMapping[std::wstring(L"insert")]       = VK_INSERT;
        m_vKeyMapping[std::wstring(L"delete")]       = VK_DELETE;
        m_vKeyMapping[std::wstring(L"del")]          = VK_DELETE;
        m_vKeyMapping[std::wstring(L"help")]         = VK_HELP;
        m_vKeyMapping[std::wstring(L"lwin")]         = VK_LWIN;
        m_vKeyMapping[std::wstring(L"rwin")]         = VK_RWIN;
        m_vKeyMapping[std::wstring(L"apps")]         = VK_APPS;
        m_vKeyMapping[std::wstring(L"sleep")]        = VK_SLEEP;
        m_vKeyMapping[std::wstring(L"*")]            = VK_MULTIPLY;
        m_vKeyMapping[std::wstring(L"+")]            = VK_ADD;
        m_vKeyMapping[std::wstring(L"separator")]    = VK_SEPARATOR;
        m_vKeyMapping[std::wstring(L"-")]            = VK_SUBTRACT;
        m_vKeyMapping[std::wstring(L"/")]            = VK_DIVIDE;
        m_vKeyMapping[std::wstring(L"f1")]           = VK_F1;
        m_vKeyMapping[std::wstring(L"f2")]           = VK_F2;
        m_vKeyMapping[std::wstring(L"f3")]           = VK_F3;
        m_vKeyMapping[std::wstring(L"f4")]           = VK_F4;
        m_vKeyMapping[std::wstring(L"f5")]           = VK_F5;
        m_vKeyMapping[std::wstring(L"f6")]           = VK_F6;
        m_vKeyMapping[std::wstring(L"f7")]           = VK_F7;
        m_vKeyMapping[std::wstring(L"f8")]           = VK_F8;
        m_vKeyMapping[std::wstring(L"f9")]           = VK_F9;
        m_vKeyMapping[std::wstring(L"f10")]          = VK_F10;
        m_vKeyMapping[std::wstring(L"f11")]          = VK_F11;
        m_vKeyMapping[std::wstring(L"f12")]          = VK_F12;
        m_vKeyMapping[std::wstring(L"f13")]          = VK_F13;
        m_vKeyMapping[std::wstring(L"f14")]          = VK_F14;
        m_vKeyMapping[std::wstring(L"f15")]          = VK_F15;
        m_vKeyMapping[std::wstring(L"f16")]          = VK_F16;
        m_vKeyMapping[std::wstring(L"f17")]          = VK_F17;
        m_vKeyMapping[std::wstring(L"f18")]          = VK_F18;
        m_vKeyMapping[std::wstring(L"f19")]          = VK_F19;
        m_vKeyMapping[std::wstring(L"f20")]          = VK_F20;
        m_vKeyMapping[std::wstring(L"f21")]          = VK_F21;
        m_vKeyMapping[std::wstring(L"f22")]          = VK_F22;
        m_vKeyMapping[std::wstring(L"f23")]          = VK_F23;
        m_vKeyMapping[std::wstring(L"f24")]          = VK_F24;
        m_vKeyMapping[std::wstring(L"numlock")]      = VK_NUMLOCK;
        m_vKeyMapping[std::wstring(L"scrolllock")]   = VK_SCROLL;
        m_vKeyMapping[std::wstring(L"lshift")]       = VK_LSHIFT;
        m_vKeyMapping[std::wstring(L"rshift")]       = VK_RSHIFT;
        m_vKeyMapping[std::wstring(L"lctrl")]        = VK_LCONTROL;
        m_vKeyMapping[std::wstring(L"rctrl")]        = VK_RCONTROL;
        m_vKeyMapping[std::wstring(L"lalt")]         = VK_LMENU;
        m_vKeyMapping[std::wstring(L"ralt")]         = VK_RMENU;
        m_vKeyMapping[std::wstring(L";")]            = VK_OEM_1;
        m_vKeyMapping[std::wstring(L"oem1")]         = VK_OEM_1;
        m_vKeyMapping[std::wstring(L",")]            = VK_OEM_COMMA;
        m_vKeyMapping[std::wstring(L"dash")]         = VK_OEM_MINUS;
        m_vKeyMapping[std::wstring(L"?")]            = VK_OEM_2;
        m_vKeyMapping[std::wstring(L"oem2")]         = VK_OEM_2;
        m_vKeyMapping[std::wstring(L"~")]            = VK_OEM_3;
        m_vKeyMapping[std::wstring(L"oem3")]         = VK_OEM_3;
        m_vKeyMapping[std::wstring(L"oem4")]         = VK_OEM_4;
        m_vKeyMapping[std::wstring(L"oem5")]         = VK_OEM_5;
        m_vKeyMapping[std::wstring(L"oem6")]         = VK_OEM_6;
        m_vKeyMapping[std::wstring(L"oem7")]         = VK_OEM_7;
        m_vKeyMapping[std::wstring(L"oem8")]         = VK_OEM_8;
        m_vKeyMapping[std::wstring(L"GamepadA")]     = VK_GAMEPAD_A;
        m_vKeyMapping[std::wstring(L"GamepadB")]     = VK_GAMEPAD_B;
        m_vKeyMapping[std::wstring(L"GamepadDpadRight")]            = VK_GAMEPAD_DPAD_RIGHT;
        m_vKeyMapping[std::wstring(L"GamepadDpadDown")]             = VK_GAMEPAD_DPAD_DOWN;
        m_vKeyMapping[std::wstring(L"GamepadDpadUp")]               = VK_GAMEPAD_DPAD_UP;
        m_vKeyMapping[std::wstring(L"GamepadDpadLeft")]             = VK_GAMEPAD_DPAD_LEFT;
        m_vKeyMapping[std::wstring(L"GamepadLeftShoulder")]         = VK_GAMEPAD_LEFT_SHOULDER;
        m_vKeyMapping[std::wstring(L"GamepadLeftTrigger")]          = VK_GAMEPAD_LEFT_TRIGGER;
        m_vKeyMapping[std::wstring(L"GamepadRightShoulder")]        = VK_GAMEPAD_RIGHT_SHOULDER;
        m_vKeyMapping[std::wstring(L"GamepadRightTrigger")]         = VK_GAMEPAD_RIGHT_TRIGGER;
        m_vKeyMapping[std::wstring(L"GamePadLeftThumbStickRight")]  = VK_GAMEPAD_LEFT_THUMBSTICK_RIGHT;
        m_vKeyMapping[std::wstring(L"GamePadLeftThumbStickDown")]   = VK_GAMEPAD_LEFT_THUMBSTICK_DOWN;
        m_vKeyMapping[std::wstring(L"GamePadLeftThumbStickUp")]     = VK_GAMEPAD_LEFT_THUMBSTICK_UP;
        m_vKeyMapping[std::wstring(L"GamePadLeftThumbStickLeft")]   = VK_GAMEPAD_LEFT_THUMBSTICK_LEFT;
        m_vKeyMapping[std::wstring(L"GamePadRightThumbStickRight")] = VK_GAMEPAD_RIGHT_THUMBSTICK_RIGHT;
        m_vKeyMapping[std::wstring(L"GamePadRightThumbStickDown")]  = VK_GAMEPAD_RIGHT_THUMBSTICK_DOWN;
        m_vKeyMapping[std::wstring(L"GamePadRightThumbStickUp")]    = VK_GAMEPAD_RIGHT_THUMBSTICK_UP;
        m_vKeyMapping[std::wstring(L"GamePadRightThumbStickLeft")]  = VK_GAMEPAD_RIGHT_THUMBSTICK_LEFT;
        m_vKeyMapping[std::wstring(L"GamePadMenu")]                 = VK_GAMEPAD_MENU;

        // RIM keyboard injection engine has bug (Some VKeys are not getting translated to WM_CHAR messages), use ScanCode instead.
        m_scanCodeMapping[std::wstring(L" ")] = 0x39;
        m_scanCodeMapping[std::wstring(L"0")] = 0x0B;
        m_scanCodeMapping[std::wstring(L"1")] = 0x02;
        m_scanCodeMapping[std::wstring(L"2")] = 0x03;
        m_scanCodeMapping[std::wstring(L"3")] = 0x04;
        m_scanCodeMapping[std::wstring(L"4")] = 0x05;
        m_scanCodeMapping[std::wstring(L"5")] = 0x06;
        m_scanCodeMapping[std::wstring(L"6")] = 0x07;
        m_scanCodeMapping[std::wstring(L"7")] = 0x08;
        m_scanCodeMapping[std::wstring(L"8")] = 0x09;
        m_scanCodeMapping[std::wstring(L"9")] = 0x0A;
        m_scanCodeMapping[std::wstring(L"a")] = 0x1E;
        m_scanCodeMapping[std::wstring(L"b")] = 0x30;
        m_scanCodeMapping[std::wstring(L"c")] = 0x2E;
        m_scanCodeMapping[std::wstring(L"d")] = 0x20;
        m_scanCodeMapping[std::wstring(L"e")] = 0x12;
        m_scanCodeMapping[std::wstring(L"f")] = 0x21;
        m_scanCodeMapping[std::wstring(L"g")] = 0x22;
        m_scanCodeMapping[std::wstring(L"h")] = 0x23;
        m_scanCodeMapping[std::wstring(L"i")] = 0x17;
        m_scanCodeMapping[std::wstring(L"j")] = 0x24;
        m_scanCodeMapping[std::wstring(L"k")] = 0x25;
        m_scanCodeMapping[std::wstring(L"l")] = 0x26;
        m_scanCodeMapping[std::wstring(L"m")] = 0x32;
        m_scanCodeMapping[std::wstring(L"n")] = 0x31;
        m_scanCodeMapping[std::wstring(L"o")] = 0x18;
        m_scanCodeMapping[std::wstring(L"p")] = 0x19;
        m_scanCodeMapping[std::wstring(L"q")] = 0x10;
        m_scanCodeMapping[std::wstring(L"r")] = 0x13;
        m_scanCodeMapping[std::wstring(L"s")] = 0x1F;
        m_scanCodeMapping[std::wstring(L"t")] = 0x14;
        m_scanCodeMapping[std::wstring(L"u")] = 0x16;
        m_scanCodeMapping[std::wstring(L"v")] = 0x2F;
        m_scanCodeMapping[std::wstring(L"w")] = 0x11;
        m_scanCodeMapping[std::wstring(L"x")] = 0x2D;
        m_scanCodeMapping[std::wstring(L"y")] = 0x15;
        m_scanCodeMapping[std::wstring(L"z")] = 0x2C;
        m_scanCodeMapping[std::wstring(L".")] = 0x34;
        m_scanCodeMapping[std::wstring(L"KeyPad0")] = 0x52;
        m_scanCodeMapping[std::wstring(L"KeyPad1")] = 0x4f;
        m_scanCodeMapping[std::wstring(L"KeyPad2")] = 0x50;
        m_scanCodeMapping[std::wstring(L"KeyPad3")] = 0x51;
        m_scanCodeMapping[std::wstring(L"KeyPad4")] = 0x4b;
        m_scanCodeMapping[std::wstring(L"KeyPad5")] = 0x4c;
        m_scanCodeMapping[std::wstring(L"KeyPad6")] = 0x4d;
        m_scanCodeMapping[std::wstring(L"KeyPad7")] = 0x47;
        m_scanCodeMapping[std::wstring(L"KeyPad8")] = 0x48;
        m_scanCodeMapping[std::wstring(L"KeyPad9")] = 0x49;
        //We've ended up using both n0 and KeyPad0 for the same keys...
        m_scanCodeMapping[std::wstring(L"n0")] = 0x52;
        m_scanCodeMapping[std::wstring(L"n1")] = 0x4f;
        m_scanCodeMapping[std::wstring(L"n2")] = 0x50;
        m_scanCodeMapping[std::wstring(L"n3")] = 0x51;
        m_scanCodeMapping[std::wstring(L"n4")] = 0x4b;
        m_scanCodeMapping[std::wstring(L"n5")] = 0x4c;
        m_scanCodeMapping[std::wstring(L"n6")] = 0x4d;
        m_scanCodeMapping[std::wstring(L"n7")] = 0x47;
        m_scanCodeMapping[std::wstring(L"n8")] = 0x48;
        m_scanCodeMapping[std::wstring(L"n9")] = 0x49;
        m_scanCodeMapping[std::wstring(L"ctrlscan")] = 0x1d;
        m_scanCodeMapping[std::wstring(L"altscan")] = 0x38;
        m_scanCodeMapping[std::wstring(L"ctrl")] = 0x1d;
        m_scanCodeMapping[std::wstring(L"alt")] = 0x38;
        m_scanCodeMapping[std::wstring(L"tab")] = 0x0f;
        m_scanCodeMapping[std::wstring(L"esc")] = 0x01;
        m_scanCodeMapping[std::wstring(L"enter")] = 0x1c;
        m_scanCodeMapping[std::wstring(L"left")] = 0x4b;
        m_scanCodeMapping[std::wstring(L"up")] = 0x48;
        m_scanCodeMapping[std::wstring(L"right")] = 0x4d;
        m_scanCodeMapping[std::wstring(L"down")] = 0x50;
        m_scanCodeMapping[std::wstring(L"space")] = 0x39;
        m_scanCodeMapping[std::wstring(L"equal")] = 0x0d;
        SetWaitKind(test_infra::KeyboardWaitKind::KeyboardWaitKind_Default);
    }
    COM_END
}

KeyboardHelperTestPoolFilter::KeyboardHelperTestPoolFilter()
{
    KeyboardHelper::SetWaitKindStatic(test_infra::KeyboardWaitKind::KeyboardWaitKind_Default);
    m_default = KeyboardHelper::GetWaitKind();
}

HRESULT KeyboardHelper::Enter()
{
    COM_START
    {
        LogThrow_IfFailed(PressKeySequence(wrl::Wrappers::HStringReference(L"$d$_enter#$u$_enter").Get()));
    }
    COM_END
}

HRESULT KeyboardHelper::Space()
{
    COM_START
    {
        LogThrow_IfFailed(PressKeySequence(wrl::Wrappers::HStringReference(L"$d$_space#$u$_space").Get()));
    }
    COM_END
}

HRESULT KeyboardHelper::Escape()
{
    COM_START
    {
        LogThrow_IfFailed(PressKeySequence(wrl::Wrappers::HStringReference(L"$d$_esc#$u$_esc").Get()));
    }
    COM_END
}

HRESULT KeyboardHelper::Tab()
{
    COM_START
    {
        LogThrow_IfFailed(PressKeySequence(wrl::Wrappers::HStringReference(L"$d$_tab#$u$_tab").Get()));
    }
    COM_END
}

HRESULT KeyboardHelper::CtrlTab()
{
    COM_START
    {
        LogThrow_IfFailed(PressKeySequence(wrl::Wrappers::HStringReference(L"$d$_ctrlscan#$d$_tab#$u$_tab#$u$_ctrlscan").Get()));
    }
    COM_END
}

HRESULT KeyboardHelper::ShiftTab()
{
    COM_START
    {
        LogThrow_IfFailed(PressKeySequence(wrl::Wrappers::HStringReference(L"$d$_shift#$d$_tab#$u$_tab#$u$_shift").Get()));
    }
        COM_END
}

HRESULT KeyboardHelper::AltTab()
{
    COM_START
    {
        auto originalWaitKind = GetWaitKind();

        auto scopeGuard = wil::scope_exit([&]
        {
            SetWaitKind(originalWaitKind);
        });

        SetWaitKind(test_infra::KeyboardWaitKind::KeyboardWaitKind_None);
        LogThrow_IfFailed(PressKeySequence(wrl::Wrappers::HStringReference(L"$d$_alt#$d$_tab#$u$_tab#$u$_alt").Get()));
    }
    COM_END
}

HRESULT KeyboardHelper::Up()
{
    COM_START
    {
        LogThrow_IfFailed(PressKeySequence(wrl::Wrappers::HStringReference(L"$d$_up#$u$_up").Get()));
    }
    COM_END
}

HRESULT KeyboardHelper::Down()
{
    COM_START
    {
        LogThrow_IfFailed(PressKeySequence(wrl::Wrappers::HStringReference(L"$d$_down#$u$_down").Get()));
    }
    COM_END
}

HRESULT KeyboardHelper::Left()
{
    COM_START
    {
        LogThrow_IfFailed(PressKeySequence(wrl::Wrappers::HStringReference(L"$d$_left#$u$_left").Get()));
    }
    COM_END
}

HRESULT KeyboardHelper::Right()
{
    COM_START
    {
        LogThrow_IfFailed(PressKeySequence(wrl::Wrappers::HStringReference(L"$d$_right#$u$_right").Get()));
    }
    COM_END
}

HRESULT KeyboardHelper::Home()
{
    COM_START
    {
        LogThrow_IfFailed(PressKeySequence(wrl::Wrappers::HStringReference(L"$d$_home#$u$_home").Get()));
    }
    COM_END
}

HRESULT KeyboardHelper::End()
{
    COM_START
    {
        LogThrow_IfFailed(PressKeySequence(wrl::Wrappers::HStringReference(L"$d$_end#$u$_end").Get()));
    }
    COM_END
}

HRESULT KeyboardHelper::PageUp()
{
    COM_START
    {
        LogThrow_IfFailed(PressKeySequence(wrl::Wrappers::HStringReference(L"$d$_pageup#$u$_pageup").Get()));
    }
    COM_END
}

HRESULT KeyboardHelper::PageDown()
{
    COM_START
    {
        LogThrow_IfFailed(PressKeySequence(wrl::Wrappers::HStringReference(L"$d$_pagedown#$u$_pagedown").Get()));
    }
    COM_END
}

HRESULT KeyboardHelper::Backspace()
{
    COM_START
    {
        LogThrow_IfFailed(PressKeySequence(wrl::Wrappers::HStringReference(L"$d$_backspace#$u$_backspace").Get()));
    }
    COM_END
}

HRESULT KeyboardHelper::Insert()
{
    COM_START
    {
        LogThrow_IfFailed(PressKeySequence(wrl::Wrappers::HStringReference(L"$d$_insert#$u$_insert").Get()));
    }
    COM_END
}

HRESULT KeyboardHelper::Delete()
{
    COM_START
    {
        LogThrow_IfFailed(PressKeySequence(wrl::Wrappers::HStringReference(L"$d$_delete#$u$_delete").Get()));
    }
    COM_END
}

HRESULT KeyboardHelper::Alt()
{
    COM_START
    {
        LogThrow_IfFailed(PressKeySequence(wrl::Wrappers::HStringReference(L"$d$_alt#$u$_alt").Get()));
    }
    COM_END
}

HRESULT KeyboardHelper::Control()
{
    COM_START
    {
        LogThrow_IfFailed(PressKeySequence(wrl::Wrappers::HStringReference(L"$d$_ctrlscan#$u$_ctrlscan").Get()));
    }
    COM_END
}

HRESULT KeyboardHelper::GamepadA()
{
    COM_START
    {
        LogThrow_IfFailed(PressKeySequence(wrl::Wrappers::HStringReference(L"$d$_GamepadA#$u$_GamepadA").Get()));
    }
    COM_END
}

HRESULT KeyboardHelper::GamepadB()
{
    COM_START
    {
        LogThrow_IfFailed(PressKeySequence(wrl::Wrappers::HStringReference(L"$d$_GamepadB#$u$_GamepadB").Get()));
    }
    COM_END
}

HRESULT KeyboardHelper::GamepadDpadRight()
{
    COM_START
    {
        LogThrow_IfFailed(PressKeySequence(wrl::Wrappers::HStringReference(L"$d$_GamepadDpadRight#$u$_GamepadDpadRight").Get()));
    }
    COM_END
}

HRESULT KeyboardHelper::GamepadDpadDown()
{
    COM_START
    {
        LogThrow_IfFailed(PressKeySequence(wrl::Wrappers::HStringReference(L"$d$_GamepadDpadDown#$u$_GamepadDpadDown").Get()));
    }
    COM_END
}

HRESULT KeyboardHelper::GamepadDpadUp()
{
    COM_START
    {
        LogThrow_IfFailed(PressKeySequence(wrl::Wrappers::HStringReference(L"$d$_GamepadDpadUp#$u$_GamepadDpadUp").Get()));
    }
    COM_END
}

HRESULT KeyboardHelper::GamepadDpadLeft()
{
    COM_START
    {
        LogThrow_IfFailed(PressKeySequence(wrl::Wrappers::HStringReference(L"$d$_GamepadDpadLeft#$u$_GamepadDpadLeft").Get()));
    }
    COM_END
}

HRESULT KeyboardHelper::GamepadLeftShoulder()
{
    COM_START
    {
        LogThrow_IfFailed(PressKeySequence(wrl::Wrappers::HStringReference(L"$d$_GamepadLeftShoulder#$u$_GamepadLeftShoulder").Get()));
    }
    COM_END
}

HRESULT KeyboardHelper::GamepadLeftTrigger()
{
    COM_START
    {
        LogThrow_IfFailed(PressKeySequence(wrl::Wrappers::HStringReference(L"$d$_GamepadLeftTrigger#$u$_GamepadLeftTrigger").Get()));
    }
    COM_END
}

HRESULT KeyboardHelper::GamepadRightShoulder()
{
    COM_START
    {
        LogThrow_IfFailed(PressKeySequence(wrl::Wrappers::HStringReference(L"$d$_GamepadRightShoulder#$u$_GamepadRightShoulder").Get()));
    }
    COM_END
}

HRESULT KeyboardHelper::GamepadRightTrigger()
{
    COM_START
    {
        LogThrow_IfFailed(PressKeySequence(wrl::Wrappers::HStringReference(L"$d$_GamepadRightTrigger#$u$_GamepadRightTrigger").Get()));
    }
        COM_END
}

HRESULT KeyboardHelper::GamePadLeftThumbStickRight()
{
    COM_START
    {
        LogThrow_IfFailed(PressKeySequence(wrl::Wrappers::HStringReference(L"$d$_GamePadLeftThumbStickRight#$u$_GamePadLeftThumbStickRight").Get()));
    }
    COM_END
}

HRESULT KeyboardHelper::GamePadLeftThumbStickDown()
{
    COM_START
    {
        LogThrow_IfFailed(PressKeySequence(wrl::Wrappers::HStringReference(L"$d$_GamePadLeftThumbStickDown#$u$_GamePadLeftThumbStickDown").Get()));
    }
        COM_END
}

HRESULT KeyboardHelper::GamePadLeftThumbStickUp()
{
    COM_START
    {
        LogThrow_IfFailed(PressKeySequence(wrl::Wrappers::HStringReference(L"$d$_GamePadLeftThumbStickUp#$u$_GamePadLeftThumbStickUp").Get()));
    }
        COM_END
}

HRESULT KeyboardHelper::GamePadLeftThumbStickLeft()
{
    COM_START
    {
        LogThrow_IfFailed(PressKeySequence(wrl::Wrappers::HStringReference(L"$d$_GamePadLeftThumbStickLeft#$u$_GamePadLeftThumbStickLeft").Get()));
    }
        COM_END
}

HRESULT KeyboardHelper::GamePadRightThumbStickRight()
{
    COM_START
    {
        LogThrow_IfFailed(PressKeySequence(wrl::Wrappers::HStringReference(L"$d$_GamePadRightThumbStickRight#$u$_GamePadRightThumbStickRight").Get()));
    }
        COM_END
}

HRESULT KeyboardHelper::GamePadRightThumbStickDown()
{
    COM_START
    {
        LogThrow_IfFailed(PressKeySequence(wrl::Wrappers::HStringReference(L"$d$_GamePadRightThumbStickDown#$u$_GamePadRightThumbStickDown").Get()));
    }
        COM_END
}

HRESULT KeyboardHelper::GamePadRightThumbStickUp()
{
    COM_START
    {
        LogThrow_IfFailed(PressKeySequence(wrl::Wrappers::HStringReference(L"$d$_GamePadRightThumbStickUp#$u$_GamePadRightThumbStickUp").Get()));
    }
        COM_END
}

HRESULT KeyboardHelper::GamePadRightThumbStickLeft()
{
    COM_START
    {
        LogThrow_IfFailed(PressKeySequence(wrl::Wrappers::HStringReference(L"$d$_GamePadRightThumbStickLeft#$u$_GamePadRightThumbStickLeft").Get()));
    }
        COM_END
}

HRESULT KeyboardHelper::GamePadMenu()
{
    COM_START
    {
        LogThrow_IfFailed(PressKeySequence(wrl::Wrappers::HStringReference(L"$d$_GamePadMenu#$u$_GamePadMenu").Get()));
    }
        COM_END
}

HRESULT KeyboardHelper::Copy()
{
    COM_START
    {
        LogThrow_IfFailed(PressKeySequence(wrl::Wrappers::HStringReference(L"$d$_ctrlscan#$d$_c#$u$_c#$u$_ctrlscan").Get()));
    }
    COM_END
}

HRESULT KeyboardHelper::Paste()
{
    COM_START
    {
        LogThrow_IfFailed(PressKeySequence(wrl::Wrappers::HStringReference(L"$d$_ctrlscan#$d$_v#$u$_v#$u$_ctrlscan").Get()));
    }
    COM_END
}

HRESULT KeyboardHelper::Cut()
{
    COM_START
    {
        LogThrow_IfFailed(PressKeySequence(wrl::Wrappers::HStringReference(L"$d$_ctrlscan#$d$_x#$u$_x#$u$_ctrlscan").Get()));
    }
    COM_END
}


// return true for mapping to scan code, false for VKey
bool KeyboardHelper::MapKeyCode(std::wstring key, UINT16 &keyCode)
{
    bool fUseScanCode = true;
    if (m_scanCodeMapping.count(key) == 0)
    {
        Throw::IfFalse(m_vKeyMapping.count(key) > 0, E_FAIL, L"A character in the key sequence is not mapped.");
        keyCode = m_vKeyMapping[key];
        fUseScanCode = false;
    }
    else
    {
        keyCode = m_scanCodeMapping[key];
    }

    return fUseScanCode;
}

HRESULT KeyboardHelper::PressKeySequence(HSTRING keySeq)
{
    COM_START
    {
        RpcClientEnsureConnected();

        if (HostingDispatcher::Get()->IsUIThread())
        {
            LOG_ERROR(L"Do not call keyboard injection from UI thread!");
            LogThrow_IfFailed(E_UNEXPECTED);
        }

        bool sendKeysOk = true;

        wrl::ComPtr<test_infra::ITestServicesStatics> spTestServicesStatics;
        LogThrow_IfFailed(wf::GetActivationFactory(
            wrl::Wrappers::HStringReference(RuntimeClass_Private_Infrastructure_TestServices).Get(),
            &spTestServicesStatics
        ));
        wrl::ComPtr<test_infra::IWindowHelper> spWindowHelper;
        LogThrow_IfFailed(spTestServicesStatics->get_WindowHelper(&spWindowHelper));

        auto testHooks = WindowHelper::GetTestHooks();

        testHooks->DisableKeyboardInputEvent();

        std::wstring wstrKeySequence(WindowsGetStringRawBuffer(keySeq, NULL));

        // In RS3, alt+shift is a hotkey and XAML will not be notified of the
        // 'shift' key. We have the same behavior for the opposite sequence: if
        // the sequence is shift+alt, XAML will not be notified of the 'alt' key.
        // To workaround this behavior, we will ignore failures when sending alt
        // and shift inputs if both are present in the sequence.
        const bool areAltAndShiftUsed =
            wstrKeySequence.find(L"alt") != std::wstring::npos &&
            wstrKeySequence.find(L"shift") != std::wstring::npos;

        // This is a cute key sequence mini-language adopted from the previous test framework.
        // It walks through the string and if it's a normal string sends it, or if it's a string
        // specifying special keys, will press those keys. Here are some valid strings:
        // "$d$_ctrl#$d$_-#$u$_-#$u$_ctrl" - Down Ctrl, Down -, Up -, Up Ctrl
        // "Hello" - Down Shift, Down H, Up H, Up Shift, Down e, Up e... etc
        size_t posStart = 0;
        wstrKeySequence.append(L"#");
        size_t posEnd = wstrKeySequence.find(L"#");
        while(posEnd != std::wstring::npos)
        {
            std::wstring keyInstruction = wstrKeySequence.substr(posStart, posEnd-posStart);

            size_t keyDownCodePos = keyInstruction.find(L"$d$_");
            size_t keyUpCodePos = keyInstruction.find(L"$u$_");
            if(keyDownCodePos == 0 || keyUpCodePos == 0)
            {
                std::wstring key = keyInstruction.substr(4, keyInstruction.length());
                UINT16 keyCode;
                bool fScanCode = MapKeyCode(key, keyCode);

                const bool isAltOrShiftKey = (key == std::wstring(L"alt")) || (key == std::wstring(L"shift"));

                if (FAILED(SendKeyInput(keyCode, keyDownCodePos == 0 /* down */, fScanCode /* scanCode */, spWindowHelper.Get())) &&
                    (!isAltOrShiftKey || !areAltAndShiftUsed))
                {
                    sendKeysOk = false;
                }
            }
            else
            {
                // This is a normal string, just send a up/down of each key.
                for(size_t i=0; i<keyInstruction.length(); i++)
                {
                    std::wstring key = keyInstruction.substr(i, 1);
                    bool shouldShift = isupper(key[0]) > 0;
                    key[0] = std::tolower<wchar_t>(key[0], std::locale());

                    if(shouldShift)
                    {
                        if (FAILED(SendKeyInput(m_vKeyMapping[std::wstring(L"shift")],
                            true /* down */, false /* scanCode */, spWindowHelper.Get())))
                        {
                            sendKeysOk = false;
                        }
                    }

                    UINT16 keyCode;
                    bool fScanCode = MapKeyCode(key, keyCode);
                    if (FAILED(SendKeyInput(keyCode, true /* down */, fScanCode /* scanCode */, spWindowHelper.Get())))
                    {
                        sendKeysOk = false;
                    }

                    if (FAILED(SendKeyInput(keyCode, false /* down */, fScanCode /* scanCode */, spWindowHelper.Get())))
                    {
                        sendKeysOk = false;
                    }

                    if (shouldShift)
                    {
                        if (FAILED(SendKeyInput(m_vKeyMapping[std::wstring(L"shift")],
                            false /* down */, false /* scanCode */, spWindowHelper.Get())))
                        {
                            sendKeysOk = false;
                        }
                    }
                }
            }

            posStart = posEnd + 1;
            posEnd =  wstrKeySequence.find(L"#", posStart);
        }

        if (!sendKeysOk)
        {
            LOG_ERROR(L"Some keys failed to send!");
            LogThrow_IfFailed(E_UNEXPECTED);
        }
    }
    COM_END
}

void KeyboardHelper::SetWaitKindStatic(test_infra::KeyboardWaitKind waitKind)
{
    s_waitKind = waitKind;
}

HRESULT KeyboardHelper::SetWaitKind(test_infra::KeyboardWaitKind waitKind)
{
    SetWaitKindStatic(waitKind);
    return S_OK;
}

HRESULT KeyboardHelper::SendKeyInput(UINT16 keyCode, bool down, bool scanCode, test_infra::IWindowHelper* pWindowHelper)
{
    if (s_waitKind & test_infra::KeyboardWaitKind::KeyboardWaitKind_WaitForEvent)
    {
        LogThrow_IfFailed(WindowHelper::GetTestHooks()->EnableKeyboardInputEvent());
    }

    if (pWindowHelper && (s_waitKind & test_infra::KeyboardWaitKind::KeyboardWaitKind_WaitForIdleBefore))
    {
        LogThrow_IfFailed(pWindowHelper->WaitForIdle());
    }

    HRESULT hr = RpcSendKeyInput(keyCode, down, scanCode);
    if (SUCCEEDED(hr))
    {
        if (s_waitKind & test_infra::KeyboardWaitKind::KeyboardWaitKind_WaitForEvent)
        {
            if (pWindowHelper && (s_waitKind & test_infra::KeyboardWaitKind::KeyboardWaitKind_WaitForIdleAfter))
            {
                LogThrow_IfFailed(pWindowHelper->WaitForIdle());
            }
            // Wait for XAML to acknowledge KeyUp/Down received, so it adds predictable delay here
            DWORD waitResult = ::WaitForSingleObject(m_keyboardInputHandle, IsDebuggerPresent() ? INFINITE : static_cast<DWORD>(Event::GetDefaultTextInputTimeout().count()));
            if (waitResult != WAIT_OBJECT_0)
            {
                LOG_OUTPUT(L"Verifying that KeyboardInputEvent firing is enabled through test hook: 0x%X", WindowHelper::GetTestHooks().Get());

                const bool canFireKeyboardInputEvent = WindowHelper::GetTestHooks()->CanFireKeyboardInputEvent();

                if (canFireKeyboardInputEvent == false)
                {
                    LOG_ERROR_SC(L"Attempting to wait for KEY DOWN event, KeyCore:%ud", keyCode);
                    return E_UNEXPECTED;
                }

                BOOLEAN isDesktop = FALSE;
                LogThrow_IfFailed(Utilities::IsDesktopStatic(&isDesktop));

                // On desktop, injecting mouse input as workaround to free the stuck key injection
                if (isDesktop)
                {
                    // TODO: remove workaround once stuck key injection issue is fixed
                    LOG_OUTPUT(L"Immediate keyboard input wait timeout, injecting some mouse input events to unblock the stuck key.");
                    POINT position = { 0, 0 };
                    // injecting mouse input form (0,0) to (6,6) should not go to XAML
                    for (int i = 0; i < 3; i++)
                    {
                        RpcSendMouseMoveInput(position);
                        position.x += 2;
                        position.y += 2;
                    }
                }

                waitResult = ::WaitForSingleObject(m_keyboardInputHandle, static_cast<long>(Event::GetDefaultTimeout().count()));
                if (waitResult != WAIT_OBJECT_0)
                {
                    LOG_ERROR_SC(L"WaitForSingleObject: XAML %ws event...", s_keyboardInputHandleName);
                    hr = E_UNEXPECTED;
                }
            }

            WindowHelper::GetTestHooks()->DisableKeyboardInputEvent();
        }
        else if (s_waitKind & test_infra::KeyboardWaitKind::KeyboardWaitKind_Sleep)
        {
            // Fallback to Sleep to avoid filling the input queue
            Sleep(30);
        }
    }

    return hr;
}
} }
