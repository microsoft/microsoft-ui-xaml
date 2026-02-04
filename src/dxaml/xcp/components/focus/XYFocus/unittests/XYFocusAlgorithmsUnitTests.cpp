// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "XYFocusAlgorithmsUnitTests.h"
#include "XYFocusAlgorithms.h"

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { 
    namespace Xaml { namespace Focus {

        using namespace DirectUI;

        XYFocusAlgorithmsUnitTests::XYFocusAlgorithmsUnitTests()
        {
            EmptyRectF(&emptyRect);
        }
        /*

                                                                XXXXXXX
                                                          XXXXXX
                                                    XXXXXX
        +-----------------------------------+XXXXXX
        |                                   |     
        |                                   |                           +----------------------+
        |                                   |                           |                      |
        |                                   |                           |                      |
        |                                   |                           |                      |
        |                                   |                           |                      |
        |                                   |                           |                      |
        |                                   |                           |                      |
        |                                   |                           +----------------------+
        |                                   |         
        |                                   |                 
        |                                   |
        +-----------------------------------+XXXXXXX
                                                    XXXXXXX
                                                           XXXXXXX
                                                                  XXXXXXX 
                                                            
        */
        
        void XYFocusAlgorithmsUnitTests::ElementCompletelyInsideVisionCodeDetected()
        {
            XRECTF_RB focusedElement = { 100, 100, 200, 200 };
            XRECTF_RB candidateElement = { 430, 0, 500, 300 };

            const double maxDistance = 600;

            FocusNavigationDirection direction = FocusNavigationDirection::Right;

            bool shouldConsider = XYFocusAlgorithms::ShouldCandidateBeConsideredForRanking(
                focusedElement, candidateElement, maxDistance, direction, emptyRect);
            VERIFY_IS_TRUE(shouldConsider);   

            candidateElement = { 50, 100, 100, 150 };
            direction = FocusNavigationDirection::Left;

            shouldConsider = XYFocusAlgorithms::ShouldCandidateBeConsideredForRanking(
                focusedElement, candidateElement, maxDistance, direction, emptyRect);
            VERIFY_IS_TRUE(shouldConsider);
        }

        /*
                                         +------------------+
                                         |                  |
                                         |                  |
                                         |                  |
                                         |                  |              XXXXX
                                         +------------------+      X XXXXXXX
                                                              XXXXXXXX
                                                        XXXXXXX
                                                   XXXXXX
                                            XXXXXX
                                     XXXXXX
        +------------------------+XXX 
        |                        |
        |                        |
        |                        |
        |                        |
        +------------------------+XXXXXXX
                                          XXXXXXX
                                                  XXXXXXX  
                                                         XXXXXXX
                                                                XXXXX
        */

        void XYFocusAlgorithmsUnitTests::ElementCompletelyOutsideVisionCodeDetected()
        {
            XRECTF_RB focusedElement = { 100, 100, 200, 200 };
            XRECTF_RB candidateElement = { 0, 0, 50, 50 };

            const double maxDistance = 600;

            FocusNavigationDirection direction = FocusNavigationDirection::Right;

            bool shouldConsider = XYFocusAlgorithms::ShouldCandidateBeConsideredForRanking(
                focusedElement, candidateElement, maxDistance, direction, emptyRect);
            VERIFY_IS_FALSE(shouldConsider);
            
            candidateElement = { 0, 500, 50, 550 };
            direction = FocusNavigationDirection::Left;

            shouldConsider = XYFocusAlgorithms::ShouldCandidateBeConsideredForRanking(
                focusedElement, candidateElement, maxDistance, direction, emptyRect);
            VERIFY_IS_FALSE(shouldConsider);
        }

    /*
                                                               +----------------------+       XX
                                                               |                      | XXXXXXX
                                                               |             XXXXXXXXXXXX
                                                               |       XXXXXXX        |
                                                               |X XXXXXX              |
                                                            XXX|                      |
                                                       XXXXX   |                      |
                                                  XXXXX        |                      |
                                         XXXXXXXXX             +----------------------+
        +--------------------------------+
        |                                |
        |                                |
        |                                |
        |                                |
        |                                |
        |                                |
        |                                |
        |                                |
        +--------------------------------+XXXXXXX
                                                 XXXXXXX
                                                        XXXXXXX  
                                                               XXXXXXX
                                                                      XXXXX
        */

        void XYFocusAlgorithmsUnitTests::ElementIntersectsVisionCodeDetected()
        {
            XRECTF_RB focusedElement = { 100, 100, 200, 200 };
            XRECTF_RB candidateElement = { 300, 30, 350, 80 };

            const double maxDistance = 600;

            FocusNavigationDirection direction = FocusNavigationDirection::Right;

            bool shouldConsider = XYFocusAlgorithms::ShouldCandidateBeConsideredForRanking(
                focusedElement, candidateElement, maxDistance, direction, emptyRect);
            VERIFY_IS_TRUE(shouldConsider);

            candidateElement = { 50, 50, 80, 80 };
            direction = FocusNavigationDirection::Up;

            shouldConsider = XYFocusAlgorithms::ShouldCandidateBeConsideredForRanking(
                focusedElement, candidateElement, maxDistance, direction, emptyRect);
            VERIFY_IS_TRUE(shouldConsider);
        }

            /*
                            XXXXX  
                       XXXXX
                  XXXXX          
        +------+X +-------+
        |      |  |       |
        |      |  |       |
        |      |  |       |
        |      |  +-------+
        |      |
        |      |
        |      |
        |      |
        |      |
        |      |
        |      |
        |      |
        |      |
        |      |
        |      |
        +------+XXXXX
                      XXXXXXX
                              XXXXXXX  
        */

        void XYFocusAlgorithmsUnitTests::VerifyCandidateUpAgainstCone()
        {
            XRECTF_RB focusedElement = { 100, 100, 120, 600 };
            XRECTF_RB candidateElement = { 120, 100, 150, 150 };

            const double maxDistance = 600;

            FocusNavigationDirection direction = FocusNavigationDirection::Right;

            bool shouldConsider = XYFocusAlgorithms::ShouldCandidateBeConsideredForRanking(
                focusedElement, candidateElement, maxDistance, direction, emptyRect);
            VERIFY_IS_TRUE(shouldConsider);

            candidateElement = { 50, 100, 100, 150 };
            direction = FocusNavigationDirection::Left;

            shouldConsider = XYFocusAlgorithms::ShouldCandidateBeConsideredForRanking(
                focusedElement, candidateElement, maxDistance, direction, emptyRect);
            VERIFY_IS_TRUE(shouldConsider);
        }

        void XYFocusAlgorithmsUnitTests::IntersectionDetectedAtEdge()
        {
            XRECTF_RB focusedElement = { 100, 100, 200, 200 };

            float left = (float)(focusedElement.left + 50 * std::cos(0));
            float top = (float)(focusedElement.top + 50 * std::sin(0));
            XRECTF_RB candidateElement = { left, top, 350, 180 };

            const double maxDistance = 600;

            FocusNavigationDirection direction = FocusNavigationDirection::Right;

            bool shouldConsider = XYFocusAlgorithms::ShouldCandidateBeConsideredForRanking(
                focusedElement, candidateElement, maxDistance, direction, emptyRect);

            VERIFY_IS_TRUE(shouldConsider);
        }

        void XYFocusAlgorithmsUnitTests::VerifyExclusionRectsContainer()
        {
            XRECTF_RB focusedElement = { 100, 100, 200, 200 };
            XRECTF_RB exclusionRect = { 200, 200, 400, 400 };

            XRECTF_RB candidateElement = { 300, 300, 350, 350 };

            const double maxDistance = 600;

            FocusNavigationDirection direction = FocusNavigationDirection::Right;

            bool shouldConsider = XYFocusAlgorithms::ShouldCandidateBeConsideredForRanking(
                focusedElement, candidateElement, maxDistance, direction, exclusionRect);

            VERIFY_IS_FALSE(shouldConsider);
        }

        void XYFocusAlgorithmsUnitTests::VerifyExclusionRectsIntersection()
        {
            XRECTF_RB focusedElement = { 100, 100, 200, 200 };
            XRECTF_RB exclusionRect = { 200, 200, 400, 400 };

            XRECTF_RB candidateElement = { 350, 350, 450, 450 };
            
            const double maxDistance = 600;

            FocusNavigationDirection direction = FocusNavigationDirection::Right;

            bool shouldConsider = XYFocusAlgorithms::ShouldCandidateBeConsideredForRanking(
                focusedElement, candidateElement, maxDistance, direction, exclusionRect);

            VERIFY_IS_FALSE(shouldConsider);
        }

        void XYFocusAlgorithmsUnitTests::VerifyEmptyRectsAreIgnoredAsCandidates()
        {
            XRECTF_RB focusedElement = { 100, 100, 200, 200 };
            
            XRECTF_RB candidateElement = { 100, 10, 100, 10 };

            const double maxDistance = 600;

            FocusNavigationDirection direction = FocusNavigationDirection::Up;

            bool shouldConsider = XYFocusAlgorithms::ShouldCandidateBeConsideredForRanking(
                focusedElement, candidateElement, maxDistance, direction, emptyRect);

            VERIFY_IS_FALSE(shouldConsider);
        }
    }}
}}}}
