// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "BasicAlgorithmUnitTests.h"
#include "AlgorithmHelper.h"

#include <XYFocusAlgorithms.h>
#include "ProximityStrategy.h"

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { 
    namespace Xaml { namespace Focus { namespace XYFocus {

        using namespace DirectUI;
        using namespace ::Focus::XYFocusPrivate;

        void BasicAlgorithmUnitTests::DoNotCalculatePrimaryDistanceFromSelf()
        {
            XRECTF_RB current = { 0, 0, 100, 100 };
            XRECTF_RB candidate = { 0, 0, 100, 100 };

            const double distance = CalculatePrimaryAxisDistance(DirectUI::FocusNavigationDirection::Left, current, candidate);
            VERIFY_IS_TRUE(distance == -1);
        }

        /*
            +---------------+   +---------------+  +----------------+
            |               |   |               |  |                |
            |               |   |               |  |                |
            |               |   |       A       |  |       B        |
            |               |   |               |  |                |
            |               |   |               |  |                |
            +---------------+   +---------------+  +----------------+

        */

        void BasicAlgorithmUnitTests::VerifyPrimaryAxisDistance()
        {
            XRECTF_RB current = { 0, 0, 100, 100 };
            XRECTF_RB candidateA = { 125, 0, 225, 100 };
            XRECTF_RB candidateB = { 235, 0, 335, 100 };

            const double distanceA = CalculatePrimaryAxisDistance(DirectUI::FocusNavigationDirection::Right, current, candidateA);
            const double distanceB = CalculatePrimaryAxisDistance(DirectUI::FocusNavigationDirection::Right, current, candidateB);
            VERIFY_IS_TRUE(distanceA < distanceB);
        }

        /*
            +-------------+
            |             |
            |             |
            |             |
            |             |
            +-------------+
                                       +---------+
                                       |         |
                                       |   A     |
                                       +---------+

                              +-------+
                              |       |
                              |  B    |
                              +-------+

        */

        void BasicAlgorithmUnitTests::VerifySecondaryAxisDistance()
        {
            XRECTF_RB current = { 0, 0, 100, 100 };
            XRECTF_RB candidateA = { 150, 125, 225, 225 };
            XRECTF_RB candidateB = { 110, 175, 220, 225 };

            double distanceA = CalculateSecondaryAxisDistance(DirectUI::FocusNavigationDirection::Right, current, candidateA);
            double distanceB = CalculateSecondaryAxisDistance(DirectUI::FocusNavigationDirection::Right, current, candidateB);
            VERIFY_IS_TRUE(distanceA < distanceB);

            // We can have situations where the secondary distance is shorter although the primary distance is longer
            distanceA = CalculatePrimaryAxisDistance(DirectUI::FocusNavigationDirection::Right, current, candidateA);
            distanceB = CalculatePrimaryAxisDistance(DirectUI::FocusNavigationDirection::Right, current, candidateB);
            VERIFY_IS_TRUE(distanceA > distanceB);
        }

        /*

        +----------------+
        |                |
        |                |
        |     B          |
        |                |
        |                |
        +----------------+

        +---------------+   +---------------+
        |               |   |               |
        |               |   |               |
        |               |   |       A       |
        |               |   |               |
        |               |   |               |
        +---------------+   +---------------+

        */

        void BasicAlgorithmUnitTests::VerifyShadow()
        {
            XRECTF_RB current = { 0, 100, 100, 200 };
            XRECTF_RB candidateA = { 125, 100, 225, 200 };
            XRECTF_RB candidateB = { 0, 0, 100, 100 };

            std::pair<double, double> potentialVertical;
            std::pair<double, double> referenceVertical;

            std::pair<double, double> potentialHorizontal;
            std::pair<double, double> referenceHorizontal;

            referenceVertical.first = current.left;
            referenceVertical.second = current.right;

            potentialVertical.first = candidateB.left;
            potentialVertical.second = candidateB.right;

            referenceHorizontal.first = current.top;
            referenceHorizontal.second = current.bottom;

            potentialHorizontal.first = candidateA.top;
            potentialHorizontal.second = candidateA.bottom;

            VERIFY_IS_TRUE(CalculatePercentInShadow(potentialVertical, referenceVertical) > 0);
            VERIFY_IS_TRUE(CalculatePercentInShadow(referenceVertical, potentialVertical) > 0);
            VERIFY_IS_TRUE(CalculatePercentInShadow(referenceHorizontal, potentialHorizontal) > 0);
            VERIFY_IS_TRUE(CalculatePercentInShadow(potentialHorizontal, referenceHorizontal) > 0);
        }

        /*
        The scenario that the test are based off
        +---------------------------------------------------------Root---------------------------------------------------------------------------------------------+
        |                                                                                                                                 _ -ve Margins            |
        |                                                                                                                                |                         |
        |                                                                                                                                V                         |
        |                                          +-----------------+              +------------------+              +-----------------++----------------+        |
        |            +-----------------+           |                 |              |                  |              |                 ||                |        |
        |            |       F         |           |                 |              |        C         |              |        G        ||       J        |        |
        |            +-----------------+           |                 |              |                  |              |                 ||                |        |
        |       +----+-----------------++          |                 |              +------------------+              +-----------------++----------------+        |
        |       |                       |          |                 |              +------------------+              +-----------------++----------------+        |
        |       |                       |          |                 |              |                  |              |                 ||                |        |
        |       |           B           |          |       Main      |              |         D        |              |         H       ||       K        |        |
        |       |                       |          |                 |              |                  |              |                 ||                |        |
        |       +----+------------------+          |                 |              +------------------+              +-----------------++----------------+        |
        |            +------------------+          |                 |              +------------------+              +-----------------++----------------+        |
        |            |                  |          |                 |              |                  |              |                 ||                |        |
        |            |       E          |          |                 |              |         A        |              |         I       ||       L        |        |
        |            |                  |          |                 |              |                  |              |                 ||                |        |
        |            +------------------+          +-----------------+              +------------------+              +-----------------++----------------+        |
        |                                                                                                                                                          |
        +----------------------------------------------------------------------------------------------------------------------------------------------------------+
        */

        XRECTF_RB m_elementMain = { 120, 20, 240, 260 };
        XRECTF_RB m_elementA = { 280, 200, 360, 260 };
        XRECTF_RB m_elementB = { 20, 120, 100, 180 };
        XRECTF_RB m_elementC = { 280, 20, 360, 80 };
        XRECTF_RB m_elementD = { 280, 120, 360, 180 };

        XRECTF_RB m_elementE = { 20, 200, 100, 260 };
        XRECTF_RB m_elementF = { 20, 20, 100, 80 };

        XRECTF_RB m_elementG = { 440, 20, 520, 80 };
        XRECTF_RB m_elementH = { 440, 120, 520, 180 };
        XRECTF_RB m_elementI = { 440, 200, 520, 260 };

        XRECTF_RB m_elementJ = { 500, 20, 580, 80 };
        XRECTF_RB m_elementK = { 500, 120, 580, 180 };
        XRECTF_RB m_elementL = { 500, 200, 580, 260 };

        static const XRECTF_RB arr[] = { m_elementF, m_elementB, m_elementE, m_elementMain, m_elementC, m_elementD, m_elementA, m_elementG, m_elementH, m_elementI, m_elementJ, m_elementK, m_elementL };
        std::vector<XRECTF_RB> scenario(arr, arr + sizeof(arr) / sizeof(arr[0]));

        XRECTF_RB BasicAlgorithmUnitTests::BestElement(
            _In_ std::vector<XRECTF_RB>& rectList,
            _In_ XRECTF_RB bounds,
            _In_ DirectUI::FocusNavigationDirection direction,
            _In_ DirectUI::XYFocusNavigationStrategy mode,
            _In_opt_ std::pair<double, double>* hManifold,
            _In_opt_ std::pair<double, double>* vManifold)
        {
            std::pair<XRECTF_RB, double> bestElement = std::make_pair(XRECTF_RB(), 0);

            ProximityStrategy proximity;
            XYFocusAlgorithms Projection;

            const double distance = 10000;

            for (XRECTF_RB rect : rectList)
            {
                double score = 0;

                if (mode == DirectUI::XYFocusNavigationStrategy::Projection)
                {
                    score = Projection.GetScore(direction, bounds, rect, *hManifold, *vManifold, distance);
                }
                else if (mode == DirectUI::XYFocusNavigationStrategy::NavigationDirectionDistance || mode == DirectUI::XYFocusNavigationStrategy::RectilinearDistance)
                {
                    score = proximity.GetScore(direction, bounds, rect, distance, mode == DirectUI::XYFocusNavigationStrategy::RectilinearDistance);
                }

                if (score > bestElement.second)
                {
                    bestElement.first = rect;
                    bestElement.second = score;
                }
            }

            return bestElement.first;
        }

        void BasicAlgorithmUnitTests::VerifyLeft()
        {
            XRECTF_RB bounds = m_elementMain;
            DirectUI::FocusNavigationDirection direction = DirectUI::FocusNavigationDirection::Left;

            std::pair<double, double> vManifold = std::make_pair(-1.0, -1.0);
            std::pair<double, double> hManifold = std::make_pair(-1.0, -1.0);

            XRECTF_RB candidate = BestElement(scenario, bounds, direction, DirectUI::XYFocusNavigationStrategy::Projection, &hManifold, &vManifold);
            VERIFY_ARE_EQUAL(candidate, m_elementF);

            candidate = BestElement(scenario, bounds, direction, DirectUI::XYFocusNavigationStrategy::NavigationDirectionDistance);
            VERIFY_ARE_EQUAL(candidate, m_elementF);

            candidate = BestElement(scenario, bounds, direction, DirectUI::XYFocusNavigationStrategy::RectilinearDistance);
            VERIFY_ARE_EQUAL(candidate, m_elementF);
        }

        void BasicAlgorithmUnitTests::VerifyRight()
        {
            XRECTF_RB bounds = m_elementE;
            DirectUI::FocusNavigationDirection direction = DirectUI::FocusNavigationDirection::Right;

            std::pair<double, double> vManifold = std::make_pair(-1.0, -1.0);
            std::pair<double, double> hManifold = std::make_pair(-1.0, -1.0);

            XRECTF_RB candidate = BestElement(scenario, bounds, direction, DirectUI::XYFocusNavigationStrategy::Projection, &hManifold, &vManifold);
            VERIFY_ARE_EQUAL(candidate, m_elementMain);

            candidate = BestElement(scenario, bounds, direction, DirectUI::XYFocusNavigationStrategy::NavigationDirectionDistance);
            VERIFY_ARE_EQUAL(candidate, m_elementMain);

            candidate = BestElement(scenario, bounds, direction, DirectUI::XYFocusNavigationStrategy::RectilinearDistance);
            VERIFY_ARE_EQUAL(candidate, m_elementMain);
        }

        void BasicAlgorithmUnitTests::VerifyUp()
        {
            XRECTF_RB bounds = m_elementE;
            DirectUI::FocusNavigationDirection direction = DirectUI::FocusNavigationDirection::Up;

            std::pair<double, double> vManifold = std::make_pair(-1.0, -1.0);
            std::pair<double, double> hManifold = std::make_pair(-1.0, -1.0);

            XRECTF_RB candidate = BestElement(scenario, bounds, direction, DirectUI::XYFocusNavigationStrategy::Projection, &hManifold, &vManifold);
            VERIFY_ARE_EQUAL(candidate, m_elementB);

            candidate = BestElement(scenario, bounds, direction, DirectUI::XYFocusNavigationStrategy::NavigationDirectionDistance);
            VERIFY_ARE_EQUAL(candidate, m_elementB);

            candidate = BestElement(scenario, bounds, direction, DirectUI::XYFocusNavigationStrategy::RectilinearDistance);
            VERIFY_ARE_EQUAL(candidate, m_elementB);
        }

        void BasicAlgorithmUnitTests::VerifyDown()
        {
            XRECTF_RB bounds = m_elementF;
            DirectUI::FocusNavigationDirection direction = DirectUI::FocusNavigationDirection::Down;

            std::pair<double, double> vManifold = std::make_pair(-1.0, -1.0);
            std::pair<double, double> hManifold = std::make_pair(-1.0, -1.0);

            XRECTF_RB candidate = BestElement(scenario, bounds, direction, DirectUI::XYFocusNavigationStrategy::Projection, &hManifold, &vManifold);
            VERIFY_ARE_EQUAL(candidate, m_elementB);

            candidate = BestElement(scenario, bounds, direction, DirectUI::XYFocusNavigationStrategy::NavigationDirectionDistance);
            VERIFY_ARE_EQUAL(candidate, m_elementB);

            candidate = BestElement(scenario, bounds, direction, DirectUI::XYFocusNavigationStrategy::RectilinearDistance);
            VERIFY_ARE_EQUAL(candidate, m_elementB);
        }

        void BasicAlgorithmUnitTests::VerifyNoCandidates()
        {
            XRECTF_RB bounds = m_elementE;
            DirectUI::FocusNavigationDirection direction = DirectUI::FocusNavigationDirection::Down;

            std::pair<double, double> vManifold = std::make_pair(-1.0, -1.0);
            std::pair<double, double> hManifold = std::make_pair(-1.0, -1.0);

            XRECTF_RB candidate = BestElement(scenario, bounds, direction, DirectUI::XYFocusNavigationStrategy::Projection, &hManifold, &vManifold);
            VERIFY_ARE_EQUAL(candidate, XRECTF_RB());

            candidate = BestElement(scenario, bounds, direction, DirectUI::XYFocusNavigationStrategy::NavigationDirectionDistance);
            VERIFY_ARE_EQUAL(candidate, XRECTF_RB());

            candidate = BestElement(scenario, bounds, direction, DirectUI::XYFocusNavigationStrategy::RectilinearDistance);
            VERIFY_ARE_EQUAL(candidate, XRECTF_RB());
        }

        void BasicAlgorithmUnitTests::VerifyManifoldAidsInDecision()
        {
            XRECTF_RB bounds = m_elementMain;
            DirectUI::FocusNavigationDirection direction = DirectUI::FocusNavigationDirection::Right;

            std::pair<double, double> vManifold = std::make_pair(-1.0, -1.0);
            std::pair<double, double> hManifold = std::make_pair(-1.0, -1.0);

            XYFocusAlgorithms::UpdateManifolds(DirectUI::FocusNavigationDirection::Right, m_elementE, m_elementMain, hManifold, vManifold);

            XRECTF_RB candidate = BestElement(scenario, bounds, direction, DirectUI::XYFocusNavigationStrategy::Projection, &hManifold, &vManifold);
            VERIFY_ARE_EQUAL(candidate, m_elementA);
        }

        void BasicAlgorithmUnitTests::ValidateOverlappingElementToRight()
        {
            XRECTF_RB bounds = m_elementG;
            DirectUI::FocusNavigationDirection direction = DirectUI::FocusNavigationDirection::Right;

            std::pair<double, double> vManifold = std::make_pair(-1.0, -1.0);
            std::pair<double, double> hManifold = std::make_pair(-1.0, -1.0);

            XRECTF_RB candidate = BestElement(scenario, bounds, direction, DirectUI::XYFocusNavigationStrategy::Projection, &hManifold, &vManifold);
            VERIFY_ARE_EQUAL(candidate, m_elementJ);

            candidate = BestElement(scenario, bounds, direction, DirectUI::XYFocusNavigationStrategy::NavigationDirectionDistance);
            VERIFY_ARE_EQUAL(candidate, m_elementJ);

            candidate = BestElement(scenario, bounds, direction, DirectUI::XYFocusNavigationStrategy::RectilinearDistance);
            VERIFY_ARE_EQUAL(candidate, m_elementJ);
        }

        /*
                                                                          +------------------+
                                                                          |                  |
                                                                          |                  |
                                                                          |       a          |
                                                                          |                  |
                                                                          |                  |
                                                                          +------------------+
                    +----------------------------------------------------------------------------+
                    |                                                                            |
                    |                                                                            |
                    |         +------------------+                                               |
                    |         |                  |                                               |
                    |         |                  |                                               |
                    |         |                  |                                               |
                    |         |                  |                                               |
                    |         |                  |                                               |
                    |         |                  |                                               |
                    |         |                  |                                               |
                    |         |                  |                                               |
                    |         |      Start       |                                               |
                    |         |                  |                                               |
                    |         |                  |                                               |
                    |         |                  |                                               |
                    |         |                  |                                               |
                    |         |                  |                                               |
                    |         |                  |                                               |
                    |         |                  |                                               |
                    |         +------------------+                                               |
                    |                                                                            |
                    |                                                                    Root    |
                    +----------------------------------------------------------------------------+
            */
        void BasicAlgorithmUnitTests::ElementWithNegativeBoundsRankedCorrectly()
        {
            XRECTF_RB start = { 120, 20, 240, 260 };
            XRECTF_RB a = { 440, -80, 520, -20 };

            DirectUI::FocusNavigationDirection direction = DirectUI::FocusNavigationDirection::Right;

            std::pair<double, double> vManifold = std::make_pair(-1.0, -1.0);
            std::pair<double, double> hManifold = std::make_pair(-1.0, -1.0);

            std::vector<XRECTF_RB> scenario;
            scenario.push_back(start);
            scenario.push_back(a);

            XRECTF_RB candidate = BestElement(scenario, start, direction, DirectUI::XYFocusNavigationStrategy::Projection, &hManifold, &vManifold);
            VERIFY_ARE_EQUAL(candidate, a);

            candidate = BestElement(scenario, start, direction, DirectUI::XYFocusNavigationStrategy::NavigationDirectionDistance);
            VERIFY_ARE_EQUAL(candidate, a);

            candidate = BestElement(scenario, start, direction, DirectUI::XYFocusNavigationStrategy::RectilinearDistance);
            VERIFY_ARE_EQUAL(candidate, a);
        }

        /*
                                                                      +------------------+
                                                                      |                  |
                                                                      |                  |
                                                                      |       start      |
                                                                      |                  |
                                                                      |                  |
                                                                      +------------------+
                +----------------------------------------------------------------------------+
                |                                                                            |
                |                                                                            |
                |         +------------------+                                               |
                |         |                  |                                               |
                |         |                  |                                               |
                |         |                  |                                               |
                |         |                  |                                               |
                |         |                  |                                               |
                |         |                  |                                               |
                |         |                  |                                               |
                |         |                  |                                               |
                |         |       a          |                                               |
                |         |                  |                                               |
                |         |                  |                                               |
                |         |                  |                                               |
                |         |                  |                                               |
                |         |                  |                                               |
                |         |                  |                                               |
                |         |                  |                                               |
                |         +------------------+                                               |
                |                                                                            |
                |                                                                    Root    |
                +----------------------------------------------------------------------------+
            */

        void BasicAlgorithmUnitTests::FocusedElementWithNegativeBoundsProducesValidCandidatesThatIsOnSreen()
        {
            XRECTF_RB a = { 120, 20, 240, 260 };
            XRECTF_RB start = { 440, -80, 520, -20 };

            DirectUI::FocusNavigationDirection direction = DirectUI::FocusNavigationDirection::Left;

            std::pair<double, double> vManifold = std::make_pair(-1.0, -1.0);
            std::pair<double, double> hManifold = std::make_pair(-1.0, -1.0);

            std::vector<XRECTF_RB> scenario;
            scenario.push_back(start);
            scenario.push_back(a);

            XRECTF_RB candidate = BestElement(scenario, start, direction, DirectUI::XYFocusNavigationStrategy::Projection, &hManifold, &vManifold);
            VERIFY_ARE_EQUAL(candidate, a);

            candidate = BestElement(scenario, start, direction, DirectUI::XYFocusNavigationStrategy::NavigationDirectionDistance);
            VERIFY_ARE_EQUAL(candidate, a);

            candidate = BestElement(scenario, start, direction, DirectUI::XYFocusNavigationStrategy::RectilinearDistance);
            VERIFY_ARE_EQUAL(candidate, a);
        }

        /*
                                                                                +------------------+
                                                                                |                  |
                                                                                |                  |
                                                                                |       a          |
                                                                                |                  |
                                                                                |                  |
                                                                                +------------------+



                                    +------------------+
                                    |                  |
                                    |                  |
                                    |                  |
                                    |                  |
                                    |                  |
                                    |                  |
                                    |                  |
                                    |                  |
                                    |       start      |
                                    |                  |
                                    |                  |
                                    |                  |
                                    |                  |
                                    |                  |
                                    |                  |
                                    |                  |
                                    +------------------+
                    +------------------------------------------------------------------------------------------+
                    |                                                                                          |
                    |                                                                                          |
                    |                                                                                          |
                    |                                                                                          |
                    |                                                                                          |
                    |                                                                                          |
                    |                                                                                          |
                    |                                                                                          |
                    |                                                                                          |
                    |                                                                                          |
                    |                                                                                          |
                    |                                                                                          |
                    |                                                                                          |
                    |                                                                                          |
                    |                                                                                          |
                    |                                                                                          |
                    |                                                                                          |
                    |                                                                                          |
                    |                                                                                          |
                    |                                                                                          |
                    |                                                                                  root    |
                    +------------------------------------------------------------------------------------------+

        */
        void BasicAlgorithmUnitTests::FocusedElementWithNegativeSelectsCandidateThatIsOffScreen()
        {
            XRECTF_RB start = { 120, -140, 240, -10 };
            XRECTF_RB a = { 280, -140, 360, -80 };

            DirectUI::FocusNavigationDirection direction = DirectUI::FocusNavigationDirection::Right;

            std::pair<double, double> vManifold = std::make_pair(-1.0, -1.0);
            std::pair<double, double> hManifold = std::make_pair(-1.0, -1.0);

            std::vector<XRECTF_RB> scenario;
            scenario.push_back(start);
            scenario.push_back(a);

            XRECTF_RB candidate = BestElement(scenario, start, direction, DirectUI::XYFocusNavigationStrategy::Projection, &hManifold, &vManifold);
            VERIFY_ARE_EQUAL(candidate, a);

            candidate = BestElement(scenario, start, direction, DirectUI::XYFocusNavigationStrategy::NavigationDirectionDistance);
            VERIFY_ARE_EQUAL(candidate, a);

            candidate = BestElement(scenario, start, direction, DirectUI::XYFocusNavigationStrategy::RectilinearDistance);
            VERIFY_ARE_EQUAL(candidate, a);
        }

        /*
            +-------------------------------------------------------------------+
            |                                                                   |
            |                                                                   |
            |                                                                   |
            |                       +------------+                              |
            |                       |            |                              |
            |                       |            |                              |
            |                       |            |                              |
            |                       |            |                              |
            |                       |            |                              |                               +--------------+
            |                       |            |                              |                               |              |
            |                       |            |                              |                               |              |
            |                       |            |                              |      Very far apart           |          a   |
            |                       |            |                              |                               +--------------+
            |                       |            |                              |
            |                       |       start|                              |
            |                       +------------+                              |
            |                                                                   |
            |                                                                   |
            |                                                                   |
            |                                                                   |
            |                                                              root |
            +-------------------------------------------------------------------+
        */

        void BasicAlgorithmUnitTests::ElementScrolledExtremleyOutOfViewShouldStillBeSelected()
        {
            XRECTF_RB start = { 120, 20, 240, 260 };
            XRECTF_RB a = { 10000, 120, 10080, 200 };

            DirectUI::FocusNavigationDirection direction = DirectUI::FocusNavigationDirection::Right;

            std::pair<double, double> vManifold = std::make_pair(-1.0, -1.0);
            std::pair<double, double> hManifold = std::make_pair(-1.0, -1.0);

            std::vector<XRECTF_RB> scenario;
            scenario.push_back(start);
            scenario.push_back(a);

            XRECTF_RB candidate = BestElement(scenario, start, direction, DirectUI::XYFocusNavigationStrategy::Projection, &hManifold, &vManifold);
            VERIFY_ARE_EQUAL(candidate, a);

            candidate = BestElement(scenario, start, direction, DirectUI::XYFocusNavigationStrategy::NavigationDirectionDistance);
            VERIFY_ARE_EQUAL(candidate, a);

            candidate = BestElement(scenario, start, direction, DirectUI::XYFocusNavigationStrategy::RectilinearDistance);
            VERIFY_ARE_EQUAL(candidate, a);
        }

    }}}
}}}}    
