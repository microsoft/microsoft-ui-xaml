// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "Projection.h"

class CPlaneProjection final : public CProjection
{
public:
    CPlaneProjection()
        : CPlaneProjection(nullptr)
    {
    }

    ~CPlaneProjection() override {};

    DECLARE_CREATE(CPlaneProjection);

    // PlaneProjection accessor to get the 3D matrix used to create a given plane projection.
    _Check_return_ HRESULT static GetProjectionMatrix(
        _In_ CDependencyObject *pObject,
        _In_ XUINT32 cArgs,
        _In_reads_(cArgs) CValue *pArgs,
        _In_opt_ IInspectable* pValueOuter,
        _Out_ CValue *pResult
        );

    // CDependencyObject overrides
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CPlaneProjection>::Index;
    }

    // CProjection overrides
    bool Is2DAligned() override
    {
        return (
            m_rRotationX == 0.0f &&
            m_rRotationY == 0.0f &&
            m_rRotationZ == 0.0f &&
            m_rLocalOffsetX == 0.0f &&
            m_rLocalOffsetY == 0.0f &&
            m_rLocalOffsetZ == 0.0f &&
            m_rGlobalOffsetX == 0.0f &&
            m_rGlobalOffsetY == 0.0f &&
            m_rGlobalOffsetZ == 0.0f );
    }

    void ReleaseDCompResources() final;

    void MakeWinRTExpression(
        _Inout_ WinRTExpressionConversionContext* pWinRTContext,
        float elementWidth,
        float elementHeight
        ) override;

    void ClearWUCExpression() override;

    void EnsureWUCAnimationStarted(_Inout_ WinRTExpressionConversionContext* context) override;

    CMILMatrix4x4 GetProjectionMatrix() const override;

protected:
    void Update3DTransform( _In_ XFLOAT rWidth, _In_ XFLOAT rHeight ) override;

private:
    CPlaneProjection(_In_ CCoreServices *pCore)
        : CProjection(pCore)
        , m_rLocalOffsetX(0.0f)
        , m_rLocalOffsetY(0.0f)
        , m_rLocalOffsetZ(0.0f)
        , m_rRotationX(0.0f)
        , m_rRotationY(0.0f)
        , m_rRotationZ(0.0f)
        , m_rCenterOfRotationX(0.5f)
        , m_rCenterOfRotationY(0.5f)
        , m_rCenterOfRotationZ(0.0f)
        , m_rGlobalOffsetX(0.0f)
        , m_rGlobalOffsetY(0.0f)
        , m_rGlobalOffsetZ(0.0f)
        , m_elementWidth(0.0f)
        , m_elementHeight(0.0f)
        , m_isLocalOffsetXAnimationDirty(false)
        , m_isLocalOffsetYAnimationDirty(false)
        , m_isLocalOffsetZAnimationDirty(false)
        , m_isRotationXAnimationDirty(false)
        , m_isRotationYAnimationDirty(false)
        , m_isRotationZAnimationDirty(false)
        , m_isCenterOfRotationXAnimationDirty(false)
        , m_isCenterOfRotationYAnimationDirty(false)
        , m_isCenterOfRotationZAnimationDirty(false)
        , m_isGlobalOffsetXAnimationDirty(false)
        , m_isGlobalOffsetYAnimationDirty(false)
        , m_isGlobalOffsetZAnimationDirty(false)
    {
    }

public:
    // Public API
    XFLOAT m_rLocalOffsetX;
    XFLOAT m_rLocalOffsetY;
    XFLOAT m_rLocalOffsetZ;

    XFLOAT m_rRotationX;
    XFLOAT m_rRotationY;
    XFLOAT m_rRotationZ;
    XFLOAT m_rCenterOfRotationX;
    XFLOAT m_rCenterOfRotationY;
    XFLOAT m_rCenterOfRotationZ;

    XFLOAT m_rGlobalOffsetX;
    XFLOAT m_rGlobalOffsetY;
    XFLOAT m_rGlobalOffsetZ;

    float m_elementWidth;
    float m_elementHeight;

    Microsoft::WRL::ComPtr<WUComp::IExpressionAnimation> m_localOffsetExpression;
    Microsoft::WRL::ComPtr<WUComp::IExpressionAnimation> m_rotationExpression;
    Microsoft::WRL::ComPtr<WUComp::IExpressionAnimation> m_rotateCenterExpression;
    Microsoft::WRL::ComPtr<WUComp::IExpressionAnimation> m_undoRotateCenterExpression;
    Microsoft::WRL::ComPtr<WUComp::IExpressionAnimation> m_rotateXExpression;
    Microsoft::WRL::ComPtr<WUComp::IExpressionAnimation> m_rotateYExpression;
    Microsoft::WRL::ComPtr<WUComp::IExpressionAnimation> m_rotateZExpression;
    Microsoft::WRL::ComPtr<WUComp::IExpressionAnimation> m_globalOffsetExpression;
    Microsoft::WRL::ComPtr<WUComp::IExpressionAnimation> m_perspectiveExpression;

    // DComp animation dirty flags
    bool m_isLocalOffsetXAnimationDirty : 1;
    bool m_isLocalOffsetYAnimationDirty : 1;
    bool m_isLocalOffsetZAnimationDirty : 1;
    bool m_isRotationXAnimationDirty : 1;
    bool m_isRotationYAnimationDirty : 1;
    bool m_isRotationZAnimationDirty : 1;
    bool m_isCenterOfRotationXAnimationDirty : 1;
    bool m_isCenterOfRotationYAnimationDirty : 1;
    bool m_isCenterOfRotationZAnimationDirty : 1;
    bool m_isGlobalOffsetXAnimationDirty : 1;
    bool m_isGlobalOffsetYAnimationDirty : 1;
    bool m_isGlobalOffsetZAnimationDirty : 1;

    // Projection constants
    static constexpr bool c_rightHanded = true;
    static constexpr float c_nearPlane = 1.0f;
    static constexpr XFLOAT c_farPlane = 1001.0f;
    static constexpr XFLOAT c_fieldOfView = 57.0f;
    static constexpr XFLOAT c_zOffset = -999.0f;    // if left handed, 999.0f
};
