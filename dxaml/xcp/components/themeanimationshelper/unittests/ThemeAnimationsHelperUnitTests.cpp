// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ThemeAnimationsHelperUnitTests.h"

#include <XamlLogging.h>

namespace tah = DirectUI::Components::ThemeAnimationsHelper;

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace ThemeAnimationsHelper {

    //
    // Test Cases
    //
    void ThemeAnimationsHelperUnitTests::ValidateGetLVIPPointerDownThemeAnimationTransformGroup()
    {
        bool needsTransformGroup = false;
        float controlWidth = 0.0f;
        float controlHeight = 0.0f;
        float halfDifferenceX = 0.0f;
        float halfDifferenceY = 0.0f;

        {
            controlWidth = 100.0f;
            controlHeight = 100.0f;
            needsTransformGroup = tah::DoesLVIPNeedTransformGroupForPointerDownThemeAnimation(200.0f /* containerWidth */,
                                                                                                200.0f /* containerHeight */,
                                                                                                controlWidth,
                                                                                                controlHeight,
                                                                                                halfDifferenceX,
                                                                                                halfDifferenceY);
            VERIFY_IS_FALSE(needsTransformGroup);
            VERIFY_ARE_EQUAL(controlWidth, 100.0f);
            VERIFY_ARE_EQUAL(controlHeight, 100.0f);
            VERIFY_ARE_EQUAL(halfDifferenceX, 0.0f);
            VERIFY_ARE_EQUAL(halfDifferenceY, 0.0f);
        }

        {
            controlWidth = 300.0f;
            controlHeight = 100.0f;
            needsTransformGroup = tah::DoesLVIPNeedTransformGroupForPointerDownThemeAnimation(200.0f /* containerWidth */,
                                                                                                200.0f /* containerHeight */,
                                                                                                controlWidth,
                                                                                                controlHeight,
                                                                                                halfDifferenceX,
                                                                                                halfDifferenceY);
            VERIFY_IS_TRUE(needsTransformGroup);
            VERIFY_ARE_EQUAL(controlWidth, 200.0f);
            VERIFY_ARE_EQUAL(controlHeight, 100.0f);
            VERIFY_ARE_EQUAL(halfDifferenceX, 50.0f);
            VERIFY_ARE_EQUAL(halfDifferenceY, 0.0f);
        }

        {
            controlWidth = 100.0f;
            controlHeight = 300.0f;
            needsTransformGroup = tah::DoesLVIPNeedTransformGroupForPointerDownThemeAnimation(200.0f /* containerWidth */,
                                                                                                200.0f /* containerHeight */,
                                                                                                controlWidth,
                                                                                                controlHeight,
                                                                                                halfDifferenceX,
                                                                                                halfDifferenceY);
            VERIFY_IS_TRUE(needsTransformGroup);
            VERIFY_ARE_EQUAL(controlWidth, 100.0f);
            VERIFY_ARE_EQUAL(controlHeight, 200.0f);
            VERIFY_ARE_EQUAL(halfDifferenceX, 0.0f);
            VERIFY_ARE_EQUAL(halfDifferenceY, 50.0f);
        }

        {
            controlWidth = 300.0f;
            controlHeight = 300.0f;
            needsTransformGroup = tah::DoesLVIPNeedTransformGroupForPointerDownThemeAnimation(200.0f /* containerWidth */,
                                                                                                200.0f /* containerHeight */,
                                                                                                controlWidth,
                                                                                                controlHeight,
                                                                                                halfDifferenceX,
                                                                                                halfDifferenceY);
            VERIFY_IS_TRUE(needsTransformGroup);
            VERIFY_ARE_EQUAL(controlWidth, 200.0f);
            VERIFY_ARE_EQUAL(controlHeight, 200.0f);
            VERIFY_ARE_EQUAL(halfDifferenceX, 50.0f);
            VERIFY_ARE_EQUAL(halfDifferenceY, 50.0f);
        }
    }

} } } } } }