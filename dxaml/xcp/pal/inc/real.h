// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

//  Abstract:
//   Contains macros for helping with floating point arithmetic.

#ifndef __REAL_H__
#define __REAL_H__

// The maximum integer value that can be converted to a float without risk
// of precision lost is 2^24 (since the significand has 23 bits.)
//
// A float is interpreted bit wise as:
//
//     Sign  |   Biased Exponent    |     Normalized Significant
// ----------+----------------------+--------------------------------
//    1 bit  |     8 bits           |        24 bits
//
// After more than 24 bits are required to represent the integer, the
// floating point cast starts using the exponent region and the low
// order bits are "lost".  The lower limit is also -2^24 since the
// sign is stored independent of the significant (2's complement is not
// used.)

union FI
{
    XFLOAT f;
    XINT32 i;
};

//+------------------------------------------------------------------------
//
//  Function:   XcpFloor
//
//  Synopsis:   Floor version of XcpRound that handles all values
//
//-------------------------------------------------------------------------
XINT32 
XcpFloor(_In_ XDOUBLE x);

//+------------------------------------------------------------------------
//
//  Function:   XcpFloor64
//
//  Synopsis:   Floor version of XcpRound that handles all values
//
//-------------------------------------------------------------------------
XINT64 
XcpFloor64(_In_ XDOUBLE x);

//+------------------------------------------------------------------------
//
//  Function:   XcpRound
//
//  Synopsis:   Faster cross-platform version of round than CRT
//
//-------------------------------------------------------------------------
XINT32 
XcpRound(_In_ XDOUBLE x);

//+------------------------------------------------------------------------
//
//  Function:   XcpRound64
//
//  Synopsis:   Faster cross-platform version of round than CRT
//
//-------------------------------------------------------------------------
XINT64 
XcpRound64(_In_ XDOUBLE x);

XINT32 
XcpCeiling(_In_ XDOUBLE x);

//+------------------------------------------------------------------------
//
//  Method:
//      XcpNextSmaller
//
//  Synopsis:
//      Compute max float less than given.
//
//  Note:
//      This routine works only for positive given numbers.
//      Negatives, zeros, infinity and NaN are asserted but not handled;
//
//-------------------------------------------------------------------------

XFLOAT
XcpNextSmaller(_In_ XFLOAT x);

#endif //#ifndef __REAL_H__


