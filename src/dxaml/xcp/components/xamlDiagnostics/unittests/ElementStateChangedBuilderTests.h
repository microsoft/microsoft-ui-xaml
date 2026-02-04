// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "precomp.h"
#include <XamlLogging.h>
#include <WexTestClass.h>

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace XamlDiagnostics {
    class ElementStateChangedBuilderUnitTests : public WEX::TestClass<ElementStateChangedBuilderUnitTests>
    {
    public:
            BEGIN_TEST_CLASS(ElementStateChangedBuilderUnitTests)
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_CLASS_CLEANUP(ClassCleanup)

            BEGIN_TEST_METHOD(TestDictionaryAccessor)
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestArrayAccessor)
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestImplicitStyleAccessor)
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(TestMultiStepAccessor)
            END_TEST_METHOD()
    };
}}}} }
