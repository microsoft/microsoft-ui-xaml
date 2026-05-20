// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "WexTestClass.h"

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { 
    namespace Xaml {
        class OcclusivityTesterUnitTests : public WEX::TestClass<OcclusivityTesterUnitTests>
        {
        public:
            BEGIN_TEST_CLASS(OcclusivityTesterUnitTests)
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_CLASS()

            BEGIN_TEST_METHOD(ElementWithSmallerBoundsIsOccluded)
                TEST_METHOD_PROPERTY(L"Description", L"A small element behind a large element is occluded")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ElementOutsideRootIsNotOccluded)
                TEST_METHOD_PROPERTY(L"Description", L"An element outside root viisual is not occluded")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ScrollBarShouldBeIgnoredWhenFindingOccludedElement)
                TEST_METHOD_PROPERTY(L"Description", L"Verify that we skip a scrollbar if it is part of the hit test results")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ScrollBarChildShouldBeIgnoredWhenFindingOccludedElement)
                TEST_METHOD_PROPERTY(L"Description", L"Verify that we walk up the tree checking if an element is a child of a scrollbar. If so, ignore the entire subtree")
            END_TEST_METHOD()    
        };
    }
}}}}
