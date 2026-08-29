// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <flags_enum.h>

enum class DirtyFlags
{
    None = 0x00,
    Render = 0x01,
    Bounds = 0x02,
    Independent = 0x04,
    ForcePropagate = 0x08,  // Only applies to content/subgraph dirty
};

template <>
struct is_flags_enum<DirtyFlags>
{
    static constexpr bool value = true;
};

class CDependencyObject;

typedef void(__stdcall *RENDERCHANGEDPFN)(
    _In_ CDependencyObject* pTarget,
    DirtyFlags flags
    );
