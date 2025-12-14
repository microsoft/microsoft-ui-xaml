// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CWideningSink;

// Tolerance
const XFLOAT MIN_TOLERANCE = 1.e-6f;
const XFLOAT SQ_LENGTH_FUZZ = 1.E-4f;

// The following is a lower bound on the length of a dash sequence. We are in device
// space, so anything smaller than ~1/8 is invisible with antialiasing. Moreoever, the
// rasterizer becomes quadratic when the number of edges per sample is > 1.
#define MIN_DASH_ARRAY_LENGTH .1f

/*
We are implicitly assuming that we are working with a left handed coordinate
system (which is the most common case, with x pointing right and y pointing
down).  That's the case with XPOINTF.TurnRight(), and say that the path is
turning right when the determinant is positive.  The algorithms do not depend
on this assumption. The only effect of a right hand coordinate system is on
the orientation of the resulting path outline: It will go clockwise in a
left handed system and counterclockwise in a right handed one.
*/

enum _RAIL_SIDE
{
    RAIL_LEFT=0,
    RAIL_RIGHT=1
};

typedef _RAIL_SIDE RAIL_SIDE;

enum _RAIL_TERMINAL
{
    RAIL_START=0,
    RAIL_END=1
};

typedef _RAIL_TERMINAL RAIL_TERMINAL;

inline RAIL_SIDE TERMINAL2SIDE(RAIL_TERMINAL x) { return static_cast<RAIL_SIDE>(x); }
inline RAIL_SIDE OPPOSITE_SIDE(RAIL_SIDE x) { return static_cast<RAIL_SIDE>(1-x); }

////////////////////////////////////////////////////////////////////////////////////
// Definition and implementation of CMatrix22
class CMatrix22
{
public:
    CMatrix22()
    {
        Reset();
    }

    CMatrix22(
        _In_ const CMatrix22 &other
            // In The matrix to copy
        );

    CMatrix22(
        _In_ const CMILMatrix &oMatrix
            // In: The CMILMatrix to copy from
        )
        : m_rM11(oMatrix.GetM11()), m_rM12(oMatrix.GetM12()),
          m_rM21(oMatrix.GetM21()), m_rM22(oMatrix.GetM22())
    {
    }

    ~CMatrix22() {}

    void Reset();

    void Set(
        XFLOAT rM11,
            // In: The value to set for M11
        XFLOAT rM12,
            // In: The value to set for M12
        XFLOAT rM21,
            // In: The value to set for M21
        XFLOAT rM22
            // In: The value to set for M22
        );

    void Prepend(
        _In_opt_ const CMILMatrix *pMatrix
        );

    bool Finalize(
        XFLOAT rThreshold,
            // Lower bound for |determinant(this)|
        _Out_ CMatrix22 &oInverse
            // The inverse of this matrix
        );

    XINT32 IsIsotropic(
        _Out_ XFLOAT &rSqMax
        ) const;

    // Apply the transformation to a vector
    void Transform(
        _Inout_ XPOINTF &P
        ) const;

    void TransformColumn(
        _Inout_ XPOINTF &P
        ) const;

    void PreFlipX();

    _Check_return_ HRESULT Invert(); // Return InvalidParameter if not invertable

    void GetInverseQuadratic(
        _Out_ XFLOAT &rCxx,
            // Out: Coefficient of x*x
        _Out_ XFLOAT &rCxy,
            // Out: Coefficient of x*y
        _Out_ XFLOAT &rCyy
            // Out: Coefficient of y*y
        );

private:
    // 4 matrix entries
    XFLOAT m_rM11;
    XFLOAT m_rM12;
    XFLOAT m_rM21;
    XFLOAT m_rM22;
};

/////////////////////////////////////////////////////////////////////////////
// Definition of CPenInterface
// CPenInterface is an abstract class representing a pen or a dasher
// It extends the CFlatteningSink interface

class CPenInterface     :   public CFlatteningSink
{
public:
    virtual _Check_return_ HRESULT StartFigure(
        _In_ const XPOINTF &pt,
            // In: Figure's first point
        _In_ const XPOINTF &vec,
            // In: First segment's direction vector
        XINT32 fClosed,
            // In: =TRUE if we're starting a closed figure
        XcpPenCap eCapType
            // In: The start cap type
        )=0;

    virtual _Check_return_ HRESULT DoCorner(
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
        )=0;

    virtual _Check_return_ HRESULT EndStrokeOpen(
        XINT32 fStarted,
            // = TRUE if the widening has started
        _In_ const XPOINTF &ptEnd,
            // Figure's endpoint
        _In_ const XPOINTF &vecEnd,
            // Direction vector there
        XcpPenCap eEndCap,
            // The type of the end cap
        XcpPenCap eStartCap=XcpPenCapFlat
            // The type of start cap (optional)
        )=0;

    virtual _Check_return_ HRESULT EndStrokeClosed(
        _In_ const XPOINTF &ptEnd,
            // Figure's endpoint
        _In_ const XPOINTF &vecEnd
            // Direction vector there
        )=0;

    virtual _Check_return_ HRESULT AcceptCurvePoint(
        _In_ const XPOINTF &point,
            // In: The point
        _In_ const XPOINTF &vec,
            // In: The tangent there
        XINT32 fLast = FALSE
            // In: Is this the last point on the curve? (optional)
        )=0;

    virtual _Check_return_ HRESULT AcceptLinePoint(
        _In_ const XPOINTF &point
            // In: The point
        )=0;

    // CFlatteningSink override
    // Here is the reason for using different names for the same thing.  CPenInterface
    // distinguishes between points on a line segment and points on a curve.  On line
    // segment the direction is known, and there is no need for a tangent.  On the other
    // hand, ALL the points sent to CFlatteningSink are on a curve, whether they come
    // with or without tangents.  GetPointOnTangent is called if fWithTangent=TRUE.
    // CPenInterface happens to need tangents for points that come from a curve, so
    // the call from the flattener is routed as a point from a curve.  There should be
    // no cost for this routing.
    _Check_return_ HRESULT AcceptPointAndTangent(
        _In_ const XPOINTF &pt,
            // The point
        _In_ const XPOINTF &vec,
            // The tangent there
        XINT32 fLast
            // Is this the last point on the curve?
        ) final
    {
        RRETURN(AcceptCurvePoint(pt, vec, fLast));
    }

