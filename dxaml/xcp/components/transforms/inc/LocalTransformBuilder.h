// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "Matrix.h"
#include <fwd/windows.ui.composition.h>

class CTransform;

struct RedirectionTransformInfo
{
    CMILMatrix* redirectionTransform;
    WUComp::IVisual* visual;
};

// Holds facade property values which we'll incorporate into GetLocalTransform.
// These properties are only used by XamlLocalTransformBuilder.
// Note that we'll only incorporate the 2D components of these properties
struct FacadeTransformInfo
{
    wfn::Vector3 scale;
    float rotationAngleInDegrees;
    wfn::Vector3 rotationAxis;
    wfn::Matrix4x4 transformMatrix;
    wfn::Vector3 centerPoint;
    float translationZ;

    bool Is2D() const
    {
        return Matrix4x4Is2D(transformMatrix)
            && rotationAxis.X == 0
            && rotationAxis.Y == 0
            && scale.Z == 1
            && translationZ == 0;
    }

    const CMILMatrix4x4 GetMatrix4x4() const;
};

class LocalTransformBuilder
{
public:
    LocalTransformBuilder()
        : m_state(LocalTransformBuilderState::Initial)
    {
    }

    virtual ~LocalTransformBuilder() = default;

    virtual void ApplyFacadeTransforms(
        _In_ FacadeTransformInfo* facadeInfo
        ) = 0;

    virtual void ApplyRenderTransform(
        _In_ CTransform* pTransform,
        float originX,
        float originY
        ) = 0;

    virtual void ApplyHandOffVisualTransform(
        _In_ CTransform* pTransform,
        wfn::Vector3 translationFacade
        ) = 0;

    virtual void ApplyTransitionTargetRenderTransform(
        _In_ CTransform* pTransform,
        float originX,
        float originY
        ) = 0;

    virtual void ApplyFlowDirection(
        bool flipRTL,
        bool flipRTLInPlace,
        float unscaledElementWidth
        ) = 0;

    virtual void ApplyOffsetAndDM(
        float offsetX,
        float offsetY,
        float dmOffsetX,
        float dmOffsetY,
        float dmZoomX,
        float dmZoomY,
        bool applyDMZoomToOffset
        ) = 0;

    virtual void ApplyDManipSharedTransform(_In_ IUnknown* pDManipSharedTransform) = 0;

    virtual void ApplyRedirectionTransform(_In_ RedirectionTransformInfo* redirInfo) = 0;

protected:
    // Order of operations:
    // Facade transforms
    // Render transform
    // Transition target render transform
    // DM zoom transform
    // Flow direction
    // Offset
    // DManip shared transform
    // Redirection transform
    // Steps can be skipped, but the order cannot change.
    enum class LocalTransformBuilderState
    {
        Initial = 0,
        HasFacadeTransforms,
        HasRenderTransform,
        HasHandOffVisualTransform,
        HasTTRenderTransform,
        HasFlowDirection,
        HasOffsetAndDM,
        HasDManipSharedTransform,
        HasRedirectionTransform
    };

    void SetBuilderState(LocalTransformBuilderState newState)
    {
#if DBG
        ASSERT(m_state < newState);
        m_state = newState;
#else
        UNREFERENCED_PARAMETER(newState);
#endif
    }

    LocalTransformBuilderState m_state;
};
