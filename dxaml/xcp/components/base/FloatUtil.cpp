// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "FloatUtil.h"

using namespace DirectUI;

// Represents a value that is not a number (NaN).
const float FloatUtil::NaN = -std::numeric_limits<float>::quiet_NaN();

// Represents positive infinity.
const float FloatUtil::PositiveInfinity = std::numeric_limits<float>::infinity();

// Returns a value indicating whether the specified number evaluates
// to a value that is not a number (NaN).
bool FloatUtil::IsNaN(float value)
{
    return !!_isnan(value);
}

// Returns a value indicating whether the specified number evaluates
// to negative or positive infinity.
bool FloatUtil::IsInfinity(float value)
{
    return !_finite(value);
}
