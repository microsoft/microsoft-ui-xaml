// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "PropertyPathCommonNames.h"

// Static table of commonly used property path strings.
// These are stored in read-only data and we can avoid heap allocations by 
// pointing to them directly when a parsed property name matches.
//
// We keep lengths in a separate array for faster scanning - the length check
// can reject most candidates without touching the name pointer array.

// X-macro: define each common property name once, expand for both arrays
#define COMMON_PROPERTY_NAMES(X) \
    X("Description") \
    X("Label") \
    X("CommandBarTemplateSettings") \
    X("TemplateSettings") \
    X("AccessKey") \
    X("KeyboardAccelerators") \
    X("IconSource") \
    X("IsEnabled") \
    X("IsVisible")

// Generate the name pointer array
#define EXPAND_NAME(str) L##str,
static const WCHAR* const s_commonPropertyNames[] = {
    COMMON_PROPERTY_NAMES(EXPAND_NAME)
};
#undef EXPAND_NAME

// Generate the lengths array (sizeof includes null terminator, so subtract 1)
// Using constexpr to ensure compile-time evaluation and avoid duplicate copies in TUs
#define EXPAND_LEN(str) static_cast<uint8_t>(std::size(L##str) - 1),
static constexpr uint8_t s_commonPropertyLengths[] = {
    COMMON_PROPERTY_NAMES(EXPAND_LEN)
};
#undef EXPAND_LEN

#undef COMMON_PROPERTY_NAMES

static_assert(std::size(s_commonPropertyNames) == std::size(s_commonPropertyLengths), 
    "Name and length arrays must have the same number of entries");

namespace DirectUI
{
    const WCHAR* TryGetCommonPropertyName(
        _In_reads_(length) const WCHAR* name,
        size_t length) noexcept
    {
        // Quick reject: length must fit in uint8_t to match any entry
        if (length > 255)
        {
            return nullptr;
        }

        const uint8_t targetLength = static_cast<uint8_t>(length);

        // Scan lengths first - this array is compact and cache-friendly
        for (size_t i = 0; i < std::size(s_commonPropertyLengths); ++i)
        {
            if (s_commonPropertyLengths[i] == targetLength)
            {
                // Length matches - now do the full string comparison
                if (wcsncmp(s_commonPropertyNames[i], name, length) == 0)
                {
                    return s_commonPropertyNames[i];
                }
            }
        }
        return nullptr;
    }
}