    virtual XINT32 Aborted()=0;
};

///////////////////////////////////////////////////////////////////
//          Definition of CSegment
/* This class abstracts the concepts of line and cubic curve.  A segment is
a parametric mapping C(t) from the interval [0,1] to the plane. For a line
the mapping is  C(t) = P + t*V, where P is a point and V is a vector. For a
cubic curve the mapping is C(t) = C0 + C1*t + C[2]*t^2 + C[3]*t^3. where
C[i] are 2D coefficients */

class CSegment
{
public:
    CSegment()
    {}

    virtual ~CSegment() {}

    virtual _Check_return_ HRESULT Widen(
        _Out_ XPOINTF &ptEnd,
            // End point
        _Out_ XPOINTF &vecEnd
            // End direction vector
        )=0;

    virtual _Check_return_ HRESULT GetFirstTangent(
        _Out_ XPOINTF &vecTangent
            // Out: Nonzero direction vector
        ) const=0;

};  // End of definition of the class CSegment

////////////////////////////////////////////////////////////////////////////
// Definition of CWidenLineSegment helper class
class CWidenLineSegment  :   public CSegment
{
public:
    CWidenLineSegment()
    {
        m_rFuzz = MIN_TOLERANCE * SQ_LENGTH_FUZZ;
        m_pTarget = NULL;
    }

    CWidenLineSegment(
        XFLOAT rTolerance
            // Widening tolerance
        )
    {
        m_pTarget = NULL;
        m_rFuzz = rTolerance * SQ_LENGTH_FUZZ;
    }

    ~CWidenLineSegment() override {}



    void SetTarget(
        _In_reads_opt_(1) CPenInterface *pTarget
        )
    {
        m_pTarget = pTarget;
    }

    void Set(
        double rStart,
            // Start parameter
        double rEnd,
            // End parameter
        _Inout_ XPOINTF &ptFirst,
            // First point, transformed, possibly modified here
        _In_ const XPOINTF &ptLast,
            // Last point (raw)
        _In_reads_opt_(1) const CMILMatrix *pMatrix
            // Transformation matrix (NULL OK)
        );

    _Check_return_ HRESULT Widen(
        _Out_ XPOINTF &ptEnd,
            // End point
        _Out_ XPOINTF &vecEnd
            // End direction vector
        ) override;

    _Check_return_ HRESULT GetFirstTangent(
        _Out_ XPOINTF &vecTangent
            // Nonzero direction vector
        ) const override;

private:
    XPOINTF m_ptEnd{};         // End point
    XPOINTF m_vecDirection{};  // Direction vector
    XFLOAT m_rFuzz;         // Zero length fuzz
    CPenInterface *m_pTarget;      // The widening target
};   // End of definition of the class CWidenLineSegment


/////////////////////////////////////////////////////////////////////////
// Definition of CCubicSegment helper class
class CCubicSegment  :   public CSegment
{
public:
    CCubicSegment()
    : m_oBezier(NULL, MIN_TOLERANCE)
    {
    }

    CCubicSegment(
        XFLOAT rTolerance
            // Widening tolerance
        )
    : m_oBezier(NULL, rTolerance)
    {
    }

    ~CCubicSegment() override {}

    void SetTarget(
        _In_reads_opt_(1) CPenInterface *pTarget
        )
    {
        m_oBezier.SetTarget(pTarget);
    }

    void Set(
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
        );

    _Check_return_ HRESULT Widen(
        _Out_ XPOINTF &ptEnd,
            // End point
        _Out_ XPOINTF &vecEnd
          // End direction vector
        ) override;

    _Check_return_ HRESULT GetFirstTangent(
        _Out_ XPOINTF &vecTangent
              // Tangent vector there
        ) const override
    {
        return m_oBezier.GetFirstTangent(vecTangent);
    }

private:
    CMILBezierFlattener     m_oBezier;
};  // End of definition of the class CCubicSegment


/////////////////////////////////////////////////////////////////////////////
// Definition of CPen

class CPen  :   public CPenInterface
{
    // Construction/destruction

public:
    CPen();

    ~CPen() override {}

    bool Set(
        _In_ const CPenGeometry &geom,
            // In: The pen's geometry information
        _In_reads_opt_(1) const CMILMatrix *pMatrix,
            // In: W to D transformation matrix (NULL OK)
        XFLOAT rTolerance
            // In: Approximation tolerance
        );

    void Copy(
        _In_ const CPen &pen
            // A pen to copy basic properties from
        );

    _Check_return_ HRESULT UpdateOffset(
        _In_ const XPOINTF &vecDirection
            // In: A nonzero direction vector
        );

    XFLOAT GetRadius() const
    {
        return m_rRadius;
    }

    _Check_return_ HRESULT ComputeRadiusVector(     // Return InvalidParameter if vecDirection=0
        _In_ const XPOINTF &vecDirection,
            // In: A not necessarily unit vector
        _Out_ XPOINTF &vecRad
            // Out: Radius vector on the pen circle
        ) const;

    void SetRadiusVector(
        _In_ const XPOINTF &vecRad
            // In: A Given radius vector
        );

    _Check_return_ HRESULT AcceptCurvePoint(
        _In_ const XPOINTF &point,
            // In: The point
        _In_ const XPOINTF &vec,
            // In: The tangent there
        XINT32 fLast = FALSE
            // In: Is this the last point on the curve?
        ) override;

    virtual _Check_return_ HRESULT RoundTo(
        _In_ const XPOINTF &vecRad,
            // In: Radius vector of the outgoing segment
        _In_ const XPOINTF &ptCenter,
            // In: Corner center point
        _In_ const XPOINTF &vecIn,
            // In: Vector in the direction coming in
        _In_ const XPOINTF &vecOut
            // In: Vector in the direction going out
        )=0;

protected:
    void GetOffsetVector(
        _In_ const XPOINTF &vecRad,
            // In: A radius vector
        _Out_ XPOINTF &vecOffset
            // Out: The corresponding offset vector
        ) const;

