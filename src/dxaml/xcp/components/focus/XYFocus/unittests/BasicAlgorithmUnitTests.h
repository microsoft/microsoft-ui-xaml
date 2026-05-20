// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "WexTestClass.h"

#include "enumdefs.g.h"
#include "minxcptypes.h"

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { 
    namespace Xaml { namespace Focus { namespace XYFocus {
        class BasicAlgorithmUnitTests : public WEX::TestClass<BasicAlgorithmUnitTests>
        {
        public:
            BEGIN_TEST_CLASS(BasicAlgorithmUnitTests)
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_CLASS()

            BEGIN_TEST_METHOD(DoNotCalculatePrimaryDistanceFromSelf)
                TEST_METHOD_PROPERTY(L"Description", L"If we pass in a candidate that is the same distance as the current bounds, we should return -1")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyPrimaryAxisDistance)
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifySecondaryAxisDistance)
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyShadow)
            END_TEST_METHOD()
                
            BEGIN_TEST_METHOD(VerifyLeft)
                TEST_METHOD_PROPERTY(L"Description", L"Verify that we find the best element to the left based on the scenario.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyRight)
                TEST_METHOD_PROPERTY(L"Description", L"Verify that we find the best element to the right based on the scenario.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyUp)
                TEST_METHOD_PROPERTY(L"Description", L"Verify that we find the best element to the up based on the scenario.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyDown)
                TEST_METHOD_PROPERTY(L"Description", L"Verify that we find the best element to the down based on the scenario.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyNoCandidates)
                TEST_METHOD_PROPERTY(L"Description", L"Verify that we do not find a candidate based on the scenario")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyManifoldAidsInDecision)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that the user can press the key x amount and get to the correct element")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateOverlappingElementToRight)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that the element to the right (with -ve margin) of the focused element can be selected")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ElementWithNegativeBoundsRankedCorrectly)
                TEST_METHOD_PROPERTY(L"Description", L"If we are trying to rank an element with negative bounds, we should still receive a valid score")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(FocusedElementWithNegativeBoundsProducesValidCandidatesThatIsOnSreen)
                TEST_METHOD_PROPERTY(L"Description", 
                    L"If we are trying to rank an element where the focused element has negative bounds, we should still receive a valid list of candidates (that are still 'onscreen'")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(FocusedElementWithNegativeSelectsCandidateThatIsOffScreen)
                TEST_METHOD_PROPERTY(L"Description", 
                    L"If we are trying to rank an element where the focused element has negative bounds, we should still receive a valid list of candidates, even candidate that also have negative bounds")
            END_TEST_METHOD()     

            BEGIN_TEST_METHOD(ElementScrolledExtremleyOutOfViewShouldStillBeSelected)
                TEST_METHOD_PROPERTY(L"Description", 
                    L"If an element is scrolled to where it exceeds the distance of the root bounds, we should calculate a new distance and use that to find the candidates")
            END_TEST_METHOD()
                

        private:
            XRECTF_RB BestElement(
                _In_ std::vector<XRECTF_RB>& rectList,
                _In_ XRECTF_RB bounds,
                _In_ DirectUI::FocusNavigationDirection direction,
                _In_ DirectUI::XYFocusNavigationStrategy mode,
                _In_opt_ std::pair<double, double>* hManifold = nullptr,
                _In_opt_ std::pair<double, double>* vManifold = nullptr);
        };
    }}}
}}}}
