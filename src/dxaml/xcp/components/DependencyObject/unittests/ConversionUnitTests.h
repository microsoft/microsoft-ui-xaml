// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Framework { namespace DependencyObject {

        class ConversionUnitTests : public WEX::TestClass<ConversionUnitTests>
        {
        public:
            BEGIN_TEST_CLASS(ConversionUnitTests)
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_CLASS_PROPERTY(L"FeatureUnderTest", L"@ClassName = 'CDependencyObject' AND @FunctionName = 'RepackageValue*'")
            END_TEST_CLASS()

            TEST_METHOD(CanRepackageDoubleAsInt)

            TEST_METHOD(CanConvertFloatToBool)
            TEST_METHOD(CanConvertFloatToDouble)
            TEST_METHOD(CanConvertFloatToEnum)
            TEST_METHOD(CanConvertFloatToInt)
            TEST_METHOD(CanConvertFloatToRect)
            TEST_METHOD(CannotConvertFloatToDateTime)
            TEST_METHOD(CanConvertFloatToDependencyObject)

            TEST_METHOD(CanConvertFloatToThicknessInPlace)
            TEST_METHOD(CanConvertFloatToGridLengthInPlace)
            TEST_METHOD(CanConvertFloatToAutoGridLengthInPlace)
            TEST_METHOD(CanConvertFloatToCornerRadiusInPlace)

            TEST_METHOD(CanConvertIntToBool)
            TEST_METHOD(CanConvertIntToColor)
            TEST_METHOD(CanConvertIntToFloat)
            TEST_METHOD(CanConvertIntToDouble)
            TEST_METHOD(CannotConvertIntToDateTime)
            TEST_METHOD(CannotConvertIntToDependencyObject)

            TEST_METHOD(CanConvertBoolToEnum)
            TEST_METHOD(CanConvertBoolToInt)
            TEST_METHOD(CanConvertEnumToBool)
            TEST_METHOD(CanConvertEnumToInt)
            TEST_METHOD(CannotConvertBoolToDependencyObject)
            TEST_METHOD(CannotConvertEnumToDependencyObject)

            TEST_METHOD(CanConvertStringToBool)

            TEST_METHOD(CannotConvertObjectToString)
            
            TEST_METHOD(CanConvertDoubleToGridLength)
            TEST_METHOD(CanConvertDoubleToAutoGridLength)

            TEST_METHOD(ConversionFromBadStringToBool)

            TEST_METHOD(DoubleGetsConvertedToFloat)
            TEST_METHOD(IntObjectGetsConvertedToIntCValue)
            TEST_METHOD(StringObjectGetsConvertedToStringCValue)
        };
    }}
} } } }
