// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "XamlLocalTransformBuilder.h"
#include "Transform.h"

const wfn::Matrix4x4 Matrix4x4_IdentityMatrix = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };

const CMILMatrix4x4 FacadeTransformInfo::GetMatrix4x4() const
{
    ASSERT(!Is2D());

    CMILMatrix4x4 matrix4x4(transformMatrix);

    // Note: No need to look at the transformMatrix. It's already been stored in the return value.
    const bool isIdentity = (scale.X == 1 && scale.Y == 1 && scale.Z == 1 && rotationAngleInDegrees == 0 && translationZ == 0);
    if (!isIdentity)
    {
        if (scale.Z != 1)
        {
            CMILMatrix4x4 scaleMatrix;
            scaleMatrix.SetToScaleAboutCenter(scale, centerPoint);
            matrix4x4.Append(scaleMatrix);
        }
        else if (scale.X != 1 || scale.Y != 1)
        {
            CMILMatrix scaleMatrix;
            scaleMatrix.SetToScaleAboutCenter(scale.X, scale.Y, centerPoint.X, centerPoint.Y);
            matrix4x4.Append(scaleMatrix);
        }

        if (rotationAngleInDegrees != 0)
        {
            // WUC's CenterPoint applies only to Rotation and Scale. Scale applies its own centering above, so we only need to use centering
            // matrices if there's a rotation.
            const bool hasCenterPoint = centerPoint.X != 0 || centerPoint.Y != 0 || centerPoint.Z != 0;

            if (hasCenterPoint)
            {
                CMILMatrix4x4 center;   // Move (x,y,z) to (0,0,0)
                center.SetToTranslation(-centerPoint.X, -centerPoint.Y, -centerPoint.Z);
                matrix4x4.Append(center);
            }

            static_assert(sizeof(CMILMatrix4x4) == sizeof(wfn_::float4x4), "Ensuring CMILMatrix4x4 size matches wfn_::float4x4 size");
            wfn_::float4x4 rotation4x4 = wfn_::make_float4x4_from_axis_angle(
                wfn_::normalize(rotationAxis),
                -ConvertDegreesToRadian(rotationAngleInDegrees));   // Negated, this function uses opposite handed-ness for rotation as DComp
            matrix4x4.Append(*reinterpret_cast<const CMILMatrix4x4*>(&rotation4x4));

            if (hasCenterPoint)
            {
                CMILMatrix4x4 uncenter; // Move (0,0,0) back to (x,y,z)
                uncenter.SetToTranslation(centerPoint.X, centerPoint.Y, centerPoint.Z);
                matrix4x4.Append(uncenter);
            }
        }

        if (translationZ != 0)
        {
            CMILMatrix4x4 translationZMatrix;
            translationZMatrix.SetToTranslation(0, 0, translationZ);    // X and Y are incorporated as offsets via GetLocalTransformHelper.
            matrix4x4.Append(translationZMatrix);
        }
    }

    return matrix4x4;
}

XamlLocalTransformBuilder::XamlLocalTransformBuilder(_In_ CMILMatrix* pCombinedLocalTransform)
    : m_pCombinedLocalTransform(pCombinedLocalTransform)
{
}

void XamlLocalTransformBuilder::ApplyFacadeTransforms(
    _In_ FacadeTransformInfo* facadeInfo
    )
{
    SetBuilderState(LocalTransformBuilderState::HasFacadeTransforms);

    // Note: If the facade properties represent a 3D transform, then we'll collect them separately via CUIElement::GetHitTestingTransform3DMatrix.
    // Note: The Translation facade's X and Y components interact with DManip and are treated as offsets. They're incorporated in ApplyOffsetAndDM.
    // The Translation.Z component is treated with the other facade properties. This separation is fine because there's nothing between facade
    // properties and OffsetAndDM that can mess with the combined transform (DManip has no Z component, and things like hand off transforms and
    // RenderTransform are disabled when facades are in use).
    // Note: Bug #18621253 will require additional work here, and below in ApplyHandOffVisualTransform(), to incorporate facades into the transform-to-root.
    if (facadeInfo->Is2D())
    {
        if (memcmp(&facadeInfo->transformMatrix, &Matrix4x4_IdentityMatrix, sizeof(wfn::Matrix4x4)) != 0)
        {
            CMILMatrix4x4 xamlMatrix4x4 = *reinterpret_cast<const CMILMatrix4x4*>(&facadeInfo->transformMatrix);
            if (xamlMatrix4x4.Is2D())
            {
                CMILMatrix matrix3x3 = xamlMatrix4x4.Get2DRepresentation();
                m_pCombinedLocalTransform->Append(matrix3x3);
            }
        }

        if (facadeInfo->scale.X != 1 || facadeInfo->scale.Y != 1)
        {
            CMILMatrix scaleTransform;
            scaleTransform.SetToScaleAboutCenter(facadeInfo->scale.X, facadeInfo->scale.Y, facadeInfo->centerPoint.X, facadeInfo->centerPoint.Y);
            m_pCombinedLocalTransform->Append(scaleTransform);
        }

        if (facadeInfo->rotationAngleInDegrees != 0)
        {
            // If the rotation axis is just the Z axis, then it's a 2D rotation
            ASSERT(facadeInfo->rotationAxis.X == 0 && facadeInfo->rotationAxis.Y == 0);

            // If the rotation angle is the negative Z axis, rotate in the other direction.
            // Treat 0 as the positive Z axis.
            float rotationAngle = facadeInfo->rotationAxis.Z >= 0 ? facadeInfo->rotationAngleInDegrees : -facadeInfo->rotationAngleInDegrees;

            CMILMatrix rotateTransform;
            rotateTransform.SetToRotationAboutCenter(rotationAngle, facadeInfo->centerPoint.X, facadeInfo->centerPoint.Y);
            m_pCombinedLocalTransform->Append(rotateTransform);
        }
    }
}

