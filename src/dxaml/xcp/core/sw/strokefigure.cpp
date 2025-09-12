// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <SafeInt/SafeInt.hpp>

const XFLOAT ARC_AS_BEZIER = 0.5522847498307933984f; // =(\/2 - 1)*4/3

extern XFLOAT
GetBezierDistance(
    _In_ XFLOAT eDot,
    _In_ XFLOAT eRadius
);

extern void
GenerateBezierFromQuadratic(
    _In_reads_(3) const XPOINTF *pptQuadratic,
    _Out_writes_(4) XPOINTF *pptCubic
);

extern void
UpdateBounds(
    _Inout_ XRECTF_RB *prcLocalSpaceBounds,          // The local space bounds
    _In_reads_(cPoints) const XPOINTF *pPtsSource,  // Source points
    _In_ XINT32 cPoints                              // Count of points
    );

extern void
UpdateBezierBounds(
    _Inout_ XRECTF_RB *prcLocalSpaceBounds,          // The local space bounds
    _In_reads_(cPoints) const XPOINTF *pPtsSource,  // Source points
    _In_range_(4,4) XINT32 cPoints                   // Count of points
    );

///////////////////////////////////////////////////////////////////////////////
/*
                            WIDENING DESIGN

The following was planned for widening that produces curves at the boundary, but
was never implemented:
CurveFitSink is derived from PathSink.  They both produce a path as the final
result.  But CurveFitSink has a curve fitting mode, set by CurveStart call.
While in that mode, it accumulates incoming points in a buffer.  It flushes
the buffer, fitting the points with Bezier segments, upon the first non-CurveTo
call.

CPen represents the elliptical pen tip.  It is instantiated from the nominal pen
and is hooked up with a sink upon construction.  It generates offset points
for a given direction and sends them to the sink.  CSimplePen is currently the
only derived class of CPen.  Additional classes may be derived in the future for
compound lines or for variable width.

Like CPen, the dash generator supports CPenInterface. The CWiden instantiates one,
hook it to the widener as the widening target, and hook a CPen for it to call with
its flattened segments.

The pen is defined with a "nominal" circle and possibly a transformation matrix
that maps it to an ellipse. The widened path is the union of the instances of this
ellipse placed at all the points of the original path, or "spine". Additional
geometry is added at the ends (caps) and at corners (rounded, mitered or beveled)

In practice, the widened path is constructed from a finitie number of instances
of the ellipse, with straight line segments connecting them. Curved segments on
the spine are approximated by polygonal paths ("flattened") for that purpose.

A vectors from the origin to a point on the pen's nominal circle is called
"radius vector".  For every spine segment, we compute the radius vector whose
image (under the pen's matrix) is in the direction of that segment.  This vector
is used to find the tip of a triangular or round cap on a segment.  The radius
vector perpendicular to it is used to find the "offset points", which are points
on the theoretical widened outline.  Between them we draw straight lines, but this
is only an approximation of the widened path. When the widened path is very curved
and wide, this is a poor approximation, and the outline would look jugged. To
prevent that, we apply a (cheap) test to the radius vector of the a new point
on a curve and previous one radius vector.  When this test fails, we switch to a
different model, approximating the path with straight segments and rounded corners.
This is an analytical version of the Hobby algorithm.  It is not cheap - that's why
reserve it to exterme cases.
*/

///////////////////////////////////////////////////////////////////////////////
// Implementation of CMatrix22
//-----------------------------------------------------------------------------
//
// Function Description:
//
// Set from raw data
//
//-----------------------------------------------------------------------------
CMatrix22::CMatrix22(
    _In_ const CMatrix22 &other) // In The matrix to copy

{
    m_rM11 = other.m_rM11;
    m_rM12 = other.m_rM12;
    m_rM21 = other.m_rM21;
    m_rM22 = other.m_rM22;
}
//-----------------------------------------------------------------------------
//
// Function Description:
//
// Reset to identity
//
//-----------------------------------------------------------------------------
void
CMatrix22::Reset()
{
    m_rM11 = m_rM22 = 1;
    m_rM12 = m_rM21 = 0;
}
//-----------------------------------------------------------------------------
//
// Function Description:
//
// Set from raw data
//
//-----------------------------------------------------------------------------
void
CMatrix22::Set(
    XFLOAT rM11, // In: The value to set for M11
    XFLOAT rM12, // In: The value to set for M12
    XFLOAT rM21, // In: The value to set for M21
    XFLOAT rM22) // In: The value to set for M22
{
    m_rM11 = rM11;
    m_rM12 = rM12;
    m_rM21 = rM21;
    m_rM22 = rM22;
}
//-----------------------------------------------------------------------------
//
// Function Description:
//
// Prepend a CMILMatrix to this matrix.
// The translation portion is ignored.
//
//-----------------------------------------------------------------------------
void
CMatrix22::Prepend(
    _In_opt_ const CMILMatrix *pMatrix
        // In: The matrix to prepend
    )
{
    if (pMatrix)
    {
        XFLOAT K[6] =
        {
            pMatrix->GetM11(),
            pMatrix->GetM12(),
            pMatrix->GetM21(),
            pMatrix->GetM22(),
            pMatrix->GetDx(),
            pMatrix->GetDy()
        };

        XFLOAT M1 = m_rM11;
        XFLOAT M2 = m_rM12;
        m_rM11 = M1 * K[0] + M2 * K[2];
        m_rM12 = M1 * K[1] + M2 * K[3];

        M1 = m_rM21;
        M2 = m_rM22;
        m_rM21 = M1 * K[0] + M2 * K[2];
        m_rM22 = M1 * K[1] + M2 * K[3];
    }
}
//+-----------------------------------------------------------------------------
//
//  Member:   CMatrix22::Finalize
//
//  Synopsis: Get the inverse of this matrix, possibly adjusting this matrix
//
//  Returns: TRUE if |determinant| >= threshold and the matrix is invertible
//
//  Notes: This is not a const method!  If this matrix represents a flipping
//  transformation, where left is switched with right, this method will prepend
//  flip to it.
//
//  We remove flips because they will switch the offset, from left to right,
//  confusing the algorithm. Since the matrix will be applied to the pen shape,
//  which is a circle, it doesn't affect the final pen's shape, due to the
//  perfect symmetry of circles.  This will not work with other pen shapes!
//
//  We are called after verifying that at least one entry of the matrix M is
//  greater than stroke-thickness threshold. That means that |V*M| > threshold*|V|
//  for some vector V.  The magification factor of M is <= its MAX(abs(eigenvalues)),
//  and det(M) is the product of its eigenvalues.  So here if
//  |det(M)| <= (threshold squared) then |W*M| <= threshold*|W| for some vector W,
//  and the pen is too thin in that direction.
//
//------------------------------------------------------------------------------
bool
CMatrix22::Finalize(
    XFLOAT rThresholdSquared,
        // Lower bound for |determinant(this)|
    _Out_ CMatrix22 &oInverse)
        // The inverse of this matrix
{
    XFLOAT rDet = m_rM11 * m_rM22 - m_rM12 * m_rM21;

    // Ignore NaNs
    ASSERT(!(rThresholdSquared <= 0));

    bool fEmpty = (XcpAbsF(rDet) < rThresholdSquared);
    if (fEmpty)
    {
        goto exit;
    }

    // Make sure the matrix does not flip
    if (rDet < 0)
    {
        // Prepend an X flip
        m_rM11 = -m_rM11;
        m_rM12 = -m_rM12;
        rDet = -rDet;
    }

    // Now set the inverse
    rDet = 1.0f / rDet;
    oInverse.m_rM11 = m_rM22 * rDet;
    oInverse.m_rM12 = -m_rM12 * rDet;
    oInverse.m_rM21 = -m_rM21 * rDet;
    oInverse.m_rM22 = m_rM11 * rDet;

exit:
    return !fEmpty;
}
//-----------------------------------------------------------------------------
//
// Function Description:
//
// Does this matrix preserve circles?
//
// It does if M11 = M22 and M12 = -M21 within tolerance.
// As a side effect (because it is needed internally) this method returns a
// bound on the squared scale factor of this matrix.  If isotropic, the bound
// is exact - it is the uniform scale factor squared.  Otherwise, it is the sum
// of the squares of the matrix entries.
//
//-----------------------------------------------------------------------------
XINT32
CMatrix22::IsIsotropic(
    _Out_ XFLOAT &rSqMax
        // Out: Bound on the squared scale factor of this matrix
) const
{
    // Exact test rather than with fuzz because it is cheaper, and a FALSE
    // negative may slow us down but will still produce correct results.
    XINT32 fIsotropic = ((m_rM11 == m_rM22) && (m_rM12 == -m_rM21));

    rSqMax = m_rM11 * m_rM11 + m_rM12 * m_rM12;
    if (!fIsotropic)
    {
        rSqMax += (m_rM21 * m_rM21 + m_rM22 * m_rM22);
    }

    return fIsotropic;
}
//-----------------------------------------------------------------------------
//
// Function Description:
//
// Invert the matrix in place
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT                // Return E_INVALIDARG if not invertable
CMatrix22::Invert()
{
    XFLOAT rDet = m_rM11 * m_rM22 - m_rM12 * m_rM21;
    if (XcpAbsF(rDet) < FUZZ)
        // The matrix is not invertible, w'll ignore the transformation
        return E_INVALIDARG;

    // Set the inverse
    rDet = 1 / rDet;
    XFLOAT rTemp = m_rM22 * rDet;
    m_rM12 = -m_rM12 * rDet;
    m_rM21 = -m_rM21 * rDet;
    m_rM22 = m_rM11 * rDet;
    m_rM11 =  rTemp;

    return S_OK;
}
//-----------------------------------------------------------------------------
//
// Function Description:
//
// Compute the coefficients of the pre-transform length
//
// Given a transformed vector V under the transformation M, we need to compute
// the length of the pre-transformed vector, which is VN where N is the inverse
// of M.  The length of VN will be sqrt((VN)(VN)'), where ' stands for
// transpose.  But (VN)(VN) = V(NN')V'.
//
//   if N = (a b) then  NN' = (p q)
//          (c d)             (q r)
//
// where p = a^2+b^2,  q = ac+bd, and r = c^2+d^2.
//
// If V = (x,y) then V(NN')V' = px^2 + 2qxy + ry^2, a quadratic function of
// x and y.  This method computes the coefficients of this function.
//
//-----------------------------------------------------------------------------
void
CMatrix22::GetInverseQuadratic(
    _Out_ XFLOAT &rCxx,
        // Out: Coefficient of x*x
    _Out_ XFLOAT &rCxy,
        // Out: Coefficient of x*y
    _Out_ XFLOAT &rCyy
        // Out: Coefficient of y*y
    )
{
    // Compute the inverse matrix
    if (SUCCEEDED(Invert()))
    {
        rCxx = m_rM11 * m_rM11 + m_rM12 * m_rM12;
        rCxy = 2 * (m_rM11 * m_rM21 + m_rM12 * m_rM22);
        rCyy = m_rM21 * m_rM21 + m_rM22 * m_rM22;
    }
    else
    {
        rCxx = 1;
        rCxy = 0;
        rCyy = 1;
    }
}
//-----------------------------------------------------------------------------
//
// Function Description:
//
// Multiply a column vector with the transpose of the matrix
//
//-----------------------------------------------------------------------------
void
CMatrix22::TransformColumn(
    _Inout_ XPOINTF &P
        // In/out: A vector, muliplied in place
    ) const
{
    XFLOAT r = m_rM11 * P.x + m_rM12 * P.y;
    P.y = m_rM21 * P.x + m_rM22 * P.y;
    P.x = r;
}
//-----------------------------------------------------------------------------
//
// Function Description:
//
// Transform in place
//
//-----------------------------------------------------------------------------
void
CMatrix22::Transform(
    _Inout_ XPOINTF &P
    ) const
{
    XFLOAT r = m_rM11 * P.x + m_rM21 * P.y;
    P.y = m_rM12 * P.x + m_rM22 * P.y;
    P.x = r;
}
//-----------------------------------------------------------------------------
//
// Function Description:
//
//   Prepend an X flip
//
//-----------------------------------------------------------------------------
void
CMatrix22::PreFlipX()
{
    m_rM11 = - m_rM11;
    m_rM12 = - m_rM12;
}
////////////////////////////////////////////////////////////////////////////////////
// Implementation of CWidener

//-----------------------------------------------------------------------------
//
// Function Description:
//
//   CWidener default constructor and destructor
//
//-----------------------------------------------------------------------------
CWidener::CWidener()
    : m_pMatrix(NULL)
    , m_rTolerance(MIN_TOLERANCE)
    , m_pTarget(&m_pen)
    , m_eStartCap(XcpPenCapFlat)
    , m_eEndCap(XcpPenCapFlat)
    , m_eLineJoin(XcpLineJoinRound)
    , m_dasher(&m_pen, NULL)
{
}

CWidener::~CWidener()
{
}

