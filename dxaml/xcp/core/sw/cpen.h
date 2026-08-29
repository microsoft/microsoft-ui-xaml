// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CPlainPen;

////////////////////////////////////////////////////////////////////////////
// Definition of CPenGeometry
// This class captures geometry the stroke emitted by a simple pen:
// No dashes, compound or line shapes

class CPenGeometry
{
public:
    CPenGeometry()
    :m_rWidth(1),
    m_rHeight(1),
    m_rAngle(0),
    m_eStartCap(XcpPenCapFlat),
    m_eEndCap(XcpPenCapFlat),
    m_eDashCap(XcpPenCapFlat),
    m_eJoin(XcpLineJoinMiter),
    m_rMiterLimit(10)
    {
    }

    CPenGeometry(
        _In_ const CPenGeometry &other)
    :m_rWidth(other.m_rWidth),
    m_rHeight(other.m_rHeight),
    m_rAngle(other.m_rAngle),
    m_eStartCap(other.m_eStartCap),
    m_eEndCap(other.m_eEndCap),
    m_eDashCap(XcpPenCapFlat),
    m_eJoin(other.m_eJoin),
    m_rMiterLimit(other.m_rMiterLimit)
    {
    }

    bool IsCircular() const
    {
        return m_rWidth == m_rHeight;
    }

    void Set(
        XFLOAT width,   // In: Pen ellipse width
        XFLOAT height,  // In: Pen ellipse height
        XFLOAT angle);  // In: Angle in radians the ellipse is rotated

    XFLOAT GetWidth() const
    {
        return m_rWidth;
    }

    void SetWidth(_In_ XFLOAT rWidth)
    {
        m_rWidth = XcpAbsF(rWidth);
    }

    XFLOAT GetHeight() const
    {
        return m_rHeight;
    }

    void SetHeight(_In_ XFLOAT rHeight)
    {
        m_rHeight = XcpAbsF(rHeight);
    }

    XFLOAT GetAngle() const
    {
        return m_rAngle;
    }

    void SetAngle(
        XFLOAT val)
    {
        m_rAngle = val;
    }

    bool IsEmpty() const
    {
        return m_rWidth == 0  ||  m_rHeight == 0;
    }

    XcpPenCap GetStartCap() const
    {
        return m_eStartCap;
    }

    void SetStartCap(XcpPenCap eCap)
    {
        m_eStartCap = eCap;
    }

    XcpPenCap GetEndCap() const
    {
        return m_eEndCap;
    }

    void SetEndCap(XcpPenCap eCap)
    {
        m_eEndCap = eCap;
    }

    XcpPenCap GetDashCap() const
    {
        return m_eDashCap;
    }

    void SetDashCap(XcpPenCap eCap)
    {
        m_eDashCap = eCap;
    }

    XcpLineJoin GetJoin() const
    {
        return m_eJoin;
    }

    void SetJoin(XcpLineJoin eJoin)
    {
        m_eJoin = eJoin;
    }

    XFLOAT GetMiterLimit() const
    {
        return m_rMiterLimit;
    }

    void SetMiterLimit(XFLOAT val)
    {
        m_rMiterLimit = MAX(val, 1.0f);
    }

    XFLOAT GetInflateFactor() const;

    XFLOAT GetExtents() const
    {
        return GetInflateFactor() * MAX(m_rWidth, m_rHeight);
    }

protected:
    // Allow A line shape to override the corresponding line cap
    friend class CPlainPen;

    // Data
    XFLOAT            m_rWidth;
    XFLOAT            m_rHeight;
    XFLOAT            m_rAngle;

    XcpPenCap    m_eStartCap;
    XcpPenCap    m_eEndCap;
    XcpPenCap    m_eDashCap;
    XcpLineJoin    m_eJoin;
    XFLOAT           m_rMiterLimit;
};



