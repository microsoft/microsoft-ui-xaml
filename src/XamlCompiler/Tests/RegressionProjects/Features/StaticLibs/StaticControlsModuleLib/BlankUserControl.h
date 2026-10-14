// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#include "BlankUserControl.g.h"

namespace winrt::StaticControlsModuleLib::implementation
{
    struct BlankUserControl : BlankUserControlT<BlankUserControl>
    {
        BlankUserControl();

        hstring Marker() const
        {
            return L"BlankUserControl";
        }
    };
}

namespace winrt::StaticControlsModuleLib::factory_implementation
{
    struct BlankUserControl : BlankUserControlT<BlankUserControl, implementation::BlankUserControl>
    {
    };
}
