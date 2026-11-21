// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "Source/Hierarchy/SourceHierarchy.h"

#include <algorithm>

void AppendSourceSubtree(SourceRecord* record, std::vector<SourceRecord*>& out)
{
    std::vector<SourceRecord*> pending{ record };
    while (!pending.empty())
    {
        SourceRecord* const next = pending.back();
        pending.pop_back();
        out.push_back(next);
        pending.insert(pending.end(), next->Children.rbegin(), next->Children.rend());
    }
}

uint32_t IndexInSourceSiblings(std::vector<SourceRecord*> const& siblings, SourceRecord const* record) noexcept
{
    return static_cast<uint32_t>(std::find(siblings.begin(), siblings.end(), record) - siblings.begin());
}
