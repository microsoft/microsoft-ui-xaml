// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

#include <RuntimeEnabledFeatureOverride.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace ListViewBaseConnectedAnimations {

    class ConnectedAnimationTests : public WEX::TestClass<ConnectedAnimationTests>
    {
    public:
        BEGIN_TEST_CLASS(ConnectedAnimationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_METHOD_PROPERTY(L"Classification", L"Integration")

        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_CLASS_CLEANUP(ClassCleanup)
        TEST_METHOD_SETUP(TestSetup)

        TEST_METHOD_CLEANUP(TestCleanup)

        //
        // Platform:Any
        //
        BEGIN_TEST_METHOD(AnimationTest)
            TEST_METHOD_PROPERTY(L"Description", L"Validates the ListViewBase helper methods for connected animations")
        END_TEST_METHOD()

    };

} } } } } }
