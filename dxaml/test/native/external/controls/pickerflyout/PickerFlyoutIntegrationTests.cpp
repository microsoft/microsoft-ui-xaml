// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "PickerFlyoutIntegrationTests.h"

#include <generic\DependencyObjectTests.h>

#include <XamlTailored.h>
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>
#include <Collection.h>

#include <TreeHelper.h>
#include <ControlHelper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;
using namespace Concurrency;


namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace PickerFlyout {

    bool PickerFlyoutIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }
     
    bool PickerFlyoutIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool PickerFlyoutIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    void PickerFlyoutIntegrationTests::CanInstantiate()
    {
        Generic::DependencyObjectTests<xaml_controls::PickerFlyout>::CanInstantiate();
    }

    void PickerFlyoutIntegrationTests::CanClickPickerFlyout()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::PickerFlyout^ pickerFlyout = nullptr;
        auto button = SetupPickerFlyoutTest();

        ControlHelper::DoClickUsingTap(button);

        RunOnUIThread([&]()
        {
            pickerFlyout = dynamic_cast<xaml_controls::PickerFlyout^>(button->Flyout);
            VERIFY_IS_NOT_NULL(pickerFlyout);

            bool confirmationButtonsVisible = pickerFlyout->ConfirmationButtonsVisible;
            VERIFY_IS_FALSE(confirmationButtonsVisible);

            auto content = pickerFlyout->Content;
            VERIFY_IS_NOT_NULL(content);
        });

        ClosePickerFlyout(pickerFlyout, false /*doClickConfirmationButton*/);
    }

    void PickerFlyoutIntegrationTests::CanOpenAndClose()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::PickerFlyout^ pickerFlyout = nullptr;

        auto button = SetupPickerFlyoutTest();

        RunOnUIThread([&]()
        {
            pickerFlyout = dynamic_cast<xaml_controls::PickerFlyout^>(button->Flyout);
            VERIFY_IS_NOT_NULL(pickerFlyout);

            xaml_primitives::PickerFlyoutBase::SetTitle(pickerFlyout, L"PickerFlyout Header");
            auto header = xaml_primitives::PickerFlyoutBase::GetTitle(pickerFlyout);

            LOG_OUTPUT(L"CanOpenAndClose: header = %s ", header->Data());
            VERIFY_ARE_EQUAL(header, ref new Platform::String(L"PickerFlyout Header"));
        });

        OpenPickerFlyout(button, pickerFlyout);

        ClosePickerFlyout(pickerFlyout, false /*doClickConfirmationButton*/);
    }

    wf::IAsyncOperation<bool>^ PickerFlyoutIntegrationTests::OpenPickerFlyout(
        xaml_controls::Button^ button,
        xaml_controls::PickerFlyout^ pickerFlyout)
    {
        wf::IAsyncOperation<bool>^ showAsyncOperation = nullptr;
        auto openedRegistration = CreateSafeEventRegistration(xaml_controls::PickerFlyout, Opened);
        auto openedEvent = std::make_shared<Event>();

        RunOnUIThread([&]()
        {
            openedRegistration.Attach(
                pickerFlyout,
                ref new wf::EventHandler<Platform::Object^>([openedEvent](Platform::Object^, Platform::Object^)
            {
                LOG_OUTPUT(L"OpenPickerFlyout: Opened event fired!");
                openedEvent->Set();
            }));

            LOG_OUTPUT(L"OpenPickerFlyout: ShowAtAsync().");
            showAsyncOperation = pickerFlyout->ShowAtAsync(button);
        });

        openedEvent->WaitForDefault();

        return showAsyncOperation;
    }

    void PickerFlyoutIntegrationTests::ClosePickerFlyout(xaml_controls::PickerFlyout^ pickerFlyout, bool doClickConfirmationButton)
    {
        auto closedRegistration = CreateSafeEventRegistration(xaml_controls::PickerFlyout, Closed);
        auto closedEvent = std::make_shared<Event>();

        RunOnUIThread([&]()
        {
            closedRegistration.Attach(
                pickerFlyout,
                ref new wf::EventHandler<Platform::Object^>([closedEvent](Platform::Object^, Platform::Object^)
            {
                LOG_OUTPUT(L"ClosePickerFlyout: Closed event fired!");
                closedEvent->Set();
            }));

            if (!doClickConfirmationButton)
            {
                LOG_OUTPUT(L"ClosePickerFlyout: Hide().");
                pickerFlyout->Hide();
            }
        });

        if (doClickConfirmationButton)
        {
            ControlHelper::ClickFlyoutCloseButton(pickerFlyout, true /*isAccept*/);
        }

        closedEvent->WaitForDefault();
    }

    xaml_controls::Button^ PickerFlyoutIntegrationTests::SetupPickerFlyoutTest()
    {
        xaml_controls::Button^ button = nullptr;

        auto loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::Grid, Loaded);

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                L"  <Button x:Name='button' Content='Test PickerFlyout' > "
                L"    <Button.Flyout> "
                L"      <PickerFlyout >"
                L"        <TextBlock Text='TextBlock on PickerFlyout' x:Name='textblock' /> "
                L"      </PickerFlyout >"
                L"    </Button.Flyout> "
                L"  </Button> "
                L"</Grid>"));

            button = dynamic_cast<xaml_controls::Button^>(rootPanel->FindName(L"button"));
            VERIFY_IS_NOT_NULL(button);

            loadedRegistration.Attach(
                rootPanel,
                ref new RoutedEventHandler([loadedEvent](Platform::Object^, RoutedEventArgs^)
            {
                LOG_OUTPUT(L"SetupPickerFlyoutTest: Loaded event fired!");
                loadedEvent->Set();
            }));

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        loadedEvent->WaitForDefault();
        return button;
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Controls::PickerFlyout
