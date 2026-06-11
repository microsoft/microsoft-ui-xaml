// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

//+----------------------------------------------------------------------------
//
//  Function: GetBezierDistance
//
//  Synopsis:
//      Get the distance from a circular arc's endpoints to the control points
//  of the Bezier arc that approximates it, as a fraction of the arc's radius.
//
//  Implementation details:
//      Since the result is relative to the arc's radius it depends strictly on
//  the arc's angle.  The arc is assumed to be of 90° of less, so therefore the
//  angle is determined by the cosine of that angle. The cosine is derived from
//  the dot product of two radius vectors. We need the Bezier curve that agrees
//  with the arc's points and tangents at the ends and midpoint. Now we compute
//  the distance from the curve's endpoints to its control points.
// 
//      Since we are looking for the relative distance, we can work on the unit
//  circle. Place the center of the circle at the origin, and put the X axis as
//  the bisector between the 2 vectors. Let a be the angle between the vectors. 
//  Then the X coordinates of the 1st & last points are cos(a/2).  Let x be the
//  X coordinate of the 2nd & 3rd points.  At t=1/2 we have a point at (1,0).
//  But the terms of the polynomial there are all equal:
//
//          (1-t)^3 = t*(1-t)^2 = t^2*(1-t) = t^3 = 1/8,
//           
//  so from the Bezier formula there we have: 
//
//          1 = (1/8) * (cos(a/2) + 3x + 3x + cos(a/2)), 
//  hence
//          x = (4 - cos(a/2)) / 3
// 
//  The X difference between that and the 1st point is:
//
//          DX = x - cos(a/2) = 4(1 - cos(a/2)) / 3.
//
//  But DX = distance / sin(a/2), hence the distance is
//
//          dist = (4/3)*(1 - cos(a/2)) / sin(a/2).
//
//*****************************************************************************/

XFLOAT
GetBezierDistance(
    _In_ XFLOAT eDot,
    _In_ XFLOAT eRadius
)
{
    XFLOAT eSquared = eRadius * eRadius;  // Squared radius

// Ignore NaNs
//    ASSERT(!(eDot < -eSquared * 0.1f));  // angle < 90 degrees
//    ASSERT(!(eDot >  eSquared * 1.1f));  // as dot product of 2 radius vectors

    XFLOAT eDist = 0;   // Acceptable fallback value
    
//  Rather than the angle a, we are given eDot = R^2 * cos(a), so we 
//  multiply top and bottom by R:
//
//                   dist = (4/3)*(R - R cos(a/2)) / R sin(a/2)
//
//  and use some trig:
//                         __________
//            cos(a/2) = \/1 + cos(a) / 2
//                           ________________         __________
//            R*cos(a/2) = \/R^2 + R^2 cos(a) / 2 = \/R^2 + eDot / 2 */
                
    XFLOAT eCos = (eSquared + eDot) / 2.0f; //  =(R*cos(a))^2
    XFLOAT eSin = 0.0f;

// This shouldn't happen but we'll exit gracefully if it does.

    if (eCos < 0.0f)
        goto Cleanup;

//                 __________________
//  R*sin(a/2) = \/R^2 - R^2 cos(a/2)

    eSin = eSquared - eCos;          //  =(R*sin(a))^2

// For an angle of 0 we shouldn't be rounding the corner but returning 0 is OK.

    if (eSin <= 0.0f)
        goto Cleanup;

    eSin = sqrtf(eSin);            //  = R*cos(a)
    eCos = sqrtf(eCos);            //  = R*sin(a)

    eDist = FOUR_THIRDS * (eRadius - eCos);

    if (eDist <= eSin * FUZZ)
    {
        eDist = 0;
    }
    else
    {
        eDist = FOUR_THIRDS * (eRadius - eCos) / eSin;
    }

Cleanup:
    return eDist;
}

//+----------------------------------------------------------------------------
//
//  Function: GetArcAngle
//
//  Synopsis:
//      Get the number of Bezier curves and their sine and cosine as well.
//
//  Implementation notes:
//      This method is only intended to be called from ArcToBezier.  It forces
//  the arc into pieces that are not exceed 90°.  The input points are assumed
//  to be on the unit circle.
//
//-----------------------------------------------------------------------------

