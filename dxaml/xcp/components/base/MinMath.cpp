// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "MinMath.h"

float ClampAngleTo0To360(float degrees)
{
    // Limit the range from 0° to 360° to reduce error for sinf/cosf/tanf
    if (degrees >= 360.0f)
    {
        int nWhole = int(floorf(degrees / 360.0f));
        degrees -= (360.0f * nWhole);
    }
    else if (degrees < 0.0f)
    {
        int nWhole = int(floorf(degrees / -360.0f));
        degrees += (360.0f * (++nWhole));
    }

    return degrees;
}

// Implementation copied from winpal. This can be switched to std::cos instead, but the behavior could be different.
// Punting the switch for now until we get better test coverage.
float MathCosDegrees(float degrees)
{
    degrees = ClampAngleTo0To360(degrees);

    // These angles become imprecise after multiplying by the imprecise radian conversion constant. Special case them.
    if (degrees == 0.0f || degrees == 360.0f)
    {
        return 1;
    }
    else if (degrees == 90.0f || degrees == 270.0f)
    {
        return 0;
    }
    else if (degrees == 180.0f)
    {
        return -1;
    }
    else
    {
        return cosf(degrees * DEGREES_TO_RADIANS);
    }
}

// Implementation copied from winpal. This can be switched to std::sin instead, but the behavior could be different.
// Punting the switch for now until we get better test coverage.
float MathSinDegrees(float degrees)
{
    degrees = ClampAngleTo0To360(degrees);

    // These angles become imprecise after multiplying by the imprecise radian conversion constant. Special case them.
    if (degrees == 0.0f || degrees == 180.0f || degrees == 360.0f)
    {
        return 0;
    }
    else if (degrees == 90.0f)
    {
        return 1;
    }
    else if (degrees == 270.0f)
    {
        return -1;
    }
    else
    {
        return sinf(degrees * DEGREES_TO_RADIANS);
    }
}

// Implementation copied from winpal. This can be switched to std::tan instead, but the behavior could be different.
// Punting the switch for now until we get better test coverage.
float MathTanDegrees(float degrees)
{
    degrees = ClampAngleTo0To360(degrees);

    return tanf(degrees * DEGREES_TO_RADIANS);
}

