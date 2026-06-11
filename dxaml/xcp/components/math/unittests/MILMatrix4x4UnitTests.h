// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>
#include <FeatureFlags.h>

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Math {

class MILMatrix4x4UnitTests : public WEX::TestClass<MILMatrix4x4UnitTests>
{
public:
    BEGIN_TEST_CLASS(MILMatrix4x4UnitTests)
        TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        TEST_CLASS_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
    END_TEST_CLASS()

    TEST_METHOD(ValidateTransformWorldToLocal_Identity)

    TEST_METHOD(ValidateTransformWorldToLocal_TranslateZ)
    TEST_METHOD(ValidateTransformWorldToLocal_TranslateXZ)

    TEST_METHOD(ValidateTransformWorldToLocal_RotationY)
    TEST_METHOD(ValidateTransformWorldToLocal_RotationX)

    TEST_METHOD(ValidateTransformWorldToLocal_ScaleXYZ)

    TEST_METHOD(ValidateTransformWorldToLocal_PerspectiveTranslateZ)

    TEST_METHOD(ValidateTransformWorldToLocal_PerspectiveRotationY)

private:
    void VerifyPoint(const XPOINTF& expected, const XPOINTF& actual, int index);
    void VerifyPoints(const CMILMatrix4x4& transform, const gsl::span<const XPOINTF>& input, const gsl::span<const XPOINTF>& expected);
    void VerifyPoints(const CMILMatrix4x4& transform, const gsl::span<const XPOINTF>& input, const gsl::span<const XPOINTF>& expected, const bool expectedWasTransformed);
};

} } } } } }
