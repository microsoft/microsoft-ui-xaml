// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "ProximityStrategyUnitTests.h"
#include <ProximityStrategy.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { 
    namespace Xaml { namespace Focus { namespace XYFocus {

        using namespace DirectUI;
        using namespace ::Focus::XYFocusPrivate;
        using namespace Microsoft::UI::Xaml::Tests;

        /*
            +---------+
            |         |
            |         |
            |         |
            |         |
            |         |
            |         |
            +---------+
                                 +------+
                                 |  A   |
                                 +------+
             +-------+
             |       |
             |   B   |
             +-------+
        
        */

        void ProximityStrategyUnitTests::VerifyProximityStrategyClosestToAxis()
        {
            ProximityStrategy algorithm;

            XRECTF_RB focusedElement = { 100, 100, 200, 200 };

            XRECTF_RB candidateElementA = { 250, 200, 300, 300 };
            XRECTF_RB candidateElementB = { 100, 310, 200, 410 };
            const double maxDistance = 600;

            FocusNavigationDirection direction = FocusNavigationDirection::Down;
            
            const double scoreA = algorithm.GetScore(direction, focusedElement, candidateElementA, maxDistance, false);
            const double scoreB = algorithm.GetScore(direction, focusedElement, candidateElementB, maxDistance, false);
            VERIFY_IS_TRUE(scoreA > scoreB);
        }

        /*
            +---------+
            |         |
            |         |
            |         |
            |         |
            |         |
            |         |
            +---------+
                                                                                                                         +------+
                                                                                                                         |  A   |
                                                                                                                         +------+
             +-------+
             |       |
             |   B   |
             +-------+
        
        */

        void ProximityStrategyUnitTests::VerifyProximityStrategyClosestToAxisWithExtremeDistance()
        {
            ProximityStrategy algorithm;

            XRECTF_RB focusedElement = { 100, 100, 200, 200 };

            XRECTF_RB candidateElementA = { 2000, 200, 2200, 300 };
            XRECTF_RB candidateElementB = { 100, 315, 200, 415 };
            const double maxDistance = 3000;

            FocusNavigationDirection direction = FocusNavigationDirection::Down;
            
            const double scoreA = algorithm.GetScore(direction, focusedElement, candidateElementA, maxDistance, false);
            const double scoreB = algorithm.GetScore(direction, focusedElement, candidateElementB, maxDistance, false);
            VERIFY_IS_TRUE(scoreA > scoreB);
        }

        /*
            +---------+
            |         |
            |         |
            |         |
            |         |
            |         |
            |         |
            +---------+
                                                             +------+
                                                             |  A   |
                                                             +------+
             +-------+
             |       |
             |   B   |
             +-------+
        
        */

        void ProximityStrategyUnitTests::VerifyProximityStrategyNearness()
        {
            ProximityStrategy algorithm;

            XRECTF_RB focusedElement = { 100, 100, 200, 200 };

            XRECTF_RB candidateElementA = { 1000, 110, 1200, 160 };
            XRECTF_RB candidateElementB = { 100, 300, 200, 400 };
            const double maxDistance = 600;

            FocusNavigationDirection direction = FocusNavigationDirection::Down;

            const double scoreA = algorithm.GetScore(direction, focusedElement, candidateElementA, maxDistance, true);
            const double scoreB = algorithm.GetScore(direction, focusedElement, candidateElementB, maxDistance, true);
            VERIFY_IS_TRUE(scoreB > scoreA);
        }

        /*
                             +------------+
                             |            |
                             |            |
                             |     B      |
                             |            |
                +--------+   |            |
                |        |   +------------+
                |   A    |   +------------+
                |        |   |    C       |
                +--------+   +------------+


        */

        void ProximityStrategyUnitTests::VerifyNearnessMeasuresShadow()
        {
            ProximityStrategy algorithm;

            XRECTF_RB a = { 0, 100, 50, 200 };
            XRECTF_RB b = { 75, 50, 125, 140 };
            XRECTF_RB c = { 75, 150, 125, 200 };
            const double maxDistance = 600;

            FocusNavigationDirection direction = FocusNavigationDirection::Down;

            const double scoreA = algorithm.GetScore(direction, a, b, maxDistance, true);
            const double scoreB = algorithm.GetScore(direction, a, c, maxDistance, true);
            VERIFY_IS_TRUE(scoreA == scoreB);
        }
    }}}
}}}}    