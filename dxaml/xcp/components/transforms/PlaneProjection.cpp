// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "PlaneProjection.h"
#include <DependencyObjectDCompRegistry.h>
#include <stack_vector.h>
#include "WinRTExpressionConversionContext.h"
#include "ExpressionHelper.h"
#include <TimeMgr.h>
#include <Animation.h>
#include "UIElement.h"

using namespace Microsoft::WRL::Wrappers;

CMILMatrix4x4 CPlaneProjection::GetProjectionMatrix() const
{
    CMILMatrix4x4 projection;
    CMILMatrix4x4 matTemp;

    // Compute 3D projection matrix:
    projection.SetToPerspective(
        c_nearPlane,
        c_farPlane,
        c_fieldOfView,
        1.0f, // aspect ratio
        c_rightHanded);

    // Adjust scale from our canonical projection
    matTemp.SetToScale(
        (c_farPlane - c_nearPlane) / projection._11,
        (c_farPlane - c_nearPlane) / projection._22,
        1.0f);
    projection.Append(matTemp);

    // Move back so that our defaults are in view
    matTemp.SetToTranslation(0.0f, 0.0f, c_zOffset);
    projection.Prepend(matTemp);

    return projection;
}

void CPlaneProjection::ReleaseDCompResources()
{
    __super::ReleaseDCompResources();

    SetDCompAnimation(nullptr, KnownPropertyIndex::PlaneProjection_CenterOfRotationXAnimation);
    SetDCompAnimation(nullptr, KnownPropertyIndex::PlaneProjection_CenterOfRotationYAnimation);
    SetDCompAnimation(nullptr, KnownPropertyIndex::PlaneProjection_CenterOfRotationZAnimation);
    SetDCompAnimation(nullptr, KnownPropertyIndex::PlaneProjection_GlobalOffsetXAnimation);
    SetDCompAnimation(nullptr, KnownPropertyIndex::PlaneProjection_GlobalOffsetYAnimation);
    SetDCompAnimation(nullptr, KnownPropertyIndex::PlaneProjection_GlobalOffsetZAnimation);
    SetDCompAnimation(nullptr, KnownPropertyIndex::PlaneProjection_LocalOffsetXAnimation);
    SetDCompAnimation(nullptr, KnownPropertyIndex::PlaneProjection_LocalOffsetYAnimation);
    SetDCompAnimation(nullptr, KnownPropertyIndex::PlaneProjection_LocalOffsetZAnimation);
    SetDCompAnimation(nullptr, KnownPropertyIndex::PlaneProjection_RotationXAnimation);
    SetDCompAnimation(nullptr, KnownPropertyIndex::PlaneProjection_RotationYAnimation);
    SetDCompAnimation(nullptr, KnownPropertyIndex::PlaneProjection_RotationZAnimation);

    m_localOffsetExpression.Reset();
    m_rotationExpression.Reset();
    m_rotateCenterExpression.Reset();
    m_undoRotateCenterExpression.Reset();
    m_rotateXExpression.Reset();
    m_rotateYExpression.Reset();
    m_rotateZExpression.Reset();
    m_globalOffsetExpression.Reset();
    m_perspectiveExpression.Reset();
}

