// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

#include <XamlMetadataProviderOverrider.h>
#include <vector>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Framework { namespace Activation {

    class ActivationIntegrationTests
    {
    public:
        BEGIN_TEST_CLASS(ActivationIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)

        BEGIN_TEST_METHOD(CreateAFloatCollection)
            TEST_METHOD_PROPERTY(L"Description",
                L"Check that a FloatCollection can be instantiated by going directly through DllGetActivationFactory.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(CreateInputEventArgs)
        END_TEST_METHOD()
    };
} } } } } }
