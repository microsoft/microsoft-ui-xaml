// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>
#include <XamlMetadataProviderOverrider.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Tools { namespace AppAnalysis {

        class DuplicateAutomationNameRuleIntegrationTests : public WEX::TestClass<DuplicateAutomationNameRuleIntegrationTests>
        {
        public:
            BEGIN_TEST_CLASS(DuplicateAutomationNameRuleIntegrationTests)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.Diagnostics.AppAnalysis.dll")
                TEST_CLASS_PROPERTY(L"ArtifactUnderTest", L"sdk\\inc\\microsoft.diagnostics.appanalysis.idl")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // All AppAnalysis tests are unreliable or failing.
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_CLASS_CLEANUP(ClassCleanup)

            TEST_METHOD_CLEANUP(TestCleanup)

            BEGIN_TEST_METHOD(CatchesSameControlsWithSameName)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that we correctly fire a notification when two controls have the same parent, name, and control type")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(DoesntCatchDifferentControlsWithSameName)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that we don't fire a notification when two controls have same parent and name, but different control type")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(DoesntCatchSameControlsWithSameNameWithDifferentParent)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that we don't fire a notification when two controls have different parent, but same name and control type.")
            END_TEST_METHOD()


        private:
            Platform::String^ GetResourcesPath() const;
            std::unique_ptr<Microsoft::UI::Xaml::Tests::Common::XamlMetadataProviderOverrider> m_provider;
        };
       
    } } 
} } } }
