// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "precomp.h"
#include <XamlLogging.h>
#include <CDependencyObject.h>
#include <KeytipAutomaticPlacementAlgorithm.h>
#include <Keytip.h>
#include <WexTestClass.h>

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace Input { namespace KeyTips {

    class KeytipAutomaticPlacementAlgorithmUnitTests : public WEX::TestClass<KeytipAutomaticPlacementAlgorithmUnitTests>
    {
    public:
        BEGIN_TEST_CLASS(KeytipAutomaticPlacementAlgorithmUnitTests)
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
        END_TEST_CLASS()

        BEGIN_TEST_METHOD(VerifyKeytipsPreferBottom)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that we prefer placing a KeyTip at the bottom of its parent.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyKeytipsChooseAlignment)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that KeyTips choose to align when multiple good positions are available.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyDoesNotOccludeAccessKeyElements)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that KeyTips would rather occlude their parent element rather than other elements with access keys enabled.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyKeytipMustFitOnScreen)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that a KeyTip must find a valid position on screen, else it is hidden.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyOffScreenKeytipMovedOnScreen)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that a KeyTip must find a valid position on screen, else it is hidden.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyUnplaceableKeytipHidden)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that a KeyTip with no possible placement is hidden.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyRightToLeftMovesKeyTipPosition)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that left to right placement works.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyMinimizesFocusableElementOverlap)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that KeyTips attempt to minimize overlap with Focusable elements.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyAlignmentWithManuallyPositionedKeyTips)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that KeyTips attempt to maximize alignment with manually positioned KeyTips.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyTwoElementColumnsAlignKeytips)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that KeyTips attempt to maximize alignment even in a two element column.")
        END_TEST_METHOD()
    };

}}}}}}
