// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#include "CppWinRTUserControlFromWinRTComponent.g.h"

namespace winrt::CppWinRTComponent::implementation
{
    struct CppWinRTUserControlFromWinRTComponent : CppWinRTUserControlFromWinRTComponentT<CppWinRTUserControlFromWinRTComponent>
    {
        CppWinRTUserControlFromWinRTComponent()
        {
            // Xaml objects should not call InitializeComponent during construction.
            // See https://github.com/microsoft/cppwinrt/tree/master/nuget#initializecomponent
        }
    };
}

namespace winrt::CppWinRTComponent::factory_implementation
{
    struct CppWinRTUserControlFromWinRTComponent : CppWinRTUserControlFromWinRTComponentT<CppWinRTUserControlFromWinRTComponent, implementation::CppWinRTUserControlFromWinRTComponent>
    {
    };
}
