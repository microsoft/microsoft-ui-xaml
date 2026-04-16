// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Tools { namespace AppAnalysis {

        class NonVirtualizingPanelRuleIntegrationTests : public WEX::TestClass<NonVirtualizingPanelRuleIntegrationTests>
        {
        public:
            BEGIN_TEST_CLASS(NonVirtualizingPanelRuleIntegrationTests)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.Diagnostics.AppAnalysis.dll")
                TEST_CLASS_PROPERTY(L"ArtifactUnderTest", L"sdk\\inc\\microsoft.diagnostics.appanalysis.idl")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
                TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"e22a917c-ad18-4a09-bff9-d3ca3e5ee0b8")
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // All AppAnalysis tests are unreliable or failing.
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_CLASS_CLEANUP(ClassCleanup)

            TEST_METHOD_CLEANUP(TestCleanup)

            BEGIN_TEST_METHOD(CatchesListViewDevirtualizedDueToPanel)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that we correctly fire a notification when the panel being used does not derive from ModernCollectionPanel.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(DoesntCatchVirtualizingPanel)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that we don't capture instances of ListBox that use VirtualizingStackPanel")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(DoesntFireFalsePositives)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that when the panel does derive from ModernCollectionBase or LV inside ScrollViewer but layout is "
                                     L"in opposite direction that we don't fire.")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            END_TEST_METHOD()

        private:
            Platform::String^ GetResourcesPath() const;
        };
       
    } } 
} } } }
