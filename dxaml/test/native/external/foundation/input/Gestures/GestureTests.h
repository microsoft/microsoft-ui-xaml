// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Input { namespace Gestures {
        class GestureTests : public WEX::TestClass<GestureTests>
        {
        public:
            BEGIN_TEST_CLASS(GestureTests)
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")

                TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"32301317-5c46-4350-8af6-a06552076e89;3192b2bd-30c5-4c19-a6c1-9856b940df63")
                TEST_CLASS_PROPERTY(L"VelocityTestPass:OneCoreStrict", L"Desktop")
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_METHOD_SETUP(TestSetup)
            TEST_METHOD_CLEANUP(TestCleanup)

            BEGIN_TEST_METHOD(LeaveTreeInTappedHandler)
                TEST_METHOD_PROPERTY(L"Description", L"Validates leaving the visual tree while handling an OnTapped event and having a manipulatable parent.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(LeaveAndReenterTreeInTappedHandler)
                TEST_METHOD_PROPERTY(L"Description", L"Validates leaving and re-entering the visual tree while handling an OnTapped event and having a manipulatable parent.")
                TEST_METHOD_PROPERTY(L"TestPass:ExcludeOn", L"WindowsCore")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(LeaveAndReenterTreeInManipulationDeltaHandler)
                TEST_METHOD_PROPERTY(L"Ignore", L"True")  // Re-enable when is resolved.
                TEST_METHOD_PROPERTY(L"Description", L"Validates leaving and re-entering the visual tree while handling a ManipulationDelta event.")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_METHOD()

        private:
            Platform::String^ GetPathToFiles() const;
        };
    } } }
} } } }
