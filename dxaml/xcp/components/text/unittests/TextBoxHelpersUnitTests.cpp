// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "TextBoxHelpers.h"
#include "TextBoxHelpersUnitTests.h"

namespace
{
    void VerifyNearZeroDimension(
        _In_ float initialOrigin,
        _In_ float initialFarCorner)
    {
        float origin = initialOrigin;
        float farCorner = initialFarCorner;
        float localDimension = 10.0f;

        VERIFY_IS_TRUE(TextBoxHelpers::Details::TryCollapseNearZeroSelectionDimension(
            origin,
            farCorner,
            localDimension));
        VERIFY_ARE_EQUAL(0.0f, localDimension);
        VERIFY_ARE_EQUAL(floorf(initialOrigin), origin);
        VERIFY_ARE_EQUAL(origin, farCorner);
    }
}

namespace Windows { namespace UI { namespace Xaml { namespace Tests {
    namespace Text {

void TextBoxHelpersUnitTests::NearZeroSelectionDimensionsRemainEmpty()
{
    VerifyNearZeroDimension(10.25f, 10.25f);
    VerifyNearZeroDimension(10.25f, 10.29f);
    VerifyNearZeroDimension(10.25f, 10.21f);

    float origin = 10.25f;
    float farCorner = 10.31f;
    float localDimension = 10.0f;
    VERIFY_IS_FALSE(TextBoxHelpers::Details::TryCollapseNearZeroSelectionDimension(
        origin,
        farCorner,
        localDimension));
    VERIFY_ARE_EQUAL(10.25f, origin);
    VERIFY_ARE_EQUAL(10.31f, farCorner);
    VERIFY_ARE_EQUAL(10.0f, localDimension);
}

    }
} } } }
