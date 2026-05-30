// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "LocalTransformBuilder.h"
#include "WinRTExpressionConversionContext.h"
#include <WinRTLocalExpressionCache.h>
#include <fwd/Windows.UI.Composition.h>

// Manages the assembling of the uber expression applied to a WUC visual's TransformMatrix.
// The expression and property group themselves are created by the comp node, outside of the builder.
class WinRTLocalExpressionBuilder : public LocalTransformBuilder
{
public:
    explicit WinRTLocalExpressionBuilder(
        _In_ WUComp::ICompositor* compositor,
        _In_ WUComp::IVisual* visual,
        _In_ WinRTLocalExpressionCache* cache
        );
    ~WinRTLocalExpressionBuilder() override;

    void ApplyProjectionExpression(_In_ WUComp::IExpressionAnimation* ProjectionExpression);

    void ApplyTransform3DExpression(_In_ WUComp::IExpressionAnimation* transform3DExpression);

    void ApplyFacadeTransforms(
        _In_ FacadeTransformInfo* facadeInfo
        ) override
    {
        // Should only ever be called on XamlLocalTransformBuilder
        ASSERT(FALSE);
    }

    void ApplyRenderTransform(
        _In_ CTransform* pTransform,
        float originX,
        float originY
        ) override;

    void ApplyHandOffVisualTransform(
        _In_ CTransform* pTransform,
        wfn::Vector3 translationFacade
        ) override;

    void ApplyTransitionTargetRenderTransform(
        _In_ CTransform* pTransform,
        float originX,
        float originY
        ) override;

    void ApplyFlowDirection(
        bool flipRTL,
        bool flipRTLInPlace,
        float unscaledElementWidth
        ) override;

    void ApplyOffsetAndDM(
        float offsetX,
        float offsetY,
        float dmOffsetX,
        float dmOffsetY,
        float dmZoomX,
        float dmZoomY,
        bool applyDMZoomToOffset
        ) override;

    void ApplyDManipSharedTransform(_In_ IUnknown* pDManipSharedTransform) override;

    void ApplyRedirectionTransform(_In_ RedirectionTransformInfo* redirInfo) override;

    void EnsureLocalExpression();

private:
    wrl::ComPtr<WUComp::IExpressionAnimation> ApplyTransform(
        _In_ CTransform* pTransform,
        float originX,
        float originY,
        _In_ const wchar_t* pTargetProperty
        );

    void EnsurePropertySet();

    std::wstring GetExpressionString();

    void ReleaseUnnecessarySubParts();

    wrl::ComPtr<WUComp::ICompositor> m_spCompositor;

    WinRTExpressionConversionContext m_winrtContext;

    wrl::ComPtr<WUComp::IVisual> m_visual;

    // The builder is given write-access to the expression related properties stored on the CompNode through the WinRTLocalExpressionCache.
    // This cache is actually owned by the CompNode, and also creates the builder on the stack, thus the pointer is a simple weak pointer.
    WinRTLocalExpressionCache* m_cache;

    // Transform component bit-flags:  each flag indicates which transform component is in use
    static const int TransformFlag_Projection = 1;
    static const int TransformFlag_Transform3D = 2;
    static const int TransformFlag_Render = 4;
    static const int TransformFlag_TTRender = 8;
    static const int TransformFlag_FlowDirection = 16;
    static const int TransformFlag_DManip = 32;
    static const int TransformFlag_Redir = 64;

    UINT m_transformFlags;   // Holds actual value of all bit-flags defined above
};
