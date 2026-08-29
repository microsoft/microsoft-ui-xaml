// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "CompositeTransform.h"
#include "WinRTExpressionConversionContext.h"
#include "ExpressionHelper.h"
#include "Matrix.h"
#include <DependencyObjectDCompRegistry.h>
#include <stack_vector.h>

void CCompositeTransform::GetTransform(_Out_ CMILMatrix *pMatrix)
{
    if (m_fDirty)
    {
        CMILMatrix t;

        // Order of operations
        //      Shift to center point
        //      Scale
        //      Skew
        //      Rotate
        //      Undo shift
        //      Translate

        m_compositeMatrix.SetToIdentity();

        if (m_ptCenter.x != 0.0f || m_ptCenter.y != 0.0f)
        {
            t.SetM11(1.0f);
            t.SetM12(0.0f);
            t.SetM21(0.0f);
            t.SetM22(1.0f);
            t.SetDx(-m_ptCenter.x);
            t.SetDy(-m_ptCenter.y);

            m_compositeMatrix.Append(t);
        }

        if (m_eScaleX != 1.0f || m_eScaleY != 1.0f)
        {
            t.SetM11(m_eScaleX);
            t.SetM12(0.0f);
            t.SetM21(0.0f);
            t.SetM22(m_eScaleY);
            t.SetDx(0.0f);
            t.SetDy(0.0f);

            m_compositeMatrix.Append(t);
        }

        if (m_eSkewX != 0.0f || m_eSkewY != 0.0f)
        {
            XPOINTF zero = {};
            BuildSkewMatrix(&t, m_eSkewX, m_eSkewY, zero);
            m_compositeMatrix.Append(t);
        }

        if (m_eRotation != 0.0f)
        {
            XPOINTF zero = {};
            BuildRotateMatrix(&t, m_eRotation, zero);
            m_compositeMatrix.Append(t);
        }

        if (m_ptCenter.x != 0.0f || m_ptCenter.y != 0.0f ||
            m_eTranslateX != 0.0f || m_eTranslateY != 0.0f)
        {
            // Undo the center point shift and apply any translation.

            t.SetM11(1.0f);
            t.SetM12(0.0f);
            t.SetM21(0.0f);
            t.SetM22(1.0f);
            t.SetDx(m_ptCenter.x + m_eTranslateX);
            t.SetDy(m_ptCenter.y + m_eTranslateY);

            m_compositeMatrix.Append(t);
        }

        m_fDirty = FALSE;
    }

    *pMatrix = m_compositeMatrix;
}

void CCompositeTransform::ReleaseDCompResources()
{
    __super::ReleaseDCompResources();

    SetDCompAnimation(nullptr, KnownPropertyIndex::CompositeTransform_CenterXAnimation);
    SetDCompAnimation(nullptr, KnownPropertyIndex::CompositeTransform_CenterYAnimation);
    SetDCompAnimation(nullptr, KnownPropertyIndex::CompositeTransform_ScaleXAnimation);
    SetDCompAnimation(nullptr, KnownPropertyIndex::CompositeTransform_ScaleYAnimation);
    SetDCompAnimation(nullptr, KnownPropertyIndex::CompositeTransform_SkewXAnimation);
    SetDCompAnimation(nullptr, KnownPropertyIndex::CompositeTransform_SkewYAnimation);
    SetDCompAnimation(nullptr, KnownPropertyIndex::CompositeTransform_RotateAnimation);
    SetDCompAnimation(nullptr, KnownPropertyIndex::CompositeTransform_TranslateXAnimation);
    SetDCompAnimation(nullptr, KnownPropertyIndex::CompositeTransform_TranslateYAnimation);

    m_spScaleExpression.Reset();
    m_spRotateExpression.Reset();
    m_spSkewExpression.Reset();
    m_spTranslateExpression.Reset();
}

