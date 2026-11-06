// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "Spec/ShapeSpec.h"

bool SortSpec::Matches(SortSpec const& other) const noexcept
{
    if (SortKind != other.SortKind)
    {
        return false;
    }

    return SortKind == Kind::Path ?
        Path == other.Path :
        Delegate.Id == other.Delegate.Id;
}

ShapeDelta Diff(ShapeSpec const& before, ShapeSpec const& after) noexcept
{
    ShapeDelta delta = ShapeDelta::None;

    if (before.Filters != after.Filters)
    {
        delta |= ShapeDelta::Filter;
    }

    // One flag for any sort change: the engine re-reads every level's keys.
    if (before.Sorts != after.Sorts)
    {
        delta |= ShapeDelta::Sort;
    }

    // A level added or removed re-buckets; GroupKey already implies the group re-sort.
    if (before.Groups.size() != after.Groups.size())
    {
        delta |= ShapeDelta::GroupKey;
        return delta;
    }

    for (size_t level = 0; level < before.Groups.size(); ++level)
    {
        GroupSpec const& was = before.Groups[level];
        GroupSpec const& now = after.Groups[level];
        if (was.Key != now.Key)
        {
            delta |= ShapeDelta::GroupKey;
        }
        else if (was.Direction != now.Direction)
        {
            delta |= ShapeDelta::GroupOrder;
        }
    }

    return delta;
}
