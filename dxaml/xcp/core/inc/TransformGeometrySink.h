// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class TransformGeometrySink final : public CXcpObjectBase< IPALGeometrySink >
{
    public:
        TransformGeometrySink(
            _In_ IPALGeometrySink* pSink,
            _In_ const CMILMatrix& transform
            );

        ~TransformGeometrySink(
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
        IPALGeometrySink* m_pSink;
        CMILMatrix m_transform;
        XPOINTF m_lastUntransformedPoint{};
};