//-----------------------------------------------------------------------------
//
// Function Description:
//
//   CWidener working constructor.
//
//-----------------------------------------------------------------------------
CWidener::CWidener(
    _In_reads_opt_(1) const CMILMatrix *pMatrix,
        // In: Transformation matrix (NULL OK)
    XFLOAT rTolerance
        // In: Approximation tolerance
    )
    : m_pMatrix(pMatrix)
    , m_rTolerance(rTolerance)
    , m_eStartCap(XcpPenCapFlat)
    , m_eEndCap(XcpPenCapFlat)
    , m_eLineJoin(XcpLineJoinRound)
    , m_pTarget(&m_pen)
    , m_oLine(rTolerance)
    , m_oCubic(rTolerance)
    , m_dasher(&m_pen, pMatrix)
{
    if (pMatrix && pMatrix->IsIdentity())
    {
        m_pMatrix = NULL;
    }
}
//-----------------------------------------------------------------------------
//
// Function Description:
//
// Construct and set the internal pen for this widening.
//
// This private method constructs an internal pen of the class that is
// determined by the geometry of the stroking pen.  This internal pen captures
// the rendering transformation, and it is hooked up the pen to the sink into
// which it will generate.
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT
CWidener::Set(
    _In_ const CPlainPen &pen,
        // The stroking pen
    _In_ CWideningSink *pSink)
        // The widening sink
{
    m_eStartCap = pen.GetStartCap();
    m_eEndCap = pen.GetEndCap();
    m_eDashCap = pen.GetDashCap();
    m_eLineJoin = pen.GetJoin();

    // Should have been detected before calling us:
    ASSERT(!pen.IsEmpty());

    m_pen.Initialize(
                pen.GetGeometry(),
                m_pMatrix,
                m_rTolerance,
                pSink);

    if (XcpDashStyleSolid == pen.GetDashStyle())
    {
        SetTarget(&m_pen);
    }
    else
    {
        // Hook in the dasher
        SetTarget(&m_dasher);

        IFC_RETURN(m_dasher.Set(pen));
    }

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// Implementation of CPen

//-----------------------------------------------------------------------------
//
// Function Description:
//
// CPen default constructor
//
//-----------------------------------------------------------------------------
CPen::CPen()
    : m_eLineJoin(XcpLineJoinRound)
    , m_rNominalMiterLimit(1)
    , m_rRadius(1)
    , m_rRadSquared(1)
    , m_rMiterLimit(1)
    , m_rMiterLimitSquared(1)
    , m_rThreshold(1)
    , m_fCircular(FALSE)
{
}

//+-----------------------------------------------------------------------------
//
//  Member:   CPen::Set
//
//  Synopsis: Set the widening pen properties
//
//  Returns: FALSE if the pen is effectively empty
//
//  Notes: The pen is empty if one of its dimensions is less than tolerance / 128.
//         Since we are dealing with radii = 1/2 dimensions, we'll approximate
//         tolerance/256 with tolerance * 0.004.
//
//------------------------------------------------------------------------------
bool
CPen::Set(
    _In_ const CPenGeometry  &geom,
        // The pen's geometry information
    _In_reads_opt_(1)const CMILMatrix *pMatrix,
        // W to D transformation matrix (NULL OK)
    XFLOAT rTolerance
        // Approximation tolerance
    )
{
    // Ignore NaNs
    ASSERT(!(rTolerance <= 0));

    // The transformation matrix, the radius, and the circular flag
    bool fEmpty = !SetPenShape(geom, pMatrix, rTolerance * 0.004f);
    if (fEmpty)
    {
        goto exit;
    }

    // Store the world to device matrix
    m_oWToDMatrix.Reset();
    m_oWToDMatrix.Prepend(pMatrix);
    m_eLineJoin = geom.GetJoin();

    m_rNominalMiterLimit = geom.GetMiterLimit();
    if (m_rNominalMiterLimit < 1)
    {
        m_rNominalMiterLimit = 1;
    }

    m_rMiterLimit = m_rNominalMiterLimit * m_rRadius;
    m_rMiterLimitSquared = m_rMiterLimit * m_rMiterLimit;

    m_rRadSquared = m_rRadius * m_rRadius;

    // Refinement threshold
    SetThreshold(pMatrix, rTolerance);

exit:
    return !fEmpty;
}
//+-----------------------------------------------------------------------------
//
//  Member:   CPen::Copy
//
//  Synopsis: Copy from another pen
//
//------------------------------------------------------------------------------
void
CPen::Copy(
    _In_ const CPen &pen
        // A pen to copy basic properties from
    )
{
    m_eLineJoin = pen.m_eLineJoin;
    m_oMatrix = pen.m_oMatrix;
    m_oInverse = pen.m_oInverse;
    m_oMatrix = pen.m_oWToDMatrix;
    m_rRadius = pen.m_rRadius;
    m_rRadSquared = pen.m_rRadSquared;
    m_rNominalMiterLimit = pen.m_rNominalMiterLimit;
    m_rMiterLimit = pen.m_rMiterLimit;
    m_rMiterLimitSquared = pen.m_rMiterLimitSquared;
    m_rThreshold = pen.m_rThreshold;
    m_fCircular = pen.m_fCircular;
}
//+-----------------------------------------------------------------------------
//
//  Member:   CPen::SetPenShape
//
//  Synopsis: Set the pen's shape parameters. Private method.
//
//  Returns: False if the pen is effectively empty (relative to threshold)
//
//  Notes: The pen is circular if its width and height are equal.  But in the presence
//  of a rendering may change circular to non-circular and vice versa.
//
//  If the pen is deemed circular then we model it as a circle of the given
//  radius with an identity transformation.  If non-circular, we model it as a
//  circle of radius 1 mapped by the transformation.
//
//-----------------------------------------------------------------------------
bool
CPen::SetPenShape(
    _In_ const CPenGeometry &geom,
        // The pen's geometry information
    _In_reads_opt_(1) const CMILMatrix *pMatrix,
        // Rendering transformation (NULL OK)
    XFLOAT rThreshold)
        // Lower bound on pen dimensions, below which it is considered empty
{
    XFLOAT rFactor = 1;
    XFLOAT w = geom.GetWidth() / 2;
    XFLOAT h = geom.GetHeight() / 2;
    bool fEmpty = false;

    XFLOAT rThresholdSquared = rThreshold * rThreshold;
    if (0 == geom.GetAngle()) // Exact test is OK, this is just a shortcut
    {
        m_oMatrix.Set(w, 0, 0, h);
    }
    else
    {
        XFLOAT c = cosf((XFLOAT)geom.GetAngle());
        XFLOAT s = sinf((XFLOAT)geom.GetAngle());
        m_oMatrix.Set(w * c,  -w * s,
                      h * s,  h * c);
    }

    if (pMatrix)
    {
        // Fold the rendering transformation into the pen's matrix
        m_oMatrix.Prepend(pMatrix);

        m_fCircular = m_oMatrix.IsIsotropic(/*out*/ rFactor);
        fEmpty = rFactor < rThresholdSquared;
        if (fEmpty)
        {
            // All the matrix entries are small
            goto exit;
        }

        if (m_fCircular)
        {
            m_rRadius = sqrtf(rFactor);
            m_oMatrix.Reset();
        }
        else
        {
            fEmpty = !m_oMatrix.Finalize(rThresholdSquared, /*out*/ m_oInverse);
            m_rRadius = 1;
        }
    }
    else
    {
        m_fCircular = geom.IsCircular();
        if (m_fCircular)
        {
            fEmpty = w < rThreshold;
            m_rRadius = w;
        }
        else
        {
            fEmpty = !m_oMatrix.Finalize(rThresholdSquared, m_oInverse);
            m_rRadius = 1;
        }
    }

exit:
    return !fEmpty;
}
//+-----------------------------------------------------------------------------------
//
//  Member: CPen::SetThreshold
//
//  Synopsis: Set the threshold for deciding when the flattening needs to be refined
//
//  Notes:  This is a private method, called when the pen is set up.
//
// The pen's nominal shape is a circle. If there is a transformation then it's an
// ellipse, which is a projection of the circle whose radius r is obtained from the
// nominal radius by the maximal maginification factor of the transformation. The error
// between the arc and the chord defined by the two directions is r*(1-cos(a/2)), where
// a is the angle between the vectors. In that circle the angle between the vectors is
// equal to the angle between the original radius vectors.  We will test if
//
//           r*(1 - cos(a/2)) <? tolerance.
// or
//           cos(a/2) >? 1 - tolerance/r.
// But                    ________________
//           cos(a/2) = \/(1 + cos(a)) / 2
// So
//           cos(a) >? 2*(1 - tolerance/r)^2 - 1
//
// We will refine the flattening whenever cos(a) < threshold.
//
//------------------------------------------------------------------------------------
void
CPen::SetThreshold(
    _In_opt_ const CMILMatrix *pMatrix,
        // Transformation matrix (NULL OK)
    XFLOAT rTolerance
        // Approximation tolerance
    )
{
    XFLOAT radius = m_rRadius;
    if (pMatrix)
    {
        // There is a transformnation, so the radius of that circle is the pen's
        // ellipse is:
        radius *= pMatrix->GetMaxFactor();
    }

    ASSERT(!(radius < 0));
    ASSERT(!(rTolerance < 0));
    if (radius < rTolerance)
    {
        // The radius is less then tolerance, we'll never need rounding.  To make
        // the test "if (cos(a) < m_rThreshold" always fail:
        m_rThreshold = -2;
    }
    else
    {
        m_rThreshold = (1 - rTolerance / radius);
        m_rThreshold = 2 * m_rThreshold * m_rThreshold - 1;
    }

    // The test described above is if (cos(a) < trheshold)  But instead of testing
    // cos(a) we will test V*W = |V|*|W|*cos(a).  V and W will be radius vectors,
    // of length m_rRadius (not transformed!). So the actual test will be
    // if (V * W) < threshold * m_rRadius^2, hence:
    m_rThreshold *= m_rRadSquared;
}
//-----------------------------------------------------------------------------
//
// Function Description:
//
// Accept a point on a Bezier segment
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT
CPen::AcceptCurvePoint(
    _In_ const XPOINTF &point,
        // In: The point
    _In_ const XPOINTF &vecTangent,
        // In: The tangent there
    XINT32 fLast
        // In: Is this the last point on the curve?
    )
{
    XPOINTF vecRad;
    if (FAILED(ComputeRadiusVector(vecTangent, vecRad)))
        // 0 derivative on the curve, skip this point
        return S_OK;

    HRESULT hr = S_OK;
    XPOINTF vecSeg = point - m_ptPrev;

    //
    // If the stroke is thick enough, small corners in the skeleton
    // curve will be magnified greatly on the outside of the stroke.
    // If so, we add additional Beziers to smooth it out.
    //
    // NOTE: This is an expensive operation that not only introduces new
    // Beziers, but can also allocate new figures. Hence, it's worth checking
    // if this fixup will be in the viewable region.
    //

    XRECTF_RB rc;

    if (m_ptPrev.x < point.x)
    {
        rc.left = m_ptPrev.x;
        rc.right = point.x;
    }
    else
    {
        rc.left = point.x;
        rc.right = m_ptPrev.x;
    }

    if (m_ptPrev.y < point.y)
    {
        rc.top = m_ptPrev.y;
        rc.bottom = point.y;
    }
    else
    {
        rc.top = point.y;
        rc.bottom = m_ptPrev.y;
    }

    if (m_vecRad * vecRad < m_rThreshold)
    {
        //
        // Round the corner from the previous direction to the new segment
        //

        XPOINTF vecSegRad; // The radius vector corresponding to vecSeg
        if (SUCCEEDED(ComputeRadiusVector(vecSeg, vecSegRad)))
        {
            IFC(RoundTo(vecSegRad, m_ptPrev, m_vecPrev, vecSeg));
        }

        //
        // Draw the new segment
        //

        IFC(ProcessCurvePoint(point, vecSeg));

        //
        // Round the corner from the segment to the next tangent direction
        //

        IFC(RoundTo(vecRad, point, vecSeg, vecTangent));

        //
        // Note that RoundTo updates the current position of the outer rail,
        // but the inner rail remains untouched and is now incorrect. This is
        // fine if the next point is also on the curve, since the next
        // ProcessCurvePoint will correct this. If this is the last point on
        // the curve, however, we need one final ProcessCurvePoint.
        //

        if (fLast)
        {
            IFC(ProcessCurvePoint(point, vecTangent));
        }
    }
    else
    {
        // Just draw the new segment, the corner is smooth enough
        SetRadiusVector(vecRad);
        IFC(ProcessCurvePoint(point, vecSeg));
    }

Cleanup:
    m_vecPrev = vecTangent;
    m_ptPrev = point;
    return hr;
}
//-----------------------------------------------------------------------------
//
// Function Description:
//
// Get the radius vector on the pen coordinates circle for a given world
// coordinates direction.
//
// The pen shape is defined by a circle in its own coordinate space, with a
// transformation M to world coordinates that may turn it into an ellipse.  The
// ray at a given a world direction V intersects that ellipse at a point.  This
// method finds the inverse image of that point on the pen's circle in pen
// coordinates.
//
// Let us denote M' = the inverse of M.  Then the inverse image of V is VM'.
// A vector of length r (=the radius pen's circle) in the same direction is
// W = (r / |VM'|) VM'.
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT
// Return E_INVALIDARG if vecDirection=0
CPen::ComputeRadiusVector(
    _In_ const XPOINTF &vecDirection,
        // In: A not necessarily unit vector
    _Out_ XPOINTF &vecRad
        // Out: Radius vector on the pen-space circle
    ) const
{
    HRESULT hr = S_OK;

    // De-transform, if necessary
    vecRad = vecDirection;
    if (!m_fCircular)
        m_oInverse.Transform(vecRad);

    // Set to the right length
    XFLOAT rLength = sqrtf(vecRad.x * vecRad.x + vecRad.y * vecRad.y);
    if (rLength <= m_rRadius * FUZZ)
        hr = E_FAIL;
    else
        vecRad *= (m_rRadius / rLength);

    return hr;
}
//-----------------------------------------------------------------------------
//
// Function Description:
//
// Set the radius vector member to a given value, and update the currenr offset
// information for that value.  See ComputeRadiusVector for the definition of
// thatv ector.
//
//-----------------------------------------------------------------------------
void
CPen::SetRadiusVector(
    _In_ const XPOINTF &vecRad
        // In: A Given radius vector
    )
{
    m_vecRad = vecRad;
    GetOffsetVector(vecRad, m_vecOffset);
}
//-----------------------------------------------------------------------------
//
// Function Description:
//
// Update the offset information for a given direction vector on the path
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT
CPen::UpdateOffset(
    _In_ const XPOINTF &vecDirection
        // In: A nonzero direction vector
    )
{
    HRESULT hr;

    hr = ComputeRadiusVector(vecDirection, m_vecRad);
    if (SUCCEEDED(hr))
    {
        GetOffsetVector(m_vecRad, m_vecOffset);
    }
    return hr;
}
//-----------------------------------------------------------------------------
//
// Function Description:
//
// Get the offset vector that correosponds to a given radius vector
// been set.
//
//-----------------------------------------------------------------------------
void
CPen::GetOffsetVector(
    _In_ const XPOINTF &vecRad,
        // In: A radius vector
    _Out_ XPOINTF &vecOffset
        // Out: The corresponding offset vector
    ) const
{
    // Update the cached offset vector
    vecOffset = TurnRight(vecRad);
    if (!m_fCircular)
        m_oMatrix.Transform(vecOffset);
}

//-----------------------------------------------------------------------------
//
// Function Description:
//
// Get the point on the world space (elliptical) pen shape that corresponds to
// a given radius vector in the (circular) pen coordinates.
//
//-----------------------------------------------------------------------------
XPOINTF
CPen::GetPenVector(
    _In_ const XPOINTF &vecRad
        // In: A radius vector
    ) const
{
    XPOINTF vec(vecRad);

    if (!m_fCircular)
        m_oMatrix.Transform(vec);
    return vec;
}
//-----------------------------------------------------------------------------
//
// Function Description:
//
// Compute the numbers associated with the turning angle
//
// This private method analyzes the corner vectors.  If they are parallel it
// returns FALSE. Otherwise it computes their determinant and dot product, and
// determines the outer side of the turn.
//
//-----------------------------------------------------------------------------
bool
// Return FALSE if this is not a turn at all
CPen::GetTurningInfo(
    _In_ const XPOINTF &vecIn,
        // In: Vector in the direction coming in
    _In_ const XPOINTF &vecOut,
        // In: Vector in the direction going out
    _Out_ XFLOAT &rDet,
        // Out: The determinant of the vectors
    _Out_ XFLOAT &rDot,
        // Out: The dot product of the vectors
    _Out_ RAIL_SIDE &side,
        // Out: The outer side of the turn
    _Out_ XINT32 &f180Degrees
        // Out: =TRUE if this is a 180 degrees turn
    ) const
{
    side = RAIL_RIGHT;
    f180Degrees = FALSE;

    rDet = Determinant(vecIn, vecOut);
    rDot = vecIn * vecOut;

    if (XcpAbsF(rDet) <= XcpAbsF(rDot) * SQ_LENGTH_FUZZ)
    {
        if (vecIn * vecOut > 0)
            return false;
        else
            f180Degrees = TRUE;
    }
    else
    {
        if (rDet > 0)
            side = RAIL_LEFT;    // In a right handed coordinate system
        else
            side = RAIL_RIGHT;   // Turning left
    }
    return true;
}
/////////////////////////////////////////////////////////////////////////////
// Implementation of CSimplePen
// Some of the methods will be pushed up to the base class CPen.

//-----------------------------------------------------------------------------
//
// Function Description:
//
// Set the current left & right points to given values.
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT
CSimplePen::SetCurrentPoints(
    _In_ const XPOINTF &ptLeft,
        // In: Left point
    _In_ const XPOINTF &ptRight
        // In: Right point
    )
{
    m_ptCurrent[0] = ptLeft;
    m_ptCurrent[1] = ptRight;

    RRETURN(m_pSink->SetCurrentPoints(m_ptCurrent));
}
//-----------------------------------------------------------------------------
//
// Function Description:
//
// Extends one of the sides to a given point.
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT
CSimplePen::MiterTo(
    RAIL_SIDE side,
        // Which side to set the point
    _In_ const XPOINTF &ptMiter,
        // Miter corner
    _In_ const XPOINTF &ptNextStart,
        // The starting point of the next segment's offset
    XINT32 fExtended)
        // Extend all the way to ptNextStart if TRUE
{
// To save the cost of an extra segment per corner, a normal corner is extended to the miter
// point only.  If this is the last corner in a closed figure and the cap is flat, this may
// leave a wedge-shaped gap, as illustrated below:
//
//                                      gap
//                                      . .-----
//                                      |\|
//                                      | |\| Starting segment
//                                      |  -|--
//                                      |   |
//                        Ending segment
//
// Called with fExtended in that case will bridge that gap by going all the way to the start
// of the next segment's offset.

    if (fExtended)
    {
        XPOINTF P[2] = {ptMiter, ptNextStart};
        IFC_RETURN(m_pSink->PolylineWedge(side, 2, P));
        m_ptCurrent[side] = ptNextStart;
    }
    else
    {
        IFC_RETURN(m_pSink->PolylineWedge(side, 1, &ptMiter));
        m_ptCurrent[side] = ptMiter;
    }

    return S_OK;

}

//-----------------------------------------------------------------------------
//
// Function Description:
//
//  Start the widening of a new figure - internal version
//
// This is basically an additional entry point into StartFigure, for the dasher.
// The dasher already has the radius vector, so let's use it and skip its
// computation in StartFigure
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT
CSimplePen::StartFigure(
    _In_ const XPOINTF &pt,
        // In: Figure's first point
    _In_ const XPOINTF &vecSeg,
        // In: First segment's direction vector
    XINT32,
        // Ignored here
    XcpPenCap eCapType
        // In: The start cap type
    )
{
    ASSERT(RAIL_LEFT == 0  &&  RAIL_RIGHT == 1);

    // Set the offset vector and current offset point
    IFC_RETURN(UpdateOffset(vecSeg));

    m_ptPrev = pt;
    m_vecPrev = vecSeg;
    m_ptCurrent[RAIL_LEFT] = pt - m_vecOffset;
    m_ptCurrent[RAIL_RIGHT] = pt + m_vecOffset;

    IFC_RETURN(m_pSink->StartWith(m_ptCurrent));

    IFC_RETURN(DoBaseCap(RAIL_START, pt, vecSeg * -1.0f, eCapType));
    return S_OK;
}
//-----------------------------------------------------------------------------
//
// Function Description:
//
// Accept a point on a line segment
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT
CSimplePen::AcceptLinePoint(
    _In_ const XPOINTF &point
        // In: Point to draw to
    )
{
    m_ptCurrent[0] = point - m_vecOffset;
    m_ptCurrent[1] = point + m_vecOffset;
    m_ptPrev = point;
    return m_pSink->QuadTo(m_ptCurrent);
}
//-----------------------------------------------------------------------------
//
// Function Description:
//
// Process a point on a curve
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT
CSimplePen::ProcessCurvePoint(
    _In_ const XPOINTF &point,
        // In: Point to draw to
    _In_ const XPOINTF &vecSeg
        // In: Direction of segment we're coming along
    )
{
    m_ptCurrent[0] = point - m_vecOffset;
    m_ptCurrent[1] = point + m_vecOffset;
    return m_pSink->QuadTo(m_ptCurrent, vecSeg, point, m_ptPrev);
}
//-----------------------------------------------------------------------------
//
// Function Description:
//
// Do a start or end base cap.  A base cap connects the two rails of the
// widened path to close the widening outline.
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT
CSimplePen::DoBaseCap(
    RAIL_TERMINAL whichEnd,
        // In: RAIL_START or RAIL_END
    _In_ const XPOINTF &ptCenter,
        // In: Cap's center
    _In_ const XPOINTF &vec,
        // In: Segment's direction vector at this point
    XcpPenCap type
        // In: The type of cap
    )
{
    switch (type)
    {
    case XcpPenCapSquare:
        IFC_RETURN(DoSquareCap(whichEnd, ptCenter));
        break;

    case XcpPenCapFlat:
        IFC_RETURN(m_pSink->CapFlat(m_ptCurrent, TERMINAL2SIDE(whichEnd)));
        break;

    case XcpPenCapTriangle:
        {
            XPOINTF P = GetPenVector(m_vecRad);
            if (RAIL_START == whichEnd)
                P = ptCenter - P;
            else
                P += ptCenter;
            IFC_RETURN(m_pSink->CapTriangle(m_ptCurrent[OPPOSITE_SIDE(TERMINAL2SIDE(whichEnd))],
                                     P,
                                     m_ptCurrent[TERMINAL2SIDE(whichEnd)]));
            break;
        }

    case XcpPenCapRound:
        IFC_RETURN(DoRoundCap(whichEnd, ptCenter));
        break;
    }
    return S_OK;
}
//-----------------------------------------------------------------------------
//
// Function Description:
//
// Compute a square line cap
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT
CSimplePen::DoSquareCap(
    RAIL_TERMINAL whichEnd,
        // RAIL_START or RAIL_END
    _In_ const XPOINTF &ptCenter
        // Cap's center
    )
{
    XPOINTF V = GetPenVector(m_vecRad);

    if (RAIL_START == whichEnd)
    {
        // Record the current start
        XPOINTF ptStart = m_ptPrev;
        XPOINTF ptStartOffsets[2] = {m_ptCurrent[RAIL_LEFT], m_ptCurrent[RAIL_RIGHT]};

        // Move the start back by V
        IFC_RETURN(SetCurrentPoints(m_ptCurrent[RAIL_LEFT] - V, m_ptCurrent[RAIL_RIGHT] - V));
        m_ptPrev -= V;

        // Start from there and fill a quad to the previous start
        IFC_RETURN(m_pSink->CapFlat(m_ptCurrent, TERMINAL2SIDE(RAIL_START)));
        IFC_RETURN(m_pSink->QuadTo(ptStartOffsets));

        // Restore current start
        m_ptPrev = ptStart;
        m_ptCurrent[RAIL_LEFT] = ptStartOffsets[RAIL_LEFT];
        m_ptCurrent[RAIL_RIGHT] = ptStartOffsets[RAIL_RIGHT];
    }
    else
    {
        // Draw a line segment in the direction of V and cap
        IFC_RETURN(AcceptLinePoint(m_ptPrev + V));
        IFC_RETURN(m_pSink->CapFlat(m_ptCurrent, TERMINAL2SIDE(RAIL_END)));
    }
    return S_OK;
}
//-----------------------------------------------------------------------------
//
// Function Description:
//
// Compute a round line cap
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT
CSimplePen::DoRoundCap(
    RAIL_TERMINAL whichEnd,
        // In: RAIL_START or RAIL_END
    _In_ const XPOINTF &ptCenter
        // In: Cap's center
    )
{
    // Mainly for code readability:
    const XPOINTF &ptEnd = m_ptCurrent[TERMINAL2SIDE(whichEnd)];
    const XPOINTF &ptStart = m_ptCurrent[OPPOSITE_SIDE(TERMINAL2SIDE(whichEnd))];

    // Construct 2 Bezier arcs
    XPOINTF vecAcross = ptEnd - ptCenter;
    vecAcross *= ARC_AS_BEZIER;
    XPOINTF vecAlong = GetPenVector(m_vecRad);
    if (RAIL_START == whichEnd)
        vecAlong = vecAlong * -1.0f;
    XPOINTF ptMid = ptCenter + vecAlong;
    vecAlong *= ARC_AS_BEZIER;

    return m_pSink->BezierCap(ptStart,
                              ptStart + vecAlong,
                              ptMid - vecAcross,
                              ptMid,
                              ptMid + vecAcross,
                              ptEnd + vecAlong,
                              ptEnd);
}
//-----------------------------------------------------------------------------
//
// Function Description:
//
// Compute the contour of a mitered, rounded or beveled corner on the
// widened path.
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT
CSimplePen::DoCorner(
    _In_ const XPOINTF &ptCenter,
        // Corner center point
    _In_ const XPOINTF &vecIn,
        // Vector in the direction coming in
    _In_ const XPOINTF &vecOut,
        // Vector in the direction going out
    XcpLineJoin eLineJoin,
        // Corner type
    XINT32 fSkipped,
        // =TRUE if this corner straddles a degenerate segment
    XINT32 fRound,
        // Enforce rounded corner if TRUE
    XINT32 fClosing
        // This is the last corner in a closed figure if TRUE
    )
{
    XPOINTF vecRad;
    XPOINTF vecOffset, ptNext[2];
    HRESULT hr = S_OK;

    XFLOAT rSavedMiterLimit = 0.0f, rSavedNominalMiterLimit = 0.0f, rSavedMiterLimitSquared = 0.0f;

    if (fRound)
    {
        // Enforce a rounded corner
        eLineJoin = XcpLineJoinRound;
    }
    else if (fSkipped  &&  eLineJoin != XcpLineJoinRound)
    {
        // This corner straddles a degenerate edge so we want to miter it with
        // miter limit 1; that will look as if a very short edge is being widened

        // Save the normal values
        rSavedMiterLimit = m_rMiterLimit;
        rSavedNominalMiterLimit = m_rNominalMiterLimit;
        rSavedMiterLimitSquared = m_rMiterLimitSquared;

        // Set to miter with miter limit 1
        m_rNominalMiterLimit = 1;
        m_rMiterLimit = m_rRadius;
        m_rMiterLimitSquared = m_rRadSquared;
        eLineJoin = XcpLineJoinMiter;
    }

    if (FAILED(ComputeRadiusVector(vecOut, vecRad)))
    {
        // Eat the error, ignoring this corner
        goto Cleanup;
    }

    // Get the new radius vector and offset points on the outgoing segment
    GetOffsetVector(vecRad, vecOffset);
    ptNext[0] = ptCenter - vecOffset;
    ptNext[1] = ptCenter + vecOffset;

    RAIL_SIDE side;  // The outer side of the turn
    XINT32 f180Degrees;
    XFLOAT rDet, rDot;

    if (!GetTurningInfo(vecIn, vecOut, rDet, rDot, side, f180Degrees))
    {
        // It's a flat join, stay with the current points
        goto Cleanup;
    }

    // Now do the outside corner
    switch (eLineJoin)
    {
    case XcpLineJoinMiterClipped:
        if (f180Degrees)
        {
            IFC(m_pSink->SwitchSides());
        }
        else
        {
            IFC(m_pSink->DoInnerCorner(OPPOSITE_SIDE(side),ptCenter, ptNext));

            XPOINTF ptMiter;
            if (GetMiterPoint(vecRad,
                              rDet,
                              m_ptCurrent[side],
                              vecIn,
                              ptNext[side],
                              vecOut,
                              rDot,
                              ptMiter))
            {
                // Miter the corner
                IFC(MiterTo(side, ptMiter, ptNext[side], fClosing));
            }
            else
            {
                // Miter failed or exceeds the limit, so bevel the corner
                IFC(BevelCorner(side, ptNext[side]));
            }
        }
        break;

    case XcpLineJoinBevel:
        if (f180Degrees)
        {
            IFC(m_pSink->SwitchSides());
        }
        else
        {
            IFC(m_pSink->DoInnerCorner(OPPOSITE_SIDE(side),ptCenter, ptNext));
            IFC(BevelCorner(side, ptNext[side]));
        }
        break;

    case (XcpLineJoinMiter):
        if (f180Degrees)
        {
            IFC(Do180DegreesMiter());
        }
        else
        {
            IFC(m_pSink->DoInnerCorner(OPPOSITE_SIDE(side),ptCenter, ptNext));

            XPOINTF ptMiter;
            if (GetMiterPoint(vecRad,
                              rDet,
                              m_ptCurrent[side],
                              vecIn,
                              ptNext[side],
                              vecOut,
                              rDot,
                              ptMiter))
            {
                // Miter the corner
                IFC(MiterTo(side, ptMiter, ptNext[side], fClosing));
            }
            else
            {
                // Miter length exceeds the limit, so clip it
                IFC(DoLimitedMiter(m_ptCurrent[side],
                                    ptNext[side],
                                    rDot,
                                    vecRad,
                                    side));
            }
        }
        break;

    case (XcpLineJoinRound):
        IFC(m_pSink->DoInnerCorner(OPPOSITE_SIDE(side),ptCenter, ptNext));
        IFC(RoundCorner(ptCenter, m_ptCurrent[side],
                             ptNext[side], vecRad, side));
        break;
    }

    // Update for the next segment
    m_vecRad = vecRad;
    m_vecOffset = vecOffset;
    m_ptPrev = ptCenter;
    m_vecPrev = vecOut;

Cleanup:
    if (fSkipped  &&  eLineJoin != XcpLineJoinRound)
    {
        // Restore the miter settings
        m_rMiterLimit = rSavedMiterLimit;
        m_rNominalMiterLimit = rSavedNominalMiterLimit;
        m_rMiterLimitSquared = rSavedMiterLimitSquared;
    }

    RRETURN(hr);
}
//-----------------------------------------------------------------------------
//
// Function Description:
//
// This is a private method called by DoCorner.  It computes the clipping line
// that cuts the corner when the miter length exceeds the miter limit.
//
// In pen coordinates, we are looking for a line that clips the corner, whose
// distance from the spine corner will be equal to m_rMiterLimit*m_rRadius.
// the vector along the outer offset from the offset point to the clip point
// is ratio * radius vector.  So we compute this ratio, and then apply it to the
// the radius vector transformed to world coordinates.  The result will take us
// from the offset point to the clip point.
//
// Notations:
//   a = the angle between the legs of the corner
//   L = miter limit
//   r = pen radius (=1/2 line width)
//   s = the distance from the offset point to the clip point on the offset line
//   dot = -dot product of the radius vectors
//
//
//           offset point
//        --*----------------------  offset line
//       | *
//       |*
//    -  * clip point
//    s  |                 spine
//    -  * offset  ................          vecRadNext
//       | point   . a                       ------>
//       |         .
//       |         .
//       |         .                        /|\
//       |  - r -  .        -------          |
//       |         .       |                 | m_vecRad
//       |         .       |                 |
//
//
// and we want to compute s / r.
//
// By trigonometry, s = (L*r - r sin(a/2)) / cos(a/2), so
//
//       s     L - sin(a/2)
//      --- = ---------------
//       r      cos(a/2)
//
// The trig formulas for half angle are:
//                   ________________                      ________________
//      cos(a/2) = \/(1 - cos(a)) / 2   and   sin(a/2) = \/(1 + cos(a)) / 2
//
// and cos(a) = dot/r^2, so
//                   ___________________            _______________
//       s     L - \/(1 - dot / r^2) / 2    L*r - \/(r^2 - dot) / 2
//      --- = ----===================--- = ----===============-----
//       r      \/(1 + dot / r^2) / 2        \/(r^2 + dot) / 2
//
// The deniminator is 0 when a = 180, hence the corner is flat, so we treat it
// as no-corner and do nothing.  (Should never happen, but remember Murphey...)
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT
CSimplePen::DoLimitedMiter(
    _In_ const XPOINTF &ptIn,
        // In: Outer offset of incoming segment
    _In_ const XPOINTF &ptNext,
        // In: Outer offset of outgoing segment
    XFLOAT rDot,
        // In: -(m_vecRad * vecRadNext)
    _In_ const XPOINTF &vecRadNext,
        // In: Radius vector of outgoing segment
    RAIL_SIDE side
        // In: Turn's outer side, RAIL_LEFT or RAIL_RIGHT
    )
{
    XFLOAT rDenom = (m_rRadSquared + rDot) / 2;

    if (rDenom > 0) // Otherwise it's not really a corner
    {
        rDenom = sqrtf((XFLOAT)rDenom);
        XFLOAT rRatio = (m_rRadSquared - rDot) / 2;   // Numerator
        if (rRatio < 0)
            rRatio = 0;
        else
            rRatio = sqrtf((XFLOAT)(rRatio));
        rRatio = m_rMiterLimit  - rRatio;
        if (rRatio < 0)  // Shouldn't happen but...
            rRatio = 0;

        if (rDenom > rRatio * FUZZ) // Otherwise it's not really a corner
        {
            rRatio /= rDenom;
            XPOINTF V = GetPenVector(m_vecRad);
            XPOINTF W = GetPenVector(vecRadNext);

            // Generate the bevel
            XPOINTF P[3] = {m_ptCurrent[side] + V * rRatio,
                             ptNext - W * rRatio,
                             ptNext};

            m_ptCurrent[side] = ptNext;
            IFC_RETURN(m_pSink->PolylineWedge(side, 3, P));
        }
    }

    return S_OK;
}

//-----------------------------------------------------------------------------
//
// Function Description:
//
// Miter the corner when the turning angle is 180 degrees.
// This is a private method called by DoCorner.
// Since there is not really a corner, we don't really miter.  Instead, we
// move the points outwards, connect left to right and right to left
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT
CSimplePen::Do180DegreesMiter()
{
    // First move the current points outwards to the miter limit
    XPOINTF vec = GetPenVector(m_vecRad);
    vec *= m_rNominalMiterLimit;
    IFC_RETURN(SetCurrentPoints(m_ptCurrent[0] + vec, m_ptCurrent[1] + vec));
    IFC_RETURN(m_pSink->SwitchSides());

    return S_OK;
}

//-----------------------------------------------------------------------------
//
// Function Description:
//
// Bevel the corner
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT
CSimplePen::BevelCorner(
    RAIL_SIDE side,
        // In: The side of the outer corner
    _In_ const XPOINTF &ptNext
        // In: The bevel's endpoint
    )
{
    m_ptCurrent[side] = ptNext;
    RRETURN(m_pSink->PolylineWedge(side, 1, &ptNext));
}

//-----------------------------------------------------------------------------
//
// Function Description:
//
// Compute the outline of the arc that rounds the outer corner.
// This is a private method called by DoCorner.
//
// The arc is approximated by 1 or 2 Bezier curves, depending on the turn angle.
// The computation is done in the pen coordinates, where the pen shape is
// circular round, and then transformed with the pen transform to world
// coordinates where the pen is elliptical.  The  curve's endpoints obviously
// coincide with the arc's endpoints, which are at the tips of the start and end
// radius-vectors.  The control points are on the tangent lines there, whose
// directions are the radius vectors turned 90 degrees left or right, depending
// on the turning direction. It remains to compute where on these tangent
// the control points should be placed.
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT
CSimplePen::RoundCorner(
    _In_ const XPOINTF &ptCenter,
        // In: Corner point on the spine
    _In_ const XPOINTF &ptIn,
        // In: Outer offset point of incoming segment
    _In_ const XPOINTF &ptNext,
        // In: Outer offset point of outgoing segment
    _In_ const XPOINTF &vecRad,
        // In: New value of m_vecRad
    RAIL_SIDE side
        // In: Side to be rounded, RAIL_LEFT or RAIL_RIGHT
    )
{
    HRESULT hr = S_OK;

    XFLOAT r = vecRad * m_vecRad;   // = rad^2 * cos(angle between radius vecs)
    if (r > m_rThreshold)
    {
        // A very flat turn, the arc can be approximated by the bevel
        IFC(m_pSink->PolylineWedge(side, 1, &ptNext));
    }
    else if (r >= 0)
    {
        // The arc can be approximated by a single Bezier curve
        r = GetBezierDistance(r, m_rRadius);
        XPOINTF ptBez1 = ptIn + GetPenVector(m_vecRad) * r;
        XPOINTF ptBez2 = ptNext - GetPenVector(vecRad) * r;
        IFC(m_pSink->CurveWedge(side, ptBez1, ptBez2, ptNext));
    }
    else
    {
        // Need to approximate the arc by 2 Bezier curves ---

        //
        // Get the radius vector for the arc's midpoint
        //
        // We use a little complex arithmetic here. Given two equal length vectors,
        // a and b (each represented as a complex number x + iy), the midpoint, c,
        // is given by sqrt(a * b). Note that -c is also a midpoint of a and b, so we
        // need to do a check at the end to see which one we need.
        //
        // In this case, a is vecRad and b is m_vecRad.
        //

        XFLOAT c2Real = vecRad.x * m_vecRad.x - vecRad.y * m_vecRad.y;  // Real component of c^2
        XFLOAT c2Imag = vecRad.x * m_vecRad.y + vecRad.y * m_vecRad.x;  // Imaginary component of c^2

        //
        // The square root of a complex number x + iy is given by the formula:
        //
        //      sqrt( (L + x)/2 ) + i sgn(y) sqrt( (L - x)/2 )
        //
        // Where L is the length of the vector (x,y) and sgn() is the sign operator:
        //
        //               / +1   (t > 0)
        //      sgn(t) = |  0   (t == 0)
        //               \ -1   (t < 0)
        //
        //  We can ignore the behavior of sgn(t) at 0, though, because when y == 0, x == L
        //  and hence the value of the sqrt() will be 0 anyway. We can also assume that |c2Real| is
        //  less than L, since no component of a vector is greater than its length. Due to numerical
        //  error, though, this might not actually hold. To ensure that we don't take the square root
        //  of a negative number, we take absolute values first.
        //

        XFLOAT L = m_rRadius * m_rRadius; // |a*b| = |a|*|b|
        XFLOAT cReal = sqrtf(XcpAbsF(0.5f *(L + c2Real)));
        XFLOAT cImag = (c2Imag > 0.0f ? 1.0f : -1.0f) * sqrtf(XcpAbsF(0.5f *(L - c2Real)));

        XPOINTF vecMid;

        vecMid.x = cReal;
        vecMid.y = cImag;

        //
        // At this point, vecMid may be pointing in the opposite direction than desired
        // (remember that c and -c from the above discussion are both square roots of a*b).
        //
        // Rotating vecRad by 90 degrees in the direction in which the curve should be added
        // will give us roughly the direction in which vecMid should be pointed ( plus or minus
        // 45 degrees ). We can thus use the dot product to determine whether we need to negate
        // vecMid.
        //

        XPOINTF direction;
        direction = TurnRight(vecRad);
        if (RAIL_LEFT == side)
        {
            direction = direction * -1.0f;
        }

        if (vecMid * direction < 0.0f)
        {
            vecMid = vecMid * -1.0f;
        }

        //
        // vecMid *should* now be pointing in the right direction but
        // unfortunately large stretch transforms can cause the angle between
        // vecIn and vecOut to be very close to 180 degrees. When inverting
        // this transform, the angle between m_vecRad and vecRad may be on the
        // *opposite* side of 180 degrees (but need not be close to 180). In
        // this case, the angle between vecMid and vecRad may be well over 90
        // degrees, which is inconsistent. Since vecIn and vecOut are closer to
        // what will actually be drawn, we trust their values and just assume that
        // vecMid really is pointing in the right direction.
        //
        XFLOAT radDotMid = XcpAbsF(vecRad * vecMid);

        // Get the relative distance to the control points
        r = GetBezierDistance(radDotMid, m_rRadius);

        // Get the arc's midpoint as the tip of the offset in this direction
        XPOINTF ptMid;
        ptMid = TurnRight(vecMid);
        if (RAIL_LEFT == side)
            ptMid = ptMid * -1.0f;
        if (!m_fCircular)
            m_oMatrix.Transform(ptMid);
        ptMid += ptCenter;

        // First arc, from ptIn to ptMid
        XPOINTF ptBez = ptIn + GetPenVector(m_vecRad) * r;
        XPOINTF vecBezAtMid = GetPenVector(vecMid) * r;
        IFC(m_pSink->CurveWedge(side, ptBez, ptMid - vecBezAtMid, ptMid));

        // Second arc, from ptMid ptNext
        ptBez = ptNext - GetPenVector(vecRad) * r;
        IFC(m_pSink->CurveWedge(side, ptMid + vecBezAtMid, ptBez, ptNext));
    }

Cleanup:
    m_ptCurrent[side] = ptNext;
    RRETURN(hr);
}
//-----------------------------------------------------------------------------
//
// Function Description:
//
// Get the outer miter point, if legitimate and within miter limit.
// In any case, compute the dot product
//
// Failure to compute a miter point (and returning FALSE) is not a big deal. It
// should only happen if vecIn and vecOut are almost collinear, and then the
// caller will gloss over the corner (if rDot > 0) or handle it as a 180 degree
// turn (if rDet <= 0).
//
//-----------------------------------------------------------------------------
XINT32
// Return FALSE if miter-limit is exceeded
CSimplePen::GetMiterPoint(
    _In_ const XPOINTF &vecRad,
        // Radius vector for the outgoing segment
    XFLOAT rDet,
        // The determinant of vecIn and vecOut
    _In_ const XPOINTF &ptIn,
        // Offset point of the incoming segment
    _In_ const XPOINTF &vecIn,
        // Vector in the direction coming in
    _In_ const XPOINTF &ptNext,
        // Offset point of the outgoing segment
    _In_ const XPOINTF &vecOut,
        // Vector in the direction going out
    _Out_ XFLOAT &rDot,
        // The dot product of the 2 radius vectors
    _Out_ XPOINTF &ptMiter
        // The outer miter point, if within limit
    )
{
    XINT32 fDone = FALSE;

    rDot = (vecRad * -1.0f) * m_vecRad;

    /* The miter point is the intersection of the extensions of the two offset segments.
       {ptIn rIn * vecIn} and {ptNext + rOut * vecOut}.

    To computed the intersection, solve the equations:

                  ptIn + rIn * vecIn = ptNext + rOut * vecOut
    or:
                  rIn * vecIn - rOut * vecOut = ptNext - ptIn

    The unknowns are rIn and rOut.  Since we have already chose to be on the outer offset,
    we expect the intersection point to be in the correct extensions of the offset segments
    - forward from ptIn on the incoming segment and backward from ptOut on the outgoing one.
    This translates to rIn > 0 and rOut < 0.  Numerical error may produces a bad point,
    which may show up as a spike, so we guard against it.

    The vector equation represents 2 scalar equations in rIn and rOut.
    By Cramer's rule the solution is:

        rIn = det(ptNext - ptIn, -vecOut) / det(vecIn, -vecOut)
        rOut = det(vecIn, ptNext - ptIn) / det(vecIn, -vecOut)

    After some basic algebra, and using pt = ptNext - ptIn:

        rIn = det(pt, vecOut) / det(vecIn, vecOut)
        rOut = det(pt, vecIn) / det(vecIn, vecOut)
     */

    XPOINTF pt = ptNext - ptIn;
    XFLOAT rIn;
    XFLOAT rInNumerator = Determinant(pt, vecOut);
    XFLOAT rOutNumerator = Determinant(pt, vecIn);

    // We don't need rOut, we only need to check its sign, so instead of the fraction's
    // sign we examine the signs of its numerator and denominator.  For rIn we will eventually
    // need to divide, but we can save the cost division if we determine that it will fail
    // by examining the numertator and denominator.
    if (rDet < 0)
    {
        fDone = rInNumerator < 0    &&         // so that rIn > 0
                rOutNumerator > 0   &&         // so that rOut < 0
                rDet < rInNumerator * FUZZ;    // so that |rDet| > |rNumerator* FUZZ|
    }                                          // and it is safe to divide
    else    // rDet >= 0
    {
        fDone = rInNumerator > 0   &&       // so that rIn > 0
                rOutNumerator < 0  &&       // so that rOut < 0
            rDet > rInNumerator * FUZZ;     // so that |rDet| > |rNumerator* FUZZ|
    }                                       // and it is safe to divide

    if (!fDone)
    {
        // The incoming and outgoing edges are almost collinear
        if (rDot < 0)
        {
            // This is a smooth join, let's just gloss over the corner.
            ptMiter = ptNext;
            fDone = TRUE;
        }
        // else this is close to 180 degree turn, we cannot miter it.
        // Either way --
        goto exit;
    }

    rIn = rInNumerator / rDet;
    ptMiter = ptIn + vecIn * rIn;
    // Miter point computed successfully

/* Check if this corner can be mitered with miter distance <= miter limit.

The test is done in pen coordinate space.  There the miter distance, which
is the distance from the center to the miter corner, is R / sin(a/2), where
R is the pen radius, and a is the angle at the corner.

The test is R/sin(a/2) <= L, where L is the limit.

But sin(a/2) = sqrt((1 - cos(a)) / 2), so the test is
R <= L*sqrt((1 - cos(a))/2), or 2R^2 <= (1 - cos(a))L^2.

Eliminating cos(a), we get cos(a) <= 1 - 2R^2 / L^2.

Multiply both sides by R^2 and substitute R^2*cos(a) = -U*V, where U and V
are the radius vectors of the 2 segments, we have:
U*V <= R^2(1 - 2 R^2 / L^2), or (U*V)L^2 <= R^2(L^2 - 2 R^2). */

    fDone = (rDot * m_rMiterLimitSquared <=
           m_rRadSquared * (m_rMiterLimitSquared - 2 * m_rRadSquared));
exit:
    return fDone;
}
//+-----------------------------------------------------------------------------
//
//  Member:   CSimplePen::EndStrokeOpen
//
//  Synopsis: End an a stroke as a open
//
//------------------------------------------------------------------------------
_Check_return_ HRESULT
CSimplePen::EndStrokeOpen(
    XINT32 fStarted,
        // = TRUE if the widening has started
    _In_ const XPOINTF &ptEnd,
        // Figure's endpoint
    _In_ const XPOINTF &vecEnd,
        // Direction vector there
    XcpPenCap eEndCap,
        // The type of the end cap
    XcpPenCap eStartCap)
        // The type of start cap (optional)
{
    if (!fStarted)
    {
        // We should be stroking but failed to start any segment, so we'll just
        // widen as a single point, with a horizontal (in shape space) direction vector.
        XPOINTF vecIn;

        vecIn.x = 1.0f;
        vecIn.y = 0.0f;

        m_oWToDMatrix.Transform(vecIn);

        IFC_RETURN(StartFigure(ptEnd, vecIn, FALSE, eStartCap));
    }

    IFC_RETURN(DoBaseCap(RAIL_END, ptEnd, vecEnd, eEndCap));
    IFC_RETURN(m_pSink->AddFigure());
    return S_OK;
}
//+-----------------------------------------------------------------------------
//
//  Member:   CSimplePen::EndStrokeClosed
//
//  Synopsis: End an a stroke as a closed
//
//------------------------------------------------------------------------------
_Check_return_ HRESULT
CSimplePen::EndStrokeClosed(
    _In_ const XPOINTF &ptEnd,
        // Figure's endpoint
    _In_ const XPOINTF &vecEnd)
        // Direction vector there
{
    IFC_RETURN(DoBaseCap(RAIL_END, ptEnd, vecEnd, XcpPenCapFlat));
    IFC_RETURN(m_pSink->AddFigure());
    return S_OK;
}

//+-----------------------------------------------------------------------------
//
//  Member:   CSimplePen::Aborted
//
//  Synopsis: Say if the widening has been aborted
//
//  Notes:    This is only used by CHitTestWideningSink for early out when a hit
//            has been detected.  It is not meant to be used for error exit.
//
//------------------------------------------------------------------------------
XINT32
CSimplePen::Aborted()
{
    // Ask the sink
    return m_pSink->Aborted();
}

//+-----------------------------------------------------------------------------
//
//  Member:   CSimplePen::RoundTo
//
//  Synopsis: Round the corner that would have been introduced by widening a very
//            curved and very wide flattened segment
//
//------------------------------------------------------------------------------
_Check_return_ HRESULT
CSimplePen::RoundTo(
    _In_ const XPOINTF &vecRad,
        // In: Radius vector of the outgoing segment
    _In_ const XPOINTF &ptCenter,
        // In: Corner center point
    _In_ const XPOINTF &vecIn,
        // In: Vector in the direction coming in
    _In_ const XPOINTF &vecOut
        // In: Vector in the direction going out
    )
{
    XPOINTF vecOffset, ptNext[2];

    // Get the new radius vector and offset points on the outgoing segment
    GetOffsetVector(vecRad, vecOffset);
    ptNext[0] = ptCenter - vecOffset;
    ptNext[1] = ptCenter + vecOffset;

    // Determine the outer side of the turn
    RAIL_SIDE side = Determinant(vecIn, vecOut) > 0 ?RAIL_LEFT : RAIL_RIGHT;

    // Round the outer corner
    IFC_RETURN(RoundCorner(ptCenter, m_ptCurrent[side], ptNext[side], vecRad, side));

    // Update for the next segment
    m_vecRad = vecRad;
    m_vecOffset = vecOffset;
    m_ptPrev = ptCenter;

    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
// Implementation of CDasher
/*
                 DASHED LINES DESIGN

There are several widening scenarios.  The following crude diagrams depict the
flow of information:

Simple pen without dashes: CWidener --> CSimplePen --> CWideningSink

Simple pen with dashes: CWidener --> CDasher --> CSimplePen --> CWideningSink

Glossary
    An EDGE is a smooth piece of the figure between corners or start and end.
    The edge is a sequence of SEGMENTS.  If the edge is a straight line then it
    comprises one segment.  If it is a curve then the segments are the result of
    its flattening.

CDasher accumulates segments with the information needed for widening and
accumulated length. At every corner (between edges) and at the figure end,
the Dasher flushes the segments buffer and sends the dashes to the pen to draw.

The buffer must contain all the information needed for the pen at flush time,
so we record points, tangents, and a flag indicating whether the segment came
from a line segment (rather than from curve flattening)

If the figure is closed then the first dash may have to be the second half of
the last dash.  So if it starts on a dash, we'll start it with a flat cap.
After the last dash we'll do the corner (between figure end and start) and exit
with a flat cap, that will abut with the flat cap of the first dash.  If there
is no end dash then we'll append a 0-length segment with the right cap.

Some of the functionality of CDasher is delegated to its class members CSegments,
capturing the data and behavior of the segments buffer, and CDashSequence,
encapsulating the dash sequence.

We dash one edge at a time. We try to dash it in a synchronized mode, i.e. always
ending at the same point (=DashOffset) in the dash sequence. For that we tweak the
sequence length. But if the edge is substantially shorter than one full instance
then we dash in unsynchronized mode. For the canned dash styles the offset is set
to half the first dash. */

////////////////////////////////////////////////////////////////////////////////
// Implementation of CSegData

// Constructor
CDasher::CSegData::CSegData(
    XINT32 fIsALine,
        // In: =TRUE if this is a line segment
    _In_ const XPOINTF &ptEnd,
        // In: Segment endpoint
    _In_ const XPOINTF &vecTangent,
        // In: The tangent vector there
    _In_ const XPOINTF &vecSeg,
        // In: The segment direction vector
    XFLOAT rLength
        // In: Accumulated length so far
    )
: m_ptEnd(ptEnd), m_vecTangent(vecTangent), m_vecSeg(vecSeg),
  m_rLength(rLength), m_fIsALine(fIsALine)
{
}

////////////////////////////////////////////////////////////////////////////////
// Implementation of CSegments

CDasher::CSegments::CSegments(
    _In_reads_opt_(1) const CMILMatrix *pMatrix
        // In: Transformation matrix (NULL OK)
    )
{
    m_uCurrentIndex = 1;
    m_rCxx = 1;
    m_rCxy = 0;
    m_rCyy = 1;
    m_cSegmentCapacity = 0;
    m_cSegments = 0;
    m_rgSegments = NULL;

    if (pMatrix)
    {
        CMatrix22 matrix(*pMatrix);
        matrix.GetInverseQuadratic(m_rCxx, m_rCxy, m_rCyy);
    }
}

CDasher::CSegments::~CSegments()
{
    delete [] m_rgSegments;
}

//-----------------------------------------------------------------------------
//
// Function Description:
//
// Start a dash
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT
CDasher::CSegments::StartWith(
    _In_ const XPOINTF &ptStart,
        // In: Starting point
    _In_ const XPOINTF &vecTangent
        // In: The tangent vector there
    )
{
    ASSERT(0 == m_cSegments);
    RRETURN(AddSegment(CSegData(TRUE, ptStart, vecTangent, vecTangent, 0)));
}

//-----------------------------------------------------------------------------
//
// Function Description:
//
// Add a segment to m_rgSegments
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT
CDasher::CSegments::AddSegment(
    _In_ const CSegData &segData
    )
{
    XUINT32 cNewCapacity = 0;

    // Ensure we have enough storage to add a segment

    if (m_cSegments + 1 > m_cSegmentCapacity)
    {
        // Allocate enough for cSegments and some extra for growing
        IFCCHECK_RETURN(SafeAdd(m_cSegments, 32, cNewCapacity));
        CSegData *pSegments = new CSegData[cNewCapacity];

        if (m_rgSegments && m_cSegments > 0)
        {
            memcpy(pSegments, m_rgSegments, m_cSegments*sizeof(CSegData));
        }

        delete [] m_rgSegments;
        m_rgSegments = pSegments;

        m_cSegmentCapacity = cNewCapacity;
    }

    // Add the segment

    m_rgSegments[m_cSegments] = segData;
    m_cSegments++;

    return S_OK;
}

//-----------------------------------------------------------------------------
//
// Function Description:
//
// Accept a point on a line segment
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT
CDasher::CSegments::Add(
    _In_ const XPOINTF &ptEnd,
        // In: Segment endpoint
    _In_reads_opt_(1) const XPOINTF *pTangent
        // Optional: Tangent vector there
    )
{
    HRESULT hr = S_OK;

    ASSERT(m_cSegments > 0);
    XPOINTF vecSeg = ptEnd - m_rgSegments[m_cSegments-1].m_ptEnd;

    // Get the pre-transform length of the segment
    XFLOAT rLength = sqrtf(m_rCxx * vecSeg.x * vecSeg.x +
                          m_rCxy * vecSeg.x * vecSeg.y +
                          m_rCyy * vecSeg.y * vecSeg.y);

    if(rLength >= FUZZ)
    {
        XINT32 fIsALine = NULL == pTangent;
        vecSeg /= rLength;
        rLength += m_rgSegments[m_cSegments-1].m_rLength;
        hr = AddSegment(CSegData(fIsALine, ptEnd,
                                           fIsALine? vecSeg : *pTangent,
                                           vecSeg, rLength));
    }

    RRETURN(hr);
}

//-----------------------------------------------------------------------------
//
// Function Description:
//
// Reset to empty state
//
//-----------------------------------------------------------------------------
void
CDasher::CSegments::Reset()
{
    m_cSegments = 0; // Reset segments
    m_uCurrentIndex = 1;
}

//-----------------------------------------------------------------------------
//
// Function Description:
//
// Get the point and vectors at a given location on the current segment
//
//-----------------------------------------------------------------------------
void
CDasher::CSegments::ProbeAt(
    XFLOAT rLoc,
        // In: The location (lengthwise) to probe at
    _Out_ XPOINTF &pt,
        // Out: Point there
    _Out_ XPOINTF &vecTangent,
        // Out: Tangent vector at segment end
    _Out_ XPOINTF &vecSeg,
        // Out: Segment direction
    XINT32 fAtSegEnd
        // In: At segment end if TRUE
    ) const
{
    if (m_uCurrentIndex >= m_cSegments)
    {
        // We have started a segment-buffer but never had a chance to add anything
        // the first point (which is captured as the first segment in the buffer)
        ASSERT(1 == m_uCurrentIndex);
        ASSERT(1 == m_cSegments);

        pt = m_rgSegments[0].m_ptEnd;
        vecTangent = vecSeg = m_rgSegments[0].m_vecSeg;
    }
    else
    {
        vecTangent = m_rgSegments[m_uCurrentIndex].m_vecTangent;
        vecSeg = m_rgSegments[m_uCurrentIndex].m_vecSeg;

        if (fAtSegEnd    ||    rLoc > m_rgSegments[m_uCurrentIndex].m_rLength)
            pt = m_rgSegments[m_uCurrentIndex].m_ptEnd;
        else
        {
            rLoc -= m_rgSegments[m_uCurrentIndex-1].m_rLength;
            if (rLoc < 0)
                rLoc = 0;
            pt = m_rgSegments[m_uCurrentIndex-1].m_ptEnd + vecSeg * rLoc;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
// Implementation of CDashSequence

// Constructor
CDasher::CDashSequence::CDashSequence()
{
    m_uCurrentIndex = 1;
    m_rFactor = 1;
    m_rInverseFactor = 1;
    m_rCurrentLoc = 0;
    m_rEdgeSpace0 = 0;
    m_uCurrentIteration = 0;
    m_uStartIndex = 1;
    m_rLength = 0;
    m_rgDashes = NULL;
    m_cDashes = 0;
}

// Destructor
CDasher::CDashSequence::~CDashSequence()
{
    delete [] m_rgDashes;
}

//-----------------------------------------------------------------------------
//
// Function Description:
//
// Set from a dash lengths array
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT
CDasher::CDashSequence::Set(
    _In_ const CPlainPen &pen
        // In: The stroking pen
    )
{
    XUINT32 count = pen.GetDashCount();
    XUINT32 i;

    XFLOAT rPenWidth = MAX(XcpAbsF(pen.GetWidth()), XcpAbsF(pen.GetHeight()));
    XFLOAT rDashOffset = pen.GetDashOffset() * rPenWidth;

    if (count < 2  ||  0 != (count & 1))
    {
        IFC_RETURN(E_INVALIDARG);
    }

    // The working representation is an array of count+1 entries. The first entry is
    // -offset, and the rest are accumulative length from there, all multiplied by
    // pen-width.

    ASSERT(0 == m_cDashes);
    ASSERT(NULL == m_rgDashes);

    m_rgDashes = new XFLOAT[count+1];

    m_cDashes = count+1;

    // Initially the dash sequence starts at 0.
    m_rgDashes[0] = 0;
    for (i = 0;   i < count;  i++)
    {
        m_rgDashes[i+1] = m_rgDashes[i] + pen.GetDash(i) * rPenWidth;
    }
    if (_isnan(m_rgDashes[count]))
    {
        IFC_RETURN(E_FAIL);
    }

    if (m_rgDashes[count] < MIN_DASH_ARRAY_LENGTH)
    {
        // To avoid an infinite loop when rendering dashes, scale all
        // the dashes up so the dash array takes up MIN_DASH_ARRAY_LENGTH.

        XFLOAT rScale = MIN_DASH_ARRAY_LENGTH/m_rgDashes[count];

        for (i = 0;   i < count-1;  i++)
        {
            // NaNs get clamped to m_rgDashes[i].
            m_rgDashes[i+1] = ClampReal(m_rgDashes[i+1]*rScale, m_rgDashes[i], MIN_DASH_ARRAY_LENGTH);
        }

        m_rgDashes[count] = MIN_DASH_ARRAY_LENGTH;
    }
    m_rLength = m_rgDashes[count];

    // Make sure the dash offset lies within the dash-sequence interval
    if (!(0 <= rDashOffset  &&  rDashOffset < m_rLength))
    {
        rDashOffset = XcpModF(static_cast<XFLOAT>(rDashOffset), static_cast<XFLOAT>(m_rLength));
        if (!(0 <= rDashOffset  &&  rDashOffset < m_rLength))
        {
            // GpModF failed, probably because rDashOffset is NaN
            rDashOffset = 0;
        }
    }
    ASSERT((0 <= rDashOffset)  &&  (rDashOffset < m_rLength));

    // Find the end of the dash/space that contains the offset
    for (m_uStartIndex = 1;
         (m_uStartIndex < count)  &&  (m_rgDashes[m_uStartIndex] < rDashOffset);
         m_uStartIndex++);

    // Now shift the dashes by the dash-offset to make 0 the starting point
    for (i = 0;   i <= count;    i++)
    {
        m_rgDashes[i] -= rDashOffset;
        ASSERT( (i == 0) || (m_rgDashes[i] >= m_rgDashes[i-1]) );
    }

    // Ordinarily, this will get set during Dasher.BeginFigure(),
    // but in case a figure is never started and we're asked to close
    // it (think the degenerate line-segment case), we need to be prepared.
    m_uCurrentIndex = m_uStartIndex;

    // Sanity check
    ASSERT(0 < m_uStartIndex &&  m_uStartIndex <= count);
    ASSERT(m_rgDashes[m_uStartIndex - 1] <= 0);
    ASSERT(m_rgDashes[m_uStartIndex] >= 0);

    return S_OK;
}

//+-----------------------------------------------------------------------------
//
//  Member:   CDasher::CDashSequence::Increment
//
//  Synopsis: Increment to the next dash or gap
//
//------------------------------------------------------------------------------
void
CDasher::CDashSequence::Increment()
{
    m_rCurrentLoc = m_rgDashes[m_uCurrentIndex];
    m_uCurrentIndex++;

    if (m_uCurrentIndex >= m_cDashes)
    {
        m_uCurrentIndex = 1;
        m_uCurrentIteration++;
        m_rCurrentLoc = m_rgDashes[0];
    }
}

//-----------------------------------------------------------------------------
//
// Function Description:
//
// Reset the dash sequence
//
//-----------------------------------------------------------------------------
void
CDasher::CDashSequence::Reset()
{
    // Reset to pristine state
    m_uCurrentIndex = m_uStartIndex;
    m_rCurrentLoc = 0;
}

////////////////////////////////////////////////////////////////////////////////
// Implementation of CDasher
CDasher::CDasher(
    _In_ CPen *pPen,
        // In: The internal widening pen
    _In_reads_opt_(1) const CMILMatrix *pMatrix)
        // In: Transformation matrix (NULL OK)
    :m_pPen(pPen), m_eDashCap(XcpPenCapFlat),
     m_fIsPenDown(FALSE), m_fIsFirstCapPending(FALSE), m_oSegments(pMatrix)
{
    ASSERT(pPen);
}

//-----------------------------------------------------------------------------
//
// Function Description:
//
// Initialize the dashing of a figure
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT
CDasher::Set(
    _In_ const CPlainPen &pen
        // In: The stroking pen
    )
{
    m_eDashCap = pen.GetDashCap();

    IFC_RETURN(m_oDashes.Set(pen));

    return S_OK;
}

//-----------------------------------------------------------------------------
//
// Function Description:
//
// Initialize the dashing of a figure
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT
CDasher::StartFigure(
    _In_ const XPOINTF &pt,
        // In: Figure's firts point
    _In_ const XPOINTF &vec,
        // In: First segment's direction vector
    XINT32 fClosed,
        // In: =TRUE if we're starting a closed figure
    XcpPenCap eCapType
        // In: The start cap type
    )
{
    m_oDashes.Reset();

    m_fIsPenDown = m_oDashes.IsOnDash();
    m_fIsFirstCapPending = FALSE;

    if (m_fIsPenDown)
    {
        if (fClosed)
        {
            // The first dash will abut the last dash with flat caps on both
            m_fIsFirstCapPending = TRUE;
            eCapType = XcpPenCapFlat;
        }
        IFC_RETURN(m_pPen->StartFigure(pt, vec, FALSE, eCapType));
    }

    IFC_RETURN(m_oSegments.StartWith(pt, vec));

    return S_OK;
}

//-----------------------------------------------------------------------------
//
// Function Description:
//
// Accept a point on a line segment
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT
CDasher::AcceptLinePoint(
    _In_ const XPOINTF &point
        // In: Point to draw to
    )
{
    RRETURN(m_oSegments.Add(point));
}

//-----------------------------------------------------------------------------
//
// Function Description:
//
// Accept a point on a line segment
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT
CDasher::AcceptCurvePoint(
    _In_ const XPOINTF &point,
        // In: The point
    _In_ const XPOINTF &vecTangent,
        // In: The tangent there
    XINT32 fLast
        // In: Is this the last point on the curve?
    )
{
    RRETURN(m_oSegments.Add(point, &vecTangent));
}

//-----------------------------------------------------------------------------
//
// Function Description:
//
// Handle a corner between 2 lines/Beziers
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT
CDasher::DoCorner(
    _In_ const XPOINTF &pt,
        // Corner center point
    _In_ const XPOINTF &vecIn,
        // Vector in the direction coming in
    _In_ const XPOINTF &vecOut,
        // Vector in the direction going out
    XcpLineJoin eLineJoin,
        // Corner type
    XINT32 fSkipped,
        // =TRUE if this corner straddles a degenerate segment
    XINT32 fRound,
        // Enforce rounded corner if TRUE
    XINT32 fClosing
        // This is the last corner in a closed figure if TRUE
    )
{
    if (!fRound)
    {
        // Lay out the dashes on the edge that ends at this corner
        IFC_RETURN(Flush(FALSE));

        if (m_fIsPenDown)
        {
            // Let the pen draw the corner
            IFC_RETURN(m_pPen->DoCorner(pt, vecIn, vecOut, eLineJoin, fSkipped, fRound, fClosing));
        }

        // Start accumlating segments on the edges that starts at this corner
        IFC_RETURN(m_oSegments.StartWith(pt, vecOut));
    }

    return S_OK;
}
//+-----------------------------------------------------------------------------
//
//  Member:   CSimplePen::EndStrokeOpen
//
//  Synopsis: End an a stroke as a open
//
//------------------------------------------------------------------------------
_Check_return_ HRESULT
CDasher::EndStrokeOpen(
    XINT32 fStarted,
        // Ignored here
    _In_ const XPOINTF &ptEnd,
        // Figure's endpoint
    _In_ const XPOINTF &vecEnd,
        // Direction vector there
    XcpPenCap eEndCap,
        // The type of the end cap
    XcpPenCap eStartCap
        // Ignored here
    )
{
    IFC_RETURN(Flush(TRUE));

    if (m_fIsPenDown || (!fStarted && m_oDashes.IsOnDash()))
    {
        // Let the pen cap the current dash its choice of cap
        IFC_RETURN(m_pPen->EndStrokeOpen(fStarted, ptEnd, vecEnd, eEndCap, eStartCap));
    }
    return S_OK;
}
//+-----------------------------------------------------------------------------
//
//  Member:   CSimplePen::EndStrokeClosed
//
//  Synopsis: End an a stroke as a closed
//
//------------------------------------------------------------------------------
_Check_return_ HRESULT
CDasher::EndStrokeClosed(
    _In_ const XPOINTF &ptEnd,
        // Figure's endpoint
    _In_ const XPOINTF &vecEnd)
        // Direction vector there
{
    IFC_RETURN(Flush(!m_fIsFirstCapPending));

    if (m_fIsPenDown)
    {
        if (m_fIsFirstCapPending)
        {
            // The first dash is waiting with a flat start cap for the last dash to abut it
            IFC_RETURN(m_pPen->EndStrokeClosed(ptEnd, vecEnd));
        }
        else
        {
            // The stroke must have started with a gap, so cap this dash with a dash cap
            IFC_RETURN(m_pPen->EndStrokeOpen(TRUE, ptEnd, vecEnd, m_eDashCap));
        }
    }
    else if (m_fIsFirstCapPending && (XcpPenCapFlat != m_eDashCap))
    {
        //
        // The first dash is waiting with a flat start cap for the last dash to complete it
        // but there is no last dash. The dash-cap is not flat, so we need to append a
        // 0 length dash with the correct cap to the first dash.
        //
        IFC_RETURN(m_pPen->StartFigure(ptEnd, vecEnd, FALSE, m_eDashCap));
        IFC_RETURN(m_pPen->EndStrokeClosed(ptEnd, vecEnd));
    }

    return S_OK;
}

//-----------------------------------------------------------------------------
//
// Function Description:
//
// Start a new dash
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT
CDasher::StartANewDash(
    XFLOAT rLoc,
        // In: The location
    XFLOAT rLength,
        // In: The dash's length
    XINT32 fAtVertex
        // In: =TRUE if we're at a segment start or end
    )
{
    ASSERT(!m_fIsPenDown);  // Should not be callled otherwise

    XPOINTF pt, vecTangent, vecSeg;
    m_oSegments.ProbeAt(rLoc, pt, vecTangent, vecSeg, fAtVertex);

    m_fIsPenDown = TRUE;

    IFC_RETURN(m_pPen->StartFigure(pt, vecSeg, FALSE, m_eDashCap));

    return S_OK;
}

//-----------------------------------------------------------------------------
//
// Function Description:
//
// Extend the current dash
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT
CDasher::ExtendCurrentDash(
    XFLOAT rLoc,
        // In: The location
    XINT32 fAtVertex
        // In: =TRUE if we're at a segment start or end
    )
{
    XPOINTF pt, vecTangent, vecSeg;

    ASSERT(m_fIsPenDown);   // Should not be callled otherwise

    m_oSegments.ProbeAt(rLoc, pt, vecTangent, vecSeg, fAtVertex);
    if (m_oSegments.IsAtALine())
    {
        IFC_RETURN(m_pPen->AcceptLinePoint(pt));
    }
    else
    {
        //
        // FUTURE-We might get slightly more aesthetic
        // output here if we passed in vecTangent instead of vecSeg. The
        // difference is really only distinguishable for gigantic pens,
        // though, and it would require us to keep track of Bezier segment
        // ends to ensure that we don't get seams when two curves abut. See
        // logic in CPen::AcceptCurvePoint for details.
        //

        IFC_RETURN(m_pPen->AcceptCurvePoint(pt, vecSeg));
    }

    return S_OK;
}

//-----------------------------------------------------------------------------
//
// Function Description:
//
// End the current dash
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT
CDasher::TerminateCurrentDash(
    XFLOAT rLoc,
        // In: The location
    XINT32 fAtVertex
        // In: =TRUE if we're at a segment start or end
    )
{
    XPOINTF pt, vecTangent, vecSeg;

    ASSERT(m_fIsPenDown);   // Should not be callled otherwise

    m_oSegments.ProbeAt(rLoc, pt, vecTangent, vecSeg, fAtVertex);
    if (m_oSegments.IsAtALine())
    {
        IFC_RETURN(m_pPen->AcceptLinePoint(pt));
    }
    else
    {
        //
        // FUTURE-We might get slightly more aesthetic
        // output here if we passed in vecTangent instead of vecSeg. The
        // difference is really only distinguishable for gigantic pens,
        // though, and it would require us to keep track of Bezier segment
        // ends to ensure that we don't get seams when two curves abut. See
        // logic in CPen::AcceptCurvePoint for details.
        //
        // Note that in any case, the vecSeg in EndStrokeOpen below should
        // match what we use here.
        //

        IFC_RETURN(m_pPen->AcceptCurvePoint(pt, vecSeg));
    }

    IFC_RETURN(m_pPen->EndStrokeOpen(TRUE, pt, vecSeg, m_eDashCap));
    m_fIsPenDown = FALSE;

    return S_OK;
}

//+-----------------------------------------------------------------------------
//
//  Member:   CDasher::Flush
//
//  Synopsis: Process the segments buffer at the end of an edge
//
//  Notes:    This method emits the dashes along the polygonal piece stored in
//            the segments buffer, and then empties the buffer.
//------------------------------------------------------------------------------
_Check_return_ HRESULT
CDasher::Flush(
    XINT32 fLastEdge
        // =TRUE if this is the figure's last edge
    )
{
    HRESULT hr = S_OK;
    XINT32 fDone = false;
    XINT32 fIsOnDash = m_oDashes.IsOnDash();

    if (m_oSegments.IsEmpty())
    {
        goto Cleanup;
    }

    // If a corner is right on the transition boundary between dash and gap,
    // we may need to update our pen state.
    if (fIsOnDash != m_fIsPenDown)
    {
        // synchronise the state of PenDown with that of IsOnDash
        if (!m_fIsPenDown)  // we should be on a dash but the pen is not down
        {
            IFC(StartANewDash(0.0, m_oDashes.GetStep(), FALSE));
        }
        else
        {
            // we should be on a gap but the pen is down
            IFC(TerminateCurrentDash(0.0, FALSE));
        }
    }

    m_oDashes.PrepareForNewEdge();

    do
    {
        XFLOAT rDashEnd = m_oDashes.GetNextEndpoint();
        XFLOAT rSegEnd = m_oSegments.GetCurrentEnd();

        // Arbitrate the next location between dashes and segments (shorter step wins).
        fIsOnDash = m_oDashes.IsOnDash();
        if (m_oSegments.IsLast()  &&  (XcpAbsF(rDashEnd - rSegEnd) < MIN_DASH_ARRAY_LENGTH))
        {
            // Special treatment for the case where dash and segment ends coincide.

            if (m_fIsPenDown)
            {
                IFC(ExtendCurrentDash(rSegEnd, TRUE));
            }

            IFC(DoDashOrGapEndAtEdgeEnd(fLastEdge, fIsOnDash));
            break;
        }
        else if (rDashEnd > rSegEnd)
        {
            // The current dash/gap goes beyond the end of the current segment,
            // so we step to the end of the segment within the current dash/gap

            if (m_fIsPenDown)
            {
                IFC(ExtendCurrentDash(rSegEnd, TRUE));
            }

            fDone = m_oSegments.Increment();
            if (!fDone  &&  m_oSegments.IsAtALine())
            {
                IFC(m_pPen->UpdateOffset(m_oSegments.GetCurrentDirection()));
            }

            m_oDashes.AdvanceTo(rSegEnd);
        }
        else
        {
            // The current segment goes beyond the end of the current dash/gap,
            // so we step to the end of the dash/gap within the current segment
            if (m_fIsPenDown)   // The pen is down
            {
                if (fIsOnDash)  // We're at the end of a dash
                {
                    IFC(TerminateCurrentDash(rDashEnd, FALSE));
                }
            }
            else if (!fIsOnDash)    // We're at the end of a gap
            {
                IFC(StartANewDash(rDashEnd, m_oDashes.GetLengthOfNextDash(), FALSE));
            }

            m_oDashes.Increment();
        }
    }
    while (!fDone);

Cleanup:
    // Reset the buffer
    m_oSegments.Reset();

    RRETURN(hr);
}
//+-----------------------------------------------------------------------------
//
//  Member:   CDasher::DoDashOrGapEndAtEdgeEnd
//
//  Synopsis: Handle an end of a dash/gap that coincide with the end of an edge
//
//------------------------------------------------------------------------------
_Check_return_ HRESULT
CDasher::DoDashOrGapEndAtEdgeEnd(
    XINT32 fLastEdge,
        // =TRUE if this is the figure's last edge
    XINT32 fIsOnDash
        // =TRUE if we are on a dash
    )
{
    if (fLastEdge)
    {
        // We're at a figure's end
        if (!fIsOnDash)
        {
            // We're at the end of a gap.  It is preferable to view it as the start
            // of a dash, and let the figure cap it with the pen's line cap
            IFC_RETURN(StartANewDash(m_oSegments.GetLength(), 0.0 /* zero-length dash */, TRUE));
        }
    }
    else
    {
        // We're at a corner
        if (fIsOnDash)
        {
            // We're at a corner at the end of a dash.  If the dash turns the corner
            // it will terminate immediately after that.  With flat or triangle dash
            // caps that may look pretty bad, so we terminate the dash here.
            IFC_RETURN(TerminateCurrentDash(m_oSegments.GetLength(), TRUE));
        }
        // else we're at a corner at the end of a gap. For the same reason we avoid
        // starting a new dash here, and let it happen after we turn the corner.
        m_oDashes.Increment();
    }

    return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
// Implementation of CSegment classes
//-----------------------------------------------------------------------------
//
// Function Description:
//
//   Construct
//
//-----------------------------------------------------------------------------
void
CWidenLineSegment::Set(
    double rStart,
        // Start parameter
    double rEnd,
        // End parameter
    _Inout_ XPOINTF &ptFirst,
        // First point, transformed, possibly modified here
    _In_ const XPOINTF &ptLast,
        // Last point  (raw)
    _In_reads_opt_(1) const CMILMatrix *pMatrix
        // Transformation matrix (NULL OK)
    )
{
    ASSERT(0 <= rStart);
    ASSERT(rStart < rEnd);
    ASSERT(rEnd <= 1);

    m_ptEnd = ptLast;
    if (pMatrix)
    {
        pMatrix->Transform(&m_ptEnd, &m_ptEnd, 1);
    }

    m_vecDirection = m_ptEnd - ptFirst;

    if (rEnd < 1.0)
    {
        m_ptEnd = ptFirst + m_vecDirection * rEnd;
    }

    if (rStart > 0)
    {
        ptFirst += m_vecDirection * rStart;
    }
}

//-----------------------------------------------------------------------------
//
// Function Description:
//
//   CWidenLineSegment::Flatten
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT
CWidenLineSegment::Widen(
    _Out_ XPOINTF &ptEnd,
        // End point
    _Out_ XPOINTF &vecEnd
        // End direction vector
    )
{
    ptEnd = m_ptEnd;
    vecEnd = m_vecDirection;
    RRETURN(m_pTarget->AcceptLinePoint(ptEnd));
}

//-----------------------------------------------------------------------------
//
// Function Description:
//
//   CWidenLineSegment tangent
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT
CWidenLineSegment::GetFirstTangent(
    _Out_ XPOINTF &vecTangent
        // Out: Nonzero direction vector
    ) const
{
    vecTangent = m_vecDirection;
    if (m_vecDirection * m_vecDirection < m_rFuzz)
    {
        RRETURN(E_FAIL);
    }
    else
    {
        return S_OK;
    }
}
////////////////////////////////////////////////////////////////////////////////////
// Implementation of CCubicSegment
// CCubicSegment represents a Bezier segment for widening

//+-----------------------------------------------------------------------------
//
//  Member:   CCubicSegment::Set
//
//  Synopsis: Set for widening the current segment
//
//------------------------------------------------------------------------------
void
CCubicSegment::Set(
    double rStart,
        // Start parameter
    double rEnd,
        // End parameter
    _Inout_ XPOINTF &ptFirst,
        // First point, transformed, possibly modified here
    _In_reads_(3) const XPOINTF *ppt,
        // The rest of the points (raw)
    _In_reads_opt_(1) const CMILMatrix *pMatrix
        // Transformation matrix (NULL OK)
    )
{
    ASSERT(ppt);
    ASSERT(0 <= rStart);
    ASSERT(rStart < rEnd);
    ASSERT(rEnd <= 1);
    m_oBezier.SetPoints(rStart, rEnd, ptFirst, ppt, pMatrix);
    if (rStart > 0)
    {
        ptFirst = m_oBezier.GetFirstPoint();
    }
}

//-----------------------------------------------------------------------------
//
// Function Description:
//
//   CCubicSegment::Widen
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT
CCubicSegment::Widen(
    _Out_ XPOINTF &ptEnd,
        // End point
    _Out_ XPOINTF &vecEnd
        // End direction vector
    )
{
    // Flatten the original curve with tangents
    IFC_RETURN(m_oBezier.Flatten(TRUE));
    ptEnd = m_oBezier.GetLastPoint();
    vecEnd = m_oBezier.GetLastTangent();

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      (ctor) - Create a new geoemtry widener that implements a geometry
//      sink.
//
//------------------------------------------------------------------------
CWidenerSink::CWidenerSink(
    _In_opt_ const CMILMatrix* pMatrix,
    XFLOAT tolerance
    )
    : CWidener(pMatrix, tolerance)
    , m_hr(S_OK)
{
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      (dtor) - Clean up widener sink.
//
//------------------------------------------------------------------------
CWidenerSink::~CWidenerSink(
    )
{
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Begin a new figure.
//
//------------------------------------------------------------------------
void
CWidenerSink::BeginFigure(
    const XPOINTF& startPoint,
    bool fIsHollow
    )
{
    //
    // m_fShouldPenBeDown indicates if we are at a segment that is SUPPOSED to
    // be widened.
    //
    // m_fIsPenDown indicates if the current widening stretch has
    // ACTUALLY started, i.e. we're not in a gap and the current segment is
    // non-degenerate.
    //
    m_fIsPenDown = FALSE;
    m_fSkippedFirst = FALSE;

    m_fShouldPenBeDown = FALSE;
    m_startedWithoutCap = FALSE;
    m_lastSegmentWasAGap = FALSE;

    m_lastVertexProcessed = startPoint;
    m_inputFigureStartPoint = startPoint;

    m_firstFigureSegmentReached = FALSE;
    m_firstFigureSegmentWasAGap = FALSE;

    m_figureContainsGap = FALSE;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      End the current figure.
//
//------------------------------------------------------------------------
void
CWidenerSink::EndFigure(
    bool fIsClosed
    )
{
    HRESULT hr = S_OK;

    bool endedWithGap;

    bool startAndEndAbutting;

    if (fIsClosed && (m_inputFigureStartPoint.x != m_lastVertexProcessed.x || m_inputFigureStartPoint.y != m_lastVertexProcessed.y))
    {
        IFC(AddSegments(
            &m_inputFigureStartPoint,
            1, // numPoints
            1, // pointsPerSegment
            1  // numSegments
            ));
    }

    endedWithGap = !m_fShouldPenBeDown;

    startAndEndAbutting = fIsClosed && !m_firstFigureSegmentWasAGap && !endedWithGap;

    if (m_fShouldPenBeDown)
    {
        if (startAndEndAbutting && m_fIsPenDown)
        {
            //
            // Do figure closing corner
            //
            IFC(m_pTarget->DoCorner(
                m_ptStart,
                m_vecIn,
                m_vecStart,
                m_eLineJoin,
                m_fSkipped  || m_fSkippedFirst,
                JoinIsSmooth(),
                true));

            IFC(m_pTarget->EndStrokeClosed(m_ptStart, m_vecStart));
        }
        else if (!m_fIsPenDown && fIsClosed && !m_figureContainsGap)
        {
            //
            // No gaps, handled as a closed stroke, but widening never
            // started. This means that the figure is degenerate, so just
            // draw a single point with round caps.  Achieved by ending it
            // as an open stroke that never started.
            //

            IFC(m_pTarget->EndStrokeOpen(
                    false,               // Never started
                    m_pt,                // Current point
                    m_vecIn,             // To be ignored, since not started
                    XcpPenCapRound,// End cap
                    XcpPenCapRound // Start cap
                ));
        }
        else
        {
            XcpPenCap startCap = m_figureContainsGap ? m_eDashCap : m_eStartCap;

            XcpPenCap endCap = (fIsClosed && m_firstFigureSegmentWasAGap) ? m_eDashCap : m_eEndCap;

            IFC(m_pTarget->EndStrokeOpen(
                m_fIsPenDown,
                m_pt,
                m_vecIn,
                endCap,
                startCap
                ));
        }
    }

    //
    // We purposefully delayed adding a start cap to the figure. Determine
    // whether we need one and add it. We do this by starting and ending a
    // degenerate figure.
    //

    if (m_startedWithoutCap)
    {
        ASSERT(!m_firstFigureSegmentWasAGap);

        XcpPenCap startCap;

        if (fIsClosed)
        {
            if (!endedWithGap)
            {
                startCap = XcpPenCapFlat;
            }
            else
            {
                startCap = m_eDashCap;
            }
        }
        else
        {
            //
            // Figure is open.
            //

            startCap = m_eStartCap;
        }

        if (startCap != XcpPenCapFlat)
        {
            IFC(m_pTarget->StartFigure(
                m_ptStart,
                m_vecStart,
                false, // fInputFigureStart
                startCap
                ));

            IFC(m_pTarget->EndStrokeClosed(m_ptStart, m_vecStart));
        }
    }

Cleanup:
    if (FAILED(hr) && SUCCEEDED(m_hr))
    {
        m_hr = hr;
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Add an arc to the figure.
//
//------------------------------------------------------------------------
void
CWidenerSink::AddArc(
    const XPOINTF& point,
    const XSIZEF& size,
    XFLOAT rotationAngle,
    bool fIsClockwise,
    bool fIsLargeArc
)
{
    //
    // Turn the arc into up to 4 Beziers.
    //
    XPOINTF bezierPoints[12];
    XINT32 curveCount = 0;

    ArcToBezier(m_lastVertexProcessed.x, m_lastVertexProcessed.y, size.width, size.height, rotationAngle, fIsLargeArc, fIsClockwise, point.x, point.y, bezierPoints, &curveCount);

    if (curveCount > 0)
    {
        //
        // Push Beziers to the sink.
        //
        AddBeziers(bezierPoints, curveCount * 3);
    }
    else
    {
        //
        // The arc is a line as it has no curves.
        //
        AddLine(point);
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Add a Bezier curve to the figure.
//
//------------------------------------------------------------------------
void
CWidenerSink::AddBezier(
    const XPOINTF& controlPoint1,
    const XPOINTF& controlPoint2,
    const XPOINTF& endPoint
    )
{
    XPOINTF bezier[] =
    {
        controlPoint1,
        controlPoint2,
        endPoint
    };

    AddBeziers(bezier, ARRAY_SIZE(bezier));
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Add a line to the figure.
//
//------------------------------------------------------------------------
void
CWidenerSink::AddLine(
    const XPOINTF& point
    )
{
    AddLines(&point, 1);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Add a quadratic Bezier to the figure.
//
//------------------------------------------------------------------------
void
CWidenerSink::AddQuadraticBezier(
    const XPOINTF& controlPoint,
    const XPOINTF& endPoint
    )
{
    XPOINTF points[2] =
    {
        controlPoint,
        endPoint
    };

    AddQuadraticBeziers(points, ARRAY_SIZE(points));
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Add multiple lines to the figure.
//
//------------------------------------------------------------------------
void
CWidenerSink::AddLines(
    _In_reads_(pointsCount) const XPOINTF *pPoints,
    XUINT32 pointsCount
    )
{
    HRESULT hr = S_OK;

    IFC(AddSegments(pPoints,
        pointsCount,
        1, // points per segment
        pointsCount // numSegments
        ));

Cleanup:
    if (FAILED(hr) && SUCCEEDED(m_hr))
    {
        m_hr = hr;
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Add multiple Bezier curves to the figure.
//
//------------------------------------------------------------------------
void
CWidenerSink::AddBeziers(
    _In_reads_(uiCount) const XPOINTF* pPoints,
    XUINT32 uiCount
    )
{
    HRESULT hr = S_OK;

    IFC(AddSegments(
        pPoints,
        uiCount, // numPoints
        3, // points per segment
        uiCount / 3 // numSegments
        ));

Cleanup:
    if (FAILED(hr) && SUCCEEDED(m_hr))
    {
        m_hr = hr;
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Add multiple quadratic Beziers to the figure.
//
//------------------------------------------------------------------------
void
CWidenerSink::AddQuadraticBeziers(
    _In_reads_(uiCount) const XPOINTF *pPoints,
    XUINT32 uiCount
    )
{
    HRESULT hr = S_OK;
    XPOINTF bezierPoints[3];
    const XPOINTF* pCurrentBezier = pPoints;

    for (XUINT32 i = 0; i < uiCount / 2; ++i)
    {
        //
        // By the degree-elevation formula for Bezier curves the cubic Bezier
        // points of this quadratic Bezier curve are:
        //      pt0
        //      (1 / 3) * pt0 + (2 / 3) * pt1
        //      (2 / 3) * pt1 + (1 / 3) * pt2
        //      pt2
        //

        bezierPoints[0].x = (1 / 3.0f) * m_lastVertexProcessed.x + (2 / 3.0f) * pCurrentBezier[0].x;
        bezierPoints[0].y = (1 / 3.0f) * m_lastVertexProcessed.y + (2 / 3.0f) * pCurrentBezier[0].y;
        bezierPoints[1].x = (2 / 3.0f) * pCurrentBezier[0].x + (1 / 3.0f) * pCurrentBezier[1].x;
        bezierPoints[1].y = (2 / 3.0f) * pCurrentBezier[0].y + (1 / 3.0f) * pCurrentBezier[1].y;
        bezierPoints[2].x = pCurrentBezier[1].x;
        bezierPoints[2].y = pCurrentBezier[1].y;

        IFC(AddSegments(bezierPoints,
            ARRAY_SIZE(bezierPoints),
            3, // points per segment
            1 // numSegments
            ));

        pCurrentBezier += 2;
    }

Cleanup:
    if (FAILED(hr) && SUCCEEDED(m_hr))
    {
        m_hr = hr;
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Set the fill mode for the figure.
//
//------------------------------------------------------------------------
void
CWidenerSink::SetFillMode(
    GeometryFillMode fillMode
    )
{
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Close the sink.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CWidenerSink::Close(
)
{
    RRETURN(m_hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Add line or Bezier segments to the figure.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CWidenerSink::AddSegments(
    _In_reads_(numPoints) const XPOINTF* pPoints,
    XUINT32 numPoints,
    XUINT32 pointsPerSegment,
    XUINT32 numSegments
    )
{
    ASSERT(numPoints == pointsPerSegment * numSegments);

    if (numSegments > 0)
    {
        if (SegmentIsGap())
        {
            IFC_RETURN(DoGap());

            if (!m_firstFigureSegmentReached)
            {
                m_firstFigureSegmentWasAGap = TRUE;
            }

            m_figureContainsGap = TRUE;

            m_lastVertexProcessed = pPoints[numPoints - 1];
        }
        else
        {
            for (XUINT32 i = 0; i < numPoints; i += pointsPerSegment)
            {
                ASSERT(i + pointsPerSegment <= numPoints);

                IFC_RETURN(DoSegment(
                    pPoints + i,
                    pointsPerSegment
                    ));

                m_lastVertexProcessed = pPoints[i + pointsPerSegment - 1];
            }
        }

        m_firstFigureSegmentReached = TRUE;
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Set for widening the current segment.
//
//------------------------------------------------------------------------
void
CWidenerSink::SetSegmentForWidening(
    _Inout_ XPOINTF& ptFirst,
    _In_reads_(numPoints) const XPOINTF *pPoints,
    XUINT32 numPoints,
    _In_opt_ const CMILMatrix* pMatrix
    )
{
    if (numPoints == 3)
    {
        //TODO: HITTESTING: Unused parameters in base class for trim?
        m_oCubic.Set(0, 1, ptFirst, pPoints, pMatrix);
        m_pSegment = &m_oCubic;
    }
    else
    {
        ASSERT(numPoints == 1);
        //TODO: HITTESTING: Unused parameters in base class for trim?
        m_oLine.Set(0, 1, ptFirst, *pPoints, pMatrix);
        m_pSegment = &m_oLine;
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Process a segment as a gap.
//
//  NOTE:
//      If the previous segment was not a gap then it needs to be capped
//      with a dash cap.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CWidenerSink::DoGap()
{
    if (m_fShouldPenBeDown)
    {
        //
        // The previous segment was not a gap, so cap it.
        // Interior segments start and end with a dash cap
        //
        IFC_RETURN(m_pTarget->EndStrokeOpen(m_fIsPenDown, m_pt, m_vecIn, m_eDashCap));

        m_fShouldPenBeDown = FALSE;
        m_fIsPenDown = FALSE;
    }

    m_lastSegmentWasAGap = TRUE;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Process a segment to be widened.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CWidenerSink::DoSegment(
    _In_reads_(numPoints) const XPOINTF* pPoints,
    XUINT32 numPoints
    )
{
    if (!m_fShouldPenBeDown)
    {
        //
        // New figure start or after a segment gap so get the initial point.
        //
        m_pt = m_lastVertexProcessed;

        if (m_pMatrix != NULL)
        {
            m_pMatrix->Transform(&m_pt, &m_pt, 1);
        }

        m_fShouldPenBeDown = TRUE;
    }

    //
    // Set up a line or Bezier widening segment object
    //
    SetSegmentForWidening(
        m_pt,
        pPoints,
        numPoints,
        m_pMatrix
        );

    if (!SUCCEEDED(m_pSegment->GetFirstTangent(m_vecOut)))
    {
        //
        // This segment is degenerate, skip it
        //
        m_fSkipped = TRUE;

        if (!m_fIsPenDown)
        {
            m_fSkippedFirst = TRUE;
        }

        return S_OK;
    }

    if (m_fIsPenDown)
    {
        //
        // This is not the first segment so do the corner before widening it
        //
        IFC_RETURN(m_pTarget->DoCorner(
                m_pt,
                m_vecIn,
                m_vecOut,
                m_eLineJoin,
                m_fSkipped,
                JoinIsSmooth(),
                FALSE));
    }
    else
    {
        //
        // This is the beginning of a stroke - either at figure start or after
        // a gap - so start the figure before widening it.
        //

        XcpPenCap startCap;

        if (!m_lastSegmentWasAGap)
        {
            //
            // Since this is not following a gap, this must be the true start
            // of the stroke. In which case, we always use a flat start cap. We
            // will come back and add the appropriate cap at the the completion
            // of widening if necessary.
            //
            m_startedWithoutCap = TRUE;
            startCap = XcpPenCapFlat;

            //
            // Track where we began so that we know where to attach the cap.
            //
            m_ptStart = m_pt;
            m_vecStart = m_vecOut;
        }
        else
        {
            //
            // This comes after a gap. Use a dash cap.
            //
            startCap = m_eDashCap;
        }

        IFC_RETURN(m_pTarget->StartFigure(
            m_pt,
            m_vecOut,
            !m_lastSegmentWasAGap,
            startCap
            ));

        m_fIsPenDown = TRUE;
    }

    IFC_RETURN(m_pSegment->Widen(m_pt, m_vecIn));

    m_fSkipped = FALSE;
    m_lastSegmentWasAGap = FALSE;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Query if the current segment should force a smooth join.
//
//------------------------------------------------------------------------
bool
CWidenerSink::JoinIsSmooth()
{
    //TODO: HITTESTING: Implement segment flags if forcing round line join feature is added.
    return false;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Query if the current segment is a gap.
//
//------------------------------------------------------------------------
bool
CWidenerSink::SegmentIsGap()
{
    //TODO: HITTESTING: Implement segment flags if segment gap feature is added.
    return false;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      (ctor) - Create a widening sink that accumulates bounds of the
//      widened geometry.
//
//------------------------------------------------------------------------
CStrokeBoundsSink::CStrokeBoundsSink(
    )
{
    m_bounds.left = XFLOAT_MAX;
    m_bounds.right = XFLOAT_MIN;
    m_bounds.top = XFLOAT_MAX;
    m_bounds.bottom = XFLOAT_MIN;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      (dtor) - Clean up bounds sink.
//
//------------------------------------------------------------------------
CStrokeBoundsSink::~CStrokeBoundsSink(
    )
{
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Set the current left and right starting positions.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CStrokeBoundsSink::StartWith(
    _In_reads_(2) const XPOINTF *ptOffset
    )
{
    UpdateBounds(&m_bounds, ptOffset, 2);

    //
    // Start the cap.
    //
    m_currentPoints[RAIL_LEFT] = ptOffset[RAIL_LEFT];

    //
    // Start the right rail.
    //
    m_currentPoints[RAIL_RIGHT] = ptOffset[RAIL_RIGHT];

    RRETURN(S_OK);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Set the current left and right positions.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CStrokeBoundsSink::SetCurrentPoints(
    _In_reads_(2) const XPOINTF *pPoints
    )
{
    UpdateBounds(&m_bounds, pPoints, 2);

    //
    // Start the cap.
    //
    m_currentPoints[RAIL_LEFT] = pPoints[RAIL_LEFT];

    //
    // Start the right rail.
    //
    m_currentPoints[RAIL_RIGHT] = pPoints[RAIL_RIGHT];

    RRETURN(S_OK);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Extend the stroke by a quadrangle.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CStrokeBoundsSink::QuadTo(
    _In_reads_(2) const XPOINTF *ptOffset
    )
{
    UpdateBounds(&m_bounds, ptOffset, 2);

    m_currentPoints[RAIL_LEFT] = ptOffset[RAIL_LEFT];
    m_currentPoints[RAIL_RIGHT] = ptOffset[RAIL_RIGHT];

    RRETURN(S_OK);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Add a pair of points to the 2 sides of the polygon also testing
//      for kinks.
//
//  NOTE:
//      This method is called when we are widening a curve segment; there
//      is no corner handling between segments, so on a sharp turn the
//      offset may go backwards. We need to check for that and generate
//      the correct geometry if that happens.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CStrokeBoundsSink::QuadTo(
    _In_reads_(2) const XPOINTF *ptOffset,
    _In_ const XPOINTF &vecSeg,
    _In_ const XPOINTF &ptSpine,
    _In_ const XPOINTF &ptSpinePrev
    )
{
    HRESULT hr = S_OK;

    //
    // Left Rail
    //
    if ((ptOffset[RAIL_LEFT] - m_currentPoints[RAIL_LEFT]) * vecSeg < 0)
    {
        UpdateBounds(&m_bounds, &ptSpinePrev, 1);
        UpdateBounds(&m_bounds, &ptOffset[RAIL_LEFT], 1);
    }
    else
    {
        UpdateBounds(&m_bounds, &ptOffset[RAIL_LEFT], 1);
    }

    //
    // Right Rail
    //
    if ((ptOffset[RAIL_RIGHT] - m_currentPoints[RAIL_RIGHT]) * vecSeg < 0)
    {
        UpdateBounds(&m_bounds, &ptSpinePrev, 1);
        UpdateBounds(&m_bounds, &ptOffset[RAIL_RIGHT], 1);
    }
    else
    {
        UpdateBounds(&m_bounds, &ptOffset[RAIL_RIGHT], 1);
    }

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Add a curved wedge to a stroke rail.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CStrokeBoundsSink::CurveWedge(
    RAIL_SIDE side,
    _In_ const XPOINTF &ptBez_1,
    _In_ const XPOINTF &ptBez_2,
    _In_ const XPOINTF &ptBez_3
    )
{
    XPOINTF points[] =
    {
        m_currentPoints[side],
        ptBez_1,
        ptBez_2,
        ptBez_3
    };

    UpdateBezierBounds(&m_bounds, points, ARRAY_SIZE(points));

    m_currentPoints[side] = ptBez_3;

    RRETURN(S_OK);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Add a polyline wedge to a stroke rail.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CStrokeBoundsSink::PolylineWedge(
    RAIL_SIDE side,
    XUINT32 count,
    _In_reads_(count) const XPOINTF *pPoints
    )
{
    UpdateBounds(&m_bounds, pPoints, count);

    if (count > 0)
    {
        m_currentPoints[side] = pPoints[count - 1];
    }

    RRETURN(S_OK);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Add an inner corner to a rail
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CStrokeBoundsSink::DoInnerCorner(
    RAIL_SIDE side,
    _In_ const XPOINTF &ptCenter,
    _In_reads_(2) const XPOINTF *ptOffset
    )
{
    m_currentPoints[side] = ptOffset[side];

    RRETURN(S_OK);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Add a Bezier cap.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CStrokeBoundsSink::BezierCap(
    _In_ const XPOINTF &ptStart,
    _In_ const XPOINTF &pt0_1,
    _In_ const XPOINTF &pt0_2,
    _In_ const XPOINTF &ptMid,
    _In_ const XPOINTF &pt1_1,
    _In_ const XPOINTF &pt1_2,
    _In_ const XPOINTF &ptEnd
    )
{
    XPOINTF firstCurve[] =
    {
        ptStart,
        pt0_1,
        pt0_2,
        ptMid
    };

    UpdateBezierBounds(&m_bounds, firstCurve, ARRAY_SIZE(firstCurve));

    XPOINTF secondCurve[] =
    {
        ptMid,
        pt1_1,
        pt1_2,
        ptEnd
    };

    UpdateBezierBounds(&m_bounds, secondCurve, ARRAY_SIZE(secondCurve));

    RRETURN(S_OK);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Add a triangle cap.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CStrokeBoundsSink::CapTriangle(
    _In_ const XPOINTF &ptStart,
    _In_ const XPOINTF &ptApex,
    _In_ const XPOINTF &ptEnd
    )
{
    UpdateBounds(&m_bounds, &ptStart, 1);
    UpdateBounds(&m_bounds, &ptApex, 1);
    UpdateBounds(&m_bounds, &ptEnd, 1);

    RRETURN(S_OK);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Switch current point sides.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CStrokeBoundsSink::SwitchSides(
    )
{
    XPOINTF ptTemp = m_currentPoints[RAIL_LEFT];

    m_currentPoints[RAIL_LEFT] = m_currentPoints[RAIL_RIGHT];
    m_currentPoints[RAIL_RIGHT] = ptTemp;

    RRETURN(S_OK);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Get the bounds of the accumulated widened figure.
//
//------------------------------------------------------------------------
void
CStrokeBoundsSink::GetBounds(
    _Out_ XRECTF_RB* pBounds
    )
{
    //
    // Sanitize output so right >= left && bottom >= top.
    //
    if(!IsEmptyRectF(m_bounds))
    {
        *pBounds = m_bounds;
    }
    else
    {
        EmptyRectF(pBounds);
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      (ctor) - Create a widener sink that can test if the widened
//      geometry contains a point.
//
//------------------------------------------------------------------------
CStrokeHitTestSink::CStrokeHitTestSink(
    HitTestHelper& hitTestHelper
    )
    : m_hitTestHelper(hitTestHelper)
    , m_hitInside(FALSE)
{
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      (dtor) - Clean up hit testing widening sink.
//
//------------------------------------------------------------------------
CStrokeHitTestSink::~CStrokeHitTestSink(
    )
{
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Set the current left and right starting positions.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CStrokeHitTestSink::StartWith(
    _In_reads_(2) const XPOINTF *ptOffset
    )
{
    //
    // Starting the cap.
    //
    m_currentPoints[RAIL_LEFT] = ptOffset[RAIL_LEFT];

    //
    // Starting the right rail.
    //
    m_currentPoints[RAIL_RIGHT] = ptOffset[RAIL_RIGHT];

    RRETURN(S_OK);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Set the current left and right positions.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CStrokeHitTestSink::SetCurrentPoints(
    _In_reads_(2) const XPOINTF *pPoints
    )
{
    //
    // Starting the cap.
    //
    m_currentPoints[RAIL_LEFT] = pPoints[RAIL_LEFT];

    //
    // Starting the right rail.
    //
    m_currentPoints[RAIL_RIGHT] = pPoints[RAIL_RIGHT];

    RRETURN(S_OK);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Extend the stroke by a quadrangle.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CStrokeHitTestSink::QuadTo(
    _In_reads_(2) const XPOINTF *ptOffset
    )
{
    bool hitInside;

    //
    // Test the edges of the quadrangle.
    //
    m_hitTestHelper.Reset();
    m_hitTestHelper.StartAt(m_currentPoints[RAIL_RIGHT]);
    m_hitTestHelper.DoLine(m_currentPoints[RAIL_LEFT]);
    m_hitTestHelper.DoLine(ptOffset[RAIL_LEFT]);
    m_hitTestHelper.DoLine(ptOffset[RAIL_RIGHT]);
    m_hitTestHelper.DoLine(m_currentPoints[RAIL_RIGHT]);

    IFC_RETURN(m_hitTestHelper.GetResult(&hitInside));

    m_hitInside = m_hitInside || hitInside;

    //
    // Update the current points.
    //
    m_currentPoints[RAIL_LEFT] = ptOffset[RAIL_LEFT];
    m_currentPoints[RAIL_RIGHT] = ptOffset[RAIL_RIGHT];

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Add a pair of points to the 2 sides of the polygon also testing
//      for kinks.
//
//  NOTE:
//      This method is called when we are widening a curve segment; there
//      is no corner handling between segments, so on a sharp turn the
//      offset may go backwards. We need to check for that and generate
//      the correct geometry if that happens.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CStrokeHitTestSink::QuadTo(
    _In_reads_(2) const XPOINTF *ptOffset,
    _In_ const XPOINTF &vecSeg,
    _In_ const XPOINTF &ptSpine,
    _In_ const XPOINTF &ptSpinePrev
    )
{
    bool hitInside;
    XPOINTF closePoint;

    m_hitTestHelper.Reset();

    //
    // Follow the left rail.
    //
    if (((ptOffset[RAIL_LEFT] - m_currentPoints[RAIL_LEFT]) * vecSeg) < 0)
    {
        //
        // Check the cross product's Z coordinate to determine winding
        //
        XPOINTF vecP1E = ptOffset[RAIL_LEFT] - m_currentPoints[RAIL_LEFT];
        XPOINTF vecP2E = ptOffset[RAIL_LEFT] - ptSpine;
        XFLOAT  crossProdZ = (vecP1E.x * vecP2E.y) - (vecP1E.y * vecP2E.x);

        if (crossProdZ > 0)
        {
            m_hitTestHelper.StartAt(ptSpinePrev);
            closePoint = ptSpinePrev;

            m_hitTestHelper.DoLine(ptOffset[RAIL_LEFT]);
        }
        else
        {
            m_hitTestHelper.StartAt(ptOffset[RAIL_LEFT]);
            closePoint = ptOffset[RAIL_LEFT];

            m_hitTestHelper.DoLine(ptSpinePrev);
        }

        m_hitTestHelper.DoLine(m_currentPoints[RAIL_LEFT]);

        m_hitTestHelper.DoLine(ptSpine);
    }
    else
    {
        m_hitTestHelper.StartAt(m_currentPoints[RAIL_LEFT]);
        closePoint = m_currentPoints[RAIL_LEFT];
    }

    m_hitTestHelper.DoLine(ptOffset[RAIL_LEFT]);

    //
    // Follow the right rail in reverse to keep winding order.
    //
    m_hitTestHelper.DoLine(ptOffset[RAIL_RIGHT]);

    if (((ptOffset[RAIL_RIGHT] - m_currentPoints[RAIL_RIGHT]) * vecSeg) < 0)
    {
        m_hitTestHelper.DoLine(ptSpine);

        m_hitTestHelper.DoLine(m_currentPoints[RAIL_RIGHT]);

        //
        // Check the cross product's Z coordinate to determine winding
        //
        XPOINTF vecP1E = ptOffset[RAIL_RIGHT] - m_currentPoints[RAIL_RIGHT];
        XPOINTF vecP2E = ptOffset[RAIL_RIGHT] - ptSpine;
        XFLOAT  crossProdZ = (vecP1E.x * vecP2E.y) - (vecP1E.y * vecP2E.x);

        if (crossProdZ > 0)
        {
            m_hitTestHelper.DoLine(ptSpinePrev);

            m_hitTestHelper.DoLine(ptOffset[RAIL_RIGHT]);
        }
        else
        {
            m_hitTestHelper.DoLine(ptOffset[RAIL_RIGHT]);

            m_hitTestHelper.DoLine(ptSpinePrev);
        }
    }
    else
    {
        m_hitTestHelper.DoLine(m_currentPoints[RAIL_RIGHT]);
    }

    //
    // Close the geometry.
    //
    m_hitTestHelper.DoLine(closePoint);

    IFC_RETURN(m_hitTestHelper.GetResult(&hitInside));

    m_hitInside = m_hitInside || hitInside;

    //
    // Update the current points
    //
    m_currentPoints[RAIL_LEFT] = ptOffset[RAIL_LEFT];
    m_currentPoints[RAIL_RIGHT] = ptOffset[RAIL_RIGHT];

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Add a curved wedge to a stroke rail.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CStrokeHitTestSink::CurveWedge(
    RAIL_SIDE side,
    _In_ const XPOINTF &ptBez_1,
    _In_ const XPOINTF &ptBez_2,
    _In_ const XPOINTF &ptBez_3
    )
{
    bool hitInside;

    //
    // Test a pie shaped segment centered at the opposite side.
    //
    m_hitTestHelper.Reset();
    m_hitTestHelper.StartAt(m_currentPoints[OPPOSITE_SIDE(side)]);
    m_hitTestHelper.DoLine(m_currentPoints[side]);
    m_hitTestHelper.DoBezier(ptBez_1, ptBez_2, ptBez_3);
    m_hitTestHelper.DoLine(m_currentPoints[OPPOSITE_SIDE(side)]);

    IFC_RETURN(m_hitTestHelper.GetResult(&hitInside));

    m_hitInside = m_hitInside || hitInside;

    //
    // Update the current point
    //
    m_currentPoints[side] = ptBez_3;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Add a polyline wedge to a stroke rail.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CStrokeHitTestSink::PolylineWedge(
    RAIL_SIDE side,
    XUINT32 count,
    _In_reads_(count) const XPOINTF *pPoints
    )
{
    bool hitInside;

    m_hitTestHelper.Reset();

    //
    // Start at wedge point.
    //
    m_hitTestHelper.StartAt(m_currentPoints[OPPOSITE_SIDE(side)]);

    //
    // Follow wedge curve.
    //
    m_hitTestHelper.DoLine(m_currentPoints[side]);

    for (XUINT32 i = 0; i < count; i++)
    {
        m_hitTestHelper.DoLine(pPoints[i]);
    }

    //
    // Join wedge to point.
    //
    m_hitTestHelper.DoLine(m_currentPoints[OPPOSITE_SIDE(side)]);

    IFC_RETURN(m_hitTestHelper.GetResult(&hitInside));

    m_hitInside = m_hitInside || hitInside;

    //
    // Update the current point.
    //
    if (count > 0)
    {
        m_currentPoints[side] = pPoints[count-1];
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Add an inner corner to a rail
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CStrokeHitTestSink::DoInnerCorner(
    RAIL_SIDE side,
    _In_ const XPOINTF &ptCenter,
    _In_reads_(2) const XPOINTF *ptOffset
    )
{
    m_currentPoints[side] = ptOffset[side];

    RRETURN(S_OK);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Add a Bezier cap.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CStrokeHitTestSink::BezierCap(
    _In_ const XPOINTF &ptStart,
    _In_ const XPOINTF &pt0_1,
    _In_ const XPOINTF &pt0_2,
    _In_ const XPOINTF &ptMid,
    _In_ const XPOINTF &pt1_1,
    _In_ const XPOINTF &pt1_2,
    _In_ const XPOINTF &ptEnd
    )
{
    bool hitInside;

    m_hitTestHelper.Reset();
    m_hitTestHelper.StartAt(ptStart);
    m_hitTestHelper.DoBezier(pt0_1, pt0_2, ptMid);
    m_hitTestHelper.DoBezier(pt1_1, pt1_2, ptEnd);
    m_hitTestHelper.DoLine(ptStart);

    IFC_RETURN(m_hitTestHelper.GetResult(&hitInside));

    m_hitInside = m_hitInside || hitInside;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Add a triangle cap.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CStrokeHitTestSink::CapTriangle(
    _In_ const XPOINTF &ptStart,
    _In_ const XPOINTF &ptApex,
    _In_ const XPOINTF &ptEnd
    )
{
    bool hitInside;

    m_hitTestHelper.Reset();
    m_hitTestHelper.StartAt(ptStart);
    m_hitTestHelper.DoLine(ptApex);
    m_hitTestHelper.DoLine(ptEnd);

    IFC_RETURN(m_hitTestHelper.GetResult(&hitInside));

    m_hitInside = m_hitInside || hitInside;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Switch current point sides.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CStrokeHitTestSink::SwitchSides(
    )
{
    HRESULT hr = S_OK;

    m_hitTestHelper.Reset();
    m_hitTestHelper.StartAt(m_currentPoints[RAIL_LEFT]);
    m_hitTestHelper.DoLine(m_currentPoints[RAIL_RIGHT]);

    XPOINTF ptTemp(m_currentPoints[RAIL_LEFT]);
    m_currentPoints[RAIL_LEFT] = m_currentPoints[RAIL_RIGHT];
    m_currentPoints[RAIL_RIGHT] = ptTemp;

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Query if the sink wants to abort.
//
//------------------------------------------------------------------------
XINT32
CStrokeHitTestSink::Aborted(
    )
{
    return m_hitInside;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Query if the geometry contained the hit test point.
//
//------------------------------------------------------------------------
bool
CStrokeHitTestSink::WasHit(
    ) const
{
    return m_hitInside;
}
