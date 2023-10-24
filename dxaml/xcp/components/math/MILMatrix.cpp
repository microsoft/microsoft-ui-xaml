// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "Matrix.h"

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Transform the specified array of points using the current matrix.
//
//  Implementation details:
//      This function assumes two things
//          1) There is always atleast 1 point
//          2) The source array DOES NOT overlap the destination array unless
//              the target address is less then or equal to the source address.
//      However, since this component can't access the PAL or the OS we can't
//      assert this behave so we have to programatically guarantee it.
//
//      The function performs the following computation
//
//      p' = p M:
//                                  ( M11 M12 0 )
//      (px', py', 1) = (px, py, 1) ( M21 M22 0 )
//                                  ( dx  dy  1 )
//
//-------------------------------------------------------------------------

void
CMILMatrix::Transform(
    _In_reads_(count) const XPOINTF *srcPoints,    // Array of source points to transform
    _Out_writes_(count) XPOINTF *destPoints,        // Destination for transformed points.
    _In_ XUINT32 count                              // Number of points to transform
    ) const
{
// Validate the parameters.

    if (!count || ((srcPoints < destPoints) && (srcPoints + count > destPoints)))
        return;

    do
    {
        Transform(*srcPoints, *destPoints);
    }
    while (destPoints++, srcPoints++, --count != 0);
}

void CMILMatrix::Transform(const gsl::span<XPOINTF>& points) const
{
    for (XPOINTF& point : points)
    {
        Transform(point, point);
    }
}

void CMILMatrix::Transform(const XPOINTF& source, XPOINTF& destination) const
{
    // Use locals to allow in-place transforms of the array.
    float x = source.x;
    float y = source.y;

    destination.x = (_11 * x) + (_21 * y) + _31;
    destination.y = (_12 * x) + (_22 * y) + _32;
}

//+------------------------------------------------------------------------
//
//  Member:
//      CMILMatrix::Invert
//
//  Synopsis:
//      Inverts the matrix
//
//-------------------------------------------------------------------------
bool CMILMatrix::Invert()
{
    float rDeterminant = GetDeterminant();
    CMILMatrix matCopy = *this;

    if (rDeterminant == 0.0f)
    {
        return false; // not invertible
    }

    if (_12 == 0 && _21 == 0)
    {
        // This is a scale/translate matrix
        _11 = 1 / _11;
        _22 = 1 / _22;
        _31 = -_31 * _11;
        _32 = -_32 * _22;
        return true; // invertible
    }

    float rDetInv = 1.f/matCopy.GetDeterminant();

    if (!IsFiniteF(rDetInv))
    {
        return false; // not invertible
    }

    _11 =  matCopy._22 * rDetInv;
    _12 = -matCopy._12 * rDetInv;
    _21 = -matCopy._21 * rDetInv;
    _22 =  matCopy._11 * rDetInv;
    _31 = (matCopy._21 * matCopy._32 - matCopy._31 * matCopy._22) * rDetInv;
    _32 = (matCopy._31 * matCopy._12 - matCopy._11 * matCopy._32) * rDetInv;

    return true; // invertible
}