void CPlaneProjection::MakeWinRTExpression(
    _Inout_ WinRTExpressionConversionContext* pWinRTContext,
    float elementWidth,
    float elementHeight
    )
{
    if (m_isWinRTProjectionDirty
        || elementWidth != m_elementWidth
        || elementHeight != m_elementHeight
        )
    {
        bool hasWidth = elementWidth != 0;
        bool hasHeight = elementHeight != 0;

        // CenterOfRotationX/Y are relative measurements, with 0 being the left/top of the element and 1 being the right/bottom.
        // This causes big problems when they animate, which we work around by adding extra transforms. When they're static, we
        // can just multiply in the width/height of the element and turn them into absolute coordinates.
        //
        // One way to set centers of rotation is to translate the element by (-centerX, -centerY), do the transform centered at
        // (0, 0), then translate the element back by (centerX, centerY). In our case, since the center is relative (and so is
        // the animation on it), we'll have to additionally do a scale down to get everything into relative coordinates, do the
        // translation, then do a scale back up. That gives us a [scale][translate][scale] on each end of the transforms.
        //
        // The perspective matrix applied by PlaneProjection is also looking at the center of the layout size of the projected
        // element, which adds another set of centering and un-centering transforms.
        //
        // Altogether, the order of matrices is:
        //
        // [leaf]-[Local XYZ]-[Scale down]-[CenterOfRotation]-[Scale up]-[Rotate X]-[Rotate Y]-[Rotate Z]-[Scale down]-
        //      [Undo CenterOfRotation]-[Scale up]-[Global XYZ]-[Perspective centering]-[Perspective]-[Undo perspective centering]-[root]
        //
        // Fortunately, if the neither of CenterOfRotationX/Y are animated, we can take the 6 centering-related matrices out of
        // the chain. Additionally, if the width or height of the element is 0, then the center value or animation will have no
        // effect.
        //
        // TODO: DComp: this means a layout change should dirty the projection render property, since the
        // center X/Y will have changed. Test after hooking everything up.
        //
        // TODO: DComp: Something still needs to update the projection matrix in Xaml - that's used for hit testing.

        const auto& centerXAnimation = GetWUCDCompAnimation(KnownPropertyIndex::PlaneProjection_CenterOfRotationXAnimation);
        const auto& centerYAnimation = GetWUCDCompAnimation(KnownPropertyIndex::PlaneProjection_CenterOfRotationYAnimation);
        const auto& centerZAnimation = GetWUCDCompAnimation(KnownPropertyIndex::PlaneProjection_CenterOfRotationZAnimation);
        const auto& globalXAnimation = GetWUCDCompAnimation(KnownPropertyIndex::PlaneProjection_GlobalOffsetXAnimation);
        const auto& globalYAnimation = GetWUCDCompAnimation(KnownPropertyIndex::PlaneProjection_GlobalOffsetYAnimation);
        const auto& globalZAnimation = GetWUCDCompAnimation(KnownPropertyIndex::PlaneProjection_GlobalOffsetZAnimation);
        const auto& localXAnimation = GetWUCDCompAnimation(KnownPropertyIndex::PlaneProjection_LocalOffsetXAnimation);
        const auto& localYAnimation = GetWUCDCompAnimation(KnownPropertyIndex::PlaneProjection_LocalOffsetYAnimation);
        const auto& localZAnimation = GetWUCDCompAnimation(KnownPropertyIndex::PlaneProjection_LocalOffsetZAnimation);
        const auto& angleXAnimation = GetWUCDCompAnimation(KnownPropertyIndex::PlaneProjection_RotationXAnimation);
        const auto& angleYAnimation = GetWUCDCompAnimation(KnownPropertyIndex::PlaneProjection_RotationYAnimation);
        const auto& angleZAnimation = GetWUCDCompAnimation(KnownPropertyIndex::PlaneProjection_RotationZAnimation);
        const auto& timeManager = GetTimeManager();

        // If the CenterOfRotationX/Y are both static, then we can just multiply in the element dimensions and explicitly
        // set a center of rotation on DComp. If one of them is animated, we'll have to add centering translations before &
        // after the rotations in DComp.
        const bool needsComplexCentering = centerXAnimation != nullptr || centerYAnimation != nullptr;
        // These are the centers used for the rotate transforms. If we already have the scale-translate-scale to take care
        // of centering, then we should use 0 for the rotate transforms themselves.
        const float staticCenterX = needsComplexCentering ? 0 : m_rCenterOfRotationX * elementWidth;
        const float staticCenterY = needsComplexCentering ? 0 : m_rCenterOfRotationY * elementHeight;
        const bool staticCenterChanged = elementWidth != m_elementWidth || elementHeight != m_elementHeight;

        const bool needsGlobalOffset =
            (globalXAnimation != nullptr) || (globalYAnimation != nullptr) || (globalZAnimation != nullptr)
            || (m_rGlobalOffsetX != 0.0f) || (m_rGlobalOffsetY != 0.0f) || (m_rGlobalOffsetZ != 0.0f);

        const bool needsLocalOffset =
            (localXAnimation != nullptr) || (localYAnimation != nullptr) || (localZAnimation != nullptr)
            || (m_rLocalOffsetX != 0.0f) || (m_rLocalOffsetY != 0.0f) || (m_rLocalOffsetZ != 0.0f);

        const bool needsRotation =
            (angleXAnimation != nullptr) || (angleYAnimation != nullptr) || (angleZAnimation != nullptr)
            || (m_rRotationX != 0.0f) || (m_rRotationY != 0.0f) || (m_rRotationZ != 0.0f);


        // Create overall PlaneProjection expression. Make sure to initialize component matrices to identity.
        if (m_spWinRTProjection == nullptr)
        {
            wrl::ComPtr<WUComp::ICompositionPropertySet> propertySet;
            pWinRTContext->CreateExpression(ExpressionHelper::sc_Expression_PlaneProjection, &m_spWinRTProjection, &propertySet);
            pWinRTContext->ConnectSubExpression(true /* useMatrix4x4 */, m_spWinRTProjection.Get(), nullptr /* subExpression */, ExpressionHelper::sc_propertyName_PlaneProjection_LocalOffset);
            pWinRTContext->ConnectSubExpression(true /* useMatrix4x4 */, m_spWinRTProjection.Get(), nullptr /* subExpression */, ExpressionHelper::sc_propertyName_PlaneProjection_Rotation);
            pWinRTContext->ConnectSubExpression(true /* useMatrix4x4 */, m_spWinRTProjection.Get(), nullptr /* subExpression */, ExpressionHelper::sc_propertyName_PlaneProjection_GlobalOffset);
            pWinRTContext->ConnectSubExpression(true /* useMatrix4x4 */, m_spWinRTProjection.Get(), nullptr /* subExpression */, ExpressionHelper::sc_propertyName_PlaneProjection_Perspective);

            if (GetDCompObjectRegistry() != nullptr)
            {
                GetDCompObjectRegistry()->EnsureObjectWithDCompResourceRegistered(this);
            }
        }

        // Local offset - optional
        {
            if (needsLocalOffset)
            {
                if (!m_localOffsetExpression)
                {
                    wrl::ComPtr<WUComp::ICompositionPropertySet> propertySet;
                    pWinRTContext->CreateExpression(ExpressionHelper::sc_Expression_Translate3D, &m_localOffsetExpression, &propertySet);

                    pWinRTContext->InitializeExpression(propertySet.Get(), ExpressionHelper::sc_paramName_TranslateX, m_rLocalOffsetX);
                    pWinRTContext->InitializeExpression(propertySet.Get(), ExpressionHelper::sc_paramName_TranslateY, m_rLocalOffsetY);
                    pWinRTContext->InitializeExpression(propertySet.Get(), ExpressionHelper::sc_paramName_TranslateZ, m_rLocalOffsetZ);
                }

                pWinRTContext->UpdateExpression(m_localOffsetExpression.Get(), ExpressionHelper::sc_paramName_TranslateX, m_rLocalOffsetX, m_isLocalOffsetXAnimationDirty, localXAnimation.get(), this, KnownPropertyIndex::PlaneProjection_LocalOffsetX, timeManager);
                pWinRTContext->UpdateExpression(m_localOffsetExpression.Get(), ExpressionHelper::sc_paramName_TranslateY, m_rLocalOffsetY, m_isLocalOffsetYAnimationDirty, localYAnimation.get(), this, KnownPropertyIndex::PlaneProjection_LocalOffsetY, timeManager);
                pWinRTContext->UpdateExpression(m_localOffsetExpression.Get(), ExpressionHelper::sc_paramName_TranslateZ, m_rLocalOffsetZ, m_isLocalOffsetZAnimationDirty, localZAnimation.get(), this, KnownPropertyIndex::PlaneProjection_LocalOffsetZ, timeManager);
                pWinRTContext->ConnectSubExpression(true /* useMatrix4x4 */, m_spWinRTProjection.Get(), m_localOffsetExpression.Get(), ExpressionHelper::sc_propertyName_PlaneProjection_LocalOffset);
            }
            else
            {
                pWinRTContext->ConnectSubExpression(true /* useMatrix4x4 */, m_spWinRTProjection.Get(), nullptr /* subExpression */, ExpressionHelper::sc_propertyName_PlaneProjection_LocalOffset);
                m_localOffsetExpression.Reset();
            }
        }

        // All Rotations - optional (though usually present)
        {
            if (needsRotation)
            {
                // We skip rotation animations if the element has 0 size, in which case we mark the WUC animation as completed
                // immediately so that we don't wait forever for a WUC completed that will never fire. The center Z animation
                // is used by all three of the rotation animations. Make sure that they're all skipped before skipping the
                // center Z animation as well.
                bool wasCenterZAnimationSkipped = true;

                if (!m_rotationExpression)
                {
                    wrl::ComPtr<WUComp::ICompositionPropertySet> propertySet;
                    pWinRTContext->CreateExpression(ExpressionHelper::sc_Expression_PlaneProjection_Rotation, &m_rotationExpression, &propertySet);
                    pWinRTContext->ConnectSubExpression(true /* useMatrix4x4 */, m_rotationExpression.Get(), nullptr /* subExpression */, ExpressionHelper::sc_propertyName_PlaneProjection_RotateCenter);
                    pWinRTContext->ConnectSubExpression(true /* useMatrix4x4 */, m_rotationExpression.Get(), nullptr /* subExpression */, ExpressionHelper::sc_paramName_RotateX);
                    pWinRTContext->ConnectSubExpression(true /* useMatrix4x4 */, m_rotationExpression.Get(), nullptr /* subExpression */, ExpressionHelper::sc_paramName_RotateY);
                    pWinRTContext->ConnectSubExpression(true /* useMatrix4x4 */, m_rotationExpression.Get(), nullptr /* subExpression */, ExpressionHelper::sc_paramName_RotateZ);
                    pWinRTContext->ConnectSubExpression(true /* useMatrix4x4 */, m_rotationExpression.Get(), nullptr /* subExpression */, ExpressionHelper::sc_propertyName_PlaneProjection_UndoRotateCenter);
                    pWinRTContext->ConnectSubExpression(true /* useMatrix4x4 */, m_spWinRTProjection.Get(), m_rotationExpression.Get(), ExpressionHelper::sc_propertyName_PlaneProjection_Rotation);
                }

                if ((hasWidth && hasHeight) &&
                     (angleXAnimation != nullptr
                     || m_rRotationX != 0.0f
                     || (staticCenterChanged && m_rotateXExpression)))
                {
                    if (m_rotateXExpression == nullptr)
                    {
                        wrl::ComPtr<WUComp::ICompositionPropertySet> propertySet;
                        pWinRTContext->CreateExpression(ExpressionHelper::sc_Expression_Rotate3D_X_CenterPoint, &m_rotateXExpression, &propertySet);

                        pWinRTContext->InitializeExpression(propertySet.Get(), ExpressionHelper::sc_paramName_RotateAngle, m_rRotationX);
                        pWinRTContext->InitializeExpression(propertySet.Get(), ExpressionHelper::sc_paramName_CenterX, staticCenterX);
                        pWinRTContext->InitializeExpression(propertySet.Get(), ExpressionHelper::sc_paramName_CenterY, staticCenterY);
                        pWinRTContext->InitializeExpression(propertySet.Get(), ExpressionHelper::sc_paramName_CenterZ, m_rCenterOfRotationZ);
                    }

                    pWinRTContext->UpdateExpression(m_rotateXExpression.Get(), ExpressionHelper::sc_paramName_RotateAngle, m_rRotationX, m_isRotationXAnimationDirty, angleXAnimation.get(), this, KnownPropertyIndex::PlaneProjection_RotationX, timeManager);
                    pWinRTContext->UpdateExpression(m_rotateXExpression.Get(), ExpressionHelper::sc_paramName_CenterX, staticCenterX, false /* isAnimationDirty */, nullptr /* animation */, nullptr /* targetObject */, KnownPropertyIndex::UnknownType_UnknownProperty, timeManager);
                    pWinRTContext->UpdateExpression(m_rotateXExpression.Get(), ExpressionHelper::sc_paramName_CenterY, staticCenterY, false /* isAnimationDirty */, nullptr /* animation */, nullptr /* targetObject */, KnownPropertyIndex::UnknownType_UnknownProperty, timeManager);
                    pWinRTContext->UpdateExpression(m_rotateXExpression.Get(), ExpressionHelper::sc_paramName_CenterZ, m_rCenterOfRotationZ, m_isCenterOfRotationZAnimationDirty, centerZAnimation.get(), this, KnownPropertyIndex::PlaneProjection_CenterOfRotationZ, timeManager);
                    pWinRTContext->ConnectSubExpression(true /* useMatrix4x4 */, m_rotationExpression.Get(), m_rotateXExpression.Get(), ExpressionHelper::sc_paramName_RotateX);

                    wasCenterZAnimationSkipped = false;
                }
                else
                {
                    pWinRTContext->ConnectSubExpression(true /* useMatrix4x4 */, m_rotationExpression.Get(), nullptr /* subExpression */, ExpressionHelper::sc_paramName_RotateX);
                    m_rotateXExpression.Reset();

                    if (angleXAnimation != nullptr)
                    {
                        // We skipped the WUC animation because the element has 0 size. Mark it as completed so that we don't wait on it forever.
                        timeManager->MarkWUCAnimationCompleted(this, KnownPropertyIndex::PlaneProjection_RotationX);
                    }
                }

                if ((hasWidth && hasHeight) &&
                     (angleYAnimation != nullptr
                     || m_rRotationY != 0.0f
                     || (staticCenterChanged && m_rotateYExpression)))
                {
                    if (m_rotateYExpression == nullptr)
                    {
                        wrl::ComPtr<WUComp::ICompositionPropertySet> propertySet;
                        pWinRTContext->CreateExpression(ExpressionHelper::sc_Expression_Rotate3D_Y_CenterPoint, &m_rotateYExpression, &propertySet);

                        pWinRTContext->InitializeExpression(propertySet.Get(), ExpressionHelper::sc_paramName_RotateAngle, m_rRotationY);
                        pWinRTContext->InitializeExpression(propertySet.Get(), ExpressionHelper::sc_paramName_CenterX, staticCenterX);
                        pWinRTContext->InitializeExpression(propertySet.Get(), ExpressionHelper::sc_paramName_CenterY, staticCenterY);
                        pWinRTContext->InitializeExpression(propertySet.Get(), ExpressionHelper::sc_paramName_CenterZ, m_rCenterOfRotationZ);
                    }

                    pWinRTContext->UpdateExpression(m_rotateYExpression.Get(), ExpressionHelper::sc_paramName_RotateAngle, m_rRotationY, m_isRotationYAnimationDirty, angleYAnimation.get(), this, KnownPropertyIndex::PlaneProjection_RotationY, timeManager);
                    pWinRTContext->UpdateExpression(m_rotateYExpression.Get(), ExpressionHelper::sc_paramName_CenterX, staticCenterX, false /* isAnimationDirty */, nullptr /* animation */, nullptr /* targetObject */, KnownPropertyIndex::UnknownType_UnknownProperty, timeManager);
                    pWinRTContext->UpdateExpression(m_rotateYExpression.Get(), ExpressionHelper::sc_paramName_CenterY, staticCenterY, false /* isAnimationDirty */, nullptr /* animation */, nullptr /* targetObject */, KnownPropertyIndex::UnknownType_UnknownProperty, timeManager);
                    pWinRTContext->UpdateExpression(m_rotateYExpression.Get(), ExpressionHelper::sc_paramName_CenterZ, m_rCenterOfRotationZ, m_isCenterOfRotationZAnimationDirty, centerZAnimation.get(), this, KnownPropertyIndex::PlaneProjection_CenterOfRotationZ, timeManager);
                    pWinRTContext->ConnectSubExpression(true /* useMatrix4x4 */, m_rotationExpression.Get(), m_rotateYExpression.Get(), ExpressionHelper::sc_paramName_RotateY);

                    wasCenterZAnimationSkipped = false;
                }
                else
                {
                    pWinRTContext->ConnectSubExpression(true /* useMatrix4x4 */, m_rotationExpression.Get(), nullptr /* subExpression */, ExpressionHelper::sc_paramName_RotateY);
                    m_rotateYExpression.Reset();

                    if (angleYAnimation != nullptr)
                    {
                        // We skipped the WUC animation because the element has 0 size. Mark it as completed so that we don't wait on it forever.
                        timeManager->MarkWUCAnimationCompleted(this, KnownPropertyIndex::PlaneProjection_RotationY);
                    }
                }

                if ((hasWidth && hasHeight) &&
                    (angleZAnimation != nullptr
                    || m_rRotationZ != 0.0f
                    || (staticCenterChanged && m_rotateZExpression)))
                {
                    if (m_rotateZExpression == nullptr)
                    {
                        wrl::ComPtr<WUComp::ICompositionPropertySet> propertySet;
                        pWinRTContext->CreateExpression(ExpressionHelper::sc_Expression_Rotate3D_Z_CenterPoint, &m_rotateZExpression, &propertySet);

                        pWinRTContext->InitializeExpression(propertySet.Get(), ExpressionHelper::sc_paramName_RotateAngle, m_rRotationZ);
                        pWinRTContext->InitializeExpression(propertySet.Get(), ExpressionHelper::sc_paramName_CenterX, staticCenterX);
                        pWinRTContext->InitializeExpression(propertySet.Get(), ExpressionHelper::sc_paramName_CenterY, staticCenterY);
                        pWinRTContext->InitializeExpression(propertySet.Get(), ExpressionHelper::sc_paramName_CenterZ, m_rCenterOfRotationZ);
                    }

                    pWinRTContext->UpdateExpression(m_rotateZExpression.Get(), ExpressionHelper::sc_paramName_RotateAngle, m_rRotationZ, m_isRotationZAnimationDirty, angleZAnimation.get(), this, KnownPropertyIndex::PlaneProjection_RotationZ, timeManager);
                    pWinRTContext->UpdateExpression(m_rotateZExpression.Get(), ExpressionHelper::sc_paramName_CenterX, staticCenterX, false /* isAnimationDirty */, nullptr /* animation */, nullptr /* targetObject */, KnownPropertyIndex::UnknownType_UnknownProperty, timeManager);
                    pWinRTContext->UpdateExpression(m_rotateZExpression.Get(), ExpressionHelper::sc_paramName_CenterY, staticCenterY, false /* isAnimationDirty */, nullptr /* animation */, nullptr /* targetObject */, KnownPropertyIndex::UnknownType_UnknownProperty, timeManager);
                    pWinRTContext->UpdateExpression(m_rotateZExpression.Get(), ExpressionHelper::sc_paramName_CenterZ, m_rCenterOfRotationZ, m_isCenterOfRotationZAnimationDirty, centerZAnimation.get(), this, KnownPropertyIndex::PlaneProjection_CenterOfRotationZ, timeManager);
                    pWinRTContext->ConnectSubExpression(true /* useMatrix4x4 */, m_rotationExpression.Get(), m_rotateZExpression.Get(), ExpressionHelper::sc_paramName_RotateZ);

                    wasCenterZAnimationSkipped = false;
                }
                else
                {
                    pWinRTContext->ConnectSubExpression(true /* useMatrix4x4 */, m_rotationExpression.Get(), nullptr /* subExpression */, ExpressionHelper::sc_paramName_RotateZ);
                    m_rotateZExpression.Reset();

                    if (angleZAnimation != nullptr)
                    {
                        // We skipped the WUC animation because the element has 0 size. Mark it as completed so that we don't wait on it forever.
                        timeManager->MarkWUCAnimationCompleted(this, KnownPropertyIndex::PlaneProjection_RotationZ);
                    }
                }

                if (wasCenterZAnimationSkipped && centerZAnimation != nullptr)
                {
                    // We skipped the WUC animation because the element has 0 size. Mark it as completed so that we don't wait on it forever.
                    timeManager->MarkWUCAnimationCompleted(this, KnownPropertyIndex::PlaneProjection_CenterOfRotationZ);
                }

                // If width or height the RotateCenter/UndoRotateCenterExpressions would have a division by 0.
                // In this corner case, nothing will render anyway, so just set them to identity
                if (needsComplexCentering && hasWidth && hasHeight)
                {
                    if (!m_rotateCenterExpression)
                    {
                        wrl::ComPtr<WUComp::ICompositionPropertySet> propertySet;
                        pWinRTContext->CreateExpression(ExpressionHelper::sc_Expression_PlaneProjection_RotateCenter, &m_rotateCenterExpression, &propertySet);

                        pWinRTContext->InitializeExpression(propertySet.Get(), ExpressionHelper::sc_propertyName_PlaneProjection_ElementWidth, elementWidth);
                        pWinRTContext->InitializeExpression(propertySet.Get(), ExpressionHelper::sc_propertyName_PlaneProjection_ElementHeight, elementHeight);
                        pWinRTContext->InitializeExpression(propertySet.Get(), ExpressionHelper::sc_paramName_CenterX, hasWidth ? m_rCenterOfRotationX : 0);
                        pWinRTContext->InitializeExpression(propertySet.Get(), ExpressionHelper::sc_paramName_CenterY, hasHeight ? m_rCenterOfRotationY : 0);
                    }

                    pWinRTContext->UpdateExpression(
                        m_rotateCenterExpression.Get(),
                        ExpressionHelper::sc_propertyName_PlaneProjection_ElementWidth,
                        elementWidth,
                        false,          // isAnimationDirty
                        nullptr,        // animation
                        nullptr,        // targetObject
                        KnownPropertyIndex::UnknownType_UnknownProperty,
                        timeManager);

                    pWinRTContext->UpdateExpression(
                        m_rotateCenterExpression.Get(),
                        ExpressionHelper::sc_propertyName_PlaneProjection_ElementHeight,
                        elementHeight,
                        false,          // isAnimationDirty
                        nullptr,        // animation
                        nullptr,        // targetObject
                        KnownPropertyIndex::UnknownType_UnknownProperty,
                        timeManager);

                    pWinRTContext->UpdateExpression(
                        m_rotateCenterExpression.Get(),
                        ExpressionHelper::sc_paramName_CenterX,
                        hasWidth ? m_rCenterOfRotationX : 0,
                        hasWidth && m_isCenterOfRotationXAnimationDirty,
                        centerXAnimation,
                        this,
                        KnownPropertyIndex::PlaneProjection_CenterOfRotationX,
                        timeManager);

                    pWinRTContext->UpdateExpression(
                        m_rotateCenterExpression.Get(),
                        ExpressionHelper::sc_paramName_CenterY,
                        hasHeight ? m_rCenterOfRotationY : 0,
                        hasHeight && m_isCenterOfRotationYAnimationDirty,
                        centerYAnimation,
                        this,
                        KnownPropertyIndex::PlaneProjection_CenterOfRotationY,
                        timeManager);

                    pWinRTContext->ConnectSubExpression(true /* useMatrix4x4 */, m_rotationExpression.Get(), m_rotateCenterExpression.Get(), ExpressionHelper::sc_propertyName_PlaneProjection_RotateCenter);

                    // UndoRotateCenter
                    if (!m_undoRotateCenterExpression)
                    {
                        wrl::ComPtr<WUComp::ICompositionPropertySet> propertySet;
                        pWinRTContext->CreateExpression(ExpressionHelper::sc_Expression_PlaneProjection_UndoRotateCenter, &m_undoRotateCenterExpression, &propertySet);

                        pWinRTContext->InitializeExpression(propertySet.Get(), ExpressionHelper::sc_propertyName_PlaneProjection_ElementWidth, elementWidth);
                        pWinRTContext->InitializeExpression(propertySet.Get(), ExpressionHelper::sc_propertyName_PlaneProjection_ElementHeight, elementHeight);
                        pWinRTContext->InitializeExpression(propertySet.Get(), ExpressionHelper::sc_paramName_CenterX, hasWidth ? m_rCenterOfRotationX : 0);
                        pWinRTContext->InitializeExpression(propertySet.Get(), ExpressionHelper::sc_paramName_CenterY, hasHeight ? m_rCenterOfRotationY : 0);
                    }

                    pWinRTContext->UpdateExpression(
                        m_undoRotateCenterExpression.Get(),
                        ExpressionHelper::sc_propertyName_PlaneProjection_ElementWidth,
                        elementWidth,
                        false,          // isAnimationDirty
                        nullptr,        // animation
                        nullptr,        // targetObject
                        KnownPropertyIndex::UnknownType_UnknownProperty,
                        timeManager);

                    pWinRTContext->UpdateExpression(
                        m_undoRotateCenterExpression.Get(),
                        ExpressionHelper::sc_propertyName_PlaneProjection_ElementHeight,
                        elementHeight,
                        false,          // isAnimationDirty
                        nullptr,        // animation
                        nullptr,        // targetObject
                        KnownPropertyIndex::UnknownType_UnknownProperty,
                        timeManager);

                    pWinRTContext->UpdateExpression(
                        m_undoRotateCenterExpression.Get(),
                        ExpressionHelper::sc_paramName_CenterX,
                        hasWidth ? m_rCenterOfRotationX : 0,
                        hasWidth && m_isCenterOfRotationXAnimationDirty,
                        centerXAnimation,
                        this,
                        KnownPropertyIndex::PlaneProjection_CenterOfRotationX,
                        timeManager);

                    pWinRTContext->UpdateExpression(
                        m_undoRotateCenterExpression.Get(),
                        ExpressionHelper::sc_paramName_CenterY,
                        hasHeight ? m_rCenterOfRotationY : 0,
                        hasHeight && m_isCenterOfRotationYAnimationDirty,
                        centerYAnimation,
                        this,
                        KnownPropertyIndex::PlaneProjection_CenterOfRotationY,
                        timeManager);

                    pWinRTContext->ConnectSubExpression(true /* useMatrix4x4 */, m_rotationExpression.Get(), m_undoRotateCenterExpression.Get(), ExpressionHelper::sc_propertyName_PlaneProjection_UndoRotateCenter);
                }
                else
                {
                    // Static center of rotation is handled by the RotateX/Y/Z transforms, so these expressions should be identity
                    // Also use these in the zero width/height case to avoid division by zero (nothing will render in that case anyway).
                    pWinRTContext->ConnectSubExpression(true /* useMatrix4x4 */, m_rotationExpression.Get(), nullptr /* subExpression */, ExpressionHelper::sc_propertyName_PlaneProjection_RotateCenter);
                    m_rotateCenterExpression.Reset();

                    pWinRTContext->ConnectSubExpression(true /* useMatrix4x4 */, m_rotationExpression.Get(), nullptr /* subExpression */, ExpressionHelper::sc_propertyName_PlaneProjection_UndoRotateCenter);
                    m_undoRotateCenterExpression.Reset();

                    if (centerXAnimation != nullptr)
                    {
                        // We skipped the WUC animation because the element has 0 size. Mark it as completed so that we don't wait on it forever.
                        timeManager->MarkWUCAnimationCompleted(this, KnownPropertyIndex::PlaneProjection_CenterOfRotationX);
                    }
                    if (centerYAnimation != nullptr)
                    {
                        // We skipped the WUC animation because the element has 0 size. Mark it as completed so that we don't wait on it forever.
                        timeManager->MarkWUCAnimationCompleted(this, KnownPropertyIndex::PlaneProjection_CenterOfRotationY);
                    }
                }
            }
            else
            {
                pWinRTContext->ConnectSubExpression(true /* useMatrix4x4 */, m_spWinRTProjection.Get(), nullptr /* subExpression */, ExpressionHelper::sc_propertyName_PlaneProjection_Rotation);
                m_rotationExpression.Reset();
            }
        }

        // Global offset - optional
        {
            if (needsGlobalOffset)
            {
                if (!m_globalOffsetExpression)
                {
                    wrl::ComPtr<WUComp::ICompositionPropertySet> propertySet;
                    pWinRTContext->CreateExpression(ExpressionHelper::sc_Expression_Translate3D, &m_globalOffsetExpression, &propertySet);

                    pWinRTContext->InitializeExpression(propertySet.Get(), ExpressionHelper::sc_paramName_TranslateX, m_rGlobalOffsetX);
                    pWinRTContext->InitializeExpression(propertySet.Get(), ExpressionHelper::sc_paramName_TranslateY, m_rGlobalOffsetY);
                    pWinRTContext->InitializeExpression(propertySet.Get(), ExpressionHelper::sc_paramName_TranslateZ, m_rGlobalOffsetZ);
                }

                pWinRTContext->UpdateExpression(m_globalOffsetExpression.Get(), ExpressionHelper::sc_paramName_TranslateX, m_rGlobalOffsetX, m_isGlobalOffsetXAnimationDirty, globalXAnimation.get(), this, KnownPropertyIndex::PlaneProjection_GlobalOffsetX, timeManager);
                pWinRTContext->UpdateExpression(m_globalOffsetExpression.Get(), ExpressionHelper::sc_paramName_TranslateY, m_rGlobalOffsetY, m_isGlobalOffsetYAnimationDirty, globalYAnimation.get(), this, KnownPropertyIndex::PlaneProjection_GlobalOffsetY, timeManager);
                pWinRTContext->UpdateExpression(m_globalOffsetExpression.Get(), ExpressionHelper::sc_paramName_TranslateZ, m_rGlobalOffsetZ, m_isGlobalOffsetZAnimationDirty, globalZAnimation.get(), this, KnownPropertyIndex::PlaneProjection_GlobalOffsetZ, timeManager);
                pWinRTContext->ConnectSubExpression(true /* useMatrix4x4 */, m_spWinRTProjection.Get(), m_globalOffsetExpression.Get(), ExpressionHelper::sc_propertyName_PlaneProjection_GlobalOffset);
            }
            else
            {
                pWinRTContext->ConnectSubExpression(true /* useMatrix4x4 */, m_spWinRTProjection.Get(), nullptr /* subExpression */, ExpressionHelper::sc_propertyName_PlaneProjection_GlobalOffset);
                m_globalOffsetExpression.Reset();
            }
        }

        // Perspective Transform - always present
        {
            if (!m_perspectiveExpression)
            {
                // We want to flatten the scene at the element with the PlaneProjection. Since the root-most transform in the chain
                // is just an XY translation, the Z flattening operation can be pushed into the second root-most transform, which is
                // the perspective matrix.
                wrl::ComPtr<WUComp::ICompositionPropertySet> propertySet;
                pWinRTContext->CreateExpression(ExpressionHelper::sc_Expression_PlaneProjection_Perspective, &m_perspectiveExpression, &propertySet);

                wfn::Matrix4x4 wfnProjectionMatrix;
                CMILMatrix4x4 projectionMatrix = GetProjectionMatrix();
                projectionMatrix.ToMatrix4x4(&wfnProjectionMatrix);

                IFCFAILFAST(propertySet->InsertMatrix4x4(HStringReference(ExpressionHelper::sc_propertyName_PlaneProjection_PerspectiveMatrix).Get(), wfnProjectionMatrix));

                pWinRTContext->InitializeExpression(propertySet.Get(), ExpressionHelper::sc_propertyName_PlaneProjection_ElementWidth, elementWidth);
                pWinRTContext->InitializeExpression(propertySet.Get(), ExpressionHelper::sc_propertyName_PlaneProjection_ElementHeight, elementHeight);

                pWinRTContext->ConnectSubExpression(true /* useMatrix4x4 */, m_spWinRTProjection.Get(), m_perspectiveExpression.Get(), ExpressionHelper::sc_propertyName_PlaneProjection_Perspective);
            }
            else if (elementWidth != m_elementWidth || elementHeight != m_elementHeight)
            {
                pWinRTContext->UpdateExpression(
                    m_perspectiveExpression.Get(),
                    ExpressionHelper::sc_propertyName_PlaneProjection_ElementWidth,
                    elementWidth,
                    false,          // isAnimationDirty
                    nullptr,        // animation
                    nullptr,        // targetObject
                    KnownPropertyIndex::UnknownType_UnknownProperty,
                    timeManager);

                pWinRTContext->UpdateExpression(
                    m_perspectiveExpression.Get(),
                    ExpressionHelper::sc_propertyName_PlaneProjection_ElementHeight,
                    elementHeight,
                    false,          // isAnimationDirty
                    nullptr,        // animation
                    nullptr,        // targetObject
                    KnownPropertyIndex::UnknownType_UnknownProperty,
                    timeManager);
            }
        }

        m_isWinRTProjectionDirty = false;
        m_isLocalOffsetXAnimationDirty = false;
        m_isLocalOffsetYAnimationDirty = false;
        m_isLocalOffsetZAnimationDirty = false;
        m_isRotationXAnimationDirty = false;
        m_isRotationYAnimationDirty = false;
        m_isRotationZAnimationDirty = false;
        m_isCenterOfRotationXAnimationDirty = false;
        m_isCenterOfRotationYAnimationDirty = false;
        m_isCenterOfRotationZAnimationDirty = false;
        m_isGlobalOffsetXAnimationDirty = false;
        m_isGlobalOffsetYAnimationDirty = false;
        m_isGlobalOffsetZAnimationDirty = false;

        m_elementWidth = elementWidth;
        m_elementHeight = elementHeight;
    }
}

