// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "DoubleUtil.h"

using namespace DirectUI;

// Represents the smallest positive Double value that is greater than zero.
// This is used to determine whether two doubles are effectively equal.  The
// value is tailored to Silverlight which actually uses floats to represent
// doubles in the core.
const double DoubleUtil::Epsilon = 1.1102230246251567e-016;

// Represents positive infinity.
const double DoubleUtil::PositiveInfinity = std::numeric_limits<double>::infinity();

// Represents negative infinity.
const double DoubleUtil::NegativeInfinity = -std::numeric_limits<double>::infinity();

// Represents a value that is not a number (NaN).
const double DoubleUtil::NaN = -std::numeric_limits<double>::quiet_NaN();

// Represents the maximum value.
const double DoubleUtil::MaxValue = DBL_MAX;

// Represents the minimum value.
const double DoubleUtil::MinValue = DBL_MIN;


// Returns a value indicating whether the specified number evaluates
// to negative or positive infinity.
bool DoubleUtil::IsInfinity(
    _In_ double value)
{
    return !_finite(value);
}

// Returns a value indicating whether the specified number evaluates
// to positive infinity.
bool DoubleUtil::IsPositiveInfinity(
    _In_ double value)
{
    return value == PositiveInfinity;
}

// Returns a value indicating whether the specified number evaluates
// to negative infinity.
bool DoubleUtil::IsNegativeInfinity(
    _In_ double value)
{
    return value == NegativeInfinity;
}

// Returns a value indicating whether the specified number evaluates
// to a value that is not a number (NaN).
bool DoubleUtil::IsNaN(
    _In_ double value)
{
    return !!_isnan(value);
}

// Return the absolute value of a number.
double DoubleUtil::Abs(
    _In_ double value)
{
    return (value < 0) ?
        -value :
        value;
}

// Returns the largest integer greater than or equal to the specified number.
// Ceil(1.5) == 2
// Ceil(-1.5) == -1
double DoubleUtil::Ceil(
    _In_ double value)
{
    return ceil(value);
}

// Returns the largest integer less than or equal to the specified number.
// Floor(1.5) == 1
// Floor(-1.5) == -2
double DoubleUtil::Floor(
    _In_ double value)
{
    return floor(value);
}

// Returns fractional part of double
double DoubleUtil::Fractional(
    _In_ double value)
{
    return value > 0 ? (value - floor(value)) : (value - ceil(value));
}

// Returns the larger of two specified numbers.
double DoubleUtil::Max(
    _In_ double value1,
    _In_ double value2)
{
    return (value1 >= value2) ?
        value1 :
        value2;
}

// Returns the smaller of two numbers.
double DoubleUtil::Min(
    _In_ double value1,
    _In_ double value2)
{
    return (value1 <= value2) ?
        value1 :
        value2;
}

// Returns whether or not two doubles are "close".  That is, whether or not they
// are within epsilon of each other.  Note that this epsilon is proportional to
// the numbers themselves so that AreClose survives scalar multiplication.
// There are plenty of ways for this to return false even for numbers which are
// theoretically identical, so no code calling this should fail to work if this
// returns false.  This is important enough to repeat: NB:  NO CODE CALLING THIS
// FUNCTION SHOULD DEPEND ON ACCURATE RESULTS - this should be used for
// optimizations *only*.
bool DoubleUtil::AreClose(
    _In_ double value1,
    _In_ double value2)

{
    double epsilon = 0.0;
    double delta = 0.0;
    
    // If they're infinities, then the epsilon check doesn't work
    if (value1 == value2)
    {
        return true;
    }
    
    // This computes (|value1-value2| / (|value1| + |value2| + 10.0)) < Epsilon
    epsilon = (Abs(value1) + Abs(value2) + 10.0) * Epsilon;
    delta = value1 - value2;
    
    return (-epsilon < delta) && (epsilon > delta);
}

