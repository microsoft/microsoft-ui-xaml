// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <windows.h>

inline HMODULE LoadLibraryForActivation(_In_z_ PCWSTR libraryName) noexcept
{
    HMODULE module = LoadLibraryW(libraryName);
    if (!module)
    {
        // Preserve existing lookup before trying directories registered with AddDllDirectory.
        module = LoadLibraryExW(libraryName, nullptr, LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    }
    return module;
}
