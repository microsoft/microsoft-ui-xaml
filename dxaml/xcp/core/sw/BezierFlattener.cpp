// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#pragma optimize("t", on)

const XDOUBLE TWICE_MIN_BEZIER_STEP_SIZE = 1.e-3; // The step size in the Bezier flattener should


//+-------------------------------------------------------------------------------------------------
//
//  Function: ApproxNorm
//
//  Synopsis: Returns the MAX x or y as the approximate normal
//
//--------------------------------------------------------------------------------------------------
XFLOAT
ApproxNorm(XPOINTF pt)
{
    return MAX(XcpAbsF(pt.x), XcpAbsF(pt.y));
}

/////////////////////////////////////////////////////////////////////////////////
//
//              Implementation of CBezierFlattener

//+-----------------------------------------------------------------------------
//
//  Member:   CBezierFlattener::Initialize
//
//  Synopsis: Initialize the sink and tolerance
//
//------------------------------------------------------------------------------
void
CBezierFlattener::Initialize(
    _In_opt_ CFlatteningSink *pSink,
        // The reciptient of the flattened data
    _In_ XFLOAT rTolerance)       // Flattening tolerance
{
    m_pSink = pSink;

    // If rTolerance == NaN or less than 0, we'll treat it as 0.
    m_rTolerance = rTolerance >= 0.0 ? rTolerance : 0.0;
    m_rFuzz = rTolerance * rTolerance * SQ_LENGTH_FUZZ;

    // The error is tested on MAX(|e2|, |e2|), which represent 6 times the actual error, so:
    m_rTolerance *= 6;
    m_rQuarterTolerance = m_rTolerance * .25;
}

//+-----------------------------------------------------------------------------
//
//  Member:   CBezierFlattener::Flatten
//
//  Synopsis: Flatten this curve
//
//  Notes:

