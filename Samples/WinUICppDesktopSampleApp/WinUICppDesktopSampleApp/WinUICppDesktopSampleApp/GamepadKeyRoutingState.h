// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include <string>

namespace WinUICppDesktopSampleApp
{
    // Returns the process-wide gamepad key routing state as "Enabled", "Disabled", or
    // "Unavailable". See GamepadKeyRoutingState.cpp for why this lives in its own file.
    std::wstring GetGamepadKeyRoutingState();
}