    XPOINTF GetPenVector(
        _In_ const XPOINTF &vecRad
            // In: A radius vector
        ) const;

    bool GetTurningInfo(
        _In_ const XPOINTF &vecIn,
            // In: Vector in the direction coming in
        _In_ const XPOINTF &vecOut,
            // In: Vector in the direction going out
        _Out_ XFLOAT &rDet,
            // Out: The determinant of the vectors
        _Out_ XFLOAT &rDot,
            // Out: The dot product of the vectors
        _Out_ RAIL_SIDE &side,
            // The outer side of the turn
        _Out_ XINT32 &f180Degrees
            // Out: =TRUE if this is a 180 degrees turn
            ) const;

    virtual _Check_return_ HRESULT ProcessCurvePoint(
        _In_ const XPOINTF &point,
            // In: Point to draw to
        _In_ const XPOINTF &vecSeg
            // In: Direction of segment
        )=0;

private:
    bool SetPenShape(
        _In_ const CPenGeometry &geom,
            // The pen's geometry information
        _In_reads_opt_(1) const CMILMatrix *pMatrix,
            // Rendering transformation (NULL OK)
        XFLOAT rThreshold
            // Lower bound on pen dimensions, below which it is considered empty
        );

    void SetThreshold(
        _In_opt_ const  CMILMatrix *pMatrix,
            // Transoformation matrix (NULL OK)
        XFLOAT rTolerance
            // Approximation tolerance
        );

protected:
    // Nominal pen data
    XcpLineJoin  m_eLineJoin;        // Line join style (TO DO: Do we need this?)

    // Derived data - fixed
    CMatrix22   m_oMatrix;            // Pen's transformation matrix
    CMatrix22   m_oInverse;           // Inverse of m_oMatrix
    CMatrix22   m_oWToDMatrix;        // The world to device matrix (ignoring translation)
    XFLOAT      m_rRadius;            // Radius in pen coordinate space
    XFLOAT      m_rRadSquared;        // The above squared
    XFLOAT      m_rNominalMiterLimit; // Nominal API miter limit
    XFLOAT      m_rMiterLimit;        // Miter limit multiplied by radius
    XFLOAT      m_rMiterLimitSquared; // The above squared
    XFLOAT      m_rThreshold;         // Flattening refinement threshold
    XINT32        m_fCircular;          // =TRUE if the pen is circular

    // Working variable data
    XPOINTF    m_vecRad;           // Current radius vector
    XPOINTF    m_vecOffset;        // Current offset vector
    XPOINTF    m_ptPrev;           // Previous point
    XPOINTF    m_vecPrev;          // Previous tangent vector
};

/////////////////////////////////////////////////////////////////////////////
// Definition of CSimplePen
// Some of the methods may be pushed up to the base class CPen.

class CSimplePen final : public CPen
{
public:
    CSimplePen()
        : m_pSink(NULL)
    {
    }

    void Initialize(
        _In_ const CPenGeometry &geom,
            // In: The pen's geometry information
        _In_ const CMILMatrix *pMatrix,
            // In: W to D transformation matrix (NULL OK)
        XFLOAT rTolerance,
            // In: Approximation tolerance
        _Inout_updates_(1) CWideningSink *pSink
            // In/out: The recipient of the results
        )
    {
        m_pSink = pSink;

        Set(geom, pMatrix, rTolerance);
    }

    // Constructor for stroking line shapes

    ~CSimplePen() override {}

    void SetFrom(
        _In_ const CPen &pen,
            // A pen to inherit some properties from
        _In_ CWideningSink *pSink
            // The recipient of the results
        )
    {
        Copy(pen);
        m_pSink = pSink;
    }

    // CPenInterface override
    _Check_return_ HRESULT StartFigure(
        _In_ const XPOINTF &pt,
            // In: Figure's firts point
        _In_ const XPOINTF &vec,
            // In: First segment's direction vector
        XINT32 fClosed,
            // In: =TRUE if we're starting a closed figure
        XcpPenCap eCapType
            // In: The start cap type
        ) override;

    _Check_return_ HRESULT DoCorner(
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
        ) override;

    _Check_return_ HRESULT EndStrokeOpen(
        XINT32 fStarted,
            // = TRUE if the widening has started
        _In_ const XPOINTF &ptEnd,
            // Figure's endpoint
        _In_ const XPOINTF &vecEnd,
            // Direction vector there
        XcpPenCap eEndCap,
            // The type of the end cap
        XcpPenCap eStartCap=XcpPenCapFlat
            // The type of start cap (optional)
        ) override;


    _Check_return_ HRESULT EndStrokeClosed(
        _In_ const XPOINTF &ptEnd,
            // Figure's endpoint
        _In_ const XPOINTF &vecEnd
            // Direction vector there
        ) override;

    _Check_return_ HRESULT AcceptLinePoint(
        _In_ const XPOINTF &point
            // In: Point to stop the pen at
        ) override;

    _Check_return_ HRESULT RoundTo(
        _In_ const XPOINTF &vecRad,
            // In: Radius vector of the outgoing segment
        _In_ const XPOINTF &ptCenter,
            // In: Corner center point
        _In_ const XPOINTF &vecIn,
            // In: Vector in the direction coming in
        _In_ const XPOINTF &vecOut
            // In: Vector in the direction going out
        ) override;

    XINT32 Aborted() override;

protected:
    // CPen overrides
    _Check_return_ HRESULT ProcessCurvePoint(
        _In_ const XPOINTF &point,
            // In: Point to draw to
        _In_ const XPOINTF &vecSeg
            // In: Direction of segment we're coming along
        ) override;

private:
    _Check_return_ HRESULT DoInnerCorner(
        RAIL_SIDE side,
            // In: The side of the inner corner
        _In_ const XPOINTF &ptCenter,
            // In: The corner's center
        const XPOINTF *ptOffset
            // In: The offset points of the new segment
        );

