// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class PolygonHitTestGeometrySink final : public HitTestGeometrySink
{
    public:
        PolygonHitTestGeometrySink(
            const HitTestPolygon& hitTestPolygon,
            XFLOAT tolerance,
            _In_opt_ const CMILMatrix* pTransform
            );

        ~PolygonHitTestGeometrySink(
            ) override;

        _Check_return_ HRESULT GetResult(
            _Out_ bool* pHit
            ) override;

        _Check_return_ HRESULT GetIntersection(
            _Out_ HitTestPolygon& resultPolygon
            );

    private:
        PolygonHitTestHelper m_hitTestHelper;
};
