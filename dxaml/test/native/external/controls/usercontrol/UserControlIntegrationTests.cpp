// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "UserControlIntegrationTests.h"

#include <XamlTailored.h>
#include <TestEvent.h>
#include <TestCleanupWrapper.h>

#include <generic\DependencyObjectTests.h>
#include <generic\FrameworkElementTests.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace UserControl {

    bool UserControlIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool UserControlIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool UserControlIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //
    void UserControlIntegrationTests::CanInstantiate()
    {
        Generic::DependencyObjectTests<xaml_controls::UserControl>::CanInstantiate();
    }

    void UserControlIntegrationTests::CanEnterAndLeaveLiveTree()
    {
        Generic::FrameworkElementTests<xaml_controls::UserControl>::CanEnterAndLeaveLiveTree();
    }

    void UserControlIntegrationTests::CanSetAndGetContentProperty()
    {
        TestCleanupWrapper cleanup;
        RunOnUIThread([&]()
        {
            auto userControl = ref new xaml_controls::UserControl();
            auto rectangle = ref new xaml_shapes::Rectangle();

            userControl->Content = rectangle;
            VERIFY_IS_TRUE(userControl->Content->Equals(rectangle));
        });
    }

    void UserControlIntegrationTests::LoadingEventFiresOnUserControl()
    {
        TestCleanupWrapper cleanup;

        auto spEventFired = std::make_shared<Event>();
        auto eventRegistration = CreateSafeEventRegistration(xaml::FrameworkElement, Loading);

        RunOnUIThread([&]()
        {
            auto page = TestServices::WindowHelper->SetupSimulatedAppPage();
            auto userControl = ref new xaml_controls::UserControl();

            eventRegistration.Attach(userControl,
                ref new wf::TypedEventHandler<xaml::FrameworkElement^, Platform::Object^>([spEventFired](Platform::Object^, Platform::Object^) {
                    spEventFired->Set();
                }));

            page->Content = userControl;
            userControl->Content = ref new xaml_controls::Button();
        });

        spEventFired->WaitForDefault();
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Controls::UserControl
