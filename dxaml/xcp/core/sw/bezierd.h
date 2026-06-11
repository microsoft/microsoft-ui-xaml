// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

//  Description:  Double precision Bezier curve with basic services
//  Synopsis: Data and basic services for a Bezier curve

class CBezier
{
public:
    CBezier()
    {
    }

    CBezier(
        _In_reads_(4) XPOINTF *pPt)
            // The defining Bezier points
    {
        ASSERT(pPt);
        memcpy(&m_ptB, pPt, 4 * sizeof(XPOINTF)); 
    }

    CBezier(
        _In_ const CBezier &other)
            // Another Bezier to copy
    {
        Copy(other); 
    }

    void Copy(
        _In_ const CBezier &other)
            // Another Bezier to copy
    {
        memcpy(&m_ptB, (void *)other.m_ptB, 4 * sizeof(XPOINTF)); 
    }

    void Initialize(
        _In_ const XPOINTF &ptFirst,
            // The first Bezier point
        _In_reads_(3) const XPOINTF *pPt)
            // The remaining 3 Bezier points
    {
        m_ptB[0] = ptFirst;
        memcpy(m_ptB + 1, (void *)pPt, 3 * sizeof(XPOINTF)); 
    }

    const XPOINTF &GetControlPoint(XUINT32 i) const
    {
        ASSERT(i < 4);
        return m_ptB[i];
    }

    const XPOINTF &GetFirstPoint() const
    {
        return m_ptB[0];
    }
    
    const XPOINTF &GetLastPoint() const
    {
        return m_ptB[3];
    }

    void GetPoint(
        _In_ double t,
            // Parameter value
        _Out_ XPOINTF &pt) const; 
            // Point there

    void TrimToStartAt(
        _In_ double t);             // Parameter value
        
    void TrimToEndAt(
        _In_ double t);             // Parameter value

protected:
    // Data
    XPOINTF        m_ptB[4]{};
        // The defining Bezier points
};

// Constants used to convert quadratic Bezier curves and elliptical arcs in to
// cubic Bezier curves.

#define ONE_THIRD       0.333333333f
#define TWO_THIRDS      0.666666667f
#define FOUR_THIRDS     1.333333333f
#define TWO_PI          6.283185307f
#define PI_OVER_180     0.017453293f
#define FUZZ            1.0E-6
#define FUZZ_SQUARED    1.0E-12

// Convert an elliptical arc to cubic Bezier curves.

void ArcToBezier(
    _In_ XFLOAT xStart,                      // Previous point's X coordinate
    _In_ XFLOAT yStart,                      // Previous point's Y coordinate
    _In_ XFLOAT xRadius,                     // The ellipse's X radius
    _In_ XFLOAT yRadius,                     // The ellipse's Y radius
    _In_ XFLOAT eAngle,                      // The rotation angle
    _In_ XINT32 bLarge,                      // Choose the larger of the two arcs
    _In_ XINT32 bClockwise,                  // Sweep clockwise if true
    _In_ XFLOAT xEnd,                        // The final point's X coordinate
    _In_ XFLOAT yEnd,                        // The final point's Y coordinate
    _Out_writes_(12) XPOINTF *ppt,           // The output array of points
    _Deref_out_range_(-1,4) XINT32 *pcCurve  // The number of curves required
);
