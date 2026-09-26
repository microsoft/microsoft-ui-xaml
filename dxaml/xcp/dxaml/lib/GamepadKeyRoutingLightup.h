// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace DirectUI
{
    // Enables routing of gamepad input to this process as keyboard virtual keys, so that XAML's
    // existing keyboard-driven navigation (XY focus, Enter/Space to invoke, Esc to dismiss) responds
    // to a gamepad without any app-specific code. See docs\design-notes\gamepad-navigation.md.
    //
    // The underlying OS setting is process-wide, so this is called only for a full WinUI app, from
    // Application.Start(). A process that embeds XAML only as an island never reaches that call and
    // is deliberately left at the OS default. An app opts back out by calling
    // Windows.UI.Input.GamepadKeyRoutingConfiguration.TrySetKeyRoutingEnabled(false) any time after
    // Application.Start().
    //
    // Defined in a light-up translation unit because the OS API requires a newer UAP contract than
    // the one this build normally targets.
    _Check_return_ HRESULT EnableGamepadKeyRouting();
}
