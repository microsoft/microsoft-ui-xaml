// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "TextBoxIntegrationTests.h"

#include <XamlTailored.h>
#include <TestEvent.h>
#include <TreeHelper.h>
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>
#include <ControlHelper.h>
#include <FlyoutHelper.h>
#include <TextControlHelper.h>

using namespace Platform;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace TextBox {

    bool TextBoxIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool TextBoxIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool TextBoxIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    void TextBoxIntegrationTests::ClearButton()
    {
        ClearButtonHelper(true /*clearButtonExpected*/);
    }

    void TextBoxIntegrationTests::ClearButtonOnXBox()
    {
        ClearButtonHelper(false /*clearButtonExpected*/);
    }

    void TextBoxIntegrationTests::ClearButtonHelper(bool clearButtonExpected) const
    {
        TestCleanupWrapper cleanup([]()
        {
            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        xaml_controls::Button^ clearSipButton = nullptr;
        xaml_controls::TextBox^ textBox = nullptr;
        xaml_controls::StackPanel^ rootStackPanel = nullptr;
        xaml_controls::Button^ clearAllButton;
        Platform::String^ strToType = ref new Platform::String(L"This is a TextBox with a Clear All Button!");

        auto loadedEvent = std::make_shared<Event>();
        auto focusTextBoxEvent = std::make_shared<Event>();
        auto focusButtonEvent = std::make_shared<Event>();
        auto textChangedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::StackPanel, Loaded);
        auto focusTextBoxRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, GotFocus);
        auto focusButtonRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);
        auto textChangedTextBoxRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, TextChanged);

        LOG_OUTPUT(L"Create the text box and stack panel.");
        RunOnUIThread([&]()
        {
            rootStackPanel = ref new xaml_controls::StackPanel();

            loadedRegistration.Attach(rootStackPanel, ref new xaml::RoutedEventHandler([loadedEvent](Platform::Object^, xaml::RoutedEventArgs^)
            {
                LOG_OUTPUT(L"Loaded event fired on StackPanel!");
                loadedEvent->Set();
            }));

            textBox = ref new xaml_controls::TextBox();
            VERIFY_IS_NOT_NULL(textBox);
            textBox->Width = 300;
            textBox->Height = 100;
            textBox->Text = strToType;

            focusTextBoxRegistration.Attach(
                textBox,
                ref new xaml::RoutedEventHandler(
                [focusTextBoxEvent](Platform::Object^ sender, xaml::IRoutedEventArgs^)
            {
                LOG_OUTPUT(L"GotFocus event fired on TextBox!");
                focusTextBoxEvent->Set();
            }));

            textChangedTextBoxRegistration.Attach(
                textBox,
                ref new xaml_controls::TextChangedEventHandler(
                [textChangedEvent](Platform::Object^ sender, xaml_controls::ITextChangedEventArgs ^)
            {
                LOG_OUTPUT(L"TextChanged event fired on TextBox!");
                textChangedEvent->Set();
            }));

            clearSipButton = ref new xaml_controls::Button();
            VERIFY_IS_NOT_NULL(clearSipButton);
            clearSipButton->Content = L"Clear Sip";

            focusButtonRegistration.Attach(
                clearSipButton,
                ref new xaml::RoutedEventHandler(
                [focusButtonEvent](Platform::Object^ sender, xaml::IRoutedEventArgs^)
            {
                LOG_OUTPUT(L"GotFocus event fired on Button!");
                focusButtonEvent->Set();
            }));

            rootStackPanel->Children->Append(textBox);
            rootStackPanel->Children->Append(clearSipButton);

            TestServices::WindowHelper->WindowContent = rootStackPanel;
        });
        TestServices::WindowHelper->WaitForIdle();
        loadedEvent->WaitForDefault();

        LOG_OUTPUT(L"Set keyboard focus on the text box.");
        RunOnUIThread([&]()
        {
            textBox->Focus(xaml::FocusState::Keyboard);
        });
        focusTextBoxEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Check clear button visibility.");
        RunOnUIThread([&]()
        {
            clearAllButton = safe_cast<xaml_controls::Button^>(TreeHelper::GetVisualChildByName(textBox, L"DeleteButton"));
            VERIFY_IS_NOT_NULL(clearAllButton);
            auto visibility = clearAllButton->Visibility;
            if (clearButtonExpected)
            {
                VERIFY_IS_TRUE(visibility == xaml::Visibility::Visible);
            }
            else
            {
                VERIFY_IS_TRUE(visibility != xaml::Visibility::Visible);
            }
        });

        if (clearButtonExpected)
        {
            LOG_OUTPUT(L"Click the clear all button.");
            textChangedEvent->Reset();
            TestServices::InputHelper->Tap(clearAllButton);
            textChangedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Make sure the text box is empty.");
            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(textBox->Text->IsEmpty());
            });
        }

        RunOnUIThread([&]()
        {
            // Disable the text box to prevent it from receiving focus (helps with stability)
            textBox->IsEnabled = false;
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Close down the SIP.");
        TestServices::InputHelper->Tap(clearSipButton);
        TestServices::WindowHelper->WaitForIdle();
        focusButtonEvent->WaitForDefault();
    }

    // Changes the view of the TextBox's inner ScrollViewer using ScrollViewer.ChangeView,
    // then scrolls back with the mouse wheel, and finally programmatically changes the view
    // again with ScrollViewer.ChangeView. Verifies the ScrollViewer.ViewChanging and
    // ScrollViewer.ViewChanged events are raised and the VerticalOffset property are updated.
    void TextBoxIntegrationTests::ChangeViewInInnerScrollViewer()
    {
        TestCleanupWrapper cleanup;

        ::Windows::Foundation::Size size(120, 120);
        TestServices::WindowHelper->SetWindowSizeOverride(size);

        xaml_controls::ScrollViewer^ scrollViewer = nullptr;
        xaml_controls::TextBox^ textBox = nullptr;
        xaml_controls::StackPanel^ rootStackPanel = nullptr;

        int viewChangingCount = 0;
        int viewChangedCount = 0;
        double verticalOffset = 0.0;

        auto loadedEvent = std::make_shared<Event>();
        auto viewChangedEvent = std::make_shared<Event>();

        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::StackPanel, Loaded);
        auto viewChangingRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, ViewChanging);
        auto viewChangedRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, ViewChanged);

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Setting up the TextBox and StackPanel.");

            rootStackPanel = ref new xaml_controls::StackPanel();
            textBox = ref new xaml_controls::TextBox();

            loadedRegistration.Attach(rootStackPanel, ref new xaml::RoutedEventHandler([loadedEvent](Platform::Object^, xaml::RoutedEventArgs^)
            {
                LOG_OUTPUT(L"StackPanel.Loaded event raised.");
                loadedEvent->Set();
            }));

            textBox->Text = ref new Platform::String(L"This is a narrow TextBox control with an extra long content that allows vertical scrolling with the ScrollViewer.ChangeView method.");
            textBox->TextWrapping = Microsoft::UI::Xaml::TextWrapping::Wrap;
            textBox->MaxWidth = 120;
            textBox->MaxHeight = 120;
            textBox->FontSize = 15;
            textBox->Padding = xaml::Thickness({ 10, 3, 6, 5 }); // Restore old TextControlThemePadding padding value when TextBox defaulted to 15 font size instead of 14

            rootStackPanel->Children->Append(textBox);

            TestServices::WindowHelper->WindowContent = rootStackPanel;
        });

        LOG_OUTPUT(L"Waiting for StackPanel.Loaded event.");
        TestServices::WindowHelper->WaitForIdle();
        loadedEvent->WaitForDefault();
        LOG_OUTPUT(L"UI set up.");

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Finding inner ScrollViewer.");
            auto grid = safe_cast<xaml::FrameworkElement^>(xaml_media::VisualTreeHelper::GetChild(textBox, 0));
            VERIFY_IS_NOT_NULL(grid);

            int childCount = xaml_media::VisualTreeHelper::GetChildrenCount(grid);
            VERIFY_IS_GREATER_THAN(childCount, 0);

            for (int childIndex = 0; childIndex < childCount; childIndex++)
            {
                auto child = safe_cast<xaml::FrameworkElement^>(xaml_media::VisualTreeHelper::GetChild(grid, childIndex));
                scrollViewer = dynamic_cast<xaml_controls::ScrollViewer^>(child);

                if (scrollViewer) break;
            }
            VERIFY_IS_NOT_NULL(scrollViewer);
            LOG_OUTPUT(L"Inner ScrollViewer found.");

            viewChangingRegistration.Attach(
                scrollViewer,
                ref new wf::EventHandler<xaml_controls::ScrollViewerViewChangingEventArgs^>(
                    [&viewChangingCount](Platform::Object^, xaml_controls::ScrollViewerViewChangingEventArgs^ args)
            {
                LOG_OUTPUT(L"ViewChanging raised. NextView=(%.3f, %.3f, %.3f), FinalView=(%.3f, %.3f, %.3f), IsInertial=%d.",
                    args->NextView->HorizontalOffset, args->NextView->VerticalOffset, args->NextView->ZoomFactor,
                    args->FinalView->HorizontalOffset, args->FinalView->VerticalOffset, args->FinalView->ZoomFactor,
                    args->IsInertial);
                viewChangingCount++;
            }));

            viewChangedRegistration.Attach(
                scrollViewer,
                ref new wf::EventHandler<xaml_controls::ScrollViewerViewChangedEventArgs^>(
                    [viewChangedEvent, &viewChangedCount](Platform::Object^, xaml_controls::ScrollViewerViewChangedEventArgs^ args)
            {
                LOG_OUTPUT(L"ViewChanged raised. IsIntermediate=%d", args->IsIntermediate);
                viewChangedCount++;
                if (!args->IsIntermediate)
                {
                    viewChangedEvent->Set();
                }
            }));

            VERIFY_IS_GREATER_THAN(scrollViewer->ScrollableHeight, 0);

            LOG_OUTPUT(L"Invoking ScrollViewer.ChangeView(null, 150, null, false)");
            bool result = scrollViewer->ChangeView(nullptr, 150.0, nullptr, false /*disableAnimation*/);
            VERIFY_IS_TRUE(result);
        });

        LOG_OUTPUT(L"Waiting for view change completion.");
        TestServices::WindowHelper->WaitForIdle();
        viewChangedEvent->WaitForDefault();
        LOG_OUTPUT(L"View change completed.");

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Post-ChangeView view: %.3f, %.3f, %.3f.", scrollViewer->HorizontalOffset, scrollViewer->VerticalOffset, scrollViewer->ZoomFactor);
            VERIFY_ARE_EQUAL(scrollViewer->HorizontalOffset, 0.0);
            VERIFY_ARE_EQUAL(scrollViewer->ZoomFactor, 1.0f);
            VERIFY_IS_GREATER_THAN(scrollViewer->VerticalOffset, 100.0);
            verticalOffset = scrollViewer->VerticalOffset;

            VERIFY_IS_GREATER_THAN(viewChangingCount, 0);
            VERIFY_IS_GREATER_THAN(viewChangedCount, 0);
            viewChangingCount = 0;
            viewChangedCount = 0;
        });

        // Note that this code assumes that one of the two mouse wheel clicks may not be processed because of on the Phone.
        for (int wheelClicks = 0; wheelClicks < 2; wheelClicks++)
        {
            LOG_OUTPUT(L"Launching vertical scroll operation with the mouse wheel.");
            viewChangedEvent->Reset();
            TestServices::InputHelper->ScrollMouseWheel(scrollViewer, 1 /* numberOfWheelClicks */);

            TestServices::WindowHelper->WaitForIdle();
        }

        LOG_OUTPUT(L"Waiting for ScrollViewer's vertical scroll to complete.");
        viewChangedEvent->WaitForDefault();
        LOG_OUTPUT(L"Scroll completed.");

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Post-scroll view: %.3f, %.3f, %.3f.", scrollViewer->HorizontalOffset, scrollViewer->VerticalOffset, scrollViewer->ZoomFactor);
            VERIFY_IS_LESS_THAN(scrollViewer->VerticalOffset, verticalOffset);
            verticalOffset = scrollViewer->VerticalOffset;

            VERIFY_IS_GREATER_THAN(viewChangingCount, 0);
            VERIFY_IS_GREATER_THAN(viewChangedCount, 0);
            viewChangingCount = 0;
            viewChangedCount = 0;

            VERIFY_IS_LESS_THAN(verticalOffset, 100.0);

            viewChangedEvent->Reset();

            LOG_OUTPUT(L"Invoking ScrollViewer.ChangeView(null, 150, null, false)");
            bool result = scrollViewer->ChangeView(nullptr, 150.0, nullptr, false /*disableAnimation*/);
            VERIFY_IS_TRUE(result);
        });

        LOG_OUTPUT(L"Waiting for view change completion.");
        TestServices::WindowHelper->WaitForIdle();
        viewChangedEvent->WaitForDefault();
        LOG_OUTPUT(L"View change completed.");

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Post-ChangeView view: %.3f, %.3f, %.3f.", scrollViewer->HorizontalOffset, scrollViewer->VerticalOffset, scrollViewer->ZoomFactor);
            VERIFY_ARE_EQUAL(scrollViewer->HorizontalOffset, 0.0);
            VERIFY_ARE_EQUAL(scrollViewer->ZoomFactor, 1.0f);
            VERIFY_IS_GREATER_THAN(scrollViewer->VerticalOffset, verticalOffset);

            VERIFY_IS_GREATER_THAN(viewChangingCount, 0);
            VERIFY_IS_GREATER_THAN(viewChangedCount, 0);
        });
    }

    void TextBoxIntegrationTests::VerifyContextMenuAppearsAtCaretWithShiftF10()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::TextBox^ textBox = nullptr;

        auto loadedEvent = std::make_shared<Event>();
        auto selectionChangedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, Loaded);
        auto selectionChangedRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, SelectionChanged);

        RunOnUIThread([&]()
        {
            auto rootStackPanel = ref new xaml_controls::StackPanel();

            textBox = ref new xaml_controls::TextBox();
            textBox->Width = 300;
            textBox->Height = 100;
            textBox->HorizontalAlignment = xaml::HorizontalAlignment::Left;
            textBox->Margin = xaml::ThicknessHelper::FromUniformLength(50);
            textBox->Text = ref new Platform::String(L"aaaaaaaaaaa");

            loadedRegistration.Attach(textBox, [loadedEvent]() { loadedEvent->Set(); });
            selectionChangedRegistration.Attach(textBox, [selectionChangedEvent]() { selectionChangedEvent->Set(); });

            rootStackPanel->Children->Append(textBox);
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
            flyoutOpenedRegistration.Attach(textBox->ContextFlyout, [flyoutOpenedEvent]() { flyoutOpenedEvent->Set(); });
            flyoutClosedRegistration.Attach(textBox->ContextFlyout, [flyoutClosedEvent]() { flyoutClosedEvent->Set(); });
        });

        LOG_OUTPUT(L"Tap on the text box to set its cursor.");
        TestServices::InputHelper->Tap(textBox);

        selectionChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(textBox->Text->Length(), static_cast<unsigned int>(textBox->SelectionStart));
        });

        LOG_OUTPUT(L"Open the context menu using Shift+F10.");
        TestServices::KeyboardHelper->PressKeySequence(L"$d$_shift#$d$_f10#$u$_f10#$u$_shift");

        flyoutOpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"The context menu should've been opened at the bottom-right edge of the caret.");
        RunOnUIThread([&]()
        {
            wfc::IVectorView<xaml_primitives::Popup^>^ popups = xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(textBox->XamlRoot);
            VERIFY_IS_GREATER_THAN(popups->Size, 0u);

            auto flyoutPresenter = safe_cast<xaml::FrameworkElement^>(popups->GetAt(popups->Size - 1)->Child);
            auto flyoutBounds = ControlHelper::GetBounds(flyoutPresenter);

            LOG_OUTPUT(L"FlyoutPresenter bounds: %f, %f, %f, %f", flyoutBounds.X, flyoutBounds.Y, flyoutBounds.Width, flyoutBounds.Height);
            VERIFY_ARE_EQUAL(139, flyoutBounds.X);
            VERIFY_ARE_EQUAL(75, flyoutBounds.Y);
        });

        LOG_OUTPUT(L"Close the menu using the Esc key.");
        TestServices::KeyboardHelper->Escape();

        flyoutClosedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
    }

    void TextBoxIntegrationTests::VerifyContextMenuRaisesCutCopyPasteEvents()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::TextBox^ textBox = nullptr;
        xaml_controls::CommandBar^ commandBar = nullptr;

        auto loadedEvent = std::make_shared<Event>();
        auto cuttingToClipboardEvent = std::make_shared<Event>();
        auto copyingToClipboardEvent = std::make_shared<Event>();
        auto pasteEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, Loaded);
        auto cuttingToClipboardRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, CuttingToClipboard);
        auto copyingToClipboardRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, CopyingToClipboard);
        auto pasteRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, Paste);

        RunOnUIThread([&]()
        {
            auto rootStackPanel = ref new xaml_controls::StackPanel();

            textBox = ref new xaml_controls::TextBox();
            textBox->Text = ref new Platform::String(L"aaaaaaaaaaa");

            loadedRegistration.Attach(textBox, [loadedEvent]() { loadedEvent->Set(); });
            cuttingToClipboardRegistration.Attach(textBox, [cuttingToClipboardEvent]() { cuttingToClipboardEvent->Set(); });
            copyingToClipboardRegistration.Attach(textBox, [copyingToClipboardEvent]() { copyingToClipboardEvent->Set(); });
            pasteRegistration.Attach(textBox, [pasteEvent]() { pasteEvent->Set(); });

            rootStackPanel->Children->Append(textBox);
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
            flyoutOpenedRegistration.Attach(textBox->ContextFlyout, [flyoutOpenedEvent]() { flyoutOpenedEvent->Set(); });
            flyoutClosedRegistration.Attach(textBox->ContextFlyout, [flyoutClosedEvent]() { flyoutClosedEvent->Set(); });

            LOG_OUTPUT(L"Select all text in the TextBox.");
            textBox->SelectAll();
        });

        auto rightClickFunc = [textBox]()
        {
            TestServices::InputHelper->ClickMouseButton(MouseButton::Right, textBox);
        };

        LOG_OUTPUT(L"Open the context menu with a right-click.");
        rightClickFunc();

        flyoutOpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        xaml_controls::AppBarButton^ cutButton = nullptr;

        RunOnUIThread([&]()
        {
            auto popups = xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(textBox->XamlRoot);
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
            textBox->SelectAll();
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

    void TextBoxIntegrationTests::VerifyPressAndHoldOnlyShowsContextMenu()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::TextBox^ textBox = nullptr;
        xaml_controls::CommandBar^ commandBar = nullptr;

        auto loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, Loaded);

        RunOnUIThread([&]()
        {
            auto rootStackPanel = ref new xaml_controls::StackPanel();

            textBox = ref new xaml_controls::TextBox();
            textBox->Text = ref new Platform::String(L"aaaaaaaaaaa");

            loadedRegistration.Attach(textBox, [loadedEvent]() { loadedEvent->Set(); });

            rootStackPanel->Children->Append(textBox);
            TestServices::WindowHelper->WindowContent = rootStackPanel;
        });

        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        auto contextFlyoutOpenedEvent = std::make_shared<Event>();
        auto contextFlyoutClosedEvent = std::make_shared<Event>();
        auto selectionFlyoutOpenedEvent = std::make_shared<Event>();
        auto contextFlyoutOpenedRegistration = CreateSafeEventRegistration(xaml_primitives::FlyoutBase, Opened);
        auto contextFlyoutClosedRegistration = CreateSafeEventRegistration(xaml_primitives::FlyoutBase, Closed);
        auto selectionFlyoutClosedRegistration = CreateSafeEventRegistration(xaml_primitives::FlyoutBase, Closed);

        RunOnUIThread([&]()
        {
            contextFlyoutOpenedRegistration.Attach(textBox->ContextFlyout, [contextFlyoutOpenedEvent]() { contextFlyoutOpenedEvent->Set(); });
            contextFlyoutClosedRegistration.Attach(textBox->ContextFlyout, [contextFlyoutClosedEvent]() { contextFlyoutClosedEvent->Set(); });
            selectionFlyoutClosedRegistration.Attach(textBox->ContextFlyout, [selectionFlyoutOpenedEvent]() { selectionFlyoutOpenedEvent->Set(); });
        });

        LOG_OUTPUT(L"Open the context menu with a press-and-hold gesture.");
        TestServices::InputHelper->Hold(textBox);

        contextFlyoutOpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        VERIFY_IS_FALSE(selectionFlyoutOpenedEvent->HasFired());

        LOG_OUTPUT(L"Hit ESC to close the context menu.");
        TestServices::KeyboardHelper->Escape();

        contextFlyoutClosedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
    }

    void TextBoxIntegrationTests::VerifySelectingTextWithTouchShowsSelectionFlyout()
    {
        TextControlHelper::VerifySelectingTextWithTouchShowsSelectionFlyout<xaml_controls::TextBox>(
            [](xaml_controls::TextBox^ textBox) { textBox->Text = "aaaaaaaaaa"; });
    }

    void TextBoxIntegrationTests::VerifyHwndFromElementIsFocusedOne()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::TextBox^ textBox = nullptr;

        auto loadedEvent = std::make_shared<Event>();
        auto selectionChangedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, Loaded);
        auto selectionChangedRegistration = CreateSafeEventRegistration(xaml_controls::TextBox, SelectionChanged);

        RunOnUIThread([&]()
        {
            auto rootStackPanel = ref new xaml_controls::StackPanel();

            textBox = ref new xaml_controls::TextBox();
            textBox->Width = 300;
            textBox->Height = 100;
            textBox->HorizontalAlignment = xaml::HorizontalAlignment::Left;
            textBox->Margin = xaml::ThicknessHelper::FromUniformLength(50);
            textBox->Text = ref new Platform::String(L"aaaaaaaaaaa");

            loadedRegistration.Attach(textBox, [loadedEvent]() { loadedEvent->Set(); });
            selectionChangedRegistration.Attach(textBox, [selectionChangedEvent]() { selectionChangedEvent->Set(); });

            rootStackPanel->Children->Append(textBox);
            TestServices::WindowHelper->WindowContent = rootStackPanel;
        });

        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Tap on the text box to set it as focused");
        TestServices::InputHelper->Tap(textBox);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]() {
            HWND focusedWindow = GetFocus();
            HWND hwnd = reinterpret_cast<HWND>(TestServices::WindowHelper->GetElementInputWindow(textBox));
            VERIFY_ARE_EQUAL(hwnd, focusedWindow);
        });
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Controls::TextBox