    _Check_return_ HRESULT DoBaseCap(
        RAIL_TERMINAL whichEnd,
            // In: START or END
        _In_ const XPOINTF &ptCenter,
            // In: Cap's center
        _In_ const XPOINTF &vec,
            // In: Vector pointing out along the segment
        XcpPenCap type
            // In: The type of cap
        );

    _Check_return_ HRESULT DoSquareCap(
        RAIL_TERMINAL whichEnd,
            // START or END
        _In_ const XPOINTF &ptCenter
            // Cap's center
        );

    _Check_return_ HRESULT DoRoundCap(
        RAIL_TERMINAL whichEnd,
            // In: START or END
        _In_ const XPOINTF &ptCenter
            // In: Cap's center
        );

    XINT32 GetMiterPoint(
        _In_ const XPOINTF &vecRad,
            // In: Radius vector for the outgoing segment
        XFLOAT rDet,
            // In: The determinant of vecIn and vecOut
        _In_ const XPOINTF &ptIn,
            // In: Offset point of the incoming segment
        _In_ const XPOINTF &vecIn,
            // In: Vector in the direction coming in
        _In_ const XPOINTF &ptNext,
            // In: Offset point of the outgoing segment
        _In_ const XPOINTF &vecOut,
            // In: Vector in the direction going out
        _Out_ XFLOAT &rDot,
            // Out: The dot product of the 2 radius vectors
        _Out_ XPOINTF &ptMiter
            // Out: The outer miter point, if within limit
        );

    _Check_return_ HRESULT DoLimitedMiter(
        _In_ const XPOINTF &ptIn,
            // In: Outer offset of incoming segment
        _In_ const XPOINTF &ptNext,
            // In: Outer offset of outgoing segment
        XFLOAT rDot,
            // In:  -(m_vecRad * vecRadNext)
        _In_ const XPOINTF &vecRadNext,
            // In: Radius vector of outgoing segment
        RAIL_SIDE side
            // In: Turn's outer side, RAIL_LEFT or RAIL_RIGHT
        );

    _Check_return_ HRESULT Do180DegreesMiter();

    _Check_return_ HRESULT BevelCorner(
        RAIL_SIDE side,
            // In: The side of the outer corner
        _In_ const XPOINTF &ptNext
            // In: The bevel's endpoint
        );

    _Check_return_ HRESULT RoundCorner(
        _In_ const XPOINTF &ptCenter,
            // In: Corner point on the spine
        _In_ const XPOINTF &ptIn,
            // In: Outer offset points of incoming segment
        _In_ const XPOINTF &ptNext,
            // In: Outer offset points of outgoing segment
        _In_ const XPOINTF &vecRad,
            // In: New value of m_vecRad
        RAIL_SIDE side
            // In: Side to be rounded, RAIL_LEFT or RAIL_RIGHT
        );

    _Check_return_ HRESULT SetCurrentPoints(
        _In_ const XPOINTF &ptLeft,
            // In: Left point
        _In_ const XPOINTF &ptRight
            // In: Right point
        );

    _Check_return_ HRESULT MiterTo(
        RAIL_SIDE side,
            // Which side to set the point
        _In_ const XPOINTF &ptMiter,
            // Miter corner
        _In_ const XPOINTF &ptNextStart,
            // The starting point of the next segment's offset
        XINT32 fExtended
            // Extend all the way to ptNextStart if TRUE
        );

private:
    CWideningSink *m_pSink;        // The sink that accepts the results
    XPOINTF       m_ptCurrent[2]; // The current left & right points
};

///////////////////////////////////////////////////////////////////////////////
// Definition of CDasher
class CDasher   :   public CPenInterface
{

// Disallow instantiation without a pen
private:
    CDasher()
    : m_pPen(NULL), m_oSegments(NULL)
    {
    }

public:
    CDasher(
        _In_ CPen *pPen,
            // In: The internal widening pen
        _In_reads_opt_(1) const CMILMatrix *pMatrix
            // In: Transformation matrix (NULL OK)
        );

    ~CDasher() override
    {
    }

    _Check_return_ HRESULT Set(
        _In_ const CPlainPen &pen
            // In: The pen we stroke with
        );

    // CPenInterface override
    _Check_return_ HRESULT StartFigure(
        _In_ const XPOINTF &pt,
            // In: Figure's firts point
        _In_ const XPOINTF &vec,
            // In: First segment's direction vector
        XINT32 fClosed,
            // In: =TRUE if we're starting a closed figure
        XcpPenCap eCapType
            // In: The start cap type
        ) override;

    _Check_return_ HRESULT AcceptLinePoint(
        _In_ const XPOINTF &point
            // In: Point to draw to
        ) override;

    _Check_return_ HRESULT AcceptCurvePoint(
        _In_ const XPOINTF &point,
            // In: The point
        _In_ const XPOINTF &vec,
            // In: The tangent there
        XINT32 fLast
            // In: Is this the last point on the curve?
        ) override;

    _Check_return_ HRESULT DoCorner(
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
        ) override;

    _Check_return_ HRESULT EndStrokeOpen(
        XINT32 fStarted,
            // = TRUE if the widening has started
        _In_ const XPOINTF &ptEnd,
            // Figure's endpoint
        _In_ const XPOINTF &vecEnd,
            // Direction vector there
        XcpPenCap eEndCap,
            // The type of the end cap
        XcpPenCap eStartCap=XcpPenCapFlat
            // The type of start cap (optional)
        ) override;

    _Check_return_ HRESULT EndStrokeClosed(
        _In_ const XPOINTF &ptEnd,
            // Figure's endpoint
        _In_ const XPOINTF &vecEnd
            // Direction vector there
        ) override;


    XINT32 Aborted() override
    {
        return m_pPen->Aborted();
    }

private:
    _Check_return_ HRESULT StartANewDash(
        XFLOAT rLoc,
            // In: The location
        XFLOAT rLength,
            // In: The dash's length
        XINT32 fAtVertex
            // In: =TRUE if we're at a segment start or end
        );

