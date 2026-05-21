// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>
#include "UIElement.h"
#include "Transform3D.h"

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Elements {

class UIElementTransform3DFlagsUnitTests : public WEX::TestClass<UIElementTransform3DFlagsUnitTests>
{
public:
    BEGIN_TEST_CLASS(UIElementTransform3DFlagsUnitTests)
        TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        TEST_CLASS_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
    END_TEST_CLASS()

    // Base case
    TEST_METHOD(ValidateHasDepthOnOneElement)

    // Add transforms to the tree
    TEST_METHOD(ValidateHasDepthInSubtreeSmallAdd)

    TEST_METHOD(ValidateHasDepthInSubtreeSmallAddNoDepth)

    TEST_METHOD(ValidateHasDepthInSubtreeTallAdd)

    TEST_METHOD(ValidateHasDepthInSubtreeAlternatingAdd)

    TEST_METHOD(ValidateHasDepthInSubtreeSiblingsAdd)

    TEST_METHOD(ValidateHasDepthInSubtreeAlternatingDepthAdd)

    // TODO: Remove transforms from the tree
    TEST_METHOD(ValidateHasDepthInSubtreeSmallRemoveDepth)

    TEST_METHOD(ValidateHasDepthInSubtreeTallRemoveDepth)

    TEST_METHOD(ValidateHasDepthInSubtreeSiblingsRemoveDepth)

private:
    void VerifyDepthAndHasDepthInSubtree(CUIElement* element, bool expectedDepth, bool expectedHasDepthInSubtree);
    void SetTransform3DOnElement(CUIElement* element, CTransform3D* transform);

};

} } } } } }
