// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "RotateTransform.h"
#include "WinRTExpressionConversionContext.h"
#include <DependencyObjectDCompRegistry.h>
#include <ExpressionHelper.h>

void CRotateTransform::ReleaseDCompResources()
{
    __super::ReleaseDCompResources();

    SetDCompAnimation(nullptr, KnownPropertyIndex::RotateTransform_CenterXAnimation);
    SetDCompAnimation(nullptr, KnownPropertyIndex::RotateTransform_CenterYAnimation);
    SetDCompAnimation(nullptr, KnownPropertyIndex::RotateTransform_AngleAnimation);
}

void CRotateTransform::MakeWinRTExpression(
    _Inout_ WinRTExpressionConversionContext* pWinRTContext,
    _Inout_opt_ WUComp::ICompositionPropertySet* pTransformGroupPS)
{
    // Even a transform that is clean might need to create an expression, if it's part of a transform group where some other
    // transform started animating, so check both the dirty flag and that we already have an expression. The corollary is that
    // calling this method on a clean transform will still create an expression for it.
    if (m_isWinRTExpressionDirty || m_spWinRTExpression == nullptr)
    {
        if (m_spWinRTExpression == nullptr)
        {
            wrl::ComPtr<WUComp::ICompositionPropertySet> propertySet;
            pWinRTContext->CreateExpression(ExpressionHelper::sc_Expression_Rotate_CenterPoint, &m_spWinRTExpression, &propertySet);

            pWinRTContext->InitializeExpression(propertySet.Get(), ExpressionHelper::sc_paramName_CenterX, m_ptCenter.x);
            pWinRTContext->InitializeExpression(propertySet.Get(), ExpressionHelper::sc_paramName_CenterY, m_ptCenter.y);
            pWinRTContext->InitializeExpression(propertySet.Get(), ExpressionHelper::sc_paramName_RotateAngle, m_eAngle);

            if (GetDCompObjectRegistry() != nullptr)
            {
                GetDCompObjectRegistry()->EnsureObjectWithDCompResourceRegistered(this);
            }
        }

        const auto& centerXAnimation = GetWUCDCompAnimation(KnownPropertyIndex::RotateTransform_CenterXAnimation);
        const auto& centerYAnimation = GetWUCDCompAnimation(KnownPropertyIndex::RotateTransform_CenterYAnimation);
        const auto& angleAnimation = GetWUCDCompAnimation(KnownPropertyIndex::RotateTransform_AngleAnimation);
        const auto& timeManager = GetTimeManager();

        // TODO: WinComp: Verify that we specified the correct axis for rotation for DComp [(0, 0, 1) vs (0, 0, -1)]. We can do this once we hook the expression up to a visual and can visually validate correct rotation.
        // TODO: WinComp: For rotation about origin we should use simple expression with just the Rotation Matrix
        // TODO: WinComp: PERF: For non-animated transforms, consider using multiplied out matrix form of rotation about point

        pWinRTContext->UpdateExpression(m_spWinRTExpression.Get(), ExpressionHelper::sc_paramName_CenterX, m_ptCenter.x, m_isCenterXAnimationDirty, centerXAnimation.get(), this, KnownPropertyIndex::RotateTransform_CenterX, timeManager);
        pWinRTContext->UpdateExpression(m_spWinRTExpression.Get(), ExpressionHelper::sc_paramName_CenterY, m_ptCenter.y, m_isCenterYAnimationDirty, centerYAnimation.get(), this, KnownPropertyIndex::RotateTransform_CenterY, timeManager);
        pWinRTContext->UpdateExpression(m_spWinRTExpression.Get(), ExpressionHelper::sc_paramName_RotateAngle, m_eAngle, m_isAngleAnimationDirty, angleAnimation.get(), this, KnownPropertyIndex::RotateTransform_Angle, timeManager);

        m_isWinRTExpressionDirty = false;
        m_isCenterXAnimationDirty = false;
        m_isCenterYAnimationDirty = false;
        m_isAngleAnimationDirty = false;
    }

    if (pTransformGroupPS)
    {
        pWinRTContext->AddExpressionToTransformGroupPropertySet(m_spWinRTExpression.Get(), pTransformGroupPS);
    }
}

void CRotateTransform::ClearWUCExpression()
{
    __super::ClearWUCExpression();

    m_isCenterXAnimationDirty = false;
    m_isCenterYAnimationDirty = false;
    m_isAngleAnimationDirty = false;
}