    _Check_return_ HRESULT ExtendCurrentDash(
        XFLOAT rLoc,
            // In: The location
        XINT32 fAtVertex
            // In: =TRUE if we're at a segment start or end
        );

    _Check_return_ HRESULT TerminateCurrentDash(
        XFLOAT rLoc,
            // In: The location
        XINT32 fAtVertex
            // In: =TRUE if we're at a segment start or end
        );

    _Check_return_ HRESULT Flush(
        XINT32 fLastEdge
            // =TRUE if this is the figure's last edge
        );

    _Check_return_ HRESULT DoDashOrGapEndAtEdgeEnd(
        XINT32 fLastEdge,
            // =TRUE if this is the figure's last edge
        XINT32 fIsOnDash
            // =TRUE if we are on a dash
        );

    ///////////////////////////////////////////////////////////////////////////
    // Definition of helper class CSegData - a single segment record
    //
    class CSegData
    {
    public:
        CSegData()
        : m_rLength(0), m_fIsALine(FALSE), m_vecTangent()
        {
        }

        CSegData(
            XINT32 fIsALine,
                // In: =TRUE if this is a line segment
            _In_ const XPOINTF &ptEnd,
                // In: Segment endpoint
            _In_ const XPOINTF &vecTangent,
                // In: The tangent vector there
            _In_ const XPOINTF &vecSeg,
                // In: The segment direction unit vector
            XFLOAT rLength
                // In: Accummulated length so far
            );

        // Really a data structure, not a class, so data is left public
        XPOINTF            m_ptEnd;      // The segment endpoint
        XPOINTF            m_vecTangent; // Tangent vector there
        XPOINTF            m_vecSeg;     // The segment direction unit vector
        XFLOAT              m_rLength;    // Accummulated length to this point
        XINT32                m_fIsALine;   // =TRUE if this is a line segment
    };

    ///////////////////////////////////////////////////////////////////////////
    // Definition of helper class CSegments - the segments buffer
    //
    class CSegments
    {
    public:
        CSegments(
            _In_reads_opt_(1) const CMILMatrix *pMatrix
                // In: Transformation matrix (NULL OK)
            );

        ~CSegments();

        _Check_return_ HRESULT StartWith(
            _In_ const XPOINTF &ptStart,
                // In: Starting point
            _In_ const XPOINTF &vecTangent
                // In: The tangent vector there
            );

        _Check_return_ HRESULT Add(
            _In_ const XPOINTF &ptEnd,
                // In: Segment endpoint
            _In_reads_opt_(1) const XPOINTF *pTangent=NULL
                // Optional: Tangent vector there
            );

        XFLOAT GetLength() const
        {
            return m_rgSegments[m_cSegments-1].m_rLength;
        }

        XINT32 IsAtALine() const
        {
            return m_rgSegments[m_uCurrentIndex].m_fIsALine;
        }

        XINT32 IsLast() const
        {
            return m_cSegments - 1 == m_uCurrentIndex;
        }

        XINT32 IsEmpty() const
        {
            // The first entry is just the starting point.
            return m_cSegments < 2;
        }

        XFLOAT GetCurrentEnd() const
        {
            return m_rgSegments[m_uCurrentIndex].m_rLength;
        }

        XPOINTF GetCurrentDirection() const
        {
            return m_rgSegments[m_uCurrentIndex].m_vecSeg;
        }

        XINT32 Increment()
        {
            m_uCurrentIndex++;
            return m_uCurrentIndex >= m_cSegments;
        }

        void Reset();

        void ProbeAt(
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
            ) const;

    protected:
        _Check_return_ HRESULT AddSegment(_In_ const CSegData &segData);

    private:
        // Data
        XUINT32                     m_uCurrentIndex;     // Current segment index
        CSegData                    *m_rgSegments;       // Segments buffer
        XUINT32                     m_cSegments;         // Segment count
        XUINT32                     m_cSegmentCapacity;  // Number of elements allocated in m_rgSegments

        // Coefficients of the pre-transform length quadratic form
        XFLOAT                      m_rCxx;          // Coefficient of x*x
        XFLOAT                      m_rCxy;          // Coefficient of x*y
        XFLOAT                      m_rCyy;          // Coefficient of y*y
    };

    ///////////////////////////////////////////////////////////////////////////
    // Definition of helper class CDashSequence - the dash array
    //
    // NOTE: All APIs take arguments in edge space (edge space records how far
    // we have travelled along an edge). Internally, all our computations are
    // done in dash space (how far along in the dash array we are). Note that
    // dash space is calculated modulo the length of the dash array, so in
    // order to convert from dash space to edge space we have to keep track of
    // how many times we've iterated over the dash array (m_uCurrentIteration).
    //

    class CDashSequence
    {
    public:
        // Constructor
        CDashSequence();
        ~CDashSequence();

        _Check_return_ HRESULT Set(
            _In_ const CPlainPen &pen
                // In: The nominal pen
            );

        XINT32 IsOnDash() const
        {
            return 0 != (m_uCurrentIndex & 1);
        }

        XFLOAT GetStep() const
        {
            return (m_rgDashes[m_uCurrentIndex] - m_rCurrentLoc) * m_rFactor ;
        }

        XFLOAT GetNextEndpoint() const
        {
            return DashToEdgeSpace(m_rgDashes[m_uCurrentIndex]);
        }

        void PrepareForNewEdge()
        {
            // Should be checked at ::Set, otherwise flushing an edge may result
            // in an infinite loop.
            ASSERT(2 * m_rLength >= MIN_DASH_ARRAY_LENGTH);

            m_uCurrentIteration = 0;
            m_rEdgeSpace0 = m_rCurrentLoc;
        }

        void Reset();

        void AdvanceTo(
            XFLOAT rEdgeSpaceLoc
                // New location (in edge space)
            )
        {
            m_rCurrentLoc = EdgeToDashSpace(rEdgeSpaceLoc);
            //
            // FUTURE-Ideally, we should be able to assert that
            // m_rCurrentLoc <= m_rgDashes[m_uCurrentIndex], but this isn't necessarily
            // TRUE, since floating point error can cause this class and CDasher::Flush
            // to get out of sync (in particular, m_rInverseFactor is computed using only
            // single precision). This is no biggie, though, since this just means that the
            // dash will be a little longer than it should be. Still, it'd be nice to clean
            // up this inconsistency.
            //
        }

