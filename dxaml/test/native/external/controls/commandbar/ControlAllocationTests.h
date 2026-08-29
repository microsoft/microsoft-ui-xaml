// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <RuntimeEnabledFeatureOverride.h>

using namespace Microsoft::UI::Xaml::Tests::Common;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls {

    class ControlAllocationTests : public WEX::TestClass<ControlAllocationTests>
    {
    public:
        BEGIN_TEST_CLASS(ControlAllocationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(MeasureCommandBarAllocations)
            TEST_METHOD_PROPERTY(L"Description", L"Measures allocation counts when creating a CommandBar.")
        END_TEST_METHOD()

    private:
        RuntimeEnabledFeatureOverride featureDisableTransitionsForTest;
    };

} } } } }
