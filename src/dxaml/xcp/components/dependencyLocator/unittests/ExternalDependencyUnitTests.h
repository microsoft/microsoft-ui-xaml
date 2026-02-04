// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>

namespace Windows { namespace UI { namespace Xaml { namespace Tests {
    namespace Framework {

        class ExternalDependencyUnitTests : public WEX::TestClass<ExternalDependencyUnitTests>
        {
        public:
            TEST_CLASS_SETUP(ClassSetup)

            BEGIN_TEST_CLASS(ExternalDependencyUnitTests)
                TEST_METHOD_PROPERTY(L"Classification", L"Integration")
                TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_CLASS()

            BEGIN_TEST_METHOD(CanLoadStorageFromLibrary)
                TEST_METHOD_PROPERTY(L"Description", L"ExternalDependency should call LoadLibrary and resolve.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(LoadsStorageOnce)
                TEST_METHOD_PROPERTY(L"Description", L"ExternalDependency should call LoadLibrary only once for the same dependency.")
            END_TEST_METHOD()
                
            BEGIN_TEST_METHOD(UnloadingReleasesReferences)
                TEST_METHOD_PROPERTY(L"Description", L"ExternalDependency should release all references when being unloaded.")
            END_TEST_METHOD()
        private:
            static std::wstring m_mockDependencyPath;
        };
    }
} } } }
