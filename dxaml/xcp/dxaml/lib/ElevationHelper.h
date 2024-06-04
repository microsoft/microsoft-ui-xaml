// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Helper functions for dealing with ThemeShadow and elevation

#pragma once

#include <optional>

namespace DirectUI
{
    // Move the control forward in Z and apply a shadow effect to it.
    // If the control is part of a tier of elevated controls (for example a MenuFlyoutSubItem),
    // you may provide an additional "depth" value that provides an additional Z offset.
    _Check_return_ HRESULT ApplyElevationEffect(_In_ xaml::IUIElement* target, unsigned int depth = 0, std::optional<int> baseElevation = std::nullopt);

    // Remove any shadow applied with ApplyElevationEffect
    _Check_return_ HRESULT ClearElevationEffect(_In_ xaml::IUIElement* target);

    // Checks if the "IsDefaultShadowEnabled" resource is defined as True or not, determining
    // if a control should enable a shadow by default.
    _Check_return_ HRESULT IsDefaultShadowEnabled(_In_ xaml::IFrameworkElement* resourceTarget, _Inout_ bool *enabled);
}
