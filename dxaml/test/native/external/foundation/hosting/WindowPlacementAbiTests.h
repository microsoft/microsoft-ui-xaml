// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Hosting {

        class WindowPlacementAbiTests : public WEX::TestClass<WindowPlacementAbiTests>
        {
        public:
            BEGIN_TEST_CLASS(WindowPlacementAbiTests)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
                TEST_CLASS_PROPERTY(L"Hosting:Mode", L"WPF")
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_METHOD_SETUP(TestSetup)
            TEST_METHOD_CLEANUP(TestCleanup)

            TEST_METHOD(FactoryCreatesPlacementWithDefaults)
            TEST_METHOD(InvalidDpiReturnsInvalidArgumentAndNoObject)
            TEST_METHOD(AllPropertiesRoundTripThroughAbi)
            TEST_METHOD(CapturedPlacementIsAnIndependentCopy)
            TEST_METHOD(DetachedLoadOfMissingIdReturnsSuccessAndNull)
            TEST_METHOD(DetachedLoadOfEmptyOrNullIdReturnsInvalidArgumentAndNoObject)
        };

    } }
} } } }
