// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "Transform3D.h"
#include "Matrix.h"

class CPerspectiveTransform3D final : public CTransform3D
{
public:
    CPerspectiveTransform3D()
        : CPerspectiveTransform3D(nullptr)
    {
    }

    ~CPerspectiveTransform3D() override = default;

    DECLARE_CREATE(CPerspectiveTransform3D);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CPerspectiveTransform3D>::Index;
    }

    void UpdateTransformMatrix(
        float elementWidth,
        float elementHeight
        ) override;

    void ReleaseDCompResources() final;

    void MakeWinRTExpression(
        _Inout_ WinRTExpressionConversionContext* pWinRTContext,
        float elementWidth,
        float elementHeight
        ) override;

    bool IsRenderedTransform2D() const override;

    bool HasDepth() override;

private:
    CPerspectiveTransform3D(_In_opt_ CCoreServices *pCore)
        : CTransform3D(pCore)
        , m_rDepth(1000.0f)
        , m_rOffsetX(0.0f)
        , m_rOffsetY(0.0f)
    {
        m_matTransform3D.SetToIdentity();
        m_matTransform3D._33 = 0.0f;
    }

public:
    // Public API
    float m_rDepth;
    float m_rOffsetX;
    float m_rOffsetY;
};
