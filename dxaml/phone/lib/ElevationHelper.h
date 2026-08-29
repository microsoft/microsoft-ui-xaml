// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Helper functions for dealing with ThemeShadow and elevation

#pragma once

XAML_ABI_NAMESPACE_BEGIN namespace Microsoft { namespace UI { namespace Xaml { namespace Controls
{
    // Move the control forward in Z and apply a shadow effect to it.
    // If the control is part of a tier of elevated controls (for example a MenuFlyoutSubItem),
    // you may provide an additional "depth" value that provides an additional Z offset.
    _Check_return_ HRESULT ApplyElevationEffect(_In_ xaml::IUIElement* target, unsigned int depth = 0);
} } } } XAML_ABI_NAMESPACE_END
