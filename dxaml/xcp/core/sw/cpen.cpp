// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"


//+-------------------------------------------------------------------------------------------------
//
//  Member:     CPPenGeometry::GetInflateFactor
//
//  Synopsis:   Get the factor by which the pen width may be inflated due to corners & caps
//
//--------------------------------------------------------------------------------------------------
XFLOAT
CPenGeometry::GetInflateFactor() const
{
    // The pen may inflates the stroked shape by its width/height
    XFLOAT rExtents = 1;

    // Include the potential expansion caused by mitered corners
    if ((XcpLineJoinMiter == m_eJoin) || (XcpLineJoinMiterClipped == m_eJoin))
    {
        ASSERT(m_rMiterLimit >= 1);
        rExtents = static_cast<XFLOAT>(m_rMiterLimit * SQRT_2);
    }
    else if(XcpPenCapSquare == m_eStartCap || XcpPenCapSquare == m_eEndCap
            || XcpPenCapSquare == m_eDashCap)
    {
        // Include the potential diagonal of a cap.
        rExtents = static_cast<XFLOAT>(SQRT_2);
    }

    return rExtents;
}
//-----------------------------------------------------------------------------
//
// Function Description:
//
// Default constructor
//
//-----------------------------------------------------------------------------
CPlainPen::CPlainPen()
{
    m_eDashStyle = XcpDashStyleSolid;
    m_rDashOffset = 0;
    m_rgDashes = NULL;
    m_cDashCount = 0;
}

//-----------------------------------------------------------------------------
//
// Function Description:
//
// Copy constructor (private)
//
//-----------------------------------------------------------------------------
CPlainPen::CPlainPen(
    _In_ const CPlainPen &other)  // In: The other pen to copy
   :m_oGeom(other.m_oGeom),
    m_eDashStyle(XcpDashStyleSolid),
    m_rDashOffset(other.m_rDashOffset)
{
    /* This constructor does NOT copy the members whose allocation on the
    heap may fail - dashes, custom shapes and compound array.  Until the
    dashes are successfuly copied the dash style is set to solid. */
}
//+-------------------------------------------------------------------------------------------------
//
//  Member:     CPlainPen::~CPlainPen
//
//  Synopsis:   Destructor
//
//--------------------------------------------------------------------------------------------------
CPlainPen::~CPlainPen()
{
    delete [] m_rgDashes;
}

