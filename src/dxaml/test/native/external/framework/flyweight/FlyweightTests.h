// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

#include <RuntimeEnabledFeatureOverride.h>

struct IVisualStateGroupCollectionTestHooks;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Framework {
        class FlyweightTests : public WEX::TestClass<FlyweightTests>
        {
        public:
            BEGIN_TEST_CLASS(FlyweightTests)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")

                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_CLASS_CLEANUP(ClassCleanup)
            TEST_METHOD_SETUP(TestSetup)
            TEST_METHOD_CLEANUP(TestCleanup)

            BEGIN_TEST_METHOD(ValidateGetterSetter)
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateInstantiateInResources)
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidatePropertyChangedCallback)
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateBindingToObjectPropertyReadback)
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateBindingToResourceReadback)
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateResourceInObjectThrows)
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(ValidateAnimateToNull)
            END_TEST_METHOD()

        private:
            Microsoft::UI::Xaml::Tests::Common::RuntimeEnabledFeatureOverride featureEnforceXbfV2Stream;
        };
    }
} } } }
