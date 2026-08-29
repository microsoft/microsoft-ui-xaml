// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "WexTestClass.h"

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { 
    namespace Xaml { namespace Focus {
        class XYFocusAlgorithmsUnitTests : public WEX::TestClass<XYFocusAlgorithmsUnitTests>
        {
        public:
            BEGIN_TEST_CLASS(XYFocusAlgorithmsUnitTests)
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_CLASS()

            BEGIN_TEST_METHOD(ElementCompletelyInsideVisionCodeDetected)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that we successfully detect an element that is completely inside the vision cone. Verifies all directions")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ElementCompletelyOutsideVisionCodeDetected)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that we successfully detect an element that is completely outside the vision cone. Verifies all directions")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ElementIntersectsVisionCodeDetected)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that we successfully detect the intersection between the element and vision cone. Verifies all directions")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(IntersectionDetectedAtEdge)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that we successfully detect the intersection between the element and vision cone when the element is at the very edge")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyExclusionRectsContainer)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that we ignore candidates found inside of an exclusion Rect")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyExclusionRectsIntersection)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that we ignore candidates found intersecting with an exclusion Rect")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyEmptyRectsAreIgnoredAsCandidates)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that we ignore candidates with empty bounds from being considered for Auto-Focus")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyCandidateUpAgainstCone)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that we select candiates right next to the cone")
            END_TEST_METHOD()
                
        private:
            XYFocusAlgorithmsUnitTests();
            XRECTF_RB emptyRect;
        };
    }}
}}}}
