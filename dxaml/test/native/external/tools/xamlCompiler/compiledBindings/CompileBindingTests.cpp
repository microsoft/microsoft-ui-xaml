// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include <XamlTailored.h>
#include <TestCleanupWrapper.h>
#include <TreeHelper.h>
#include "FileLoader.h"
#include "CompileBindingTests.h"
#include "CustomTypes.XamlTypeInfo.g.h"
#include "XamlCompilerPage.xaml.h"

using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Shapes;
using namespace Microsoft::UI::Xaml::Data;
using namespace test_infra;

DEFINE_GUID(CLSID_TEST_TAP, 0x6228CA0A, 0xCDD7, 0x433E, 0x98, 0x54, 0x9A, 0xD5, 0x07, 0x75, 0x4F, 0x83);

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Tools {  
    namespace XamlCompiler {

        bool CompileBindingTestsCpp::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();

            // Ensure our metadata and custom DPs are registered
            m_provider.reset(new XamlMetadataProviderOverrider(
                ref new MetadataProvider));
            return true;
        }

        bool CompileBindingTestsCpp::TestCleanup()
        {
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        void CompileBindingTestsCpp::OneTimeBindingTest()
        {
            TestCleanupWrapper cleanup;

            ::CustomTypes::XamlCompilerPage^ rootPage;
            RunOnUIThread([&]()
            {
                rootPage = ref new ::CustomTypes::XamlCompilerPage();
                Application::LoadComponent(
                    rootPage,
                    // This page is binplaced during the build process and is actually the XBF file.
                    ref new ::Windows::Foundation::Uri("ms-appx:///resources/native/tools/XamlCompilerPage.xaml"),
                    Primitives::ComponentResourceLocation::Application);

                VERIFY_IS_NOT_NULL(rootPage);

                TestServices::WindowHelper->WindowContent = rootPage;
            });

            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                TextBlock^ textBlock1 = safe_cast<TextBlock^> (rootPage->FindName(L"textBlock1"));
                VERIFY_IS_NOT_NULL(textBlock1);
                Platform::String^ expectedText = L"Chester";
                Platform::String^ actualText = textBlock1->Text;
                VERIFY_IS_TRUE(Platform::String::CompareOrdinal(expectedText, actualText) == 0);

            });

        }
    }
}}}}}
