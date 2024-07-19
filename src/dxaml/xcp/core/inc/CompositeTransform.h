// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "Transform.h"
#include "Matrix.h"

class WinRTExpressionConversionContext;

// Transform class that lets users specify several of the common
// transformations in a fixed order.  This order is common in the
// industry and allows designers to express most useful transforms
// with much less XAML and consequently fewer DependencyObjects.
class CCompositeTransform final : public CTransform
{
private:
    CCompositeTransform(_In_ CCoreServices *pCore)
        : CTransform(pCore)
        , m_isCenterXAnimationDirty(false)
        , m_isCenterYAnimationDirty(false)
        , m_isScaleXAnimationDirty(false)
        , m_isScaleYAnimationDirty(false)
        , m_isSkewXAnimationDirty(false)
        , m_isSkewYAnimationDirty(false)
        , m_isRotateAnimationDirty(false)
        , m_isTranslateXAnimationDirty(false)
        , m_isTranslateYAnimationDirty(false)
    {
        m_fDirty      = FALSE; // Transform starts as identity so it's not dirty
        m_isTransitionClipTransform = FALSE;
        m_hasMarkedParentAsAnimated = FALSE;
        m_hasSimpleExpression = false;
        m_compositeMatrix.SetToIdentity();
    }

protected:
    CCompositeTransform(_In_ const CCompositeTransform& original, _Out_ HRESULT& hr)
        : CTransform(original,hr)
        , m_isCenterXAnimationDirty(false)
        , m_isCenterYAnimationDirty(false)
        , m_isScaleXAnimationDirty(false)
        , m_isScaleYAnimationDirty(false)
        , m_isSkewXAnimationDirty(false)
        , m_isSkewYAnimationDirty(false)
        , m_isRotateAnimationDirty(false)
        , m_isTranslateXAnimationDirty(false)
        , m_isTranslateYAnimationDirty(false)
        , m_hasUndoCenterTransformFlip(false)
    {
        m_ptCenter.x      = original.m_ptCenter.x;
        m_ptCenter.y      = original.m_ptCenter.y;
        m_eScaleX         = original.m_eScaleX;
        m_eScaleY         = original.m_eScaleY;
        m_eSkewX          = original.m_eSkewX;
        m_eSkewY          = original.m_eSkewY;
        m_eRotation       = original.m_eRotation;
        m_eTranslateX     = original.m_eTranslateX;
        m_eTranslateY     = original.m_eTranslateY;
        m_compositeMatrix = original.m_compositeMatrix;
        m_fDirty          = original.m_fDirty;
        m_isTransitionClipTransform = original.m_isTransitionClipTransform;
        m_hasMarkedParentAsAnimated = original.m_hasMarkedParentAsAnimated;
        m_hasSimpleExpression = original.m_hasSimpleExpression;
    }

public:
    CCompositeTransform()
        : CCompositeTransform(nullptr)
    {
        m_fDirty = true;    // This ctor is for testing, which will set fields directly and will not mark the transform as dirty.
    }

    DECLARE_CREATE(CCompositeTransform);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CCompositeTransform>::Index;
    }

    _Check_return_ HRESULT SetValue(_In_ const SetValueParams& args) override;

    DECLARE_SHAREABLEDEPENDENCYOBJECT_CLONE(CCompositeTransform);

    void GetTransform(_Out_ CMILMatrix *pMatrix) override;

    static void NWSetDirty(_In_ CDependencyObject *pTarget, DirtyFlags flags);

    void SetIsTransitionClipTransform() { m_isTransitionClipTransform = TRUE; }
    bool IsTransitionClipTransform() const { return m_isTransitionClipTransform; }

    void ReleaseDCompResources() final;

    void MakeWinRTExpression(
        _Inout_ WinRTExpressionConversionContext* pWinRTContext,
        _Inout_opt_ ixp::ICompositionPropertySet* pTransformGroupPS = nullptr
        ) override;

    void ClearWUCExpression() override;

    wrl::ComPtr<ixp::IExpressionAnimation> CreateSimpleExpression(_In_ WinRTExpressionConversionContext* conversionContext, bool use2D);
    void UpdateSimpleExpression(_In_ WinRTExpressionConversionContext* conversionContext, _In_ ixp::IExpressionAnimation* expression);

    XPOINTF m_ptCenter = {};
    XFLOAT m_eScaleX = 1.0f;
    XFLOAT m_eScaleY = 1.0f;
    XFLOAT m_eSkewX = 0.0f;
    XFLOAT m_eSkewY = 0.0f;
    XFLOAT m_eRotation = 0.0f;
    XFLOAT m_eTranslateX = 0.0f;
    XFLOAT m_eTranslateY = 0.0f;

    Microsoft::WRL::ComPtr<ixp::IExpressionAnimation> m_spScaleExpression;
    Microsoft::WRL::ComPtr<ixp::IExpressionAnimation> m_spRotateExpression;
    Microsoft::WRL::ComPtr<ixp::IExpressionAnimation> m_spSkewExpression;
    Microsoft::WRL::ComPtr<ixp::IExpressionAnimation> m_spTranslateExpression;

    bool m_isCenterXAnimationDirty : 1;
    bool m_isCenterYAnimationDirty : 1;
    bool m_isScaleXAnimationDirty : 1;
    bool m_isScaleYAnimationDirty : 1;
    bool m_isSkewXAnimationDirty : 1;
    bool m_isSkewYAnimationDirty : 1;
    bool m_isRotateAnimationDirty : 1;
    bool m_isTranslateXAnimationDirty : 1;
    bool m_isTranslateYAnimationDirty : 1;
    bool m_hasUndoCenterTransformFlip : 1;

private:
    bool m_fDirty : 1;
    bool m_isTransitionClipTransform : 1;
    bool m_hasMarkedParentAsAnimated : 1;

    // Most CompositeTransforms have no rotation, skew, or center point. In that case we can just use a simple
    // "Matrix3x2(sx, 0, 0, sy, x, y)" expression rather than multiply together the different components. This flag
    // marks whether we're using a simple expression (stored in the base's CTransform::m_spWinRTExpression).
    bool m_hasSimpleExpression : 1;

    CMILMatrix m_compositeMatrix;
};
