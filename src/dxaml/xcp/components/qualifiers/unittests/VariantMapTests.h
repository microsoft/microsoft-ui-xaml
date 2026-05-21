// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>

namespace Windows { namespace UI { namespace Xaml { namespace Tests {
    namespace Qualifiers {

        class VariantMapUnitTests : public WEX::TestClass<VariantMapUnitTests>
        {
        public:
            BEGIN_TEST_CLASS(VariantMapUnitTests)
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_CLASS()

            BEGIN_TEST_METHOD(AddQualifiers)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies qualifiers can be added to VariantMap.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(Evaluation)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies VariantMap evaluation when QualifierContext changes or item added.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(Selection)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies VariantMap returns select item or nullptr if no valid items exist.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VariantMapCallback)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies VariantMap notifies listeners on SelectedItem change.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(RemoveItem)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies an item in VariantMapItem can be removed.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(UpdateItem)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies an item in VariantMapItem can be updated.")
            END_TEST_METHOD()
        };
    }
} } } }

