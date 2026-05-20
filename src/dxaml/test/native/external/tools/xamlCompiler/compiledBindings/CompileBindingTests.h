// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

#include <memory>
#include <map>
#include <tuple>
#include <wrl\module.h>
#include <wil\resource.h>

#include "XamlOM.WinUI.h"
#include <TestCleanupWrapper.h>
#include <XamlMetadataProviderOverrider.h>

using namespace Microsoft::UI::Xaml::Controls;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Tools {  
    namespace XamlCompiler {

        class CompileBindingTestsCpp : public WEX::TestClass < CompileBindingTestsCpp >
        {
        public:
            BEGIN_TEST_CLASS(CompileBindingTestsCpp)
                TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
                TEST_CLASS_PROPERTY(L"RunAs", L"UAP")

                TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"e22a917c-ad18-4a09-bff9-d3ca3e5ee0b8")
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
             END_TEST_CLASS()

            BEGIN_TEST_METHOD(OneTimeBindingTest)
                TEST_METHOD_PROPERTY(L"Description", L"Validates that one time binding works as expected.")
                TEST_METHOD_PROPERTY(L"Hosting:Mode", L"UAP")
            END_TEST_METHOD()

            TEST_CLASS_SETUP(ClassSetup)
            TEST_METHOD_CLEANUP(TestCleanup)

        private:
            std::unique_ptr<Microsoft::UI::Xaml::Tests::Common::XamlMetadataProviderOverrider> m_provider;

        };
    }
}}}}}
