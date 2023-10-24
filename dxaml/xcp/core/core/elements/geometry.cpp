// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "D2DAccelerated.h"
#include "D2DAcceleratedPrimitives.h"
#include "VisualContentRenderer.h"
#include "d3d11device.h"
#include "WindowsGraphicsDeviceManager.h"
#include <FrameworkUdk/Containment.h>

// Bug 45792810: Explorer first frame - DesktopWindowXamlSource spends 30ms on RoGetActivationFactory
// Bug 46468883: [1.4 servicing] Explorer first frame - DesktopWindowXamlSource spends 30ms on RoGetActivationFactory
#define WINAPPSDK_CHANGEID_46468883 46468883

//------------------------------------------------------------------------
//
//  Synopsis:
//      Constructor for path block
//
//------------------------------------------------------------------------
CPathBlock::CPathBlock()
{
    m_pNext = NULL;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Constructor for geometry builder
//
//------------------------------------------------------------------------
CGeometryBuilder::CGeometryBuilder(_In_ CCoreServices *pCoreServices)
    : m_pCoreNoRef(pCoreServices)
    , m_pPathFigures(NULL)
    , m_pCurrentPathFigure(NULL)
{
    XCP_WEAK(&m_pCoreNoRef);

    m_pHead = NULL;
    m_pCurr = NULL;
    m_cPoint = 0;
    m_fTypeFlags = 0;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Destructor for geometry builder
//
//------------------------------------------------------------------------
CGeometryBuilder::~CGeometryBuilder()
{
    CPathBlock *next;
    while (m_pHead)
    {
        next = m_pHead->m_pNext;
        delete m_pHead;
        m_pHead = next;
    }

    ReleaseInterface(m_pPathFigures);
    ReleaseInterface(m_pCurrentPathFigure);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Make sure enough contiguous points are available in the current block
//      then convert the points to single precision floats and store them.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CGeometryBuilder::StorePoints(
    _In_ PathPointType type,
    _In_ XUINT32 cPoint
    )
{
    ASSERT(cPoint <= BLOCK_LIMIT, L"Point store exceeds block limit.\n");
    HRESULT hr;
    CPathBlock *temp;

    if (cPoint + m_pCurr->m_cPoint > BLOCK_LIMIT)
    {
        temp = m_pCurr->m_pNext;

        if (NULL == temp)
        {
            temp = new CPathBlock;

            m_pCurr->m_pNext = temp;
        }

        temp->m_cPoint = 0;
        m_pCurr = temp;
    }

// There is enough space in the current block so convert them to fixed for storage.

    XPOINTF *pptSrc;
    XPOINTF *pptTrg;
    XUINT8  *pType;

    pptSrc = m_pAbsolute;
    pptTrg = &m_pCurr->m_aPoint[m_pCurr->m_cPoint];
    pType  = &m_pCurr->m_aType[m_pCurr->m_cPoint];

// Move signals the start of a new figure.  Remember its starting point

    if (PathPointTypeStart == type)
        m_ptStart = *pptSrc;

// Update the point count in the path block as well as the total geometry

    m_pCurr->m_cPoint += XUINT8(cPoint);
    m_cPoint += cPoint;

    while (cPoint--)
    {
       *pptTrg++ = *pptSrc++;
       *pType++  = (XUINT8)(type | m_fTypeFlags);
    }

    hr = S_OK;

// Remember the type of the segment and update the pointer to the point that
// represents the current position.

    m_typePrev = type;
    m_pptCurr = --pptTrg;

// If the segment was a cubic Bezier or quadratic curve then we also need to
// remember the point before as well in case we need to compute the reflection
// from it for the next segment.

    if (type & PathPointTypeCurve)
    {
        m_pptReflect = --pptTrg;
    }

    RRETURN(hr);//RRETURN_REMOVAL
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Opens a new geometry in the geometry builder
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CGeometryBuilder::OpenGeometry()
{
    HRESULT hr = S_OK;

    if (NULL == m_pHead)
    {
        m_pHead = new CPathBlock;
    }

// Initialize the first block

    m_pCurr = m_pHead;
    m_pCurr->m_cPoint = 0;

// Initialize the flags and other state of the builder

    m_cPoint = 0;
    m_ptStart.x = 0.0;
    m_ptStart.y = 0.0;
    m_pptCurr = &m_ptStart;
    m_typePrev = 0;
    m_bOpen = FALSE;
    m_bPendingMove = FALSE;
    m_bValid = FALSE;

    RRETURN(hr);//RRETURN_REMOVAL
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Start a new figure in a geometry
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CGeometryBuilder::OpenFigure(
    _In_ XINT32 flags,
    _In_ XPOINTF *ppt
    )
{
    HRESULT hr = S_OK;
    CPathFigure *pPathFigure = NULL;

    // Opening a figure sets a new start position
    if (VerifyFlag(flags, XcpPointType_Relative))
    {
        // If we are relative, we need to use the last point we entered.
        // This will be zero if this is the first command.
        m_ptStart = *m_pptCurr + *ppt;
    }
    else
    {
        m_ptStart = *ppt;
    }

    m_ptLastMove = m_ptStart;
    m_pAbsolute = &m_ptLastMove;

    // Our current point is the start point
    m_pptCurr = &m_ptStart;

    // We don't want to compute smooth curves across figures
    m_typePrev = 0;

    m_bOpen = TRUE;
    m_bValid = TRUE;
    m_fTypeFlags &= ~PathPointTypeEmptyFill; // Each figure starts out filled

    if (IsBuildingFigure())
    {
        CREATEPARAMETERS cp(m_pCoreNoRef);
        IFC(CPathFigure::Create(reinterpret_cast<CDependencyObject **>(&pPathFigure), &cp));
        pPathFigure->m_ptStart = m_ptStart;

        IFC(m_pPathFigures->Append(pPathFigure));
        ReplaceInterface(m_pCurrentPathFigure, pPathFigure);
    }
    else
    {
        m_bPendingMove = TRUE; // Defer the store until we see a real segment
    }

Cleanup:
    ReleaseInterface(pPathFigure);

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Sets the filled status of the current figure.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CGeometryBuilder::SetFigureFilled(
    _In_ bool bFill
    )
{
    if (bFill)
    {
        m_fTypeFlags &= ~PathPointTypeEmptyFill;
    }
    else
    {
        m_fTypeFlags |= PathPointTypeEmptyFill;
    }

    if (IsBuildingFigure())
    {
        m_pCurrentPathFigure->m_bFilled = !!bFill;
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Close the currently open figure in the geometry
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CGeometryBuilder::CloseFigure()
{
    HRESULT hr = S_OK;

    if (IsBuildingFigure())
    {
        if (m_bOpen)
        {
            ASSERT(m_pCurrentPathFigure != NULL);
            m_pCurrentPathFigure->m_bClosed = true;
        }
    }
    else
    {
        if (   m_bOpen                        // the figure must be open
            && m_pCurr && m_pCurr->m_cPoint   // there must be points in the figure
            && m_typePrev != 0 )              // we must have added a valid draw command after a move
        {
            m_pCurr->m_aType[m_pCurr->m_cPoint - 1] |= PathPointTypeCloseSubpath;

            // If the final point was a start/end pair then remove it from the figure.
            if ((PathPointTypeStart | PathPointTypeCloseSubpath) == m_pCurr->m_aType[m_pCurr->m_cPoint - 1])
            {
                m_pCurr->m_cPoint--;
                m_cPoint--;
            }
            else
            {
                // If we are asked to close, use the start point
                m_pptCurr = &m_ptStart;
            }
        }
    }

    // Reset our previous state
    m_bOpen = FALSE;
    m_bPendingMove = FALSE;
    m_typePrev = 0;

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Computes the reflection point for the specified curve
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CGeometryBuilder::ComputeReflection(
    _In_ XINT32 flags,
    _In_ PathPointType type,
    _Out_ XPOINTF *ppt
    )
{
    // Test for path validity
    if (!m_bValid)
        return E_UNEXPECTED;

    // If the previous segment type doesn't match then just use the current point.
    if ((XUINT32)type != m_typePrev)
    {
        if (VerifyFlag(flags, XcpPointType_Relative))
        {
            //
            // If we are relative then set the current point to the origin. When we map back to
            // absolute, we will properly offset the point.
            //
            // It's possible that we haven't added the pending move point yet. But the move point
            // will be added in AddBezier or AddQuadratic before this reflected point is added,
            // so the relative offset should still be (0,0).
            //
            // It's also possible that we aren't open yet. In which case AddBezier or AddQuadratic
            // will call OpenFigure first thing, which sets m_bPendingMove. Then it's the same
            // case as above.
            //
            ppt->x = 0;
            ppt->y = 0;
        }
        else
        {
            // If we are absolute, use the current point.
            *ppt = *m_pptCurr;
        }
    }
    else
    {
        if (VerifyFlag(flags, XcpPointType_Relative))
        {
            ppt->x = m_pptCurr->x - m_pptReflect->x;
            ppt->y = m_pptCurr->y - m_pptReflect->y;
        }
        else
        {
            ppt->x = 2 * m_pptCurr->x - m_pptReflect->x;
            ppt->y = 2 * m_pptCurr->y - m_pptReflect->y;
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Replace the value of the last point added
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CGeometryBuilder::ReplaceLastPoint(
    _In_ XPOINTF *ppt
    )
{
    HRESULT hr = S_OK;

    if (m_bPendingMove)
    {
        m_ptLastMove = *ppt;
    }
    else
    {
        ASSERT(m_bOpen && m_pCurr != NULL && m_pCurr->m_cPoint > 0);

        m_pCurr->m_aPoint[m_pCurr->m_cPoint-1] = *ppt;
    }

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Add a straight line segment to the figure
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CGeometryBuilder::AddLine(
    _In_ XINT32 flags,
    _In_ XPOINTF *ppt
    )
{
    HRESULT hr;
    CLineSegment *pLineSegment = NULL;

// Test for path validity

    if (!m_bValid)
        return E_UNEXPECTED;

    // Fill in assumed coordinate for horizontal or vertical line segments
    if (!VerifyFlag(flags, VALID_X))
    {
        // In the case of V and H, we only get one coordinate fill in in the X slot.
        // For V we must swap this to th Y value.
        ppt->y = ppt->x;
        ppt->x = VerifyFlag(flags, XcpPointType_Relative) ? 0 : m_pptCurr->x;
    }
    else if (!VerifyFlag(flags, VALID_Y))
    {
        ppt->y = VerifyFlag(flags, XcpPointType_Relative) ? 0 : m_pptCurr->y;
    }

    // If we're not in an open figure open then open one at the current position,
    if (!m_bOpen)
    {
        IFC(OpenFigure(FALSE, &m_ptStart));
    }

    if (IsBuildingFigure())
    {
        CREATEPARAMETERS cp(m_pCoreNoRef);

        IFC(CLineSegment::Create(reinterpret_cast<CDependencyObject **>(&pLineSegment), &cp));
        MakeAbsolute(flags, 1, ppt);
        pLineSegment->m_pt = *m_pAbsolute;

        if (!m_pCurrentPathFigure->m_pSegments)
        {
            IFC(CPathSegmentCollection::Create(reinterpret_cast<CDependencyObject **>(&m_pCurrentPathFigure->m_pSegments), &cp));
        }
        IFC(m_pCurrentPathFigure->m_pSegments->Append(pLineSegment));

        m_ptCurr = *m_pAbsolute;
        m_pptCurr = &m_ptCurr;
        m_typePrev = PathPointTypeLine;
    }
    else
    {
        // If we have a pending move, store that point here
        if (m_bPendingMove)
        {
            IFC(StorePoints(PathPointTypeStart, 1));
            m_bPendingMove = FALSE;
        }

        MakeAbsolute(flags, 1, ppt);
        IFC(StorePoints(PathPointTypeLine, 1));
    }

Cleanup:
    ReleaseInterface(pLineSegment);

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Add a cubic Bezier segment to the figure
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CGeometryBuilder::AddBezier(
    _In_ XINT32 flags,
    _In_reads_(3) XPOINTF *ppt
    )
{
    HRESULT hr;
    XUINT32 cPoint = 3;
    CBezierSegment *pBezierSegment = NULL;

    // Test for path validity
    if (!m_bValid)
        return E_UNEXPECTED;

    // If we're not in an open figure open then open one at the current position,
    if (!m_bOpen)
    {
        IFC(OpenFigure(FALSE, &m_ptStart));
    }

    if (IsBuildingFigure())
    {
        CREATEPARAMETERS cp(m_pCoreNoRef);

        IFC(CBezierSegment::Create(reinterpret_cast<CDependencyObject **>(&pBezierSegment), &cp));
        MakeAbsolute(flags, cPoint, ppt);
        pBezierSegment->m_ptOne = m_pAbsolute[0];
        pBezierSegment->m_ptTwo = m_pAbsolute[1];
        pBezierSegment->m_ptThree = m_pAbsolute[2];

        if (!m_pCurrentPathFigure->m_pSegments)
        {
            IFC(CPathSegmentCollection::Create(reinterpret_cast<CDependencyObject **>(&m_pCurrentPathFigure->m_pSegments), &cp));
        }
        IFC(m_pCurrentPathFigure->m_pSegments->Append(pBezierSegment));

        m_ptCurr = pBezierSegment->m_ptThree;
        m_pptCurr = &m_ptCurr;
        m_ptReflect = pBezierSegment->m_ptTwo;
        m_pptReflect = &m_ptReflect;
        m_typePrev = PathPointTypeBezier;
    }
    else
    {
        // If we have a pending move, store that point here
        if (m_bPendingMove)
        {
            IFC(StorePoints(PathPointTypeStart, 1));
            m_bPendingMove = FALSE;
        }

        MakeAbsolute(flags, cPoint, ppt);
        IFC(StorePoints(PathPointTypeBezier, cPoint));
    }

Cleanup:
    ReleaseInterface(pBezierSegment);

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Add a quadratic curve segment to the figure
//
//------------------------------------------------------------------------
_Check_return_
HRESULT
CGeometryBuilder::AddQuadratic(
    _In_ XINT32 flags,
    _In_reads_(2) XPOINTF *ppt
    )
{
    HRESULT hr;
    XUINT32 cPoint = 2;
    CQuadraticSegment *pQuadraticSegment = NULL;

    // Test for path validity
    if (!m_bValid)
        return E_UNEXPECTED;

    // If we're not in an open figure open then open one at the current position,
    if (!m_bOpen)
    {
        IFC(OpenFigure(FALSE, &m_ptStart));
    }

    if (IsBuildingFigure())
    {
        CREATEPARAMETERS cp(m_pCoreNoRef);

        IFC(CQuadraticSegment::Create(reinterpret_cast<CDependencyObject **>(&pQuadraticSegment), &cp));
        MakeAbsolute(flags, cPoint, ppt);
        pQuadraticSegment->m_ptOne = m_pAbsolute[0];
        pQuadraticSegment->m_ptTwo = m_pAbsolute[1];

        if (!m_pCurrentPathFigure->m_pSegments)
        {
            IFC(CPathSegmentCollection::Create(reinterpret_cast<CDependencyObject **>(&m_pCurrentPathFigure->m_pSegments), &cp));
        }
        IFC(m_pCurrentPathFigure->m_pSegments->Append(pQuadraticSegment));

        m_ptCurr = pQuadraticSegment->m_ptTwo;
        m_pptCurr = &m_ptCurr;
        m_ptReflect = pQuadraticSegment->m_ptOne;
        m_pptReflect = &m_ptReflect;
        m_typePrev = PathPointTypeQuadratic;
    }
    else
    {
        // If we have a pending move, store that point here
        if (m_bPendingMove)
        {
            IFC(StorePoints(PathPointTypeStart, 1));
            m_bPendingMove = FALSE;
        }

        MakeAbsolute(flags, cPoint, ppt);
        IFC(StorePoints(PathPointTypeQuadratic, cPoint));
    }

Cleanup:
    ReleaseInterface(pQuadraticSegment);

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Add an arc segment to the figure.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CGeometryBuilder::AddArc(
    _In_ XINT32 flags,
    _In_reads_(2) XPOINTF *ppt,    // 0 is endpoint, 1 is size
    _In_ XFLOAT eAngle,
    _In_ XINT32 bLarge,
    _In_ XINT32 bClockwise,
    _In_opt_ const CMILMatrix *pMatrix
    )
{
    HRESULT hr;
    XPOINTF ptArcAsBezier[12];
    XPOINTF ptStart;
    XINT32 nCurves;
    CArcSegment *pArcSegment = NULL;

    // Test for path validity
    if (!m_bValid)
        return E_UNEXPECTED;

    // If we're not in an open figure open then open one at the current position,
    if (!m_bOpen)
    {
        // If we are not open, we came from the path mini-language and thus
        // have no transform matrix which can only be set via the Geometry OM
        ASSERT( pMatrix == NULL );
        IFC(OpenFigure(FALSE, &m_ptStart));
    }

    if (IsBuildingFigure())
    {
        CREATEPARAMETERS cp(m_pCoreNoRef);

        //
        // The matrix is only passed in by CArcSegment to handle the geometry transform, and in
        // D2D rendering the geometry transform is handled outside the figures and segments in
        // the geometry.
        //
        ASSERT(pMatrix == NULL);

        IFC(CArcSegment::Create(reinterpret_cast<CDependencyObject **>(&pArcSegment), &cp));
        MakeAbsolute(flags, 1, ppt);
        pArcSegment->m_ptBase = m_pAbsolute[0];
        pArcSegment->m_size.width = ppt[1].x;
        pArcSegment->m_size.height = ppt[1].y;
        pArcSegment->m_eAngle = eAngle;
        pArcSegment->m_bLarge = !!bLarge;
        pArcSegment->m_bClockwise = !!bClockwise;

        if (!m_pCurrentPathFigure->m_pSegments)
        {
            IFC(CPathSegmentCollection::Create(reinterpret_cast<CDependencyObject **>(&m_pCurrentPathFigure->m_pSegments), &cp));
        }
        IFC(m_pCurrentPathFigure->m_pSegments->Append(pArcSegment));

        m_ptCurr = m_pAbsolute[0];
        m_pptCurr = &m_ptCurr;
        // Coerce the state so that next smooth curve doesn't continue onto these
        // Use a line type since that is never smoothly joined.
        m_typePrev = PathPointTypeLine;
    }
    else
    {
        // If we have a pending move, store that point here
        if (m_bPendingMove)
        {
            IFC(StorePoints(PathPointTypeStart, 1));
            m_bPendingMove = FALSE;
        }

        // Arcs don't really fit the storage model for a few reasons.  But we're going
        // to make them work by converting them to Beziers and storing those.
        // Where the arc ends (which we oddly enough store at the beginning of
        // the point array we pass this function) is the only value that can be relative.
        MakeAbsolute(flags, 1, ppt);

        // The second point is actually the size of the arc.  If we've got an absolute
        // arc then we don't have to copy it.
        if (VerifyFlag(flags, XcpPointType_Relative))
            m_pAbsolute[1] = ppt[1];

        // Get the start point
        ptStart = *m_pptCurr;
        if (pMatrix)
        {
            // NOTE: we could potentially store the UN-transformed start point
            // when we open the figure and use that instead to avoid the
            // transform/inverse issue below, since this will skip the Bezier conversion
            // on non-invertible transforms.

            // If we have a transform, we already transformed the start point
            // - so get it in its original form.
            CMILMatrix matInverse = *pMatrix;
            if (matInverse.Invert())
            {
                matInverse.Transform( &ptStart, &ptStart, 1 );
            }
            else
            {
                // We can't invert this transform, so store this is as a line
                // and exit.  This is likely a scale by 0 in one dimension.
                pMatrix->Transform( m_pAbsolute, m_pAbsolute, 1 );
                IFC(StorePoints(PathPointTypeLine, 1));
                goto Cleanup;
            }
        }

        // Call the utility function to do the real work.
        ArcToBezier(
            ptStart.x,        // The first point is the current point
            ptStart.y,
            m_pAbsolute[1].x, // The size
            m_pAbsolute[1].y,
            eAngle,           // The angle
            bLarge,           // The bLarge flag
            bClockwise,       // The bClockwise flag
            m_pAbsolute[0].x, // The end point
            m_pAbsolute[0].y,
            ptArcAsBezier,
            &nCurves);

        // Now handle the different possible outcomes
        if (nCurves > 0)
        {
            // Transform all points in the curve if there's a transform set
            if (pMatrix)
            {
                pMatrix->Transform( ptArcAsBezier, ptArcAsBezier, 3*nCurves );
            }

            // Add this as a poly-Bezier
            IFC(AddSegments(PathPointTypeBezier, 3*nCurves, ptArcAsBezier));

            // coerce the state so that next smooth curve doesn't continue onto these
            // Use a line type since that is never smoothly joined.
            m_typePrev = PathPointTypeLine;
        }
        else
        {
            // Transform this lone point if there's a transform set
            if (pMatrix)
            {
                pMatrix->Transform(m_pAbsolute, m_pAbsolute, 1);
            }

            // Store this is as a line
            IFC(StorePoints(PathPointTypeLine, 1));
        }
    }

Cleanup:
    ReleaseInterface(pArcSegment);

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Add a collection of segments of the same type to the figure
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CGeometryBuilder::AddSegments(
    _In_ PathPointType type,
    _In_ XUINT32 cPoint,
    _In_reads_(cPoint) const XPOINTF *pPoint
    )
{
    XUINT32 cSegment;
    XUINT32 nSegment;
    XUINT32 nStride;

    // Test for path validity and since polyline segments can only be added from
    // open figures in the OM then it is an error for the figure to be closed.
    if (!m_bValid || !m_bOpen)
        return E_UNEXPECTED;

     // If we have a pending move, store that point here
    if (m_bPendingMove)
    {
        IFC_RETURN(StorePoints(PathPointTypeStart, 1));
        m_bPendingMove = FALSE;
    }

    switch (type)
    {
    case PathPointTypeLine:
        nStride = 1;
        break;

    case PathPointTypeQuadratic:
        nStride = 2;
        break;

    case PathPointTypeBezier:
        nStride = 3;
        break;

    default:
        return E_UNEXPECTED;
    }

    // Compute the number of segments from the number of points and the length of a
    // single segment.  Note it is an error if it isn't an exact multiple.
    cSegment = cPoint / nStride;
    if (cPoint % nStride)
        return E_UNEXPECTED;

    // Store as many points as possible in the current block.
    do
    {
        nSegment = (BLOCK_LIMIT - m_pCurr->m_cPoint) / nStride;
        if (nSegment > cSegment)
            nSegment = cSegment;

        // Note: const_cast here is safe, since the use of m_pAbsolute in this scenario
        // will not write into the points. (Other uses of m_pAbsolute do write, so it can't be const.)
        m_pAbsolute = const_cast<XPOINTF*>(pPoint);
        IFC_RETURN(StorePoints(type, nSegment * nStride));

        cSegment -= nSegment;

        // If all the segments didn't fit then advance to the next block and keep
        // copying them until we're done.
        if (cSegment)
        {
            CPathBlock *temp;

            if (NULL == (temp = m_pCurr->m_pNext))
            {
                temp = new CPathBlock;

                m_pCurr->m_pNext = temp;
            }

            temp->m_cPoint = 0;
            m_pCurr = temp;
            pPoint += (nSegment * nStride);
        }
    } while (cSegment);

    return S_OK;
}

//------------------------------------------------------------------------
//
//------------------------------------------------------------------------
void
CGeometryBuilder::CalculateRoundedCornersRectangle(
    _Out_writes_(8) XPOINTF *pCorners,
    _In_ const XRECTF& rc,
    _In_ const XCORNERRADIUS& rCornerRadius,
    _In_opt_ const XTHICKNESS *pBorders,
    bool fOuter
    )
{
    XPOINTF topLeft;    XPOINTF topRight;
    XPOINTF leftTop;    XPOINTF leftBottom;
    XPOINTF rightTop;   XPOINTF rightBottom;
    XPOINTF bottomLeft; XPOINTF bottomRight;

    XFLOAT fLeftTop;    XFLOAT fLeftBottom;
    XFLOAT fTopLeft;    XFLOAT fTopRight;
    XFLOAT fRightTop;   XFLOAT fRightBottom;
    XFLOAT fBottomLeft; XFLOAT fBottomRight;

    XFLOAT left;
    XFLOAT right;
    XFLOAT top;
    XFLOAT bottom;

    // If the caller wants to take the border into account
    // initialize the borders variables
    if (pBorders)
    {
        left     = 0.5f * pBorders->left;
        right    = 0.5f * pBorders->right;
        top      = 0.5f * pBorders->top;
        bottom   = 0.5f * pBorders->bottom;
    }
    else
    {
        left     = 0.0f;
        right    = 0.0f;
        top      = 0.0f;
        bottom   = 0.0f;
    }


    // The following if/else block initializes the variables
    // of which the points of the path will be created
    // In case of outer, add the border - if any.
    // Otherwise (inner rectangle) subtract the border - if any
    if (fOuter)
    {
        if (IsCloseReal(rCornerRadius.topLeft, 0.0f))
        {
            fLeftTop    = 0.0f;
            fTopLeft    = 0.0f;
        }
        else
        {
            fLeftTop        = rCornerRadius.topLeft     + left;
            fTopLeft        = rCornerRadius.topLeft     + top;
        }
        if (IsCloseReal(rCornerRadius.topRight, 0.0f))
        {
            fTopRight   = 0.0f;
            fRightTop   = 0.0f;
        }
        else
        {
            fTopRight       = rCornerRadius.topRight    + top;
            fRightTop       = rCornerRadius.topRight    + right;
        }
        if (IsCloseReal(rCornerRadius.bottomRight, 0.0f))
        {
            fRightBottom    = 0.0f;
            fBottomRight    = 0.0f;
        }
        else
        {
            fRightBottom    = rCornerRadius.bottomRight + right;
            fBottomRight    = rCornerRadius.bottomRight + bottom;
        }
        if (IsCloseReal(rCornerRadius.bottomLeft, 0.0f))
        {
            fBottomLeft = 0.0f;
            fLeftBottom = 0.0f;
        }
        else
        {
            fBottomLeft     = rCornerRadius.bottomLeft  + bottom;
            fLeftBottom     = rCornerRadius.bottomLeft  + left;
        }

    }
    else
    {
        fLeftTop        = MAX(0.0f, rCornerRadius.topLeft       - left);
        fTopLeft        = MAX(0.0f, rCornerRadius.topLeft       - top);
        fTopRight       = MAX(0.0f, rCornerRadius.topRight      - top);
        fRightTop       = MAX(0.0f, rCornerRadius.topRight      - right);
        fRightBottom    = MAX(0.0f, rCornerRadius.bottomRight   - right);
        fBottomRight    = MAX(0.0f, rCornerRadius.bottomRight   - bottom);
        fBottomLeft     = MAX(0.0f, rCornerRadius.bottomLeft    - bottom);
        fLeftBottom     = MAX(0.0f, rCornerRadius.bottomLeft    - left);
    }

    topLeft.x       = fLeftTop;
    topLeft.y       = 0;

    topRight.x      = rc.Width - fRightTop;
    topRight.y      = 0;

    rightTop.x      = rc.Width;
    rightTop.y      = fTopRight;

    rightBottom.x   = rc.Width;
    rightBottom.y   = rc.Height - fBottomRight;

    bottomRight.x   = rc.Width - fRightBottom;
    bottomRight.y   = rc.Height;

    bottomLeft.x    = fLeftBottom;
    bottomLeft.y    = rc.Height;

    leftBottom.x    = 0;
    leftBottom.y    = rc.Height - fBottomLeft;

    leftTop.x       = 0;
    leftTop.y       = fTopLeft;

    //
    //  check keypoints for overlap and resolve by partitioning radii according to
    //  the percentage of each one.
    //

    //  top edge
    if (topLeft.x > topRight.x)
    {
        XFLOAT v = (fLeftTop) / (fLeftTop + fRightTop) * rc.Width;
        topLeft.x = topRight.x = v;
    }
    //  right edge
    if (rightTop.y > rightBottom.y)
    {
        XFLOAT v = (fTopRight) / (fTopRight + fBottomRight) * rc.Height;
        rightTop.y  = rightBottom.y = v;
    }
    //  bottom edge
    if (bottomRight.x < bottomLeft.x)
    {
        XFLOAT v = (fLeftBottom) / (fLeftBottom + fRightBottom) * rc.Width;
        bottomRight.x   = bottomLeft.x = v;
    }
    // left edge
    if (leftBottom.y < leftTop.y)
    {
        XFLOAT v = (fTopLeft) / (fTopLeft + fBottomLeft) * rc.Height;
        leftBottom.y = leftTop.y = v;
    }

    pCorners[0] = topLeft;
    pCorners[1] = topRight;
    pCorners[2] = leftTop;
    pCorners[3] = leftBottom;
    pCorners[4] = rightTop;
    pCorners[5] = rightBottom;
    pCorners[6] = bottomLeft;
    pCorners[7] = bottomRight;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Creates a D2D path in the shape of a given rounded rectangle.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CGeometryBuilder::DrawRoundedCornersRectangle(
    _Outptr_ IPALAcceleratedGeometry **ppPALGeometry,
    _In_ IPALAcceleratedGraphicsFactory *pD2DFactory,
    _In_ const XRECTF& rc,
    _In_ const XCORNERRADIUS& rCornerRadius,
    _In_opt_ const XTHICKNESS *pBorders,
    bool fOuter
    )
{
    HRESULT hr = S_OK;
    IPALAcceleratedPathGeometry *pPALGeometry = NULL;
    IPALGeometrySink *pPALGeometrySink = NULL;

    IFC(pD2DFactory->CreatePathGeometry(&pPALGeometry));

    IFC(pPALGeometry->Open(&pPALGeometrySink));

    IFC(DrawRoundedCornersRectangle(pPALGeometrySink, rc, rCornerRadius, pBorders, fOuter));

    IFC(pPALGeometrySink->Close());

    *ppPALGeometry = pPALGeometry;
    pPALGeometry = NULL;

Cleanup:
    ReleaseInterface(pPALGeometrySink);
    ReleaseInterface(pPALGeometry);

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Creates a D2D path in the shape of a given rounded rectangle.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CGeometryBuilder::DrawRoundedCornersRectangle(
    _In_ IPALGeometrySink* pSink,
    _In_ const XRECTF& rc,
    _In_ const XCORNERRADIUS& rCornerRadius,
    _In_opt_ const XTHICKNESS *pBorders,
    bool fOuter
    )
{
    HRESULT hr = S_OK;

    // topLeft, topRight, leftTop, leftBottom, rightTop, rightBottom, bottomLeft, bottomRight
    XPOINTF corners[8];
    XSIZEF arcSize;

    CalculateRoundedCornersRectangle(
        corners,
        rc,
        rCornerRadius,
        pBorders,
        fOuter
        );

    for (XUINT32 i = 0; i < 8; i++)
    {
        corners[i].x += rc.X;
        corners[i].y += rc.Y;
    }

    // start at topRight = 1
    // arc to rightTop = 4
    // line to rightBottom = 5
    // arc to bottomRight = 7
    // line to bottomLeft = 6
    // arc to leftBottom = 3
    // line to leftTop = 2
    // arc to topLeft = 0
    // close figure (implicit line back to topRight = 1)

    pSink->BeginFigure(
        corners[1],
        FALSE
        );

    arcSize.width = (corners[4].x - corners[1].x);
    arcSize.height = (corners[4].y - corners[1].y);
    arcSize.width = arcSize.width > 0 ? arcSize.width : -arcSize.width;
    arcSize.height = arcSize.height > 0 ? arcSize.height : -arcSize.height;

    pSink->AddArc(
        corners[4],
        arcSize,
        0.0f,
        TRUE,
        FALSE
        );

    pSink->AddLine(corners[5]);

    arcSize.width = (corners[7].x - corners[5].x);
    arcSize.height = (corners[7].y - corners[5].y);
    arcSize.width = arcSize.width > 0 ? arcSize.width : -arcSize.width;
    arcSize.height = arcSize.height > 0 ? arcSize.height : -arcSize.height;

    pSink->AddArc(
        corners[7],
        arcSize,
        0.0f,
        TRUE,
        FALSE
        );

    pSink->AddLine(corners[6]);

    arcSize.width = (corners[3].x - corners[6].x);
    arcSize.height = (corners[3].y - corners[6].y);
    arcSize.width = arcSize.width > 0 ? arcSize.width : -arcSize.width;
    arcSize.height = arcSize.height > 0 ? arcSize.height : -arcSize.height;

    pSink->AddArc(
        corners[3],
        arcSize,
        0.0f,
        TRUE,
        FALSE
        );

    pSink->AddLine(corners[2]);

    arcSize.width = (corners[0].x - corners[2].x);
    arcSize.height = (corners[0].y - corners[2].y);
    arcSize.width = arcSize.width > 0 ? arcSize.width : -arcSize.width;
    arcSize.height = arcSize.height > 0 ? arcSize.height : -arcSize.height;

    pSink->AddArc(
        corners[0],
        arcSize,
        0.0f,
        TRUE,
        FALSE
        );

    pSink->EndFigure(TRUE);

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Creates an accelerated geometry in the shape of multiple disjoint lines.
//      Set of points given should have even number of points.
//      A line is constructed between 1st and 2nd point, 3rd and 4th and so on.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CGeometryBuilder::DrawMultipleLines(
    _Outptr_ IPALAcceleratedGeometry **ppPALGeometry,
    _In_ IPALAcceleratedGraphicsFactory *pD2DFactory,
    XUINT32 cPoints,
    _In_reads_(cPoints) const XPOINTF *pPoints
    )
{
    HRESULT hr = S_OK;
    IPALAcceleratedPathGeometry *pPALGeometry = NULL;
    IPALGeometrySink *pPALGeometrySink = NULL;

    ASSERT(cPoints % 2 == 0);

    if (cPoints > 0)
    {
        IFC(pD2DFactory->CreatePathGeometry(
            &pPALGeometry
            ));

        IFC(pPALGeometry->Open(&pPALGeometrySink));

        for (XUINT32 i = 0; i < cPoints - 1; i += 2)
        {
            pPALGeometrySink->BeginFigure(
                pPoints[i],
                FALSE
                );

            pPALGeometrySink->AddLine(pPoints[i + 1]);

            pPALGeometrySink->EndFigure(FALSE);
        }

        IFC(pPALGeometrySink->Close());
    }

    ReplaceInterface(*ppPALGeometry, pPALGeometry);

Cleanup:
    ReleaseInterface(pPALGeometrySink);
    ReleaseInterface(pPALGeometry);

    RRETURN(hr);
}


//------------------------------------------------------------------------
//
//  CGeometryBuilder::DrawPoints
//
//  Synopsis:
//      Creates an accelerated geometry from a series of points and types.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CGeometryBuilder::AddPoints(
    _Inout_ IPALAcceleratedPathGeometry *pPALGeometry,
    _In_reads_(cPoints) XPOINTF* pPoints,
    XUINT32 cPoints,
    _In_reads_(cPoints) XPATHTYPE* pTypes,
    XcpFillMode fillMode
    )
{
    HRESULT hr = S_OK;
    IPALGeometrySink* pPALGeometrySink = NULL;
    XUINT32 itrPoint = 0;

    IFC(pPALGeometry->Open(&pPALGeometrySink));
    pPALGeometrySink->SetFillMode(static_cast<GeometryFillMode>(fillMode));

    while(itrPoint < cPoints)
    {
        switch(pTypes[itrPoint] & PathPointTypePathTypeMask)
        {
            case PathPointTypeStart:
            {
                pPALGeometrySink->BeginFigure(pPoints[itrPoint], FALSE);
                itrPoint++;
                break;
            }
            case PathPointTypeLine:
            {
                pPALGeometrySink->AddLine(pPoints[itrPoint]);
                itrPoint++;
                break;
            }
            case PathPointTypeQuadratic:
            {
                IFCEXPECT(itrPoint+1 < cPoints
                    && (pTypes[itrPoint+1] & PathPointTypePathTypeMask) == PathPointTypeQuadratic);
                pPALGeometrySink->AddQuadraticBezier(pPoints[itrPoint], pPoints[itrPoint+1]);
                itrPoint += 2;
                break;
             }
            case PathPointTypeBezier:
            {
                IFCEXPECT(itrPoint+2 < cPoints
                   && (pTypes[itrPoint+1] & PathPointTypePathTypeMask) == PathPointTypeBezier
                   && (pTypes[itrPoint+2] & PathPointTypePathTypeMask) == PathPointTypeBezier);
                pPALGeometrySink->AddBezier(
                    pPoints[itrPoint],
                    pPoints[itrPoint+1],
                    pPoints[itrPoint+2]);
                itrPoint += 3;
                break;
             }
             default:
                 IFC(E_FAIL);
                 break;
        }
        if (pTypes[itrPoint-1] & PathPointTypeCloseSubpath)
        {
            pPALGeometrySink->EndFigure(FALSE);
        }
    }
    IFC(pPALGeometrySink->Close());

Cleanup:
    ReleaseInterface(pPALGeometrySink);
    RRETURN(hr);
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Opens a geometry builder to build new CPathFigures rather than
//      new CRasterizerPaths.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CGeometryBuilder::OpenPathGeometryBuilder()
{
    HRESULT hr = S_OK;
    CREATEPARAMETERS cp(m_pCoreNoRef);

    CPathFigureCollection *pPathFigures = NULL;
    IFC(CPathFigureCollection::Create(reinterpret_cast<CDependencyObject **>(&pPathFigures), &cp));

    ReplaceInterface(m_pPathFigures, pPathFigures);

    IFC(OpenGeometry());

Cleanup:
    ReleaseInterface(pPathFigures);

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Closes the geometry and adds the accumulated CPathFigures to the
//      geometry
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CGeometryBuilder::ClosePathGeometryBuilder(
    _In_ CPathGeometry *pPathGeometry
    )
{
    HRESULT hr = S_OK;

    ReplaceInterface(pPathGeometry->m_pFigures, m_pPathFigures);
    ReleaseInterface(m_pPathFigures);

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Destructor for base geometry object
//
//------------------------------------------------------------------------
CGeometry::~CGeometry()
{
    ReleaseInterface(m_pTransform);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Creates an instance of the object
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CGeometry::Create(
    _Outptr_ CDependencyObject **ppObject,
    _In_ CREATEPARAMETERS *pCreate
    )
{
// We want to create a path geometry if we're given a parse string

    if (pCreate->m_value.GetType() == valueString)
    {
        return CPathGeometry::Create(ppObject, pCreate);
    }

// If we can't return an object, fail

    return E_FAIL;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns bounds for this geometry, called by pInvoke from managed
//      Geometry.Bounds property getter.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CGeometry::GetBounds(
    _In_ CDependencyObject *pObject,
    _In_ XUINT32 cArgs,
    _In_reads_(cArgs) CValue *pArgs,
    _In_opt_ IInspectable* pValueOuter,
    _Out_ CValue *pResult
    )
{
    HRESULT hr = S_OK;
    CGeometry* pGeometry = NULL;
    XRECTF_RB geometryBounds;
    CRect* pBounds = NULL;

    if (pObject == NULL)
    {
        IFC(E_FAIL);
    }

    IFC(DoPointerCast(pGeometry, pObject));

    //
    // Get natural bounds of geometry.
    //
    IFC(pGeometry->GetBounds(&geometryBounds));

    //
    // Create rectangle to return.
    //
    {
        CREATEPARAMETERS cp(pGeometry->GetContext());

        IFC(CRect::Create((CDependencyObject **)&pBounds, &cp));

        pBounds->m_rc = ToXRectF(geometryBounds);

        pResult->SetObjectNoRef(pBounds);
        pBounds = NULL;
    }

Cleanup:
    ReleaseInterface(pBounds);

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Appends the transform needed to stretch
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CGeometry::ComputeStretchMatrix(
    DirectUI::Stretch stretch,
    _In_ const XRECTF *pRenderBounds,
    _In_ const XRECTF *pNaturalBounds,
    _Inout_ CMILMatrix *pTransform
    )
{
    ASSERT(pRenderBounds);
    ASSERT(pNaturalBounds);
    ASSERT(pTransform);

    // Early out if natural w/h is < 0
    ASSERT( pNaturalBounds->Width >= 0 );
    ASSERT( pNaturalBounds->Height >= 0 );

    XFLOAT  rNaturalWidth = pNaturalBounds->Width > 0 ? pNaturalBounds->Width : 1.0f;
    XFLOAT  rNaturalHeight = pNaturalBounds->Height > 0 ? pNaturalBounds->Height : 1.0f;
    XFLOAT  rRenderWidth = pRenderBounds->Width < XFLOAT_MAX ? pRenderBounds->Width : rNaturalWidth;
    XFLOAT  rRenderHeight = pRenderBounds->Height  < XFLOAT_MAX ? pRenderBounds->Height : rNaturalHeight;

    // start with identity
    CMILMatrix matTransform(TRUE);

    // We need a valid set of dimensions to stretch into, otherwise just use identity
    if ( rRenderWidth > 0 && rRenderHeight > 0 )
    {
        XFLOAT  rDeltaX = 0.0f;
        XFLOAT  rDeltaY = 0.0f;
        XFLOAT  rScaleX = rRenderWidth / rNaturalWidth;
        XFLOAT  rScaleY = rRenderHeight / rNaturalHeight;
        XFLOAT  rAspectRatioXY = rScaleX / rScaleY;

        // consider offsets
        CMILMatrix matStrokeAdjust(TRUE);
        CMILMatrix matBoundsAdjust(TRUE);
        CMILMatrix matScale(TRUE);
        CMILMatrix matCentering(TRUE);

        // Compute based on stretch mode
        switch ( stretch )
        {
            // Stretch both ways, all of the shape is visible.
            case DirectUI::Stretch::Fill:
                rDeltaX = 0.0f;
                rDeltaY = 0.0f;
                break;

            // Stretch to fill the smaller dimension and keep aspect ratio:
            //   all of the shape is visible, part of the bounds may not be filled.
            case DirectUI::Stretch::Uniform:
                if ( rAspectRatioXY > 1.0)
                {
                    rScaleX = rScaleY;
                    rDeltaY = 0.0f;
                    rDeltaX = rRenderWidth - ( rNaturalWidth * rScaleX );
                }
                else
                {
                    rScaleY = rScaleX;
                    rDeltaX = 0.0f;
                    rDeltaY = rRenderHeight - ( rNaturalHeight * rScaleY );
                }
                break;

            // Stretch to fill the larger dimension and keep aspect ratio:
            //   portions of the shape may be clipped off, all of the bounds is filled.
            case DirectUI::Stretch::UniformToFill:
                if ( rAspectRatioXY > 1.0)
                {
                    rScaleY = rScaleX;
                    rDeltaX = 0.0f;
                }
                else
                {
                    rScaleX = rScaleY;
                    rDeltaY = 0.0f;
                }
                break;

            // No stretching, just use the natural bounds
            case DirectUI::Stretch::None:
                ASSERT( FALSE );
                // We should never get here - Stretch=None should be the default and given an early out path
                //    that avoids double bounding the shape.
                break;

            // Unsupported mode
            default:
                ASSERT( FALSE );
                IFC_RETURN(E_UNEXPECTED);
                break;
        }

        // Right alignment is computed in the stretch calculation,
        //   so center is half of that.
        rDeltaX *= 0.5f;
        // Bottom alignment is computed in the stretch calculation,
        //   so center is half of that.
        rDeltaY *= 0.5f;

        // Update adjust scale matrix
        matScale.SetM11(rScaleX);
        matScale.SetM22(rScaleY);
        // Update adjust centering matrix
        matCentering.SetDx(rDeltaX);
        matCentering.SetDy(rDeltaY);

        // Now update the offsets ...
        // ... Move the result down and left by the stroke offset
        matStrokeAdjust.SetDx(pRenderBounds->X);
        matStrokeAdjust.SetDy(pRenderBounds->Y);
        // ... Move the natural bounds back to 0,0 for stretching
        matBoundsAdjust.SetDx(-pNaturalBounds->X);
        matBoundsAdjust.SetDy(-pNaturalBounds->Y);

        // Matrix operation rationale:
        // #1 - The natural bounds may include some offset: eliminate that first
        // #2 - Next is the actual scale from one size to the other
        // #3 - Next is the stroke offset
        // #4 - Next is the centering offset from aspect ratio preserving stretches
        matTransform.Append(matBoundsAdjust);
        matTransform.Append(matScale);
        matTransform.Append(matStrokeAdjust);
        matTransform.Append(matCentering);
    }

    *pTransform = matTransform;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Destructor for geometry object
//
//------------------------------------------------------------------------
CPathGeometry::~CPathGeometry()
{
    ReleaseInterface(m_pFigures);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Creates an instance of the object
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CPathGeometry::Create(
    _Outptr_ CDependencyObject **ppObject,
    _In_ CREATEPARAMETERS *pCreate
    )
{
    HRESULT hr = S_OK;
    CPathGeometry *_this = NULL;

    // There is only one kind of path geometry.
    _this = new CPathGeometry(pCreate->m_pCore);

    ASSERT((pCreate->m_value.GetType() == valueString) || (pCreate->m_value.GetType() == valueAny));
    if (pCreate->m_value.GetType() == valueString)
    {
        CGeometryBuilder *pBuilder;
        pCreate->m_pCore->ResetGeometryBuilder(0);

        //
        // If PC is on, there's no way to tell whether this path will be render with D2D or with
        // software, so we need to keep the figures around. If we render with software, it will
        // generate the rasterizer path from the figures.
        //
        IFC(pCreate->m_pCore->GetGeometryBuilder(
            0,
            &pBuilder,
            TRUE));

        // If we're given a string then parse it now.
        XUINT32 cString = 0;
        const WCHAR* pString = pCreate->m_value.AsEncodedString().GetBufferAndCount(&cString);
        IFC(_this->ParseGeometry(pBuilder, cString, pString, TRUE));

        IFC(pBuilder->ClosePathGeometryBuilder(_this));
    }

// Return the object to the caller

    *ppObject = static_cast<CDependencyObject *>(_this);
    _this = NULL;

Cleanup:
    ReleaseInterface(_this);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Parses the geometry mini-language.  Could be used to build either or
//      both the Path.Data and Path.Clip attributes.
//
//------------------------------------------------------------------------

#define MAX_NUMERICS    8

_Check_return_ HRESULT
CPathGeometry::ParseGeometry(
    _In_ CGeometryBuilder *pBuilder,
    _In_ XUINT32 cData,
    _In_reads_(cData) const WCHAR *pRasterizerPath,
    _In_ XINT32 bAllowFill
    )
{
    XINT32      bComma = FALSE;         // Is a comma allowed at this time
    WCHAR       cmd = L'\0';            // Current command marker
    XUINT32     cUnsigned = 0;          // For arcs the first two values can't be signed
    XUINT32     cNumeric = 0;           // Remaining numeric values to parse in this command
    XUINT32     iNumeric = 0;           // Index of current numeric value
    XFLOAT      aNumeric[MAX_NUMERICS]; // Buffer for numeric values in a command
    XPOINTF     aptArc[3];              // Buffer for shuffling arc values
    XINT32      mBoolean = 0;           // Mask for field type
    XINT32      flags;

// While there are characters left keep parsing

    while (cData)
    {
    // Consume white space and optional comma

        while (cData && xisspace(*pRasterizerPath))
        {
            pRasterizerPath++;
            cData--;
        }

    // We might be allowed to have a comma

        if (bComma && cData && (L',' == *pRasterizerPath))
        {
            bComma = FALSE;
            cData--;
            pRasterizerPath++;

        // After the comma there can be more white space

            while (cData && xisspace(*pRasterizerPath))
            {
                pRasterizerPath++;
                cData--;
            }
        }

    // We've found a token. If cNumeric is non-zero it must be a numeric value.

        if (cNumeric)
        {
        // Ensure future code changes don't cause a buffer overrun in aNumeric

            if (iNumeric >= MAX_NUMERICS)
            {
                IFC_RETURN(E_UNEXPECTED);
            }

        // Read the boolean or floating point value from the data
#if DBG
            const WCHAR* pPrevious = pRasterizerPath;
#endif
            const XUINT32 cPrevious = cData;

            if (mBoolean & 1)
            {
                if (cData && ((L'0' == *pRasterizerPath) || (L'1' == *pRasterizerPath)))
                {
                    *((XINT32 *) &aNumeric[iNumeric++]) = XINT32(*pRasterizerPath - L'0');

                // Update pRasterizerPath so it appears we read some data.  Note that if
                // pRasterizerPath isn't adjusted then the code below will interpret it
                // as a parse failure and exit with an error.

                    pRasterizerPath = pRasterizerPath + 1;
                    cData = cData - 1;
                }
            }
            else
            {
                if (cUnsigned)
                {
                    cUnsigned--;
                    IFC_RETURN(NonSignedFromString(cData, pRasterizerPath, &cData, &pRasterizerPath, &aNumeric[iNumeric++]));
                }
                else
                {
                    IFC_RETURN(FloatFromString(cData, pRasterizerPath, &cData, &pRasterizerPath, &aNumeric[iNumeric++]));
                }
            }

            ASSERT(cData <= cPrevious);
            ASSERT(pPrevious <= pRasterizerPath && (pPrevious + cPrevious) <= (pRasterizerPath + cData));

        // If we failed to parse a value then an error has occurred.

            if (cData == cPrevious)
            {
                IFC_RETURN(E_UNEXPECTED);
            }

        // Shift the boolean mask and adjust the remaining count of numerics

            mBoolean >>= 1;
            if (--cNumeric)
            {
            // If there are more numeric values then allow a comma

                bComma = TRUE;
            }
            else
            {
            // At the end of a command there can be no more commas

                bComma = FALSE;

            // We parsed all the values necessary for the current command.  See
            // if it contained relative offsets or absolute positions

                flags = xislower(cmd) ? XcpPointType_Relative : XcpPointType_Absolute;

            // Now we can submit the object to the geometry builder

                switch (cmd & 0x00df)
                {
            // Add an arc to the geometry.  We're going to have to shuffle the
            // values around to get this to work.  Note that we can't copy the
            // whole point from the sixth and seventh fields at once since this
            // could cause an unaligned address exception on some architectures
            // since the 64 bit field isn't aligned properly.

                case 'A':
                    aptArc[0].x = aNumeric[5];
                    aptArc[0].y = aNumeric[6];
                    aptArc[1] = *((XPOINTF *) &aNumeric[0]);
                    IFC_RETURN(pBuilder->AddArc(flags, aptArc, aNumeric[2], *((XINT32 *) &aNumeric[3]), *((XINT32 *) &aNumeric[4])));
                    break;

            // Add a cubic Bezier curve to the geometry

                case 'S':
                    IFC_RETURN(pBuilder->ComputeReflection(flags, PathPointTypeBezier, (XPOINTF *) &aNumeric[0]));

                case 'C':
                    IFC_RETURN(pBuilder->AddBezier(flags, (XPOINTF *) &aNumeric[0]));
                    break;

            // Set the fill mode of the geometry

                case 'F':
                    m_fillMode = *((XINT32 *) &aNumeric[0]) ? XcpFillModeWinding : XcpFillModeAlternate;
                    break;

            // Add a line segment to the geometry

                case 'H':
                    IFC_RETURN(pBuilder->AddLine(flags | VALID_X, (XPOINTF *) &aNumeric[0]));
                    break;

                case 'L':
                    IFC_RETURN(pBuilder->AddLine(flags | VALID_XY, (XPOINTF *) &aNumeric[0]));
                    break;

                case 'V':
                    IFC_RETURN(pBuilder->AddLine(flags | VALID_Y, (XPOINTF *) &aNumeric[0]));
                    break;

            // Start a new figure in the geometry

                case 'M':
                    IFC_RETURN(pBuilder->OpenFigure(flags, (XPOINTF *) &aNumeric[0]));

                // Any points after the 'M' are implicitly a lineto unless there is another command
                // Use cmd-- to convert an 'M' to 'L' and an 'm' to 'l'

                    cmd--;
                    break;

            // Add a quadratic Bezier curve to the geometry

                case 'T':
                    IFC_RETURN(pBuilder->ComputeReflection(flags, PathPointTypeQuadratic, (XPOINTF *) &aNumeric[0]));

                case 'Q':
                    IFC_RETURN(pBuilder->AddQuadratic(flags, (XPOINTF *) &aNumeric[0]));
                    break;
                }
            }
        }
        else if (cData)
        {
        // If the next token is a numeric value and we have a previous command
        // then we can repeat the previous command.

            if (xisfleading(*pRasterizerPath))
            {
                if (!cmd)
                {
                    IFC_RETURN(E_UNEXPECTED);
                }
            }
            else if (L',' == *pRasterizerPath)
            {
            // Special check for interior commas.  On the surface this seems to
            // violate the SVG specification but it does not.  For example take
            // the 'Q' command. In our implementation this has 4 parameters but
            // in the SVG specification it has a multiple of 4 parameters.  So
            // we check for the comma here as a repeat of the previous command
            // and consume it before continuing.

                if (!cmd)
                {
                    IFC_RETURN(E_UNEXPECTED);
                }

                pRasterizerPath++;
                cData--;
            }
            else
            {
            // Get the new command marker

                cmd = *pRasterizerPath;
                pRasterizerPath++;
                cData--;
            }

        // Reset the index and assume all values are signed floats

            iNumeric = 0;
            mBoolean = 0;
            cUnsigned = 0;

        // Process the new command

            switch (cmd & 0x00df)
            {
            case 'A':   // Adds an arc.
                cNumeric = 7;
                cUnsigned = 2;      // The radii must not be signed
                mBoolean = 0x0018;  // The flags are booleans
                break;

            case 'C':   // Adds 3 control points to define a cubic Bezier
                cNumeric = 6;
                break;

            case 'F':   // Sets the fill mode (0 = Alternate, 1 = Winding)
                if (!bAllowFill)
                {
                    IFC_RETURN(E_UNEXPECTED);
                }

                cNumeric = 1;
                mBoolean = 0x0001;
                break;

            case 'H':   // Horizontal line to
                cNumeric = 1;
                break;

            case 'L':   // Arbitrary line to
                cNumeric = 2;
                break;

            case 'M':   // Move to (starts a figure)
                cNumeric = 2;
                break;

            case 'Q':   // Adds 2 control points to define a quadratic curve
                cNumeric = 4;
                break;

            case 'S':   // Adds 2 control points to continue a cubic Bezier
                iNumeric = 2;
                cNumeric = 4;
                break;

            case 'T':   // Adds 1 control point to continue a quadratic curve
                iNumeric = 2;
                cNumeric = 2;
                break;

            case 'V':   // Vertical line to (Y value is passed in X)
                cNumeric = 1;
                break;

            case 'Z':   // Close a figure
                IFC_RETURN(pBuilder->CloseFigure());
                cmd = 0;
                break;

            default:
                IFC_RETURN(E_UNEXPECTED);
            }
        }

    // After the first token we can no longer allow F{0|1} to be parsed.

        bAllowFill = FALSE;
    }

// If we got here with parameters remaining to be read it is an error

    if (cNumeric)
    {
        IFC_RETURN(E_UNEXPECTED);
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  CPathGeometry::GetPrintGeometryVirtual
//
//  Synopsis:
//      Creates and returns a print geometry for this Silverlight geometry.
//      It does not account for any transforms, stretching or clipping.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CPathGeometry::GetPrintGeometryVirtual(
    _In_ const D2DPrecomputeParams& cp,
    _In_ const D2DRenderParams &printParams,
    IPALAcceleratedGeometry** ppGeometry
    )
{
    HRESULT hr = S_OK;
    IPALAcceleratedPathGeometry *pPALGeometry = NULL;
    IPALGeometrySink *pPALGeometrySink = NULL;

    IFCEXPECT(m_pFigures);
    IFC(cp.GetFactory()->CreatePathGeometry(&pPALGeometry));

    IFC(pPALGeometry->Open(&pPALGeometrySink));
    pPALGeometrySink->SetFillMode(static_cast<GeometryFillMode>(m_fillMode));
    IFC(m_pFigures->AddAcceleratedFigures(pPALGeometrySink));
    IFC(pPALGeometrySink->Close());

    SetInterface(*ppGeometry, pPALGeometry);

Cleanup:
    ReleaseInterface(pPALGeometrySink);
    ReleaseInterface(pPALGeometry);
    RRETURN(hr);
}


//------------------------------------------------------------------------
//
//  CPathGeometry::DrawLines
//
//  Synopsis:
//      Fills in the path with the given polyline.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CPathGeometry::DrawLines(
    _In_reads_(uiPointCount) const XPOINTF *pPoints,
    XUINT32 uiPointCount,
    bool fIsClosed
    )
{
    HRESULT hr = S_OK;
    CREATEPARAMETERS cp(GetContext());
    CPathFigureCollection *pPathFigureCollection = NULL;
    CPathFigure *pPathFigure = NULL;
    CPathSegmentCollection *pPathSegmentCollection = NULL;
    CLineSegment *pLineSegment = NULL;

    ASSERT(uiPointCount > 1);

    if (!m_pFigures)
    {
        IFC(CPathFigureCollection::Create(
            reinterpret_cast<CDependencyObject **>(&pPathFigureCollection),
            &cp
            ));
        m_pFigures = pPathFigureCollection;     // Steal ref
        pPathFigureCollection = NULL;
    }

    IFC(m_pFigures->Clear());

    IFC(CPathFigure::Create(
        reinterpret_cast<CDependencyObject **>(&pPathFigure),
        &cp
        ));

    IFC(CPathSegmentCollection::Create(
        reinterpret_cast<CDependencyObject **>(&pPathSegmentCollection),
        &cp
        ));

    for (XUINT32 i = 1; i < uiPointCount; i++)
    {
        IFC(CLineSegment::Create(
            reinterpret_cast<CDependencyObject **>(&pLineSegment),
            &cp
            ));
        pLineSegment->m_pt = pPoints[i];
        IFC(pPathSegmentCollection->Append(pLineSegment));   // Append AddRefs
        ReleaseInterface(pLineSegment);
    }

    pPathFigure->m_pSegments = pPathSegmentCollection;  // Steal ref
    pPathSegmentCollection = NULL;

    pPathFigure->m_bClosed = !!fIsClosed;
    pPathFigure->m_ptStart = pPoints[0];
    IFC(m_pFigures->Append(pPathFigure));   // Append AddRefs

Cleanup:
    ReleaseInterface(pPathFigureCollection);
    ReleaseInterface(pPathFigure);
    ReleaseInterface(pPathSegmentCollection);
    ReleaseInterface(pLineSegment);

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Add geometry to the specified sink.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CPathGeometry::VisitSinkInternal(
    _In_ IPALGeometrySink* pSink
    )
{
    pSink->SetFillMode((m_fillMode == XcpFillModeAlternate || m_fillMode == XcpFillModeAlternateClipped) ? GeometryFillMode::Alternate : GeometryFillMode::Winding);

    if (m_pFigures != NULL)
    {
        IFC_RETURN(m_pFigures->AddAcceleratedFigures(pSink));
    }

    return S_OK;
}

WUComp::ICompositionGeometry* CPathGeometry::GetCompositionGeometry(_In_ VisualContentRenderer* renderer)
{
    if (IsGeometryDirty() || m_wucGeometry == nullptr)
    {
        wrl::ComPtr<WUComp::ICompositionPathGeometry> pathGeo;

        if (m_wucGeometry == nullptr)
        {
            IFCFAILFAST(renderer->GetCompositor5()->CreatePathGeometry(&pathGeo));
            IFCFAILFAST(pathGeo.As(&m_wucGeometry))
        }
        else
        {
            IFCFAILFAST(m_wucGeometry.As(&pathGeo));
        }

        CD2DFactory* const pD2DFactory = renderer->GetSharedD2DFactoryNoRef();

        D2DPrecomputeParams const cp(pD2DFactory, nullptr);
        D2DRenderParams const printParams(nullptr, nullptr, TRUE);

        wrl::ComPtr<IPALAcceleratedGeometry> pathGeometry;
        IFCFAILFAST(GetPrintGeometry(cp, printParams, &pathGeometry));

        wrl::ComPtr<ID2D1Geometry> pD2DGeometry;
        IFCFAILFAST(UnwrapD2DGeometry(pathGeometry.Get(), &pD2DGeometry));

        // Update the wrapper CGeometrySource class
        if (m_geometrySource == nullptr)
        {
            m_geometrySource.Attach(new CGeometrySource());
        }
        m_geometrySource->UpdateD2DGeometry(pD2DGeometry.Get());

        wrl::ComPtr<WUComp::ICompositionPath> compositionPath;
        WUComp::ICompositionPathFactory* pathFactoryNoRef = nullptr;
        if (WinAppSdk::Containment::IsChangeEnabled<WINAPPSDK_CHANGEID_46468883>())
        {
            pathFactoryNoRef = ActivationFactoryCache::GetActivationFactoryCache()->GetPathFactory();
        }
        else
        {
            pathFactoryNoRef = renderer->GetDCompTreeHost()->GetPathFactory();
        }
        IFCFAILFAST(pathFactoryNoRef->Create(m_geometrySource.Get(), &compositionPath));

        IFCFAILFAST(pathGeo->put_Path(compositionPath.Get()));

        SetWUCGeometryDirty(false);
    }

    return m_wucGeometry.Get();
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Add geometry to the specified sink.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CGeometryCollection::VisitSink(
    _In_ IPALGeometrySink* pSink
    )
{
    HRESULT hr = S_OK;
    GeometryGroupSink* pGroupSink = NULL;

    pGroupSink = new GeometryGroupSink(pSink);

    for (auto item : (*this))
    {
        IFC(static_cast<CGeometry*>(item)->VisitSink(pGroupSink));
    }

Cleanup:
    ReleaseInterface(pGroupSink);

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns a D2D geometry for this collection for printing purposes.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CGeometryCollection::GetPrintGeometry(
    _In_ const D2DPrecomputeParams &cp,
    _In_ const D2DRenderParams &printParams,
    XcpFillMode fillMode,
    _Outptr_ IPALAcceleratedGeometry **ppPALGeometry
    )
{
    HRESULT hr = S_OK;
    IPALAcceleratedGeometry *pPALGeometry = NULL;
    XUINT32 uiCount = 0;

    IPALAcceleratedGeometry **ppPALGeometries = new(ZERO_MEM_FAIL_FAST) IPALAcceleratedGeometry*[GetCount()];
    uiCount = GetCount();

        // Create D2D geometries for each geometry in the collection.
        for (XUINT32 i = 0; i != uiCount; i++)
        {
            IFC(static_cast<CGeometry*>((*this)[i])->GetPrintGeometry(cp, printParams, &(ppPALGeometries[i])));
        }

        // Create a D2D geometry group from the D2D geometries.
        IFC(cp.GetFactory()->CreateGeometryGroup(
            fillMode == XcpFillModeWinding || fillMode == XcpFillModeWindingClipped,
            ppPALGeometries,
            uiCount,
            &pPALGeometry
            ));

        *ppPALGeometry = pPALGeometry;
        pPALGeometry = NULL;

Cleanup:
    ReleaseInterface(pPALGeometry);
    for (XUINT32 i = 0; i < uiCount; i++)
    {
        ReleaseInterface(ppPALGeometries[i]);
    }
    delete[] ppPALGeometries;
    RRETURN(hr);
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Destructor for transform group object
//
//------------------------------------------------------------------------
CGeometryGroup::~CGeometryGroup()
{
    ReleaseInterface(m_pChild);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Add geometry to the specified sink.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CGeometryGroup::VisitSinkInternal(
    _In_ IPALGeometrySink* pSink
    )
{
    pSink->SetFillMode((m_fillMode == XcpFillModeAlternate || m_fillMode == XcpFillModeAlternateClipped) ? GeometryFillMode::Alternate : GeometryFillMode::Winding);

    if (m_pChild != NULL)
    {
        IFC_RETURN(m_pChild->VisitSink(pSink));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Gets a D2D geometry for this group.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CGeometryGroup::GetPrintGeometryVirtual(
    _In_ const D2DPrecomputeParams& cp,
    _In_ const D2DRenderParams &printParams,
    _Outptr_ IPALAcceleratedGeometry** ppGeometry
    )
{
    HRESULT hr = S_OK;
    IPALAcceleratedGeometry *pPALGeometry = NULL;

    if (m_pChild)
    {
        IFC(m_pChild->GetPrintGeometry(
            cp,
            printParams,
            m_fillMode,
            &pPALGeometry
            ));
    }
    SetInterface(*ppGeometry, pPALGeometry);

Cleanup:
    ReleaseInterface(pPALGeometry);
    RRETURN(hr);
}

WUComp::ICompositionGeometry* CGeometryGroup::GetCompositionGeometry(_In_ VisualContentRenderer* renderer)
{
    // WUCShapes_TODO
    // Create shape-specific geometry
    // Set geometry parameters to reflect this particular shape
    // Set the _Out_ geometry to our shape-specific geometry
    return m_wucGeometry.Get();
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the element bounds after accounting for the stroke. Uses
//      the bounds passed in if available, otherwise falls back to the
//      natural bounds of the element.
//
//-------------------------------------------------------------------------
void
CGeometry::GetStrokeAdjustedElementBounds(
    _In_opt_ XRECTF *pElementBounds,
    _In_ const XRECTF& naturalBounds,
    XFLOAT penThickness,
    _Out_ XRECTF *pStrokeAdjustedElementBounds
    )
{
    XRECTF elementBounds;

    if (pElementBounds != NULL)
    {
        ASSERT(pElementBounds->Width > 0.0f && pElementBounds->Height > 0.0f);

        //
        // Compensate for stroke thickness. The element bounds came from a layout size or an
        // explicit size and should be interpreted as the outer bound of the shape, which
        // includes the entire stroke. We need the bounds of the shape at the middle of the
        // stroke. Therefore the bounds need to be inset by half stroke thickness on each
        // side (a whole stroke thickness total).
        //
        elementBounds.Width = MAX(pElementBounds->Width - penThickness, 0.0f);
        elementBounds.Height = MAX(pElementBounds->Height - penThickness, 0.0f);

        //
        // ComputeStretchMatrix requires that the target dimensions be greater than 0, so
        // if the bounds (after compensating by the pen) have a zero dimension, set it to the
        // pen's thickness.
        //
        if (elementBounds.Width == 0.0f)
        {
            elementBounds.Width = penThickness;
        }
        if (elementBounds.Height == 0.0f)
        {
            elementBounds.Height = penThickness;
        }
    }
    else
    {
        //
        // Do not compensate for stroke thickness. There are no element bounds, which means
        // there is no layout size and no explicit size. The bounds come from the natural size
        // of the geometry, which is already defined at the middle of the stroke.
        //
        elementBounds.Width = naturalBounds.Width;
        elementBounds.Height = naturalBounds.Height;
    }
    elementBounds.X = penThickness / 2.0f;
    elementBounds.Y = penThickness / 2.0f;

    *pStrokeAdjustedElementBounds = elementBounds;
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Draws/fills the D2D geometry.
//
//-------------------------------------------------------------------------
/* static */ _Check_return_ HRESULT
CGeometry::DrawAccelerated(
    _In_ IPALAcceleratedGeometry *pPALAcceleratedGeometry,
    _In_ const D2DRenderParams& d2dRP,
    _In_ const CMILMatrix *pWorldTransform,
    XFLOAT strokeThickness,
    _In_opt_ IPALStrokeStyle *pStrokeStyle,
    XFLOAT opacity,
    _In_ IPALAcceleratedBrush *pPALBrush,
    _In_ AcceleratedBrushParams *pPALBrushParams
    )
{
    IPALAcceleratedRenderTarget *pRenderTargetNoRef = d2dRP.GetD2DRenderTarget();

    // There isn't always a brush. e.g. if an ImageBrush has a bad image
    if (pPALBrush != NULL)
    {
        XRECTF_RB geometryBounds;
        bool pushedBrushClipLayer = false;
        bool pushedAxisAlignedBrushClip = false;
        bool drawStroke = strokeThickness > 0.0f;
        XRECTF_RB *pContentBounds = NULL;

        IFC_RETURN(pPALBrush->SetTransform(
            &pPALBrushParams->m_transform
            ));

        if (!drawStroke)
        {
            //
            // When filling a geometry, determine the bounds of the fill
            // If the brush clip contains the fill bounds, then PushBrushClip will do nothing
            // This is not done for strokes because GetWidenedBounds is too expensive
            //
            IFC_RETURN(pPALAcceleratedGeometry->GetBounds(&geometryBounds));
            pContentBounds = &geometryBounds;
        }

        // If the bounds of the object are completely contained by the brush clip
        // then the brush clip is not useful

        IFC_RETURN(pPALBrushParams->PushBrushClip(
            !d2dRP.GetIsPrintTarget() /* allowPushAxisAlignedClip */,
            pWorldTransform,
            pRenderTargetNoRef,
            pContentBounds,
            &pushedBrushClipLayer,
            &pushedAxisAlignedBrushClip
            ));

        if (drawStroke)
        {
            IFC_RETURN(pPALAcceleratedGeometry->Draw(
                pRenderTargetNoRef,
                pPALBrush,
                strokeThickness,
                pStrokeStyle,
                opacity
                ));
        }
        else
        {
            IFC_RETURN(pPALAcceleratedGeometry->Fill(
                pRenderTargetNoRef,
                pPALBrush,
                opacity
                ));
        }

        IFC_RETURN(CUIElement::D2DPopClipHelper(
            pRenderTargetNoRef,
            pushedBrushClipLayer,
            pushedAxisAlignedBrushClip
            ));
    }

    return S_OK;
}


//------------------------------------------------------------------------
//
//  CGeometry::GetPrintGeometry
//
//  Synopsis:
//      Creates and returns a print geometry for this Silverlight geometry.
//      It accounts for Geometry.Transform but not for stretch.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CGeometry::GetPrintGeometry(
    _In_ const D2DPrecomputeParams& cp,
    _In_ const D2DRenderParams &printParams,
    _Outptr_ IPALAcceleratedGeometry** ppGeometry
    )
{
    HRESULT hr = S_OK;
    IPALAcceleratedGeometry *pGeometry = NULL;
    IPALAcceleratedGeometry *pTransformedGeometry = NULL;

    // Call the virtual to get the IPALAcceleratedGeometry for this geometry.
    IFC(GetPrintGeometryVirtual(cp, printParams, &pGeometry));

    // If Geometry.Transform is present, create a transformed PAL geometry.
    if (m_pTransform)
    {
        CMILMatrix transform;
        m_pTransform->GetTransform(&transform);
        if(transform.IsIdentity() == false)
        {
            IFC(cp.GetFactory()->CreateTransformedGeometry(
                pGeometry,
                &transform,
                &pTransformedGeometry
                ));
        }
    }
    else
    {
        SetInterface(pTransformedGeometry, pGeometry);
    }

    SetInterface(*ppGeometry, pTransformedGeometry);

Cleanup:
    ReleaseInterface(pGeometry);
    ReleaseInterface(pTransformedGeometry);
    RRETURN(hr);
}


//------------------------------------------------------------------------
//
//  CGeometry::GetPrintGeometry
//
//  Synopsis:
//      Creates and returns a print geometry for this Silverlight geometry.
//      Accounts for any transforms or stretch.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CGeometry::GetPrintGeometry(
    _In_ const D2DPrecomputeParams& cp,
    _In_ const D2DRenderParams &printParams,
    _In_opt_ XRECTF *pElementBounds,
    DirectUI::Stretch stretch,
    XFLOAT rPenThickness,
    _Outptr_ IPALAcceleratedGeometry** ppGeometry
    )
{
    HRESULT hr = S_OK;
    IPALAcceleratedGeometry *pTransformedGeometry = NULL;
    IPALAcceleratedGeometry *pStretchedTransformedGeometry = NULL;
    CMILMatrix stretchMatrix(true);

    IFC(GetPrintGeometry(cp, printParams, &pTransformedGeometry));

    // Compute the stretch matrix and transform the PAL geometry
    if (pTransformedGeometry && stretch != DirectUI::Stretch::None)
    {
        XRECTF_RB naturalBoundsRB;
        XRECTF naturalBounds;
        XRECTF elementBounds = {};

        IFC(pTransformedGeometry->GetBounds(&naturalBoundsRB));
        naturalBounds = ToXRectF(naturalBoundsRB);

        GetStrokeAdjustedElementBounds(
            pElementBounds,
            naturalBounds,
            rPenThickness,
            &elementBounds
            );

        IFC(ComputeStretchMatrix(
            stretch,
            &elementBounds,
            &naturalBounds,
            &stretchMatrix
            ));

        if (!stretchMatrix.IsIdentity())
        {
            IFC(cp.GetFactory()->CreateTransformedGeometry(
                pTransformedGeometry,
                &stretchMatrix,
                &pStretchedTransformedGeometry
                ));
        }
        else
        {
            SetInterface(pStretchedTransformedGeometry, pTransformedGeometry);
        }
    }
    else
    {
        SetInterface(pStretchedTransformedGeometry, pTransformedGeometry);
    }

    SetInterface(*ppGeometry, pStretchedTransformedGeometry);

Cleanup:
    ReleaseInterface(pStretchedTransformedGeometry);
    ReleaseInterface(pTransformedGeometry);
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  CGeometry::GetPrintGeometryVirtual
//
//  Synopsis:
//      Overridden by subclasses to ensures a print geometry exists for this
//      Silverlight geometry.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CGeometry::GetPrintGeometryVirtual(
    _In_ const D2DPrecomputeParams& cp,
    _In_ const D2DRenderParams &printParams,
    IPALAcceleratedGeometry** ppGeometry
    )
{
    RRETURN(S_OK);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Generate bounds for the geometry.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CGeometry::GetBounds(
    _Out_ XRECTF_RB* pBounds
    )
{
    HRESULT hr = S_OK;
    BoundsGeometrySink* pSink = NULL;

    pSink = new BoundsGeometrySink();

    IFC(VisitSink(pSink));

    IFC(pSink->Close());

    IFC(pSink->GetBounds(pBounds));

Cleanup:
    ReleaseInterface(pSink);

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Generate widened bounds for the geometry.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CGeometry::GetWidenedBounds(
    _In_ const CPlainPen& pen,
    _Out_ XRECTF_RB* pBounds
    )
{
    if (!pen.IsEmpty())
    {
        CStrokeBoundsSink boundsSink;

        IFC_RETURN(WidenToSink(pen, NULL, &boundsSink));

        boundsSink.GetBounds(pBounds);
    }
    else
    {
        EmptyRectF(pBounds);
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Widen the geometry to a widening sink.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CGeometry::WidenToSink(
    const CPlainPen& pen,
    _In_opt_ const CMILMatrix* pTransform,
    _In_ CWideningSink* pSink
    )
{
    HRESULT hr = S_OK;
    bool emptyPen = false;
    CWidenerSink* pWidener = NULL;
    TransformGeometrySink* pTransformSink = NULL;
    IPALGeometrySink* pTargetSink = NULL;

    pWidener = new CWidenerSink(NULL, 0.25f);

    //
    // NOTE: Transform of geometry happens prior to being sent to the widener.
    //       This means that the stroke width will remain constant and the
    //       size of the pen.
    //

    if (pTransform && !pTransform->IsIdentity())
    {
        pTransformSink = new TransformGeometrySink(pWidener, *pTransform);

        pTargetSink = pTransformSink;
    }
    else
    {
        pTargetSink = pWidener;
    }

    IFC(pWidener->Set(pen, pSink, NULL, emptyPen));

    IFC(VisitSink(pTargetSink));

    IFC(pWidener->Close());

Cleanup:
    ReleaseInterface(pTransformSink);
    ReleaseInterface(pWidener);

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Test if the fill of this geometry contains the specified point.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CGeometry::FillContainsPoint(
    _In_ const XPOINTF& target,
    _In_opt_ const CMILMatrix* pTransform,
    _Out_ bool* pContainsPoint
    )
{
    RRETURN(HitTestFill(target, pTransform, pContainsPoint));
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Test if the fill of this geometry contains the specified point.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CGeometry::HitTestFill(
    _In_ const XPOINTF& target,
    _In_opt_ const CMILMatrix* pTransform,
    _Out_ bool* pHit
    )
{
    HRESULT hr = S_OK;
    bool hit = true;
    PointHitTestGeometrySink* pSink = NULL;

    pSink = new PointHitTestGeometrySink(target, 0.25f, pTransform);

    IFC(VisitSink(pSink));

    IFC(pSink->Close());

    if (FAILED(pSink->GetResult(&hit)))
    {
        //
        // Some geometry may not be valid such as have degenerate transforms or infinite
        // points. In the case they are encountered the geometry is considered not hit.
        //

        hit = FALSE;
    }

    *pHit = hit;

Cleanup:
    ReleaseInterface(pSink);

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Test if the fill of this geometry intersects the specified polygon.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CGeometry::HitTestFill(
    _In_ const HitTestPolygon& target,
    _In_opt_ const CMILMatrix* pTransform,
    _Out_ bool* pHit
    )
{
    HRESULT hr = S_OK;
    bool hit = true;
    PolygonHitTestGeometrySink* pSink = NULL;

    pSink = new PolygonHitTestGeometrySink(target, 0.25f, pTransform);

    IFC(VisitSink(pSink));

    IFC(pSink->Close());

    if (FAILED(pSink->GetResult(&hit)))
    {
        //
        // Some geometry may not be valid such as have degenerate transforms or infinite
        // points. In the case they are encountered the geometry is considered not hit.
        //

        hit = FALSE;
    }

    *pHit = hit;

Cleanup:
    ReleaseInterface(pSink);

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Clips the specified point to this geometry.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CGeometry::ClipToFill(
    _Inout_ XPOINTF& target,
    _In_opt_ const CMILMatrix* pTransform,
    _Out_ bool* pHit
    )
{
    // No additional work for points.
    RRETURN(HitTestFill(target, pTransform, pHit));
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Clips the specified polygon to this geometry.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CGeometry::ClipToFill(
    _Inout_ HitTestPolygon& target,
    _In_opt_ const CMILMatrix* pTransform,
    _Out_ bool* pHit
    )
{
    HRESULT hr = S_OK;
    bool hit = true;
    PolygonHitTestGeometrySink* pSink = NULL;

    pSink = new PolygonHitTestGeometrySink(target, 0.25f, pTransform);

    IFC(VisitSink(pSink));

    IFC(pSink->Close());

    if (FAILED(pSink->GetResult(&hit)))
    {
        //
        // Some geometry may not be valid such as have degenerate transforms or infinite
        // points. In the case they are encountered the geometry is considered not hit.
        //

        hit = FALSE;
    }

    if (hit)
    {
        IFC(pSink->GetIntersection(target));
    }

    *pHit = hit;

Cleanup:
    ReleaseInterface(pSink);

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Test if the stroke of this geometry contains the specified point.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CGeometry::StrokeContainsPoint(
    _In_ const XPOINTF& target,
    _In_ const CPlainPen& pen,
    _In_opt_ const CMILMatrix* pTransform,
    _Out_ bool* pContainsPoint
    )
{
    RRETURN(HitTestStroke(target, pen, pTransform, pContainsPoint));
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Test if the stroke of this geometry contains the specified point.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CGeometry::HitTestStroke(
    _In_ const XPOINTF& target,
    _In_ const CPlainPen& pen,
    _In_opt_ const CMILMatrix* pTransform,
    _Out_ bool* pHit
    )
{
    if (!pen.IsEmpty())
    {
        PointHitTestHelper hitTestHelper(target, 0.25f, NULL);
        CStrokeHitTestSink sink(hitTestHelper);

        IFC_RETURN(WidenToSink(pen, pTransform, &sink));
        *pHit = sink.WasHit();
    }
    else
    {
        *pHit = FALSE;
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Test if the stroke of this geometry intersects the specified polygon.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CGeometry::HitTestStroke(
    _In_ const HitTestPolygon& target,
    _In_ const CPlainPen& pen,
    _In_opt_ const CMILMatrix* pTransform,
    _Out_ bool* pHit
    )
{
    if (!pen.IsEmpty())
    {
        PolygonHitTestHelper hitTestHelper(target, 0.25f, NULL);
        CStrokeHitTestSink sink(hitTestHelper);

        IFC_RETURN(WidenToSink(pen, pTransform, &sink));
        *pHit = sink.WasHit();
    }
    else
    {
        *pHit = FALSE;
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Add geometry to the specified sink.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CGeometry::VisitSink(
    _In_ IPALGeometrySink* pSink
    )
{
    HRESULT hr = S_OK;
    TransformGeometrySink* pTransformSink = NULL;

    //
    // If a geometry transform is set then all subsequent geometry
    // needs to be transformed.
    //
    if (m_pTransform != NULL)
    {
        CMILMatrix geometryTransform;

        m_pTransform->GetTransform(&geometryTransform);

        pTransformSink = new TransformGeometrySink(pSink, geometryTransform);

        IFC(VisitSinkInternal(pTransformSink));
    }
    else
    {
        IFC(VisitSinkInternal(pSink));
    }

Cleanup:
    ReleaseInterface(pTransformSink);

    RRETURN(hr);
}
_Check_return_ HRESULT CGeometry::LeaveImpl(
    _In_ CDependencyObject *pNamescopeOwner,
    LeaveParams params)
{
    if (params.fIsLive)
    {
        m_wucGeometry.Reset();
        m_isWUCGeometryDirty = true;
    }

    IFC_RETURN(__super::LeaveImpl(pNamescopeOwner, params));

    return S_OK;
}

void CGeometry::ReleaseDCompResources()
{
    __super::ReleaseDCompResources();

    m_wucGeometry.Reset();
    m_isWUCGeometryDirty = true;
}