void CCompositeTransform::MakeWinRTExpression(
    _Inout_ WinRTExpressionConversionContext* pWinRTContext,
    _Inout_opt_ ixp::ICompositionPropertySet* pTransformGroupPS
    )
{
    // Even a transform that is clean might need to create an expression, if it's part of a transform group where some other
    // transform started animating, so check both the dirty flag and that we already have an expression. The corollary is that
    // calling this method on a clean transform will still create an expression for it.
    if (m_isWinRTExpressionDirty || m_spWinRTExpression == nullptr)
    {
        const auto& scaleXAnimation = GetWUCDCompAnimation(KnownPropertyIndex::CompositeTransform_ScaleXAnimation);
        const auto& scaleYAnimation = GetWUCDCompAnimation(KnownPropertyIndex::CompositeTransform_ScaleYAnimation);
        const auto& skewXAnimation = GetWUCDCompAnimation(KnownPropertyIndex::CompositeTransform_SkewXAnimation);
        const auto& skewYAnimation = GetWUCDCompAnimation(KnownPropertyIndex::CompositeTransform_SkewYAnimation);
        const auto& rotateAnimation = GetWUCDCompAnimation(KnownPropertyIndex::CompositeTransform_RotateAnimation);
        const auto& translateXAnimation = GetWUCDCompAnimation(KnownPropertyIndex::CompositeTransform_TranslateXAnimation);
        const auto& translateYAnimation = GetWUCDCompAnimation(KnownPropertyIndex::CompositeTransform_TranslateYAnimation);
        const auto& centerXAnimation = GetWUCDCompAnimation(KnownPropertyIndex::CompositeTransform_CenterXAnimation);
        const auto& centerYAnimation = GetWUCDCompAnimation(KnownPropertyIndex::CompositeTransform_CenterYAnimation);
        const auto& timeManager = GetTimeManager();

        // Most CCompositeTransforms are axis aligned (just a scale and a translation) and don't have a center point.
        // These don't need a big chained expression of 6 components multiplied together. They can just make a matrix
        // with the 4 numbers.
        const bool canUseSimpleExpression =
            !skewXAnimation && !skewYAnimation && !rotateAnimation && !centerXAnimation && !centerYAnimation &&
            m_eSkewX == 0 && m_eSkewY == 0 && m_eRotation == 0 && m_ptCenter.x == 0 && m_ptCenter.y == 0;

        if (canUseSimpleExpression != m_hasSimpleExpression)
        {
            m_spWinRTExpression.Reset();
        }

        // Create overall CompositeTransform expression
        if (m_spWinRTExpression == nullptr)
        {
            if (canUseSimpleExpression)
            {
                m_spWinRTExpression = CreateSimpleExpression(pWinRTContext, true /* use2D */);
            }
            else
            {
                wrl::ComPtr<ixp::ICompositionPropertySet> propertySet;
                pWinRTContext->CreateExpression(ExpressionHelper::sc_Expression_CompositeTransform_CenterPoint, &m_spWinRTExpression, &propertySet);

                pWinRTContext->InitializeExpression(propertySet.Get(), ExpressionHelper::sc_paramName_CenterX, m_ptCenter.x);
                pWinRTContext->InitializeExpression(propertySet.Get(), ExpressionHelper::sc_paramName_CenterY, m_ptCenter.y);
            }

            if (GetDCompObjectRegistry() != nullptr)
            {
                GetDCompObjectRegistry()->EnsureObjectWithDCompResourceRegistered(this);
            }

            m_hasSimpleExpression = canUseSimpleExpression;
        }

        if (m_hasSimpleExpression)
        {
            UpdateSimpleExpression(pWinRTContext, m_spWinRTExpression.Get());
        }
        else
        {
            // Set each of the four components (Translate/Scale/Rotate/Skew)
            if (m_eScaleX != 1 || m_eScaleY != 1 || scaleXAnimation != nullptr || scaleYAnimation != nullptr)
            {
                if (m_spScaleExpression == nullptr)
                {
                    // Note we use simple form of the expression, since CompositeTransform components don't have individual center points
                    wrl::ComPtr<ixp::ICompositionPropertySet> propertySet;
                    pWinRTContext->CreateExpression(ExpressionHelper::sc_Expression_Scale, &m_spScaleExpression, &propertySet);

                    pWinRTContext->InitializeExpression(propertySet.Get(), ExpressionHelper::sc_paramName_ScaleX, m_eScaleX);
                    pWinRTContext->InitializeExpression(propertySet.Get(), ExpressionHelper::sc_paramName_ScaleY, m_eScaleY);
                }

                pWinRTContext->UpdateExpression(m_spScaleExpression.Get(), ExpressionHelper::sc_paramName_ScaleX, m_eScaleX, m_isScaleXAnimationDirty, scaleXAnimation.get(), this, KnownPropertyIndex::CompositeTransform_ScaleX, timeManager);
                pWinRTContext->UpdateExpression(m_spScaleExpression.Get(), ExpressionHelper::sc_paramName_ScaleY, m_eScaleY, m_isScaleYAnimationDirty, scaleYAnimation.get(), this, KnownPropertyIndex::CompositeTransform_ScaleY, timeManager);
                pWinRTContext->UpdateCompositeTransformComponent(false /* useMatrix4x4 */, m_spWinRTExpression.Get(), m_spScaleExpression.Get(), ExpressionHelper::sc_paramName_Scale);
            }
            else
            {
                pWinRTContext->UpdateCompositeTransformComponent(false /* useMatrix4x4 */, m_spWinRTExpression.Get(), nullptr /* componentExpression */, ExpressionHelper::sc_paramName_Scale);
                m_spScaleExpression.Reset();
            }

            if (m_eSkewX != 0 || m_eSkewY != 0 || skewXAnimation != nullptr || skewYAnimation != nullptr)
            {
                if (m_spSkewExpression == nullptr)
                {
                    // Note we use simple form of the expression, since CompositeTransform components don't have individual center points
                    wrl::ComPtr<ixp::ICompositionPropertySet> propertySet;
                    pWinRTContext->CreateExpression(ExpressionHelper::sc_Expression_Skew, &m_spSkewExpression, &propertySet);

                    pWinRTContext->InitializeExpression(propertySet.Get(), ExpressionHelper::sc_paramName_SkewAngleX, m_eSkewX);
                    pWinRTContext->InitializeExpression(propertySet.Get(), ExpressionHelper::sc_paramName_SkewAngleY, m_eSkewY);
                }

                pWinRTContext->UpdateExpression(m_spSkewExpression.Get(), ExpressionHelper::sc_paramName_SkewAngleX, m_eSkewX, m_isSkewXAnimationDirty, skewXAnimation.get(), this, KnownPropertyIndex::CompositeTransform_SkewX, timeManager);
                pWinRTContext->UpdateExpression(m_spSkewExpression.Get(), ExpressionHelper::sc_paramName_SkewAngleY, m_eSkewY, m_isSkewYAnimationDirty, skewYAnimation.get(), this, KnownPropertyIndex::CompositeTransform_SkewY, timeManager);
                pWinRTContext->UpdateCompositeTransformComponent(false /* useMatrix4x4 */, m_spWinRTExpression.Get(), m_spSkewExpression.Get(), ExpressionHelper::sc_paramName_Skew);
            }
            else
            {
                pWinRTContext->UpdateCompositeTransformComponent(false /* useMatrix4x4 */, m_spWinRTExpression.Get(), nullptr /* componentExpression */, ExpressionHelper::sc_paramName_Skew);
                m_spSkewExpression.Reset();
            }

            if (m_eRotation != 0 || rotateAnimation != nullptr)
            {
                if (m_spRotateExpression == nullptr)
                {
                    // Note we use simple form of the expression, since CompositeTransform components don't have individual center points
                    wrl::ComPtr<ixp::ICompositionPropertySet> propertySet;
                    pWinRTContext->CreateExpression(ExpressionHelper::sc_Expression_Rotate, &m_spRotateExpression, &propertySet);

                    pWinRTContext->InitializeExpression(propertySet.Get(), ExpressionHelper::sc_paramName_RotateAngle, m_eRotation);
                }

                pWinRTContext->UpdateExpression(m_spRotateExpression.Get(), ExpressionHelper::sc_paramName_RotateAngle, m_eRotation, m_isRotateAnimationDirty, rotateAnimation.get(), this, KnownPropertyIndex::CompositeTransform_Rotation, timeManager);
                pWinRTContext->UpdateCompositeTransformComponent(false /* useMatrix4x4 */, m_spWinRTExpression.Get(), m_spRotateExpression.Get(), ExpressionHelper::sc_paramName_Rotate);
            }
            else
            {
                pWinRTContext->UpdateCompositeTransformComponent(false /* useMatrix4x4 */, m_spWinRTExpression.Get(), nullptr /* componentExpression */, ExpressionHelper::sc_paramName_Rotate);
                m_spRotateExpression.Reset();
            }

            if (m_eTranslateX != 0 || m_eTranslateY != 0 || translateXAnimation != nullptr || translateYAnimation != nullptr)
            {
                if (m_spTranslateExpression == nullptr)
                {
                    wrl::ComPtr<ixp::ICompositionPropertySet> propertySet;
                    pWinRTContext->CreateExpression(ExpressionHelper::sc_Expression_Translate, &m_spTranslateExpression, &propertySet);

                    pWinRTContext->InitializeExpression(propertySet.Get(), ExpressionHelper::sc_paramName_TranslateX, m_eTranslateX);
                    pWinRTContext->InitializeExpression(propertySet.Get(), ExpressionHelper::sc_paramName_TranslateY, m_eTranslateY);
                }

                pWinRTContext->UpdateExpression(m_spTranslateExpression.Get(), ExpressionHelper::sc_paramName_TranslateX, m_eTranslateX, m_isTranslateXAnimationDirty, translateXAnimation.get(), this, KnownPropertyIndex::CompositeTransform_TranslateX, timeManager);
                pWinRTContext->UpdateExpression(m_spTranslateExpression.Get(), ExpressionHelper::sc_paramName_TranslateY, m_eTranslateY, m_isTranslateYAnimationDirty, translateYAnimation.get(), this, KnownPropertyIndex::CompositeTransform_TranslateY, timeManager);
                pWinRTContext->UpdateCompositeTransformComponent(false /* useMatrix4x4 */, m_spWinRTExpression.Get(), m_spTranslateExpression.Get(), ExpressionHelper::sc_paramName_Translate);
            }
            else
            {
                pWinRTContext->UpdateCompositeTransformComponent(false /* useMatrix4x4 */, m_spWinRTExpression.Get(), m_spTranslateExpression.Get(), ExpressionHelper::sc_paramName_Translate);
                m_spTranslateExpression.Reset();
            }

            // Update CenterOffset in overall expression
            pWinRTContext->UpdateExpression(m_spWinRTExpression.Get(), ExpressionHelper::sc_paramName_CenterX, m_ptCenter.x, m_isCenterXAnimationDirty, centerXAnimation.get(), this, KnownPropertyIndex::CompositeTransform_CenterX, timeManager);
            pWinRTContext->UpdateExpression(m_spWinRTExpression.Get(), ExpressionHelper::sc_paramName_CenterY, m_ptCenter.y, m_isCenterYAnimationDirty, centerYAnimation.get(), this, KnownPropertyIndex::CompositeTransform_CenterY, timeManager);
        }

        m_isWinRTExpressionDirty = false;
        m_isCenterXAnimationDirty = false;
        m_isCenterYAnimationDirty = false;
        m_isScaleXAnimationDirty = false;
        m_isScaleYAnimationDirty = false;
        m_isSkewXAnimationDirty = false;
        m_isSkewYAnimationDirty = false;
        m_isRotateAnimationDirty = false;
        m_isTranslateXAnimationDirty = false;
        m_isTranslateYAnimationDirty = false;
    }

    if (pTransformGroupPS)
    {
        pWinRTContext->AddExpressionToTransformGroupPropertySet(m_spWinRTExpression.Get(), pTransformGroupPS);
    }
}

