// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "Spec/ShapeDelegates.h"

ShapeDelegates::ShapeDelegates(ITrackerHandleManager const* trackerOwner) noexcept
    : m_trackerOwner(trackerOwner)
{
}

ShapeDelegates::~ShapeDelegates() = default;

void ShapeDelegates::Clear() noexcept
{
    // Drops every tracked delegate while the owner is alive. Registration lands in Milestone 4.
    m_entries.clear();
    m_idByAddress.clear();
    m_trackerOwner = nullptr;
}