//-----------------------------------------------------------------------------
//
// Function Description:
//
// Set dash style and array
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT
CPlainPen::SetDashStyle(
    _In_ XcpDashStyle style) // Dash style, cannot be custom
{
    XFLOAT    dashes[6];
    XINT32     count=0;

    switch(style)
    {
    case XcpDashStyleSolid:
        break;

    case XcpDashStyleDash:
        count = 2;
        dashes[0] = 2;   // a dash
        dashes[1] = 2;   // a space
        break;

    case XcpDashStyleDot:
        count = 2;
        dashes[0] = 0;   // a dot
        dashes[1] = 2;   // a space
        break;

    case XcpDashStyleDashDot:
        count = 4;
        dashes[0] = 2;   // a dash
        dashes[1] = 2;   // a space
        dashes[2] = 0;   // a dot
        dashes[3] = 2;   // a space
        break;

    case XcpDashStyleDashDotDot:
        count = 6;
        dashes[0] = 2;   // a dash
        dashes[1] = 2;   // a space
        dashes[2] = 0;   // a dot
        dashes[3] = 2;   // a space
        dashes[4] = 0;   // a dot
        dashes[5] = 2;   // a space
        break;

    default:
        // The dash style must be one of the predefined ones.  Custom dash
        // style can only be set internally when the user sets the dash
        // array
        return E_INVALIDARG;
    }

    if (count > 0)
    {
        // Set Dash Offset to be at half the first dash, so that there will
        // always be a solid dash at the path's start, end and corners.
        m_rDashOffset = dashes[0] / 2;

        delete [] m_rgDashes;
        m_rgDashes = NULL;
        m_cDashCount = 0;

        m_rgDashes = new XFLOAT[count];
        m_cDashCount  = count;

        memcpy(m_rgDashes, dashes, count*sizeof(XFLOAT));
    }

    // Set the style
    m_eDashStyle = style;

    return S_OK;
}
//+-------------------------------------------------------------------------------------------------
//
//  Member:     CPlainPen::SetDashArray
//
//  Synopsis:   Set the dash array to the input array if valid
//
//--------------------------------------------------------------------------------------------------
_Check_return_ HRESULT
CPlainPen::SetDashArray(_In_ const std::vector<float>& dashes)
{
    HRESULT hr = S_OK;
    unsigned int nTargetCount;
    unsigned int dashesCount = static_cast<unsigned int>(dashes.size());

    // If we have an odd number of elements, the behavior is to dupliate the
    // values.  For example, dash array 1,2,3 needs to become 1,2,3,1,2,3

    if ((dashesCount % 2) != 0)
    {
        nTargetCount = dashesCount * 2;
    }
    else
    {
        nTargetCount = dashesCount;
    }

    delete [] m_rgDashes;
    m_rgDashes = NULL;

    m_rgDashes = new XFLOAT[nTargetCount];
    m_cDashCount  = nTargetCount;

    // Add the dashes and gaps, making sure they are nonnegative
    for (XUINT32 i = 0;  i < nTargetCount;  i++)
    {
        m_rgDashes[i] = XcpAbsF(dashes[i % dashesCount]);
    }

    m_eDashStyle = XcpDashStyleCustom;

    RRETURN(hr);//RRETURN_REMOVAL
}
//-----------------------------------------------------------------------------
//
// Function Description:
//
// Get the dash array
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT
CPlainPen::GetDashArray(
    _In_ XUINT32 count,
        // Output buffer size
    _Out_writes_(count) /* _part(count,m_rgDashes.Count) */ XFLOAT *dashes)
        // The arrray of dash starts/ends
{
    XUINT32 dashCount = m_cDashCount;

    if (!dashes  ||  dashCount > count)
    {
        IFC_RETURN(E_INVALIDARG);
    }

    for (XUINT32 i = 0;  i < dashCount;  i++)
    {
        dashes[i] = m_rgDashes[i];
    }
    return S_OK;
}

//+-------------------------------------------------------------------------------------------------
//
//  Member:     CPlainPen::GetExtents
//
//  Synopsis:   Get the extents of the stroke
//
//--------------------------------------------------------------------------------------------------
_Check_return_ HRESULT
CPlainPen::GetExtents(_Out_ XFLOAT &rExtents) const
{
    // The pen may inflates the stroked shape by its width/height, but that may be extended further
    // by mitered corners, square caps and line shapes.  These multiply the pen's width or height,
    // so let us first compute the maximal factor

    XFLOAT rThickness = MAX(m_oGeom.GetWidth(), m_oGeom.GetHeight()) / 2;
    rExtents = rThickness * m_oGeom.GetInflateFactor();

    if (_isnan(rExtents))
    {
        IFC_RETURN(E_FAIL);
    }

#ifdef LINE_SHAPES_ENABLED
    XFLOAT r;

     // Include the potential expansion caused by line shapes
    if (m_pStartShape)
    {
        IFC_RETURN(m_pStartShape->GetExtents(rThickness, rExtents, _Out_ r));
        if (r > rExtents)
        {
            rExtents = r;
        }
    }
    if (m_pEndShape)
    {
        IFC_RETURN(m_pEndShape->GetExtents(rThickness, rExtents, _Out_ r));
        if (r > rExtents)
        {
            rExtents = r;
        }
    }
#endif // LINE_SHAPES_ENABLED

    return S_OK;
}
////////////////////////////////////////////////////////////////////////////
//      Implementation of CPenGeometry
//+-------------------------------------------------------------------------------------------------
//
//  Member:     CPenGeometry::Set
//
//  Synopsis:   Set the width, height and angle of the pen's ellipse
//
//--------------------------------------------------------------------------------------------------
void
CPenGeometry::Set(
    XFLOAT width,   // In: Pen ellipse width
    XFLOAT height,  // In: Pen ellipse height
    XFLOAT angle)   // In: Angle in radians the ellipse is rotated
{
    m_rWidth = XcpAbsF(width);
    m_rHeight = XcpAbsF(height);
    m_rAngle = angle;
}
