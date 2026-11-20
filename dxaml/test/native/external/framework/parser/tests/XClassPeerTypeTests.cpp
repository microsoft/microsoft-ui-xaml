// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include <XamlTailored.h>
#include <TestCleanupWrapper.h>
#include <DisableErrorReportingScopeGuard.h>
#include <RuntimeEnabledFeatureOverride.h>
#include <string>
#include "XClassPeerTypeTests.h"

using namespace Platform;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Framework { namespace Parser {

    namespace
    {
        const HRESULT c_xamlParseFailed = static_cast<HRESULT>(0x802B000A);

        // x:Class is validated against the root object by the text ObjectWriter. The default test
        // configuration converts text XAML to an XBFv2 stream (EnforceXbfV2Stream), which does not
        // perform this check, so these tests exercise the direct text-parsing route.
        void LoadIntoUserControl(UserControl^ root, const wchar_t* resourceName)
        {
            std::wstring uri(L"ms-appx:///resources/native/framework/parser/");
            uri += resourceName;
            Application::LoadComponent(
                root,
                ref new ::Windows::Foundation::Uri(ref new String(uri.c_str())),
                Primitives::ComponentResourceLocation::Application);
        }

        void VerifyContentLoaded(UserControl^ root)
        {
            auto grid = dynamic_cast<Grid^>(root->Content);
            VERIFY_IS_NOT_NULL(grid, L"The component's content must be applied to the root.");
            VERIFY_IS_TRUE(grid == dynamic_cast<Grid^>(root->FindName(L"XClassRootGrid")), L"Named content must be registered in the root's namescope.");
        }
    }

    bool XClassPeerTypeTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool XClassPeerTypeTests::TestSetup()
    {
        TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool XClassPeerTypeTests::TestCleanup()
    {
        TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    void XClassPeerTypeTests::LoadComponentWithXClassMatchingRootTypeSucceeds()
    {
        TestCleanupWrapper cleanup;
        RuntimeEnabledFeatureOverride featureEnforceXbfV2Stream(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnforceXbfV2Stream, false);

        RunOnUIThread([&]()
        {
            auto root = ref new UserControl();
            LoadIntoUserControl(root, L"XClassExactType.xaml"); // x:Class="Microsoft.UI.Xaml.Controls.UserControl"
            VerifyContentLoaded(root);
        });
    }

    void XClassPeerTypeTests::LoadComponentWithXClassNamingBaseTypeSucceeds()
    {
        TestCleanupWrapper cleanup;
        RuntimeEnabledFeatureOverride featureEnforceXbfV2Stream(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnforceXbfV2Stream, false);

        RunOnUIThread([&]()
        {
            auto root = ref new UserControl();
            LoadIntoUserControl(root, L"XClassBaseType.xaml"); // x:Class="Microsoft.UI.Xaml.Controls.Control"
            VerifyContentLoaded(root);
        });
    }

    void XClassPeerTypeTests::LoadComponentWithUnrelatedXClassFails()
    {
        TestCleanupWrapper cleanup;
        DisableErrorReportingScopeGuard disableErrors;
        RuntimeEnabledFeatureOverride featureEnforceXbfV2Stream(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnforceXbfV2Stream, false);

        RunOnUIThread([&]()
        {
            auto root = ref new UserControl();
            bool threw = false;
            HRESULT hr = S_OK;
            std::wstring message;
            try
            {
                LoadIntoUserControl(root, L"XClassUnrelatedType.xaml"); // x:Class="Microsoft.UI.Xaml.Controls.Button"
            }
            catch (Exception^ e)
            {
                threw = true;
                hr = static_cast<HRESULT>(e->HResult);
                if (e->Message != nullptr)
                {
                    message = e->Message->Data();
                }
            }

            VERIFY_IS_TRUE(threw, L"A UserControl does not derive from Button, so the x:Class check must reject the component.");
            VERIFY_ARE_EQUAL(c_xamlParseFailed, hr);
            LOG_OUTPUT(L"Error message: %s", message.c_str());
            VERIFY_IS_TRUE(message.find(L"xClassNotDerivedFromElement") != std::wstring::npos);
            VERIFY_IS_NULL(root->Content, L"Rejected component content must not be applied.");
        });
    }

} } } } } }