////////////////////////////////////////////////////////////////////////////
// Definition of CPlainPen
// This class captures geometric properties of the stroke emitted by a pen:
// No concept of color or brush.
// The class design has hooks for compound lines, but the feature is not yet
// implemented in our widening code.  Until it is implemented, these hooks are
// hidden inside #ifdef COMPOUND_PEN_IMPLEMENTED.

class CPlainPen
{
public:
    CPlainPen();

    ~CPlainPen();



    CPlainPen(_In_ const CPlainPen &other);

    void Set(
        XFLOAT width,   // In: Pen ellipse width
        XFLOAT height,  // In: Pen ellipse height
        XFLOAT angle)   // In: Angle in radians the ellipse is rotated
    {
        m_oGeom.Set(width, height, angle);
    }

    bool IsEmpty() const
    {
        return m_oGeom.IsEmpty();
    }

    bool IsCircular() const
    {
        return m_oGeom.IsCircular();
    }

    bool IsSimple() const
    {
        return (m_eDashStyle == XcpDashStyleSolid);
    }

    _Check_return_ HRESULT SetDashStyle(
        _In_ XcpDashStyle style);     // Dash style, cannot be custom

    _Check_return_ HRESULT GetDashArray(
        _In_ XUINT32 count,
            // Output buffer size
        _Out_writes_(count) /* _part(count, m_rgDashes.Count) */ XFLOAT *dashes);
            // The arrray of dash starts/ends

    _Check_return_ HRESULT SetDashArray(_In_ const std::vector<float>& dashes);

    XFLOAT GetWidth() const
    {
        return m_oGeom.GetWidth();
    }

    void SetWidth(
        XFLOAT val)
    {
        m_oGeom.SetWidth(val);
    }

    XFLOAT GetHeight() const
    {
        return m_oGeom.GetHeight();
    }

    void SetHeight(
        XFLOAT val)
    {
        m_oGeom.SetHeight(val);
    }

    XFLOAT GetAngle() const
    {
        return m_oGeom.GetAngle();
    }

    void SetAngle(
        XFLOAT val)
    {
        m_oGeom.SetAngle(val);
    }

    XcpPenCap GetStartCap() const
    {
        return m_oGeom.GetStartCap();
    }

    void SetStartCap(
        XcpPenCap val)
    {
        m_oGeom.SetStartCap(val);
    }

    XcpPenCap GetEndCap() const
    {
        return m_oGeom.GetEndCap();
    }

    void SetEndCap(
        XcpPenCap val)
    {
        m_oGeom.SetEndCap(val);
    }

    XcpPenCap GetDashCap() const
    {
        return m_oGeom.GetDashCap();
    }

    void SetDashCap(
        XcpPenCap val)
    {
        m_oGeom.SetDashCap(val);
    }

    XcpLineJoin GetJoin() const
    {
        return m_oGeom.GetJoin();
    }

    void SetJoin(
        XcpLineJoin val)
    {
        m_oGeom.SetJoin(val);
    }

    XFLOAT GetMiterLimit() const
    {
        return m_oGeom.GetMiterLimit();
    }

    void SetMiterLimit(
        XFLOAT val)
    {
        m_oGeom.SetMiterLimit(val);
    }

    XcpDashStyle GetDashStyle() const
    {
        return m_eDashStyle;
    }

    XFLOAT GetDashOffset() const
    {
        return m_rDashOffset;
    }

    void SetDashOffset(
        XFLOAT val)
    {
        m_rDashOffset = val;
    }

    XUINT32 GetDashCount() const
    {
        return m_cDashCount;
    }

    XFLOAT GetDash(XINT32 i) const
    {
        return m_rgDashes[i];
    }

    const CPenGeometry &GetGeometry() const
    {
        return m_oGeom;
    }

    _Check_return_ HRESULT GetExtents(_Out_ XFLOAT &rExtents) const;

    // Private methods
protected:

    // Data
protected:
    CPenGeometry   m_oGeom;

    XcpDashStyle   m_eDashStyle;
    XFLOAT         m_rDashOffset{};

    _Field_size_(m_cDashCount) XFLOAT         *m_rgDashes;
    XUINT32        m_cDashCount{};
};

