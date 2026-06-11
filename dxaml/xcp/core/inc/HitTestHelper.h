// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class HitTestHelper : private CFlatteningSink
{
    public:
        HitTestHelper(
            XFLOAT tolerance,
            _In_opt_ const CMILMatrix* pTransform
            );

        ~HitTestHelper(
            ) override;

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

        virtual _Check_return_ HRESULT GetResult(
            bool *pWasHit
            ) = 0;

        virtual void Reset(
            ) = 0;

    protected:
        virtual void AcceptPoint(
            _In_ const XPOINTF& endPoint
            ) = 0;

        void CheckForNaN(
            _In_ const XPOINTF& endPoint
            );

        CMILMatrix m_transform;
        XFLOAT m_tolerance{};
        XPOINTF m_currentPoint{};
        XPOINTF m_currentPointUntransformed{};
        bool m_encounteredNaN;

    private:
        _Check_return_ HRESULT AcceptPoint(
            _In_ const XPOINTF &pt,
            _In_ XFLOAT t,
            _Out_ XINT32 &fAborted
            ) override;
};
