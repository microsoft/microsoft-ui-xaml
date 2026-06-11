// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>
#include "CDependencyObject.h"
#include "MinPal.h"
#include "TypeTableStructs.h"

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Animation {

class DoubleAnimationUnitTests : public WEX::TestClass<DoubleAnimationUnitTests>
{
public:
    BEGIN_TEST_CLASS(DoubleAnimationUnitTests)
        TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        TEST_CLASS_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
    END_TEST_CLASS()

    TEST_METHOD(ValidateDurationWUC)

    TEST_METHOD(ValidateReverseWUC)

    TEST_METHOD(ValidateRepeatWUC)

    BEGIN_TEST_METHOD(ValidateBeginTimeWUC)
#if defined(_WIN64) // Disable the test on 64-bit build: AreEqual(static_cast<INT64>(beginTime * 1000 * 1000 * 10), mock->m_delayTime.Duration) - Values (30999999, 31000000) 
        TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")
#endif
    END_TEST_METHOD()

    TEST_METHOD(ValidateSpeedRatioWUC)

    TEST_METHOD(ValidateOffsetWUCAnimation)

    TEST_METHOD(ValidateTranslateWUCAnimation)

    TEST_METHOD(ValidateScaleWUCAnimation)

    TEST_METHOD(ValidateRotateWUCAnimation)

    TEST_METHOD(ValidateSkewWUCAnimation)

    TEST_METHOD(ValidateCompositeWUCAnimation)

    TEST_METHOD(ValidatePlaneProjectionWUCAnimation)

    TEST_METHOD(ValidateCompositeTransform3DWUCAnimation)

private:
    void ValidateOffsetAnimation(
        _In_ const wchar_t* outputText,
        KnownPropertyIndex propIndex,
        bool hasXAnimation,
        bool hasYAnimation
        );

    void ValidateTranslateAnimation(
        _In_ const wchar_t* outputText,
        KnownPropertyIndex propIndex,
        bool hasXAnimation,
        bool hasYAnimation
        );

    void ValidateScaleAnimation(
        _In_ const wchar_t* outputText,
        KnownPropertyIndex propIndex,
        bool hasCenterXAnimation,
        bool hasCenterYAnimation,
        bool hasScaleXAnimation,
        bool hasScaleYAnimation
        );

    void ValidateRotateAnimation(
        _In_ const wchar_t* outputText,
        KnownPropertyIndex propIndex,
        bool hasCenterXAnimation,
        bool hasCenterYAnimation,
        bool hasAngleAnimation
        );

    void ValidateSkewAnimation(
        _In_ const wchar_t* outputText,
        KnownPropertyIndex propIndex,
        bool hasCenterXAnimation,
        bool hasCenterYAnimation,
        bool hasAngleXAnimation,
        bool hasAngleYAnimation
        );

    void ValidateCompositeAnimation(
        _In_ const wchar_t* outputText,
        KnownPropertyIndex propIndex,
        bool hasCenterXAnimation,
        bool hasCenterYAnimation,
        bool hasScaleXAnimation,
        bool hasScaleYAnimation,
        bool hasSkewXAnimation,
        bool hasSkewYAnimation,
        bool hasRotationAnimation,
        bool hasTranslateXAnimation,
        bool hasTranslateYAnimation
        );

    void ValidatePlaneProjectionAnimation(
        _In_ const wchar_t* outputText,
        KnownPropertyIndex propIndex,
        bool hasCenterXAnimation,
        bool hasCenterYAnimation,
        bool hasCenterZAnimation,
        bool hasRotationXAnimation,
        bool hasRotationYAnimation,
        bool hasRotationZAnimation,
        bool hasLocalOffsetXAnimation,
        bool hasLocalOffsetYAnimation,
        bool hasLocalOffsetZAnimation,
        bool hasGlobalOffsetXAnimation,
        bool hasGlobalOffsetYAnimation,
        bool hasGlobalOffsetZAnimation
        );

    void ValidateCompositeTransform3DAnimation(
        _In_ const wchar_t* outputText,
        KnownPropertyIndex propIndex,
        bool hasCenterXAnimation,
        bool hasCenterYAnimation,
        bool hasCenterZAnimation,
        bool hasScaleXAnimation,
        bool hasScaleYAnimation,
        bool hasScaleZAnimation,
        bool hasRotationXAnimation,
        bool hasRotationYAnimation,
        bool hasRotationZAnimation,
        bool hasTranslateXAnimation,
        bool hasTranslateYAnimation,
        bool hasTranslateZAnimation
        );
};

} } } } } }
