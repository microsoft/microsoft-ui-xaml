// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class HitTestGeometrySink : public CXcpObjectBase< IPALGeometrySink >
{
    public:
        HitTestGeometrySink(
            );

        ~HitTestGeometrySink(
            ) override;

        void BeginFigure(
            const XPOINTF& startPoint,
            bool fIsHollow
            ) final;

        void EndFigure(
            bool fIsClosed
            ) final;

        void AddArc(
            const XPOINTF& point,
            const XSIZEF& size,
            XFLOAT rotationAngle,
            bool fIsClockwise,
            bool fIsLargeArc
            ) final;

        void AddBezier(
            const XPOINTF& controlPoint1,
            const XPOINTF& controlPoint2,
            const XPOINTF& endPoint
            ) final;

        void AddLine(
            const XPOINTF& point
            ) final;

        void AddQuadraticBezier(
            const XPOINTF& controlPoint,
            const XPOINTF& endPoint
            ) final;

        void AddLines(
            _In_reads_(uiCount) const XPOINTF *pPoints,
            XUINT32 uiCount
            ) final;

        void AddBeziers(
            _In_reads_(uiCount) const XPOINTF *pPoints,
            XUINT32 uiCount
            ) final;

        void AddQuadraticBeziers(
            _In_reads_(uiCount) const XPOINTF *pPoints,
            XUINT32 uiCount
            ) final;

        void SetFillMode(
            GeometryFillMode fillMode
            ) final;

        _Check_return_ HRESULT Close(
            ) final;

        virtual _Check_return_ HRESULT GetResult(
            _Out_ bool* pHit
            ) = 0;

    protected:
        GeometryFillMode m_fillMode;
        XPOINTF m_startPoint;
        bool m_figureActive;

        HitTestHelper* m_pBaseHitTestHelper{};
};
