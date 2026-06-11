// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "CompositeTransform3D.h"
#include <DependencyObjectDCompRegistry.h>
#include <stack_vector.h>
#include "WinRTExpressionConversionContext.h"
#include "ExpressionHelper.h"

// Update the matrix that represents the transforms
void CCompositeTransform3D::UpdateTransformMatrix(
    float elementWidth,
    float elementHeight)
{
    CMILMatrix4x4 matTemp;
    CMILMatrix4x4 matFwdCenter;
    CMILMatrix4x4 matBackCenter;

    matFwdCenter.SetToTranslation(-m_rCenterX, -m_rCenterY, -m_rCenterZ);
    matBackCenter.SetToTranslation(m_rCenterX, m_rCenterY, m_rCenterZ);

    // Compute 3D transformation matrix
    // Order of operations
    //      [Leaf]
    //      Scale
    //      Rotate (X, Y, Z)
    //      Translate
    //      [Root]

    // Apply Scale
    m_matTransform3D.SetToScale(m_rScaleX, m_rScaleY, m_rScaleZ);

    // Apply rotations
    matTemp.SetToRotationX(m_rRotationX);
    m_matTransform3D.Append(matTemp);

    matTemp.SetToRotationY(m_rRotationY);
    m_matTransform3D.Append(matTemp);

    matTemp.SetToRotationZ(m_rRotationZ);
    m_matTransform3D.Append(matTemp);

    m_matTransform3D.Prepend(matFwdCenter);
    m_matTransform3D.Append(matBackCenter);

    // Apply Translation
    matTemp.SetToTranslation(m_rTranslateX, m_rTranslateY, m_rTranslateZ);
    m_matTransform3D.Append(matTemp);
}

bool CCompositeTransform3D::IsRenderedTransform2D() const
{
    return m_matTransform3D.Is2D();
}

bool CCompositeTransform3D::HasDepth()
{
    UpdateTransformMatrix(0, 0);

    return !m_matTransform3D.Is2D();
}

void CCompositeTransform3D::ReleaseDCompResources()
{
    __super::ReleaseDCompResources();

    SetDCompAnimation(nullptr, KnownPropertyIndex::CompositeTransform3D_CenterXAnimation);
    SetDCompAnimation(nullptr, KnownPropertyIndex::CompositeTransform3D_CenterYAnimation);
    SetDCompAnimation(nullptr, KnownPropertyIndex::CompositeTransform3D_CenterZAnimation);
    SetDCompAnimation(nullptr, KnownPropertyIndex::CompositeTransform3D_ScaleXAnimation);
    SetDCompAnimation(nullptr, KnownPropertyIndex::CompositeTransform3D_ScaleYAnimation);
    SetDCompAnimation(nullptr, KnownPropertyIndex::CompositeTransform3D_ScaleZAnimation);
    SetDCompAnimation(nullptr, KnownPropertyIndex::CompositeTransform3D_RotationXAnimation);
    SetDCompAnimation(nullptr, KnownPropertyIndex::CompositeTransform3D_RotationYAnimation);
    SetDCompAnimation(nullptr, KnownPropertyIndex::CompositeTransform3D_RotationZAnimation);
    SetDCompAnimation(nullptr, KnownPropertyIndex::CompositeTransform3D_TranslateXAnimation);
    SetDCompAnimation(nullptr, KnownPropertyIndex::CompositeTransform3D_TranslateYAnimation);
    SetDCompAnimation(nullptr, KnownPropertyIndex::CompositeTransform3D_TranslateZAnimation);

    m_scaleExpression.Reset();
    m_rotateXExpression.Reset();
    m_rotateYExpression.Reset();
    m_rotateZExpression.Reset();
    m_translateExpression.Reset();
}