// The algorithm is described in detail in the 1995 patent # 5367617 "System and
// method of hybrid forward differencing to render Bezier splines" to be found
// on the Microsoft legal dept. web site (LCAWEB).  Additional references are:
//     Lien, Shantz and Vaughan Pratt, "Adaptive Forward Differencing for
//     Rendering Curves and Surfaces", Computer Graphics, July 1987
//     Chang and Shantz, "Rendering Trimmed NURBS with Adaptive Forward
//         Differencing", Computer Graphics, August 1988
//     Foley and Van Dam, "Fundamentals of Interactive Computer Graphics"
//
// The basic idea is to replace the Bernstein basis (underlying Bezier curves)
// with the Hybrid Forward Differencing (HFD) basis which is more efficient at
// for flattening.  Each one of the 3 actions - Step, Halve and Double (step
// size) this basis affords very efficient formulas for computing coefficients
// for the new interval.
//
// The coefficients of the HFD basis are defined in terms of the Bezier
// coefficients as follows:
//
//          e0 = p0, e1 = p3 - p0, e2 = 6(p1 - 2p2 + p3), e3 = 6(p0 - 2p1 + p2),
//
// but formulas may be easier to understand by going through the power basis
// representation:  f(t) = a*t + b*t + c * t^2 + d * t^3.
//
//  The conversion is then:
//                               e0 = a
//                               e1 = f(1) - f(0) = b + c + d
//                               e2 = f"(1) = 2c + 6d
//                               e3 = f"(0) = 2c
//
// This is inverted to:
//                              a = e0
//                              c = e3 / 2
//                              d = (e2 - 2c) / 6 = (e2 - e3) / 6
//                              b = e1 - c - d = e1 - e2 / 6 - e3 / 3
//
// a, b, c, d for the new (halved, doubled or forwarded) interval are derived
// and then converted to e0, e1, e2, e3 using these relationships.
//
//------------------------------------------------------------------------------
_Check_return_ HRESULT
CBezierFlattener::Flatten(
    _In_ XINT32  fWithTangents)   // Return tangents with the points if TRUE
{
    XINT32 fAbort = FALSE;

    if (!m_pSink)
    {
        IFC_RETURN(E_UNEXPECTED);
    }

    m_fWithTangents = fWithTangents;

    m_cSteps = 1;

    m_rParameter = 0;
    m_rStepSize = 1;

    // Compute the HFD basis
    m_ptE[0] = m_ptB[0];
    m_ptE[1] = m_ptB[3] - m_ptB[0];
    m_ptE[2] = (m_ptB[1] - m_ptB[2] * 2.0 + m_ptB[3]) * 6.0;    // The second derivative at curve end
    m_ptE[3] = (m_ptB[0] - m_ptB[1] * 2.0 + m_ptB[2]) * 6.0;    // The second derivative at curve start

    // Determine the initial step size
    m_cSteps = 1;
    while (((ApproxNorm(m_ptE[2]) > m_rTolerance)  ||  (ApproxNorm(m_ptE[3]) > m_rTolerance)) &&
           (m_rStepSize > TWICE_MIN_BEZIER_STEP_SIZE))
    {
        HalveTheStep();
    }

    while (m_cSteps > 1)
    {
        IFC_RETURN(Step(fAbort));
        if (fAbort)
            return S_OK;

        // E[3] was already tested as E[2] in the previous step
        if (ApproxNorm(m_ptE[2]) > m_rTolerance &&
            m_rStepSize > TWICE_MIN_BEZIER_STEP_SIZE)
        {
            // Halving the step once is provably sufficient (see Notes above), so ---
            HalveTheStep();
        }
        else
        {
            // --- but the step can possibly be more than doubled, hence the while loop
            while (TryDoubleTheStep())
                continue;
        }
    }

    // Last point
    if (m_fWithTangents)
    {
        IFC_RETURN(m_pSink->AcceptPointAndTangent(m_ptB[3], GetLastTangent(), TRUE /* last point */));
    }
    else
    {
        IFC_RETURN(m_pSink->AcceptPoint(m_ptB[3], 1, fAbort));
    }

    return S_OK;
}
//+-----------------------------------------------------------------------------------------------
//
//  Member:   CBezierFlattener::Step
//
//  Synopsis: Step forward on the polygonal approximation of the curve
//
//  Notes: Taking a step means replacing a,b,c,d by coefficients of g(t) = f(t+1).
//         Express those in terms of a,b,c,d and convert to e0, e1, e2, e3 to get:
//
//          New e0 = e0 + e1
//          New e1 = e1 + e2
//          New e2 = 2e2 - e3
//          New e3 = e2
//
//  The patent application (see above) explains why.
//
//  Getting a tangent vector is a minor enhancement along the same lines:
//
//                               f'(0) = b = 6e1 - e2 - 2e3.
//
//------------------------------------------------------------------------------------------------
_Check_return_ HRESULT
CBezierFlattener::Step(
    _Out_ XINT32 &fAbort)   // Set to TRUE if flattening should be aborted, untouched otherwise
{
    // Compute the basis for the same curve on the next interval
    XPOINTF pt;

    m_ptE[0] += m_ptE[1];
    pt = m_ptE[2];
    m_ptE[1] += pt;
    m_ptE[2] += pt;  m_ptE[2] -= m_ptE[3];
    m_ptE[3] = pt;

    // Increment the parameter
    m_rParameter += m_rStepSize;

    // Generate the start point of the new interval
    if (m_fWithTangents)
    {
        // Compute the tangent there
        pt = m_ptE[1] * 6.0 - m_ptE[2] - m_ptE[3] * 2.0;  //  = twice the derivative at E[0]
        IFC_RETURN(m_pSink->AcceptPointAndTangent(m_ptE[0], pt, FALSE /* not the last point */));
    }
    else
    {
        IFC_RETURN(m_pSink->AcceptPoint(m_ptE[0], (XFLOAT)m_rParameter, fAbort));
    }

    m_cSteps--;
    return S_OK;
}
//+-----------------------------------------------------------------------------
//
//  Member:   CBezierFlattener::HalveTheStep
//
//  Synopsis: Halve the size of the step
//
//  Notes: Halving the step means replacing a,b,c,d by coefficients of
//         g(t) = f(t/2). Express those in terms of a,b,c,d and convert to
//         e0, e1, e2, e3 to get:
//
//          New e0 = e0
//          New e1 = (e1 - e2) / 2
//          New e2 = (e2 + e3) / 8
//          New e3 = e3 / 4
//
//  The patent application (see above) explains why.
//
//------------------------------------------------------------------------------
void
CBezierFlattener::HalveTheStep()
{
    m_ptE[2] += m_ptE[3];   m_ptE[2] *= .125;
    m_ptE[1] -= m_ptE[2];    m_ptE[1] *= .5;
    m_ptE[3] *= .25;

    m_cSteps *= 2;  // Double the number of steps left
    m_rStepSize *= .5;
}
//+-----------------------------------------------------------------------------
//
//  Member:   CBezierFlattener::TryDoubleTheStep
//
//  Synopsis: Double the step size if possible within tolerance.
//
//  Notes: Coubling the step means replacing a,b,c,d by coefficients of
//         g(t) = f(2t). Express those in terms of a,b,c,d and convert to
//         e0, e1, e2, e3 to get:
//
//          New e0 = e0
//          New e1 = 2e1 + e2
//          New e2 = 8e2 - 4e3
//          New e3 = 4e3
//
//  The patent application (see above) explains why.  Note also that these
//  formulas are the inverse of those for halving the step.
//------------------------------------------------------------------------------
XINT32
CBezierFlattener::TryDoubleTheStep()
{
    XINT32 fDoubled = (0 == (m_cSteps & 1));
    if (fDoubled)
    {
        XPOINTF ptTemp = m_ptE[2] * 2.0 - m_ptE[3];

        fDoubled = (ApproxNorm(m_ptE[3]) <= m_rQuarterTolerance) &&
                   (ApproxNorm(ptTemp) <= m_rQuarterTolerance);

        if (fDoubled)
        {
            m_ptE[1] *= 2.0;  m_ptE[1] += m_ptE[2];
            m_ptE[3] *= 4.0;
            m_ptE[2] = ptTemp * 4.0;

            m_cSteps /= 2;      // Halve the number of steps left
            m_rStepSize *= 2;
        }
    }

    return fDoubled;
}
//+-------------------------------------------------------------------------------------------------
//
//  Member: CBezierFlattener::GetFirstTangent
//
//  Synopsis: Get the tangent at curve start
//
//  Return: MILERR_ZEROVECTOR if the tangent vector has practically 0 length
//
//  Notes: This method can return an error if all the points are bunched together. The idea is that
//         the caller will detect that, abandon this curve, and never call GetLasttangent, which
//         can therefore be presumed to succeed.  The failure here is benign.
//
//--------------------------------------------------------------------------------------------------
_Check_return_ HRESULT
CBezierFlattener::GetFirstTangent(
    _Out_ XPOINTF &vecTangent) const // Tangent vector there