        void Increment();

        XFLOAT GetLengthOfNextDash() const
        {
            XUINT32 iStart;

            if (m_uCurrentIndex == m_cDashes - 1)
            {
                iStart = 0;
            }
            else
            {
                iStart = m_uCurrentIndex;
            }

            if(iStart >= m_cDashes - 1)
            {
                return 0;
            }

            return (m_rgDashes[iStart+1] - m_rgDashes[iStart]) * m_rFactor;
        }

    protected:

        XFLOAT DashToEdgeSpace(XFLOAT rDashSpaceLoc) const
        {
            //
            // If the dash array has very long dashes, it's easy to get NaNs,
            // even though the behavior may be well-defined. To guard against
            // this, we special case the m_uCurrentIteration == 0 case.
            //

            if (m_uCurrentIteration == 0)
            {
                return (rDashSpaceLoc - m_rEdgeSpace0)*m_rFactor;
            }
            else
            {
                return ((rDashSpaceLoc - m_rEdgeSpace0) +
                    m_uCurrentIteration * m_rLength)*m_rFactor;
            }
        }

        XFLOAT EdgeToDashSpace(XFLOAT rEdgeSpaceLoc) const
        {
            //
            // If the dash array has very long dashes, it's easy to get NaNs,
            // even though the behavior may be well-defined. To guard against
            // this, we special case the m_uCurrentIteration == 0 case.
            //

            if (m_uCurrentIteration == 0)
            {
                return rEdgeSpaceLoc*m_rInverseFactor
                    + m_rEdgeSpace0;
            }
            else
            {
                return rEdgeSpaceLoc*m_rInverseFactor
                    - m_uCurrentIteration * m_rLength
                    + m_rEdgeSpace0;
            }

        }

    private:

        // Data
        XUINT32         m_uCurrentIndex;     // Current dash/space end index
        XUINT32         m_uCurrentIteration; // Number of times we've wrapped around since
                                             // the last call to PrepareForNewEdge()
        XFLOAT          m_rFactor;           // Dash to segments adjustment factor
        XFLOAT          m_rInverseFactor;    // Inverse of the above
        XFLOAT          m_rCurrentLoc;       // Current location in the dash sequence
                                             // (in dash coordinates)
        XFLOAT          m_rEdgeSpace0;       // The value of m_rCurrentLoc at the time
                                             // of the last PrepareForNewEdge()
        XFLOAT          m_rLength;           // Sequence's total length
        XUINT32         m_uStartIndex;       // The dash/space where the dash sequence starts
        _Field_size_opt_(m_cDashes) XFLOAT          *m_rgDashes;         // Dash/space ends array
        XUINT32         m_cDashes;           // Dash array count
    };

private:
    // CDasher data
    CSegments        m_oSegments;            // Segments buffer
    CDashSequence    m_oDashes;              // The dash sequence
    CPen             *m_pPen;                // The widening pen

    XcpPenCap        m_eDashCap;             // Dash cap
    XINT32           m_fIsPenDown;           // =TRUE when we're on a dash
    XINT32           m_fIsFirstCapPending;   // =TRUE if the first cap is yet to be set
};

////////////////////////////////////////////////////////////////////////////////////
// Definition of CWidener
// The widener is more of a structure than a class. That's why its data is public.
class CWidener
{
public:
    CWidener();

    virtual ~CWidener();

    CWidener(
        _In_reads_opt_(1) const CMILMatrix *pMatrix,
            // In: Transformation matrix (NULL OK)
        XFLOAT rTolerance
            // In: Approximation tolerance
        );

    _Check_return_ HRESULT Set(
        _In_ const CPlainPen &pen,
            // The stroking pen
        _In_ CWideningSink *pSink
            // The widening sink
        );

    _Check_return_ HRESULT WidenLineShape(
        _In_ const CShape &shape,
            // The widened shape
        _In_reads_opt_(1) const CMILMatrix *pMatrix
            // The positioning matrix
        );

    const CPen &GetPen() const
    {
        return m_pen;
    }

    void SetTarget(
        _In_reads_opt_(1) CPenInterface *pTarget
        )
    {
        m_pTarget = pTarget;
        m_oLine.SetTarget(m_pTarget);
        m_oCubic.SetTarget(m_pTarget);
    }

protected:
// Fixed data members, set for the life of the widener, which is one call of
// CShapeBase::WidenToSink
    const CMILMatrix  *m_pMatrix;       // Transformation matrix
    XFLOAT            m_rTolerance;     // Approximation tolerance

    // The caps seem to duplicate the pen's start/end cap settings, but the
    // widener's target may be dasher.  In that case, the pen's caps will vary
    // between original start/end cap and dash cap.  So here we keep track of
    // the original cap types
    XcpPenCap      m_eStartCap;        // Start cap style
    XcpPenCap      m_eEndCap;          // End cap style
    XcpPenCap      m_eDashCap;         // Dash cap style

    XcpLineJoin     m_eLineJoin;        // Line join style
    CSimplePen      m_pen;              // Widening pen
    CDasher         m_dasher;
    CPenInterface   *m_pTarget;         // Pen or Dasher

// Working variables
    // Fixed for the duration of a figure
    mutable XPOINTF    m_vecStart;     // Figure's start direction vector
    mutable XPOINTF    m_ptStart;      // Figure's first point
    mutable XFLOAT      m_rStartTrim;   // Trim parameter at figure start
    mutable XFLOAT      m_rEndTrim;     // Trim parameter at figure end

