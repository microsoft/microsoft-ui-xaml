// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace DirectUI
{
    class FloatUtil
    {
        public:
            // Represents a value that is not a number (NaN).
            static const float NaN;

            // Represents positive infinity.
            static const float PositiveInfinity;

            // Returns a value indicating whether the specified number evaluates
            // to a value that is not a number (NaN).
            static bool IsNaN(float value);

            // Returns a value indicating whether the specified number evaluates
            // to negative or positive infinity.
            static bool IsInfinity(float value);
    };
}