void CPlaneProjection::ClearWUCExpression()
{
    __super::ClearWUCExpression();

    m_localOffsetExpression.Reset();
    m_rotationExpression.Reset();
    m_rotateCenterExpression.Reset();
    m_undoRotateCenterExpression.Reset();
    m_rotateXExpression.Reset();
    m_rotateYExpression.Reset();
    m_rotateZExpression.Reset();
    m_globalOffsetExpression.Reset();
    m_perspectiveExpression.Reset();

    m_isLocalOffsetXAnimationDirty = false;
    m_isLocalOffsetYAnimationDirty = false;
    m_isLocalOffsetZAnimationDirty = false;
    m_isRotationXAnimationDirty = false;
    m_isRotationYAnimationDirty = false;
    m_isRotationZAnimationDirty = false;
    m_isCenterOfRotationXAnimationDirty = false;
    m_isCenterOfRotationYAnimationDirty = false;
    m_isCenterOfRotationZAnimationDirty = false;
    m_isGlobalOffsetXAnimationDirty = false;
    m_isGlobalOffsetYAnimationDirty = false;
    m_isGlobalOffsetZAnimationDirty = false;
}

void CPlaneProjection::EnsureWUCAnimationStarted(_Inout_ WinRTExpressionConversionContext* context)
{
    // We're attaching the animation so we get the completed event. Use the cached element sizes from before, so that only
    // the dirty flag is checked. Passing in a different element size will result in us updating the expression again, which
    // we don't want - we only want to update it if it was missed by the render walk entirely.
    MakeWinRTExpression(context, m_elementWidth, m_elementHeight);
}
