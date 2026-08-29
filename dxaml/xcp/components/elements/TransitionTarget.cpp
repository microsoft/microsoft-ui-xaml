// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "TransitionTarget.h"
#include <UIElement.h>
#include <WinRTExpressionConversionContext.h>
#include <ExpressionHelper.h>
#include <DependencyObjectDCompRegistry.h>

bool CTransitionTarget::NeedsWUCOpacityExpression()
{
    // The independent animation flag is set on the UIElement that has this CTransitionTarget.
    return GetWUCDCompAnimation(KnownPropertyIndex::TransitionTarget_OpacityAnimation) != nullptr;
}

void CTransitionTarget::EnsureWUCOpacityExpression(_Inout_ WinRTExpressionConversionContext* context)
{
    if (m_isOpacityAnimationDirty)
    {
        if (!m_opacityExpression)
        {
            wrl::ComPtr<WUComp::ICompositionPropertySet> propertySet;
            context->CreateExpression(ExpressionHelper::sc_Expression_TransitionTargetOpacity, m_opacityExpression.ReleaseAndGetAddressOf(), &propertySet);
            context->InitializeExpression(propertySet.Get(), ExpressionHelper::sc_paramName_TransitionTargetOpacity, m_opacity);

            auto dcompRegistry = GetDCompObjectRegistry();
            if (dcompRegistry)
            {
                dcompRegistry->EnsureObjectWithDCompResourceRegistered(this);
            }
        }

        const auto& opacityAnimation = GetWUCDCompAnimation(KnownPropertyIndex::TransitionTarget_OpacityAnimation);
        // Note: Animations with explicit Duration="Forever" will not generate WUC animations, so there's no guarantee that we'll have one.

        const auto& timeManager = GetTimeManager();

        context->UpdateExpression(m_opacityExpression.get(), ExpressionHelper::sc_paramName_TransitionTargetOpacity, m_opacity, m_isOpacityAnimationDirty, opacityAnimation, this, KnownPropertyIndex::TransitionTarget_Opacity, timeManager);

        m_isOpacityAnimationDirty = false;
    }
}

void CTransitionTarget::EnsureWUCOpacityExpression(
    _Inout_ WinRTExpressionConversionContext* context,
    float prependOpacity)
{
    EnsureWUCOpacityExpression(context);

    const auto& timeManager = GetTimeManager();

    context->UpdateExpression(m_opacityExpression.get(), ExpressionHelper::sc_paramName_PrependOpacity, prependOpacity, false /* animation dirty */, nullptr /* animation */, this, KnownPropertyIndex::TransitionTarget_Opacity, timeManager);
}

void CTransitionTarget::ClearWUCOpacityExpression()
{
    m_opacityExpression.reset();
    m_isOpacityAnimationDirty = false;
}

xref_ptr<WUComp::IExpressionAnimation> CTransitionTarget::GetWUCOpacityExpression() const
{
    return m_opacityExpression;
}

void CTransitionTarget::CleanupDeviceRelatedResourcesRecursive(bool cleanupDComp)
{
    if (cleanupDComp)
    {
        m_opacityExpression.reset();
    }
}

void CTransitionTarget::ReleaseDCompResources()
{
    __super::ReleaseDCompResources();

    m_opacityExpression.reset();
    SetDCompAnimation(nullptr, KnownPropertyIndex::TransitionTarget_OpacityAnimation);
}

void CTransitionTarget::EnsureWUCAnimationStarted(_Inout_ WinRTExpressionConversionContext* context)
{
    if (NeedsWUCOpacityExpression())
    {
        EnsureWUCOpacityExpression(context);
    }
    else
    {
        ClearWUCOpacityExpression();
    }
}
