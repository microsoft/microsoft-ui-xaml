// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Shell {

        class StartMenuTests : public WEX::TestClass<StartMenuTests>
        {
        public:
            BEGIN_TEST_CLASS(StartMenuTests)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"32301317-5c46-4350-8af6-a06552076e89;3192b2bd-30c5-4c19-a6c1-9856b940df63")
                TEST_CLASS_PROPERTY(L"Hosting:Mode", L"UAP") // Test uses private APIs that will never be available in Win32
            END_TEST_CLASS()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_METHOD_SETUP(TestSetup)

            TEST_METHOD_CLEANUP(TestCleanup)

            BEGIN_TEST_METHOD(SetAtlasSizeHint)
                TEST_METHOD_PROPERTY(L"Description", L"Tests the internal API for shell to override the atlas size hint for DComp.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(AtlasRequest)
                TEST_METHOD_PROPERTY(L"Description", L"Tests IAtlasRequestProvider callback")
            END_TEST_METHOD()

        private:
            Platform::String^ GetResourcesPath() const;
            Platform::String^ GetEtwFilterString(unsigned int width, unsigned int height);
        };

    } }
} } } }