wrl::ComPtr<ixp::IExpressionAnimation> CCompositeTransform::CreateSimpleExpression(_In_ WinRTExpressionConversionContext* conversionContext, bool use2D)
{
    wrl::ComPtr<ixp::IExpressionAnimation> expression;
    wrl::ComPtr<ixp::ICompositionPropertySet> propertySet;
    conversionContext->CreateExpression(use2D ? ExpressionHelper::sc_Expression_CompositeTransform_ScaleTranslate : ExpressionHelper::sc_Expression_CompositeTransform_ScaleTranslate_4x4, &expression, &propertySet);

    conversionContext->InitializeExpression(propertySet.Get(), ExpressionHelper::sc_paramName_ScaleX, m_eScaleX);
    conversionContext->InitializeExpression(propertySet.Get(), ExpressionHelper::sc_paramName_ScaleY, m_eScaleY);
    conversionContext->InitializeExpression(propertySet.Get(), ExpressionHelper::sc_paramName_TranslateX, m_eTranslateX);
    conversionContext->InitializeExpression(propertySet.Get(), ExpressionHelper::sc_paramName_TranslateY, m_eTranslateY);

    return expression;
}

void CCompositeTransform::UpdateSimpleExpression(_In_ WinRTExpressionConversionContext* conversionContext, _In_ ixp::IExpressionAnimation* expression)
{
    const auto& scaleXAnimation = GetWUCDCompAnimation(KnownPropertyIndex::CompositeTransform_ScaleXAnimation);
    const auto& scaleYAnimation = GetWUCDCompAnimation(KnownPropertyIndex::CompositeTransform_ScaleYAnimation);
    const auto& translateXAnimation = GetWUCDCompAnimation(KnownPropertyIndex::CompositeTransform_TranslateXAnimation);
    const auto& translateYAnimation = GetWUCDCompAnimation(KnownPropertyIndex::CompositeTransform_TranslateYAnimation);
    const auto& timeManager = GetTimeManager();

    // Just update the scales and translates. The expression makes a Matrix3x2 with these 4 numbers/animations.
    conversionContext->UpdateExpression(expression, ExpressionHelper::sc_paramName_ScaleX, m_eScaleX, m_isScaleXAnimationDirty, scaleXAnimation.get(), this, KnownPropertyIndex::CompositeTransform_ScaleX, timeManager);
    conversionContext->UpdateExpression(expression, ExpressionHelper::sc_paramName_ScaleY, m_eScaleY, m_isScaleYAnimationDirty, scaleYAnimation.get(), this, KnownPropertyIndex::CompositeTransform_ScaleY, timeManager);
    conversionContext->UpdateExpression(expression, ExpressionHelper::sc_paramName_TranslateX, m_eTranslateX, m_isTranslateXAnimationDirty, translateXAnimation.get(), this, KnownPropertyIndex::CompositeTransform_TranslateX, timeManager);
    conversionContext->UpdateExpression(expression, ExpressionHelper::sc_paramName_TranslateY, m_eTranslateY, m_isTranslateYAnimationDirty, translateYAnimation.get(), this, KnownPropertyIndex::CompositeTransform_TranslateY, timeManager);
}

void CCompositeTransform::ClearWUCExpression()
{
    __super::ClearWUCExpression();

    m_spScaleExpression.Reset();
    m_spRotateExpression.Reset();
    m_spSkewExpression.Reset();
    m_spTranslateExpression.Reset();

    m_isCenterXAnimationDirty = false;
    m_isCenterYAnimationDirty = false;
    m_isScaleXAnimationDirty = false;
    m_isScaleYAnimationDirty = false;
    m_isSkewXAnimationDirty = false;
    m_isSkewYAnimationDirty = false;
    m_isRotateAnimationDirty = false;
    m_isTranslateXAnimationDirty = false;
    m_isTranslateYAnimationDirty = false;
}