void
GetArcAngle(
    _In_ const XPOINTF &ptStart,
    _In_ const XPOINTF &ptEnd,
    _In_ const XINT32 bLarge,
    _In_ const XINT32 bClockwise,
    _Out_ XFLOAT *peCosAngle,
    _Out_ XFLOAT *peSinAngle,
    _Deref_out_range_(1,4) XINT32 *pcCurve
)
{
    XFLOAT  eAngle;

// The points are on the unit circle, so:

   *peCosAngle = ptStart.x * ptEnd.x + ptStart.y * ptEnd.y;
   *peSinAngle = ptStart.x * ptEnd.y - ptStart.y * ptEnd.x;

    if (*peCosAngle >= 0)
    {
        if (bLarge)
        {
        // The angle is between 270° and 360° so there are 4 curves required.

           *pcCurve = 4;
        }
        else
        {
        // The angle is between 0° and 90° so there is one curve

           *pcCurve = 1;
            return;
        }
    }
    else
    {
        if (bLarge)
        {
        // The angle is between 180° and 270° so there are 3 curves required.

           *pcCurve = 3;
        }
        else
        {
        // The angle is between 90° and 180° so there are 2 curves required.

           *pcCurve = 2;
        }
    }

// We have to chop the arc into the computed number of pieces.  For a count of
// 2 or 4 curves we could have used the half-angle trig formulae. But for the
// 3 curve solution it requires solving a cubic equation. So we'll just get the
// angle, divide it, and return the resulting sine and cosine.

    eAngle = atan2f(*peSinAngle, *peCosAngle);

    if (bClockwise)
    {
        if (eAngle < 0)
        {
            eAngle += TWO_PI;
        }
    }
    else
    {
        if (eAngle > 0)
        {
            eAngle -= TWO_PI;
        }
    }

    eAngle /= *pcCurve;
   *peCosAngle = cosf(eAngle);
   *peSinAngle = sinf(eAngle);
}

//-----------------------------------------------------------------------------
//
//  Function: AcceptRadius
//
//  Synopsis: 
//      Verifies the radius is not unacceptably small
//
//  Implementation notes:
//      This method is only intended to be called from ArcToBezier.
//
//-----------------------------------------------------------------------------

XINT32
AcceptRadius(
    _In_ XFLOAT eHalfChordSquared,    // (1/2 chord length) squared
    _In_ XFLOAT eRadius
)
{
    return !(eRadius * eRadius <= eHalfChordSquared * FUZZ_SQUARED);
}

//+----------------------------------------------------------------------------
//
//  Function: ArcToBezier
//
//  Synopsis:
//      Compute the cubic Bezier approximation of an elliptical arc.
//
//  Implementation details:
//      This function was copied from the MIL code.  It computes a series of
//  one to four cubic Bezier curves that match the SVG arc specifications.  The
//  ellipse is defined by a previous end point, the arc's size, radii, whether
//  to draw the large or small arc, clockwise or counter-clockwise, and the end
//  point of the arc.
//
//-----------------------------------------------------------------------------

