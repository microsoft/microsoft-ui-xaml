// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ExpressionHelper.h"
#include "WinRTLocalExpressionBuilder.h"
#include "Transform.h"
#include <DCompTreeHost.h>

using namespace Microsoft::WRL::Wrappers;
using namespace Microsoft::WRL;

static const wfn::Matrix3x2 c_identityMatrix3x2 = { 1, 0, 0, 1, 0, 0 };
static const wfn::Matrix4x4 c_identityMatrix4x4 = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };

WinRTLocalExpressionBuilder::WinRTLocalExpressionBuilder(
    _In_ WUComp::ICompositor* compositor,
    _In_ WUComp::IVisual* visual,
    _In_ WinRTLocalExpressionCache* cache
    )
    : m_winrtContext(compositor),
      m_spCompositor(compositor),
      m_visual(visual),
      m_cache(cache),
      m_transformFlags(0)
{
    EnsurePropertySet();
}

WinRTLocalExpressionBuilder::~WinRTLocalExpressionBuilder()
{
}

void  WinRTLocalExpressionBuilder::EnsurePropertySet()
{
    if (!m_cache->m_localExpressionComponentsPS)
    {
        IFCFAILFAST(m_spCompositor->CreatePropertySet(&m_cache->m_localExpressionComponentsPS));
    }
}

void WinRTLocalExpressionBuilder::ApplyProjectionExpression(_In_ WUComp::IExpressionAnimation* projectionExpression)
{
    IFCFAILFAST(m_cache->m_localExpressionComponentsPS->InsertMatrix4x4(HStringReference(ExpressionHelper::sc_propertyName_Projection).Get(), c_identityMatrix4x4));

    Microsoft::WRL::ComPtr<WUComp::ICompositionAnimation> projectionCA;
    Microsoft::WRL::ComPtr<WUComp::ICompositionObject> psCO;
    VERIFYHR(projectionExpression->QueryInterface(IID_PPV_ARGS(&projectionCA)));
    VERIFYHR(m_cache->m_localExpressionComponentsPS.As(&psCO));
    IFCFAILFAST(psCO->StartAnimation(wrl_wrappers::HStringReference(ExpressionHelper::sc_propertyName_Projection).Get(), projectionCA.Get()));

    m_transformFlags |= TransformFlag_Projection;
}

void WinRTLocalExpressionBuilder::ApplyTransform3DExpression(_In_ WUComp::IExpressionAnimation* transform3DExpression)
{
    IFCFAILFAST(m_cache->m_localExpressionComponentsPS->InsertMatrix4x4(HStringReference(ExpressionHelper::sc_propertyName_Transform3D).Get(), c_identityMatrix4x4));

    Microsoft::WRL::ComPtr<WUComp::ICompositionAnimation> transform3D_CA;
    Microsoft::WRL::ComPtr<WUComp::ICompositionObject> psCO;
    VERIFYHR(transform3DExpression->QueryInterface(IID_PPV_ARGS(&transform3D_CA)));
    VERIFYHR(m_cache->m_localExpressionComponentsPS.As(&psCO));
    IFCFAILFAST(psCO->StartAnimation(wrl_wrappers::HStringReference(ExpressionHelper::sc_propertyName_Transform3D).Get(), transform3D_CA.Get()));

    m_transformFlags |= TransformFlag_Transform3D;
}

void WinRTLocalExpressionBuilder::ApplyRenderTransform(
    _In_ CTransform* pTransform,
    float originX,
    float originY
    )
{
    SetBuilderState(LocalTransformBuilderState::HasRenderTransform);

    IFCFAILFAST(m_cache->m_localExpressionComponentsPS->InsertMatrix3x2(HStringReference(ExpressionHelper::sc_propertyName_RenderTransform).Get(), c_identityMatrix3x2));

    // ApplyTransform gets called to generate RenderTransform and TransitionTargetRenderTransform. These expressions
    // target Matrix3x2 since it's possible for the app to use the same transform as a RenderTransform and a Clip.Transform,
    // and the latter is always a Matrix3x2. We'll convert these to 4x4 matrices here in the final local tranform expression.
    const auto& expression = ApplyTransform(pTransform, originX, originY, ExpressionHelper::sc_propertyName_RenderTransform);

    // If there's a RenderTransformOrigin involved, then we wrapped the Xaml transform's expression in an outer expression.
    // Keep the outer expression alive in the cache.
    if (originX != 0 || originY != 0)
    {
        m_cache->m_rtoExpression = expression;
    }
    else
    {
        m_cache->m_rtoExpression.Reset();
    }

    m_transformFlags |= TransformFlag_Render;
}

