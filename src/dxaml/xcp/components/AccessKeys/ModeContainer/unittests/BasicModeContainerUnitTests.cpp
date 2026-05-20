// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "BasicModeContainerUnitTests.h"
#include <paltypes.h>
#include <XamlLogging.h>
#include "ModeContainer.h"

bool eventRaised = false;
HRESULT expectedHR = S_OK;

namespace AccessKeys
{
    _Check_return_ HRESULT AKOnIsActiveChanged(_In_opt_ CFocusManager* focusManager, _In_opt_ IInspectable* sender, _In_opt_ IInspectable* args)
    {
        eventRaised = true;
        return expectedHR;
    }
}

using namespace WEX::Common;

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace AccessKeys {

bool BasicModeContainerUnitTests::ClassSetup()
{
    eventRaised = false;
    expectedHR = S_OK;
    return true;
}

void BasicModeContainerUnitTests::SmokeTest()
{
    ::AccessKeys::AKModeContainer container;
    eventRaised = false;

    // default to false
    VERIFY_IS_FALSE(container.GetIsActive());
    VERIFY_SUCCEEDED(container.SetIsActive(false));
    VERIFY_IS_FALSE(eventRaised);

    // basic set/get
    VERIFY_SUCCEEDED(container.SetIsActive(true));
    VERIFY_IS_TRUE(container.GetIsActive());

    VERIFY_IS_TRUE(eventRaised);
    eventRaised = false;

    // Changing property should fire event
    VERIFY_SUCCEEDED(container.SetIsActive(false));
    VERIFY_IS_TRUE(eventRaised);
    eventRaised = false;

    // Setting property to same value should not fire event
    VERIFY_SUCCEEDED(container.SetIsActive(false));
    VERIFY_IS_FALSE(eventRaised);

    VERIFY_SUCCEEDED(container.SetIsActive(false));
    VERIFY_SUCCEEDED(container.SetIsActive(true));
    VERIFY_IS_TRUE(container.GetIsActive());
}

InputMessage* GetInputMessage(bool isAltKey, MessageMap msgId, wsy::VirtualKey keyCode, bool isMenuKey)
{
    static InputMessage msg { };

    msg.m_modifierKeys = (isAltKey && msgId == XCP_KEYDOWN) ? KEY_MODIFIER_ALT : 0;
    msg.m_physicalKeyStatus.m_bIsExtendedKey = false;

    msg.m_msgID = msgId;
    msg.m_platformKeyCode = keyCode;

    msg.m_physicalKeyStatus.m_bIsMenuKeyDown = !!isMenuKey;
    msg.m_physicalKeyStatus.m_bWasKeyDown = !isMenuKey;

    return &msg;
}

void BasicModeContainerUnitTests::EnterViaKeyPress()
{
    ::AccessKeys::AKModeContainer container;

    VERIFY_IS_FALSE(container.GetIsActive());

    LOG_OUTPUT(L"Simulate press of alt to activate access key navigation");
    bool shouldEvaluate = false;
    VERIFY_SUCCEEDED(container.EvaluateAccessKeyMode(GetInputMessage(true /*isAltKey*/, XCP_KEYDOWN, wsy::VirtualKey::VirtualKey_Menu, false /*MenuKey*/), &shouldEvaluate));
    VERIFY_IS_FALSE(shouldEvaluate);
    VERIFY_IS_FALSE(container.GetIsActive());
    VERIFY_IS_FALSE(container.HasAKModeChanged());

    VERIFY_SUCCEEDED(container.EvaluateAccessKeyMode(GetInputMessage(true /*isAltKey*/, XCP_KEYUP, wsy::VirtualKey::VirtualKey_Menu, false /*MenuKey*/), &shouldEvaluate));
    VERIFY_IS_TRUE(shouldEvaluate);
    VERIFY_IS_TRUE(container.GetIsActive());
    VERIFY_IS_TRUE(container.HasAKModeChanged());

    LOG_OUTPUT(L"Simulate tapping alt key to exit access key navigation");
    VERIFY_SUCCEEDED(container.EvaluateAccessKeyMode(GetInputMessage(true /*isAltKey*/, XCP_KEYUP, wsy::VirtualKey::VirtualKey_Menu, false /*MenuKey*/), &shouldEvaluate));
    VERIFY_IS_TRUE(shouldEvaluate);
    VERIFY_IS_FALSE(container.GetIsActive());
    VERIFY_IS_TRUE(container.HasAKModeChanged());
}

void BasicModeContainerUnitTests::ShouldEvaluateProperMessage()
{
    ::AccessKeys::AKModeContainer container;

    bool shouldEvaluate = false;
    VERIFY_SUCCEEDED(container.EvaluateAccessKeyMode(GetInputMessage(true /*isAltKey*/, XCP_KEYDOWN, wsy::VirtualKey::VirtualKey_Menu, false /*MenuKey*/), &shouldEvaluate));
    VERIFY_IS_FALSE(shouldEvaluate);
    VERIFY_IS_FALSE(container.GetIsActive());
    VERIFY_IS_FALSE(container.HasAKModeChanged());

    VERIFY_SUCCEEDED(container.EvaluateAccessKeyMode(GetInputMessage(true /*isAltKey*/, XCP_KEYUP, wsy::VirtualKey::VirtualKey_Menu, false /*MenuKey*/), &shouldEvaluate));
    VERIFY_IS_TRUE(shouldEvaluate);
    VERIFY_IS_TRUE(container.GetIsActive());
    VERIFY_IS_TRUE(container.HasAKModeChanged());

    VERIFY_SUCCEEDED(container.EvaluateAccessKeyMode(GetInputMessage(false /*isAltKey*/, XCP_CHAR, wsy::VirtualKey::VirtualKey_None, false /*MenuKey*/), &shouldEvaluate));
    VERIFY_IS_TRUE(shouldEvaluate);
    VERIFY_IS_TRUE(container.GetIsActive());
    VERIFY_IS_FALSE(container.HasAKModeChanged());

    VERIFY_SUCCEEDED(container.EvaluateAccessKeyMode(GetInputMessage(false /*isAltKey*/, XCP_CHAR, wsy::VirtualKey::VirtualKey_None, false /*MenuKey*/), &shouldEvaluate));
    VERIFY_IS_TRUE(shouldEvaluate);
    VERIFY_IS_TRUE(container.GetIsActive());
    VERIFY_IS_FALSE(container.HasAKModeChanged());

    VERIFY_SUCCEEDED(container.EvaluateAccessKeyMode(GetInputMessage(true /*isAltKey*/, XCP_KEYUP, wsy::VirtualKey::VirtualKey_Menu, false /*MenuKey*/), &shouldEvaluate));
    VERIFY_IS_TRUE(shouldEvaluate);
    VERIFY_IS_TRUE(container.GetIsActive());
    VERIFY_IS_FALSE(container.HasAKModeChanged());
}

void BasicModeContainerUnitTests::EnterViaHotkey()
{
    ::AccessKeys::AKModeContainer container;

    bool shouldEvaluate = false;
    VERIFY_SUCCEEDED(container.EvaluateAccessKeyMode(GetInputMessage(false /*isAltKey*/, XCP_KEYDOWN, wsy::VirtualKey::VirtualKey_S, true /*wsy::VirtualKey*/), &shouldEvaluate));
    VERIFY_IS_TRUE(shouldEvaluate);
    VERIFY_IS_FALSE(container.GetIsActive());
    VERIFY_IS_TRUE(container.HasAKModeChanged());
}

void BasicModeContainerUnitTests::VerifyMultipleHotkeyPresses()
{
    ::AccessKeys::AKModeContainer container;

    bool shouldEvaluate = false;
    VERIFY_SUCCEEDED(container.EvaluateAccessKeyMode(GetInputMessage(false /*isAltKey*/, XCP_KEYDOWN, wsy::VirtualKey::VirtualKey_M, true /*MenuKey*/), &shouldEvaluate));
    VERIFY_IS_TRUE(shouldEvaluate);
    VERIFY_IS_FALSE(container.GetIsActive());
    VERIFY_IS_TRUE(container.HasAKModeChanged());

    VERIFY_SUCCEEDED(container.EvaluateAccessKeyMode(GetInputMessage(false /*isAltKey*/, XCP_KEYDOWN, wsy::VirtualKey::VirtualKey_A, true /*MenuKey*/), &shouldEvaluate));
    VERIFY_IS_TRUE(shouldEvaluate);
    VERIFY_IS_FALSE(container.GetIsActive());
    VERIFY_IS_TRUE(container.HasAKModeChanged());

    VERIFY_SUCCEEDED(container.EvaluateAccessKeyMode(GetInputMessage(false /*isAltKey*/, XCP_KEYDOWN, wsy::VirtualKey::VirtualKey_T, true /*MenuKey*/), &shouldEvaluate));
    VERIFY_IS_TRUE(shouldEvaluate);
    VERIFY_IS_FALSE(container.GetIsActive());
    VERIFY_IS_TRUE(container.HasAKModeChanged());

    VERIFY_SUCCEEDED(container.EvaluateAccessKeyMode(GetInputMessage(false /*isAltKey*/, XCP_KEYDOWN, wsy::VirtualKey::VirtualKey_H, true /*MenuKey*/), &shouldEvaluate));
    VERIFY_IS_TRUE(shouldEvaluate);
    VERIFY_IS_FALSE(container.GetIsActive());
    VERIFY_IS_TRUE(container.HasAKModeChanged());
}

void BasicModeContainerUnitTests::HotkeyShouldNotEnterAKMode()
{
    ::AccessKeys::AKModeContainer container;

    bool shouldEvaluate = false;
    VERIFY_SUCCEEDED(container.EvaluateAccessKeyMode(GetInputMessage(false /*isAltKey*/, XCP_KEYDOWN, wsy::VirtualKey::VirtualKey_S, true /*MenuKey*/), &shouldEvaluate));
    VERIFY_IS_TRUE(shouldEvaluate);
    VERIFY_IS_FALSE(container.GetIsActive());
    VERIFY_IS_TRUE(container.HasAKModeChanged());

    VERIFY_SUCCEEDED(container.EvaluateAccessKeyMode(GetInputMessage(false /*isAltKey*/, XCP_CHAR, wsy::VirtualKey::VirtualKey_None, false /*MenuKey*/), &shouldEvaluate));
    VERIFY_IS_FALSE(shouldEvaluate);
    VERIFY_IS_FALSE(container.GetIsActive());
    VERIFY_IS_FALSE(container.HasAKModeChanged());
}


void BasicModeContainerUnitTests::InvalidResultIsReported()
{
    ::AccessKeys::AKModeContainer container;

    expectedHR = E_NOTIMPL;
    bool shouldEvaluate = false;
    VERIFY_SUCCEEDED(container.EvaluateAccessKeyMode(GetInputMessage(true /*isAltKey*/, XCP_KEYDOWN, wsy::VirtualKey::VirtualKey_Menu, false /*MenuKey*/), &shouldEvaluate));
    VERIFY_ARE_EQUAL(expectedHR, container.EvaluateAccessKeyMode(GetInputMessage(true /*isAltKey*/, XCP_KEYUP, wsy::VirtualKey::VirtualKey_Menu, false /*MenuKey*/), &shouldEvaluate));
    VERIFY_IS_TRUE(container.GetIsActive());
    expectedHR = S_OK;
}

void BasicModeContainerUnitTests::TabShouldExitAKMode()
{
    ::AccessKeys::AKModeContainer container;
    bool shouldEvaluate = false;

    InputMessage message;
    message.m_platformKeyCode = wsy::VirtualKey::VirtualKey_Tab;

    VERIFY_SUCCEEDED(container.SetIsActive(true));
    VERIFY_SUCCEEDED(container.EvaluateAccessKeyMode(&message, &shouldEvaluate));

    VERIFY_IS_TRUE(shouldEvaluate);
    VERIFY_IS_TRUE(container.ShouldForciblyExitAKMode());
    VERIFY_IS_FALSE(container.GetIsActive());
}

void BasicModeContainerUnitTests::DirectionKeysShouldExitAKMode()
{
    ::AccessKeys::AKModeContainer container;
    bool shouldEvaluate = false;

    InputMessage message;
    message.m_platformKeyCode = wsy::VirtualKey::VirtualKey_Down;

    VERIFY_SUCCEEDED(container.SetIsActive(true));
    VERIFY_SUCCEEDED(container.EvaluateAccessKeyMode(&message, &shouldEvaluate));

    VERIFY_IS_TRUE(shouldEvaluate);
    VERIFY_IS_TRUE(container.ShouldForciblyExitAKMode());
    VERIFY_IS_FALSE(container.GetIsActive());

    message.m_platformKeyCode = wsy::VirtualKey::VirtualKey_Up;

    VERIFY_SUCCEEDED(container.SetIsActive(true));
    VERIFY_SUCCEEDED(container.EvaluateAccessKeyMode(&message, &shouldEvaluate));

    VERIFY_IS_TRUE(shouldEvaluate);
    VERIFY_IS_TRUE(container.ShouldForciblyExitAKMode());
    VERIFY_IS_FALSE(container.GetIsActive());

    message.m_platformKeyCode = wsy::VirtualKey::VirtualKey_Left;

    VERIFY_SUCCEEDED(container.SetIsActive(true));
    VERIFY_SUCCEEDED(container.EvaluateAccessKeyMode(&message, &shouldEvaluate));

    VERIFY_IS_TRUE(shouldEvaluate);
    VERIFY_IS_TRUE(container.ShouldForciblyExitAKMode());
    VERIFY_IS_FALSE(container.GetIsActive());

    message.m_platformKeyCode = wsy::VirtualKey::VirtualKey_Right;

    VERIFY_SUCCEEDED(container.SetIsActive(true));
    VERIFY_SUCCEEDED(container.EvaluateAccessKeyMode(&message, &shouldEvaluate));

    VERIFY_IS_TRUE(shouldEvaluate);
    VERIFY_IS_TRUE(container.ShouldForciblyExitAKMode());
    VERIFY_IS_FALSE(container.GetIsActive());
}

void BasicModeContainerUnitTests::EscapeKeyNotHandledWithModifiers()
{
    ::AccessKeys::AKModeContainer container;
    bool shouldEvaluate = false;
    InputMessage message;

    VERIFY_SUCCEEDED(container.SetIsActive(true));

    message.m_msgID = XCP_KEYUP;
    message.m_modifierKeys = KEY_MODIFIER_CTRL;
    message.m_platformKeyCode = wsy::VirtualKey::VirtualKey_Escape;
    message.m_physicalKeyStatus.m_bIsMenuKeyDown = false;
    message.m_physicalKeyStatus.m_bWasKeyDown = false;

    VERIFY_SUCCEEDED(container.EvaluateAccessKeyMode(&message, &shouldEvaluate));
    VERIFY_IS_FALSE(shouldEvaluate);

    message.m_msgID = XCP_KEYDOWN;

    VERIFY_SUCCEEDED(container.EvaluateAccessKeyMode(&message, &shouldEvaluate));
    VERIFY_IS_FALSE(shouldEvaluate);
}

} } } } }
