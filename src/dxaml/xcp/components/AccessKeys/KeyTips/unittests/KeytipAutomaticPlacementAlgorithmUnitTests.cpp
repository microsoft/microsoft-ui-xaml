// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.


#include "precomp.h"
#include "KeytipAutomaticPlacementAlgorithmUnitTests.h"

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace Input { namespace KeyTips {

    using namespace DirectUI;


    /*

                +---------------------------+
                |                           |
                |         Element           |
                |                           |
                |                           |
                |                           |
                +--------+--------+---------+
                         | Keytip |
                         +--------+

    */

    void KeytipAutomaticPlacementAlgorithmUnitTests::VerifyKeytipsPreferBottom()
    {
        XRECTF_RB testScreenBounds = { 0, 0, 1000, 1000 };
        XRECTF_RB finalKeytipPosition = { 130, 200, 170, 210 };

        std::vector<KeyTip> activeElementsOnScreen;
        std::vector<XRECTF_RB> focusableElementBounds;
        XRECTF_RB objectBounds = { 100, 100, 200, 200 };
        XRECTF_RB keyTipBounds = { 0, 0, 40, 10 };

        KeyTip element1(objectBounds, keyTipBounds);
        element1.PlacementMode = DirectUI::KeyTipPlacementMode::Auto;

        activeElementsOnScreen.push_back(std::move(element1));

        KeytipAutomaticPlacementAlgorithm::PositionKeyTips(
            activeElementsOnScreen,
            testScreenBounds,
            focusableElementBounds,
            nullptr /*xamlIslandRoot*/,
            false /*enableMonitorDetection*/);

        VERIFY_ARE_EQUAL(activeElementsOnScreen[0].KeytipBounds, finalKeytipPosition);

    }

    /*
                        +-----+-------------------+
                        |Key  |   Element0        |
                        |Tip1 |                   |
                        +-------------------------+
                        |Key  |   Element1        |
                        |Tip2 |                   |
                        +-------------------------+
                        |Key  |   Element2        |
                        |Tip3 |                   |
                        +-----+-------------------+

    */
    void KeytipAutomaticPlacementAlgorithmUnitTests::VerifyKeytipsChooseAlignment()
    {
        XRECTF_RB testScreenBounds = { 0, 0, 1000, 1000 };
        XRECTF_RB keyTipBounds = { 0, 0, 10, 10 };
        const int num_elements = 3;
        std::vector<KeyTip> activeElementsOnScreen;
        std::vector<XRECTF_RB> focusableElementBounds;
        std::vector<XRECTF_RB> finalKeytipPositions;

        for (int i = 0; i < num_elements; i++)
        {
            XRECTF_RB objectBounds = { 10.0f, (i + 1) * 10.0f, 30.0f, (i + 2)*10.0f };
            XRECTF_RB expectedKeytipBounds = { 0, (i + 1) * 10.0f, 10.0f, (i + 2) * 10.0f };

            finalKeytipPositions.push_back(expectedKeytipBounds);
            KeyTip element(objectBounds, keyTipBounds);
            element.PlacementMode = DirectUI::KeyTipPlacementMode::Auto;
            activeElementsOnScreen.push_back(std::move(element));
        }

        KeytipAutomaticPlacementAlgorithm::PositionKeyTips(
            activeElementsOnScreen,
            testScreenBounds,
            focusableElementBounds,
            nullptr /*xamlIslandRoot*/,
            false /*enableMonitorDetection*/);

        for (int i = 0; i < num_elements; i++)
        {
            VERIFY_ARE_EQUAL(activeElementsOnScreen[i].KeytipBounds, finalKeytipPositions[i]);
        }
    }

    /*

                             +---------+---------+---------+
                             | KeyTip0 | KeyTip3 | KeyTip6 |
                             |         |         |         |
                             |         |         |         |
                             |         |         |         |
                             +-----------------------------+
                             |         |         |         |
                             | Element0| Element3| Element6|
                             |         |         |         |
                             |         |         |         |
                    +------------------------------------------------+
                    |        |         |         |         |         |
                    | KeyTip1| Element1| Element4| Element7| KeyTip7 |
                    |        |         |& KeyTip4|         |         |
                    |        |         |         |         |         |
                    +------------------------------------------------+
                             |         |         |         |
                             | Element2| Element5| Element8|
                             |         |         |         |
                             |         |         |         |
                             +-----------------------------+
                             |         |         |         |
                             | Keytip2 |  KeyTip5| KeyTip8 |
                             |         |         |         |
                             |         |         |         |
                             +---------+---------+---------+


    */
    void KeytipAutomaticPlacementAlgorithmUnitTests::VerifyDoesNotOccludeAccessKeyElements()
    {
        XRECTF_RB testScreenBounds = { 0, 0, 1000, 1000 };
        XRECTF_RB keyTipBounds = { 0, 0, 10, 10 };
        const int num_rows = 3;
        const int num_columns = 3;
        std::vector<KeyTip> activeElementsOnScreen;
        std::vector<XRECTF_RB> focusableElementBounds;
        //Establishing object positions
        for (int x = 0; x < num_rows; x++)
        {
            for (int y = 0; y < num_columns; y++)
            {
                XRECTF_RB objectBounds = { (x + 1)*10.0f ,(y + 1)*10.0f, (x + 2)*10.0f, (y + 2)*10.0f };
                KeyTip element( objectBounds, keyTipBounds);
                element.PlacementMode = DirectUI::KeyTipPlacementMode::Auto;
                activeElementsOnScreen.push_back(std::move(element));
            }
        }

        XRECTF_RB expectedKeytipBounds;

        std::vector<XRECTF_RB> finalKeytipPositions;

        expectedKeytipBounds = { 10, 0, 20, 10 };
        finalKeytipPositions.push_back(expectedKeytipBounds);

        expectedKeytipBounds = { 0, 20, 10, 30 };
        finalKeytipPositions.push_back(expectedKeytipBounds);

        expectedKeytipBounds = { 10, 40, 20, 50 };
        finalKeytipPositions.push_back(expectedKeytipBounds);

        expectedKeytipBounds = { 20, 0, 30, 10 };
        finalKeytipPositions.push_back(expectedKeytipBounds);

        expectedKeytipBounds = { 20, 20, 30, 30 };
        finalKeytipPositions.push_back(expectedKeytipBounds);

        expectedKeytipBounds = { 20, 40, 30, 50 };
        finalKeytipPositions.push_back(expectedKeytipBounds);

        expectedKeytipBounds = { 30, 0, 40, 10 };
        finalKeytipPositions.push_back(expectedKeytipBounds);

        expectedKeytipBounds = { 40, 20, 50, 30 };
        finalKeytipPositions.push_back(expectedKeytipBounds);

        expectedKeytipBounds = { 30, 40, 40, 50 };
        finalKeytipPositions.push_back(expectedKeytipBounds);

        KeytipAutomaticPlacementAlgorithm::PositionKeyTips(
            activeElementsOnScreen,
            testScreenBounds,
            focusableElementBounds,
            nullptr /*xamlIslandRoot*/,
            false /*enableMonitorDetection*/);

        for (int i = 0; i < (num_columns*num_rows); i++)
        {
            VERIFY_ARE_EQUAL(activeElementsOnScreen[i].KeytipBounds, finalKeytipPositions[i]);
        }
    }

    /*
            +--------------+-----------+
            |              |           |
            |    Screen    |           |
            |              |           |
            |              |           |
            +--------------+           |
            | KeyTip                   |
            |                          |
            |                          |
            |                          |
            |                          |
            +--------------------------+

            then keytip is hidden


    */
    void KeytipAutomaticPlacementAlgorithmUnitTests::VerifyKeytipMustFitOnScreen()
    {
        XRECTF_RB testScreenBounds = { 0, 0, 10, 10 };

        std::vector<KeyTip> activeElementsOnScreen;
        std::vector<XRECTF_RB> focusableElementBounds;
        XRECTF_RB objectBounds = { 100, 100, 200, 200 };
        XRECTF_RB keytipBounds = { 0, 0, 40, 10 };

        KeyTip element1(objectBounds, keytipBounds);
        element1.PlacementMode = DirectUI::KeyTipPlacementMode::Auto;
        activeElementsOnScreen.push_back(std::move(element1));

        KeytipAutomaticPlacementAlgorithm::PositionKeyTips(
            activeElementsOnScreen,
            testScreenBounds,
            focusableElementBounds,
            nullptr /*xamlIslandRoot*/,
            false /*enableMonitorDetection*/);

        VERIFY_ARE_EQUAL(activeElementsOnScreen[0].PlacementMode, DirectUI::KeyTipPlacementMode::Hidden);

    }

    /*
            +------------------------+
            |----+   |       |       |
            | KT1|   |Element|KeyTip |
            |----+   |   2   |   2   |
            |Element0|       |       |
            +------------------------+
            |Element1|Element| KeyTip|
            |        |3 &    |   3   |
            |        |KeyTip1|       |
            +------------------------+

            KT1 : KeyTip1
    */
    void KeytipAutomaticPlacementAlgorithmUnitTests::VerifyOffScreenKeytipMovedOnScreen()
    {
        XRECTF_RB testScreenBounds = { 0, 0, 1000, 1000 };
        XRECTF_RB elem0KeyTipBounds = { 0, 0, 10, 10 };
        XRECTF_RB otherElemsKeyTipBounds = { 0, 0, 20, 20 };
        XRECTF_RB finalKeytipPosition = {0, 5, 10, 15};
        std::vector<XRECTF_RB> focusableElementBounds;
        std::vector<KeyTip> activeElementsOnScreen;

        XRECTF_RB objectBounds0 = { 0, 0, 20, 20 };
        XRECTF_RB objectBounds1 = { 0, 20, 20, 40 };
        XRECTF_RB objectBounds2 = { 20, 0, 40, 20 };
        XRECTF_RB objectBounds3 = { 20, 20, 40, 40 };

        KeyTip element0( objectBounds0, elem0KeyTipBounds);
        element0.PlacementMode = DirectUI::KeyTipPlacementMode::Auto;

        KeyTip element1(objectBounds1, otherElemsKeyTipBounds);
        element1.PlacementMode = DirectUI::KeyTipPlacementMode::Right;
        KeyTip element2(objectBounds2, otherElemsKeyTipBounds);
        element2.PlacementMode = DirectUI::KeyTipPlacementMode::Right;
        KeyTip element3(objectBounds3, otherElemsKeyTipBounds);
        element3.PlacementMode = DirectUI::KeyTipPlacementMode::Right;

        activeElementsOnScreen.push_back(std::move(element0));
        activeElementsOnScreen.push_back(std::move(element1));
        activeElementsOnScreen.push_back(std::move(element2));
        activeElementsOnScreen.push_back(std::move(element3));

        KeytipAutomaticPlacementAlgorithm::PositionKeyTips(
            activeElementsOnScreen,
            testScreenBounds,
            focusableElementBounds,
            nullptr /*xamlIslandRoot*/,
            false /*enableMonitorDetection*/);

        VERIFY_ARE_EQUAL(activeElementsOnScreen[0].KeytipBounds, finalKeytipPosition);

    }

    /*
        +--------------------------+
        |  Screen                  |
        |  Element0                |
        |  Keytip0                 |
        |  Element1                |
        |                          |
        |  (all exactly            |
        |  overlapped)             |
        |                          |
        |                          |
        +--------------------------+

        KeyTip1 should be hidden

    */
    void KeytipAutomaticPlacementAlgorithmUnitTests::VerifyUnplaceableKeytipHidden()
    {
        XRECTF_RB testScreenBounds = { 0, 0, 10, 10 };
        XRECTF_RB keyTipBounds = { 0, 0, 10, 10 };
        XRECTF_RB elementBounds = { 0, 0, 10, 10 };

        std::vector<KeyTip> activeElementsOnScreen;
        std::vector<XRECTF_RB> focusableElementBounds;
        KeyTip element0(elementBounds, keyTipBounds);
        element0.PlacementMode = DirectUI::KeyTipPlacementMode::Center;
        element0.HorizontalOffset = 0;
        element0.VerticalOffset = 0;

        KeyTip element1(elementBounds, keyTipBounds);
        element1.PlacementMode = DirectUI::KeyTipPlacementMode::Auto;


        activeElementsOnScreen.push_back(std::move(element0));
        activeElementsOnScreen.push_back(std::move(element1));


        KeytipAutomaticPlacementAlgorithm::PositionKeyTips(
            activeElementsOnScreen,
            testScreenBounds,
            focusableElementBounds,
            nullptr /*xamlIslandRoot*/,
            false /*enableMonitorDetection*/);

        VERIFY_ARE_EQUAL(activeElementsOnScreen[1].PlacementMode, DirectUI::KeyTipPlacementMode::Hidden);

    }
    /* Placement Mode: Right
        +--------+  +----------+
        |        |  |          |
        | KeyTip |  | Parent   |
        |        |  |          |
        |        |  |          |
        +--------+  +----------+
    */
    void KeytipAutomaticPlacementAlgorithmUnitTests::VerifyRightToLeftMovesKeyTipPosition()
    {
        XRECTF_RB testScreenBounds = { 0, 0, 1000, 1000 };
        XRECTF_RB finalKeytipPosition = { 90, 100, 190, 200 };

        std::vector<KeyTip> activeElementsOnScreen;
        std::vector<XRECTF_RB> focusableElementBounds;
        XRECTF_RB objectBounds = { 200, 100, 300, 200 };
        XRECTF_RB keyTipBounds = { 0, 0, 100, 100 };

        KeyTip element1(objectBounds, keyTipBounds);
        element1.PlacementMode = DirectUI::KeyTipPlacementMode::Right;
        element1.IsRightToLeft = true;
        element1.HorizontalOffset = 10;
        activeElementsOnScreen.push_back(std::move(element1));

        KeytipAutomaticPlacementAlgorithm::PositionKeyTips(
            activeElementsOnScreen,
            testScreenBounds,
            focusableElementBounds,
            nullptr /*xamlIslandRoot*/,
            false /*enableMonitorDetection*/);

        VERIFY_ARE_EQUAL(activeElementsOnScreen[0].KeytipBounds, finalKeytipPosition);

    }

    /*
        F: Focusable without access keys enabled
        AKE: Access Keys Enabled
        KT: KeyTip

             +----+
             | F  |
        +--------------+
        |    |AKE |    |
        | F  |    | KT |
        +--------------+
             |    |
             | F  |
             +----+

    */
    void KeytipAutomaticPlacementAlgorithmUnitTests::VerifyMinimizesFocusableElementOverlap()
    {
        XRECTF_RB testScreenBounds = { 0, 0, 1000, 1000 };
        XRECTF_RB finalKeytipPosition = { 20, 10, 30, 20 };

        std::vector<KeyTip> activeElementsOnScreen;
        std::vector<XRECTF_RB> focusableElements;

        XRECTF_RB focusableElement1 = { 0, 10, 10, 20 };
        XRECTF_RB focusableElement2 = { 10, 0, 20, 10 };
        XRECTF_RB focusableElement3 = { 10, 20, 20, 30 };

        focusableElements.push_back(focusableElement1);
        focusableElements.push_back(focusableElement2);
        focusableElements.push_back(focusableElement3);

        XRECTF_RB objectBounds = { 10, 10, 20, 20 };
        XRECTF_RB keytipBounds = { 0, 0, 10, 10 };

        KeyTip element(objectBounds, keytipBounds);
        element.PlacementMode = DirectUI::KeyTipPlacementMode::Auto;
        activeElementsOnScreen.push_back(std::move(element));

        KeytipAutomaticPlacementAlgorithm::PositionKeyTips(
            activeElementsOnScreen,
            testScreenBounds,
            focusableElements,
            nullptr /*xamlIslandRoot*/,
            false /*enableMonitorDetection*/);

        VERIFY_ARE_EQUAL(activeElementsOnScreen[0].KeytipBounds, finalKeytipPosition);

    }

        /*
        +-------------------------------------+---------------+
        |       Element0                      |               |
        |                                     |    Keytip     |
        |                                     |    (Auto)     |
        +-----------------------------------------------------+
        |        Element1                     |               |
        |                                     |     Keytip    |
        |                                     |     (Auto)    |
        +-----------------------------------------------------+
        |       Element2                      |               |
        |                                     |    Keytip     |
        |                                     |    (Auto)     |
        |                                     |               |
        +-----------------------------------------------------+
        |                                     |  KeyTip       |
        |       Element num_elements-1        |  num_elements |
        |                                     |   - 1         |
        |                                     |  (Right)      |
        +-------------------------------------+---------------+

        */

    void KeytipAutomaticPlacementAlgorithmUnitTests::VerifyAlignmentWithManuallyPositionedKeyTips()
    {
        XRECTF_RB testScreenBounds = { 0, 0, 1000, 1000 };
        XRECTF_RB keyTipBounds = { 0, 0, 10, 50 };
        const int num_elements = 14;
        std::vector<KeyTip> activeElementsOnScreen;
        std::vector<XRECTF_RB> focusableElementBounds;

        for (int i = 0; i < num_elements; i++)
        {
            XRECTF_RB objectBounds = { 10.0f, (i + 1) * 50.0f, 60.0f, (i + 2)*50.0f };

            KeyTip element(objectBounds, keyTipBounds);
            if (i ==  (num_elements - 1))
            {
                element.PlacementMode = DirectUI::KeyTipPlacementMode::Right;
            }
            else
            {
                element.PlacementMode = DirectUI::KeyTipPlacementMode::Auto;
            }
            activeElementsOnScreen.push_back(std::move(element));
        }

        KeytipAutomaticPlacementAlgorithm::PositionKeyTips(
            activeElementsOnScreen,
            testScreenBounds,
            focusableElementBounds,
            nullptr /*xamlIslandRoot*/,
            false /*enableMonitorDetection*/);

        for (int i = num_elements - 1; i >= 0; i--)
        {
            VERIFY_ARE_EQUAL(activeElementsOnScreen[i].PlacementMode, DirectUI::KeyTipPlacementMode::Right);
        }
    }

        /*


        +-----+-------------------+
        |Key  |   Element0        |
        |Tip1 |                   |
        +-------------------------+
        |Key  |   Element1        |
        |Tip2 |                   |
        +-------------------------+


        */

    void KeytipAutomaticPlacementAlgorithmUnitTests::VerifyTwoElementColumnsAlignKeytips()
    {
        XRECTF_RB testScreenBounds = { 0, 0, 1000, 1000 };
        XRECTF_RB keyTipBounds = { 0, 0, 10, 50 };
        const int num_elements = 2;
        std::vector<KeyTip> activeElementsOnScreen;
        std::vector<XRECTF_RB> focusableElementBounds;

        for (int i = 0; i < num_elements; i++)
        {
            XRECTF_RB objectBounds = { 10.0f, (i + 1) * 50.0f, 60.0f, (i + 2)*50.0f };

            KeyTip element(objectBounds, keyTipBounds);
            element.PlacementMode = DirectUI::KeyTipPlacementMode::Auto;
            activeElementsOnScreen.push_back(std::move(element));
        }

        KeytipAutomaticPlacementAlgorithm::PositionKeyTips(
            activeElementsOnScreen,
            testScreenBounds,
            focusableElementBounds,
            nullptr /*xamlIslandRoot*/,
            false /*enableMonitorDetection*/);

        for (int i = 0; i < num_elements; i++)
        {
            VERIFY_ARE_EQUAL(activeElementsOnScreen[i].PlacementMode, DirectUI::KeyTipPlacementMode::Left);
        }

    }

}}}}}}