void WinRTLocalExpressionBuilder::ApplyHandOffVisualTransform(
    _In_ CTransform* pTransform,
    wfn::Vector3 translationFacade
    )
{
    // The HandOff Visual Transform is only needed for XamlLocalTransformBuilder.
    // For this usage, the HandOff visual is already in the DComp visual tree and carries this transform
    ASSERT(FALSE, "Unexpected call to WinRTLocalExpressionBuilder::ApplyHandOffVisualTransform");
}

void WinRTLocalExpressionBuilder::ApplyTransitionTargetRenderTransform(
    _In_ CTransform* pTransform,
    float originX,
    float originY
    )
{
    SetBuilderState(LocalTransformBuilderState::HasTTRenderTransform);

    IFCFAILFAST(m_cache->m_localExpressionComponentsPS->InsertMatrix3x2(HStringReference(ExpressionHelper::sc_propertyName_TransitionTargetRenderTransform).Get(), c_identityMatrix3x2));

    const auto& expression = ApplyTransform(pTransform, originX, originY, ExpressionHelper::sc_propertyName_TransitionTargetRenderTransform);

    // If there's a RenderTransformOrigin involved, then we wrapped the Xaml transform's expression in an outer expression.
    // Keep the outer expression alive in the cache.
    if (originX != 0 || originY != 0)
    {
        m_cache->m_transitionTargetRTOExpression = expression;
    }
    else
    {
        m_cache->m_transitionTargetRTOExpression.Reset();
    }

    m_transformFlags |= TransformFlag_TTRender;
}

wrl::ComPtr<WUComp::IExpressionAnimation> WinRTLocalExpressionBuilder::ApplyTransform(
    _In_ CTransform* pTransform,
    float originX,
    float originY,
    _In_ const wchar_t* pTargetProperty
    )
{
    Microsoft::WRL::ComPtr<WUComp::IExpressionAnimation> expression;
    Microsoft::WRL::ComPtr<WUComp::ICompositionAnimation> spExpressionAsCompositionAnimation;
    Microsoft::WRL::ComPtr<WUComp::ICompositionObject> spPropertySetAsCO;

    pTransform->MakeWinRTExpressionWithOrigin(
        originX,
        originY,
        &m_winrtContext,
        expression.ReleaseAndGetAddressOf()
        );

    IFCFAILFAST(expression.As(&spExpressionAsCompositionAnimation));
    IFCFAILFAST(m_cache->m_localExpressionComponentsPS.As(&spPropertySetAsCO));
    IFCFAILFAST(spPropertySetAsCO->StartAnimation(HStringReference(pTargetProperty).Get(), spExpressionAsCompositionAnimation.Get()));

    return expression;
}

void WinRTLocalExpressionBuilder::ApplyFlowDirection(
    bool flipRTL,
    bool flipRTLInPlace,
    float unscaledElementWidth
    )
{
    SetBuilderState(LocalTransformBuilderState::HasFlowDirection);

    if (flipRTL)
    {
        IFCFAILFAST(m_cache->m_localExpressionComponentsPS->InsertMatrix4x4(HStringReference(ExpressionHelper::sc_propertyName_FlowDirectionTransform).Get(), c_identityMatrix4x4));

        Microsoft::WRL::ComPtr<WUComp::IExpressionAnimation> spWinRTExpression;
        Microsoft::WRL::ComPtr<WUComp::ICompositionAnimation> spExpressionAsCompositionAnimation;
        Microsoft::WRL::ComPtr<WUComp::ICompositionObject> spPropertySetAsCO;

        CMILMatrix matFlip(TRUE);

        matFlip.SetM11(-1.0f);
        if (flipRTLInPlace)
        {
            // Since the element was flipped over the y-axis it needs to be translated back by its
            // width to stay in the same position.
            matFlip.SetDx(unscaledElementWidth);
        }

        m_winrtContext.CreateMatrixTransform(true /* useMatrix4x4 */, &matFlip, &spWinRTExpression);

        IFCFAILFAST(spWinRTExpression.As(&spExpressionAsCompositionAnimation));
        IFCFAILFAST(m_cache->m_localExpressionComponentsPS.As(&spPropertySetAsCO));
        IFCFAILFAST(spPropertySetAsCO->StartAnimation(HStringReference(ExpressionHelper::sc_propertyName_FlowDirectionTransform).Get(), spExpressionAsCompositionAnimation.Get()));

        // Keep the RTL expression alive in the cache.
        m_cache->m_rtlExpression = spWinRTExpression;

        m_transformFlags |= TransformFlag_FlowDirection;
    }
}

