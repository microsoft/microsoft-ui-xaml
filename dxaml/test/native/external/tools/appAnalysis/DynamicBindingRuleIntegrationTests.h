// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>
#include <XamlMetadataProviderOverrider.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Tools { namespace AppAnalysis {

        class DynamicBindingRuleIntegrationTests : public WEX::TestClass<DynamicBindingRuleIntegrationTests>
        {
        public:
            BEGIN_TEST_CLASS(DynamicBindingRuleIntegrationTests)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.Diagnostics.AppAnalysis.dll")
                TEST_CLASS_PROPERTY(L"ArtifactUnderTest", L"sdk\\inc\\microsoft.diagnostics.appanalysis.idl")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
                TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"e22a917c-ad18-4a09-bff9-d3ca3e5ee0b8")
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // All AppAnalysis tests are unreliable or failing.
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_CLASS_CLEANUP(ClassCleanup)
            TEST_METHOD_SETUP(TestSetup)
            TEST_METHOD_CLEANUP(TestCleanup)

            BEGIN_TEST_METHOD(CorrectlyCatchesBasicBinding)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that we correctly flag binding in the most basic scenario.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CorrectlyCatchesBindingInDataTemplate)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that we correctly flag binding inside of a data template where x:Bind could've been used.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(DoesntFlagBindingUsingSource)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that we don't flag when Binding uses Source property.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(DoesntFlagUsesOfTemplateBinding)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that we don't flag uses of TemplateBinding inside ControlTemplate.")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(DoesntFlagUsesOfTemplatedParent)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that we don't flag uses of Binding inside ControlTemplate when binding to TemplatedParent.")
            END_TEST_METHOD()

        private:
            Platform::String^ GetResourcesPath() const;
        };
       
    } } 
} } } }
