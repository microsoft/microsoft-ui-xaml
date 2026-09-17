// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "TestPoolFilter.h"
#include <TestServices.h>

namespace Private { namespace Infrastructure {

    class KeyboardHelper : public Microsoft::WRL::RuntimeClass<test_infra::IKeyboardHelper>
    {
        InspectableClass(RuntimeClass_Private_Infrastructure_KeyboardHelper, TrustLevel::BaseTrust);

    public:
        KeyboardHelper(DWORD uiThreadId)
            : m_keyboardInputHandle(OpenNamedEvent(uiThreadId, s_keyboardInputHandleName))
        {
            WEX::Common::Throw::LastErrorIf(!m_keyboardInputHandle.IsValid(), L"Failed to create KeyboardInputReceived handle.");
        }

        IFACEMETHOD(RuntimeClassInitialize)();

        IFACEMETHOD(Enter)() override;
        IFACEMETHOD(Space)() override;
        IFACEMETHOD(Escape)() override;
        IFACEMETHOD(CtrlTab)() override;
        IFACEMETHOD(ShiftTab)() override;
        IFACEMETHOD(AltTab)() override;
        IFACEMETHOD(Tab)() override;
        IFACEMETHOD(Up)() override;
        IFACEMETHOD(Down)() override;
        IFACEMETHOD(Left)() override;
        IFACEMETHOD(Right)() override;
        IFACEMETHOD(Home)() override;
        IFACEMETHOD(End)() override;
        IFACEMETHOD(PageUp)() override;
        IFACEMETHOD(PageDown)() override;
        IFACEMETHOD(Backspace)() override;
        IFACEMETHOD(Insert)() override;
        IFACEMETHOD(Delete)() override;
        IFACEMETHOD(Alt)() override;
        IFACEMETHOD(Control)() override;
        IFACEMETHOD(GamepadDpadRight)() override;
        IFACEMETHOD(GamepadDpadDown)() override;
        IFACEMETHOD(GamepadDpadUp)() override;
        IFACEMETHOD(GamepadDpadLeft)() override;
        IFACEMETHOD(GamepadA)() override;
        IFACEMETHOD(GamepadB)() override;
        IFACEMETHOD(GamepadLeftShoulder)() override;
        IFACEMETHOD(GamepadLeftTrigger)() override;
        IFACEMETHOD(GamepadRightShoulder)() override;
        IFACEMETHOD(GamepadRightTrigger)() override;
        IFACEMETHOD(GamePadLeftThumbStickRight)() override;
        IFACEMETHOD(GamePadLeftThumbStickDown)() override;
        IFACEMETHOD(GamePadLeftThumbStickUp)() override;
        IFACEMETHOD(GamePadLeftThumbStickLeft)() override;
        IFACEMETHOD(GamePadRightThumbStickRight)() override;
        IFACEMETHOD(GamePadRightThumbStickDown)() override;
        IFACEMETHOD(GamePadRightThumbStickUp)() override;
        IFACEMETHOD(GamePadRightThumbStickLeft)() override;
        IFACEMETHOD(GamePadMenu)() override;
        IFACEMETHOD(Copy)() override;
        IFACEMETHOD(Cut)() override;
        IFACEMETHOD(Paste)() override;
        IFACEMETHOD(PressKeySequence)(HSTRING keySeq) override;

        IFACEMETHOD(SetWaitKind)(test_infra::KeyboardWaitKind waitKind) override;
        static void SetWaitKindStatic(test_infra::KeyboardWaitKind waitKind);

        static test_infra::KeyboardWaitKind GetWaitKind() { return s_waitKind; };

    private:
        std::map<std::wstring, UINT16> m_vKeyMapping;
        std::map<std::wstring, UINT16> m_scanCodeMapping;
        bool MapKeyCode(std::wstring key, UINT16 &keyCode);

        static const wchar_t* s_keyboardInputHandleName;
        Microsoft::UI::Xaml::Tests::Common::Handle m_keyboardInputHandle;

        HRESULT SendKeyInput(UINT16 keyCode, bool down, bool scanCode, test_infra::IWindowHelper* pWindowHelper);
        static test_infra::KeyboardWaitKind s_waitKind;
    };

    class KeyboardHelperTestPoolFilter : public TestPoolFilter
    {
    public:
        const wchar_t* GetName() const override { return L"KeyboardHelperTestPoolFilter"; }

        KeyboardHelperTestPoolFilter();

        bool IsDirty() override
        {
            return (KeyboardHelper::GetWaitKind() != m_default);
        }

    private:
        test_infra::KeyboardWaitKind m_default;
    };

} }
