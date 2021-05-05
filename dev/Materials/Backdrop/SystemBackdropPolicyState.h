#pragma once

#include <type_traits>

namespace SystemBackdropComponentInternal
{
    enum class SystemBackdropPolicyState : unsigned char
    {
        Active = 0x0,
        WindowOutOfFocus = 0x1,
        PowerSavingMode = WindowOutOfFocus << 1,
        HighContrastMode = PowerSavingMode << 1,
        IncompatibleGraphicsDevice = HighContrastMode << 1,
        TransparencyDisabled = IncompatibleGraphicsDevice << 1,
    };

    inline SystemBackdropPolicyState operator~(const SystemBackdropPolicyState& rhs)
    {
        return static_cast<SystemBackdropPolicyState>(
            ~static_cast<std::underlying_type<SystemBackdropPolicyState>::type>(rhs));
    }

    inline SystemBackdropPolicyState operator|(const SystemBackdropPolicyState& lhs, const SystemBackdropPolicyState& rhs)
    {
        return static_cast<SystemBackdropPolicyState>(
            static_cast<std::underlying_type<SystemBackdropPolicyState>::type>(lhs) |
            static_cast<std::underlying_type<SystemBackdropPolicyState>::type>(rhs));
    }

    inline SystemBackdropPolicyState& operator|=(SystemBackdropPolicyState& lhs, const SystemBackdropPolicyState& rhs)
    {
        lhs = lhs | rhs;
        return lhs;
    }

    inline SystemBackdropPolicyState operator&(const SystemBackdropPolicyState& lhs, const SystemBackdropPolicyState& rhs)
    {
        return static_cast<SystemBackdropPolicyState>(
            static_cast<std::underlying_type<SystemBackdropPolicyState>::type>(lhs) &
            static_cast<std::underlying_type<SystemBackdropPolicyState>::type>(rhs));
    }

    inline SystemBackdropPolicyState& operator&=(SystemBackdropPolicyState& lhs, const SystemBackdropPolicyState& rhs)
    {
        lhs = lhs & rhs;
        return lhs;
    }
}