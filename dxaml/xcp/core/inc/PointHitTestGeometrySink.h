// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class PointHitTestGeometrySink final : public HitTestGeometrySink
{
    public:
        PointHitTestGeometrySink(
            const XPOINTF& hitTestPoint,
            XFLOAT tolerance,
            _In_opt_ const CMILMatrix* pTransform
            );

        ~PointHitTestGeometrySink() override;

        _Check_return_ HRESULT GetResult(
            _Out_ bool* pPointInside
            ) override;

    private:
        PointHitTestHelper m_hitTestHelper;
};
