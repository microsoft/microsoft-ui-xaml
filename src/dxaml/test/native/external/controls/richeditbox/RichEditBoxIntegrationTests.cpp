// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "RichEditBoxIntegrationTests.h"

#include <XamlTailored.h>
#include <TestEvent.h>
#include <TreeHelper.h>
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>
#include <TextControlHelper.h>

using namespace Platform;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace RichEditBox {

    bool RichEditBoxIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool RichEditBoxIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool RichEditBoxIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    void RichEditBoxIntegrationTests::VerifyContextMenuRaisesCutCopyPasteEvents()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::RichEditBox^ richEditBox = nullptr;
        xaml_controls::CommandBar^ commandBar = nullptr;

        auto loadedEvent = std::make_shared<Event>();
        auto cuttingToClipboardEvent = std::make_shared<Event>();
        auto copyingToClipboardEvent = std::make_shared<Event>();
        auto pasteEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::RichEditBox, Loaded);
        auto cuttingToClipboardRegistration = CreateSafeEventRegistration(xaml_controls::RichEditBox, CuttingToClipboard);
        auto copyingToClipboardRegistration = CreateSafeEventRegistration(xaml_controls::RichEditBox, CopyingToClipboard);
        auto pasteRegistration = CreateSafeEventRegistration(xaml_controls::RichEditBox, Paste);

        RunOnUIThread([&]()
        {
            auto rootStackPanel = ref new xaml_controls::StackPanel();

            richEditBox = ref new xaml_controls::RichEditBox();
            richEditBox->Document->SetText(Microsoft::UI::Text::TextSetOptions::None, L"aaaaaaaaaaa");

            loadedRegistration.Attach(richEditBox, [loadedEvent]() { loadedEvent->Set(); });
            cuttingToClipboardRegistration.Attach(richEditBox, [cuttingToClipboardEvent]() { cuttingToClipboardEvent->Set(); });
            copyingToClipboardRegistration.Attach(richEditBox, [copyingToClipboardEvent]() { copyingToClipboardEvent->Set(); });
            pasteRegistration.Attach(richEditBox, [pasteEvent]() { pasteEvent->Set(); });

            rootStackPanel->Children->Append(richEditBox);
            TestServices::WindowHelper->WindowContent = rootStackPanel;
        });

        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        auto flyoutOpenedEvent = std::make_shared<Event>();
        auto flyoutClosedEvent = std::make_shared<Event>();
        auto flyoutOpenedRegistration = CreateSafeEventRegistration(xaml_primitives::FlyoutBase, Opened);
        auto flyoutClosedRegistration = CreateSafeEventRegistration(xaml_primitives::FlyoutBase, Closed);

        RunOnUIThread([&]()
        {
            flyoutOpenedRegistration.Attach(richEditBox->ContextFlyout, [flyoutOpenedEvent]() { flyoutOpenedEvent->Set(); });
            flyoutClosedRegistration.Attach(richEditBox->ContextFlyout, [flyoutClosedEvent]() { flyoutClosedEvent->Set(); });

            LOG_OUTPUT(L"Select all text in the RichEditBox.");
            richEditBox->Document->Selection->Expand(Microsoft::UI::Text::TextRangeUnit::Story);
        });

        auto rightClickFunc = [richEditBox]()
        {
            TestServices::InputHelper->ClickMouseButton(MouseButton::Right, richEditBox);
        };

        LOG_OUTPUT(L"Open the context menu with a right-click.");
        rightClickFunc();

        flyoutOpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        xaml_controls::AppBarButton^ cutButton = nullptr;

        RunOnUIThread([&]()
        {
            auto popups = xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(richEditBox->XamlRoot);
            VERIFY_IS_GREATER_THAN(popups->Size, 0u);

            auto flyoutPresenter = safe_cast<xaml::FrameworkElement^>(popups->GetAt(popups->Size - 1)->Child);
            commandBar = TreeHelper::GetVisualChildByType<xaml_controls::CommandBar>(flyoutPresenter);
            cutButton = safe_cast<xaml_controls::AppBarButton^>(commandBar->SecondaryCommands->GetAt(0));
        });

        LOG_OUTPUT(L"Click on the cut button. The CuttingToClipboard event should be raised.");
        TestServices::InputHelper->LeftMouseClick(cutButton);

        cuttingToClipboardEvent->WaitForDefault();
        flyoutClosedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Open the context menu with a right-click again.");
        rightClickFunc();

        flyoutOpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        xaml_controls::AppBarButton^ pasteButton = nullptr;

        RunOnUIThread([&]()
        {
            pasteButton = safe_cast<xaml_controls::AppBarButton^>(commandBar->SecondaryCommands->GetAt(0));
        });

        LOG_OUTPUT(L"Click on the paste button. The Paste event should be raised.");
        TestServices::InputHelper->LeftMouseClick(pasteButton);

        pasteEvent->WaitForDefault();
        flyoutClosedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Select all text and open the context menu with a right-click again.");

        RunOnUIThread([&]()
        {
            richEditBox->Document->Selection->Expand(Microsoft::UI::Text::TextRangeUnit::Story);
        });

        rightClickFunc();

        flyoutOpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        xaml_controls::AppBarButton^ copyButton = nullptr;

        RunOnUIThread([&]()
        {
            copyButton = safe_cast<xaml_controls::AppBarButton^>(commandBar->SecondaryCommands->GetAt(1));
        });

        LOG_OUTPUT(L"Click on the copy button. The CopyingToClipboard event should be raised.");
        TestServices::InputHelper->LeftMouseClick(copyButton);

        copyingToClipboardEvent->WaitForDefault();
        flyoutClosedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
    }

    void RichEditBoxIntegrationTests::VerifySelectingTextWithTouchShowsSelectionFlyout()
    {
        TextControlHelper::VerifySelectingTextWithTouchShowsSelectionFlyout<xaml_controls::RichEditBox>(
            [](xaml_controls::RichEditBox^ richEditBox) { richEditBox->Document->SetText(Microsoft::UI::Text::TextSetOptions::None, "aaaaaaaaaa"); });
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Controls::RichEditBox
