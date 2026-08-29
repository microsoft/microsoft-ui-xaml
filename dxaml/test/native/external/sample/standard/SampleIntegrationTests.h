// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Sample {
        class SampleIntegrationTests : public WEX::TestClass<SampleIntegrationTests>
        {
        public:
            BEGIN_TEST_CLASS(SampleIntegrationTests)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")

                TEST_METHOD_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"dad50b0b-4b6e-4b44-ae60-31e745fd8f74;2848f544-9be5-4b98-bd69-bf3f2a4a04f3;a6e2c276-e8c0-4d93-80de-8aeb6ef87fd7")
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_METHOD_CLEANUP(TestCleanup)

            BEGIN_TEST_METHOD(CreateAGrid)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully create a grid.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(LoadedEvent)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can fire a Loaded event.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(LoadSingleXamlFile)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully load a single xaml file.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(LoadMultipleXamlFilesAsync)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that we can successfully load multiple xaml files asynchronously.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(SimpleDCompValidation)
                TEST_METHOD_PROPERTY(L"Description", L"Validates the DComp tree and surfaces using MockDComp.")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
                TEST_METHOD_PROPERTY(L"HasAssociatedMasterFile", L"True")
            END_TEST_METHOD()

        private:
            Platform::String^ GetPathToFiles() const;
        };
    }
} } } }