    // Changing between segments
    mutable XPOINTF    m_vecIn;        // Corner incoming direction vector
    mutable XPOINTF    m_vecOut;       // Corner outgoing direction vector
    mutable XPOINTF    m_pt;           // Current point
    mutable XINT32     m_fShouldPenBeDown;  // = TRUE when we should be drawing
    mutable XINT32     m_fIsPenDown;   // = TRUE when we are actually drawing
    mutable XINT32     m_fClosed;      // = TRUE if widened as a closed figure
    mutable XINT32     m_fSkipped;     // =TRUE if recent degenerate segment was skipped
    mutable XINT32     m_fSkippedFirst;// =TRUE if first degenerate segment was skipped
    mutable XcpPenCap  m_eCap;         // Current start-cap style (may be dash style by gaps)
    mutable XINT32     m_fNeedToRecordStart; // =TRUE if we need to record the first point and vector
    mutable XINT32     m_fSmoothJoin;  // =TRUE if the join is known to be smooth

    // The data of the current segment, set for widening
    mutable CWidenLineSegment  m_oLine;  // Line segment data
    mutable CCubicSegment m_oCubic;     // Cubic curve data
    mutable CSegment      *m_pSegment;  // The current choice between the above
};


class CWidenerSink final : public CWidener,
                     public CXcpObjectBase< IPALGeometrySink >
{
    public:
        CWidenerSink(
            _In_opt_ const CMILMatrix* pMatrix,
            XFLOAT tolerance
            );

        ~CWidenerSink(
            ) override;

        void BeginFigure(
            const XPOINTF& startPoint,
            bool fIsHollow
            ) override;

        void EndFigure(
            bool fIsClosed
            ) override;

        void AddArc(
            const XPOINTF& point,
            const XSIZEF& size,
            XFLOAT rotationAngle,
            bool fIsClockwise,
            bool fIsLargeArc
            ) override;

        void AddBezier(
            const XPOINTF& controlPoint1,
            const XPOINTF& controlPoint2,
            const XPOINTF& endPoint
            ) override;

        void AddLine(
            const XPOINTF& point
            ) override;

        void AddQuadraticBezier(
            const XPOINTF& controlPoint,
            const XPOINTF& endPoint
            ) override;

        void AddLines(
            _In_reads_(uiCount) const XPOINTF *pPoints,
            XUINT32 uiCount
            ) override;

        void AddBeziers(
            _In_reads_(uiCount) const XPOINTF *pPoints,
            XUINT32 uiCount
            ) override;

        void AddQuadraticBeziers(
            _In_reads_(uiCount) const XPOINTF *pPoints,
            XUINT32 uiCount
            ) override;

        void SetFillMode(
            GeometryFillMode fillMode
            ) override;

        _Check_return_ HRESULT Close(
            ) override;

    protected:
        _Check_return_ HRESULT AddSegments(
            _In_reads_(numPoints) const XPOINTF* pPoints,
            XUINT32 numPoints,
            XUINT32 pointsPerSegment,
            XUINT32 numSegments
            );

        void SetSegmentForWidening(
            _Inout_ XPOINTF& ptFirst,
            _In_reads_(numPoints) const XPOINTF *pPoints,
            XUINT32 numPoints,
            _In_opt_ const CMILMatrix* pMatrix
            );

        _Check_return_ HRESULT DoGap();

        _Check_return_ HRESULT DoSegment(
            _In_reads_(numPoints) const XPOINTF* pPoints,
            XUINT32 numPoints
            );

        bool JoinIsSmooth(
            );

        bool SegmentIsGap(
            );

        bool m_startedWithoutCap;
        bool m_lastSegmentWasAGap;
        XPOINTF m_lastVertexProcessed;
        XPOINTF m_inputFigureStartPoint;
        bool m_firstFigureSegmentReached;
        bool m_firstFigureSegmentWasAGap;
        bool m_figureContainsGap;
        HRESULT m_hr;
};

///////////////////////////////////////////////////////////////////////////////
// Definition of CWideningSink

// CWideningSink abstracts the recipient of the results of path widening.  The
// various implementations support generating the outline path, computing bounds
// and hit testing

class CWideningSink
{
public:

    CWideningSink() {}
    virtual ~CWideningSink() {}

    virtual _Check_return_ HRESULT StartWith(
        _In_reads_(2) const XPOINTF *ptOffset
            // Left and right offset points
        )=0;

    virtual _Check_return_ HRESULT QuadTo(
        _In_reads_(2) const XPOINTF *ptOffset
            // In: Left & right offset points
        )=0;

    virtual _Check_return_ HRESULT QuadTo(
        _In_reads_(2) const XPOINTF *ptOffset,
            // Left & right offset points thereof
        _In_ const XPOINTF &vecSeg,
            // Segment direction we're coming from
        _In_ const XPOINTF &ptSpine,
            // The corresponding point on the stroke's spine
        _In_ const XPOINTF &ptSpinePrev
            // The previous point on the stroke's spine
        )=0;

    virtual _Check_return_ HRESULT CurveWedge(
        RAIL_SIDE side,
            // Which side to add to
        _In_ const XPOINTF &ptBez_1,
            // First control point
        _In_ const XPOINTF &ptBez_2,
            // Second control point
        _In_ const XPOINTF &ptBez_3
            // Last point
        )=0;

    virtual _Check_return_ HRESULT BezierCap(
        _In_ const XPOINTF &ptStart,
            // The cap's first point,
        _In_ const XPOINTF &pt0_1,
            // First arc's first control point
        _In_ const XPOINTF &pt0_2,
            // First arc's second control point
        _In_ const XPOINTF &ptMid,
            // The point separating the 2 arcs
        _In_ const XPOINTF &pt1_1,
            // Second arc's first control point
        _In_ const XPOINTF &pt1_2,
            // Second arc's second control point
        _In_ const XPOINTF &ptEnd
            // The cap's last point
        )=0;

    virtual _Check_return_ HRESULT SetCurrentPoints(
        _In_reads_(2) const XPOINTF *P
            // In: Array of 2 points
        )=0;

    virtual _Check_return_ HRESULT DoInnerCorner(
        RAIL_SIDE side,
            // In: The side of the inner corner
        _In_ const XPOINTF &ptCenter,
            // In: The corner's center
        _In_reads_(2) const XPOINTF *ptOffset
            // In: The offset points of new segment
        )=0;

