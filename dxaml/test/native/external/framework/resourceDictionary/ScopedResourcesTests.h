// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>
#include <RuntimeEnabledFeatureOverride.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Framework {
        class ScopedResourcesTests : public WEX::TestClass<ScopedResourcesTests>
        {
        public:
            BEGIN_TEST_CLASS(ScopedResourcesTests)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_CLASS_CLEANUP(ClassCleanup)
            TEST_METHOD_SETUP(TestSetup)
            TEST_METHOD_CLEANUP(TestCleanup)

            TEST_METHOD(IMapAPITests)
            TEST_METHOD(BasicFunctionality)
            TEST_METHOD(ValidateAllProperties)
            TEST_METHOD(OverrideLookupOnPage)
            TEST_METHOD(ThemeChange)
            BEGIN_TEST_METHOD(PickupOverrideFromAppXAML)
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            END_TEST_METHOD()
            TEST_METHOD(HighContrast)
            TEST_METHOD(NoopForResourceDictionary)
            TEST_METHOD(RedefinedResourcesOnPageDontTriggerOverride)
            TEST_METHOD(RedefinedResourcesInAppDontTriggerOverride)
            // _MUXC_REMOVAL
            /*
            TEST_METHOD(CanCloneSupportedTypes)
            TEST_METHOD(DestinationDictionaryTests)
            */

        private:
            Microsoft::UI::Xaml::Tests::Common::RuntimeEnabledFeatureOverride featureEnforceXbfV2Stream;
        };
    }
} } } }