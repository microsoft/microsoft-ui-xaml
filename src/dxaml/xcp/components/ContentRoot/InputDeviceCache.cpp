// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "InputManager.h"
#include "InputDeviceCache.h"

#include "XboxUtility.h"

#include "corep.h"

using namespace ContentRootInput;

InputDeviceCache::InputDeviceCache(_In_ CInputManager& inputManager)
    : m_inputManager(inputManager)
{
}

void InputDeviceCache::Init()
{
    if (XboxUtility::IsOnXbox())
    {
        //Start with a good default for Xbox, will re-establish this when first user input comes in
        SetLastInputDeviceType(DirectUI::InputDeviceType::GamepadOrRemote);
    }
    else
    {
        SetLastInputDeviceType(DirectUI::InputDeviceType::None);
    }
}

DirectUI::FocusInputDeviceKind InputDeviceCache::GetLastFocusInputDeviceKind() const
{
    /* Expect InputDeviceType to match FocusInputDeviceKind*/
    static_assert(DirectUI::InputDeviceType::None == static_cast<DirectUI::InputDeviceType>(DirectUI::FocusInputDeviceKind::None), "InputDeviceType and FocusInputDeviceKind enums mismatched for None");
    static_assert(DirectUI::InputDeviceType::Mouse == static_cast<DirectUI::InputDeviceType>(DirectUI::FocusInputDeviceKind::Mouse), "InputDeviceType and FocusInputDeviceKind enums mismatched for Mouse");
    static_assert(DirectUI::InputDeviceType::Touch == static_cast<DirectUI::InputDeviceType>(DirectUI::FocusInputDeviceKind::Touch), "InputDeviceType and FocusInputDeviceKind enums mismatched for Touch");
    static_assert(DirectUI::InputDeviceType::Pen == static_cast<DirectUI::InputDeviceType>(DirectUI::FocusInputDeviceKind::Pen), "InputDeviceType and FocusInputDeviceKind enums mismatched for Pen");
    static_assert(DirectUI::InputDeviceType::Keyboard == static_cast<DirectUI::InputDeviceType>(DirectUI::FocusInputDeviceKind::Keyboard), "InputDeviceType and FocusInputDeviceKind enums mismatched for Keyboard");
    static_assert(DirectUI::InputDeviceType::GamepadOrRemote == static_cast<DirectUI::InputDeviceType>(DirectUI::FocusInputDeviceKind::GameController), "InputDeviceType and FocusInputDeviceKind enums mismatched for Gamepad");

    return static_cast<DirectUI::FocusInputDeviceKind>(m_eLastInputDeviceType);
}

DirectUI::InputDeviceType InputDeviceCache::GetLastInputDeviceType() const
{
    if (m_inputManager.m_pointerInputProcessor.HasPrimaryPointerLastPositionOverride())
    {
        return DirectUI::InputDeviceType::Mouse;
    }

    return m_eLastInputDeviceType;
}

void InputDeviceCache::SetLastInputDeviceType(DirectUI::InputDeviceType deviceType, bool keepUIAFocusState)
{
    m_eLastInputDeviceType = deviceType;

    if (!keepUIAFocusState)
    {
        m_uiaFocusSinceLastInput = false;
    }

    if (deviceType != DirectUI::InputDeviceType::Pen)
    {
        m_inputManager.m_pointerInputProcessor.SetBarrelButtonPressed(false);
    }

    const auto contentRoot = m_inputManager.GetContentRoot();
    // Verifies that when we have an Engaged Control (which also implies focus), make sure that the
    // input received is from an expected input device: Gamepad or Remote, otherwise remove engagement
    if (deviceType != DirectUI::InputDeviceType::GamepadOrRemote)
    {
        if (CControl *pEngagedControl = contentRoot->GetFocusManagerNoRef()->GetEngagedControlNoRef())
        {
            VERIFYHR(pEngagedControl->RemoveFocusEngagement());
        }
    }
}

bool InputDeviceCache::LastInputWasNonFocusNavigationKeyFromSIP() const
{
    ASSERT(m_eLastInputDeviceType == DirectUI::InputDeviceType::Keyboard);
    return m_lastInputWasNonFocusNavigationKeyFromSIP;
}

void InputDeviceCache::SetLastInputWasNonFocusNavigationKeyFromSIP(_In_ bool value)
{
    m_lastInputWasNonFocusNavigationKeyFromSIP = value;
}

void InputDeviceCache::OnSetFocusFromUIA()
{
    m_uiaFocusSinceLastInput = true;
}

bool InputDeviceCache::GetWasUIAFocusSetSinceLastInput() const
{
    return m_uiaFocusSinceLastInput;
}