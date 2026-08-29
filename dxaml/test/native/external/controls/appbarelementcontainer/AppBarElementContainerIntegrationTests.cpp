// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "AppBarElementContainerIntegrationTests.h"

#include <generic\DependencyObjectTests.h>
#include <generic\FrameworkElementTests.h>

#include <XamlTailored.h>
#include <TreeHelper.h>

using namespace ::Windows::Foundation;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace AppBarElementContainer {

    bool AppBarElementContainerIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }
     
    bool AppBarElementContainerIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool AppBarElementContainerIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //
    void AppBarElementContainerIntegrationTests::CanInstantiate()
    {
        Generic::DependencyObjectTests<xaml_controls::AppBarElementContainer>::CanInstantiate();
    }

    void AppBarElementContainerIntegrationTests::CanEnterAndLeaveLiveTree()
    {
        Generic::FrameworkElementTests<xaml_controls::AppBarElementContainer>::CanEnterAndLeaveLiveTree();
    }

    void AppBarElementContainerIntegrationTests::CanContainStringContent()
    {
        TestCleanupWrapper cleanup;
        
        xaml_controls::AppBarElementContainer^ appBarElementContainer = nullptr;
        Platform::String^ testString = ref new Platform::String(L"Test string");

        RunOnUIThread([&]()
        {
            appBarElementContainer = ref new xaml_controls::AppBarElementContainer();
            appBarElementContainer->Content = testString;

            TestServices::WindowHelper->WindowContent = appBarElementContainer;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto textBlock = TreeHelper::GetVisualChildByType<xaml_controls::TextBlock>(appBarElementContainer);
            
            VERIFY_IS_NOT_NULL(textBlock);
            VERIFY_IS_TRUE(Platform::String::CompareOrdinal(testString, textBlock->Text) == 0);
        });
    }

    void AppBarElementContainerIntegrationTests::CanContainUIElementContent()
    {
        TestCleanupWrapper cleanup;
        
        xaml_controls::AppBarElementContainer^ appBarElementContainer = nullptr;
        xaml_controls::Button^ button = nullptr;
        Platform::String^ testString = ref new Platform::String(L"Test string");

        RunOnUIThread([&]()
        {
            button = ref new xaml_controls::Button();
            button->Content = testString;
            
            appBarElementContainer = ref new xaml_controls::AppBarElementContainer();
            appBarElementContainer->Content = button;

            TestServices::WindowHelper->WindowContent = appBarElementContainer;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto button = TreeHelper::GetVisualChildByType<xaml_controls::Button>(appBarElementContainer);
            
            VERIFY_IS_NOT_NULL(button);
            VERIFY_IS_TRUE(Platform::String::CompareOrdinal(testString, dynamic_cast<Platform::String^>(button->Content)) == 0);
        });
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Controls::AppBarElementContainer
