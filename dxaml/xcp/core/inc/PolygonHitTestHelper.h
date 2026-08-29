// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class PolygonHitTestHelper final : public HitTestHelper
{
    public:
        PolygonHitTestHelper(
            const HitTestPolygon& hitTestPolygon,
            XFLOAT tolerance,
            _In_opt_ const CMILMatrix* pTransform
            );

        ~PolygonHitTestHelper(
            ) override;

        _Check_return_ HRESULT GetResult(
            bool *pWasHit
            ) override;

        void Reset(
            ) override;

        _Check_return_ HRESULT GetIntersection(
            _Out_ HitTestPolygon& resultPolygon
            );

    protected:
        void AcceptPoint(
            _In_ const XPOINTF& endPoint
            ) override;

    private:
        const HitTestPolygon& m_hitTestPolygon;
        xvector<XPOINTF> m_points;
};
