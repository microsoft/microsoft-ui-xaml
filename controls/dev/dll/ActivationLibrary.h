// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <windows.h>

namespace MuxcActivation
{
    inline HMODULE LoadLibraryForActivation(PCWSTR name) noexcept
    {
        HMODULE module = LoadLibraryW(name);
        if (!module)
        {
            // Preserve existing lookup before trying directories registered with AddDllDirectory.
            module = LoadLibraryExW(name, nullptr, LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
        }
        return module;
    }
}
