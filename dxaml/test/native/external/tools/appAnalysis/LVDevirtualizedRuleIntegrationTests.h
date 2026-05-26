// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Tools { namespace AppAnalysis {

        class LVDevirtualizedRuleIntegrationTests : public WEX::TestClass<LVDevirtualizedRuleIntegrationTests>
        {
        public:
            BEGIN_TEST_CLASS(LVDevirtualizedRuleIntegrationTests)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.Diagnsostics.AppAnalysis.dll")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
                TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"e22a917c-ad18-4a09-bff9-d3ca3e5ee0b8")
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_METHOD_PROPERTY(L"Ignore", L"TRUE") // All AppAnalysis tests are unreliable or failing.
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_CLASS_CLEANUP(ClassCleanup)

            TEST_METHOD_CLEANUP(TestCleanup)

            BEGIN_TEST_METHOD(CatchesDevirtualizedDueToLayout)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that we correctly fire a notification when LV is being layed out with infinite size.")
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
