// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "TranslateTransform.h"
#include "WinRTExpressionConversionContext.h"
#include <DependencyObjectDCompRegistry.h>
#include <ExpressionHelper.h>

void CTranslateTransform::ReleaseDCompResources()
{
    __super::ReleaseDCompResources();

    SetDCompAnimation(nullptr, KnownPropertyIndex::TranslateTransform_XAnimation);
    SetDCompAnimation(nullptr, KnownPropertyIndex::TranslateTransform_YAnimation);
}

void CTranslateTransform::MakeWinRTExpression(
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
            pWinRTContext->CreateExpression(ExpressionHelper::sc_Expression_Translate, &m_spWinRTExpression, &propertySet);

            pWinRTContext->InitializeExpression(propertySet.Get(), ExpressionHelper::sc_paramName_TranslateX, m_eX);
            pWinRTContext->InitializeExpression(propertySet.Get(), ExpressionHelper::sc_paramName_TranslateY, m_eY);

            if (GetDCompObjectRegistry() != nullptr)
            {
                GetDCompObjectRegistry()->EnsureObjectWithDCompResourceRegistered(this);
            }
        }

        const auto& xAnimation = GetWUCDCompAnimation(KnownPropertyIndex::TranslateTransform_XAnimation);
        const auto& yAnimation = GetWUCDCompAnimation(KnownPropertyIndex::TranslateTransform_YAnimation);
        const auto& timeManager = GetTimeManager();

        pWinRTContext->UpdateExpression(m_spWinRTExpression.Get(), ExpressionHelper::sc_paramName_TranslateX, m_eX, m_isXAnimationDirty, xAnimation.get(), this, KnownPropertyIndex::TranslateTransform_X, timeManager);
        pWinRTContext->UpdateExpression(m_spWinRTExpression.Get(), ExpressionHelper::sc_paramName_TranslateY, m_eY, m_isYAnimationDirty, yAnimation.get(), this, KnownPropertyIndex::TranslateTransform_Y, timeManager);

        m_isWinRTExpressionDirty = false;
        m_isXAnimationDirty = false;
        m_isYAnimationDirty = false;
    }

    if (pTransformGroupPS)
    {
        pWinRTContext->AddExpressionToTransformGroupPropertySet(m_spWinRTExpression.Get(), pTransformGroupPS);
    }
}

void CTranslateTransform::ClearWUCExpression()
{
    __super::ClearWUCExpression();

    m_isXAnimationDirty = false;
    m_isYAnimationDirty = false;
}