void CCompositeTransform3D::MakeWinRTExpression(
    _Inout_ WinRTExpressionConversionContext* pWinRTContext,
    float elementWidth,
    float elementHeight
    )
{
    UNREFERENCED_PARAMETER(elementWidth);
    UNREFERENCED_PARAMETER(elementHeight);

    if (m_isWinRTExpressionDirty)
    {
        const auto& centerXAnimation = GetWUCDCompAnimation(KnownPropertyIndex::CompositeTransform3D_CenterXAnimation);
        const auto& centerYAnimation = GetWUCDCompAnimation(KnownPropertyIndex::CompositeTransform3D_CenterYAnimation);
        const auto& centerZAnimation = GetWUCDCompAnimation(KnownPropertyIndex::CompositeTransform3D_CenterZAnimation);
        const auto& scaleXAnimation = GetWUCDCompAnimation(KnownPropertyIndex::CompositeTransform3D_ScaleXAnimation);
        const auto& scaleYAnimation = GetWUCDCompAnimation(KnownPropertyIndex::CompositeTransform3D_ScaleYAnimation);
        const auto& scaleZAnimation = GetWUCDCompAnimation(KnownPropertyIndex::CompositeTransform3D_ScaleZAnimation);
        const auto& rotateXAnimation = GetWUCDCompAnimation(KnownPropertyIndex::CompositeTransform3D_RotationXAnimation);
        const auto& rotateYAnimation = GetWUCDCompAnimation(KnownPropertyIndex::CompositeTransform3D_RotationYAnimation);
        const auto& rotateZAnimation = GetWUCDCompAnimation(KnownPropertyIndex::CompositeTransform3D_RotationZAnimation);
        const auto& translateXAnimation = GetWUCDCompAnimation(KnownPropertyIndex::CompositeTransform3D_TranslateXAnimation);
        const auto& translateYAnimation = GetWUCDCompAnimation(KnownPropertyIndex::CompositeTransform3D_TranslateYAnimation);
        const auto& translateZAnimation = GetWUCDCompAnimation(KnownPropertyIndex::CompositeTransform3D_TranslateZAnimation);
        const auto& timeManager = GetTimeManager();

        // Create overall CompositeTransform expression
        if (m_spWinRTExpression == nullptr)
        {
            wrl::ComPtr<WUComp::ICompositionPropertySet> propertySet;
            pWinRTContext->CreateExpression(ExpressionHelper::sc_Expression_CompositeTransform_CenterPoint3D, &m_spWinRTExpression, &propertySet);

            pWinRTContext->InitializeExpression(propertySet.Get(), ExpressionHelper::sc_paramName_CenterX, m_rCenterX);
            pWinRTContext->InitializeExpression(propertySet.Get(), ExpressionHelper::sc_paramName_CenterY, m_rCenterY);
            pWinRTContext->InitializeExpression(propertySet.Get(), ExpressionHelper::sc_paramName_CenterZ, m_rCenterZ);

            if (GetDCompObjectRegistry() != nullptr)
            {
                GetDCompObjectRegistry()->EnsureObjectWithDCompResourceRegistered(this);
            }
        }

        // Set each of the 5 components (Scale/RotateX/RotateY/RotateZ/Translate)
        // Note we try to avoid creating an expression if the component is not set, and destroy the expression if it takes on a static matrix with default value
        if (m_rScaleX != 1 || m_rScaleY != 1 || m_rScaleZ != 1 || scaleXAnimation != nullptr || scaleYAnimation != nullptr || scaleZAnimation != nullptr)
        {
            if (m_scaleExpression == nullptr)
            {
                // Note we use simple form of the expression, since CompositeTransform components don't have individual center points
                wrl::ComPtr<WUComp::ICompositionPropertySet> propertySet;
                pWinRTContext->CreateExpression(ExpressionHelper::sc_Expression_Scale3D, &m_scaleExpression, &propertySet);

                pWinRTContext->InitializeExpression(propertySet.Get(), ExpressionHelper::sc_paramName_ScaleX, m_rScaleX);
                pWinRTContext->InitializeExpression(propertySet.Get(), ExpressionHelper::sc_paramName_ScaleY, m_rScaleY);
                pWinRTContext->InitializeExpression(propertySet.Get(), ExpressionHelper::sc_paramName_ScaleZ, m_rScaleZ);
            }

            pWinRTContext->UpdateExpression(m_scaleExpression.Get(), ExpressionHelper::sc_paramName_ScaleX, m_rScaleX, m_isScaleXAnimationDirty, scaleXAnimation.get(), this, KnownPropertyIndex::CompositeTransform3D_ScaleX, timeManager);
            pWinRTContext->UpdateExpression(m_scaleExpression.Get(), ExpressionHelper::sc_paramName_ScaleY, m_rScaleY, m_isScaleYAnimationDirty, scaleYAnimation.get(), this, KnownPropertyIndex::CompositeTransform3D_ScaleY, timeManager);
            pWinRTContext->UpdateExpression(m_scaleExpression.Get(), ExpressionHelper::sc_paramName_ScaleZ, m_rScaleZ, m_isScaleZAnimationDirty, scaleZAnimation.get(), this, KnownPropertyIndex::CompositeTransform3D_ScaleZ, timeManager);
            pWinRTContext->UpdateCompositeTransformComponent(true /* useMatrix4x4 */, m_spWinRTExpression.Get(), m_scaleExpression.Get(), ExpressionHelper::sc_paramName_Scale);
        }
        else
        {
            pWinRTContext->UpdateCompositeTransformComponent(true /* useMatrix4x4 */, m_spWinRTExpression.Get(), nullptr /* componentExpression */, ExpressionHelper::sc_paramName_Scale);
            m_scaleExpression.Reset();
        }

        if (m_rRotationX != 0 || rotateXAnimation != nullptr)
        {
            if (m_rotateXExpression == nullptr)
            {
                // Note we use simple form of the expression, since CompositeTransform components don't have individual center points
                wrl::ComPtr<WUComp::ICompositionPropertySet> propertySet;
                pWinRTContext->CreateExpression(ExpressionHelper::sc_Expression_Rotate3D_X, &m_rotateXExpression, &propertySet);

                pWinRTContext->InitializeExpression(propertySet.Get(), ExpressionHelper::sc_paramName_RotateAngle, m_rRotationX);
            }

            pWinRTContext->UpdateExpression(m_rotateXExpression.Get(), ExpressionHelper::sc_paramName_RotateAngle, m_rRotationX, m_isRotateXAnimationDirty, rotateXAnimation.get(), this, KnownPropertyIndex::CompositeTransform3D_RotationX, timeManager);
            pWinRTContext->UpdateCompositeTransformComponent(true /* useMatrix4x4 */, m_spWinRTExpression.Get(), m_rotateXExpression.Get(), ExpressionHelper::sc_paramName_RotateX);
        }
        else
        {
            pWinRTContext->UpdateCompositeTransformComponent(true /* useMatrix4x4 */, m_spWinRTExpression.Get(), nullptr /* componentExpression */, ExpressionHelper::sc_paramName_RotateX);
            m_rotateXExpression.Reset();
        }


        if (m_rRotationY != 0 || rotateYAnimation != nullptr)
        {
            if (m_rotateYExpression == nullptr)
            {
                // Note we use simple form of the expression, since CompositeTransform components don't have individual center points
                wrl::ComPtr<WUComp::ICompositionPropertySet> propertySet;
                pWinRTContext->CreateExpression(ExpressionHelper::sc_Expression_Rotate3D_Y, &m_rotateYExpression, &propertySet);

                pWinRTContext->InitializeExpression(propertySet.Get(), ExpressionHelper::sc_paramName_RotateAngle, m_rRotationY);
            }

            pWinRTContext->UpdateExpression(m_rotateYExpression.Get(), ExpressionHelper::sc_paramName_RotateAngle, m_rRotationY, m_isRotateYAnimationDirty, rotateYAnimation.get(), this, KnownPropertyIndex::CompositeTransform3D_RotationY, timeManager);
            pWinRTContext->UpdateCompositeTransformComponent(true /* useMatrix4x4 */, m_spWinRTExpression.Get(), m_rotateYExpression.Get(), ExpressionHelper::sc_paramName_RotateY);
        }
        else
        {
            pWinRTContext->UpdateCompositeTransformComponent(true /* useMatrix4x4 */, m_spWinRTExpression.Get(), nullptr /* componentExpression */, ExpressionHelper::sc_paramName_RotateY);
            m_rotateYExpression.Reset();
        }

        if (m_rRotationZ != 0 || rotateZAnimation != nullptr)
        {
            if (m_rotateZExpression == nullptr)
            {
                // Note we use simple form of the expression, since CompositeTransform components don't have individual center points
                wrl::ComPtr<WUComp::ICompositionPropertySet> propertySet;
                pWinRTContext->CreateExpression(ExpressionHelper::sc_Expression_Rotate3D_Z, &m_rotateZExpression, &propertySet);

                pWinRTContext->InitializeExpression(propertySet.Get(), ExpressionHelper::sc_paramName_RotateAngle, m_rRotationZ);
            }

            pWinRTContext->UpdateExpression(m_rotateZExpression.Get(), ExpressionHelper::sc_paramName_RotateAngle, m_rRotationZ, m_isRotateZAnimationDirty, rotateZAnimation.get(), this, KnownPropertyIndex::CompositeTransform3D_RotationZ, timeManager);
            pWinRTContext->UpdateCompositeTransformComponent(true /* useMatrix4x4 */, m_spWinRTExpression.Get(), m_rotateZExpression.Get(), ExpressionHelper::sc_paramName_RotateZ);
        }
        else
        {
            pWinRTContext->UpdateCompositeTransformComponent(true /* useMatrix4x4 */, m_spWinRTExpression.Get(), nullptr /* componentExpression */, ExpressionHelper::sc_paramName_RotateZ);
            m_rotateZExpression.Reset();
        }

        if (m_rTranslateX != 0 || m_rTranslateY != 0 || m_rTranslateZ != 0 || translateXAnimation != nullptr || translateYAnimation != nullptr || translateZAnimation != nullptr)
        {
            if (m_translateExpression == nullptr)
            {
                // Note we use simple form of the expression, since CompositeTransform components don't have individual center points
                wrl::ComPtr<WUComp::ICompositionPropertySet> propertySet;
                pWinRTContext->CreateExpression(ExpressionHelper::sc_Expression_Translate3D, &m_translateExpression, &propertySet);

                pWinRTContext->InitializeExpression(propertySet.Get(), ExpressionHelper::sc_paramName_TranslateX, m_rTranslateX);
                pWinRTContext->InitializeExpression(propertySet.Get(), ExpressionHelper::sc_paramName_TranslateY, m_rTranslateY);
                pWinRTContext->InitializeExpression(propertySet.Get(), ExpressionHelper::sc_paramName_TranslateZ, m_rTranslateZ);
            }

            pWinRTContext->UpdateExpression(m_translateExpression.Get(), ExpressionHelper::sc_paramName_TranslateX, m_rTranslateX, m_isTranslateXAnimationDirty, translateXAnimation.get(), this, KnownPropertyIndex::CompositeTransform3D_TranslateX, timeManager);
            pWinRTContext->UpdateExpression(m_translateExpression.Get(), ExpressionHelper::sc_paramName_TranslateY, m_rTranslateY, m_isTranslateYAnimationDirty, translateYAnimation.get(), this, KnownPropertyIndex::CompositeTransform3D_TranslateY, timeManager);
            pWinRTContext->UpdateExpression(m_translateExpression.Get(), ExpressionHelper::sc_paramName_TranslateZ, m_rTranslateZ, m_isTranslateZAnimationDirty, translateZAnimation.get(), this, KnownPropertyIndex::CompositeTransform3D_TranslateZ, timeManager);
            pWinRTContext->UpdateCompositeTransformComponent(true /* useMatrix4x4 */, m_spWinRTExpression.Get(), m_translateExpression.Get(), ExpressionHelper::sc_paramName_Translate);
        }
        else
        {
            pWinRTContext->UpdateCompositeTransformComponent(true /* useMatrix4x4 */, m_spWinRTExpression.Get(), nullptr /* componentExpression */, ExpressionHelper::sc_paramName_Translate);
            m_translateExpression.Reset();
        }

        // Update CenterOffset in overall expression
        pWinRTContext->UpdateExpression(m_spWinRTExpression.Get(), ExpressionHelper::sc_paramName_CenterX, m_rCenterX, m_isCenterXAnimationDirty, centerXAnimation.get(), this, KnownPropertyIndex::CompositeTransform3D_CenterX, timeManager);
        pWinRTContext->UpdateExpression(m_spWinRTExpression.Get(), ExpressionHelper::sc_paramName_CenterY, m_rCenterY, m_isCenterYAnimationDirty, centerYAnimation.get(), this, KnownPropertyIndex::CompositeTransform3D_CenterY, timeManager);
        pWinRTContext->UpdateExpression(m_spWinRTExpression.Get(), ExpressionHelper::sc_paramName_CenterZ, m_rCenterZ, m_isCenterZAnimationDirty, centerZAnimation.get(), this, KnownPropertyIndex::CompositeTransform3D_CenterZ, timeManager);

        UpdateTransformMatrix(elementWidth, elementHeight);

        m_isWinRTExpressionDirty = false;
        m_isCenterXAnimationDirty = false;
        m_isCenterYAnimationDirty = false;
        m_isCenterZAnimationDirty = false;
        m_isScaleXAnimationDirty = false;
        m_isScaleYAnimationDirty = false;
        m_isScaleZAnimationDirty = false;
        m_isRotateXAnimationDirty = false;
        m_isRotateYAnimationDirty = false;
        m_isRotateZAnimationDirty = false;
        m_isTranslateXAnimationDirty = false;
        m_isTranslateYAnimationDirty = false;
        m_isTranslateZAnimationDirty = false;
    }
}

void CCompositeTransform3D::ClearWUCExpression()
{
    __super::ClearWUCExpression();

    m_scaleExpression.Reset();
    m_rotateXExpression.Reset();
    m_rotateYExpression.Reset();
    m_rotateZExpression.Reset();
    m_translateExpression.Reset();

    m_isCenterXAnimationDirty = false;
    m_isCenterYAnimationDirty = false;
    m_isCenterZAnimationDirty = false;
    m_isScaleXAnimationDirty = false;
    m_isScaleYAnimationDirty = false;
    m_isScaleZAnimationDirty = false;
    m_isRotateXAnimationDirty = false;
    m_isRotateYAnimationDirty = false;
    m_isRotateZAnimationDirty = false;
    m_isTranslateXAnimationDirty = false;
    m_isTranslateYAnimationDirty = false;
    m_isTranslateZAnimationDirty = false;
}
