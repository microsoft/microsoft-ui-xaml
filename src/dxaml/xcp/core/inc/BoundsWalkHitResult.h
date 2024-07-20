// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <flags_enum.h>

// There are multiple states for a hit testing walk. We could be looking for the element that will be hit. We could
// have hit an element in the subtree and be walking back out of the tree while collecting its ancestor elements.
// We could have hit an element in the subtree and decided not to include ancestors. These multiple states require
// an enum to track.
enum class BoundsWalkHitResult
{
    Stop            = 0x00,
    Continue        = 0x01,
    IncludeParents  = 0x02
};

template <>
struct is_flags_enum<BoundsWalkHitResult>
{
    static constexpr bool value = true;
};