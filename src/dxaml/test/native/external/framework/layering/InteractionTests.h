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
    class InteractionTests : public WEX::TestClass<InteractionTests>
    {
    public:
        BEGIN_TEST_CLASS(InteractionTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(CanAddInteractionsToElements)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that developers can add interactions to UIElement-based elements.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DoesReceiveKeyEvents)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that attached input objects receive key events.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DoesReceivePointerEvents)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that attached input objects receive pointer events.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DoesReceiveTouchTapEvents)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that attached input objects receive touch tap events.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DoesReceiveTouchHoldingEvents)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that attached input objects receive touch holding events.")
            TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DoesReceiveDragEvents)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that attached input objects receive drag events.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(DoesReceiveDropEvent)
            TEST_METHOD_PROPERTY(L"Description", L"Validates that attached input objects receive drop event.")
            TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
        END_TEST_METHOD()
    };
#endif // WI_IS_FEATURE_PRESENT(Feature_Xaml2018)

} } } } } }
