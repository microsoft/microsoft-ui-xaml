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

    return SortKind == Kind::Path ? Path == other.Path : Delegate.Id == other.Delegate.Id;
}

ShapeDelta Diff(ShapeSpec const& before, ShapeSpec const& after) noexcept
{
    ShapeDelta delta = ShapeDelta::None;

    if (!(before.Filters == after.Filters))
    {
        delta |= ShapeDelta::Filter;
    }

    if (!(before.Sorts == after.Sorts))
    {
        delta |= ShapeDelta::Sort;
    }

    if (before.Groups.size() != after.Groups.size())
    {
        delta |= ShapeDelta::GroupKey;
    }
    else
    {
        for (size_t i = 0; i < before.Groups.size(); ++i)
        {
            if (!(before.Groups[i].Key == after.Groups[i].Key))
            {
                delta |= ShapeDelta::GroupKey;
            }
            else if (before.Groups[i].Direction != after.Groups[i].Direction)
            {
                delta |= ShapeDelta::GroupOrder;
            }
        }
    }

    return delta;
}
