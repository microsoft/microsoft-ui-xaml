// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>

namespace Windows { namespace UI { namespace Xaml { namespace Tests {
    namespace Qualifiers {

        class IntegrationTests : public WEX::TestClass<IntegrationTests>
        {
        public:
            BEGIN_TEST_CLASS(IntegrationTests)
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_CLASS()

            BEGIN_TEST_METHOD(QualifierContextWithVariantMaps)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies VariantMaps works with a QualifierContext instance.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(MinWidth)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies MinWidthQualifier end-to-end with VM and QC.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(MinHeight)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies MinHeightQualifier end-to-end with VM and QC.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(MultiQualifierWidthHeight)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies MinHeightQualifier end-to-end with VM and QC.")
            END_TEST_METHOD()
        };
    }
} } } }

