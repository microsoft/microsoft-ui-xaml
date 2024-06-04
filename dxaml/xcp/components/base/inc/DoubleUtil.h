// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace DirectUI
{
    class DoubleUtil
    {
        public:
            // Represents the smallest positive double value that is greater
            // than zero.  This is used to determine whether two doubles are
            // effectively equal.  The value is tailored to Silverlight which
            // actually uses floats to represent doubles in the core.
            static const double Epsilon;
            
            // Represents positive infinity.
            static const double PositiveInfinity;
            
            // Represents negative infinity.
            static const double NegativeInfinity;
            
            // Represents a value that is not a number (NaN).
            static const double NaN;
            
            // Represents the maximum value.
            static const double MaxValue;

            // Represents the minimum value.
            static const double MinValue;
            
            // Returns a value indicating whether the specified number evaluates
            // to negative or positive infinity.
            static bool IsInfinity(
                _In_ double value);
            
            // Returns a value indicating whether the specified number evaluates
            // to positive infinity.
            static bool IsPositiveInfinity(
                _In_ double value);
            
            // Returns a value indicating whether the specified number evaluates
            // to negative infinity.
            static bool IsNegativeInfinity(
                _In_ double value);
            
            // Returns a value indicating whether the specified number evaluates
            // to a value that is not a number (NaN).
            static bool IsNaN(
                _In_ double value);
            
            // Return the absolute value of a number.
            static double Abs(
                _In_ double value);

            // Returns the largest integer greater than or equal to the specified number.
            // Ceil(1.5) == 2
            // Ceil(-1.5) == -1
            static double Ceil(
                _In_ double value);

            // Returns the largest integer less than or equal to the specified number.
            // Floor(1.5) == 1
            // Floor(-1.5) == -2
            static double Floor(
                _In_ double value);
        
            // Returns fractional part of double
            static double Fractional(
                _In_ double value);

            // Returns the larger of two specified numbers.
            static double Max(
                _In_ double value1,
                _In_ double value2);
            
            // Returns the smaller of two numbers.
            static double Min(
                _In_ double value1,
                _In_ double value2);
            
            // Returns whether or not two doubles are "close".  That is, whether
            // or not they are within Epsilon of each other.  Note that this
            // Epsilon is proportional to the numbers themselves so that
            // AreClose survives scalar multiplication.  There are plenty of
            // ways for this to return false even for numbers which are
            // theoretically identical, so no code calling this should fail to
            // work if this returns false.  This is important enough to repeat:
            // NB: NO CODE CALLING THIS FUNCTION SHOULD DEPEND ON ACCURATE
            // RESULTS - this should be used for optimizations *only*.
            static bool AreClose(
                _In_ double value1,
                _In_ double value2);
            
            // Returns whether or not the first double is greater than the
            // second double.  That is, whether or not the first is strictly
            // greater than *and* not within epsilon of the other number.  Note
            // that this epsilon is proportional to the numbers themselves to
            // that AreClose survives scalar multiplication.  Note, There are
            // plenty of ways for this to return false even for numbers which
            // are theoretically identical, so no code calling this should fail
            // to work if this returns false.  This is important enough to
            // repeat: NB: NO CODE CALLING THIS FUNCTION SHOULD DEPEND ON
            // ACCURATE RESULTS - this should be used for optimizations *only*.
            static bool GreaterThan(
                _In_ double value1,
                _In_ double value2);
            
            // Returns whether or not the first double is greater than or close
            // to the second double.  That is, whether or not the first is
            // strictly greater than or within epsilon of the other number.
            // Note that this epsilon is proportional to the numbers themselves
            // to that AreClose survives scalar multiplication.  Note, There are
            // plenty of ways for this to return false even for numbers which
            // are theoretically identical, so no code calling this should fail
            // to work if this returns false.  This is important enough to
            // repeat: NB: NO CODE CALLING THIS FUNCTION SHOULD DEPEND ON
            // ACCURATE RESULTS - this should be used for optimizations *only*.
            static bool GreaterThanOrClose(
                _In_ double value1,
                _In_ double value2);
            
            // Returns whether or not the double is "close" to 0.  Same as
            // AreClose(double, 0), but this is faster.
            static bool IsZero(
                _In_ double value);
            
            // Returns whether or not the first double is less than the second
            // double.  That is, whether or not the first is strictly less
            // than *and* not within epsilon of the other number.  Note that
            // this epsilon is proportional to the numbers themselves to that
            // AreClose survives scalar multiplication.  Note, There are plenty
            // of ways for this to return false even for numbers which are
            // theoretically identical, so no code calling this should fail to
            // work if this returns false.  This is important enough to repeat:
            // NB: NO CODE CALLING THIS FUNCTION SHOULD DEPEND ON ACCURATE
            // RESULTS - this should be used for optimizations *only*.
            static bool LessThan(
                _In_ double value1,
                _In_ double value2);
            
            // Returns whether or not the first double is less than or close to
            // the second double.  That is, whether or not the first is strictly
            // less than or within epsilon of the other number.  Note that this
            // epsilon is proportional to the numbers themselves to that
            // AreClose survives scalar multiplication.  Note, There are
            // plenty of ways for this to return false even for numbers which
            // are theoretically identical, so no code calling this should fail
            // to work if this returns false.  This is important enough to
            // repeat: NB: NO CODE CALLING THIS FUNCTION SHOULD DEPEND ON
            // ACCURATE RESULTS - this should be used for optimizations *only*.
            static bool LessThanOrClose(
                _In_ double value1,
                _In_ double value2);

            // Rounds the fractional part of a double to the given number of decimal places.
            // If the digit found one past numDecimalPlaces is 5 or greater, then the digit at
            // numDecimalPlaces will be rounded up.  Otherwise, the value will simply be truncated
            // with numDecimalPlaces fractional digits.
            // NOTE: Does not handle the case where numDecimalPlaces causes us to go below
            // the min value of double.
            static double Round(
                _In_ double originalValue,
                _In_ unsigned int numDecimalPlaces);
                
            // Returns TRUE if a and b are within the given tolerance of each other.
            static bool AreWithinTolerance(
                _In_ double a,
                _In_ double b,
                _In_ double tolerance);
    };
}