void WinRTLocalExpressionBuilder::ApplyOffsetAndDM(
    float offsetX,
    float offsetY,
    float dmOffsetX,
    float dmOffsetY,
    float dmZoomX,
    float dmZoomY,
    bool applyDMZoomToOffset
    )
{
    // Gracefully no-op in this implementation - CUIElement doesn't have enough context to know when to avoid calling this.
}

void WinRTLocalExpressionBuilder::ApplyDManipSharedTransform(_In_ IUnknown* dmanipSharedTransform)
{
    SetBuilderState(LocalTransformBuilderState::HasDManipSharedTransform);

    Microsoft::WRL::ComPtr<WUComp::ICompositionPropertySet> dmanipTransformAsPS;
    VERIFYHR(dmanipSharedTransform->QueryInterface(IID_PPV_ARGS(&dmanipTransformAsPS)));
    if (m_cache->m_dmanipTransformPS != dmanipTransformAsPS)
    {
        // Notes:  Here, the object we've been handed, DManipSharedTransform, is a PropertySet that has a Matrix4x4 inside it called "Matrix".
        // This matrix carries the final manipulation transform matrix produced by DManipDataWinRT.  See comments in DManipData.cpp for more details
        // on how this PropertySet is put together.
        // Given this PropertySet, we create an additional property in the LOCAL PropertySet to hold this value and combine this with all the
        // other values in LOCAL to produce the final LocalTransform.
        IFCFAILFAST(m_cache->m_localExpressionComponentsPS->InsertMatrix4x4(HStringReference(ExpressionHelper::sc_propertyName_LocalDManipTransform).Get(), c_identityMatrix4x4));

        Microsoft::WRL::ComPtr<WUComp::IExpressionAnimation> expressionAnimation;
        IFCFAILFAST(m_winrtContext.GetCompositorNoRef()->CreateExpressionAnimationWithExpression(HStringReference(ExpressionHelper::sc_Expression_DMTransform).Get(), &expressionAnimation));

        Microsoft::WRL::ComPtr<WUComp::ICompositionObject> dmanipTransformAsCO;
        VERIFYHR(dmanipSharedTransform->QueryInterface(IID_PPV_ARGS(&dmanipTransformAsCO)));

        Microsoft::WRL::ComPtr<WUComp::ICompositionAnimation> expressionAnimationAsCA;
        VERIFYHR(expressionAnimation.As(&expressionAnimationAsCA));
        expressionAnimationAsCA->SetReferenceParameter(HStringReference(ExpressionHelper::sc_paramName_PropertySet).Get(), dmanipTransformAsCO.Get());

        Microsoft::WRL::ComPtr<WUComp::ICompositionObject> propertySetAsCO;
        IFCFAILFAST(m_cache->m_localExpressionComponentsPS.As(&propertySetAsCO));
        IFCFAILFAST(propertySetAsCO->StartAnimation(HStringReference(ExpressionHelper::sc_propertyName_LocalDManipTransform).Get(), expressionAnimationAsCA.Get()));

        m_cache->m_dmanipTransformPS = dmanipTransformAsPS;
    }

    m_transformFlags |= TransformFlag_DManip;
}

