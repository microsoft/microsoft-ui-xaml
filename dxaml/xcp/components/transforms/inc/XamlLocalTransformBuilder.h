// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "LocalTransformBuilder.h"

// Wraps a CMILMatrix and updates it in place as the local transform is built.
class XamlLocalTransformBuilder : public LocalTransformBuilder
{
public:
    XamlLocalTransformBuilder(_In_ CMILMatrix* pCombinedLocalTransform);

    void ApplyFacadeTransforms(
        _In_ FacadeTransformInfo* facadeInfo
        ) override;

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

private:
    void ApplyTransform(
        _In_ CTransform* pTransform,
        float originX,
        float originY
        );

protected:
    CMILMatrix* m_pCombinedLocalTransform;
};
