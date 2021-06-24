#include "pch.h"
#include "common.h"

#include "SystemBackdropPolicyStateMachine.h"

namespace SystemBackdropComponentInternal
{
    SystemBackdropPolicyStateMachine::SystemBackdropPolicyStateMachine(SystemBackdropPolicyState initialState) :
        m_state(initialState)
    {

    }

    void SystemBackdropPolicyStateMachine::SetWindowNotInFocus(bool isNotInFocus)
    {
        if (isNotInFocus)
        {
            m_state |= SystemBackdropPolicyState::WindowOutOfFocus;
        }
        else
        {
            m_state &= ~SystemBackdropPolicyState::WindowOutOfFocus;
        }
    }

    void SystemBackdropPolicyStateMachine::SetPowerSavingMode(bool isEnabled)
    {
        if (isEnabled)
        {
            m_state |= SystemBackdropPolicyState::PowerSavingMode;
        }
        else
        {
            m_state &= ~SystemBackdropPolicyState::PowerSavingMode;
        }
    }

    void SystemBackdropPolicyStateMachine::SetHighContrastMode(bool isEnabled)
    {
        if (isEnabled)
        {
            m_state |= SystemBackdropPolicyState::HighContrastMode;
        }
        else
        {
            m_state &= ~SystemBackdropPolicyState::HighContrastMode;
        }
    }

    void SystemBackdropPolicyStateMachine::SetIncompatibleGraphicsDevice(bool isIncompatibleGraphicsDevice)
    {
        if (isIncompatibleGraphicsDevice)
        {
            m_state |= SystemBackdropPolicyState::IncompatibleGraphicsDevice;
        }
        else
        {
            m_state &= ~SystemBackdropPolicyState::IncompatibleGraphicsDevice;
        }
    }

    void SystemBackdropPolicyStateMachine::SetTransparencyDisabled(bool isDisabled)
    {
        if (isDisabled)
        {
            m_state |= SystemBackdropPolicyState::TransparencyDisabled;
        }
        else
        {
            m_state &= ~SystemBackdropPolicyState::TransparencyDisabled;
        }
    }

    bool SystemBackdropPolicyStateMachine::IsActive() const
    {
        return m_state == SystemBackdropPolicyState::Active;
    }
}
