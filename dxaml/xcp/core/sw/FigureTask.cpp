// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#pragma optimize("t", on)

const XFLOAT DEFAULT_FLATTENING_TOLERANCE = .25;

/////////////////////////////////////////////////////////////////////////////////
//
//              Implementation of CMILBezierFlattener
//+-----------------------------------------------------------------------------
//
//  Member:   CMILBezierFlattener::CMILBezierFlattener
//
//  Synopsis: Constructor from individual points, no trimming
//
//------------------------------------------------------------------------------
CMILBezierFlattener::CMILBezierFlattener(
    _In_ const XPOINTF &ptFirst,
        // First point (transformed)
    _In_ const XPOINTF &ptControl1,
        // First control point
    _In_ const XPOINTF &ptControl2,
        // Second control point
    _In_ const XPOINTF &ptEnd,
        // Last point
    _In_reads_opt_(1) CFlatteningSink *pSink,
        // Flattening sink
    _In_ const CMILMatrix &matrix)
        // Transformation matrix
    : CBezierFlattener(pSink, DEFAULT_FLATTENING_TOLERANCE)
{
     m_ptB[0] = ptFirst;
     matrix.Transform(&ptControl1, &m_ptB[1], 1);
     matrix.Transform(&ptControl2, &m_ptB[2], 1);
     matrix.Transform(&ptEnd, &m_ptB[3], 1);
}
//+-----------------------------------------------------------------------------
//
//  Member:   CMILBezierFlattener::SetPoints
//
//  Synopsis: Set the coefficients for a possibly transformed and possibly trimmed curve
//
//  Notes:   This method is geared towards traversing a path with a transformation.
//           The first point is equal to the last point of the previous segment, which
//           has already been transformed; that is why it is entered separately.
//
//           The curve defined by the input points is a parametric mapping from the 
//           interval [0,1]. The input arguments rStart and rEnd allow the caller to 
//           specify setting the coefficients to represent a trimmed portion of the 
//           of the original curve.
//
//------------------------------------------------------------------------------
void
CMILBezierFlattener::SetPoints(
    double rStart,
        // Start parameter
    double rEnd,
        // End parameter
    _In_ const XPOINTF  &ptFirst,
        // First point (transformed)
    _In_reads_(3) const XPOINTF *pt,
        // The last 3 points (raw)
    _In_reads_opt_(1) const CMILMatrix  *pMatrix)
        // Transformation matrix (NULL OK)
{
    // The caller should not be asking for trimming outside [0,1].
    // Ignore NaNs
    ASSERT(!(0 > rStart)); 
    ASSERT(!(rStart > rEnd));
    ASSERT(!(rEnd > 1));

    m_ptB[0] = ptFirst;
    if (pMatrix)
    {
        pMatrix->Transform(pt, m_ptB + 1, 3);
    }
    else
    {
        for (int i = 1;  i <= 3;  i++)
        {
            m_ptB[i] = pt[i - 1];
        }
    }

    // Trimming = computing Bezier points for a curve that represents a portion
    // of the curve defined by the input points.
    if (rEnd <= rStart + FUZZ)
    {
        // The trimmed curve degenerates to a point
        XPOINTF ptLocal;
        GetPoint(rStart, ptLocal);
        m_ptB[0] = m_ptB[1] = m_ptB[2] = m_ptB[3] = ptLocal;
    }
    else
    {
        if (rStart > 0)
        {
            TrimToStartAt(rStart);
        }
        if (rEnd < 1)
        {
        // If rStart > 0 then the curve has been trimmed, but the Bezier points represent a
        // curve with parameter domain [0,1], oblivious to that trimming.  So we need to
        // adjust the second trimming parameter to reflect the first trimming. For example, 
        // supposed rStart = 0.2 and rEnd = 0.6. After trimming 0.2 from the start, we want the
        // second trim to leave us with [0.2, 0.6].  The size of this domain is 0.4, which is
        // 0.5 of 0.8 - the size remaining after the first trim.  We get that with 
        // (0.6-0.2)/(1-0.2). In general, the new trim parameter is 
        // (rEnd - rStart) / (1 - rStart).

            if (rStart > 0)
            {
                // Ignore NaNs
                ASSERT(!(FUZZ >= 1 - rStart));  // Since rStart + FUZZ < rEnd <= 1
                rEnd = (rEnd - rStart) / (1 - rStart);
            }
            TrimToEndAt(rEnd);
        }
    }
}

