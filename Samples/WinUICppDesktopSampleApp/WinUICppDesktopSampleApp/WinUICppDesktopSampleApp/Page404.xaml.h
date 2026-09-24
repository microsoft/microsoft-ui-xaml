// Copyright (c) Microsoft Corporation and Contributors.
// Licensed under the MIT License.

#pragma once

#include "Page404.g.h"

namespace winrt::WinUICppDesktopSampleApp::implementation
{
    struct Page404 : Page404T<Page404>
    {
        Page404();
    };
}

namespace winrt::WinUICppDesktopSampleApp::factory_implementation
{
    struct Page404 : Page404T<Page404, implementation::Page404>
    {
    };
}
