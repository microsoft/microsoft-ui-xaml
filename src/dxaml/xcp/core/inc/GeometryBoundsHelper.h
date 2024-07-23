// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class GeometryBoundsHelper
{
    public:
        GeometryBoundsHelper(
            );

        virtual ~GeometryBoundsHelper(
            );
        
        void StartAt(
            _In_ const XPOINTF& firstPoint
            );

        void DoLine(
            _In_ const XPOINTF& endPoint
            );

        void DoBezier(
            _In_ const XPOINTF& controlPoint1,
            _In_ const XPOINTF& controlPoint2,
            _In_ const XPOINTF& endPoint
            );

        void DoQuadraticBezier(
            _In_ const XPOINTF& controlPoint1,
            _In_ const XPOINTF& endPoint
            );

        void DoArc(
            const XPOINTF& point,
            const XSIZEF& size,
            XFLOAT rotationAngle,
            bool fIsClockwise,
            bool fIsLargeArc
            );

        _Check_return_ HRESULT GetBounds(
            _Out_ XRECTF_RB* pBounds
            );

    private:
        void AcceptPoint(
            _In_ const XPOINTF& endPoint
            );

        XRECTF_RB m_bounds{};
        XPOINTF m_currentPoint{};
        bool m_hasFirstPoint{};
};
