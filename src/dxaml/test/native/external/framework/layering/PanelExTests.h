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
    class PanelExTests : public WEX::TestClass<PanelExTests>
    {
    public:
        BEGIN_TEST_CLASS(PanelExTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(CanAccessChildrenProperty)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that developers can access the children property.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DoesPerformLayout)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the FrameworkElementEx's children performs layout.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DoesRespectMinSize)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the FrameworkElementEx respects its min size.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DoesRespectMaxSize)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the FrameworkElementEx respects its max size.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DoesRespectMargin)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that the FrameworkElementEx respects its margin.")
        END_TEST_METHOD()
    };
#endif // WI_IS_FEATURE_PRESENT(Feature_Xaml2018)

} } } } } }
