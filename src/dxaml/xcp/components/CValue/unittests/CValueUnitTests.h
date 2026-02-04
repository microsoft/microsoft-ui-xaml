// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>

namespace Windows { namespace UI { namespace Xaml { namespace Tests {
    namespace CValue {

        class CValueUnitTests : public WEX::TestClass<CValueUnitTests>
        {
        public:
            BEGIN_TEST_CLASS(CValueUnitTests)
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_CLASS_CLEANUP(ClassCleanup)

            BEGIN_TEST_METHOD(ValidateCopyCtor)
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateMoveCtor)
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateIsNull)
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateIsUnset)
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateIsFloatingPoint)
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateIsArray)
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateCompare)
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateCompareWrappedIPV)
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateAccessorsForValues)
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateAccessorsForReferences)
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateAccessorsForArrays)
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateAccessorsForRefCounted)
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateWrapValue)
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateCopyValue)
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CopyFromDouble)
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(FloatAsDouble)
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(GetSetColor)
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(UnknownTypeIsNull)
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CanCorrectlyCompareDateAndTime)
                TEST_METHOD_PROPERTY(L"Description", L"Validates DateTime comparison so DateChanged event doesn't fire when the selected date hasn't changed.")
            END_TEST_METHOD()
        };
    }}
} } }
