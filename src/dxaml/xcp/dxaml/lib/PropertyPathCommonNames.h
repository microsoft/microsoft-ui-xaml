// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace DirectUI
{
    // Looks up a property name in the common strings table.
    // If found, returns a pointer to the static string. If not found, returns nullptr.
    // This avoids heap allocations for frequently-used property name strings.
    const WCHAR* TryGetCommonPropertyName(
        _In_reads_(length) const WCHAR* name,
        size_t length) noexcept;
}