void
ArcToBezier(
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
)
{
    XFLOAT x, y, eHalfChordSquared;
    XFLOAT eCos, eSin;
    XFLOAT eCosAngle, eSinAngle;
    XFLOAT xCenter, yCenter;
    XFLOAT eTemp, eBezierDistance;
    XPOINTF ptStart, ptEnd, ptTemp;
    XPOINTF vecBezierOne, vecBezierTwo;
    CMILMatrix matEllipse;
    XINT32 bZeroCenter = FALSE;
    XUINT32 i, j;

// Assume this is a degenerate ellipse

   *pcCurve = -1;

// Compute the midpoint of the chord between the start point to the end point.
// This is the first transform.

    x = (xEnd - xStart) / 2.0f;
    y = (yEnd - yStart) / 2.0f;

    eHalfChordSquared = x * x + y * y;     // (half chord length)^2

// Test for degenerate ellipse

    if (eHalfChordSquared < FUZZ_SQUARED)
        return;

// Test for straight line

    if (!AcceptRadius(eHalfChordSquared, xRadius) || !AcceptRadius(eHalfChordSquared, yRadius))
    {
       *pcCurve = 0;
        return;
    }

// The second transform rotates us to the ellipse's coordinate system.

    eAngle = -eAngle * PI_OVER_180;
    eCos = cosf(eAngle);
    eSin = sinf(eAngle);

    eTemp = x * eCos - y * eSin; 
    y = x * eSin + y * eCos;
    x = eTemp;

// The third transform converts the ellipse to a unit circle.

    x /= xRadius;
    y /= yRadius;

// We find the center of that circle along a vector perpendicular to the chord
// from the origin, which is the chord's midpoint. According to the Pythagorean
// theory the length of that vector is sqrt(1 - (half chord)^2).  We'll need to
// recompute the half chord distance in the new coordinate space.

    eHalfChordSquared = x * x + y * y;

    if (eHalfChordSquared > 1)
    {
    // The chord is longer than the diameter of the circle so we scale both of
    // the radii uniformly so that the chord will be a diameter.  Then the
    // center will be the chord's midpoint and its origin.

        eTemp = sqrtf(eHalfChordSquared);
        xRadius *= eTemp;
        yRadius *= eTemp;
        xCenter = 0.0f;
        yCenter = 0.0;
        bZeroCenter = true;

    // Adjust the unit-circle coordinates x and y

        x /= eTemp;
        y /= eTemp;
    }
    else
    {
    // The length of (-y,x) or (x,-y) is sqrt(eHalfChordSquared), and we want a
    // vector of length sqrt(1 - eHalfChordSquared), so we'll multiply it by:

        eTemp = sqrtf((1.0f - eHalfChordSquared) / eHalfChordSquared);

        if (bLarge != bClockwise)
        {
        // Going in the direction of (-y, x)

            xCenter = -eTemp * y;
            yCenter =  eTemp * x;
        }
        else
        {
        // Going in the direction of (y, -x)

            xCenter =  eTemp * y;
            yCenter = -eTemp * x;
        }
    }

// The final transformation has us shift the origin to the center of the circle
// which then becomes the unit circle. Since the chord's midpoint is the origin
// the start point is (-x,-y) and the end point is (x, y).

    ptStart.x = -x - xCenter;
    ptStart.y = -y - yCenter;
    ptEnd.x = x - xCenter;
    ptEnd.y = y - yCenter;

// Set up the transform that will take us back to our coordinate system.  This
// transform is the inverse of the combinations of the four transforms we just
// computed.

    matEllipse.SetM11( eCos * xRadius);
    matEllipse.SetM12(-eSin * xRadius);
    matEllipse.SetM21( eSin * yRadius);          
    matEllipse.SetM22( eCos * yRadius);

    if (bZeroCenter)
    {
    // For a zeroed center value this math is easier.

        matEllipse.SetDx((xEnd + xStart) / 2.0f);
        matEllipse.SetDy((yEnd + yStart) / 2.0f);
    }
    else
    {
    // Darn, do the harder transform.

        matEllipse.SetDx((xEnd + xStart) / 2.0f + eCos * xRadius * xCenter + eSin * yRadius * yCenter);
        matEllipse.SetDy((yEnd + yStart) / 2.0f - eSin * xRadius * xCenter + eCos * yRadius * yCenter);
    }

// Get the sine and cosine of the angle that will generate the arc parts

    GetArcAngle(ptStart, ptEnd, bLarge, bClockwise, &eCosAngle, &eSinAngle, pcCurve);

// Get the vector to the first Bezier control point

    eBezierDistance = GetBezierDistance(eCosAngle, 1.0f);

    if (!bClockwise)
    {
        eBezierDistance = -eBezierDistance;
    }

    vecBezierOne.x = -eBezierDistance * ptStart.y;
    vecBezierOne.y =  eBezierDistance * ptStart.x;

// Add the arc pieces, except for the last

    j = 0;

    for (i = 1;  i < XUINT32(*pcCurve);  i++)
    {
    // Get the arc piece's endpoint

        XPOINTF ptPieceEnd;

        ptPieceEnd.x = ptStart.x * eCosAngle - ptStart.y * eSinAngle;
        ptPieceEnd.y = ptStart.x * eSinAngle + ptStart.y * eCosAngle;
        vecBezierTwo.x = -eBezierDistance * ptPieceEnd.y;
        vecBezierTwo.y =  eBezierDistance * ptPieceEnd.x;

        ptTemp.x = ptStart.x + vecBezierOne.x;
        ptTemp.y = ptStart.y + vecBezierOne.y;
        matEllipse.Transform(&ptTemp, &ppt[j++], 1);

        ptTemp.x = ptPieceEnd.x - vecBezierTwo.x;
        ptTemp.y = ptPieceEnd.y - vecBezierTwo.y;
        matEllipse.Transform(&ptTemp, &ppt[j++], 1);

        matEllipse.Transform(&ptPieceEnd, &ppt[j++], 1);

    // Move on to the next arc

        ptStart = ptPieceEnd;
        vecBezierOne = vecBezierTwo;
    }
    
// For the last arc we know the endpoint

    vecBezierTwo.x = -eBezierDistance * ptEnd.y;
    vecBezierTwo.y =  eBezierDistance * ptEnd.x;

    ptTemp.x = ptStart.x + vecBezierOne.x;
    ptTemp.y = ptStart.y + vecBezierOne.y;
    matEllipse.Transform(&ptTemp, &ppt[j++], 1);

    ptTemp.x = ptEnd.x - vecBezierTwo.x;
    ptTemp.y = ptEnd.y - vecBezierTwo.y;
    matEllipse.Transform(&ptTemp, &ppt[j++], 1);

    ppt[j].x = xEnd;
    ppt[j].y = yEnd;
}

