// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "Transform.h"
#include "WinRTExpressionConversionContext.h"
#include "MinMath.h"
#include "Real.h"
#include <DependencyObjectDCompRegistry.h>
#include <stack_vector.h>
#include "ExpressionHelper.h"

using namespace Microsoft::WRL::Wrappers;

CTransform::~CTransform()
{
    if (GetDCompObjectRegistry() != nullptr)
    {
        GetDCompObjectRegistry()->UnregisterObject(this);
    }
}

void CTransform::MakeWinRTExpressionWithOrigin(
    XFLOAT originX,
    XFLOAT originY,
    _Inout_ WinRTExpressionConversionContext* pWinRTContext,
    _Outptr_result_maybenull_ WUComp::IExpressionAnimation** ppExpression
    )
{
    MakeWinRTExpression(pWinRTContext);

    const bool needsOriginAdjustment = (originX != 0.0f || originY != 0.0f);

    if (needsOriginAdjustment)
    {
        Microsoft::WRL::ComPtr<WUComp::IExpressionAnimation> actualExpressionEA;
        Microsoft::WRL::ComPtr<WUComp::ICompositionAnimation> actualExpressionCA;
        Microsoft::WRL::ComPtr<WUComp::IExpressionAnimation> overallExpressionEA;
        Microsoft::WRL::ComPtr<WUComp::ICompositionPropertySet> propertySet;
        Microsoft::WRL::ComPtr<WUComp::ICompositionObject> propertySetCO;

        *ppExpression = nullptr;

        // Create overall expression
        pWinRTContext->CreateExpression(ExpressionHelper::sc_RtoFull3x2, &overallExpressionEA, &propertySet);

        IFCFAILFAST(propertySet->InsertMatrix3x2(HStringReference(ExpressionHelper::sc_RtoExpression).Get(), pWinRTContext->c_identityMatrix3x2));

        pWinRTContext->InitializeExpression(propertySet.Get(), ExpressionHelper::sc_paramName_TranslateX, originX);
        pWinRTContext->InitializeExpression(propertySet.Get(), ExpressionHelper::sc_paramName_TranslateY, originY);

        actualExpressionEA = GetWinRTExpression();
        IFCFAILFAST(actualExpressionEA.As(&actualExpressionCA));
        IFCFAILFAST(propertySet.As(&propertySetCO));
        IFCFAILFAST(propertySetCO->StartAnimation(HStringReference(ExpressionHelper::sc_RtoExpression).Get(), actualExpressionCA.Get()));

        overallExpressionEA.CopyTo(ppExpression);
    }
    else
    {
        SetInterface(*ppExpression, GetWinRTExpression());
    }
}

float CTransform::ClampAngle360(float originalAngle)
{
    float clamped = originalAngle;
    int nRotations = XcpFloor(originalAngle / 360.0f);

    // if the number of rotations doesn't fix in an XINT32, default to zero
    if ((nRotations == XINT32_MAX) || (nRotations == XINT32_MIN))
    {
        return 0.0f;
    }

    clamped -= (360.0f * nRotations);

    while (clamped < 0.0f)
    {
        clamped += 360.0f;
    }
    while (clamped >= 360.0f)
    {
        clamped -= 360.0f;
    }

    return clamped;
}

void CTransform::BuildRotateMatrix(
    _Out_ CMILMatrix *pMatrix,
    float eAngle,
    XPOINTF ptCenter)
{
    float eCos, eSin, clampedAngle;

    // Due to errors inherent in binary floating-point arithmetic, answers to
    //  some common angles aren't *exactly* what we would expect.  Hard-code the
    //  precise answers so we don't have to do fuzzy "close enough" comparisons
    //  elsewhere in the code base.

    // If the angle is outside of the range of 0-360, convert to its equivalent
    //  within that range so things like 540 degrees will still work with the
    //  special cases below.
    clampedAngle = ClampAngle360(eAngle);

    // Zero does not need a special case, because zero degrees will be converted
    //  to precisely zero radians.
    if (clampedAngle == 90.0f)
    {
        eCos = 0.0f;
        eSin = 1.0f;
    }
    else if (clampedAngle == 180.0f)
    {
        eCos = -1.0f;
        eSin = 0.0f;
    }
    else if (clampedAngle == 270.0f)
    {
        eCos = 0.0f;
        eSin = -1.0f;
    }
    else
    {
        eCos = MathCosDegrees(eAngle);
        eSin = MathSinDegrees(eAngle);
    }

    pMatrix->SetM11(eCos);
    pMatrix->SetM12(eSin);
    pMatrix->SetM21(-eSin);
    pMatrix->SetM22(eCos);
    pMatrix->SetDx(ptCenter.x * (1.0f - eCos) + (ptCenter.y * eSin));
    pMatrix->SetDy(ptCenter.y * (1.0f - eCos) - (ptCenter.x * eSin));
}

