// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>
#include <FeatureFlags.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Activation {

    class ActivationTests : public WEX::TestClass<ActivationTests>
    {
    public:
        BEGIN_TEST_CLASS(ActivationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"IsolationLevel", L"Test")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)

        //
        // Platform:Any
        //

        BEGIN_TEST_METHOD(VerifyCanActivateTypes)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that all types marked as activatable are, in fact, activatable.")
        END_TEST_METHOD()
    };

} } } } }