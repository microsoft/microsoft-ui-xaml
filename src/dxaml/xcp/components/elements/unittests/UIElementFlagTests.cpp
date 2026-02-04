// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "XamlLogging.h"
#include "UIElementFlagTests.h"
#include "UIElement.h"
#include "LayoutTransitionElement.h"

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Elements {

void UIElementFlagTests::ForceNoCulling()
{
    auto R = make_xref<CUIElement>();
    auto P = make_xref<CUIElement>();
    auto A = make_xref<CUIElement>();
    auto B = make_xref<CUIElement>();
    VERIFY_SUCCEEDED(R->AddChild(P));
    VERIFY_SUCCEEDED(P->AddChild(A));
    VERIFY_SUCCEEDED(P->AddChild(B));

    LOG_OUTPUT(L"ForceNoCulling: ON for A");
    A->SetAndPropagateForceNoCulling(true);
    VERIFY_IS_TRUE(A->IsForceNoCulling());
    VERIFY_IS_TRUE(P->IsForceNoCulling());
    VERIFY_IS_TRUE(R->IsForceNoCulling());
    VERIFY_IS_FALSE(B->IsForceNoCulling());

    LOG_OUTPUT(L"ForceNoCulling: ON for B");
    B->SetAndPropagateForceNoCulling(true);
    VERIFY_IS_TRUE(A->IsForceNoCulling());
    VERIFY_IS_TRUE(P->IsForceNoCulling());
    VERIFY_IS_TRUE(R->IsForceNoCulling());
    VERIFY_IS_TRUE(B->IsForceNoCulling());

    LOG_OUTPUT(L"ForceNoCulling: OFF for A");
    A->SetAndPropagateForceNoCulling(false);
    VERIFY_IS_FALSE(A->IsForceNoCulling());
    VERIFY_IS_TRUE(P->IsForceNoCulling());
    VERIFY_IS_TRUE(R->IsForceNoCulling());
    VERIFY_IS_TRUE(B->IsForceNoCulling());

    LOG_OUTPUT(L"ForceNoCulling: OFF for B");
    B->SetAndPropagateForceNoCulling(false);
    VERIFY_IS_FALSE(A->IsForceNoCulling());
    VERIFY_IS_FALSE(P->IsForceNoCulling());
    VERIFY_IS_FALSE(R->IsForceNoCulling());
    VERIFY_IS_FALSE(B->IsForceNoCulling());
}

void UIElementFlagTests::ForceNoCullingLTE()
{
    auto R = make_xref<CUIElement>();
    auto P = make_xref<CUIElement>();
    auto A = make_xref<CUIElement>();
    xref_ptr<CLayoutTransitionElement> LTE;
    LTE.attach(new CLayoutTransitionElement(A, FALSE));
    VERIFY_SUCCEEDED(R->AddChild(P));
    VERIFY_SUCCEEDED(R->AddChild(LTE));
    VERIFY_SUCCEEDED(P->AddChild(A));
    VERIFY_SUCCEEDED(A->AddLayoutTransitionRenderer(LTE));

    LOG_OUTPUT(L"ForceNoCulling: ON for A");
    A->SetAndPropagateForceNoCulling(true);
    VERIFY_IS_TRUE(A->IsForceNoCulling());
    VERIFY_IS_TRUE(P->IsForceNoCulling());
    VERIFY_IS_TRUE(LTE->IsForceNoCulling());
    VERIFY_IS_TRUE(R->IsForceNoCulling());

    LOG_OUTPUT(L"ForceNoCulling: OFF for A");
    A->SetAndPropagateForceNoCulling(false);
    VERIFY_IS_FALSE(A->IsForceNoCulling());
    VERIFY_IS_FALSE(P->IsForceNoCulling());
    VERIFY_IS_FALSE(LTE->IsForceNoCulling());
    VERIFY_IS_FALSE(R->IsForceNoCulling());
}

}}}}}}