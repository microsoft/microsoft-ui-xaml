// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <cstdint>
#include <functional>
#include <vector>

#include <winrt/Microsoft.UI.Xaml.Data.h>
#include <winrt/Windows.Foundation.h>

#include "ShapingHelpers.h"

// The selector type the Tabular rows stack reads items through.

namespace ShapingHelpers
{
    // Extracts a value from an item (for example a tree's node key). Returning nullptr is legal;
    // throwing is the caller's concern, not this layer's.
    using KeySelector = std::function<winrt::IInspectable(winrt::IInspectable const& item)>;
}