//-----------------------------------------------------------------------------
//
//  Function:  GenerateBezierFromQuadratic
//
//  Synopsis:
//      Converts a quadratic curve into a cubic Bezier
//
//-----------------------------------------------------------------------------
void
GenerateBezierFromQuadratic(
    _In_reads_(3) const XPOINTF *pptQuadratic,
    _Out_writes_(4) XPOINTF *pptCubic
    )
{
// Copy the start point to the output buffer

    pptCubic[0] = pptQuadratic[0];

// Compute the next point as 1/3 pt[0] + 2/3 pt[1]

// the extra XFLOAT() casting reduce the float precision difference between OS/platforms

    pptCubic[1].x = XFLOAT((XFLOAT(pptQuadratic[0].x * XFLOAT(ONE_THIRD)) + XFLOAT(pptQuadratic[1].x * XFLOAT(TWO_THIRDS))));
    pptCubic[1].y = XFLOAT((XFLOAT(pptQuadratic[0].y * XFLOAT(ONE_THIRD)) + XFLOAT(pptQuadratic[1].y * XFLOAT(TWO_THIRDS))));

// Compute the next point as 2/3 pt[1] + 1/3 pt[2]

    pptCubic[2].x = XFLOAT((XFLOAT(pptQuadratic[1].x * XFLOAT(TWO_THIRDS)) + XFLOAT(pptQuadratic[2].x * XFLOAT(ONE_THIRD))));
    pptCubic[2].y = XFLOAT((XFLOAT(pptQuadratic[1].y * XFLOAT(TWO_THIRDS)) + XFLOAT(pptQuadratic[2].y * XFLOAT(ONE_THIRD))));

// Copy the final point to the output buffer

    pptCubic[3] = pptQuadratic[2];
}

//-----------------------------------------------------------------------------
//
//  Function:  UpdateBounds
//
//  Synopsis:
//      Update our bounding box based on input points
//
//-----------------------------------------------------------------------------
void
UpdateBounds(
    _Inout_ XRECTF_RB *prcLocalSpaceBounds,          // The local space bounds
    _In_reads_(cPoints) const XPOINTF *pPtsSource,  // Source points
    _In_ XINT32 cPoints                              // Count of points
    )
{
    if (prcLocalSpaceBounds)
    {
        while (cPoints)
        {
            if (pPtsSource->x < prcLocalSpaceBounds->left)
            {
                prcLocalSpaceBounds->left = pPtsSource->x;
            }

            if (pPtsSource->x > prcLocalSpaceBounds->right)
            {
                prcLocalSpaceBounds->right = pPtsSource->x;
            }

            if (pPtsSource->y < prcLocalSpaceBounds->top)
            {
                prcLocalSpaceBounds->top = pPtsSource->y;
            }

            if (pPtsSource->y > prcLocalSpaceBounds->bottom)
            {
                prcLocalSpaceBounds->bottom = pPtsSource->y;
            }

            pPtsSource++;
            cPoints--;
        }
    }
}

const XFLOAT fuzz = 1.0e-12f;

