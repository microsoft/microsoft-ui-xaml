// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Tools { namespace AppAnalysis {

        class CollapsedElementsRuleIntegrationTests : public WEX::TestClass<CollapsedElementsRuleIntegrationTests>
        {
        public:
            BEGIN_TEST_CLASS(CollapsedElementsRuleIntegrationTests)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.Diagnostics.AppAnalysis.dll")
                TEST_CLASS_PROPERTY(L"ArtifactUnderTest", L"sdk\\inc\\microsoft.diagnostics.appanalysis.idl")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
                TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"e22a917c-ad18-4a09-bff9-d3ca3e5ee0b8")
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"IsolationLevel", L"Class")
                TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // All AppAnalysis tests are unreliable or failing.
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_CLASS_CLEANUP(ClassCleanup)
            TEST_METHOD_SETUP(TestSetup)
            TEST_METHOD_CLEANUP(TestCleanup)

            BEGIN_TEST_METHOD(CorrectlyCountCollapsedElements)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that we get the collapsed element notification/count.")
                TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CorrectlyCountCollapsedElementsInTemplate)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that we get the collapsed element notification/count when it is in a DataTemplate.")
            END_TEST_METHOD()
                
            BEGIN_TEST_METHOD(VerifyElementNameWithoutSourceInfo)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that the description contains the name of the collapsed element.")
                TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyDoesntFlagWhenUsingEnC)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that the description contains the name of the collapsed element.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
                TEST_METHOD_PROPERTY(L"Ignore", L"TRUE")
            END_TEST_METHOD()

        private:
            Platform::String^ GetResourcesPath() const;
        };
       
    } } 
} } } }