    virtual _Check_return_ HRESULT CapTriangle(
        _In_ const XPOINTF &ptStart,
            // Triangle base start point
        _In_ const XPOINTF &ptApex,
            // Triangle's apex
        _In_ const XPOINTF &ptEnd
            // Triangle base end point
        )=0;

    virtual _Check_return_ HRESULT CapFlat(
        _In_ const XPOINTF *,
            // Ignored
        RAIL_SIDE
            // Ignored
        )
    {
        // Do nothing stub, OK to call
        return S_OK;
    }

    virtual _Check_return_ HRESULT PolylineWedge(
        RAIL_SIDE side,
            // Which side to add to - RAIL_RIGHT or RAIL_LEFT
        XUINT32 count,
            // Number of points
        _In_reads_(count) const XPOINTF *pPoints
            // The polyline vertices
        )=0;

    virtual _Check_return_ HRESULT AddFigure()
    {
        // Do nothing stub, OK to call
        return S_OK;
    }

    virtual _Check_return_ HRESULT SwitchSides()=0;

    virtual XINT32 Aborted()
    {
        // Most sinks never abort
        return FALSE;
    }

    // No data
};

class CStrokeBoundsSink : public CWideningSink
{
    public:
        CStrokeBoundsSink(
            );

        ~CStrokeBoundsSink(
            ) override;

        _Check_return_ HRESULT StartWith(
            _In_reads_(2) const XPOINTF *ptOffset
            ) override;

        _Check_return_ HRESULT SetCurrentPoints(
            _In_reads_(2) const XPOINTF *pPoints
            ) override;

        _Check_return_ HRESULT QuadTo(
            _In_reads_(2) const XPOINTF *ptOffset
            ) override;

        _Check_return_ HRESULT QuadTo(
            _In_reads_(2) const XPOINTF *ptOffset,
            _In_ const XPOINTF &vecSeg,
            _In_ const XPOINTF &ptSpine,
            _In_ const XPOINTF &ptSpinePrev
            ) override;

        _Check_return_ HRESULT CurveWedge(
            RAIL_SIDE side,
            _In_ const XPOINTF &ptBez_1,
            _In_ const XPOINTF &ptBez_2,
            _In_ const XPOINTF &ptBez_3
            ) override;

        _Check_return_ HRESULT PolylineWedge(
            RAIL_SIDE side,
            XUINT32 count,
            _In_reads_(count) const XPOINTF *pPoints
            ) override;

        _Check_return_ HRESULT DoInnerCorner(
            RAIL_SIDE side,
            _In_ const XPOINTF &ptCenter,
            _In_reads_(2) const XPOINTF *ptOffset
            ) override;

        _Check_return_ HRESULT SwitchSides(
            ) override;

        _Check_return_ HRESULT BezierCap(
            _In_ const XPOINTF &ptStart,
            _In_ const XPOINTF &pt0_1,
            _In_ const XPOINTF &pt0_2,
            _In_ const XPOINTF &ptMid,
            _In_ const XPOINTF &pt1_1,
            _In_ const XPOINTF &pt1_2,
            _In_ const XPOINTF &ptEnd
            ) override;

        _Check_return_ HRESULT CapTriangle(
            _In_ const XPOINTF &ptStart,
            _In_ const XPOINTF &ptApex,
            _In_ const XPOINTF &ptEnd
            ) override;

        void GetBounds(
            _Out_ XRECTF_RB* pBounds
            );

    protected:
        XRECTF_RB m_bounds;
        XPOINTF m_currentPoints[2];
};

class HitTestHelper;

class CStrokeHitTestSink : public CWideningSink
{
public:
    CStrokeHitTestSink(
        HitTestHelper& hitTestHelper
        );

    ~CStrokeHitTestSink(
        ) override;

    _Check_return_ HRESULT StartWith(
        _In_reads_(2) const XPOINTF *ptOffset
        ) override;

    _Check_return_ HRESULT SetCurrentPoints(
        _In_reads_(2) const XPOINTF *pPoints
        ) override;

    _Check_return_ HRESULT QuadTo(
        _In_reads_(2) const XPOINTF *ptOffset
        ) override;

    _Check_return_ HRESULT QuadTo(
        _In_reads_(2) const XPOINTF *ptOffset,
        _In_ const XPOINTF &vecSeg,
        _In_ const XPOINTF &ptSpine,
        _In_ const XPOINTF &ptSpinePrev
        ) override;

    _Check_return_ HRESULT CurveWedge(
        RAIL_SIDE side,
        _In_ const XPOINTF &ptBez_1,
        _In_ const XPOINTF &ptBez_2,
        _In_ const XPOINTF &ptBez_3
        ) override;

    _Check_return_ HRESULT PolylineWedge(
        RAIL_SIDE side,
        XUINT32 count,
        _In_reads_(count) const XPOINTF *pPoints
        ) override;

    _Check_return_ HRESULT DoInnerCorner(
        RAIL_SIDE side,
        _In_ const XPOINTF &ptCenter,
        _In_reads_(2) const XPOINTF *ptOffset
        ) override;

    _Check_return_ HRESULT SwitchSides(
        ) override;

    _Check_return_ HRESULT BezierCap(
        _In_ const XPOINTF &ptStart,
        _In_ const XPOINTF &pt0_1,
        _In_ const XPOINTF &pt0_2,
        _In_ const XPOINTF &ptMid,
        _In_ const XPOINTF &pt1_1,
        _In_ const XPOINTF &pt1_2,
        _In_ const XPOINTF &ptEnd
        ) override;

    _Check_return_ HRESULT CapTriangle(
        _In_ const XPOINTF &ptStart,
        _In_ const XPOINTF &ptApex,
        _In_ const XPOINTF &ptEnd
        ) override;

    XINT32 Aborted(
        ) override;

    bool WasHit(
        ) const;

protected:
    CStrokeHitTestSink(
        const CStrokeHitTestSink& other
        );

    CStrokeHitTestSink& operator=(
        const CStrokeHitTestSink& other
        );

    XPOINTF m_currentPoints[2]{};
    bool m_hitInside;
    HitTestHelper& m_hitTestHelper;
};
