// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class PointHitTestHelper : public HitTestHelper
{
    public:
        PointHitTestHelper(
            const XPOINTF& hitTestPoint,
            XFLOAT tolerance,
            _In_opt_ const CMILMatrix* pTransform
            );

        ~PointHitTestHelper(
            ) override;


        HRESULT GetWindingNumber(
            _Out_ XINT32* pWindingNumber
            );

        bool EncounteredEdgeNear(
            );

        _Check_return_ HRESULT GetResult(
            bool *pWasHit
            ) override;

        void Reset(
            ) override;

    protected:
        void AcceptPoint(
            _In_ const XPOINTF& endPoint
            ) override;

    private:
        void CheckIfSegmentNearTheOrigin(
            _In_ const XPOINTF& endPoint
            );

        XPOINTF m_hitTestPoint;
        XINT32 m_windingNumber;
        bool m_encounteredEdgeNear;
};
