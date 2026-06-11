// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>
#include <RuntimeEnabledFeatureOverride.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>

#include "FeatureFlags.h"

using namespace Microsoft::UI::Xaml::Tests::Common;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Framework { namespace Layering {

#if WI_IS_FEATURE_PRESENT(Feature_Xaml2018)
    class StrictModeTests : public WEX::TestClass<StrictModeTests>
    {
    public:
        BEGIN_TEST_CLASS(StrictModeTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(ValidateNonStrictType)
            TEST_METHOD_PROPERTY(L"Description", L"Validates strict-mode with non-strict types.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP") // WPF Dispatcher timing of the UI is vastly different than UWP
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateStrictType)
            TEST_METHOD_PROPERTY(L"Description", L"Validates strict-mode with strict types.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(ValidateVisualTreeHelperDisallowedInStrictMode)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that VisualTreeHelper.GetChildrenCount() & GetChild() methods are disallowed.")
        END_TEST_METHOD()
    };
#endif // WI_IS_FEATURE_PRESENT(Feature_Xaml2018)

} } } } } }