//-----------------------------------------------------------------------------
//
//  Function:  SolveSpecialQuadratic
//
//  Synopsis:
//      Find the real positive roots of the equation a*x^2 + 2*b*x + c
//      This is a special case solver:
//      it should not be used as a general purpose quadratic equation solver
//
//      Implementation of the procedure for computing the relevant
//      zeros of the derivative of a cubic Bezier curve,
//      as discussed in the paper:
//
//        Michael Kallay.
//        Computing Tight Bounds for a Bezier Curve.
//        Journal of Graphics Tools, 7(3):13-17, 2002
//
//-----------------------------------------------------------------------------
XINT32                          // Return the number of relevant roots
SolveSpecialQuadratic(
    _In_  XFLOAT a,             // In: Coefficient of x^2
    _In_  XFLOAT b,             // In: Coefficient of 2*x
    _In_  XFLOAT c,             // In: Constant term
    _Out_writes_(2) XFLOAT * r) // Out: An array of size 2 to receive the zeros
{
    XINT32 nZeros = 0;
    XFLOAT d = b * b - a * c;    // = the discriminant

    if (d > 0)
    {
        d = sqrtf(d);
        b = - b;

        // Get the larger root - only if it is positive
        r[nZeros] = (b - d) / a;
        if (r[nZeros] > 0) { nZeros++; }

        // Get the larger root - only if it is positive
        r[nZeros] = (b + d) / a;
        if (r[nZeros] > 0) { nZeros++; }
    }
    // If the discriminant is negative, then the roots are complex
    //    and so we want our end points to be the answer.
    // The same goes for cases where both roots are negative since the
    //    answer should lie in [0-1].

    return nZeros;
}

//-----------------------------------------------------------------------------
//
//  Function:  GetDerivativeZeros
//
//  Synopsis:
//      Get the relevant zeros of the derivative of a cubic Bezier polynomial
//
//      Implementation of the procedure for computing the relevant
//      zeros of the derivative of a cubic Bezier curve,
//      as discussed in the paper:
//
//        Michael Kallay.
//        Computing Tight Bounds for a Bezier Curve.
//        Journal of Graphics Tools, 7(3):13-17, 2002
//
//-----------------------------------------------------------------------------
XINT32                          // Return the number of relevant zeros
GetDerivativeZeros(
    _In_ XFLOAT a,              // In: Bezier coefficient of (1-t)^3
    _In_ XFLOAT b,              // In: Bezier coefficient of 3t(1-t)^2
    _In_ XFLOAT c,              // In: Bezier coefficient of 3(1-t)t^2
    _In_ XFLOAT d,              // In: Bezier coefficient of t^3
    _Out_writes_(2) XFLOAT * r) // Out: An array of size 2 to receive the zeros
{
    XINT32 nZeros = 0;

    // Exact comparison is appropriate here
    if ((b - a) * (d - b) >= 0  &&  (c - a) * (d - c) >= 0)
    {
        // b and c lie between a and b.  By the convex hull property, all the
        // values lie between a and b, which we're considering anyway as the
        // endpoints, so derivative zeros are irrelevant
        return nZeros;
    }

    // The derivative of
    //      a(1-t)^3 + 3bt(1-t)^2 + 3c(1-t)t^2 + dt^3
    // is
    //      3 ((b-a)(1-t)^2 + 2(c-b)t(1-t) + (d-c)t^2)),
    // so:
    a = b - a;
    b = c - b;
    c = d - c;
    XFLOAT fa = XcpAbsF(a);
    XFLOAT fb = XcpAbsF(b) * fuzz;
    XFLOAT fc = XcpAbsF(c);

    if (fa < fb  &&  fc < fb)
    {
        // There are no relevant zeros
        return nZeros;
    }

    // The size of buffer r is 2 elements. Max value of buffer index i is 1 since max value of nzeros is 2.
    // disabling overflow warning & the warning about the predicates used for limiting the loop
    // termination being potentially unrelated to the buffer size
#pragma warning (push)
#pragma warning (disable : 26000)
#pragma warning (disable : 22102)

    if (fa > fc)
    {
        // Solve the quadratic a*s^2 + 2*b*s + c = 0, where s = (1-t)/t
        nZeros = SolveSpecialQuadratic(a, b, c, r);

        // Now s = (1-t)/t,  hence t = 1/(1+s)
        for (int i = 0;  i < nZeros;  i++)
            r[i] = 1.0f / (1 + r[i]);
    }
    else
    {
        // Solve the quadratic c + 2*b*s + a*s^2 = 0, where s = t/(1-t)
        nZeros = SolveSpecialQuadratic(c, b, a, r);

        // Now s = t / (1-t), hence s = s /(1+s)
        for (int i = 0;  i < nZeros;  i++)
            r[i] = r[i] / (1 + r[i]);
    }
#pragma warning (pop)
    return nZeros;
}

