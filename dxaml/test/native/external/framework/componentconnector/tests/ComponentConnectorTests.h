// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

#include <RuntimeEnabledFeatureOverride.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Framework {
        class ComponentConnectorTests : public WEX::TestClass<ComponentConnectorTests>
        {
        public:
            BEGIN_TEST_CLASS(ComponentConnectorTests)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_CLASS_CLEANUP(ClassCleanup)
            TEST_METHOD_SETUP(TestSetup)
            TEST_METHOD_CLEANUP(TestCleanup)

            BEGIN_TEST_METHOD(ValidateConnectOnIComponentConnectorGetsCalled)
                TEST_METHOD_PROPERTY(L"Classification", L"Integration")
                TEST_METHOD_PROPERTY(L"Platform", L"Any")
                TEST_METHOD_PROPERTY(L"BCQ", L"TRUE")
                TEST_METHOD_PROPERTY(L"Description", L"Validates basic ICC scenario")
                TEST_METHOD_PROPERTY(L"EnabledOnOneCore", L"TRUE")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateConnectOnIComponentConnectorGetsCalledForDeferredElements)
                TEST_METHOD_PROPERTY(L"Classification", L"Integration")
                TEST_METHOD_PROPERTY(L"Platform", L"Any")
                TEST_METHOD_PROPERTY(L"BCQ", L"TRUE")
                TEST_METHOD_PROPERTY(L"Description", L"Validates ICC is called for deferred elements")
                TEST_METHOD_PROPERTY(L"EnabledOnOneCore", L"TRUE")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            END_TEST_METHOD()

        private:
            Microsoft::UI::Xaml::Tests::Common::RuntimeEnabledFeatureOverride featureEnforceXbfV2Stream;
        };
    }
} } } }
