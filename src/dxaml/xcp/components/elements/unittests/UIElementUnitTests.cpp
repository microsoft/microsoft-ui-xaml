// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "XamlLogging.h"
#include "UIElementUnitTests.h"
#include "UIElement.h"
#include "Matrix.h"
#include "TransitionTarget.h"
#include "CompositeTransform.h"
#include "TranslateTransform.h"
#include "XamlLocalTransformBuilder.h"

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Elements {

static bool CompareFloatsWithEpsilon(float a, float b, float epsilon)
{
    bool passed = (a - b < epsilon) && (b - a < epsilon);
    if (!passed)
    {
        LOG_OUTPUT(L"Expected %f, actual %f, epsilon %f", a, b, epsilon);
    }
    return passed;
}

void UIElementUnitTests::ValidateGetLocalTransform_Xaml()
{
    WEX::TestExecution::SetVerifyOutput verifySettings(WEX::TestExecution::VerifyOutputSettings::LogOnlyFailures);

    {
        LOG_OUTPUT(L"Offset");
        CMILMatrix output(true);
        XamlLocalTransformBuilder builder(&output);
        XPOINTF transformOrigin = {};

        CUIElement::GetLocalTransformHelper(
            &builder,
            1234, // offsetX
            4321, // offsetY
            0, // dmOffsetX
            0, // dmOffsetY
            1, // dmZoomFactorX
            1, // dmZoomFactorY
            false, // flipRTL
            false, // flipRTLInPlace
            100, // elementWidth
            100, // elementHeight
            nullptr, // pRenderTransform
            transformOrigin,
            nullptr, // pTransitionTarget
            false, // applyDMZoomToOffset
            nullptr, // pDManipSharedTransform
            nullptr, // redirInfo
            nullptr  // facadeInfo
            );

        VERIFY_ARE_EQUAL(1, output.GetM11());
        VERIFY_ARE_EQUAL(0, output.GetM12());
        VERIFY_ARE_EQUAL(0, output.GetM21());
        VERIFY_ARE_EQUAL(1, output.GetM22());
        VERIFY_ARE_EQUAL(1234, output.GetDx());
        VERIFY_ARE_EQUAL(4321, output.GetDy());
    }

    {
        LOG_OUTPUT(L"Offset with DM zoom");
        CMILMatrix output(true);
        XamlLocalTransformBuilder builder(&output);
        XPOINTF transformOrigin = {};

        CUIElement::GetLocalTransformHelper(
            &builder,
            1234, // offsetX
            4321, // offsetY
            0, // dmOffsetX
            0, // dmOffsetY
            0.5, // dmZoomFactorX
            2, // dmZoomFactorY
            false, // flipRTL
            false, // flipRTLInPlace
            100, // elementWidth
            100, // elementHeight
            nullptr, // pRenderTransform
            transformOrigin,
            nullptr, // pTransitionTarget
            true, // applyDMZoomToOffset
            nullptr, // pDManipSharedTransform
            nullptr, // redirInfo
            nullptr  // facadeInfo
            );

        VERIFY_ARE_EQUAL(0.5, output.GetM11());
        VERIFY_ARE_EQUAL(0, output.GetM12());
        VERIFY_ARE_EQUAL(0, output.GetM21());
        VERIFY_ARE_EQUAL(2, output.GetM22());
        VERIFY_ARE_EQUAL(617, output.GetDx());
        VERIFY_ARE_EQUAL(8642, output.GetDy());
    }

    {
        LOG_OUTPUT(L"RTL");
        CMILMatrix output(true);
        XamlLocalTransformBuilder builder(&output);
        XPOINTF transformOrigin = {};

        CUIElement::GetLocalTransformHelper(
            &builder,
            0, // offsetX
            0, // offsetY
            0, // dmOffsetX
            0, // dmOffsetY
            1, // dmZoomFactorX
            1, // dmZoomFactorY
            true, // flipRTL
            false, // flipRTLInPlace
            100, // elementWidth
            100, // elementHeight
            nullptr, // pRenderTransform
            transformOrigin,
            nullptr, // pTransitionTarget
            false, // applyDMZoomToOffset
            nullptr, // pDManipSharedTransform
            nullptr, // redirInfo
            nullptr  // facadeInfo
            );

        VERIFY_ARE_EQUAL(-1, output.GetM11());
        VERIFY_ARE_EQUAL(0, output.GetM12());
        VERIFY_ARE_EQUAL(0, output.GetM21());
        VERIFY_ARE_EQUAL(1, output.GetM22());
        VERIFY_ARE_EQUAL(0, output.GetDx());
        VERIFY_ARE_EQUAL(0, output.GetDy());
    }

    {
        LOG_OUTPUT(L"RTL in place");
        CMILMatrix output(true);
        XamlLocalTransformBuilder builder(&output);
        XPOINTF transformOrigin = {};

        CUIElement::GetLocalTransformHelper(
            &builder,
            0, // offsetX
            0, // offsetY
            0, // dmOffsetX
            0, // dmOffsetY
            1, // dmZoomFactorX
            1, // dmZoomFactorY
            true, // flipRTL
            true, // flipRTLInPlace
            100, // elementWidth
            100, // elementHeight
            nullptr, // pRenderTransform
            transformOrigin,
            nullptr, // pTransitionTarget
            false, // applyDMZoomToOffset
            nullptr, // pDManipSharedTransform
            nullptr, // redirInfo
            nullptr  // facadeInfo
            );

        VERIFY_ARE_EQUAL(-1, output.GetM11());
        VERIFY_ARE_EQUAL(0, output.GetM12());
        VERIFY_ARE_EQUAL(0, output.GetM21());
        VERIFY_ARE_EQUAL(1, output.GetM22());
        VERIFY_ARE_EQUAL(100, output.GetDx());
        VERIFY_ARE_EQUAL(0, output.GetDy());
    }

    {
        LOG_OUTPUT(L"RTL in place with zoom");
        CMILMatrix output(true);
        XamlLocalTransformBuilder builder(&output);
        XPOINTF transformOrigin = {};

        CUIElement::GetLocalTransformHelper(
            &builder,
            0, // offsetX
            0, // offsetY
            0, // dmOffsetX
            0, // dmOffsetY
            2, // dmZoomFactorX
            1, // dmZoomFactorY
            true, // flipRTL
            true, // flipRTLInPlace
            100, // elementWidth
            100, // elementHeight
            nullptr, // pRenderTransform
            transformOrigin,
            nullptr, // pTransitionTarget
            false, // applyDMZoomToOffset
            nullptr, // pDManipSharedTransform
            nullptr, // redirInfo
            nullptr  // facadeInfo
            );

        VERIFY_ARE_EQUAL(-2, output.GetM11());
        VERIFY_ARE_EQUAL(0, output.GetM12());
        VERIFY_ARE_EQUAL(0, output.GetM21());
        VERIFY_ARE_EQUAL(1, output.GetM22());
        VERIFY_ARE_EQUAL(200, output.GetDx());
        VERIFY_ARE_EQUAL(0, output.GetDy());
    }

    {
        LOG_OUTPUT(L"DM scale");
        CMILMatrix output(true);
        XamlLocalTransformBuilder builder(&output);
        XPOINTF transformOrigin = {};

        CUIElement::GetLocalTransformHelper(
            &builder,
            0, // offsetX
            0, // offsetY
            0, // dmOffsetX
            0, // dmOffsetY
            2, // dmZoomFactorX
            -0.5, // dmZoomFactorY
            false, // flipRTL
            false, // flipRTLInPlace
            100, // elementWidth
            100, // elementHeight
            nullptr, // pRenderTransform
            transformOrigin,
            nullptr, // pTransitionTarget
            false, // applyDMZoomToOffset
            nullptr, // pDManipSharedTransform
            nullptr, // redirInfo
            nullptr  // facadeInfo
            );

        VERIFY_ARE_EQUAL(2, output.GetM11());
        VERIFY_ARE_EQUAL(0, output.GetM12());
        VERIFY_ARE_EQUAL(0, output.GetM21());
        VERIFY_ARE_EQUAL(-0.5, output.GetM22());
        VERIFY_ARE_EQUAL(0, output.GetDx());
        VERIFY_ARE_EQUAL(0, output.GetDy());
    }

    {
        LOG_OUTPUT(L"Transition target");
        CMILMatrix output(true);
        XamlLocalTransformBuilder builder(&output);
        XPOINTF transformOrigin = {};

        CCompositeTransform transform;
        transform.m_eScaleX = 123;
        transform.m_eScaleY = 321;

        CTransitionTarget tt;
        tt.m_pxf = &transform;
        tt.m_ptRenderTransformOrigin = transformOrigin;

        CUIElement::GetLocalTransformHelper(
            &builder,
            0, // offsetX
            0, // offsetY
            0, // dmOffsetX
            0, // dmOffsetY
            1, // dmZoomFactorX
            1, // dmZoomFactorY
            false, // flipRTL
            false, // flipRTLInPlace
            100, // elementWidth
            100, // elementHeight
            nullptr, // pRenderTransform
            transformOrigin,
            &tt, // pTransitionTarget
            false, // applyDMZoomToOffset
            nullptr, // pDManipSharedTransform
            nullptr, // redirInfo
            nullptr  // facadeInfo
            );

        VERIFY_ARE_EQUAL(123, output.GetM11());
        VERIFY_ARE_EQUAL(0, output.GetM12());
        VERIFY_ARE_EQUAL(0, output.GetM21());
        VERIFY_ARE_EQUAL(321, output.GetM22());
        VERIFY_ARE_EQUAL(0, output.GetDx());
        VERIFY_ARE_EQUAL(0, output.GetDy());
    }

    {
        LOG_OUTPUT(L"Transition target with origin");
        CMILMatrix output(true);
        XamlLocalTransformBuilder builder(&output);
        XPOINTF transformOrigin = {};

        CCompositeTransform transform;
        transform.m_eScaleX = 123;
        transform.m_eScaleY = 321;

        CTransitionTarget tt;
        tt.m_pxf = &transform;
        tt.m_ptRenderTransformOrigin.x = 0.5;
        tt.m_ptRenderTransformOrigin.y = 1;

        CUIElement::GetLocalTransformHelper(
            &builder,
            0, // offsetX
            0, // offsetY
            0, // dmOffsetX
            0, // dmOffsetY
            1, // dmZoomFactorX
            1, // dmZoomFactorY
            false, // flipRTL
            false, // flipRTLInPlace
            100, // elementWidth
            100, // elementHeight
            nullptr, // pRenderTransform
            transformOrigin,
            &tt, // pTransitionTarget
            false, // applyDMZoomToOffset
            nullptr, // pDManipSharedTransform
            nullptr, // redirInfo
            nullptr  // facadeInfo
            );

        VERIFY_ARE_EQUAL(123, output.GetM11());
        VERIFY_ARE_EQUAL(0, output.GetM12());
        VERIFY_ARE_EQUAL(0, output.GetM21());
        VERIFY_ARE_EQUAL(321, output.GetM22());
        VERIFY_ARE_EQUAL(-6100, output.GetDx());
        VERIFY_ARE_EQUAL(-32000, output.GetDy());
    }

    {
        LOG_OUTPUT(L"Render transform");
        CMILMatrix output(true);
        XamlLocalTransformBuilder builder(&output);
        XPOINTF transformOrigin = {};

        CCompositeTransform transform;
        transform.m_eScaleX = 123;
        transform.m_eScaleY = 321;

        CUIElement::GetLocalTransformHelper(
            &builder,
            0, // offsetX
            0, // offsetY
            0, // dmOffsetX
            0, // dmOffsetY
            1, // dmZoomFactorX
            1, // dmZoomFactorY
            false, // flipRTL
            false, // flipRTLInPlace
            100, // elementWidth
            100, // elementHeight
            &transform, // pRenderTransform
            transformOrigin,
            nullptr, // pTransitionTarget
            false, // applyDMZoomToOffset
            nullptr, // pDManipSharedTransform
            nullptr, // redirInfo
            nullptr  // facadeInfo
            );

        VERIFY_ARE_EQUAL(123, output.GetM11());
        VERIFY_ARE_EQUAL(0, output.GetM12());
        VERIFY_ARE_EQUAL(0, output.GetM21());
        VERIFY_ARE_EQUAL(321, output.GetM22());
        VERIFY_ARE_EQUAL(0, output.GetDx());
        VERIFY_ARE_EQUAL(0, output.GetDy());
    }

    {
        LOG_OUTPUT(L"Render transform with origin");
        CMILMatrix output(true);
        XamlLocalTransformBuilder builder(&output);
        XPOINTF transformOrigin = {};
        transformOrigin.x = -0.5;
        transformOrigin.y = 1;

        CCompositeTransform transform;
        transform.m_eScaleX = 123;
        transform.m_eScaleY = 321;

        CUIElement::GetLocalTransformHelper(
            &builder,
            0, // offsetX
            0, // offsetY
            0, // dmOffsetX
            0, // dmOffsetY
            1, // dmZoomFactorX
            1, // dmZoomFactorY
            false, // flipRTL
            false, // flipRTLInPlace
            100, // elementWidth
            100, // elementHeight
            &transform, // pRenderTransform
            transformOrigin,
            nullptr, // pTransitionTarget
            false, // applyDMZoomToOffset
            nullptr, // pDManipSharedTransform
            nullptr, // redirInfo
            nullptr  // facadeInfo
            );

        VERIFY_ARE_EQUAL(123, output.GetM11());
        VERIFY_ARE_EQUAL(0, output.GetM12());
        VERIFY_ARE_EQUAL(0, output.GetM21());
        VERIFY_ARE_EQUAL(321, output.GetM22());
        VERIFY_ARE_EQUAL(6100, output.GetDx());
        VERIFY_ARE_EQUAL(-32000, output.GetDy());
    }

    {
        LOG_OUTPUT(L"Facade Transforms");
        CMILMatrix output(true);
        XamlLocalTransformBuilder builder(&output);
        XPOINTF transformOrigin = {};

        FacadeTransformInfo facadeInfo;
        facadeInfo.scale = {2, 2, 1};
        facadeInfo.rotationAngleInDegrees = 90;
        facadeInfo.transformMatrix = {1.5, 0, 0, 0, 0, 1.5, 0, 0, 0, 0, 1, 0, 100, 100, 0, 1};
        facadeInfo.centerPoint = {0, 0, 0};
        facadeInfo.rotationAxis = {0, 0, 1};
        facadeInfo.translationZ = 0;

        CUIElement::GetLocalTransformHelper(
            &builder,
            0, // offsetX
            0, // offsetY
            0, // dmOffsetX
            0, // dmOffsetY
            1, // dmZoomFactorX
            1, // dmZoomFactorY
            false, // flipRTL
            false, // flipRTLInPlace
            100, // elementWidth
            100, // elementHeight
            nullptr, // pRenderTransform
            transformOrigin,
            nullptr, // pTransitionTarget
            false, // applyDMZoomToOffset
            nullptr, // pDManipSharedTransform
            nullptr, // redirInfo
            &facadeInfo
            );

        XRECTF_RB testme = {0,0,100,0};
        output.TransformBounds(&testme, &testme);

        // Compare with epsilon, due to sin/cos producing small delta
        VERIFY_IS_TRUE(CompareFloatsWithEpsilon(0.0f, output.GetM11(), 0.001f));
        VERIFY_IS_TRUE(CompareFloatsWithEpsilon(-3.0f, output.GetM12(), 0.001f));
        VERIFY_IS_TRUE(CompareFloatsWithEpsilon(3.0f, output.GetM21(), 0.001f));
        VERIFY_IS_TRUE(CompareFloatsWithEpsilon(0.0f, output.GetM22(), 0.001f));
        VERIFY_IS_TRUE(CompareFloatsWithEpsilon(200.0f, output.GetDx(), 0.001f));
        VERIFY_IS_TRUE(CompareFloatsWithEpsilon(-200.0f, output.GetDy(), 0.001f));
    }

    {
        LOG_OUTPUT(L"Redirection transform");
        CMILMatrix output(true);
        XamlLocalTransformBuilder builder(&output);
        XPOINTF transformOrigin = {};


        CMILMatrix redirectionTransform(true);
        redirectionTransform.Scale(2.0f, 3.0f);
        RedirectionTransformInfo redirInfo = {&redirectionTransform, nullptr};

        CUIElement::GetLocalTransformHelper(
            &builder,
            20, // offsetX
            40, // offsetY
            0, // dmOffsetX
            0, // dmOffsetY
            1, // dmZoomFactorX
            1, // dmZoomFactorY
            false, // flipRTL
            false, // flipRTLInPlace
            100, // elementWidth
            100, // elementHeight
            nullptr, // pRenderTransform
            transformOrigin,
            nullptr, // pTransitionTarget
            false, // applyDMZoomToOffset
            nullptr, // pDManipSharedTransform
            &redirInfo,  // redirInfo
            nullptr  // facadeInfo
            );

        VERIFY_ARE_EQUAL(2.0f, output.GetM11());
        VERIFY_ARE_EQUAL(0, output.GetM12());
        VERIFY_ARE_EQUAL(0, output.GetM21());
        VERIFY_ARE_EQUAL(3.0f, output.GetM22());
        VERIFY_ARE_EQUAL(40.0f, output.GetDx());
        VERIFY_ARE_EQUAL(120.0f, output.GetDy());
    }

    {
        LOG_OUTPUT(L"Everything");
        CMILMatrix output(true);
        XamlLocalTransformBuilder builder(&output);
        XPOINTF transformOrigin = {};
        transformOrigin.x = -0.5;
        transformOrigin.y = 1;

        CCompositeTransform transform;
        transform.m_eScaleX = 1.5f;
        transform.m_eScaleY = 0.25f;

        CTransitionTarget tt;
        tt.m_pxf = &transform;
        tt.m_ptRenderTransformOrigin.x = 0.25;
        tt.m_ptRenderTransformOrigin.y = -0.5;

        CUIElement::GetLocalTransformHelper(
            &builder,
            10, // offsetX
            20, // offsetY
            30, // dmOffsetX
            40, // dmOffsetY
            0.5f, // dmZoomFactorX
            1.5f, // dmZoomFactorY
            true, // flipRTL
            false, // flipRTLInPlace
            200, // elementWidth
            100, // elementHeight
            &transform, // pRenderTransform
            transformOrigin,
            &tt, // pTransitionTarget
            true, // applyDMZoomToOffset
            nullptr, // pDManipSharedTransform
            nullptr, // redirInfo
            nullptr  // facadeInfo
            );

        VERIFY_ARE_EQUAL(-1.125f, output.GetM11());
        VERIFY_ARE_EQUAL(0, output.GetM12());
        VERIFY_ARE_EQUAL(0, output.GetM21());
        VERIFY_ARE_EQUAL(0.09375f, output.GetM22());
        VERIFY_ARE_EQUAL(10, output.GetDx());
        VERIFY_ARE_EQUAL(41.875, output.GetDy());
    }
}

void UIElementUnitTests::ValidateGetLocalTransform_Hybrid()
{
    WEX::TestExecution::SetVerifyOutput verifySettings(WEX::TestExecution::VerifyOutputSettings::LogOnlyFailures);

    {
        LOG_OUTPUT(L"Offset");
        CMILMatrix output(true);
        XamlLocalTransformBuilder builder(&output);
        XPOINTF transformOrigin = {};

        CUIElement::GetLocalTransformHelper(
            &builder,
            1234, // offsetX
            4321, // offsetY
            0, // dmOffsetX
            0, // dmOffsetY
            1, // dmZoomFactorX
            1, // dmZoomFactorY
            false, // flipRTL
            false, // flipRTLInPlace
            100, // elementWidth
            100, // elementHeight
            nullptr, // pRenderTransform
            transformOrigin,
            nullptr, // pTransitionTarget
            false, // applyDMZoomToOffset
            nullptr, // pDManipSharedTransform
            nullptr, // redirInfo
            nullptr  // facadeInfo
            );

        VERIFY_ARE_EQUAL(1, output.GetM11());
        VERIFY_ARE_EQUAL(0, output.GetM12());
        VERIFY_ARE_EQUAL(0, output.GetM21());
        VERIFY_ARE_EQUAL(1, output.GetM22());
        VERIFY_ARE_EQUAL(1234, output.GetDx());
        VERIFY_ARE_EQUAL(4321, output.GetDy());
    }
}

void UIElementUnitTests::ValidateHideElement()
{
    xref_ptr<CUIElement> obj;
    obj.attach(new CUIElement());

    obj->SetOpacityLocal(1.0f);

    LOG_OUTPUT(L"Verifying current opacity and dirty flags");
    VERIFY_IS_FALSE(!!obj->NWNeedsRendering());  // the object shouldn't need rendering at this point
    VERIFY_IS_TRUE(obj->GetOpacityCombined() > 0.9f);

    LOG_OUTPUT(L"Hiding object and verifying dirty flags are set");
    obj->HideElementForSuspendRendering(true /*hideElement*/);
    VERIFY_IS_TRUE(obj->GetOpacityCombined() < 0.1f);
    VERIFY_IS_TRUE(!!obj->NWNeedsRendering());

    LOG_OUTPUT(L"Showing element and verifying dirty flags are set");
    obj->HideElementForSuspendRendering(false /*hideElement*/);
    VERIFY_IS_TRUE(obj->GetOpacityCombined() > 0.9f);
    VERIFY_IS_TRUE(!!obj->NWNeedsRendering());
}

void UIElementUnitTests::ValidateInvalidateViewport()
{
    auto a = make_xref<CUIElement>();
    auto b = make_xref<CUIElement>();
    auto c = make_xref<CUIElement>();
    auto d = make_xref<CUIElement>();

    VERIFY_SUCCEEDED(a->AddChild(b));
    VERIFY_SUCCEEDED(b->AddChild(c));
    VERIFY_SUCCEEDED(c->AddChild(d));

    LOG_OUTPUT(L"Invalidate viewport; no subscribers.");
    d->InvalidateViewport();

    VERIFY_IS_FALSE(a->GetIsViewportDirty());
    VERIFY_IS_FALSE(a->GetIsOnViewportDirtyPath());
    VERIFY_IS_FALSE(b->GetIsViewportDirty());
    VERIFY_IS_FALSE(b->GetIsOnViewportDirtyPath());
    VERIFY_IS_FALSE(c->GetIsViewportDirty());
    VERIFY_IS_FALSE(c->GetIsOnViewportDirtyPath());
    VERIFY_IS_FALSE(d->GetIsViewportDirty());
    VERIFY_IS_FALSE(d->GetIsOnViewportDirtyPath());

    // Simulate an element subscribing to effective viewport changes.
    d->SetWantsViewport(TRUE);
    c->SetContributesToViewport(TRUE);
    b->SetContributesToViewport(TRUE);
    a->SetContributesToViewport(TRUE);

    LOG_OUTPUT(L"Invalidate viewport on element marked as LF_WANTS_VIEWPORT.");
    d->InvalidateViewport();

    VERIFY_IS_FALSE(a->GetIsViewportDirty());
    VERIFY_IS_TRUE(a->GetIsOnViewportDirtyPath());
    VERIFY_IS_FALSE(b->GetIsViewportDirty());
    VERIFY_IS_TRUE(b->GetIsOnViewportDirtyPath());
    VERIFY_IS_FALSE(c->GetIsViewportDirty());
    VERIFY_IS_TRUE(c->GetIsOnViewportDirtyPath());
    VERIFY_IS_TRUE(d->GetIsViewportDirty());
    VERIFY_IS_FALSE(d->GetIsOnViewportDirtyPath());

    // Reset flags.
    d->SetIsViewportDirty(FALSE);
    c->SetIsOnViewportDirtyPath(FALSE);
    b->SetIsOnViewportDirtyPath(FALSE);
    a->SetIsOnViewportDirtyPath(FALSE);

    LOG_OUTPUT(L"Invalidate viewport on element marked as LF_CONTRIBUTES_TO_VIEWPORT.");
    c->InvalidateViewport();

    VERIFY_IS_FALSE(a->GetIsViewportDirty());
    VERIFY_IS_TRUE(a->GetIsOnViewportDirtyPath());
    VERIFY_IS_FALSE(b->GetIsViewportDirty());
    VERIFY_IS_TRUE(b->GetIsOnViewportDirtyPath());
    VERIFY_IS_TRUE(c->GetIsViewportDirty());
    VERIFY_IS_FALSE(c->GetIsOnViewportDirtyPath());
    VERIFY_IS_FALSE(d->GetIsViewportDirty());
    VERIFY_IS_FALSE(d->GetIsOnViewportDirtyPath());
}

void UIElementUnitTests::ValidateEffectiveViewportWalk()
{
    auto a = make_xref<CUIElement>();
    auto b = make_xref<CUIElement>();
    auto c = make_xref<CUIElement>();
    auto d = make_xref<CUIElement>();
    auto e = make_xref<CUIElement>();
    auto f = make_xref<CUIElement>();
    auto g = make_xref<CUIElement>();
    auto h = make_xref<CUIElement>();
    auto i = make_xref<CUIElement>();
    auto j = make_xref<CUIElement>();
    auto k = make_xref<CUIElement>();
    auto l = make_xref<CUIElement>();
    auto m = make_xref<CUIElement>();

    a->EnsureLayoutStorage();
    b->EnsureLayoutStorage();
    c->EnsureLayoutStorage();
    d->EnsureLayoutStorage();
    e->EnsureLayoutStorage();
    f->EnsureLayoutStorage();
    g->EnsureLayoutStorage();
    h->EnsureLayoutStorage();
    i->EnsureLayoutStorage();
    j->EnsureLayoutStorage();
    k->EnsureLayoutStorage();
    l->EnsureLayoutStorage();
    m->EnsureLayoutStorage();

    VERIFY_SUCCEEDED(a->AddChild(b));
    VERIFY_SUCCEEDED(b->AddChild(c));
    VERIFY_SUCCEEDED(b->AddChild(d));
    VERIFY_SUCCEEDED(b->AddChild(e));
    VERIFY_SUCCEEDED(b->AddChild(f));
    VERIFY_SUCCEEDED(e->AddChild(g));
    VERIFY_SUCCEEDED(f->AddChild(h));
    VERIFY_SUCCEEDED(f->AddChild(i));
    VERIFY_SUCCEEDED(f->AddChild(j));
    VERIFY_SUCCEEDED(h->AddChild(k));
    VERIFY_SUCCEEDED(i->AddChild(l));
    VERIFY_SUCCEEDED(k->AddChild(m));

    a->SetIsOnViewportDirtyPath(TRUE);
    a->SetContributesToViewport(TRUE);
    b->SetIsOnViewportDirtyPath(TRUE);
    b->SetContributesToViewport(TRUE);
    d->SetWantsViewport(TRUE);
    e->SetContributesToViewport(TRUE);
    f->SetIsViewportDirty(TRUE);
    f->SetContributesToViewport(TRUE);
    g->SetWantsViewport(TRUE);
    h->SetContributesToViewport(TRUE);
    i->SetWantsViewport(TRUE);
    k->SetContributesToViewport(TRUE);
    k->SetWantsViewport(TRUE);
    m->SetWantsViewport(TRUE);

    //                 (A^*)
    //                   |
    //                 (B^*)
    //                   |
    //        ___________|__________
    //       |      |       |       |
    //      (C)    (D+)    (E*)    (F#*)
    //                      |       |
    //                     (G+)     |
    //                              |
    //                       _______|_______
    //                      |       |       |
    //                     (H*)    (I+)    (J)
    //                      |       |
    //                    (K*+)    (L)
    //                      |
    //                     (M+)
    //
    // ^  =  LF_ON_VIEWPORT_DIRTY_PATH
    // #  =  LF_VIEWPORT_DIRTY
    // *  =  LF_CONTRIBUTES_TO_VIEWPORT
    // +  =  LF_WANTS_VIEWPORT

    std::vector<CUIElement::TransformToPreviousViewport> emptyTransformViewpointVector;
    std::vector<CUIElement::UnidimensionalViewportInformation> emptyInformationVector;
    VERIFY_SUCCEEDED(a->EffectiveViewportWalk(
        false,
        emptyTransformViewpointVector,
        emptyInformationVector,
        emptyInformationVector));

    VERIFY_IS_FALSE(a->GetIsOnViewportDirtyPath());
    VERIFY_IS_FALSE(b->GetIsOnViewportDirtyPath());
    VERIFY_IS_FALSE(f->GetIsViewportDirty());
}

void UIElementUnitTests::ValidateComputeUnidimensionalEffectiveViewport()
{
    float visibleOffset = 0.0f;
    float visibleLength = 0.0f;
    std::vector<CUIElement::UnidimensionalViewportInformation> viewports;

    viewports.clear();
    ComputeUnidimensionalEffectiveViewport(viewports, visibleOffset, visibleLength);
    VERIFY_ARE_EQUAL(std::numeric_limits<float>::infinity(), visibleOffset);
    VERIFY_ARE_EQUAL(-std::numeric_limits<float>::infinity(), visibleLength);

    viewports.push_back(CUIElement::UnidimensionalViewportInformation(0.0f, 1000.0f));
    viewports.push_back(CUIElement::UnidimensionalViewportInformation(100.0f, 200.0f));
    viewports.push_back(CUIElement::UnidimensionalViewportInformation(200.0f, 600.0f));
    ComputeUnidimensionalEffectiveViewport(viewports, visibleOffset, visibleLength);
    VERIFY_ARE_EQUAL(200.0f, visibleOffset);
    VERIFY_ARE_EQUAL(100.0f, visibleLength);
    viewports.clear();

    viewports.push_back(CUIElement::UnidimensionalViewportInformation(0.0f, 1000.0f));
    viewports.push_back(CUIElement::UnidimensionalViewportInformation(400.0f, 200.0f));
    viewports.push_back(CUIElement::UnidimensionalViewportInformation(200.0f, 600.0f));
    ComputeUnidimensionalEffectiveViewport(viewports, visibleOffset, visibleLength);
    VERIFY_ARE_EQUAL(400.0f, visibleOffset);
    VERIFY_ARE_EQUAL(200.0f, visibleLength);
    viewports.clear();

    viewports.push_back(CUIElement::UnidimensionalViewportInformation(0.0f, 1000.0f));
    viewports.push_back(CUIElement::UnidimensionalViewportInformation(700.0f, 200.0f));
    viewports.push_back(CUIElement::UnidimensionalViewportInformation(200.0f, 600.0f));
    ComputeUnidimensionalEffectiveViewport(viewports, visibleOffset, visibleLength);
    VERIFY_ARE_EQUAL(700.0f, visibleOffset);
    VERIFY_ARE_EQUAL(100.0f, visibleLength);
    viewports.clear();

    // The viewports do not intersect.
    viewports.push_back(CUIElement::UnidimensionalViewportInformation(0.0f, 500.0f));
    viewports.push_back(CUIElement::UnidimensionalViewportInformation(100.0f, 500.0f));
    viewports.push_back(CUIElement::UnidimensionalViewportInformation(700.0f, 200.0f));
    ComputeUnidimensionalEffectiveViewport(viewports, visibleOffset, visibleLength);
    VERIFY_ARE_EQUAL(std::numeric_limits<float>::infinity(), visibleOffset);
    VERIFY_ARE_EQUAL(-std::numeric_limits<float>::infinity(), visibleLength);
    viewports.clear();
}

void UIElementUnitTests::ValidateComputeUnidimensionalMaxViewport()
{
    float maxOffset = 0.0f;
    float maxLength = 0.0f;
    std::vector<CUIElement::UnidimensionalViewportInformation> viewports;

    viewports.clear();
    ComputeUnidimensionalMaxViewport(viewports, maxOffset, maxLength);
    VERIFY_ARE_EQUAL(std::numeric_limits<float>::infinity(), maxOffset);
    VERIFY_ARE_EQUAL(-std::numeric_limits<float>::infinity(), maxLength);

    viewports.push_back(CUIElement::UnidimensionalViewportInformation(0.0f, 1000.0f));
    viewports.push_back(CUIElement::UnidimensionalViewportInformation(100.0f, 100.0f));
    viewports.push_back(CUIElement::UnidimensionalViewportInformation(300.0f, 400.0f));
    ComputeUnidimensionalMaxViewport(viewports, maxOffset, maxLength);
    VERIFY_ARE_EQUAL(300.0f, maxOffset);
    VERIFY_ARE_EQUAL(100.0f, maxLength);
    viewports.clear();

    viewports.push_back(CUIElement::UnidimensionalViewportInformation(0.0f, 1000.0f));
    viewports.push_back(CUIElement::UnidimensionalViewportInformation(450.0f, 100.0f));
    viewports.push_back(CUIElement::UnidimensionalViewportInformation(300.0f, 400.0f));
    ComputeUnidimensionalMaxViewport(viewports, maxOffset, maxLength);
    VERIFY_ARE_EQUAL(450.0f, maxOffset);
    VERIFY_ARE_EQUAL(100.0f, maxLength);
    viewports.clear();

    viewports.push_back(CUIElement::UnidimensionalViewportInformation(0.0f, 1000.0f));
    viewports.push_back(CUIElement::UnidimensionalViewportInformation(800.0f, 100.0f));
    viewports.push_back(CUIElement::UnidimensionalViewportInformation(300.0f, 400.0f));
    ComputeUnidimensionalMaxViewport(viewports, maxOffset, maxLength);
    VERIFY_ARE_EQUAL(600.0f, maxOffset);
    VERIFY_ARE_EQUAL(100.0f, maxLength);
    viewports.clear();
}

void UIElementUnitTests::ValidateComputeUnidimensionalBringIntoViewDistance()
{
    float distance = 0.0f;
    std::vector<CUIElement::UnidimensionalViewportInformation> viewports;

    viewports.clear();
    ComputeUnidimensionalBringIntoViewDistance(400.0f, 100.0f, viewports, distance);
    VERIFY_ARE_EQUAL(std::numeric_limits<float>::infinity(), distance);

    viewports.push_back(CUIElement::UnidimensionalViewportInformation(0.0f, 1000.0f));
    viewports.push_back(CUIElement::UnidimensionalViewportInformation(100.0f, 300.0f));
    viewports.push_back(CUIElement::UnidimensionalViewportInformation(300.0f, 400.0f));
    ComputeUnidimensionalBringIntoViewDistance(200.0f, 200.0f, viewports, distance);
    VERIFY_ARE_EQUAL(200.0f, distance);
    viewports.clear();

    viewports.push_back(CUIElement::UnidimensionalViewportInformation(0.0f, 1000.0f));
    viewports.push_back(CUIElement::UnidimensionalViewportInformation(600.0f, 300.0f));
    viewports.push_back(CUIElement::UnidimensionalViewportInformation(300.0f, 400.0f));
    ComputeUnidimensionalBringIntoViewDistance(600.0f, 200.0f, viewports, distance);
    VERIFY_ARE_EQUAL(200.0f, distance);
    viewports.clear();

    viewports.push_back(CUIElement::UnidimensionalViewportInformation(0.0f, 1000.0f));
    viewports.push_back(CUIElement::UnidimensionalViewportInformation(200.0f, 300.0f));
    viewports.push_back(CUIElement::UnidimensionalViewportInformation(400.0f, 400.0f));
    ComputeUnidimensionalBringIntoViewDistance(0.0f, 500.0f, viewports, distance);
    VERIFY_ARE_EQUAL(500.0f, distance);
    viewports.clear();

    viewports.push_back(CUIElement::UnidimensionalViewportInformation(0.0f, 1000.0f));
    viewports.push_back(CUIElement::UnidimensionalViewportInformation(500.0f, 300.0f));
    viewports.push_back(CUIElement::UnidimensionalViewportInformation(200.0f, 400.0f));
    ComputeUnidimensionalBringIntoViewDistance(500.0f, 500.0f, viewports, distance);
    VERIFY_ARE_EQUAL(500.0f, distance);
    viewports.clear();

    // The element is fully visible through the viewport;
    // no correction is needed.
    viewports.push_back(CUIElement::UnidimensionalViewportInformation(0.0f, 1000.0f));
    viewports.push_back(CUIElement::UnidimensionalViewportInformation(300.0f, 400.0f));
    viewports.push_back(CUIElement::UnidimensionalViewportInformation(200.0f, 600.0f));
    ComputeUnidimensionalBringIntoViewDistance(400.0f, 200.0f, viewports, distance);
    VERIFY_ARE_EQUAL(0.0f, distance);
    viewports.clear();

    // The element is larger than the viewport, but the max area is fully
    // visible; no correction is needed.
    viewports.push_back(CUIElement::UnidimensionalViewportInformation(0.0f, 1000.0f));
    viewports.push_back(CUIElement::UnidimensionalViewportInformation(400.0f, 200.0f));
    viewports.push_back(CUIElement::UnidimensionalViewportInformation(300.0f, 400.0f));
    ComputeUnidimensionalBringIntoViewDistance(200.0f, 600.0f, viewports, distance);
    VERIFY_ARE_EQUAL(0.0f, distance);
    viewports.clear();
}

} } } } } }
