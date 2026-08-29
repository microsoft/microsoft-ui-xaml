// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "Transform3D.h"

class CCompositeTransform3D final : public CTransform3D
{
public:
    CCompositeTransform3D()
        : CCompositeTransform3D(nullptr)
    {
    }

    ~CCompositeTransform3D() override = default;

    DECLARE_CREATE(CCompositeTransform3D);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CCompositeTransform3D>::Index;
    }

    void ReleaseDCompResources() final;

    void MakeWinRTExpression(
        _Inout_ WinRTExpressionConversionContext* pWinRTContext,
        float elementWidth,
        float elementHeight
        ) override;

    void ClearWUCExpression() override;

    void UpdateTransformMatrix(
        float elementWidth /*ignored*/,
        float elementHeight /*ignored*/
        ) override;

    bool IsRenderedTransform2D() const override;

    bool HasDepth() override;

private:
    CCompositeTransform3D(_In_opt_ CCoreServices* pCore)
        : CTransform3D(pCore)
        , m_rCenterX(0.0f)
        , m_rCenterY(0.0f)
        , m_rCenterZ(0.0f)
        , m_rRotationX(0.0f)
        , m_rRotationY(0.0f)
        , m_rRotationZ(0.0f)
        , m_rScaleX(1.0f)
        , m_rScaleY(1.0f)
        , m_rScaleZ(1.0f)
        , m_rTranslateX(0.0f)
        , m_rTranslateY(0.0f)
        , m_rTranslateZ(0.0f)
        , m_isCenterXAnimationDirty(false)
        , m_isCenterYAnimationDirty(false)
        , m_isCenterZAnimationDirty(false)
        , m_isScaleXAnimationDirty(false)
        , m_isScaleYAnimationDirty(false)
        , m_isScaleZAnimationDirty(false)
        , m_isRotateXAnimationDirty(false)
        , m_isRotateYAnimationDirty(false)
        , m_isRotateZAnimationDirty(false)
        , m_isTranslateXAnimationDirty(false)
        , m_isTranslateYAnimationDirty(false)
        , m_isTranslateZAnimationDirty(false)
    {
    }

public:
    Microsoft::WRL::ComPtr<WUComp::IExpressionAnimation> m_scaleExpression;
    Microsoft::WRL::ComPtr<WUComp::IExpressionAnimation> m_rotateXExpression;
    Microsoft::WRL::ComPtr<WUComp::IExpressionAnimation> m_rotateYExpression;
    Microsoft::WRL::ComPtr<WUComp::IExpressionAnimation> m_rotateZExpression;
    Microsoft::WRL::ComPtr<WUComp::IExpressionAnimation> m_translateExpression;

    bool m_isCenterXAnimationDirty    : 1;
    bool m_isCenterYAnimationDirty    : 1;
    bool m_isCenterZAnimationDirty    : 1;
    bool m_isScaleXAnimationDirty     : 1;
    bool m_isScaleYAnimationDirty     : 1;
    bool m_isScaleZAnimationDirty     : 1;
    bool m_isRotateXAnimationDirty    : 1;
    bool m_isRotateYAnimationDirty    : 1;
    bool m_isRotateZAnimationDirty    : 1;
    bool m_isTranslateXAnimationDirty : 1;
    bool m_isTranslateYAnimationDirty : 1;
    bool m_isTranslateZAnimationDirty : 1;

    // Public API
    float m_rCenterX;
    float m_rCenterY;
    float m_rCenterZ;

    float m_rRotationX;
    float m_rRotationY;
    float m_rRotationZ;

    float m_rScaleX;
    float m_rScaleY;
    float m_rScaleZ;

    float m_rTranslateX;
    float m_rTranslateY;
    float m_rTranslateZ;
};
