// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

// There are certain features/behaviors that we want to turn off under the designer because they make for a bad
// designer experience.
enum class DesignerMode : uint8_t
{
    None        = 0x0,         // Not in design mode
    V2Only      = 0x2,         // Should be used by DCompTreeHost for hooking up visual to CoreApplication.
};

namespace DirectUI
{
    class DxamlCoreTestHooks;
}

DEFINE_ENUM_FLAG_OPERATORS(DesignerMode);

class DesignerInterop
{
    // Only allow the window test hooks to call SetAllowDesignModeMock
    friend DirectUI::DxamlCoreTestHooks;
public:

    static bool GetDesignerMode(DesignerMode mode);

private:

    // Used via test infrastructre. Allows people to mock the RoGetDesignMode* APIs
    // so that they will return true during tests.
    static void SetAllowDesignModeMock(bool allowMocks);
    static DesignerMode EvaluateDesignerMode();

    static bool s_allowMocks;
    static UINT32 s_hasCheckedDesignMode;
};