void XamlLocalTransformBuilder::ApplyRenderTransform(
    _In_ CTransform* pTransform,
    float originX,
    float originY
    )
{
    SetBuilderState(LocalTransformBuilderState::HasRenderTransform);

    ApplyTransform(pTransform, originX, originY);
}

void XamlLocalTransformBuilder::ApplyHandOffVisualTransform(
    _In_ CTransform* pTransform,
    wfn::Vector3 translationFacade
    )
{
    SetBuilderState(LocalTransformBuilderState::HasHandOffVisualTransform);

    ApplyTransform(pTransform, 0, 0);

    // Incorporate the Translation facade, but only if it isn't 3D.  If it is 3D, we will incorporate it
    // later in GetHitTestingTransform3DMatrix().
    // Note:  When used during the RenderWalk, the presence of 3D will force a CompNode and we'll incorporate
    // the X/Y channels correctly into the CompNode, however we will lose the sub-pixel offsets that contribute
    // to the transform-to-root.  We should address this as part of Bug #18621253.
    if (translationFacade.Z == 0)
    {
        m_pCombinedLocalTransform->AppendTranslation(translationFacade.X, translationFacade.Y);
    }
}

void XamlLocalTransformBuilder::ApplyTransitionTargetRenderTransform(
    _In_ CTransform* pTransform,
    float originX,
    float originY
    )
{
    SetBuilderState(LocalTransformBuilderState::HasTTRenderTransform);

    ApplyTransform(pTransform, originX, originY);
}

void XamlLocalTransformBuilder::ApplyTransform(
    _In_ CTransform* pTransform,
    float originX,
    float originY
    )
{
    CMILMatrix renderTransform(true);
    pTransform->GetTransform(&renderTransform);

    if (!renderTransform.IsIdentity())
    {
        const bool needsOriginAdjustment = (originX != 0.0f || originY != 0.0f);
        CMILMatrix matOriginAdjust;

        // Move to the origin before the local transform.
        if (needsOriginAdjustment)
        {
            matOriginAdjust.SetToIdentity();
            matOriginAdjust.SetDx(-originX);
            matOriginAdjust.SetDy(-originY);
            m_pCombinedLocalTransform->Append(matOriginAdjust);
        }

        m_pCombinedLocalTransform->Append(renderTransform);

        // Move back from origin once transform is applied.
        if (needsOriginAdjustment)
        {
            matOriginAdjust.SetDx(originX);
            matOriginAdjust.SetDy(originY);
            m_pCombinedLocalTransform->Append(matOriginAdjust);
        }
    }
}

void XamlLocalTransformBuilder::ApplyFlowDirection(
    bool flipRTL,
    bool flipRTLInPlace,
    float unscaledElementWidth
    )
{
    SetBuilderState(LocalTransformBuilderState::HasFlowDirection);

    if (flipRTL)
    {
        CMILMatrix matFlip(true);

        matFlip.SetM11(-1.0f);
        m_pCombinedLocalTransform->Append(matFlip);

        // Since the element was flipped over the y-axis it needs to be translated back by its
        // width to stay in the same position.
        if (flipRTLInPlace)
        {
            matFlip.SetM11(1);
            matFlip.SetDx(unscaledElementWidth);
            m_pCombinedLocalTransform->Append(matFlip);
        }
    }
}

void XamlLocalTransformBuilder::ApplyOffsetAndDM(
    float offsetX,
    float offsetY,
    float dmOffsetX,
    float dmOffsetY,
    float dmZoomX,
    float dmZoomY,
    bool applyDMZoomToOffset
    )
{
    SetBuilderState(LocalTransformBuilderState::HasOffsetAndDM);

    if (dmZoomX != 1.0f || dmZoomY != 1.0f)
    {
        CMILMatrix dmScaling(true);
        dmScaling.Scale(dmZoomX, dmZoomY);
        m_pCombinedLocalTransform->Append(dmScaling);
    }

    if (offsetX != 0.0f || offsetY != 0.0f || dmOffsetX != 0.0f || dmOffsetY != 0.0f)
    {
        CMILMatrix offset(true);
        float x = (applyDMZoomToOffset ? offsetX * dmZoomX : offsetX) + dmOffsetX;
        float y = (applyDMZoomToOffset ? offsetY * dmZoomY : offsetY) + dmOffsetY;
        offset.SetDx(x);
        offset.SetDy(y);
        m_pCombinedLocalTransform->Append(offset);
    }
}

void XamlLocalTransformBuilder::ApplyDManipSharedTransform(_In_ IUnknown* pDManipSharedTransform)
{
    ASSERT(FALSE);
}

void XamlLocalTransformBuilder::ApplyRedirectionTransform(_In_ RedirectionTransformInfo* redirInfo)
{
    SetBuilderState(LocalTransformBuilderState::HasRedirectionTransform);

    m_pCombinedLocalTransform->Append(*redirInfo->redirectionTransform);
}

