// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "SkewTransform.h"
#include "WinRTExpressionConversionContext.h"
#include <DependencyObjectDCompRegistry.h>
#include <ExpressionHelper.h>

void CSkewTransform::ReleaseDCompResources()
{
    __super::ReleaseDCompResources();

    SetDCompAnimation(nullptr, KnownPropertyIndex::SkewTransform_CenterXAnimation);
    SetDCompAnimation(nullptr, KnownPropertyIndex::SkewTransform_CenterYAnimation);
    SetDCompAnimation(nullptr, KnownPropertyIndex::SkewTransform_AngleXAnimation);
    SetDCompAnimation(nullptr, KnownPropertyIndex::SkewTransform_AngleYAnimation);
}

void CSkewTransform::MakeWinRTExpression(
    _Inout_ WinRTExpressionConversionContext* pWinRTContext,
    _Inout_opt_ WUComp::ICompositionPropertySet* pTransformGroupPS
    )
{
    // Even a transform that is clean might need to create an expression, if it's part of a transform group where some other
    // transform started animating, so check both the dirty flag and that we already have an expression. The corollary is that
    // calling this method on a clean transform will still create an expression for it.
    if (m_isWinRTExpressionDirty || m_spWinRTExpression == nullptr)
    {
        if (m_spWinRTExpression == nullptr)
        {
            wrl::ComPtr<WUComp::ICompositionPropertySet> propertySet;
            pWinRTContext->CreateExpression(ExpressionHelper::sc_Expression_Skew_CenterPoint, &m_spWinRTExpression, &propertySet);

            pWinRTContext->InitializeExpression(propertySet.Get(), ExpressionHelper::sc_paramName_CenterX, m_ptCenter.x);
            pWinRTContext->InitializeExpression(propertySet.Get(), ExpressionHelper::sc_paramName_CenterY, m_ptCenter.y);
            pWinRTContext->InitializeExpression(propertySet.Get(), ExpressionHelper::sc_paramName_SkewAngleX, m_eAngleX);
            pWinRTContext->InitializeExpression(propertySet.Get(), ExpressionHelper::sc_paramName_SkewAngleY, m_eAngleY);

            if (GetDCompObjectRegistry() != nullptr)
            {
                GetDCompObjectRegistry()->EnsureObjectWithDCompResourceRegistered(this);
            }
        }

        const auto& centerXAnimation = GetWUCDCompAnimation(KnownPropertyIndex::SkewTransform_CenterXAnimation);
        const auto& centerYAnimation = GetWUCDCompAnimation(KnownPropertyIndex::SkewTransform_CenterYAnimation);
        const auto& angleXAnimation = GetWUCDCompAnimation(KnownPropertyIndex::SkewTransform_AngleXAnimation);
        const auto& angleYAnimation = GetWUCDCompAnimation(KnownPropertyIndex::SkewTransform_AngleYAnimation);
        const auto& timeManager = GetTimeManager();

        // TODO: WinComp: PERF: For non-animated transforms, consider using multiplied out matrix form of skew about point
        // TODO: WinComp: For skew about origin we should use simple expression with just the skew components
        pWinRTContext->UpdateExpression(m_spWinRTExpression.Get(), ExpressionHelper::sc_paramName_CenterX, m_ptCenter.x, m_isCenterXAnimationDirty, centerXAnimation.get(), this, KnownPropertyIndex::SkewTransform_CenterX, timeManager);
        pWinRTContext->UpdateExpression(m_spWinRTExpression.Get(), ExpressionHelper::sc_paramName_CenterY, m_ptCenter.y, m_isCenterYAnimationDirty, centerYAnimation.get(), this, KnownPropertyIndex::SkewTransform_CenterY, timeManager);
        pWinRTContext->UpdateExpression(m_spWinRTExpression.Get(), ExpressionHelper::sc_paramName_SkewAngleX, m_eAngleX, m_isAngleXAnimationDirty, angleXAnimation.get(), this, KnownPropertyIndex::SkewTransform_AngleX, timeManager);
        pWinRTContext->UpdateExpression(m_spWinRTExpression.Get(), ExpressionHelper::sc_paramName_SkewAngleY, m_eAngleY, m_isAngleYAnimationDirty, angleYAnimation.get(), this, KnownPropertyIndex::SkewTransform_AngleY, timeManager);

        m_isWinRTExpressionDirty = false;
        m_isCenterXAnimationDirty = false;
        m_isCenterYAnimationDirty = false;
        m_isAngleXAnimationDirty = false;
        m_isAngleYAnimationDirty = false;
    }

    if (pTransformGroupPS)
    {
        pWinRTContext->AddExpressionToTransformGroupPropertySet(m_spWinRTExpression.Get(), pTransformGroupPS);
    }
}

void CSkewTransform::ClearWUCExpression()
{
    __super::ClearWUCExpression();

    m_isCenterXAnimationDirty = false;
    m_isCenterYAnimationDirty = false;
    m_isAngleXAnimationDirty = false;
    m_isAngleYAnimationDirty = false;
}