void CTransform::BuildSkewMatrix(
    _Out_ CMILMatrix* pMatrix,
    float eAngleX,
    float eAngleY,
    XPOINTF ptCenter)
{
    float eTanX, eTanY, clampedX, clampedY;

    // Due to errors inherent in binary floating-point arithmetic, answers to
    //  some common angles aren't *exactly* what we would expect.  Hard-code the
    //  precise answers so we don't have to do fuzzy "close enough" comparisons
    //  elsewhere in the codebase.

    // If the angle is outside of the range of 0-360, convert to its equivalent
    //  within that range so things like 540 degrees will still work with the
    //  special cases below.
    clampedX = ClampAngle360(eAngleX);
    clampedY = ClampAngle360(eAngleY);

    // Zero does not need a special case, because zero degrees will be converted
    //  to precisely zero radians.
    if (clampedX == 180.0f)
    {
        eTanX = 0.0f;
    }
    else
    {
        eTanX = MathTanDegrees(eAngleX);
    }

    if (clampedY == 180.0f)
    {
        eTanY = 0.0f;
    }
    else
    {
        eTanY = MathTanDegrees(eAngleY);
    }

    pMatrix->SetM11(1.0f);
    pMatrix->SetM12(eTanY);
    pMatrix->SetM21(eTanX);
    pMatrix->SetM22(1.0f);
    pMatrix->SetDx(-ptCenter.y * eTanX);
    pMatrix->SetDy(-ptCenter.x * eTanY);
}

void CTransform::ReleaseDCompResources()
{
    __super::ReleaseDCompResources();

    m_spWinRTExpression.Reset();
    m_isWinRTExpressionDirty = true;
}

void CTransform::SetDCompResourceDirty()
{
    if (!m_isWinRTExpressionDirty)
    {
        m_isWinRTExpressionDirty = true;
    }
}

void CTransform::NWSetRenderDirty(
    _In_ CDependencyObject *pTarget,
    DirtyFlags flags
    )
{
    ASSERT(pTarget->OfTypeByIndex<KnownTypeIndex::Transform>());

    CTransform *pTransform = static_cast<CTransform*>(pTarget);

    if (!pTransform->m_isWinRTExpressionDirty
        && !flags_enum::is_set(flags, DirtyFlags::Independent))
    {
        // TODO: DCompAnim: This will currently over-dirty the animated transform. Xaml will evaluate the transform one
        // last time when the storyboard expires, which goes through SetAnimatedValue with a dependent value, which then
        // gets here. This will mark the (currently animating in DComp) resource dirty, which will cause us to regenerate
        // the DComp transform without an animation. The result is that the DComp animation ends prematurely.
        // We'll have the same problem any time the animation evaluates on the Xaml UI thread and calls SetValue with
        // something dependent, which shows up as the DComp animation jumping forward or backwards. This is a temporary
        // problem - when all animations are converted to DComp, we can stop setting dependent values for DComp animations.
        pTransform->SetDCompResourceDirty();
    }

    __super::NWSetRenderDirty(pTarget, flags);
}

void CTransform::EnsureWUCAnimationStarted(_Inout_ WinRTExpressionConversionContext* context)
{
    // This method exists to make sure we connect WUC animations on transforms that aren't in the tree, because we still need to
    // know when those WUC animations end in order to let the Xaml animations complete. It also exists so that we disconnect WUC
    // animations on transforms that aren't in the tree when they stop animating, so that the DWM can stop ticking animations.
    //
    // For something like opacity or offset we can make a check here - if this object is not animated anymore, then we can release
    // its expression altogether. However, we can't do the same for transforms. Even if this transform has lost its last animation,
    // it could be part of a group where some other transform has an animation, and thus every transform in the group needs an
    // expression. So we have to assume that an expression is needed in general.
    //
    // There are specific cases where we can get away with not having an expression. If the entire transform group became static,
    // and it was part of the visible tree, then we would have visited it during the comp tree walk and explicitly released all
    // the WUC expressions. When we go and visit the transforms again from CTimeManager::UpdateTargetDOs at the end of the frame,
    // we can avoid creating new expressions for them by making a dirty flag check. Transforms that have already been visited are
    // clean. Leave them be - if they need an expression, they have one already, and if they don't, then they've released it already.
    if (m_isWinRTExpressionDirty)
    {
        MakeWinRTExpression(context, nullptr /* transformGroupPS */);
    }
}

void CTransform::ClearWUCExpression()
{
    m_spWinRTExpression.Reset();

    m_isWinRTExpressionDirty = false;
}