{
    vecTangent = m_ptB[1] - m_ptB[0];
    if (vecTangent * vecTangent > m_rFuzz)
    {
        return S_OK;  // - we're done
    }
    // Zero first derivative, go for the second
    vecTangent = m_ptB[2] - m_ptB[0];
    if (vecTangent * vecTangent > m_rFuzz)
    {
        return S_OK;  // - we're done
    }
    // Zero second derivative, go for the third
    vecTangent = m_ptB[3] - m_ptB[0];

    if (vecTangent * vecTangent <= m_rFuzz)
    {
        return E_FAIL;    // no IFC, this is a benign failure
    }

    return S_OK;      // no RRETURN, this is a benign failure
}
//+-------------------------------------------------------------------------------------------------
//
//  Member: CBezierFlattener::GetLastTangent
//
//  Synopsis: Get the tangent at curve end
//
//  Return: The tangent
//
//  Notes: This method has no error return while GetFirstTangent returns MILERR_ZEROVECTOR if the
//         tangent is zero.  The idea is that we should only fail if all the control points
//         coincide, that should have been detected at GetFirstTangent, and then we should have not
//         be called.
//
//--------------------------------------------------------------------------------------------------
XPOINTF
CBezierFlattener::GetLastTangent() const
{
    XPOINTF vecTangent = m_ptB[3] - m_ptB[2];

    // If the curve is degenerate, we should have detected it at curve-start, skipped this curve
    // altogether and not be here.  But the test in GetFirstTangent is for the point-differences
    // 1-0, 2-0 and 3-0, while here it is for points 3-2, 3-1 and 3-0, which is not quite the same.
    // Still, In a disk of radius r no 2 points are more than 2r apart.  The tests are done with
    // squared distance, and m_rFuzz is the minimal accepted squared distance.  GetFirstTangent()
    // succeeded, so there is a pair of points whose squared distance is greater than m_rfuzz.
    // So the squared radius of a disk about point 3 that contains the remaining points must be
    // at least m_rFuzz/4.  Allowing some margin for arithmetic error:

    double rLastTangentFuzz = m_rFuzz/8;

    if (vecTangent * vecTangent <= rLastTangentFuzz)
    {
        // Zero first derivative, go for the second
        vecTangent = m_ptB[3] - m_ptB[1];
        if (vecTangent * vecTangent <= rLastTangentFuzz)
        {
            // Zero second derivative, go for the third
            vecTangent = m_ptB[3] - m_ptB[0];
        }
    }

    ASSERT(!(vecTangent * vecTangent < rLastTangentFuzz)); // Ignore NaNs

    return vecTangent;
}