void WinRTLocalExpressionBuilder::ApplyRedirectionTransform(_In_ RedirectionTransformInfo* redirInfo)
{
    SetBuilderState(LocalTransformBuilderState::HasRedirectionTransform);

    IFCFAILFAST(m_cache->m_localExpressionComponentsPS->InsertMatrix4x4(HStringReference(ExpressionHelper::sc_propertyName_PostRedirectionTransform).Get(), c_identityMatrix4x4));

    // First put the redirection transform into our local property set
    ComPtr<WUComp::IExpressionAnimation> winRTExpression;
    ComPtr<WUComp::ICompositionAnimation> expressionCA;
    ComPtr<WUComp::ICompositionPropertySet> expressionPS;
    ComPtr<WUComp::ICompositionObject> propertySetCO;

    // Create an expression that properly accounts for the Visual's Offset...
    // This expression effectively pushes the redirection transform above the Offset by undoing, applying the redirection transform, then redoing the Offset
    // A picture is helpful here.  Drawing the transforms from top to bottom, what we'd like is to apply this chain of transforms to the Visual:
    //
    // Redirection
    // Offset (set as the Offset property)
    // TransformMatrix
    //
    // The problem is in applying these in this order.  There's no transform component we can use to achieve this as Offset is always applied first.
    // So what we do is create this chain of transforms, which is equivalent:
    //
    // Offset (set as the Offset property)
    // -Offset
    // Redirection
    // Offset
    // TransformMatrix
    //
    // That's what this expression does - it combines the top 3 together and puts it into a single property,
    // the rest of the transform builder combines this with everything else and finally targets the TransformMatrix.

    m_winrtContext.CreateExpression(ExpressionHelper::sc_Expression_PostRedirectionTransform, &winRTExpression, &expressionPS);
    IFCFAILFAST(winRTExpression.As(&expressionCA));

    // Add the redirection transform
    // TODO_WinRT:  For now we're converting to 4x4 as everything in LOCAL is 4x4.  Change this to 3x2 after we change LOCAL to use 3x2 when possible.
    wfn::Matrix4x4 transform4x4;
    redirInfo->redirectionTransform->ToMatrix4x4(&transform4x4);

    IFCFAILFAST(expressionPS->InsertMatrix4x4(HStringReference(ExpressionHelper::sc_propertyName_RedirectionTransform).Get(), transform4x4));

    // Add the visual as a reference parameter, which we'll use as input.
    // TODO_WinRT:  We can optimize this to a single matrix, when the offset is not being animated, by multiplying the 3 pieces together ourselves.
    ComPtr<WUComp::ICompositionObject> visualCO;
    VERIFYHR(redirInfo->visual->QueryInterface(IID_PPV_ARGS(&visualCO)));
    IFCFAILFAST(expressionCA->SetReferenceParameter(HStringReference(ExpressionHelper::sc_paramName_Visual).Get(), visualCO.Get()));

    IFCFAILFAST(m_cache->m_localExpressionComponentsPS.As(&propertySetCO));
    IFCFAILFAST(propertySetCO->StartAnimation(HStringReference(ExpressionHelper::sc_propertyName_PostRedirectionTransform).Get(), expressionCA.Get()));

    // Keep the redirection expression alive in the cache.
    m_cache->m_redirectionExpression = winRTExpression;

    m_transformFlags |= TransformFlag_Redir;
}

// Helper function that creates the desired local expression string
std::wstring WinRTLocalExpressionBuilder::GetExpressionString()
{
    ASSERT(m_transformFlags != 0);
    std::wstring result;

    // Buid a little array that maps from flag => sub-expression string
    struct ExpressionEntry
    {
        int flag;
        const wchar_t* str;
    };
    const ExpressionEntry entries[] =
    {
        {TransformFlag_Projection,      ExpressionHelper::sc_Expression_LocalTransform_Projection},
        {TransformFlag_Transform3D,     ExpressionHelper::sc_Expression_LocalTransform_Transform3D},
        {TransformFlag_Render,          ExpressionHelper::sc_Expression_LocalTransform_Render},
        {TransformFlag_TTRender,        ExpressionHelper::sc_Expression_LocalTransform_TTRender},
        {TransformFlag_FlowDirection,   ExpressionHelper::sc_Expression_LocalTransform_FlowDirection},
        {TransformFlag_DManip,          ExpressionHelper::sc_Expression_LocalTransform_DManip},
        {TransformFlag_Redir,           ExpressionHelper::sc_Expression_LocalTransform_Redir},
    };

    result.reserve(256);

    // Iterate through the flags, for each flag that is currently set, add the appropriate string to the expression
    for (UINT i = 0; i < ARRAYSIZE(entries); i++)
    {
        if ((m_transformFlags & entries[i].flag) != 0)
        {
            if (!result.empty())
            {
                result += L" * ";
            }
            result += entries[i].str;
        }
    }
    
    return result;
 }

