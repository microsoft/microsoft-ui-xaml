// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Framework { namespace Collections {

    class VectorIntegrationTests : public WEX::TestClass<VectorIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(VectorIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"f62323d4-fd46-4c98-aa85-334db95ba8f6;27f34780-4ef6-4102-929f-29737dfda1b9")
            TEST_METHOD_PROPERTY(L"Classification", L"Integration")
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_METHOD_SETUP(TestSetup)
            TEST_METHOD_CLEANUP(TestCleanup)

            //
            // Platform:Any
            //
            BEGIN_TEST_METHOD(VerifyVectorOperationsBounds)
                TEST_METHOD_PROPERTY(L"Description", L"Validates vector operations and bounds exceptions.")
            END_TEST_METHOD()

    };

} } } } } } // Microsoft::UI::Xaml::Tests::Framework::Layout
