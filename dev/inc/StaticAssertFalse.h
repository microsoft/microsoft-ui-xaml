// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace details
{
    template<typename >
    inline constexpr bool dependent_false_v{ false };
}

// Use this macro in place of static_assert(false,...)
// C++ compiler will always evaluate parse static_assert(false,...),
// raising the assert even if the method calling it is never used.
// Adding a template parameter causes it to only be compiled/assert
// if the method calling it is used
#define static_assert_false(message) \
while(1){ \
struct nested{}; \
static_assert(::details::dependent_false_v<nested>, message); \
}