//-----------------------------------------------------------------------------
//
//  Function:  GetBezierPolynomValue
//
//  Synopsis:
//      Get the value at t of a given Bezier polynomial
//
//-----------------------------------------------------------------------------
XFLOAT          // Return a(1-t)^3 + 3bt(1-t)^2 + 3c(1-t)t^2 + dt^3
GetBezierPolynomValue(
    XFLOAT a,   // In: Coefficient of (1-t)^3
    XFLOAT b,   // In: Coefficient of 3t(1-t)^2
    XFLOAT c,   // In: Coefficient of 3(1-t)t^2
    XFLOAT d,   // In: Coefficient of t^3
    XFLOAT t)   // In: Parameter value t
{
    // Ignore NaNs
    ASSERT(!(-FUZZ >= t) && !(t >= 1 + FUZZ));
    XFLOAT t2 = t * t;
    XFLOAT s = 1 - t;
    XFLOAT s2 = s * s;

    return     a * s * s2
        +  3 * b * t * s2
        +  3 * c * t2 * s
        +      d * t * t2;
}


//-----------------------------------------------------------------------------
//
//  Function:  UpdateBezierBounds
//
//  Synopsis:
//      Update our bounding box based on input points for Bezier segments
//
//-----------------------------------------------------------------------------
void
UpdateBezierBounds(
    _Inout_ XRECTF_RB *prcLocalSpaceBounds,          // The local space bounds
    _In_reads_(cPoints) const XPOINTF *pPtsSource,  // Source points
    _In_range_(4,4) XINT32 cPoints                   // Count of points
    )
{
    // We need 4 points for a Bezier segment
    ASSERT( cPoints == 4 );

    if (prcLocalSpaceBounds)
    {
        XPOINTF ptMin;
        XPOINTF ptMax;
        XFLOAT rZeros[2];
        XINT32 nZeros;

        // X-loop
        ptMin.x = ptMax.x = pPtsSource[0].x;
        nZeros = GetDerivativeZeros(
            pPtsSource[0].x,
            pPtsSource[1].x,
            pPtsSource[2].x,
            pPtsSource[3].x,
            &rZeros[0]);

        for ( XINT32 i = 0; i<nZeros; i++ )
        {
            XFLOAT x = GetBezierPolynomValue(
                pPtsSource[0].x,
                pPtsSource[1].x,
                pPtsSource[2].x,
                pPtsSource[3].x,
                rZeros[i] );

            // Update Min
            if ( x < ptMin.x )
            {
                ptMin.x = x;
            }
            // Update Max
            if ( x > ptMax.x )
            {
                ptMax.x = x;
            }
        }

        // Update Min for last point
        if ( pPtsSource[3].x < ptMin.x )
        {
            ptMin.x = pPtsSource[3].x;
        }
        // Update Max for last point
        if ( pPtsSource[3].x > ptMax.x )
        {
            ptMax.x = pPtsSource[3].x;
        }

        // Y-loop
        ptMin.y = ptMax.y = pPtsSource[0].y;
        nZeros = GetDerivativeZeros(
            pPtsSource[0].y,
            pPtsSource[1].y,
            pPtsSource[2].y,
            pPtsSource[3].y,
            &rZeros[0]);

        for ( XINT32 i = 0; i<nZeros; i++ )
        {
            XFLOAT y = GetBezierPolynomValue(
                pPtsSource[0].y,
                pPtsSource[1].y,
                pPtsSource[2].y,
                pPtsSource[3].y,
                rZeros[i] );

            // Update Min
            if ( y < ptMin.y )
            {
                ptMin.y = y;
            }
            // Update Max
            if ( y > ptMax.y )
            {
                ptMax.y = y;
            }
        }

        // Update Min for last point
        if ( pPtsSource[3].y < ptMin.y )
        {
            ptMin.y = pPtsSource[3].y;
        }
        // Update Max for last point
        if ( pPtsSource[3].y > ptMax.y )
        {
            ptMax.y = pPtsSource[3].y;
        }

        // Now update bounds
        if (ptMin.x < prcLocalSpaceBounds->left)
        {
            prcLocalSpaceBounds->left = ptMin.x;
        }
        if (ptMax.x > prcLocalSpaceBounds->right)
        {
            prcLocalSpaceBounds->right = ptMax.x;
        }

        if (ptMin.y < prcLocalSpaceBounds->top)
        {
            prcLocalSpaceBounds->top = ptMin.y;
        }
        if (ptMax.y > prcLocalSpaceBounds->bottom)
        {
            prcLocalSpaceBounds->bottom = ptMax.y;
        }
    }
}

//
// The following function is very difficult for prefast to analyze.
// Disabling this for now.
// Bug 357: Evaluate this function more thoroughly and look for ways
// to refactor.
//
#pragma warning(push)
#pragma warning(disable : 26015)


#pragma warning(pop)
