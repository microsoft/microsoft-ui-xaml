// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "AccessKeyIntegrationTests.h"

#include <XamlTailored.h>
#include <TreeHelper.h>
#include <TestCleanupWrapper.h>

#include "AccessKeyTestHelper.h"


using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace AccessKeys {

    //
    // Test Cases
    //

    void AccessKeyIntegrationTests::InvokeAccessKeysOnMultipleButtonsOfContentDialog()
    {
        TestCleanupWrapper cleanup;

        std::vector<xaml_primitives::ButtonBase^> buttons;
        xaml_controls::ContentDialog^ contentDialog;
        xaml_controls::Button ^button1;
        xaml_controls::Button ^button2;
        xaml_controls::Button ^button3;

        RunOnUIThread([&]()
        {
            contentDialog = ref new xaml_controls::ContentDialog();
            contentDialog->Title = L"ContentDialog Title";
            contentDialog->PrimaryButtonText = L"OK";
            contentDialog->SecondaryButtonText = L"Cancel";

            auto stackPanel = ref new xaml_controls::StackPanel();
            button1 = ref new xaml_controls::Button();
            button1->Content = L"BT1";
            stackPanel->Children->Append(button1);

            button2 = ref new xaml_controls::Button();
            button2->Content = L"BT2";
            stackPanel->Children->Append(button2);

            button3 = ref new xaml_controls::Button();
            button3->Content = L"BT3";
            stackPanel->Children->Append(button3);

            contentDialog->Content = stackPanel;

            auto windowContent = ref new xaml_controls::StackPanel();
            TestServices::WindowHelper->WindowContent = windowContent;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto xamlRoot = TestServices::WindowHelper->WindowContent->XamlRoot;

            if (xamlRoot)
            {
                contentDialog->XamlRoot = xamlRoot;
            }
        });
        
        TestServices::WindowHelper->WaitForIdle();
        AccessKeyTestHelper::TryMovingFocusToXamlForInit();

        OpenContentDialog(contentDialog);

        xaml_primitives::ButtonBase^ primaryButton = nullptr;

        RunOnUIThread([&]()
        {
            primaryButton = safe_cast<xaml_primitives::ButtonBase^>(TreeHelper::GetVisualChildByName(contentDialog, L"PrimaryButton"));
        });
        TestServices::WindowHelper->WaitForIdle();

        buttons.push_back(std::move(button1));
        buttons.push_back(std::move(button2));
        buttons.push_back(std::move(button3));
        buttons.push_back(std::move(primaryButton)); // also testing the primary button of the content dialog

        AccessKeyTestHelper::InvokeAccessKeysOnMultipleButtons(buttons);

        // content dialog should have already been close because primary button invoked

        TestServices::WindowHelper->WaitForIdle();
    }
    
    wf::IAsyncOperation<xaml_controls::ContentDialogResult>^
        AccessKeyIntegrationTests::OpenContentDialog(xaml_controls::ContentDialog^ contentDialog)
    {
        auto openedRegistration = CreateSafeEventRegistration(xaml_controls::ContentDialog, Opened);
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::ContentDialog, Loaded);

        auto openedEvent = std::make_shared<Event>();
        auto loadedEvent = std::make_shared<Event>();

        wf::IAsyncOperation<xaml_controls::ContentDialogResult>^ showAsyncOperation = nullptr;

        openedRegistration.Attach(contentDialog,
            ref new wf::TypedEventHandler<xaml_controls::ContentDialog^, xaml_controls::ContentDialogOpenedEventArgs^>(
                [openedEvent](xaml_controls::ContentDialog^ sender, xaml_controls::ContentDialogOpenedEventArgs^ args)
        {
            openedEvent->Set();
        }));

        loadedRegistration.Attach(contentDialog, ref new xaml::RoutedEventHandler([loadedEvent](Platform::Object^ sender, xaml::RoutedEventArgs^ e)
        {
            loadedEvent->Set();
        }));

        RunOnUIThread([&]()
        {
            showAsyncOperation = contentDialog->ShowAsync();
        });

        // There are three things that we need to wait for when opening a ContentDialog:
        //
        // First, the opened event needs to be fired, indicating that the ContentDialog has begun opening.
        // Second, the loaded event needs to be fired, indicating that the ContentDialog has been added to the visual tree.
        // Third, we need to wait for the transition-in animation to complete for the ContentDialog to be interactable.
        //
        openedEvent->WaitForDefault();
        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        return showAsyncOperation;
    }

} } } } } 
