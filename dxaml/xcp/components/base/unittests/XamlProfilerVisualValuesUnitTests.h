// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace Base {

    class XamlProfilerVisualValuesUnitTests
    {
    public:
        BEGIN_TEST_CLASS(XamlProfilerVisualValuesUnitTests)
        END_TEST_CLASS()

        BEGIN_TEST_METHOD(ValidateRepeatedPropertyIsSkipped)
            TEST_METHOD_PROPERTY(L"Classification", L"Integration")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Description", L"Validates that a scalar or vector visual property is written only when it differs, bit for bit, from the value last written for that visual and property")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateRepeatedTransformIsSkipped)
            TEST_METHOD_PROPERTY(L"Classification", L"Integration")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Description", L"Validates that a visual's transform matrix is written only when it differs from the one last written")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateOnlyRepeatedClearIsSkipped)
            TEST_METHOD_PROPERTY(L"Classification", L"Integration")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Description", L"Validates that an object-valued visual property is always written, except a clear that follows a clear")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateVisualCreatedForgetsItsValues)
            TEST_METHOD_PROPERTY(L"Classification", L"Integration")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Description", L"Validates that WucVisualCreated forgets the values recorded for that visual only")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateSessionChangeForgetsValues)
            TEST_METHOD_PROPERTY(L"Classification", L"Integration")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Description", L"Validates that enabling or disabling the provider, or a capture-state request, forgets every recorded value")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateCapacityForgetsValues)
            TEST_METHOD_PROPERTY(L"Classification", L"Integration")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Description", L"Validates that recording one visual more than c_visualValuesCapacity forgets the values of the others")
        END_TEST_METHOD()
    };

} } } } }
