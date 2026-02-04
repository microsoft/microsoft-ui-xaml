// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>

namespace Windows { namespace UI { namespace Xaml { namespace Tests {
    namespace Qualifiers {

        class QualifierContextUnitTests : public WEX::TestClass<QualifierContextUnitTests>
        {
        public:
            BEGIN_TEST_CLASS(QualifierContextUnitTests)
            END_TEST_CLASS()

            BEGIN_TEST_METHOD(HeightContextChanged)
                TEST_METHOD_PROPERTY(L"Classification", L"Integration")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Description", L"Verifies a QualifierContext notifies listeners on Height context changed.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(WidthContextChanged)
                TEST_METHOD_PROPERTY(L"Classification", L"Integration")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Description", L"Verifies a QualifierContext notifies listeners on Width context changed.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(PlatformContextChanged)
                TEST_METHOD_PROPERTY(L"Classification", L"Integration")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Description", L"Verifies a QualifierContext notifies listeners on Platform context changed.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(UnregisterListener)
                TEST_METHOD_PROPERTY(L"Classification", L"Integration")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Description", L"Verifies a QualifierContext can unregister listener.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(UpdateListener)
                TEST_METHOD_PROPERTY(L"Classification", L"Integration")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Description", L"Verifies a listener can update the context changes it is interested in.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ListenersAreCalledBack)
                TEST_METHOD_PROPERTY(L"Classification", L"Integration")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Description", L"Verifies callback mechanism of QualifierContext.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(MultipleListeners)
                TEST_METHOD_PROPERTY(L"Classification", L"Integration")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Description", L"Verifies a QualifierContext handles a large number of listeners.")
            END_TEST_METHOD()
        };
    }
} } } }
