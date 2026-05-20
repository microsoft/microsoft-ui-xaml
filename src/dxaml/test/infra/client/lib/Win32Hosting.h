// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <wil\result.h>

namespace Win32Hosting
{
    wrl::ComPtr<test_infra::Hosting::IWin32Host> StartWin32Host(
        const wchar_t* factory,
        test_infra::Hosting::DpiAwarenessContext dpiAwarenessContext,
        bool initCore);
    wrl::ComPtr<msy::IDispatcherQueue> GetDispatcherQueueFromWin32XamlContentRoot(wrl::ComPtr<test_infra::Hosting::IWin32Host> win32Host);
}