// Helper function that releases sub-expressions and property-sets held in cache that are no longer in use
void WinRTLocalExpressionBuilder::ReleaseUnnecessarySubParts()
{
    // Build a little array that maps from flag => sub-expression ComPtr reference
    struct CachedExpressionEntry
    {
        int flag;
        wrl::ComPtr<WUComp::IExpressionAnimation>& expression;
    };
    const CachedExpressionEntry entries[] =
    {
        {TransformFlag_Render,          m_cache->m_rtoExpression},
        {TransformFlag_TTRender,        m_cache->m_transitionTargetRTOExpression},
        {TransformFlag_Redir,           m_cache->m_redirectionExpression},
        {TransformFlag_FlowDirection,   m_cache->m_rtlExpression},
    };

    // Iterate through the flags, for each flag that is current un-set, release the appropriate sub-expression
    for (UINT i = 0; i < ARRAYSIZE(entries); i++)
    {
        if ((m_cache->m_transformFlags & entries[i].flag) == 0)
        {
            entries[i].expression.Reset();
        }
    }

    if ((m_cache->m_transformFlags & TransformFlag_DManip) == 0)
    {
         m_cache->m_dmanipTransformPS.Reset();
    }
}

void WinRTLocalExpressionBuilder::EnsureLocalExpression()
{
    // Create, or re-create the local expression if the general form of the expression has changed.
    if (!m_cache->m_localExpression || m_cache->m_transformFlags != m_transformFlags)
    {
        // TODO: WinRT: This expression template can be cached. We can swap out the property set and create instances.
        // Note that if we do this, the cache will need to keep track of each general form of expression since that can vary.

        std::wstring strExpression = GetExpressionString();
        IFCFAILFAST(m_winrtContext.GetCompositorNoRef()->CreateExpressionAnimationWithExpression(HStringReference(strExpression.c_str()).Get(), m_cache->m_localExpression.ReleaseAndGetAddressOf()));

        ComPtr<WUComp::ICompositionAnimation> spExpressionAsCompositionAnimation;
        IFCFAILFAST(m_cache->m_localExpression.As(&spExpressionAsCompositionAnimation));

        ComPtr<WUComp::ICompositionObject> spPropertySetAsCO;
        IFCFAILFAST(m_cache->m_localExpressionComponentsPS.As(&spPropertySetAsCO));

        IFCFAILFAST(spExpressionAsCompositionAnimation->SetReferenceParameter(
            wrl_wrappers::HStringReference(ExpressionHelper::sc_LocalTransformPropertySetName).Get(),
            spPropertySetAsCO.Get()));

        wrl::ComPtr<WUComp::ICompositionObject> visualCO;
        VERIFYHR(m_visual.As(&visualCO));

        // If the DManip component of the transform is present, we need to make sure the visual is set as a reference parameter,
        // as this part of the expression uses Visual.Offset as an input.
        if ((m_transformFlags & TransformFlag_DManip) != 0)
        {
            IFCFAILFAST(spExpressionAsCompositionAnimation->SetReferenceParameter(HStringReference(ExpressionHelper::sc_paramName_Visual).Get(), visualCO.Get()));
        }

        wrl::ComPtr<WUComp::ICompositionAnimation> animation;
        VERIFYHR(m_cache->m_localExpression.As(&animation));

        IFCFAILFAST(visualCO->StartAnimation(wrl_wrappers::HStringReference(ExpressionHelper::sc_propertyName_TransformMatrix).Get(), animation.Get()));

        m_cache->m_transformFlags = m_transformFlags;

        // Now that we have created a possibly new expression, release any previously used sub-expressions and property-sets that are no longer in use from the cache.
        ReleaseUnnecessarySubParts();
    }
}