// Returns whether or not the first double is greater than the second double.
// That is, whether or not the first is strictly greater than *and* not within
// epsilon of the other number.  Note that this epsilon is proportional to the
// numbers themselves to that AreClose survives scalar multiplication.  Note,
// There are plenty of ways for this to return false even for numbers which are
// theoretically identical, so no code calling this should fail to work if this
// returns false.  This is important enough to repeat: NB: NO CODE CALLING THIS
// FUNCTION SHOULD DEPEND ON ACCURATE RESULTS - this should be used for
// optimizations *only*.
bool DoubleUtil::GreaterThan(
    _In_ double value1,
    _In_ double value2)
{
    return (value1 > value2) && !AreClose(value1, value2);
}

// Returns whether or not the first double is greater than or close to the
// second double.  That is, whether or not the first is strictly greater than
// or within epsilon of the other number.  Note that this epsilon is proportional
// to the numbers themselves to that AreClose survives scalar multiplication.
// Note, There are plenty of ways for this to return false even for numbers
// which are theoretically identical, so no code calling this should fail to
// work if this returns false.  This is important enough to repeat: NB: NO CODE
// CALLING THIS FUNCTION SHOULD DEPEND ON ACCURATE RESULTS - this should be used
// for optimizations *only*.
bool DoubleUtil::GreaterThanOrClose(
    _In_ double value1,
    _In_ double value2)
{
    return (value1 > value2) || AreClose(value1, value2);
}

// Returns whether or not the double is "close" to 0.  Same as AreClose(double,
// 0), but this is faster.
bool DoubleUtil::IsZero(
    _In_ double value)
{
    return Abs(value) < 10.0 * Epsilon;
}

// Returns whether or not the first double is less than the second double.  That
// is, whether or not the first is strictly less than *and* not within epsilon
// of the other number.  Note that this epsilon is proportional to the numbers
// themselves to that AreClose survives scalar multiplication.  Note, There are
// plenty of ways for this to return false even for numbers which are
// theoretically identical, so no code calling this should fail to work if this
// returns false.  This is important enough to repeat: NB: NO CODE CALLING THIS
// FUNCTION SHOULD DEPEND ON ACCURATE RESULTS - this should be used for
// optimizations *only*.
bool DoubleUtil::LessThan(
    _In_ double value1,
    _In_ double value2)
{
    return (value1 < value2) && !AreClose(value1, value2);
}

// Returns whether or not the first double is less than or close to the second
// double.  That is, whether or not the first is strictly less than or within
// epsilon of the other number.  Note that this epsilon is proportional to the
// numbers themselves to that AreClose survives scalar multiplication.  Note,
// There are plenty of ways for this to return false even for numbers which are
// theoretically identical, so no code calling this should fail to work if this
// returns false.  This is important enough to repeat: NB: NO CODE CALLING THIS
// FUNCTION SHOULD DEPEND ON ACCURATE RESULTS - this should be used for
// optimizations *only*.
bool DoubleUtil::LessThanOrClose(
    _In_ double value1,
    _In_ double value2)
{
    return (value1 < value2) || AreClose(value1, value2);
}

// Rounds the fractional part of a double to the given number of decimal places.
// If the digit found one past numDecimalPlaces is 5 or greater, then the digit at
// numDecimalPlaces will be rounded up.  Otherwise, the value will simply be truncated
// with numDecimalPlaces fractional digits.
// NOTE: Does not handle the case where numDecimalPlaces causes us to go below
// the min value of double.
double DoubleUtil::Round(
    _In_ double originalValue,
    _In_ unsigned int numDecimalPlaces)
{
    double scale = 1;

    for (unsigned int i = 0; i < numDecimalPlaces; i++)
        scale /= 10;

    return Floor(originalValue / scale + 0.5) * scale;
}

// Returns TRUE if a and b are within the given tolerance of each other.
bool DoubleUtil::AreWithinTolerance(
    _In_ double a,
    _In_ double b,
    _In_ double tolerance)
{
    return LessThanOrClose(Abs(a - b), tolerance);
}
