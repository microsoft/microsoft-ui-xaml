// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Shell {

        class BlockingAppFocusTests : public WEX::TestClass<BlockingAppFocusTests>
        {
        public:
            BEGIN_TEST_CLASS(BlockingAppFocusTests)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")

                TEST_METHOD_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"32301317-5c46-4350-8af6-a06552076e89")
                TEST_CLASS_PROPERTY(L"Hosting:Mode", L"UAP")   // Focus engagement bugs in lifted islands
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_METHOD_SETUP(TestSetup)
            TEST_METHOD_CLEANUP(TestCleanup)

            BEGIN_TEST_METHOD(CheckTabWraparound)
                TEST_METHOD_PROPERTY(L"Description", L"Checks that wrapping around from the last element to the first element works (normal Jupiter scenario).")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(CheckShiftTabWraparound)
                TEST_METHOD_PROPERTY(L"Description", L"Checks that wrapping around from the first element to the last element works (normal Jupiter scenario).")
            END_TEST_METHOD()

        private:
            Platform::String^ GetPathToFiles() const;
        };

    } }
} } } }
