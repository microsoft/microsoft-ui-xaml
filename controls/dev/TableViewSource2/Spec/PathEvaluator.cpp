// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "Spec/PathEvaluator.h"
#include "SortMemberPathResolver.h"

PathEvaluator::PathEvaluator() noexcept
{
}

PathEvaluator::~PathEvaluator() = default;

void PathEvaluator::Clear() noexcept
{
    // Evaluation lands in Milestone 4.
    m_resolvers.clear();
}
