// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CMILMatrix;

// Encapsulates a convex polygon used for hit testing.
class HitTestPolygon
{
public:
    void SetRect(_In_ const XRECTF& rect);
    void SetPoints(
        XUINT32 numPoints,
        _In_reads_(numPoints) const XPOINTF *pPoints);
    void SetEmpty();
    void CopyTo(_In_ HitTestPolygon *pDest) const;

    bool UpdateStateFromPoints();
    void UpdateStateFromRect();

    void Transform(const CMILMatrix& transform);
    void Transform(const CMILMatrix4x4& transform);
    bool TransformWorldToLocalWithInverse(const CMILMatrix4x4& worldTransform);
    bool TransformWorldToLocalWithInterpolation(const CMILMatrix4x4& worldTransform);

    _Check_return_ HRESULT IntersectsPolygon(
        XUINT32 numPoints,
        _In_reads_(numPoints) const XPOINTF *pPoints,
        _Out_ bool *pIntersects) const;
    bool IntersectsRect(_In_ const XRECTF& rect) const;
    bool IntersectsRect(_In_ const XRECTF_RB& rectRB) const;

    _Check_return_ HRESULT ClipToPolygon(
        XUINT32 numPoints,
        _In_reads_(numPoints) const XPOINTF *pPoints);
    _Check_return_ HRESULT ClipToRect(const XRECTF& rect);

    XPOINTF *GetPoints() { return &m_points[0]; }
    size_t GetSize() const { return m_points.size(); }
    bool IsEmpty() const { return m_points.size() == 0; }

    XRECTF_RB GetPolygonBounds() const;

    bool IsRect() const { return m_isRect; }
    XRECTF GetRect() const { return m_rect; }

private:
    void Ensure3DMode();

private:
    // Always wound counterclockwise.
    std::vector<XPOINTF> m_points;
    std::vector<XPOINTF4> m_3DPoints;

    // Optimization for when we're in 2D mode and the points represent an axis aligned rect
    XRECTF m_rect;

    bool m_isRect {false};

    // The HitTestPolygon starts in 2D mode, with m_points filled out. Once a 3D matrix is involved, it changes to
    // 3D mode, with m_3DPoints filled out.
    bool m_is3D {false};
};
