#pragma once

#include "SystemBackdropPolicyState.h"

namespace SystemBackdropComponentInternal
{
    class SystemBackdropPolicyStateMachine final
    {
    public:
        SystemBackdropPolicyStateMachine(SystemBackdropPolicyState intialState);

        void SetWindowNotInFocus(bool isNotInFocus);
        void SetPowerSavingMode(bool isEnabled);
        void SetHighContrastMode(bool isEnabled);
        void SetIncompatibleGraphicsDevice(bool isIncompatibleGraphicsDevice);
        void SetTransparencyDisabled(bool isDisabled);

        bool IsActive() const;

    private:
        SystemBackdropPolicyState m_state;
    };
}