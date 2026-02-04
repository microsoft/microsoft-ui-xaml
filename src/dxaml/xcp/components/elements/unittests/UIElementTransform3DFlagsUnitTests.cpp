// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "XamlLogging.h"
#include "UIElementTransform3DFlagsUnitTests.h"

#include "UIElementCollection.h"

#include "PerspectiveTransform3D.h"
#include "CompositeTransform3D.h"

#include <MetadataAPI.h>
#include <TypeTableStructs.h>
#include <MockDependencyProperty.h>
#include "RuntimeEnabledFeatures.h"

using namespace RuntimeFeatureBehavior;
using namespace Microsoft::UI::Xaml::Tests::Metadata;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Elements {

// HasDepth() returns true IFF the element itself contains a transform with depth
void UIElementTransform3DFlagsUnitTests::ValidateHasDepthOnOneElement()
{
    {
        auto element = make_xref<CUIElement>();
        auto pTx3D = make_xref<CPerspectiveTransform3D>();
        CValue value;
        value.WrapObjectNoRef(pTx3D);
        VERIFY_SUCCEEDED(element->SetValueByKnownIndex(KnownPropertyIndex::UIElement_Transform3D, value));

        // PerspectiveTransform3D still has depth now that we preserve 3D
        element->UpdateHas3DDepth();
        VERIFY_IS_TRUE(element->Has3DDepth());
    }

    {
        auto element = make_xref<CUIElement>();
        auto cTx3D = make_xref<CCompositeTransform3D>();
        CValue value;
        value.WrapObjectNoRef(cTx3D);
        VERIFY_SUCCEEDED(element->SetValueByKnownIndex(KnownPropertyIndex::UIElement_Transform3D, value));

        // Default CompositeTransform3D does not have depth
        element->UpdateHas3DDepth();
        VERIFY_IS_FALSE(element->Has3DDepth());

        // ScaleX/Y, TranslationX/Y, RotationZ do not have depth. (These cases are tested in CompositeTransform3DUnitTests.cpp)
        cTx3D->m_rScaleX = 2;
        element->UpdateHas3DDepth();
        VERIFY_IS_FALSE(element->Has3DDepth());

        // RotationX/Y, ScaleZ, TranslateZ do have depth.
        cTx3D->m_rRotationY = 20;
        element->UpdateHas3DDepth();
        VERIFY_IS_TRUE(element->Has3DDepth());
    }

    {
        auto element = make_xref<CUIElement>();
        auto cTx3D = make_xref<CCompositeTransform3D>();
        CValue value;
        value.WrapObjectNoRef(cTx3D);
        VERIFY_SUCCEEDED(element->SetValueByKnownIndex(KnownPropertyIndex::UIElement_Transform3D, value));

        // RotationY has depth if it's not default
        cTx3D->m_rRotationY = 20;
        element->UpdateHas3DDepth();
        VERIFY_IS_TRUE(element->Has3DDepth());

        // Ensure that resetting RotationY gets rid of depth
        cTx3D->m_rRotationY = 0;
        element->UpdateHas3DDepth();
        VERIFY_IS_FALSE(element->Has3DDepth());
    }
}

void UIElementTransform3DFlagsUnitTests::ValidateHasDepthInSubtreeSmallAdd()
{
    // Simple tree with three elements:
    //  (R)
    //   |
    //  (A)  {PTx3D}
    //   |
    //  (B)  {CTx3D with Depth}
    //
    // Intermediate state based on if PTx3D or CTx3D added first.
    //
    // End state:
    // R: HasDepth = false, HasDepthInSubtree = true
    // A: HasDepth = true, HasDepthInSubtree = true
    // B: HasDepth = true, HasDepthInSubtree = false

    {
        // TESTCASE: Add PTx3D first.
        // Intermediate state:
        // R: HasDepth = false, HasDepthInSubtree = true
        // A: HasDepth = true, HasDepthInSubtree = false
        // B: HasDepth = false, HasDepthInSubtree = false
        auto R = make_xref<CUIElement>();
        auto A = make_xref<CUIElement>();
        auto B = make_xref<CUIElement>();

        VERIFY_SUCCEEDED(R->AddChild(A));
        VERIFY_SUCCEEDED(A->AddChild(B));

        auto pTx3D = make_xref<CPerspectiveTransform3D>();
        auto cTx3D1 = make_xref<CCompositeTransform3D>();
        cTx3D1->m_rRotationY = 20; // Depth transform

        // Add perspective.
        SetTransform3DOnElement(A, pTx3D);

        // Check Intermediate state.
        VerifyDepthAndHasDepthInSubtree(R, false, true);
        VerifyDepthAndHasDepthInSubtree(A, true, false);
        VerifyDepthAndHasDepthInSubtree(B, false, false);

        // Then add CTx3D.
        SetTransform3DOnElement(B, cTx3D1);

        // Verify final state.
        VerifyDepthAndHasDepthInSubtree(R, false, true);
        VerifyDepthAndHasDepthInSubtree(A, true, true);
        VerifyDepthAndHasDepthInSubtree(B, true, false);
    }

    {
        // TESTCASE: Add PTx3D first, then identity CTx3D, then add depth to CTx3D
        // Intermediate state:
        // R: HasDepth = false, HasDepthInSubtree = true
        // A: HasDepth = true, HasDepthInSubtree = false
        // B: HasDepth = false, HasDepthInSubtree = false
        auto R = make_xref<CUIElement>();
        auto A = make_xref<CUIElement>();
        auto B = make_xref<CUIElement>();

        VERIFY_SUCCEEDED(R->AddChild(A));
        VERIFY_SUCCEEDED(A->AddChild(B));

        auto pTx3D = make_xref<CPerspectiveTransform3D>();
        auto cTx3D1 = make_xref<CCompositeTransform3D>();

        // Add perspective.
        SetTransform3DOnElement(A, pTx3D);

        // Check Intermediate state.
        VerifyDepthAndHasDepthInSubtree(R, false, true);
        VerifyDepthAndHasDepthInSubtree(A, true, false);
        VerifyDepthAndHasDepthInSubtree(B, false, false);

        // Then add CTx3D.
        SetTransform3DOnElement(B, cTx3D1);

        // Check Intermediate state.
        VerifyDepthAndHasDepthInSubtree(R, false, true);
        VerifyDepthAndHasDepthInSubtree(A, true, false);
        VerifyDepthAndHasDepthInSubtree(B, false, false);

        // Add depth to existing CTx3D
        cTx3D1->m_rRotationY = 20; // Depth transform
        B->UpdateHas3DDepth();

        // Verify final state.
        VerifyDepthAndHasDepthInSubtree(R, false, true);
        VerifyDepthAndHasDepthInSubtree(A, true, true);
        VerifyDepthAndHasDepthInSubtree(B, true, false);
    }

    {
        // TESTCASE: Add CTx3D first.
        // Intermediate state:
        // R: HasDepth = false, HasDepthInSubtree = true
        // A: HasDepth = false, HasDepthInSubtree = true
        // B: HasDepth = true, HasDepthInSubtree = false

        auto R = make_xref<CUIElement>();
        auto A = make_xref<CUIElement>();
        auto B = make_xref<CUIElement>();

        VERIFY_SUCCEEDED(R->AddChild(A));
        VERIFY_SUCCEEDED(A->AddChild(B));

        auto pTx3D = make_xref<CPerspectiveTransform3D>();
        auto cTx3D1 = make_xref<CCompositeTransform3D>();
        cTx3D1->m_rRotationY = 20; // Depth transform

        // Add CTx3D.
        SetTransform3DOnElement(B, cTx3D1);

        // Check Intermediate state.
        VerifyDepthAndHasDepthInSubtree(R, false, true);
        VerifyDepthAndHasDepthInSubtree(A, false, true);
        VerifyDepthAndHasDepthInSubtree(B, true, false);

        // Then add PTx3D.
        SetTransform3DOnElement(A, pTx3D);

        // Verify final state.
        VerifyDepthAndHasDepthInSubtree(R, false, true);
        VerifyDepthAndHasDepthInSubtree(A, true, true);
        VerifyDepthAndHasDepthInSubtree(B, true, false);
    }

    {
        // TESTCASE: Add CTx3D without depth first, then add depth, then add Perspective.
        // Intermediate state:
        // R: HasDepth = false, HasDepthInSubtree = true
        // A: HasDepth = false, HasDepthInSubtree = true
        // B: HasDepth = true, HasDepthInSubtree = false

        auto R = make_xref<CUIElement>();
        auto A = make_xref<CUIElement>();
        auto B = make_xref<CUIElement>();

        VERIFY_SUCCEEDED(R->AddChild(A));
        VERIFY_SUCCEEDED(A->AddChild(B));

        auto pTx3D = make_xref<CPerspectiveTransform3D>();
        auto cTx3D1 = make_xref<CCompositeTransform3D>();

        // Add CTx3D without depth
        SetTransform3DOnElement(B, cTx3D1);

        // Verify nothing has depth yet.
        VerifyDepthAndHasDepthInSubtree(R, false, false);
        VerifyDepthAndHasDepthInSubtree(A, false, false);
        VerifyDepthAndHasDepthInSubtree(B, false, false);

        // Add depth
        cTx3D1->m_rRotationY = 20; // Depth transform
        B->UpdateHas3DDepth();

        // Check Intermediate state.
        VerifyDepthAndHasDepthInSubtree(R, false, true);
        VerifyDepthAndHasDepthInSubtree(A, false, true);
        VerifyDepthAndHasDepthInSubtree(B, true, false);

        // Then add PTx3D.
        SetTransform3DOnElement(A, pTx3D);

        // Verify final state.
        VerifyDepthAndHasDepthInSubtree(R, false, true);
        VerifyDepthAndHasDepthInSubtree(A, true, true);
        VerifyDepthAndHasDepthInSubtree(B, true, false);
    }
}

void UIElementTransform3DFlagsUnitTests::ValidateHasDepthInSubtreeSmallAddNoDepth()
{
    // Simple tree with three elements:
    //  (R)
    //   |
    //  (A)  {PTx3D}
    //   |
    //  (B)  {CTx3D without Depth}
    //
    // Intermediate state based on if PTx3D or CTx3D added first.
    //
    // End state:
    // R: HasDepth = false, HasDepthInSubtree = true
    // A: HasDepth = true, HasDepthInSubtree = false
    // B: HasDepth = false, HasDepthInSubtree = false

    {
        // TESTCASE: Add PTx3D first.
        // Intermediate state:
        // R: HasDepth = false, HasDepthInSubtree = true
        // A: HasDepth = true, HasDepthInSubtree = false
        // B: HasDepth = false, HasDepthInSubtree = false
        auto R = make_xref<CUIElement>();
        auto A = make_xref<CUIElement>();
        auto B = make_xref<CUIElement>();

        VERIFY_SUCCEEDED(R->AddChild(A));
        VERIFY_SUCCEEDED(A->AddChild(B));

        auto pTx3D = make_xref<CPerspectiveTransform3D>();
        auto cTx3D1 = make_xref<CCompositeTransform3D>();
        cTx3D1->m_rTranslateY = 20; // No depth transform

        // Add perspective.
        SetTransform3DOnElement(A, pTx3D);

        // Check Intermediate state.
        VerifyDepthAndHasDepthInSubtree(R, false, true);
        VerifyDepthAndHasDepthInSubtree(A, true, false);
        VerifyDepthAndHasDepthInSubtree(B, false, false);

        // Then add CTx3D1.
        SetTransform3DOnElement(B, cTx3D1);

        // Verify final state.
        VerifyDepthAndHasDepthInSubtree(R, false, true);
        VerifyDepthAndHasDepthInSubtree(A, true, false);
        VerifyDepthAndHasDepthInSubtree(B, false, false);
    }

    {
        // TESTCASE: Add CTx3D1 first.
        // Intermediate state:
        // R: HasDepth = false, HasDepthInSubtree = false
        // A: HasDepth = false, HasDepthInSubtree = false
        // B: HasDepth = false, HasDepthInSubtree = false

        auto R = make_xref<CUIElement>();
        auto A = make_xref<CUIElement>();
        auto B = make_xref<CUIElement>();

        VERIFY_SUCCEEDED(R->AddChild(A));
        VERIFY_SUCCEEDED(A->AddChild(B));

        auto pTx3D = make_xref<CPerspectiveTransform3D>();
        auto cTx3D1 = make_xref<CCompositeTransform3D>();
        cTx3D1->m_rTranslateY = 20; // No depth transform

        // Add CTx3D1.
        SetTransform3DOnElement(B, cTx3D1);

        // Check Intermediate state.
        VerifyDepthAndHasDepthInSubtree(R, false, false);
        VerifyDepthAndHasDepthInSubtree(A, false, false);
        VerifyDepthAndHasDepthInSubtree(B, false, false);

        // Then add PTx3D.
        SetTransform3DOnElement(A, pTx3D);

        // Verify final state.
        VerifyDepthAndHasDepthInSubtree(R, false, true);
        VerifyDepthAndHasDepthInSubtree(A, true, false);
        VerifyDepthAndHasDepthInSubtree(B, false, false);
    }
}

void UIElementTransform3DFlagsUnitTests::ValidateHasDepthInSubtreeTallAdd()
{
    //  (R)
    //   |
    //  (A)  {PTx3D}
    //   |
    //  (X)
    //   |
    //  (Y)
    //   |
    //  (B)  {CTx3D with Depth}
    //   |
    //  (Z)
    //
    // Intermediate state based on if PTx3D or CTx3D added first.
    //
    // End state:
    // R: HasDepth = false, HasDepthInSubtree = true
    // A: HasDepth = true, HasDepthInSubtree = true
    // X: HasDepth = false, HasDepthInSubtree = true
    // Y: HasDepth = false, HasDepthInSubtree = true
    // B: HasDepth = true, HasDepthInSubtree = false
    // Z: HasDepth = false, HasDepthInSubtree = false

    {
        // TESTCASE: Add PTx3D first.
        // Intermediate state:
        // R: HasDepth = false, HasDepthInSubtree = true
        // A: HasDepth = true, HasDepthInSubtree = false
        // X: HasDepth = false, HasDepthInSubtree = false
        // Y: HasDepth = false, HasDepthInSubtree = false
        // B: HasDepth = false, HasDepthInSubtree = false
        // Z: HasDepth = false, HasDepthInSubtree = false
        auto R = make_xref<CUIElement>();
        auto A = make_xref<CUIElement>();
        auto B = make_xref<CUIElement>();
        auto X = make_xref<CUIElement>();
        auto Y = make_xref<CUIElement>();
        auto Z = make_xref<CUIElement>();

        VERIFY_SUCCEEDED(R->AddChild(A));
        VERIFY_SUCCEEDED(A->AddChild(X));
        VERIFY_SUCCEEDED(X->AddChild(Y));
        VERIFY_SUCCEEDED(Y->AddChild(B));
        VERIFY_SUCCEEDED(B->AddChild(Z));

        auto pTx3D = make_xref<CPerspectiveTransform3D>();
        auto cTx3D1 = make_xref<CCompositeTransform3D>();
        cTx3D1->m_rRotationY = 20; // Depth transform

        // Add perspective.
        SetTransform3DOnElement(A, pTx3D);

        // Check Intermediate state.
        VerifyDepthAndHasDepthInSubtree(R, false, true);
        VerifyDepthAndHasDepthInSubtree(A, true, false);
        VerifyDepthAndHasDepthInSubtree(X, false, false);
        VerifyDepthAndHasDepthInSubtree(Y, false, false);
        VerifyDepthAndHasDepthInSubtree(B, false, false);
        VerifyDepthAndHasDepthInSubtree(Z, false, false);

        // Then add CTx3D1.
        SetTransform3DOnElement(B, cTx3D1);

        // Verify final state.
        VerifyDepthAndHasDepthInSubtree(R, false, true);
        VerifyDepthAndHasDepthInSubtree(A, true, true);
        VerifyDepthAndHasDepthInSubtree(X, false, true);
        VerifyDepthAndHasDepthInSubtree(Y, false, true);
        VerifyDepthAndHasDepthInSubtree(B, true, false);
        VerifyDepthAndHasDepthInSubtree(Z, false, false);
    }

    {
        // TESTCASE: Add CTx3D1 first.
        // Intermediate state:
        // R: HasDepth = false, HasDepthInSubtree = true
        // A: HasDepth = false, HasDepthInSubtree = true
        // X: HasDepth = false, HasDepthInSubtree = true
        // Y: HasDepth = false, HasDepthInSubtree = true
        // B: HasDepth = true, HasDepthInSubtree = false
        // Z: HasDepth = false, HasDepthInSubtree = false

        auto R = make_xref<CUIElement>();
        auto A = make_xref<CUIElement>();
        auto B = make_xref<CUIElement>();
        auto X = make_xref<CUIElement>();
        auto Y = make_xref<CUIElement>();
        auto Z = make_xref<CUIElement>();

        VERIFY_SUCCEEDED(R->AddChild(A));
        VERIFY_SUCCEEDED(A->AddChild(X));
        VERIFY_SUCCEEDED(X->AddChild(Y));
        VERIFY_SUCCEEDED(Y->AddChild(B));
        VERIFY_SUCCEEDED(B->AddChild(Z));

        auto pTx3D = make_xref<CPerspectiveTransform3D>();
        auto cTx3D1 = make_xref<CCompositeTransform3D>();
        cTx3D1->m_rRotationY = 20; // Depth transform

        // Add CTx3D1.
        SetTransform3DOnElement(B, cTx3D1);

        // Check Intermediate state.
        VerifyDepthAndHasDepthInSubtree(R, false, true);
        VerifyDepthAndHasDepthInSubtree(A, false, true);
        VerifyDepthAndHasDepthInSubtree(X, false, true);
        VerifyDepthAndHasDepthInSubtree(Y, false, true);
        VerifyDepthAndHasDepthInSubtree(B, true, false);
        VerifyDepthAndHasDepthInSubtree(Z, false, false);

        // Then add PTx3D.
        SetTransform3DOnElement(A, pTx3D);

        // Verify final state.
        VerifyDepthAndHasDepthInSubtree(R, false, true);
        VerifyDepthAndHasDepthInSubtree(A, true, true);
        VerifyDepthAndHasDepthInSubtree(X, false, true);
        VerifyDepthAndHasDepthInSubtree(Y, false, true);
        VerifyDepthAndHasDepthInSubtree(B, true, false);
        VerifyDepthAndHasDepthInSubtree(Z, false, false);
    }
}

void UIElementTransform3DFlagsUnitTests::ValidateHasDepthInSubtreeAlternatingAdd()
{
    //  (R)
    //   |
    //  (A)  {PTx3D}
    //   |
    //  (B)  {CTx3D with Depth}
    //   |
    //  (C)  {PTx3D}
    //   |
    //  (D)  {CTx3D with Depth}
    //
    // Intermediate state based on if PTx3D or CTx3D added first, assumed tested by other cases.
    //
    // End state:
    // R: HasDepth = false, HasDepthInSubtree = true
    // A: HasDepth = true, HasDepthInSubtree = true
    // B: HasDepth = true, HasDepthInSubtree = true
    // C: HasDepth = true, HasDepthInSubtree = true
    // D: HasDepth = true, HasDepthInSubtree = false

    {
        // TESTCASE: Add in forward order
        auto R = make_xref<CUIElement>();
        auto A = make_xref<CUIElement>();
        auto B = make_xref<CUIElement>();
        auto C = make_xref<CUIElement>();
        auto D = make_xref<CUIElement>();

        VERIFY_SUCCEEDED(R->AddChild(A));
        VERIFY_SUCCEEDED(A->AddChild(B));
        VERIFY_SUCCEEDED(B->AddChild(C));
        VERIFY_SUCCEEDED(C->AddChild(D));

        auto pTx3D1 = make_xref<CPerspectiveTransform3D>();
        auto cTx3D1 = make_xref<CCompositeTransform3D>();
        auto pTx3D2 = make_xref<CPerspectiveTransform3D>();
        auto cTx3D2 = make_xref<CCompositeTransform3D>();

        cTx3D1->m_rRotationY = 20; // Depth transform
        cTx3D2->m_rRotationY = 30; // Depth transform

        // Add perspective.
        SetTransform3DOnElement(A, pTx3D1);

        // Then add CTx3D.
        SetTransform3DOnElement(B, cTx3D1);

        // Add the second perspective.
        SetTransform3DOnElement(C, pTx3D2);

        // Then add the secondCTx3D.
        SetTransform3DOnElement(D, cTx3D2);

        VerifyDepthAndHasDepthInSubtree(R, false, true);
        VerifyDepthAndHasDepthInSubtree(A, true, true);
        VerifyDepthAndHasDepthInSubtree(B, true, true);
        VerifyDepthAndHasDepthInSubtree(C, true, true);
        VerifyDepthAndHasDepthInSubtree(D, true, false);
    }

    {
        // TESTCASE: Add in backward order
        auto R = make_xref<CUIElement>();
        auto A = make_xref<CUIElement>();
        auto B = make_xref<CUIElement>();
        auto C = make_xref<CUIElement>();
        auto D = make_xref<CUIElement>();

        VERIFY_SUCCEEDED(R->AddChild(A.get()));
        VERIFY_SUCCEEDED(A->AddChild(B.get()));
        VERIFY_SUCCEEDED(B->AddChild(C.get()));
        VERIFY_SUCCEEDED(C->AddChild(D.get()));

        auto pTx3D1 = make_xref<CPerspectiveTransform3D>();
        auto cTx3D1 = make_xref<CCompositeTransform3D>();
        auto pTx3D2 = make_xref<CPerspectiveTransform3D>();
        auto cTx3D2 = make_xref<CCompositeTransform3D>();
        cTx3D1->m_rRotationY = 20; // Depth transform
        cTx3D2->m_rRotationY = 30; // Depth transform

        // Add the last CTx3D
        SetTransform3DOnElement(D.get(), cTx3D2);

        // Add the second perspective.
        SetTransform3DOnElement(C.get(), pTx3D2);

        // Add the top CTx3D.
        SetTransform3DOnElement(B.get(), cTx3D1);

        // Add the first perspective.
        SetTransform3DOnElement(A.get(), pTx3D1);

        VerifyDepthAndHasDepthInSubtree(R.get(), false, true);
        VerifyDepthAndHasDepthInSubtree(A.get(), true, true);
        VerifyDepthAndHasDepthInSubtree(B.get(), true, true);
        VerifyDepthAndHasDepthInSubtree(C.get(), true, true);
        VerifyDepthAndHasDepthInSubtree(D.get(), true, false);
    }

    {
        // TESTCASE: B, D, A, C
        auto R = make_xref<CUIElement>();
        auto A = make_xref<CUIElement>();
        auto B = make_xref<CUIElement>();
        auto C = make_xref<CUIElement>();
        auto D = make_xref<CUIElement>();

        VERIFY_SUCCEEDED(R->AddChild(A.get()));
        VERIFY_SUCCEEDED(A->AddChild(B.get()));
        VERIFY_SUCCEEDED(B->AddChild(C.get()));
        VERIFY_SUCCEEDED(C->AddChild(D.get()));

        auto pTx3D1 = make_xref<CPerspectiveTransform3D>();
        auto cTx3D1 = make_xref<CCompositeTransform3D>();
        auto pTx3D2 = make_xref<CPerspectiveTransform3D>();
        auto cTx3D2 = make_xref<CCompositeTransform3D>();
        cTx3D1->m_rRotationY = 20; // Depth transform
        cTx3D2->m_rRotationY = 30; // Depth transform

        // Add the top CTx3D.
        SetTransform3DOnElement(B.get(), cTx3D1);

        VerifyDepthAndHasDepthInSubtree(R.get(), false, true);
        VerifyDepthAndHasDepthInSubtree(A.get(), false, true);
        VerifyDepthAndHasDepthInSubtree(B.get(), true, false);
        VerifyDepthAndHasDepthInSubtree(C.get(), false, false);
        VerifyDepthAndHasDepthInSubtree(D.get(), false, false);

        // Add the last CTx3D
        SetTransform3DOnElement(D.get(), cTx3D2);

        VerifyDepthAndHasDepthInSubtree(R.get(), false, true);
        VerifyDepthAndHasDepthInSubtree(A.get(), false, true);
        VerifyDepthAndHasDepthInSubtree(B.get(), true, true);
        VerifyDepthAndHasDepthInSubtree(C.get(), false, true);
        VerifyDepthAndHasDepthInSubtree(D.get(), true, false);

        // Add the first perspective.
        SetTransform3DOnElement(A.get(), pTx3D1);

        VerifyDepthAndHasDepthInSubtree(R.get(), false, true);
        VerifyDepthAndHasDepthInSubtree(A.get(), true, true);
        VerifyDepthAndHasDepthInSubtree(B.get(), true, true);
        VerifyDepthAndHasDepthInSubtree(C.get(), false, true);
        VerifyDepthAndHasDepthInSubtree(D.get(), true, false);

        // Add the second perspective.
        SetTransform3DOnElement(C.get(), pTx3D2);

        // Check final state
        VerifyDepthAndHasDepthInSubtree(R.get(), false, true);
        VerifyDepthAndHasDepthInSubtree(A.get(), true, true);
        VerifyDepthAndHasDepthInSubtree(B.get(), true, true);
        VerifyDepthAndHasDepthInSubtree(C.get(), true, true);
        VerifyDepthAndHasDepthInSubtree(D.get(), true, false);
    }
}

void UIElementTransform3DFlagsUnitTests::ValidateHasDepthInSubtreeSiblingsAdd()
{
    //  (R)
    //   |
    //  (A)  {PTx3D}
    //   |
    //  (B)
    //   | \
    //  (C)(D) {CTx3D with Depth on D}
    //
    // Intermediate state based on if PTx3D or CTx3D added first, assumed tested by other cases.
    //
    // End state:
    // R: HasDepth = false, HasDepthInSubtree = true
    // A: HasDepth = true, HasDepthInSubtree = true
    // B: HasDepth = false, HasDepthInSubtree = true
    // C: HasDepth = false, HasDepthInSubtree = false
    // D: HasDepth = true, HasDepthInSubtree = false

    {
        // TESTCASE: Add PTx3D first, then CTx3D
        auto R = make_xref<CUIElement>();
        auto A = make_xref<CUIElement>();
        auto B = make_xref<CUIElement>();
        auto C = make_xref<CUIElement>();
        auto D = make_xref<CUIElement>();

        VERIFY_SUCCEEDED(R->AddChild(A));
        VERIFY_SUCCEEDED(A->AddChild(B));
        VERIFY_SUCCEEDED(B->AddChild(C));
        VERIFY_SUCCEEDED(B->AddChild(D));

        auto pTx3D = make_xref<CPerspectiveTransform3D>();
        auto cTx3D1 = make_xref<CCompositeTransform3D>();
        cTx3D1->m_rRotationY = 20; // Depth transform

        // Add perspective.
        SetTransform3DOnElement(A, pTx3D);

        // Then add CTx3D1.
        SetTransform3DOnElement(D, cTx3D1);

        VerifyDepthAndHasDepthInSubtree(R, false, true);
        VerifyDepthAndHasDepthInSubtree(A, true, true);
        VerifyDepthAndHasDepthInSubtree(B, false, true);
        VerifyDepthAndHasDepthInSubtree(C, false, false);
        VerifyDepthAndHasDepthInSubtree(D, true, false);
    }

    {
        // TESTCASE: Add CTx3D1 first, then PTx3D
        auto R = make_xref<CUIElement>();
        auto A = make_xref<CUIElement>();
        auto B = make_xref<CUIElement>();
        auto C = make_xref<CUIElement>();
        auto D = make_xref<CUIElement>();

        VERIFY_SUCCEEDED(R->AddChild(A));
        VERIFY_SUCCEEDED(A->AddChild(B));
        VERIFY_SUCCEEDED(B->AddChild(C));
        VERIFY_SUCCEEDED(B->AddChild(D));

        auto pTx3D = make_xref<CPerspectiveTransform3D>();
        auto cTx3D1 = make_xref<CCompositeTransform3D>();
        cTx3D1->m_rRotationY = 20; // Depth transform

        // Then add CTx3D1.
        SetTransform3DOnElement(D, cTx3D1);

        // Add perspective.
        SetTransform3DOnElement(A, pTx3D);

        VerifyDepthAndHasDepthInSubtree(R, false, true);
        VerifyDepthAndHasDepthInSubtree(A, true, true);
        VerifyDepthAndHasDepthInSubtree(B, false, true);
        VerifyDepthAndHasDepthInSubtree(C, false, false);
        VerifyDepthAndHasDepthInSubtree(D, true, false);
    }

    {
        // TESTCASE: Add CTx3D1 without depth first, then PTx3D, then add depth to CTx3D1
        auto R = make_xref<CUIElement>();
        auto A = make_xref<CUIElement>();
        auto B = make_xref<CUIElement>();
        auto C = make_xref<CUIElement>();
        auto D = make_xref<CUIElement>();

        VERIFY_SUCCEEDED(R->AddChild(A));
        VERIFY_SUCCEEDED(A->AddChild(B));
        VERIFY_SUCCEEDED(B->AddChild(C));
        VERIFY_SUCCEEDED(B->AddChild(D));

        auto pTx3D = make_xref<CPerspectiveTransform3D>();
        auto cTx3D1 = make_xref<CCompositeTransform3D>();
        cTx3D1->m_rRotationY = 0; // No depth transform

        // Then add CTx3D1.
        SetTransform3DOnElement(D, cTx3D1);

        // Add perspective.
        SetTransform3DOnElement(A, pTx3D);

        VerifyDepthAndHasDepthInSubtree(R, false, true);
        VerifyDepthAndHasDepthInSubtree(A, true, false);
        VerifyDepthAndHasDepthInSubtree(B, false, false);
        VerifyDepthAndHasDepthInSubtree(C, false, false);
        VerifyDepthAndHasDepthInSubtree(D, false, false);

        cTx3D1->m_rRotationY = 20; // Depth transform
        D->UpdateHas3DDepth();

        VerifyDepthAndHasDepthInSubtree(R, false, true);
        VerifyDepthAndHasDepthInSubtree(A, true, true);
        VerifyDepthAndHasDepthInSubtree(B, false, true);
        VerifyDepthAndHasDepthInSubtree(C, false, false);
        VerifyDepthAndHasDepthInSubtree(D, true, false);
    }

    {
        // TESTCASE: Add PTx3D first, then CTx3D on C, then on D
        auto R = make_xref<CUIElement>();
        auto A = make_xref<CUIElement>();
        auto B = make_xref<CUIElement>();
        auto C = make_xref<CUIElement>();
        auto D = make_xref<CUIElement>();

        VERIFY_SUCCEEDED(R->AddChild(A));
        VERIFY_SUCCEEDED(A->AddChild(B));
        VERIFY_SUCCEEDED(B->AddChild(C));
        VERIFY_SUCCEEDED(B->AddChild(D));

        auto pTx3D = make_xref<CPerspectiveTransform3D>();
        auto cTx3D1 = make_xref<CCompositeTransform3D>();
        cTx3D1->m_rRotationY = 20; // Depth transform
        auto cTx3D2 = make_xref<CCompositeTransform3D>();
        cTx3D2->m_rRotationY = 30; // Depth transform

        // Add perspective.
        SetTransform3DOnElement(A, pTx3D);

        // Then add CTx3D to C.
        SetTransform3DOnElement(C, cTx3D1);

        // Check intermediate state
        VerifyDepthAndHasDepthInSubtree(R, false, true);
        VerifyDepthAndHasDepthInSubtree(A, true, true);
        VerifyDepthAndHasDepthInSubtree(B, false, true);
        VerifyDepthAndHasDepthInSubtree(C, true, false);
        VerifyDepthAndHasDepthInSubtree(D, false, false);

        // Then add CTx3D to D.
        SetTransform3DOnElement(D, cTx3D2);

        // Check final state
        VerifyDepthAndHasDepthInSubtree(R, false, true);
        VerifyDepthAndHasDepthInSubtree(A, true, true);
        VerifyDepthAndHasDepthInSubtree(B, false, true);
        VerifyDepthAndHasDepthInSubtree(C, true, false);
        VerifyDepthAndHasDepthInSubtree(D, true, false);
    }
}

void UIElementTransform3DFlagsUnitTests::ValidateHasDepthInSubtreeAlternatingDepthAdd()
{
    //  (R)
    //   |
    //  (A)  {PTx3D}
    //   |
    //  (B)  {CTx3D with Depth}
    //   |
    //  (C)  {CTx3D without Depth}
    //   |
    //  (D)
    //
    // Intermediate state based on if PTx3D or CTx3D added first, assumed tested by other cases.
    //
    // End state:
    // R: HasDepth = false, HasDepthInSubtree = true
    // A: HasDepth = true, HasDepthInSubtree = true
    // B: HasDepth = true, HasDepthInSubtree = false
    // C: HasDepth = false, HasDepthInSubtree = false
    // D: HasDepth = false, HasDepthInSubtree = false

    {
        // TESTCASE: Add in forward order
        auto R = make_xref<CUIElement>();
        auto A = make_xref<CUIElement>();
        auto B = make_xref<CUIElement>();
        auto C = make_xref<CUIElement>();
        auto D = make_xref<CUIElement>();

        VERIFY_SUCCEEDED(R->AddChild(A));
        VERIFY_SUCCEEDED(A->AddChild(B));
        VERIFY_SUCCEEDED(B->AddChild(C));
        VERIFY_SUCCEEDED(C->AddChild(D));

        auto pTx3D = make_xref<CPerspectiveTransform3D>();
        auto cTx3D1 = make_xref<CCompositeTransform3D>();
        auto cTx3D2 = make_xref<CCompositeTransform3D>();
        cTx3D1->m_rRotationY = 20; // Depth transform
        cTx3D2->m_rScaleX = 0.5; // No depth transform

        // Add perspective.
        SetTransform3DOnElement(A, pTx3D);

        // Then add CTx3D.
        SetTransform3DOnElement(B, cTx3D1);

        // Add the second CTx3D.
        SetTransform3DOnElement(C, cTx3D2);

        VerifyDepthAndHasDepthInSubtree(R, false, true);
        VerifyDepthAndHasDepthInSubtree(A, true, true);
        VerifyDepthAndHasDepthInSubtree(B, true, false);
        VerifyDepthAndHasDepthInSubtree(C, false, false);
        VerifyDepthAndHasDepthInSubtree(D, false, false);
    }

    {
        // TESTCASE: Include depth on D
        // R: HasDepth = false, HasDepthInSubtree = false
        // A: HasDepth = false, HasDepthInSubtree = true
        // B: HasDepth = true, HasDepthInSubtree = true
        // C: HasDepth = false, HasDepthInSubtree = true
        // D: HasDepth = true, HasDepthInSubtree = false
        auto R = make_xref<CUIElement>();
        auto A = make_xref<CUIElement>();
        auto B = make_xref<CUIElement>();
        auto C = make_xref<CUIElement>();
        auto D = make_xref<CUIElement>();

        VERIFY_SUCCEEDED(R->AddChild(A));
        VERIFY_SUCCEEDED(A->AddChild(B));
        VERIFY_SUCCEEDED(B->AddChild(C));
        VERIFY_SUCCEEDED(C->AddChild(D));

        auto pTx3D = make_xref<CPerspectiveTransform3D>();
        auto cTx3D1 = make_xref<CCompositeTransform3D>();
        auto cTx3D2 = make_xref<CCompositeTransform3D>();
        auto cTx3D3 = make_xref<CCompositeTransform3D>();
        cTx3D1->m_rRotationY = 20; // Depth transform
        cTx3D2->m_rScaleX = 0.5; // No depth transform
        cTx3D3->m_rRotationY = 30; // Depth transform

        // Add perspective.
        SetTransform3DOnElement(A, pTx3D);

        // Then add CTx3D.
        SetTransform3DOnElement(B, cTx3D1);

        // Add the second CTx3D.
        SetTransform3DOnElement(C, cTx3D2);

        // Add the second CTx3D.
        SetTransform3DOnElement(D, cTx3D3);

        VerifyDepthAndHasDepthInSubtree(R, false, true);
        VerifyDepthAndHasDepthInSubtree(A, true, true);
        VerifyDepthAndHasDepthInSubtree(B, true, true);
        VerifyDepthAndHasDepthInSubtree(C, false, true);
        VerifyDepthAndHasDepthInSubtree(D, true, false);
    }
}

void UIElementTransform3DFlagsUnitTests::ValidateHasDepthInSubtreeSmallRemoveDepth()
{
    // Simple tree with three elements:
    //  (R)
    //   |
    //  (A)  {PTx3D}
    //   |
    //  (B)  {CTx3D with Depth}

    MockDependencyProperty transform3DProperty(KnownPropertyIndex::UIElement_Transform3D);

    {
        // TESTCASE: Remove depth on CTx3D, removal base case
        auto R = make_xref<CUIElement>();
        auto A = make_xref<CUIElement>();
        auto B = make_xref<CUIElement>();

        VERIFY_SUCCEEDED(R->AddChild(A));
        VERIFY_SUCCEEDED(A->AddChild(B));

        auto pTx3D = make_xref<CPerspectiveTransform3D>();
        auto cTx3D1 = make_xref<CCompositeTransform3D>();
        cTx3D1->m_rRotationY = 20; // Depth transform

        // Add perspective.
        SetTransform3DOnElement(A, pTx3D);

        // Then add CTx3D1.
        SetTransform3DOnElement(B, cTx3D1);

        // Ensure state is consistent before removing anything
        VerifyDepthAndHasDepthInSubtree(R, false, true);
        VerifyDepthAndHasDepthInSubtree(A, true, true);
        VerifyDepthAndHasDepthInSubtree(B, true, false);

        cTx3D1->m_rRotationY = 0;
        B->UpdateHas3DDepth();
        VERIFY_IS_FALSE(B->Has3DDepth());

        // Depth has been removed from the tree, so everything is false.
        VerifyDepthAndHasDepthInSubtree(R, false, true);
        VerifyDepthAndHasDepthInSubtree(A, true, false);
        VerifyDepthAndHasDepthInSubtree(B, false, false);

        // Remove the PerspectiveTransform3D.
        VERIFY_SUCCEEDED(A->ClearValue(&transform3DProperty));
        A->UpdateHas3DDepth();
        VerifyDepthAndHasDepthInSubtree(R, false, false);
        VerifyDepthAndHasDepthInSubtree(A, false, false);
        VerifyDepthAndHasDepthInSubtree(B, false, false);
    }

    {
        // TESTCASE: Remove CTx3D, removal base case
        auto R = make_xref<CUIElement>();
        auto A = make_xref<CUIElement>();
        auto B = make_xref<CUIElement>();

        VERIFY_SUCCEEDED(R->AddChild(A));
        VERIFY_SUCCEEDED(A->AddChild(B));

        xref_ptr<CPerspectiveTransform3D> pTx3D;
        pTx3D.attach(new CPerspectiveTransform3D);
        xref_ptr<CCompositeTransform3D> cTx3D1;
        cTx3D1.attach(new CCompositeTransform3D);
        cTx3D1->m_rRotationY = 20; // Depth transform

        // Add perspective.
        SetTransform3DOnElement(A, pTx3D);

        // Then add CTx3D1.
        SetTransform3DOnElement(B, cTx3D1);

        // Ensure state is consistent before removing anything
        VerifyDepthAndHasDepthInSubtree(R, false, true);
        VerifyDepthAndHasDepthInSubtree(A, true, true);
        VerifyDepthAndHasDepthInSubtree(B, true, false);

        // Remove the CTx3D on B.
        VERIFY_SUCCEEDED(B->ClearValue(&transform3DProperty));
        B->UpdateHas3DDepth();
        VERIFY_IS_FALSE(B->Has3DDepth());

        // Depth has been removed from the tree, so everything is false.
        VerifyDepthAndHasDepthInSubtree(R, false, true);
        VerifyDepthAndHasDepthInSubtree(A, true, false);
        VerifyDepthAndHasDepthInSubtree(B, false, false);

        // Remove the PerspectiveTransform3D.
        VERIFY_SUCCEEDED(A->ClearValue(&transform3DProperty));
        A->UpdateHas3DDepth();
        VerifyDepthAndHasDepthInSubtree(R, false, false);
        VerifyDepthAndHasDepthInSubtree(A, false, false);
        VerifyDepthAndHasDepthInSubtree(B, false, false);
    }

    //  (R)
    //   |
    //  (A)  {PTx3D}
    //   |
    //  (B)  {CTx3D with Depth}
    //   |
    //  (C)  {CTx3D with Depth}

    {
        // TESTCASE: Remove depth on CTx3D on B.
        auto R = make_xref<CUIElement>();
        auto A = make_xref<CUIElement>();
        auto B = make_xref<CUIElement>();
        auto C = make_xref<CUIElement>();

        VERIFY_SUCCEEDED(R->AddChild(A));
        VERIFY_SUCCEEDED(A->AddChild(B));
        VERIFY_SUCCEEDED(B->AddChild(C));

        auto pTx3D = make_xref<CPerspectiveTransform3D>();
        auto cTx3D1 = make_xref<CCompositeTransform3D>();
        cTx3D1->m_rRotationY = 20; // Depth transform
        auto cTx3D2 = make_xref<CCompositeTransform3D>();
        cTx3D2->m_rRotationY = 30; // Depth transform

        // Add perspective.
        SetTransform3DOnElement(A, pTx3D);

        // Then add CTx3D1.
        SetTransform3DOnElement(B, cTx3D1);

        // Then add CTx3D1.
        SetTransform3DOnElement(C, cTx3D2);

        // Ensure state is consistent before removing anything
        VerifyDepthAndHasDepthInSubtree(R, false, true);
        VerifyDepthAndHasDepthInSubtree(A, true, true);
        VerifyDepthAndHasDepthInSubtree(B, true, true);
        VerifyDepthAndHasDepthInSubtree(C, true, false);

        cTx3D1->m_rRotationY = 0;
        B->UpdateHas3DDepth();
        VERIFY_IS_FALSE(B->Has3DDepth());

        VerifyDepthAndHasDepthInSubtree(R, false, true);
        VerifyDepthAndHasDepthInSubtree(A, true, true);
        VerifyDepthAndHasDepthInSubtree(B, false, true);
        VerifyDepthAndHasDepthInSubtree(C, true, false);
    }

    {
        // TESTCASE: Remove CTx3D on B.
        auto R = make_xref<CUIElement>();
        auto A = make_xref<CUIElement>();
        auto B = make_xref<CUIElement>();
        auto C = make_xref<CUIElement>();

        VERIFY_SUCCEEDED(R->AddChild(A));
        VERIFY_SUCCEEDED(A->AddChild(B));
        VERIFY_SUCCEEDED(B->AddChild(C));

        auto pTx3D = make_xref<CPerspectiveTransform3D>();
        auto cTx3D1 = make_xref<CCompositeTransform3D>();
        cTx3D1->m_rRotationY = 20; // Depth transform
        auto cTx3D2 = make_xref<CCompositeTransform3D>();
        cTx3D2->m_rRotationY = 30; // Depth transform

        // Add perspective.
        SetTransform3DOnElement(A, pTx3D);

        // Then add CTx3D1.
        SetTransform3DOnElement(B, cTx3D1);

        // Then add CTx3D1.
        SetTransform3DOnElement(C, cTx3D2);

        // Ensure state is consistent before removing anything
        VerifyDepthAndHasDepthInSubtree(R, false, true);
        VerifyDepthAndHasDepthInSubtree(A, true, true);
        VerifyDepthAndHasDepthInSubtree(B, true, true);
        VerifyDepthAndHasDepthInSubtree(C, true, false);

        VERIFY_SUCCEEDED(B->ClearValue(&transform3DProperty));
        B->UpdateHas3DDepth();
        VERIFY_IS_FALSE(B->Has3DDepth());

        VerifyDepthAndHasDepthInSubtree(R, false, true);
        VerifyDepthAndHasDepthInSubtree(A, true, true);
        VerifyDepthAndHasDepthInSubtree(B, false, true);
        VerifyDepthAndHasDepthInSubtree(C, true, false);
    }

    {
        // TESTCASE: Remove depth on CTx3D on C.
        auto R = make_xref<CUIElement>();
        auto A = make_xref<CUIElement>();
        auto B = make_xref<CUIElement>();
        auto C = make_xref<CUIElement>();

        VERIFY_SUCCEEDED(R->AddChild(A));
        VERIFY_SUCCEEDED(A->AddChild(B));
        VERIFY_SUCCEEDED(B->AddChild(C));

        auto pTx3D = make_xref<CPerspectiveTransform3D>();
        auto cTx3D1 = make_xref<CCompositeTransform3D>();
        cTx3D1->m_rRotationY = 20; // Depth transform
        auto cTx3D2 = make_xref<CCompositeTransform3D>();
        cTx3D2->m_rRotationY = 30; // Depth transform

        // Add perspective.
        SetTransform3DOnElement(A, pTx3D);

        // Then add CTx3D1.
        SetTransform3DOnElement(B, cTx3D1);

        // Then add CTx3D1.
        SetTransform3DOnElement(C, cTx3D2);

        // Ensure state is consistent before removing anything
        VerifyDepthAndHasDepthInSubtree(R, false, true);
        VerifyDepthAndHasDepthInSubtree(A, true, true);
        VerifyDepthAndHasDepthInSubtree(B, true, true);
        VerifyDepthAndHasDepthInSubtree(C, true, false);

        cTx3D2->m_rRotationY = 0;
        C->UpdateHas3DDepth();
        VERIFY_IS_FALSE(C->Has3DDepth());

        VerifyDepthAndHasDepthInSubtree(R, false, true);
        VerifyDepthAndHasDepthInSubtree(A, true, true);
        VerifyDepthAndHasDepthInSubtree(B, true, false);
        VerifyDepthAndHasDepthInSubtree(C, false, false);
    }

    {
        // TESTCASE: Remove CTx3D on C.
        auto R = make_xref<CUIElement>();
        auto A = make_xref<CUIElement>();
        auto B = make_xref<CUIElement>();
        auto C = make_xref<CUIElement>();

        VERIFY_SUCCEEDED(R->AddChild(A));
        VERIFY_SUCCEEDED(A->AddChild(B));
        VERIFY_SUCCEEDED(B->AddChild(C));

        auto pTx3D = make_xref<CPerspectiveTransform3D>();
        auto cTx3D1 = make_xref<CCompositeTransform3D>();
        cTx3D1->m_rRotationY = 20; // Depth transform
        auto cTx3D2 = make_xref<CCompositeTransform3D>();
        cTx3D2->m_rRotationY = 30; // Depth transform

        // Add perspective.
        SetTransform3DOnElement(A, pTx3D);

        // Then add CTx3D1.
        SetTransform3DOnElement(B, cTx3D1);

        // Then add CTx3D1.
        SetTransform3DOnElement(C, cTx3D2);

        // Ensure state is consistent before removing anything
        VerifyDepthAndHasDepthInSubtree(R, false, true);
        VerifyDepthAndHasDepthInSubtree(A, true, true);
        VerifyDepthAndHasDepthInSubtree(B, true, true);
        VerifyDepthAndHasDepthInSubtree(C, true, false);

        VERIFY_SUCCEEDED(C->ClearValue(&transform3DProperty));
        C->UpdateHas3DDepth();
        VERIFY_IS_FALSE(C->Has3DDepth());

        VerifyDepthAndHasDepthInSubtree(R, false, true);
        VerifyDepthAndHasDepthInSubtree(A, true, true);
        VerifyDepthAndHasDepthInSubtree(B, true, false);
        VerifyDepthAndHasDepthInSubtree(C, false, false);
    }

    //  (R)
    //   |
    //  (A)  {PTx3D}
    //   |
    //  (B)  {CTx3D with Depth}
    //   |
    //  (C)  {CTx3D with Depth}
    //   |
    //  (D)  {CTx3D with Depth}

    {
        // TESTCASE: Remove depth on CTx3D on C.
        auto R = make_xref<CUIElement>();
        auto A = make_xref<CUIElement>();
        auto B = make_xref<CUIElement>();
        auto C = make_xref<CUIElement>();
        auto D = make_xref<CUIElement>();

        VERIFY_SUCCEEDED(R->AddChild(A));
        VERIFY_SUCCEEDED(A->AddChild(B));
        VERIFY_SUCCEEDED(B->AddChild(C));
        VERIFY_SUCCEEDED(C->AddChild(D));

        auto pTx3D = make_xref<CPerspectiveTransform3D>();
        auto cTx3D1 = make_xref<CCompositeTransform3D>();
        cTx3D1->m_rRotationY = 20; // Depth transform
        auto cTx3D2 = make_xref<CCompositeTransform3D>();
        cTx3D2->m_rRotationY = 30; // Depth transform
        auto cTx3D3 = make_xref<CCompositeTransform3D>();
        cTx3D3->m_rRotationY = 40; // Depth transform

        // Add perspective.
        SetTransform3DOnElement(A, pTx3D);

        // Then add CTx3D1.
        SetTransform3DOnElement(B, cTx3D1);

        // Then add CTx3D1.
        SetTransform3DOnElement(C, cTx3D2);

        // Then add CTx3D1.
        SetTransform3DOnElement(D, cTx3D3);

        // Ensure state is consistent before removing anything
        VerifyDepthAndHasDepthInSubtree(R, false, true);
        VerifyDepthAndHasDepthInSubtree(A, true, true);
        VerifyDepthAndHasDepthInSubtree(B, true, true);
        VerifyDepthAndHasDepthInSubtree(C, true, true);
        VerifyDepthAndHasDepthInSubtree(D, true, false);

        cTx3D2->m_rRotationY = 0;
        C->UpdateHas3DDepth();
        VERIFY_IS_FALSE(C->Has3DDepth());

        VerifyDepthAndHasDepthInSubtree(R, false, true);
        VerifyDepthAndHasDepthInSubtree(A, true, true);
        VerifyDepthAndHasDepthInSubtree(B, true, true);
        VerifyDepthAndHasDepthInSubtree(C, false, true);
        VerifyDepthAndHasDepthInSubtree(D, true, false);

        // Remove everything else.
        VERIFY_SUCCEEDED(A->ClearValue(&transform3DProperty));
        A->UpdateHas3DDepth();
        VERIFY_SUCCEEDED(B->ClearValue(&transform3DProperty));
        B->UpdateHas3DDepth();
        VERIFY_SUCCEEDED(D->ClearValue(&transform3DProperty));
        D->UpdateHas3DDepth();
        VerifyDepthAndHasDepthInSubtree(R, false, false);
        VerifyDepthAndHasDepthInSubtree(A, false, false);
        VerifyDepthAndHasDepthInSubtree(B, false, false);
        VerifyDepthAndHasDepthInSubtree(C, false, false);
        VerifyDepthAndHasDepthInSubtree(D, false, false);
    }

}

void UIElementTransform3DFlagsUnitTests::ValidateHasDepthInSubtreeTallRemoveDepth()
{
    //  (R)
    //   |
    //  (A)  {PTx3D}
    //   |
    //  (X)
    //   |
    //  (Y)
    //   |
    //  (B)  {CTx3D with Depth}
    //   |
    //  (Z)

    {
        auto R = make_xref<CUIElement>();
        auto A = make_xref<CUIElement>();
        auto B = make_xref<CUIElement>();
        auto X = make_xref<CUIElement>();
        auto Y = make_xref<CUIElement>();
        auto Z = make_xref<CUIElement>();

        VERIFY_SUCCEEDED(R->AddChild(A));
        VERIFY_SUCCEEDED(A->AddChild(X));
        VERIFY_SUCCEEDED(X->AddChild(Y));
        VERIFY_SUCCEEDED(Y->AddChild(B));
        VERIFY_SUCCEEDED(B->AddChild(Z));

        auto pTx3D = make_xref<CPerspectiveTransform3D>();
        auto cTx3D1 = make_xref<CCompositeTransform3D>();
        cTx3D1->m_rRotationY = 20; // Depth transform

        // Add perspective.
        SetTransform3DOnElement(A, pTx3D);

        // Then add CTx3D1.
        SetTransform3DOnElement(B, cTx3D1);

        // Verify state before removing
        VerifyDepthAndHasDepthInSubtree(R, false, true);
        VerifyDepthAndHasDepthInSubtree(A, true, true);
        VerifyDepthAndHasDepthInSubtree(X, false, true);
        VerifyDepthAndHasDepthInSubtree(Y, false, true);
        VerifyDepthAndHasDepthInSubtree(B, true, false);
        VerifyDepthAndHasDepthInSubtree(Z, false, false);

        cTx3D1->m_rRotationY = 0;
        B->UpdateHas3DDepth();
        VERIFY_IS_FALSE(B->Has3DDepth());

        // Verify nothing has depth
        VerifyDepthAndHasDepthInSubtree(R, false, true);
        VerifyDepthAndHasDepthInSubtree(A, true, false);
        VerifyDepthAndHasDepthInSubtree(X, false, false);
        VerifyDepthAndHasDepthInSubtree(Y, false, false);
        VerifyDepthAndHasDepthInSubtree(B, false, false);
        VerifyDepthAndHasDepthInSubtree(Z, false, false);
    }
}

void UIElementTransform3DFlagsUnitTests::ValidateHasDepthInSubtreeSiblingsRemoveDepth()
{
    //  (R)
    //   |
    //  (A)  {PTx3D}
    //   |
    //  (B)
    //   | \
    //  (C)(D) {CTx3D with Depth on D, or both}

    {
        // TESTCASE: Only D has depth, remove it.
        auto R = make_xref<CUIElement>();
        auto A = make_xref<CUIElement>();
        auto B = make_xref<CUIElement>();
        auto C = make_xref<CUIElement>();
        auto D = make_xref<CUIElement>();

        VERIFY_SUCCEEDED(R->AddChild(A));
        VERIFY_SUCCEEDED(A->AddChild(B));
        VERIFY_SUCCEEDED(B->AddChild(C));
        VERIFY_SUCCEEDED(B->AddChild(D));

        auto pTx3D = make_xref<CPerspectiveTransform3D>();
        auto cTx3D1 = make_xref<CCompositeTransform3D>();
        cTx3D1->m_rRotationY = 20; // Depth transform

        // Add perspective.
        SetTransform3DOnElement(A, pTx3D);

        // Then add CTx3D1.
        SetTransform3DOnElement(D, cTx3D1);

        // Verify state before removing
        VerifyDepthAndHasDepthInSubtree(R, false, true);
        VerifyDepthAndHasDepthInSubtree(A, true, true);
        VerifyDepthAndHasDepthInSubtree(B, false, true);
        VerifyDepthAndHasDepthInSubtree(C, false, false);
        VerifyDepthAndHasDepthInSubtree(D, true, false);

        cTx3D1->m_rRotationY = 0;
        D->UpdateHas3DDepth();
        VERIFY_IS_FALSE(D->Has3DDepth());

        // Verify nothing has depth
        VerifyDepthAndHasDepthInSubtree(R, false, true);
        VerifyDepthAndHasDepthInSubtree(A, true, false);
        VerifyDepthAndHasDepthInSubtree(B, false, false);
        VerifyDepthAndHasDepthInSubtree(C, false, false);
        VerifyDepthAndHasDepthInSubtree(D, false, false);
    }

    {
        // TESTCASE: Depth on C and D, remove depth on C, then on D
        auto R = make_xref<CUIElement>();
        auto A = make_xref<CUIElement>();
        auto B = make_xref<CUIElement>();
        auto C = make_xref<CUIElement>();
        auto D = make_xref<CUIElement>();

        VERIFY_SUCCEEDED(R->AddChild(A));
        VERIFY_SUCCEEDED(A->AddChild(B));
        VERIFY_SUCCEEDED(B->AddChild(C));
        VERIFY_SUCCEEDED(B->AddChild(D));

        auto pTx3D = make_xref<CPerspectiveTransform3D>();
        auto cTx3D1 = make_xref<CCompositeTransform3D>();
        cTx3D1->m_rRotationY = 20; // Depth transform
        auto cTx3D2 = make_xref<CCompositeTransform3D>();
        cTx3D2->m_rRotationY = 30; // Depth transform

        // Add perspective.
        SetTransform3DOnElement(A, pTx3D);

        // Then add CTx3D to C.
        SetTransform3DOnElement(C, cTx3D1);

        // Check intermediate state
        VerifyDepthAndHasDepthInSubtree(R, false, true);
        VerifyDepthAndHasDepthInSubtree(A, true, true);
        VerifyDepthAndHasDepthInSubtree(B, false, true);
        VerifyDepthAndHasDepthInSubtree(C, true, false);
        VerifyDepthAndHasDepthInSubtree(D, false, false);

        // Then add CTx3D to D.
        SetTransform3DOnElement(D, cTx3D2);

        // Check state before removing
        VerifyDepthAndHasDepthInSubtree(R, false, true);
        VerifyDepthAndHasDepthInSubtree(A, true, true);
        VerifyDepthAndHasDepthInSubtree(B, false, true);
        VerifyDepthAndHasDepthInSubtree(C, true, false);
        VerifyDepthAndHasDepthInSubtree(D, true, false);

        cTx3D1->m_rRotationY = 0;
        C->UpdateHas3DDepth();
        VERIFY_IS_FALSE(C->Has3DDepth());

        // Verify C has lost depth, otherwise nothing else changed (since an immediate sibling has depth)
        VerifyDepthAndHasDepthInSubtree(R, false, true);
        VerifyDepthAndHasDepthInSubtree(A, true, true);
        VerifyDepthAndHasDepthInSubtree(B, false, true);
        VerifyDepthAndHasDepthInSubtree(C, false, false);
        VerifyDepthAndHasDepthInSubtree(D, true, false);

        cTx3D2->m_rRotationY = 0;
        D->UpdateHas3DDepth();
        VERIFY_IS_FALSE(D->Has3DDepth());

        // Since both C and D have lost depth, nothing should have depth.
        VerifyDepthAndHasDepthInSubtree(R, false, true);
        VerifyDepthAndHasDepthInSubtree(A, true, false);
        VerifyDepthAndHasDepthInSubtree(B, false, false);
        VerifyDepthAndHasDepthInSubtree(C, false, false);
        VerifyDepthAndHasDepthInSubtree(D, false, false);
    }

    //  (R)
    //   |
    //  (A)  {PTx3D}
    //   |
    //  (B)
    //   | \
    //  (X)(D) {CTx3D with Depth on D}
    //   |
    //  (Y)
    //   |
    //  (C)    {CTx3D with Depth on C}

    {
        // TESTCASE: Depth on C and D, remove depth on C, then on D
        auto R = make_xref<CUIElement>();
        auto A = make_xref<CUIElement>();
        auto B = make_xref<CUIElement>();
        auto C = make_xref<CUIElement>();
        auto D = make_xref<CUIElement>();
        auto X = make_xref<CUIElement>();
        auto Y = make_xref<CUIElement>();

        VERIFY_SUCCEEDED(R->AddChild(A));
        VERIFY_SUCCEEDED(A->AddChild(B));
        VERIFY_SUCCEEDED(B->AddChild(X));
        VERIFY_SUCCEEDED(B->AddChild(D));
        VERIFY_SUCCEEDED(X->AddChild(Y));
        VERIFY_SUCCEEDED(Y->AddChild(C));

        auto pTx3D = make_xref<CPerspectiveTransform3D>();
        auto cTx3D1 = make_xref<CCompositeTransform3D>();
        cTx3D1->m_rRotationY = 20; // Depth transform
        auto cTx3D2 = make_xref<CCompositeTransform3D>();
        cTx3D2->m_rRotationY = 30; // Depth transform

        // Add perspective.
        SetTransform3DOnElement(A, pTx3D);

        // Then add CTx3D to C.
        SetTransform3DOnElement(C, cTx3D1);

        // Then add CTx3D to D.
        SetTransform3DOnElement(D, cTx3D2);

        // Check state before removing
        VerifyDepthAndHasDepthInSubtree(R, false, true);
        VerifyDepthAndHasDepthInSubtree(A, true, true);
        VerifyDepthAndHasDepthInSubtree(B, false, true);
        VerifyDepthAndHasDepthInSubtree(X, false, true);
        VerifyDepthAndHasDepthInSubtree(Y, false, true);
        VerifyDepthAndHasDepthInSubtree(C, true, false);
        VerifyDepthAndHasDepthInSubtree(D, true, false);

        cTx3D1->m_rRotationY = 0;
        C->UpdateHas3DDepth();
        VERIFY_IS_FALSE(C->Has3DDepth());

        // Verify C has lost depth, and thus so has X and Y in Subtree
        VerifyDepthAndHasDepthInSubtree(R, false, true);
        VerifyDepthAndHasDepthInSubtree(A, true, true);
        VerifyDepthAndHasDepthInSubtree(B, false, true);
        VerifyDepthAndHasDepthInSubtree(X, false, false);
        VerifyDepthAndHasDepthInSubtree(Y, false, false);
        VerifyDepthAndHasDepthInSubtree(C, false, false);
        VerifyDepthAndHasDepthInSubtree(D, true, false);

        cTx3D2->m_rRotationY = 0;
        D->UpdateHas3DDepth();
        VERIFY_IS_FALSE(D->Has3DDepth());

        // Since both C and D have lost depth, nothing should have depth in Subtree
        VerifyDepthAndHasDepthInSubtree(R, false, true);
        VerifyDepthAndHasDepthInSubtree(A, true, false);
        VerifyDepthAndHasDepthInSubtree(B, false, false);
        VerifyDepthAndHasDepthInSubtree(X, false, false);
        VerifyDepthAndHasDepthInSubtree(Y, false, false);
        VerifyDepthAndHasDepthInSubtree(C, false, false);
        VerifyDepthAndHasDepthInSubtree(D, false, false);
    }
}

void UIElementTransform3DFlagsUnitTests::SetTransform3DOnElement(CUIElement* element, CTransform3D* transform)
{
    CValue value;

    value.WrapObjectNoRef(transform);
    VERIFY_SUCCEEDED(element->SetValueByKnownIndex(KnownPropertyIndex::UIElement_Transform3D, value));
    element->UpdateHas3DDepth(); // Called any time Tx3D property updated.
}

void UIElementTransform3DFlagsUnitTests::VerifyDepthAndHasDepthInSubtree(CUIElement* element, bool expectedDepth, bool expectedHasDepthInSubtree)
{
    VERIFY_IS_TRUE(expectedDepth == element->Has3DDepth());
    VERIFY_IS_TRUE(expectedHasDepthInSubtree == !!element->Has3DDepthInSubtree());
}

} } } } } }
