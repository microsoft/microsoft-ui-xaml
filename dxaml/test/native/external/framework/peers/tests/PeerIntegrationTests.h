// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <XamlMetadataProviderOverrider.h>
#include <vector>
#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Framework { namespace Peers {

    class PeerIntegrationTests : public WEX::TestClass<PeerIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(PeerIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")

            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"f62323d4-fd46-4c98-aa85-334db95ba8f6;27f34780-4ef6-4102-929f-29737dfda1b9;0a9cdf5f-1e1b-4b1d-9659-b354bf5f4ca6")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)

        TEST_METHOD_CLEANUP(TestCleanup)
        
        BEGIN_TEST_METHOD(ParentPeerNotCreated)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that parent peer is not unnecessarily created.")
        END_TEST_METHOD()
    };
} } } } } }
