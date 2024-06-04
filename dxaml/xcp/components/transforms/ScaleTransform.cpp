// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ScaleTransform.h"
#include "WinRTExpressionConversionContext.h"
#include <DependencyObjectDCompRegistry.h>
#include <ExpressionHelper.h>

void CScaleTransform::ReleaseDCompResources()
{
    __super::ReleaseDCompResources();

    SetDCompAnimation(nullptr, KnownPropertyIndex::ScaleTransform_CenterXAnimation);
    SetDCompAnimation(nullptr, KnownPropertyIndex::ScaleTransform_CenterYAnimation);
    SetDCompAnimation(nullptr, KnownPropertyIndex::ScaleTransform_ScaleXAnimation);
    SetDCompAnimation(nullptr, KnownPropertyIndex::ScaleTransform_ScaleYAnimation);
}

void CScaleTransform::MakeWinRTExpression(
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
            pWinRTContext->CreateExpression(ExpressionHelper::sc_Expression_Scale_CenterPoint, &m_spWinRTExpression, &propertySet);

            pWinRTContext->InitializeExpression(propertySet.Get(), ExpressionHelper::sc_paramName_CenterX, m_ptCenter.x);
            pWinRTContext->InitializeExpression(propertySet.Get(), ExpressionHelper::sc_paramName_CenterY, m_ptCenter.y);
            pWinRTContext->InitializeExpression(propertySet.Get(), ExpressionHelper::sc_paramName_ScaleX, m_eScaleX);
            pWinRTContext->InitializeExpression(propertySet.Get(), ExpressionHelper::sc_paramName_ScaleY, m_eScaleY);

            if (GetDCompObjectRegistry() != nullptr)
            {
                GetDCompObjectRegistry()->EnsureObjectWithDCompResourceRegistered(this);
            }
        }

        const auto& centerXAnimation = GetWUCDCompAnimation(KnownPropertyIndex::ScaleTransform_CenterXAnimation);
        const auto& centerYAnimation = GetWUCDCompAnimation(KnownPropertyIndex::ScaleTransform_CenterYAnimation);
        const auto& scaleXAnimation = GetWUCDCompAnimation(KnownPropertyIndex::ScaleTransform_ScaleXAnimation);
        const auto& scaleYAnimation = GetWUCDCompAnimation(KnownPropertyIndex::ScaleTransform_ScaleYAnimation);
        const auto& timeManager = GetTimeManager();

        // TODO: WinComp: For scale about origin we should use simple expression with just the Scale Matrix
        // TODO: WinComp: PERF: For non-animated transforms, consider using multiplied out matrix form of scale about point

        pWinRTContext->UpdateExpression(m_spWinRTExpression.Get(), ExpressionHelper::sc_paramName_CenterX, m_ptCenter.x, m_isCenterXAnimationDirty, centerXAnimation.get(), this, KnownPropertyIndex::ScaleTransform_CenterX, timeManager);
        pWinRTContext->UpdateExpression(m_spWinRTExpression.Get(), ExpressionHelper::sc_paramName_CenterY, m_ptCenter.y, m_isCenterYAnimationDirty, centerYAnimation.get(), this, KnownPropertyIndex::ScaleTransform_CenterY, timeManager);
        pWinRTContext->UpdateExpression(m_spWinRTExpression.Get(), ExpressionHelper::sc_paramName_ScaleX, m_eScaleX, m_isScaleXAnimationDirty, scaleXAnimation.get(), this, KnownPropertyIndex::ScaleTransform_ScaleX, timeManager);
        pWinRTContext->UpdateExpression(m_spWinRTExpression.Get(), ExpressionHelper::sc_paramName_ScaleY, m_eScaleY, m_isScaleYAnimationDirty, scaleYAnimation.get(), this, KnownPropertyIndex::ScaleTransform_ScaleY, timeManager);

        m_isWinRTExpressionDirty = false;
        m_isCenterXAnimationDirty = false;
        m_isCenterYAnimationDirty = false;
        m_isScaleXAnimationDirty = false;
        m_isScaleYAnimationDirty = false;
    }

    if (pTransformGroupPS)
    {
        pWinRTContext->AddExpressionToTransformGroupPropertySet(m_spWinRTExpression.Get(), pTransformGroupPS);
    }
}

void CScaleTransform::ClearWUCExpression()
{
    __super::ClearWUCExpression();

    m_isCenterXAnimationDirty = false;
    m_isCenterYAnimationDirty = false;
    m_isScaleXAnimationDirty = false;
    m_isScaleYAnimationDirty = false;
}
