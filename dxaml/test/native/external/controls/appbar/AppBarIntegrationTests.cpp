// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "AppBarIntegrationTests.h"

#include <generic\DependencyObjectTests.h>

#include <XamlTailored.h>
#include <ControlHelper.h>
#include <TestEvent.h>
#include <FileLoader.h>
#include <FlyoutHelper.h>
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>
#include <TreeHelper.h>
#include <WUCRenderingScopeGuard.h>
#include "KeyboardInjectionOverride.h"

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;
using namespace MockDComp;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace AppBar {

    bool AppBarIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();

        // Disable control state transitions to reduce test execution time.
        featureDisableTransitionsForTest.Initialize(RuntimeFeatureBehavior::RuntimeEnabledFeature::DisableTransitionsForTest, true);
        return true;
    }

    bool AppBarIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool AppBarIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //
    void AppBarIntegrationTests::CanInstantiate()
    {
        Generic::DependencyObjectTests<xaml_controls::AppBar>::CanInstantiate();
    }

    void AppBarIntegrationTests::CanEnterAndLeaveLiveTree()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::AppBar^ appBar = nullptr;
        xaml_controls::Page^ page = nullptr;

        auto hasLoadedEvent = std::make_shared<Event>();
        auto hasUnloadedEvent = std::make_shared<Event>();

        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::AppBar, Loaded);
        auto unloadedRegistration = CreateSafeEventRegistration(xaml_controls::AppBar, Unloaded);

        // Setup our environment.
        RunOnUIThread([&]()
        {
            appBar = ref new xaml_controls::AppBar();
            loadedRegistration.Attach(appBar, [&](){ hasLoadedEvent->Set(); });
            unloadedRegistration.Attach(appBar, [&](){ hasUnloadedEvent->Set(); });

            page = TestServices::WindowHelper->SetupSimulatedAppPage();
        });
        TestServices::WindowHelper->WaitForIdle();

        // Verify enter/leave for top appbar.
        LOG_OUTPUT(L"Verify enter/leave for top appbar.");
        RunOnUIThread([&]()
        {
            page->TopAppBar = appBar;
            appBar->IsOpen = true;
        });
        hasLoadedEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            page->TopAppBar = nullptr;
        });
        hasUnloadedEvent->WaitForDefault();

        // Verify enter/leave for bottom appbar.
        LOG_OUTPUT(L"Verify enter/leave for bottom appbar.");
        RunOnUIThread([&]()
        {
            page->BottomAppBar = appBar;
            appBar->IsOpen = true;
        });
        hasLoadedEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            page->BottomAppBar = nullptr;
        });
        hasUnloadedEvent->WaitForDefault();

        // Verify enter/leave for inline appbar.
        LOG_OUTPUT(L"Verify enter/leave for inline appbar.");
        RunOnUIThread([&]()
        {
            page->Content = appBar;
            appBar->IsOpen = true;
        });
        hasLoadedEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            page->Content = nullptr;
        });
        hasUnloadedEvent->WaitForDefault();
    }

    void AppBarIntegrationTests::CanOpenAndCloseUsingAPI()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::AppBar^ appBar = nullptr;
        xaml_controls::Page^ page = nullptr;

        auto openedEvent = std::make_shared<Event>();
        auto closedEvent = std::make_shared<Event>();

        auto openedRegistration = CreateSafeEventRegistration(xaml_controls::AppBar, Opened);
        auto closedRegistration = CreateSafeEventRegistration(xaml_controls::AppBar, Closed);

        // Setup our environment.
        RunOnUIThread([&]()
        {
            appBar = ref new xaml_controls::AppBar();
            openedRegistration.Attach(appBar, [&](){ openedEvent->Set(); });
            closedRegistration.Attach(appBar, [&](){ closedEvent->Set(); });

            page = TestServices::WindowHelper->SetupSimulatedAppPage();
        });
        TestServices::WindowHelper->WaitForIdle();

        // Verify open/close for top appbar.
        LOG_OUTPUT(L"Verify open/close for top appbar.");
        RunOnUIThread([&]()
        {
            page->TopAppBar = appBar;
            appBar->IsOpen = true;
        });
        openedEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            appBar->IsOpen = false;
        });
        closedEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            page->TopAppBar = nullptr;
        });
        TestServices::WindowHelper->WaitForIdle();

        // Verify open/close for bottom appbar.
        LOG_OUTPUT(L"Verify open/close for bottom appbar.");
        RunOnUIThread([&]()
        {
            page->BottomAppBar = appBar;
            appBar->IsOpen = true;
        });
        openedEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            appBar->IsOpen = false;
        });
        closedEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            page->BottomAppBar = nullptr;
        });
        TestServices::WindowHelper->WaitForIdle();

        // Verify open/close for inline appbar.
        LOG_OUTPUT(L"Verify open/close for inline appbar.");
        RunOnUIThread([&]()
        {
            page->Content = appBar;
            appBar->IsOpen = true;
        });
        openedEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            appBar->IsOpen = false;
        });
        closedEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            page->Content = nullptr;
        });
    }

    void AppBarIntegrationTests::CanOpenAndCloseUsingKeyboard()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::AppBar^ topAppBar = nullptr;
        xaml_controls::AppBar^ bottomAppBar = nullptr;
        xaml_controls::AppBar^ inlineAppBar = nullptr;
        xaml_controls::Page^ page = SetupTopBottomInlineAppBarsPage();

        Platform::String^ contextMenuKeySequence = "$d$_apps#$u$_apps";

        auto topOpenedEvent = std::make_shared<Event>();
        auto topClosedEvent = std::make_shared<Event>();
        auto bottomOpenedEvent = std::make_shared<Event>();
        auto bottomClosedEvent = std::make_shared<Event>();
        auto inlineOpenedEvent = std::make_shared<Event>();
        auto inlineClosedEvent = std::make_shared<Event>();

        auto topOpenedRegistration = CreateSafeEventRegistration(xaml_controls::AppBar, Opened);
        auto topClosedRegistration = CreateSafeEventRegistration(xaml_controls::AppBar, Closed);
        auto bottomOpenedRegistration = CreateSafeEventRegistration(xaml_controls::AppBar, Opened);
        auto bottomClosedRegistration = CreateSafeEventRegistration(xaml_controls::AppBar, Closed);
        auto inlineOpenedRegistration = CreateSafeEventRegistration(xaml_controls::AppBar, Opened);
        auto inlineClosedRegistration = CreateSafeEventRegistration(xaml_controls::AppBar, Closed);

        RunOnUIThread([&]()
        {
            topAppBar = page->TopAppBar;
            bottomAppBar = page->BottomAppBar;
            inlineAppBar = safe_cast<xaml_controls::AppBar^>(safe_cast<xaml_controls::Panel^>(page->Content)->FindName(L"inlineAppBar"));
        });
        TestServices::WindowHelper->WaitForIdle();

        AttachOpenedAndClosedHandlers(topAppBar, topOpenedEvent, topOpenedRegistration, topClosedEvent, topClosedRegistration);
        AttachOpenedAndClosedHandlers(bottomAppBar, bottomOpenedEvent, bottomOpenedRegistration, bottomClosedEvent, bottomClosedRegistration);
        AttachOpenedAndClosedHandlers(inlineAppBar, inlineOpenedEvent, inlineOpenedRegistration, inlineClosedEvent, inlineClosedRegistration);

        LOG_OUTPUT(L"Pressing the CONTEXTMENU key opens the top and bottom AppBars only (and not the inline AppBar).");
        TestServices::KeyboardHelper->PressKeySequence(contextMenuKeySequence);
        topOpenedEvent->WaitForDefault();
        bottomOpenedEvent->WaitForDefault();
        VERIFY_IS_FALSE(inlineOpenedEvent->HasFired());

        LOG_OUTPUT(L"Pressing the CONTEXTMENU key closes the top and bottom AppBars only (and not the inline AppBar).");
        TestServices::KeyboardHelper->PressKeySequence(contextMenuKeySequence);
        topClosedEvent->WaitForDefault();
        bottomClosedEvent->WaitForDefault();
        VERIFY_IS_FALSE(inlineClosedEvent->HasFired());
    }

    void AppBarIntegrationTests::CanCloseNonStickyAppBarUsingEscapeKey()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Button^ stickyTopExpandButton = nullptr;
        xaml_controls::Button^ bottomExpandButton = nullptr;
        xaml_controls::Button^ inlineExpandButton = nullptr;
        xaml_controls::Button^ stickyInlineExpandButton = nullptr;
        xaml_controls::AppBar^ stickyTopAppBar = nullptr;
        xaml_controls::AppBar^ bottomAppBar = nullptr;
        xaml_controls::AppBar^ inlineAppBar = nullptr;
        xaml_controls::AppBar^ stickyInlineAppBar = nullptr;
        xaml_controls::Page^ page = SetupTopBottomInlineAppBarsPage();

        Platform::String^ escapeKeySequence = "$d$_esc#$u$_esc";
        Platform::String^ expectedFocusSequence = "[BEB][STEB][FEB][STEB]";
        Platform::String^ focusSequence = "";

        auto stickyTopOpenedEvent = std::make_shared<Event>();
        auto stickyTopClosedEvent = std::make_shared<Event>();
        auto bottomOpenedEvent = std::make_shared<Event>();
        auto bottomClosedEvent = std::make_shared<Event>();
        auto inlineOpenedEvent = std::make_shared<Event>();
        auto inlineClosedEvent = std::make_shared<Event>();
        auto stickyInlineOpenedEvent = std::make_shared<Event>();
        auto stickyInlineClosedEvent = std::make_shared<Event>();

        auto stickyTopOpenedRegistration = CreateSafeEventRegistration(xaml_controls::AppBar, Opened);
        auto stickyTopClosedRegistration = CreateSafeEventRegistration(xaml_controls::AppBar, Closed);
        auto bottomOpenedRegistration = CreateSafeEventRegistration(xaml_controls::AppBar, Opened);
        auto bottomClosedRegistration = CreateSafeEventRegistration(xaml_controls::AppBar, Closed);
        auto inlineOpenedRegistration = CreateSafeEventRegistration(xaml_controls::AppBar, Opened);
        auto inlineClosedRegistration = CreateSafeEventRegistration(xaml_controls::AppBar, Closed);
        auto stickyInlineOpenedRegistration = CreateSafeEventRegistration(xaml_controls::AppBar, Opened);
        auto stickyInlineClosedRegistration = CreateSafeEventRegistration(xaml_controls::AppBar, Closed);

        auto pageGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Page, GotFocus);
        auto topAppBarGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::AppBar, GotFocus);
        auto bottomAppBarGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::AppBar, GotFocus);

        RunOnUIThread([&]()
        {
            auto gotFocusHandler = ref new xaml::RoutedEventHandler([&](Platform::Object^ sender, xaml::RoutedEventArgs^ args)
                {
                    focusSequence += "[" + safe_cast<xaml::FrameworkElement^>(args->OriginalSource)->Tag + "]";
                });
            pageGotFocusRegistration.Attach(page, gotFocusHandler);
            topAppBarGotFocusRegistration.Attach(page->TopAppBar, gotFocusHandler);
            bottomAppBarGotFocusRegistration.Attach(page->BottomAppBar, gotFocusHandler);

            stickyTopAppBar = page->TopAppBar;
            stickyTopAppBar->IsSticky = true;

            bottomAppBar = page->BottomAppBar;

            auto panel = safe_cast<xaml_controls::Panel^>(page->Content);
            inlineAppBar = safe_cast<xaml_controls::AppBar^>(panel->FindName(L"inlineAppBar"));

            stickyInlineAppBar = safe_cast<xaml_controls::AppBar^>(xaml_markup::XamlReader::Load(
                LR"(<AppBar xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"
                        x:Name="stickyInlineAppBar" IsSticky="True">
                        <Rectangle Width="100" Height="60" HorizontalAlignment="Left" VerticalAlignment="Top" Fill="Red"/>
                    </AppBar>)"));
            panel->Children->Append(stickyInlineAppBar);
        });
        TestServices::WindowHelper->WaitForIdle();

        AttachOpenedAndClosedHandlers(stickyTopAppBar, stickyTopOpenedEvent, stickyTopOpenedRegistration, stickyTopClosedEvent, stickyTopClosedRegistration);
        AttachOpenedAndClosedHandlers(bottomAppBar, bottomOpenedEvent, bottomOpenedRegistration, bottomClosedEvent, bottomClosedRegistration);
        AttachOpenedAndClosedHandlers(inlineAppBar, inlineOpenedEvent, inlineOpenedRegistration, inlineClosedEvent, inlineClosedRegistration);
        AttachOpenedAndClosedHandlers(stickyInlineAppBar, stickyInlineOpenedEvent, stickyInlineOpenedRegistration, stickyInlineClosedEvent, stickyInlineClosedRegistration);

        // Setup for the test by opening the AppBars.
        RunOnUIThread([&]()
        {
            stickyTopExpandButton = safe_cast<xaml_controls::Button^>(TreeHelper::GetVisualChildByName(stickyTopAppBar, L"ExpandButton"));
            stickyTopExpandButton->Tag = "STEB";

            bottomExpandButton = safe_cast<xaml_controls::Button^>(TreeHelper::GetVisualChildByName(bottomAppBar, L"ExpandButton"));
            bottomExpandButton->Tag = "BEB";

            inlineExpandButton = safe_cast<xaml_controls::Button^>(TreeHelper::GetVisualChildByName(inlineAppBar, L"ExpandButton"));
            inlineExpandButton->Tag = "FEB";

            stickyInlineExpandButton = safe_cast<xaml_controls::Button^>(TreeHelper::GetVisualChildByName(stickyInlineAppBar, L"ExpandButton"));
            stickyInlineExpandButton->Tag = "SFEB";

            stickyTopAppBar->IsOpen = true;
            bottomAppBar->IsOpen = true;
            inlineAppBar->IsOpen = true;
            stickyInlineAppBar->IsOpen = true;
        });
        stickyTopOpenedEvent->WaitForDefault();
        bottomOpenedEvent->WaitForDefault();
        inlineOpenedEvent->WaitForDefault();
        stickyInlineOpenedEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            focusSequence = "";

            // Start with the focus on the bottom AppBar expand button.
            bottomExpandButton->Focus(xaml::FocusState::Programmatic);
        });

        // Tab once to move to the sticky, top AppBar expand button.
        TestServices::KeyboardHelper->Tab();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Try closing the non-sticky bottom AppBar using ESC key.");
        TestServices::KeyboardHelper->PressKeySequence(escapeKeySequence);
        bottomClosedEvent->WaitForDefault();

        LOG_OUTPUT(L"Try closing the non-sticky inline Appbar using ESC key.");
        TestServices::KeyboardHelper->PressKeySequence(escapeKeySequence);
        inlineClosedEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            // Verify that IsOpen is set to false for all AppBars EXCEPT the sticky ones.
            VERIFY_IS_TRUE(stickyTopAppBar->IsOpen);
            VERIFY_IS_FALSE(bottomAppBar->IsOpen);
            VERIFY_IS_FALSE(inlineAppBar->IsOpen);
            VERIFY_IS_TRUE(stickyInlineAppBar->IsOpen);

            LOG_OUTPUT(L"Expected focus sequence: %s", expectedFocusSequence->Data());
            LOG_OUTPUT(L"Actual focus sequence: %s", focusSequence->Data());
            VERIFY_ARE_EQUAL(expectedFocusSequence, focusSequence);
        });
    }

    void AppBarIntegrationTests::CanOpenAndCloseUsingMouse()
    {
        CanOpenAndCloseUsingRightTappedEvent(false /* usePen */);
    }

    void AppBarIntegrationTests::CanOpenAndCloseUsingPen()
    {
        CanOpenAndCloseUsingRightTappedEvent(true /* usePen */);
    }

    void AppBarIntegrationTests::CanOpenAndCloseUsingRightTappedEvent(bool usePen)
    {
        TestCleanupWrapper cleanup;

        xaml_controls::AppBar^ topAppBar = nullptr;
        xaml_controls::AppBar^ bottomAppBar = nullptr;
        xaml_controls::Page^ page = nullptr;

        wf::Rect pageBounds = {};

        auto topOpenedEvent = std::make_shared<Event>();
        auto topClosedEvent = std::make_shared<Event>();
        auto bottomOpenedEvent = std::make_shared<Event>();
        auto bottomClosedEvent = std::make_shared<Event>();

        auto topOpenedRegistration = CreateSafeEventRegistration(xaml_controls::AppBar, Opened);
        auto topClosedRegistration = CreateSafeEventRegistration(xaml_controls::AppBar, Closed);
        auto bottomOpenedRegistration = CreateSafeEventRegistration(xaml_controls::AppBar, Opened);
        auto bottomClosedRegistration = CreateSafeEventRegistration(xaml_controls::AppBar, Closed);

        // Setup our environment.
        RunOnUIThread([&]()
        {
            topAppBar = ref new xaml_controls::AppBar();
            topOpenedRegistration.Attach(topAppBar, [&](){ topOpenedEvent->Set(); });
            topClosedRegistration.Attach(topAppBar, [&](){ topClosedEvent->Set(); });

            bottomAppBar = ref new xaml_controls::AppBar();
            bottomOpenedRegistration.Attach(bottomAppBar, [&](){ bottomOpenedEvent->Set(); });
            bottomClosedRegistration.Attach(bottomAppBar, [&](){ bottomClosedEvent->Set(); });

            page = TestServices::WindowHelper->SetupSimulatedAppPage();
            page->TopAppBar = topAppBar;
            page->BottomAppBar = bottomAppBar;

            page->Focus(xaml::FocusState::Keyboard);
        });
        TestServices::WindowHelper->WaitForIdle();

        if (usePen)
        {
            LOG_OUTPUT(L"Open the appbars using pen + barrel button.");
            TestServices::InputHelper->PenBarrelTap(page);
        }
        if (!usePen)
        {
            LOG_OUTPUT(L"Open the appbars using right mouse click.");
            TestServices::InputHelper->ClickMouseButton(MouseButton::Right, page);
        }

        topOpenedEvent->WaitForDefault();
        bottomOpenedEvent->WaitForDefault();

        if (usePen)
        {
            LOG_OUTPUT(L"Open the appbars using pen + barrel button.");
            TestServices::InputHelper->PenBarrelTap(page);
        }
        else
        {
            LOG_OUTPUT(L"Close the appbars using right mouse click.");
            TestServices::InputHelper->ClickMouseButton(MouseButton::Right, page);
        }

        topClosedEvent->WaitForDefault();
        bottomClosedEvent->WaitForDefault();
    }

    void AppBarIntegrationTests::CanOpenMinimalAppBarUsingMouse()
    {
        TestCleanupWrapper cleanup;

        auto page = SetupTopBottomInlineAppBarsPage();

        xaml_controls::AppBar^ topAppBar = nullptr;
        xaml_controls::AppBar^ bottomAppBar = nullptr;
        xaml_controls::AppBar^ inlineAppBar = nullptr;

        RunOnUIThread([&]()
        {
            topAppBar = page->TopAppBar;
            bottomAppBar = page->BottomAppBar;
            inlineAppBar = safe_cast<xaml_controls::AppBar^>(safe_cast<xaml_controls::Panel^>(page->Content)->FindName(L"inlineAppBar"));
        });
        TestServices::WindowHelper->WaitForIdle();

        CanOpenMinimalAppBarUsingMouseHelper(topAppBar);
        CanOpenMinimalAppBarUsingMouseHelper(bottomAppBar);
        CanOpenMinimalAppBarUsingMouseHelper(inlineAppBar);
    }

    void AppBarIntegrationTests::CanOpenMinimalAppBarUsingMouseHelper(xaml_controls::AppBar^ appBar)
    {
        wf::Rect appBarBounds = {};

        RunOnUIThread([&]()
        {
            // We are only using AppBarClosedDisplayMode::Minimal since only Minimal AppBars can
            // be opened by clicking on the bar itself.
            appBar->ClosedDisplayMode = xaml_controls::AppBarClosedDisplayMode::Minimal;
        });
        TestServices::WindowHelper->WaitForIdle();

        auto openedEvent = std::make_shared<Event>();
        auto closedEvent = std::make_shared<Event>();
        auto openedRegistration = CreateSafeEventRegistration(xaml_controls::AppBar, Opened);
        auto closedRegistration = CreateSafeEventRegistration(xaml_controls::AppBar, Closed);
        AttachOpenedAndClosedHandlers(appBar, openedEvent, openedRegistration, closedEvent, closedRegistration);

        TestServices::WindowHelper->WaitForIdle();

        // Click the center of the minimal AppBars where there are no buttons, to open it.
        TestServices::InputHelper->ClickMouseButton(MouseButton::Left, appBar);
        openedEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            appBar->IsOpen = false;
        });
        closedEvent->WaitForDefault();
    }

    void AppBarIntegrationTests::CanOpenAndCloseUsingExpandButton()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Page^ page = SetupClosedDisplayModeTestEnvironment(true /* setClosedDisplayModeValues */);
        xaml_controls::Button^ expandButton = nullptr;

        auto bottomOpenedEvent = std::make_shared<Event>();
        auto bottomClosedEvent = std::make_shared<Event>();

        auto bottomOpenedRegistration = CreateSafeEventRegistration(xaml_controls::AppBar, Opened);
        auto bottomClosedRegistration = CreateSafeEventRegistration(xaml_controls::AppBar, Closed);

        RunOnUIThread([&]()
        {
            bottomOpenedRegistration.Attach(page->BottomAppBar, [&](){ bottomOpenedEvent->Set(); });
            bottomClosedRegistration.Attach(page->BottomAppBar, [&](){ bottomClosedEvent->Set(); });

            expandButton = safe_cast<xaml_controls::Button^>(TreeHelper::GetVisualChildByName(page->BottomAppBar, Platform::StringReference(L"ExpandButton").GetString()));
        });

        TestServices::InputHelper->Tap(expandButton);

        bottomOpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(page->BottomAppBar->IsOpen);
        });

        TestServices::InputHelper->Tap(expandButton);

        bottomClosedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_FALSE(page->BottomAppBar->IsOpen);
        });
    }

    void AppBarIntegrationTests::CanTabThroughChildItems()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::AppBar^ appBar = nullptr;
        xaml_controls::Page^ page = nullptr;
        xaml_controls::Button^ button = nullptr;

        Platform::String^ focusSequence = "";

        std::vector<SafeEventRegistrationType(xaml_controls::AppBarButton, GotFocus)> topGotFocusRegistrations;
        std::vector<SafeEventRegistrationType(xaml_controls::AppBarButton, GotFocus)> bottomGotFocusRegistrations;
        auto buttonGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);

        const size_t itemsPerBar = 3;
        Platform::String^ expectedFocusSequence = "[B0][B1][B2][T0][T1][T2][BTN][B0]";

        // Setup our environment.
        RunOnUIThread([&]()
        {
            page = TestServices::WindowHelper->SetupSimulatedAppPage();
            page->TopAppBar = ref new xaml_controls::AppBar();
            page->TopAppBar->IsOpen = true;
            page->TopAppBar->IsSticky = true;

            auto topStackPanel = ref new xaml_controls::StackPanel();
            topStackPanel->Orientation = xaml_controls::Orientation::Horizontal;

            auto gotFocusHandler = ref new xaml::RoutedEventHandler([&](Platform::Object^ sender, xaml::RoutedEventArgs^ args)
            {
                focusSequence += "[" + safe_cast<xaml::FrameworkElement^>(sender)->Name + "]";
            });
            for (size_t i = 0; i < itemsPerBar; ++i)
            {
                auto button = ref new xaml_controls::AppBarButton();
                button->Name = "T" + static_cast<wchar_t>(L'0' + i);
                topStackPanel->Children->Append(button);

                auto gotFocusRegistration = CreateSafeEventRegistration(xaml_controls::AppBarButton, GotFocus);
                gotFocusRegistration.Attach(button, gotFocusHandler);
                topGotFocusRegistrations.push_back(std::move(gotFocusRegistration));
            }

            page->BottomAppBar = ref new xaml_controls::AppBar();
            page->BottomAppBar->IsOpen = true;
            page->BottomAppBar->IsSticky = true;

            auto bottomStackPanel = ref new xaml_controls::StackPanel();
            bottomStackPanel->Orientation = xaml_controls::Orientation::Horizontal;

            for (size_t i = 0; i < itemsPerBar; ++i)
            {
                auto button = ref new xaml_controls::AppBarButton();
                button->Name = "B" + static_cast<wchar_t>(L'0' + i);
                bottomStackPanel->Children->Append(button);

                auto gotFocusRegistration = CreateSafeEventRegistration(xaml_controls::AppBarButton, GotFocus);
                gotFocusRegistration.Attach(button, gotFocusHandler);
                bottomGotFocusRegistrations.push_back(std::move(gotFocusRegistration));
            }

            button = ref new xaml_controls::Button();
            button->Name = "BTN";
            buttonGotFocusRegistration.Attach(button, gotFocusHandler);

            page->Content = button;
            page->TopAppBar->Content = topStackPanel;
            page->BottomAppBar->Content = bottomStackPanel;

            TestServices::WindowHelper->WindowContent = page;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // Start the test off with the button focused.
            button->Focus(xaml::FocusState::Programmatic);
        });
        TestServices::WindowHelper->WaitForIdle();

        // Clear out the focus sequence before we run our scenario.
        focusSequence = "";

        LOG_OUTPUT(L"Validate tabbing through app bar items and page content.");

        // We iterate (itemsPerBar * 2 + 4) times because we want to
        // tab through all the top and bottom app bar items, the expand buttons, and the
        // button in the page's content, and then back into the first
        // app bar item in the bottom app bar.
        for (size_t i = 0; i < (itemsPerBar * 2 + 4); ++i)
        {
            TestServices::KeyboardHelper->Tab();
            TestServices::WindowHelper->WaitForIdle();
        }

        LOG_OUTPUT(L"Expected focus sequence: %s", expectedFocusSequence->Data());
        LOG_OUTPUT(L"Actual focus sequence: %s", focusSequence->Data());
        VERIFY_ARE_EQUAL(expectedFocusSequence, focusSequence);
    }

    void AppBarIntegrationTests::CanClickAButtonInAnAppBar()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::AppBar^ appBar = nullptr;
        xaml_controls::Page^ page = nullptr;
        xaml_controls::AppBarButton^ button = nullptr;

        // Make these shared_ptr's because we're going to capture them
        // in off-thread lambdas.
        auto appBarLoadedEvent = std::make_shared<Event>();
        auto clickedEvent = std::make_shared<Event>();

        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::AppBar, Loaded);
        auto clickRegistration = CreateSafeEventRegistration(xaml_controls::AppBarButton, Click);

        // Setup our environment.
        RunOnUIThread([&]()
        {
            button = ref new xaml_controls::AppBarButton();
            clickRegistration.Attach(button, [&](){ clickedEvent->Set(); });

            appBar = ref new xaml_controls::AppBar();
            appBar->ClosedDisplayMode = xaml_controls::AppBarClosedDisplayMode::Compact;
            appBar->Content = button;
            loadedRegistration.Attach(appBar, [&](){ appBarLoadedEvent->Set(); });

            page = TestServices::WindowHelper->SetupSimulatedAppPage();
            page->TopAppBar = appBar;
        });
        appBarLoadedEvent->WaitForDefault();

        // Wait for edge theme animation to finish.
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Validate clicking a button in the top app bar.");
        TestServices::InputHelper->Tap(button);
        clickedEvent->WaitForDefault();

        LOG_OUTPUT(L"Validate clicking a button in the bottom app bar.");
        RunOnUIThread([&]
        {
            page->TopAppBar = nullptr;
            page->BottomAppBar = appBar;
        });
        TestServices::WindowHelper->WaitForIdle();

        TestServices::InputHelper->Tap(button);
        clickedEvent->WaitForDefault();

        LOG_OUTPUT(L"Validate clicking a button in an inline app bar.");
        RunOnUIThread([&]
        {
            page->BottomAppBar = nullptr;
            page->Content = appBar;
        });
        TestServices::WindowHelper->WaitForIdle();

        TestServices::InputHelper->Tap(button);
        clickedEvent->WaitForDefault();
    }

    void AppBarIntegrationTests::CanGetAndSetClosedDisplayMode()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Page^ page = nullptr;

        page = SetupClosedDisplayModeTestEnvironment(false /* setClosedDisplayModeValues */);

        RunOnUIThread([&]()
        {
            // Validate default value.
            VERIFY_ARE_EQUAL(page->TopAppBar->ClosedDisplayMode, xaml_controls::AppBarClosedDisplayMode::Minimal);

            page->TopAppBar->ClosedDisplayMode = xaml_controls::AppBarClosedDisplayMode::Compact;
            VERIFY_ARE_EQUAL(page->TopAppBar->ClosedDisplayMode, xaml_controls::AppBarClosedDisplayMode::Compact);
        });
    }

    void AppBarIntegrationTests::CanClosedDisplayModesControlLayout()
    {
        TestCleanupWrapper cleanup;

        auto validationRules = ref new Platform::String(DefaultUIElementTreeValidationRules);

        // Set the content first before send Tab() key not to hang for injecting input.
        RunOnUIThread([&]()
        {
            TestServices::WindowHelper->WindowContent = ref new xaml_controls::Grid();
        });
        TestServices::WindowHelper->WaitForIdle();

        // AppBars give programmatic focus to their first button, which shows up
        // in the FocusState property on the button in the visual tree dump.
        // Programmatic focus sets FocusState to whatever was the last
        // method of user input, which means that, without something to make it
        // deterministic, FocusState will be whatever the previous test did
        // in terms of user input.  This leads to inconsistent FocusState values.
        // To fix that issue, we inject keyboard input here to ensure that
        // the last method of user input is always keyboard, so we'll have a
        // consistent value for FocusState in the visual tree dump.
        TestServices::KeyboardHelper->Tab();

        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 300));

        xaml_controls::Page^ page = SetupClosedDisplayModeTestEnvironment(true /* setClosedDisplayModeValues */);

        auto topOpenedEvent = std::make_shared<Event>();
        auto bottomOpenedEvent = std::make_shared<Event>();

        auto topOpenedRegistration = CreateSafeEventRegistration(xaml_controls::AppBar, Opened);
        auto bottomOpenedRegistration = CreateSafeEventRegistration(xaml_controls::AppBar, Opened);

        TestServices::Utilities->VerifyUIElementTreeWithRulesInline("Closed", validationRules);

        RunOnUIThread([&]()
        {
            topOpenedRegistration.Attach(page->TopAppBar, [&](){ topOpenedEvent->Set(); });
            bottomOpenedRegistration.Attach(page->BottomAppBar, [&](){ bottomOpenedEvent->Set(); });

            page->TopAppBar->IsOpen = true;
            page->TopAppBar->LightDismissOverlayMode = xaml_controls::LightDismissOverlayMode::Off;

            page->BottomAppBar->IsOpen = true;
            page->BottomAppBar->LightDismissOverlayMode = xaml_controls::LightDismissOverlayMode::Off;
        });

        topOpenedEvent->WaitForDefault();
        bottomOpenedEvent->WaitForDefault();

        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyUIElementTreeWithRulesInline("Open", validationRules);
    }

    void AppBarIntegrationTests::CanHideAppBarWithHiddenClosedDisplayMode()
    {
        TestCleanupWrapper cleanup;

        auto page = SetupClosedDisplayModeTestEnvironment(true /* setClosedDisplayModeValues */);

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Set TopAppBar.ClosedDisplayMode to Hidden.");
            page->TopAppBar->ClosedDisplayMode = xaml_controls::AppBarClosedDisplayMode::Hidden;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(page->TopAppBar->ActualHeight, 0);

            LOG_OUTPUT(L"Now set TopAppBar.ClosedDisplayMode back to Minimal.");
            page->TopAppBar->ClosedDisplayMode = xaml_controls::AppBarClosedDisplayMode::Minimal;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_GREATER_THAN(page->TopAppBar->ActualHeight, 0);
        });
    }

    void AppBarIntegrationTests::CanChangeAppBarHeightWithClosedDisplayMode()
    {
        TestCleanupWrapper cleanup;

        double originalTopAppBarHeight = 0;
        xaml_controls::AppBar^ appBar = nullptr;

        RunOnUIThread([&]()
        {
            auto root = safe_cast<xaml::FrameworkElement^>(xaml_markup::XamlReader::Load(
                LR"(<Grid xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml">
                        <AppBar x:Name="appBar" ClosedDisplayMode="Minimal" Width="320"/>
                    </Grid>)"));

            appBar = safe_cast<xaml_controls::AppBar^>(root->FindName(L"appBar"));

            TestServices::WindowHelper->WindowContent = root;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            originalTopAppBarHeight = appBar->ActualHeight;

            LOG_OUTPUT(L"Set ClosedDisplayMode to Compact - its actual height should become larger.");
            appBar->ClosedDisplayMode = xaml_controls::AppBarClosedDisplayMode::Compact;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_GREATER_THAN(appBar->ActualHeight, originalTopAppBarHeight);

            LOG_OUTPUT(L"Now set ClosedDisplayMode back to Minimal - its actual height should return to its original height.");
            appBar->ClosedDisplayMode = xaml_controls::AppBarClosedDisplayMode::Minimal;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(appBar->ActualHeight, originalTopAppBarHeight);
        });
    }

    void AppBarIntegrationTests::ValidateFocusShiftWhenClosedAppBarIsAdded()
    {
        TestCleanupWrapper cleanup;

        auto page = SetupFocusShiftTestPage();
        auto appBar = CreateFocusShiftTestAppBar(false);
        xaml_controls::Button^ pageButton = nullptr;

        // Start the focus on the button on the page.
        RunOnUIThread([&]()
        {
            pageButton = safe_cast<xaml_controls::Button^>(TreeHelper::GetVisualChildByName(page, L"PageButton"));
            pageButton->Focus(xaml::FocusState::Keyboard);
        });
        TestServices::WindowHelper->WaitForIdle();

        // Add Closed Compact AppBar dynamically.
        RunOnUIThread([&]()
        {
            page->BottomAppBar = appBar;
        });
        TestServices::WindowHelper->WaitForIdle();

        // Verify that the focus stayed on the pageButton.
        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(pageButton));
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    void AppBarIntegrationTests::ValidateFocusShiftWhenOpenedAppBarIsAdded()
    {
        TestCleanupWrapper cleanup;

        auto page = SetupFocusShiftTestPage();
        auto appBar = CreateFocusShiftTestAppBar(true);
        xaml_controls::Button^ pageButton = nullptr;
        xaml_controls::Button^ appBarButton = nullptr;

        // Start the focus on the button on the page.
        RunOnUIThread([&]()
        {
            pageButton = safe_cast<xaml_controls::Button^>(TreeHelper::GetVisualChildByName(page, L"PageButton"));
            pageButton->Focus(xaml::FocusState::Keyboard);
        });
        TestServices::WindowHelper->WaitForIdle();

        // Add Opened Compact AppBar dynamically.
        RunOnUIThread([&]()
        {
            page->BottomAppBar = appBar;
        });
        TestServices::WindowHelper->WaitForIdle();

        // Verify that the focus moved onto the appBarButton.
        RunOnUIThread([&]()
        {
            appBarButton = safe_cast<xaml_controls::Button^>(TreeHelper::GetVisualChildByName(page->BottomAppBar, L"AppBarButton"));
            VERIFY_IS_TRUE(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(appBarButton));
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    void AppBarIntegrationTests::ValidateFocusShiftWhenClosedUnfocusedAppBarIsOpenedAndClosed()
    {
        TestCleanupWrapper cleanup;

        auto page = SetupFocusShiftTestPage();
        auto appBar = CreateFocusShiftTestAppBar(false);
        xaml_controls::Button^ pageButton = nullptr;
        xaml_controls::Button^ appBarButton = nullptr;

        // Start the focus on the button on the page.
        RunOnUIThread([&]()
        {
            pageButton = safe_cast<xaml_controls::Button^>(TreeHelper::GetVisualChildByName(page, L"PageButton"));
            pageButton->Focus(xaml::FocusState::Keyboard);
        });
        TestServices::WindowHelper->WaitForIdle();

        // Add Closed Compact AppBar dynamically.
        RunOnUIThread([&]()
        {
            page->BottomAppBar = appBar;
        });
        TestServices::WindowHelper->WaitForIdle();

        // Then, open the AppBar programmatically.
        RunOnUIThread([&]()
        {
            appBar->IsOpen = true;
        });
        TestServices::WindowHelper->WaitForIdle();

        // Verify that the focus moved to the appBarButton (first focusable element of the AppBar) since the AppBar was opened.
        // Then, close the AppBar programmatically.
        RunOnUIThread([&]()
        {
            appBarButton = safe_cast<xaml_controls::Button^>(TreeHelper::GetVisualChildByName(page->BottomAppBar, L"AppBarButton"));
            VERIFY_IS_TRUE(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(appBarButton));
            appBar->IsOpen = false;
        });
        TestServices::WindowHelper->WaitForIdle();

        // Verify that the focus moved back to the pageButton (previously focused element) since the AppBar was closed.
        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(pageButton));
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    void AppBarIntegrationTests::ValidateFocusShiftWhenClosedFocusedAppBarIsOpenedAndClosed()
    {
        TestCleanupWrapper cleanup;

        auto page = SetupFocusShiftTestPage();
        auto appBar = CreateFocusShiftTestAppBar(false);
        xaml_controls::Button^ expandButton = nullptr;

        // Add Closed Compact AppBar dynamically.
        RunOnUIThread([&]()
        {
            page->BottomAppBar = appBar;
        });
        TestServices::WindowHelper->WaitForIdle();

        // Start the focus on the expandButton of the AppBar.
        RunOnUIThread([&]()
        {
            expandButton = safe_cast<xaml_controls::Button^>(TreeHelper::GetVisualChildByName(page->BottomAppBar, L"ExpandButton"));
            expandButton->Focus(xaml::FocusState::Keyboard);
        });
        TestServices::WindowHelper->WaitForIdle();

        // Then, open the AppBar programmatically.
        RunOnUIThread([&]()
        {
            appBar->IsOpen = true;
        });
        TestServices::WindowHelper->WaitForIdle();

        // Verify that the focus stays on the expandButton since the AppBar already had focus.
        // Then, close the AppBar programmatically.
        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(expandButton));
            appBar->IsOpen = false;
        });
        TestServices::WindowHelper->WaitForIdle();

        // Verify that the focus is still on the expandButton after the AppBar closed.
        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(expandButton));
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    void AppBarIntegrationTests::CanResizeAppBarAfterOpeningAndClosing()
    {
        TestCleanupWrapper cleanup;

        auto page = SetupClosedDisplayModeTestEnvironment(true /* setClosedDisplayModeValues */);
        xaml_controls::Button^ expandButton = nullptr;
        wf::Point originalExpandButtonPosition;

        RunOnUIThread([&]()
        {
            expandButton = safe_cast<xaml_controls::Button^>(TreeHelper::GetVisualChildByName(page->BottomAppBar, Platform::StringReference(L"ExpandButton").GetString()));

            page->BottomAppBar->Margin = xaml::Thickness({ 0, 0, 300, 0 });
            page->BottomAppBar->IsOpen = true;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            originalExpandButtonPosition = expandButton->TransformToVisual(nullptr)->TransformPoint(wf::Point(0, 0));
            page->BottomAppBar->IsOpen = false;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            page->BottomAppBar->Margin = xaml::Thickness({ 0, 0, 0, 0 });
            page->BottomAppBar->IsOpen = true;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto newExpandButtonPosition = expandButton->TransformToVisual(nullptr)->TransformPoint(wf::Point(0, 0));
            VERIFY_IS_GREATER_THAN(newExpandButtonPosition.X, originalExpandButtonPosition.X);
        });
    }

    void AppBarIntegrationTests::ValidateInlineAppBars()
    {
        ControlHelper::ValidateUIElementTree(
            wf::Size(400, 600),
            1.f,
            []()
            {
                {
                    // Inject a tab to ensure a consistent FocusState when we create
                    // the visual tree dump.
                    KeyboardInjectionIgnoreEventWaitOverride keyboardEventsOverride;
                    TestServices::KeyboardHelper->Tab();
                }
                xaml_controls::Grid^ rootGrid = nullptr;

                RunOnUIThread([&]()
                {
                    rootGrid = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
                        LR"(<Grid xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml">
                                <Grid.RowDefinitions>
                                    <RowDefinition/>
                                    <RowDefinition/>
                                    <RowDefinition/>
                                    <RowDefinition/>
                                    <RowDefinition/>
                                    <RowDefinition/>
                                </Grid.RowDefinitions>
                                <AppBar Grid.Row="0" IsOpen="False" IsSticky="True" ClosedDisplayMode="Compact" LightDismissOverlayMode="Off"/>
                                <AppBar Grid.Row="1" IsOpen="True" IsSticky="True" ClosedDisplayMode="Compact" LightDismissOverlayMode="Off"/>
                                <AppBar Grid.Row="2" IsOpen="False" IsSticky="True" ClosedDisplayMode="Minimal" LightDismissOverlayMode="Off"/>
                                <AppBar Grid.Row="3" IsOpen="True" IsSticky="True" ClosedDisplayMode="Minimal" LightDismissOverlayMode="Off"/>
                                <AppBar Grid.Row="4" IsOpen="False" IsSticky="True" ClosedDisplayMode="Hidden" LightDismissOverlayMode="Off"/>
                                <AppBar Grid.Row="5" IsOpen="True" IsSticky="True" ClosedDisplayMode="Hidden" LightDismissOverlayMode="Off"/>
                            </Grid>)"));

                    TestServices::WindowHelper->WindowContent = rootGrid;
                });
                TestServices::WindowHelper->WaitForIdle();

                return rootGrid;
            },
            nullptr /*cleanupFunc*/,
            true /*disableHitTestingOnRoot*/,
            true /*ignorePopups*/
        );
    }

    void AppBarIntegrationTests::ValidateAppBarWithParentedLTE()
    {
        // ParentedLTE here refers to the fact that an AppBar placed inside a PopupRoot or
        // a MediaElementRoot will have it's LTE explicitly parented. This scenario was not
        // being covered by existing tests and we needed to cover this, even though AppBar
        // within a PopupRoot is a rare actual scenario, to safeguard against breaking
        // CommandBar that is used by MediaElement.
        TestCleanupWrapper cleanup;

        xaml_controls::AppBar^ appBar = nullptr;
        xaml_controls::Flyout^ flyout = nullptr;

        auto target = FlyoutHelper::CreateTarget(
            100 /*width*/, 100 /*height*/,
            ThicknessHelper::FromUniformLength(100),
            xaml::HorizontalAlignment::Center,
            xaml::VerticalAlignment::Center);

        RunOnUIThread([&]()
        {
            auto rootPanel = ref new xaml_controls::Grid();
            rootPanel->Children->Append(target);

            appBar = safe_cast<xaml_controls::AppBar^>(xaml_markup::XamlReader::Load(
                LR"(<AppBar xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" Width="200" Height="80">
                        <AppBarButton Label="AppBarButton"/>
                    </AppBar>)"));

            flyout = ref new xaml_controls::Flyout();
            flyout->Content = appBar;

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        // Validate that we can open and close this AppBar placed inside the Popup.
        auto openedEvent = std::make_shared<Event>();
        auto closedEvent = std::make_shared<Event>();

        auto openedRegistration = CreateSafeEventRegistration(xaml_controls::AppBar, Opened);
        auto closedRegistration = CreateSafeEventRegistration(xaml_controls::AppBar, Closed);

        AttachOpenedAndClosedHandlers(appBar, openedEvent, openedRegistration, closedEvent, closedRegistration);

        FlyoutHelper::OpenFlyout(flyout, target, FlyoutOpenMethod::Programmatic_ShowAt);

        RunOnUIThread([&]()
        {
            appBar->IsOpen = true;
        });
        openedEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            appBar->IsOpen = false;
        });
        closedEvent->WaitForDefault();

        FlyoutHelper::HideFlyout(flyout);
    }

    void AppBarIntegrationTests::CanClosedDisplayModesAffectTabbingWhenClosed()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::Page^ rootPage = nullptr;
        xaml_controls::Button^ button = nullptr;

        auto loadedEvent = std::make_shared<Event>();

        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::Page, Loaded);
        auto pageGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Page, GotFocus);
        auto topAppBarGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::AppBar, GotFocus);
        auto bottomAppBarGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::AppBar, GotFocus);

        Platform::String^ xamlFile = GetPackageFolder() + L"resources\\native\\controls\\AppBar\\AppBarForKeyboarding.xaml";
        rootPage = safe_cast<xaml_controls::Page^>(LoadXamlFileOnUIThread(xamlFile));
        loadedRegistration.Attach(rootPage, [&](){ loadedEvent->Set(); });

        Platform::String^ focusSequence = "";
        Platform::String^ expectedFocusSequence = "[AB4][AB5][AB6][BEB][TEB][B]";

        RunOnUIThread([&]()
        {
            button = safe_cast<xaml_controls::Button^>(TreeHelper::GetVisualChildByName(rootPage, L"ExternalButton"));

            auto gotFocusHandler = ref new xaml::RoutedEventHandler([&](Platform::Object^ sender, xaml::RoutedEventArgs^ args)
                {
                    focusSequence += "[" + safe_cast<xaml::FrameworkElement^>(args->OriginalSource)->Tag + "]";
                });
            pageGotFocusRegistration.Attach(rootPage, gotFocusHandler);
            topAppBarGotFocusRegistration.Attach(rootPage->TopAppBar, gotFocusHandler);
            bottomAppBarGotFocusRegistration.Attach(rootPage->BottomAppBar, gotFocusHandler);

            TestServices::WindowHelper->WindowContent = rootPage;
        });
        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            xaml_controls::Button^ topExpandButton = safe_cast<xaml_controls::Button^>(TreeHelper::GetVisualChildByName(rootPage->TopAppBar, L"ExpandButton"));
            xaml_controls::Button^ bottomExpandButton = safe_cast<xaml_controls::Button^>(TreeHelper::GetVisualChildByName(rootPage->BottomAppBar, L"ExpandButton"));

            topExpandButton->Tag = "TEB";
            bottomExpandButton->Tag = "BEB";
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            button->Focus(xaml::FocusState::Programmatic);
        });
        TestServices::WindowHelper->WaitForIdle();

        focusSequence = "";

        LOG_OUTPUT(L"Tab six times, which should move focus back to the external button.");

        for (size_t i = 0; i < 6; ++i)
        {
            TestServices::KeyboardHelper->Tab();
            TestServices::WindowHelper->WaitForIdle();
        }

        LOG_OUTPUT(L"Expected focus sequence: %s", expectedFocusSequence->Data());
        LOG_OUTPUT(L"Actual focus sequence: %s", focusSequence->Data());
        VERIFY_ARE_EQUAL(expectedFocusSequence, focusSequence);
    }

    void AppBarIntegrationTests::CanClosedDisplayModesAffectTabbingWhenOpen()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::Page^ rootPage = nullptr;
        xaml_controls::Button^ button = nullptr;

        auto loadedEvent = std::make_shared<Event>();

        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::Page, Loaded);
        auto pageGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Page, GotFocus);
        auto topAppBarGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::AppBar, GotFocus);
        auto bottomAppBarGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::AppBar, GotFocus);

        Platform::String^ xamlFile = GetPackageFolder() + L"resources\\native\\controls\\AppBar\\AppBarForKeyboarding.xaml";
        rootPage = safe_cast<xaml_controls::Page^>(LoadXamlFileOnUIThread(xamlFile));
        loadedRegistration.Attach(rootPage, [&](){ loadedEvent->Set(); });

        Platform::String^ focusSequence = "";
        Platform::String^ expectedFocusSequence = "[AB1][AB2][AB3][TEB][B][AB4][AB5][AB6][BEB][AB1]";

        RunOnUIThread([&]()
        {
            button = safe_cast<xaml_controls::Button^>(TreeHelper::GetVisualChildByName(rootPage, L"ExternalButton"));

            auto gotFocusHandler = ref new xaml::RoutedEventHandler([&](Platform::Object^ sender, xaml::RoutedEventArgs^ args)
                {
                    focusSequence += "[" + safe_cast<xaml::FrameworkElement^>(args->OriginalSource)->Tag + "]";
                });
            pageGotFocusRegistration.Attach(rootPage, gotFocusHandler);
            topAppBarGotFocusRegistration.Attach(rootPage->TopAppBar, gotFocusHandler);
            bottomAppBarGotFocusRegistration.Attach(rootPage->BottomAppBar, gotFocusHandler);

            TestServices::WindowHelper->WindowContent = rootPage;
        });
        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            xaml_controls::Button^ topExpandButton = safe_cast<xaml_controls::Button^>(TreeHelper::GetVisualChildByName(rootPage->TopAppBar, L"ExpandButton"));
            xaml_controls::Button^ bottomExpandButton = safe_cast<xaml_controls::Button^>(TreeHelper::GetVisualChildByName(rootPage->BottomAppBar, L"ExpandButton"));

            topExpandButton->Tag = "TEB";
            bottomExpandButton->Tag = "BEB";
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            button->Focus(xaml::FocusState::Programmatic);
        });
        TestServices::WindowHelper->WaitForIdle();

        focusSequence = "";

        LOG_OUTPUT(L"Open both app bars, which should now enable us to tab into the top app bar that was previously minimized.");

        RunOnUIThread([&]()
        {
            rootPage->TopAppBar->IsOpen = true;
            rootPage->BottomAppBar->IsOpen = true;
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Tab nine times, which should move focus to the first AppBarButton in the TopAppBar again.");

        for (size_t i = 0; i < 9; ++i)
        {
            TestServices::KeyboardHelper->Tab();
            TestServices::WindowHelper->WaitForIdle();
        }

        LOG_OUTPUT(L"Expected focus sequence: %s", expectedFocusSequence->Data());
        LOG_OUTPUT(L"Actual focus sequence: %s", (focusSequence)->Data());
        VERIFY_ARE_EQUAL(expectedFocusSequence, focusSequence);
    }

    void AppBarIntegrationTests::ValidateWinBlueTabbingIsPreserved()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::Page^ rootPage = nullptr;
        xaml_controls::Button^ button = nullptr;

        auto loadedEvent = std::make_shared<Event>();

        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::Page, Loaded);
        auto pageGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Page, GotFocus);
        auto topAppBarGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::AppBar, GotFocus);
        auto bottomAppBarGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::AppBar, GotFocus);

        Platform::String^ xamlFile = GetPackageFolder() + L"resources\\native\\controls\\AppBar\\AppBarForKeyboarding.xaml";
        rootPage = safe_cast<xaml_controls::Page^>(LoadXamlFileOnUIThread(xamlFile));
        loadedRegistration.Attach(rootPage, [&](){ loadedEvent->Set(); });

        Platform::String^ focusSequence = "";
        Platform::String^ expectedFocusSequence = "[AB2][AB3][TEB][AB4][AB5][AB6][BEB][AB1]";

        RunOnUIThread([&]()
        {
            button = safe_cast<xaml_controls::Button^>(TreeHelper::GetVisualChildByName(rootPage, L"ExternalButton"));

            auto gotFocusHandler = ref new xaml::RoutedEventHandler([&](Platform::Object^ sender, xaml::RoutedEventArgs^ args)
                {
                    focusSequence += "[" + safe_cast<xaml::FrameworkElement^>(args->OriginalSource)->Tag + "]";
                });
            pageGotFocusRegistration.Attach(rootPage, gotFocusHandler);
            topAppBarGotFocusRegistration.Attach(rootPage->TopAppBar, gotFocusHandler);
            bottomAppBarGotFocusRegistration.Attach(rootPage->BottomAppBar, gotFocusHandler);

            TestServices::WindowHelper->WindowContent = rootPage;
        });
        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            xaml_controls::Button^ topExpandButton = safe_cast<xaml_controls::Button^>(TreeHelper::GetVisualChildByName(rootPage->TopAppBar, L"ExpandButton"));
            xaml_controls::Button^ bottomExpandButton = safe_cast<xaml_controls::Button^>(TreeHelper::GetVisualChildByName(rootPage->BottomAppBar, L"ExpandButton"));

            topExpandButton->Tag = "TEB";
            bottomExpandButton->Tag = "BEB";

            rootPage->TopAppBar->ClosedDisplayMode = xaml_controls::AppBarClosedDisplayMode::Hidden;
            rootPage->TopAppBar->IsSticky = false;
            rootPage->TopAppBar->IsOpen = true;
            rootPage->BottomAppBar->ClosedDisplayMode = xaml_controls::AppBarClosedDisplayMode::Hidden;
            rootPage->BottomAppBar->IsSticky = false;
            rootPage->BottomAppBar->IsOpen = true;
        });
        TestServices::WindowHelper->WaitForIdle();

        focusSequence = "";

        LOG_OUTPUT(L"Tab eight times, which should move focus through the AppBars without ever giving the external button focus.");

        for (size_t i = 0; i < 8; ++i)
        {
            TestServices::KeyboardHelper->Tab();
            TestServices::WindowHelper->WaitForIdle();
        }

        LOG_OUTPUT(L"Expected focus sequence: %s", expectedFocusSequence->Data());
        LOG_OUTPUT(L"Actual focus sequence: %s", (focusSequence)->Data());
        VERIFY_ARE_EQUAL(expectedFocusSequence, focusSequence);
    }

    void AppBarIntegrationTests::CanCloseAppBarUsingGamepadB()
    {
        CanCloseAppBarUsingDevice(InputDevice::Gamepad);
    }

    void AppBarIntegrationTests::CanCloseAppBarUsingEsc()
    {
        CanCloseAppBarUsingDevice(InputDevice::Keyboard);
    }

    void AppBarIntegrationTests::CanCloseAppBarUsingDevice(InputDevice device)
    {
        TestCleanupWrapper cleanup;

        auto page = SetupTopBottomInlineAppBarsPage();

        CanCloseAppBarHelper([&](bool expectedHandledValue, xaml_controls::AppBar^ appbar)
        {
            // We want to make sure the the key press gets handled/not handled as expected.
            // We cannot listen to the Page.KeyDown event, because Page.TopAppBar/Page.BottomAppBar is not
            // actually a visual child of the Page (they are hosted in a Popup), so events don't route
            // from the AppBar to the Page.
            // So, we listen to the KeyDown event on the parent of the appbar.

            xaml::UIElement^ parent;
            RunOnUIThread([&]()
            {
                parent = safe_cast<xaml::UIElement^>(appbar->Parent);
            });

            auto pageKeyDownEvent = std::make_shared<Event>();
            auto pageKeyDownRegistration = CreateSafeEventRegistrationForHandledEvents(xaml::UIElement, KeyDownEvent);
            pageKeyDownRegistration.Attach(parent, ref new xaml_input::KeyEventHandler([&](Platform::Object^ sender, xaml_input::KeyRoutedEventArgs^ e)
            {
                VERIFY_ARE_EQUAL(e->Handled, expectedHandledValue);
                pageKeyDownEvent->Set();
            }));

            CommonInputHelper::Cancel(device);
            pageKeyDownEvent->WaitForDefault();
        },
        page);
    }

    void AppBarIntegrationTests::CanCloseAppBarHelper(std::function<void(bool, xaml_controls::AppBar^)> closeFunction, xaml_controls::Page^ page)
    {
        xaml_controls::AppBar^ topAppBar = nullptr;
        xaml_controls::AppBar^ bottomAppBar = nullptr;
        xaml_controls::AppBar^ inlineAppBar = nullptr;
        xaml_controls::Button^ expandButton = nullptr;

        RunOnUIThread([&]()
        {
            topAppBar = page->TopAppBar;
            bottomAppBar = page->BottomAppBar;
            inlineAppBar = safe_cast<xaml_controls::AppBar^>(safe_cast<xaml_controls::Panel^>(page->Content)->FindName(L"inlineAppBar"));
        });
        TestServices::WindowHelper->WaitForIdle();

        auto topOpenedEvent = std::make_shared<Event>();
        auto topClosedEvent = std::make_shared<Event>();
        auto bottomOpenedEvent = std::make_shared<Event>();
        auto bottomClosedEvent = std::make_shared<Event>();
        auto inlineOpenedEvent = std::make_shared<Event>();
        auto inlineClosedEvent = std::make_shared<Event>();

        auto topOpenedRegistration = CreateSafeEventRegistration(xaml_controls::AppBar, Opened);
        auto topClosedRegistration = CreateSafeEventRegistration(xaml_controls::AppBar, Closed);
        auto bottomOpenedRegistration = CreateSafeEventRegistration(xaml_controls::AppBar, Opened);
        auto bottomClosedRegistration = CreateSafeEventRegistration(xaml_controls::AppBar, Closed);
        auto inlineOpenedRegistration = CreateSafeEventRegistration(xaml_controls::AppBar, Opened);
        auto inlineClosedRegistration = CreateSafeEventRegistration(xaml_controls::AppBar, Closed);

        AttachOpenedAndClosedHandlers(topAppBar, topOpenedEvent, topOpenedRegistration, topClosedEvent, topClosedRegistration);
        AttachOpenedAndClosedHandlers(bottomAppBar, bottomOpenedEvent, bottomOpenedRegistration, bottomClosedEvent, bottomClosedRegistration);
        AttachOpenedAndClosedHandlers(inlineAppBar, inlineOpenedEvent, inlineOpenedRegistration, inlineClosedEvent, inlineClosedRegistration);

        RunOnUIThread([&]()
        {
            topAppBar->IsOpen = true;
            bottomAppBar->IsOpen = true;
        });
        topOpenedEvent->WaitForDefault();
        bottomOpenedEvent->WaitForDefault();

        LOG_OUTPUT(L"Close both Top and Bottom AppBars using the 'close function'.");
        closeFunction(true, topAppBar);
        topClosedEvent->WaitForDefault();
        bottomClosedEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            inlineAppBar->IsOpen = true;
        });
        inlineOpenedEvent->WaitForDefault();

        LOG_OUTPUT(L"Close the inline AppBar using the 'close function'.");
        closeFunction(true, inlineAppBar);
        inlineClosedEvent->WaitForDefault();

        LOG_OUTPUT(L"After closing AppBars, further calls to 'close function', while focus is on inline AppBar, should not get handled.");
        closeFunction(false, inlineAppBar);

        // Move focus to top AppBar.
        RunOnUIThread([&]()
        {
            expandButton = safe_cast<xaml_controls::Button^>(TreeHelper::GetVisualChildByName(page->TopAppBar, L"ExpandButton"));
            expandButton->Focus(xaml::FocusState::Keyboard);
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(expandButton));
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"After closing AppBars, further calls to 'close function', while focus is on top AppBar, should not get handled.");
        closeFunction(false, topAppBar);
    }

    xaml_controls::Page^ AppBarIntegrationTests::SetupClosedDisplayModeTestEnvironment(bool setClosedDisplayModeValues)
    {
        xaml_controls::Page^ page = nullptr;

        // Setup our environment.
        RunOnUIThread([&]()
        {
            page = TestServices::WindowHelper->SetupSimulatedAppPage();

            page->TopAppBar = ref new xaml_controls::AppBar();

            if (setClosedDisplayModeValues)
            {
                page->TopAppBar->ClosedDisplayMode = xaml_controls::AppBarClosedDisplayMode::Minimal;
            }

            auto topStackPanel = ref new xaml_controls::StackPanel();
            topStackPanel->Orientation = xaml_controls::Orientation::Horizontal;

            auto topButton1 = ref new xaml_controls::AppBarButton();
            topButton1->Label = "First button";
            topStackPanel->Children->Append(topButton1);

            auto topButton2 = ref new xaml_controls::AppBarButton();
            topButton2->Label = "Second button";
            topStackPanel->Children->Append(topButton2);

            page->BottomAppBar = ref new xaml_controls::AppBar();

            if (setClosedDisplayModeValues)
            {
                page->BottomAppBar->ClosedDisplayMode = xaml_controls::AppBarClosedDisplayMode::Compact;
            }

            auto bottomStackPanel = ref new xaml_controls::StackPanel();
            bottomStackPanel->Orientation = xaml_controls::Orientation::Horizontal;

            auto bottomButton1 = ref new xaml_controls::AppBarButton();
            bottomButton1->Label = "First button";
            bottomStackPanel->Children->Append(bottomButton1);

            auto bottomButton2 = ref new xaml_controls::AppBarButton();
            bottomButton2->Label = "Second button";
            bottomStackPanel->Children->Append(bottomButton2);

            page->TopAppBar->Content = topStackPanel;
            page->BottomAppBar->Content = bottomStackPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        return page;
    }

    xaml_controls::Page^ AppBarIntegrationTests::SetupTopBottomInlineAppBarsPage()
    {
        xaml_controls::Page^ page = nullptr;

        RunOnUIThread([&]()
        {
            auto topAppBar = ref new xaml_controls::AppBar();
            auto bottomAppBar = ref new xaml_controls::AppBar();

            auto panel = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                LR"(<StackPanel xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml">
                        <AppBar x:Name="inlineAppBar" VerticalAlignment="Center">
                            <Rectangle Width="100" Height="60" HorizontalAlignment="Left" VerticalAlignment="Top" Fill="Orange"/>
                        </AppBar>
                    </StackPanel>)"));

            page = TestServices::WindowHelper->SetupSimulatedAppPage();
            page->TopAppBar = topAppBar;
            page->BottomAppBar = bottomAppBar;
            page->Content = panel;
        });
        TestServices::WindowHelper->WaitForIdle();

        return page;
    }

    xaml_controls::Page^ AppBarIntegrationTests::SetupFocusShiftTestPage()
    {
        xaml_controls::Page^ page = nullptr;

        RunOnUIThread([&]()
        {
            auto pageButton = ref new xaml_controls::Button();
            pageButton->Content = L"Add CommandBar";
            pageButton->Name = L"PageButton";
            pageButton->Tag = L"PB";

            page = TestServices::WindowHelper->SetupSimulatedAppPage();
            page->Content = pageButton;
        });
        TestServices::WindowHelper->WaitForIdle();

        return page;
    }

    xaml_controls::AppBar^ AppBarIntegrationTests::CreateFocusShiftTestAppBar(bool isOpen)
    {
        xaml_controls::AppBar^ appBar = nullptr;

        RunOnUIThread([&]()
        {
            appBar = ref new xaml_controls::AppBar();
            appBar->IsOpen = isOpen;
            // Use a Compact AppBar so the AppBarButton is visible and focusable even when the AppBar is closed.
            appBar->ClosedDisplayMode = xaml_controls::AppBarClosedDisplayMode::Compact;

            auto appBarButton = ref new xaml_controls::AppBarButton();
            appBarButton->Label = L"AppBarButton";
            appBarButton->Name = L"AppBarButton";
            appBarButton->Tag = L"ABB";
            appBar->Content = appBarButton;
        });
        TestServices::WindowHelper->WaitForIdle();

        return appBar;
    }

    void AppBarIntegrationTests::ValidateExpandButtonVisualInDisabledState()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::AppBar^ appBar = nullptr;
        xaml_controls::Page^ page = nullptr;
        xaml_controls::FontIcon^ ellipsisIcon = nullptr; //Ellipsis FontIcon in the ExpandButton.
        xaml_media::Brush^ expectedBrushEnabled = nullptr; //The expected Brush used for the Fill of the ellipsis when enabled
        xaml_media::Brush^ expectedBrushDisabled = nullptr; //The expected Brush used for the Fill of the ellipsis when disabled.

        RunOnUIThread([&]()
        {
            appBar = ref new xaml_controls::AppBar();
            page = TestServices::WindowHelper->SetupSimulatedAppPage();
            page->BottomAppBar = appBar;

            expectedBrushEnabled = safe_cast<xaml_media::Brush^>(xaml::Application::Current->Resources->Lookup(L"SystemControlForegroundBaseHighBrush"));
            expectedBrushDisabled = safe_cast<xaml_media::Brush^>(xaml::Application::Current->Resources->Lookup(L"SystemControlDisabledBaseMediumLowBrush"));
            VERIFY_IS_NOT_NULL(expectedBrushEnabled);
            VERIFY_IS_NOT_NULL(expectedBrushDisabled);
        });
        TestServices::WindowHelper->WaitForIdle();

        //Verify that the ellipsis is the correct color in the Enabled AppBar:
        RunOnUIThread([&]()
        {
            ellipsisIcon = safe_cast<xaml_controls::FontIcon^>(TreeHelper::GetVisualChildByName(appBar, L"EllipsisIcon"));

            VERIFY_ARE_EQUAL(expectedBrushEnabled, ellipsisIcon->Foreground);

            appBar->IsEnabled = false;
        });
        TestServices::WindowHelper->WaitForIdle();

        //Verify that the ellipsis is the correct color in the Disabled AppBar:
        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(expectedBrushDisabled, ellipsisIcon->Foreground);

            page->BottomAppBar = nullptr;
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    void AppBarIntegrationTests::CanNotTabIntoWhenClosedAndHidden()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Button^ button = nullptr;

        auto appBarGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::AppBar, GotFocus);
        bool didAppBarGetFocus = false;

        RunOnUIThread([&]()
        {
            auto appBar = ref new xaml_controls::AppBar();
            appBar->ClosedDisplayMode = xaml_controls::AppBarClosedDisplayMode::Hidden;

            // Add some content to make sure it doesn't get focus either.
            appBar->Content = ref new xaml_controls::AppBarButton();

            appBarGotFocusRegistration.Attach(appBar, [&](){ didAppBarGetFocus = true; });

            button = ref new xaml_controls::Button();
            button->Content = L"button";

            auto root = ref new xaml_controls::Grid();
            root->Children->Append(button);
            root->Children->Append(appBar);

            TestServices::WindowHelper->WindowContent = root;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            button->Focus(xaml::FocusState::Programmatic);
        });
        TestServices::WindowHelper->WaitForIdle();

        TestServices::KeyboardHelper->Tab();
        TestServices::WindowHelper->WaitForIdle();

        VERIFY_IS_FALSE(didAppBarGetFocus);
    }

    void AppBarIntegrationTests::AttachOpenedAndClosedHandlers(
        xaml_controls::AppBar^& appbar,
        std::shared_ptr<Microsoft::UI::Xaml::Tests::Common::Event>& openedEvent,
        SafeEventRegistrationType(xaml_controls::AppBar, Opened)& openedRegistration,
        std::shared_ptr<Microsoft::UI::Xaml::Tests::Common::Event>& closedEvent,
        SafeEventRegistrationType(xaml_controls::AppBar, Closed)& closedRegistration)
    {
        openedRegistration.Attach(appbar, [&](){ openedEvent->Set(); });
        closedRegistration.Attach(appbar, [&](){ closedEvent->Set(); });
    }

    void AppBarIntegrationTests::ValidateFootprint()
    {
        TestCleanupWrapper cleanup;

        double const expectedAppBarWidth = 500;

        double const expectedAppBarCompactClosedHeight = 48;
        double const expectedAppBarCompactOpenHeight = 48;

        double const expectedAppBarMinimalClosedHeight = 24;
        double const expectedAppBarMinimalOpenHeight = 24;

        double const expectedAppBarHiddenClosedHeight = 0;
        double const expectedAppBarHiddenOpenHeight = 0;

        xaml_controls::AppBar^ appBarCompactClosed = nullptr;
        xaml_controls::AppBar^ appBarCompactOpen = nullptr;
        xaml_controls::AppBar^ appBarMinimalClosed = nullptr;
        xaml_controls::AppBar^ appBarMinimalOpen = nullptr;
        xaml_controls::AppBar^ appBarHiddenClosed = nullptr;
        xaml_controls::AppBar^ appBarHiddenOpen = nullptr;

        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(500, 600));

        RunOnUIThread([&]()
        {
            auto rootPanel = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                LR"(<StackPanel xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml">
                        <AppBar x:Name="appBarCompactClosed" IsOpen="False" ClosedDisplayMode="Compact"/>
                        <AppBar x:Name="appBarCompactOpen" IsOpen="True" ClosedDisplayMode="Compact"/>
                        <AppBar x:Name="appBarMinimalClosed" IsOpen="False" ClosedDisplayMode="Minimal"/>
                        <AppBar x:Name="appBarMinimalOpen" IsOpen="True" ClosedDisplayMode="Minimal"/>
                        <AppBar x:Name="appBarHiddenClosed" IsOpen="False" ClosedDisplayMode="Hidden"/>
                        <AppBar x:Name="appBarHiddenOpen" IsOpen="True" ClosedDisplayMode="Hidden"/>
                    </StackPanel>)"));

            appBarCompactClosed = safe_cast<xaml_controls::AppBar^>(rootPanel->FindName(L"appBarCompactClosed"));
            appBarCompactOpen = safe_cast<xaml_controls::AppBar^>(rootPanel->FindName(L"appBarCompactOpen"));
            appBarMinimalClosed = safe_cast<xaml_controls::AppBar^>(rootPanel->FindName(L"appBarMinimalClosed"));
            appBarMinimalOpen = safe_cast<xaml_controls::AppBar^>(rootPanel->FindName(L"appBarMinimalOpen"));
            appBarHiddenClosed = safe_cast<xaml_controls::AppBar^>(rootPanel->FindName(L"appBarHiddenClosed"));
            appBarHiddenOpen = safe_cast<xaml_controls::AppBar^>(rootPanel->FindName(L"appBarHiddenOpen"));

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(expectedAppBarWidth, appBarCompactClosed->ActualWidth);
            VERIFY_ARE_EQUAL(expectedAppBarCompactClosedHeight, appBarCompactClosed->ActualHeight);

            VERIFY_ARE_EQUAL(expectedAppBarWidth, appBarCompactOpen->ActualWidth);
            VERIFY_ARE_EQUAL(expectedAppBarCompactOpenHeight, appBarCompactOpen->ActualHeight);

            VERIFY_ARE_EQUAL(expectedAppBarWidth, appBarMinimalClosed->ActualWidth);
            VERIFY_ARE_EQUAL(expectedAppBarMinimalClosedHeight, appBarMinimalClosed->ActualHeight);

            VERIFY_ARE_EQUAL(expectedAppBarWidth, appBarMinimalOpen->ActualWidth);
            VERIFY_ARE_EQUAL(expectedAppBarMinimalOpenHeight, appBarMinimalOpen->ActualHeight);

            VERIFY_ARE_EQUAL(expectedAppBarWidth, appBarHiddenClosed->ActualWidth);
            VERIFY_ARE_EQUAL(expectedAppBarHiddenClosedHeight, appBarHiddenClosed->ActualHeight);

            VERIFY_ARE_EQUAL(expectedAppBarWidth, appBarHiddenOpen->ActualWidth);
            VERIFY_ARE_EQUAL(expectedAppBarHiddenOpenHeight, appBarHiddenOpen->ActualHeight);
        });
    }

    void AppBarIntegrationTests::ValidateLightDismissOverlayMode()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::AppBar^ appBar = nullptr;

        RunOnUIThread([&]()
        {
            appBar = ref new xaml_controls::AppBar();
            appBar->IsOpen = true;

            TestServices::WindowHelper->WindowContent = appBar;
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Validate that the default is Auto and the AppBar's overlay is not visible (or visible if on Xbox)");
        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(appBar->LightDismissOverlayMode, xaml_controls::LightDismissOverlayMode::Auto);
            ValidateVisibilityOfOverlayElement(appBar, TestServices::Utilities->IsXBox);
        });

        LOG_OUTPUT(L"Validate that when set to On the AppBar's overlay is visible.");
        RunOnUIThread([&]()
        {
            appBar->LightDismissOverlayMode = xaml_controls::LightDismissOverlayMode::On;
            ValidateVisibilityOfOverlayElement(appBar, true);
        });

        LOG_OUTPUT(L"Validate that when set to Off the AppBar's overlay is not visible.");
        RunOnUIThread([&]()
        {
            appBar->LightDismissOverlayMode = xaml_controls::LightDismissOverlayMode::Off;
            ValidateVisibilityOfOverlayElement(appBar, false);
        });
    }

    void AppBarIntegrationTests::ValidateLightDismissOverlayModeForTopBottomAppBars()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Page^ page = nullptr;

        RunOnUIThread([&]()
        {
            auto topAppBar = ref new xaml_controls::AppBar();
            auto bottomAppBar = ref new xaml_controls::AppBar();

            page = TestServices::WindowHelper->SetupSimulatedAppPage();
            page->TopAppBar = topAppBar;
            page->BottomAppBar = bottomAppBar;

            TestServices::WindowHelper->WindowContent = page;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // Make sure we don't see the "More options" tooltip, which fails some
            // assertions in the test where only 1 popup should be open at a time.
            auto topExpandButton = safe_cast<xaml_controls::Button^>(TreeHelper::GetVisualChildByName(page->TopAppBar, L"ExpandButton"));
            xaml_controls::ToolTipService::SetToolTip(topExpandButton, nullptr);
        });

        // Top and Bottom AppBars are hosted in the ApplicationBarService which
        // uses a popup to display them above all other content.  Overlays
        // for top and bottom AppBars end up configuring this popup's overlay
        // so this test is just going to check that that is configured as
        // expected.
        // Since there is one overlay shared between the 2 app bars, the
        // expected mode represents the most-visible mode after examining
        // the setting on both.
        for (size_t topMode = 0; topMode < 3; ++topMode)
        {
            for (size_t bottomMode = 0; bottomMode < 3; ++bottomMode)
            {
                auto expectedMode = xaml_controls::LightDismissOverlayMode::Off;

                auto topOverlayMode = static_cast<xaml_controls::LightDismissOverlayMode>(topMode);
                auto bottomOverlayMode = static_cast<xaml_controls::LightDismissOverlayMode>(bottomMode);

                // Determine the expected mode.
                if (topOverlayMode == xaml_controls::LightDismissOverlayMode::On ||
                    bottomOverlayMode == xaml_controls::LightDismissOverlayMode::On)
                {
                    expectedMode = xaml_controls::LightDismissOverlayMode::On;
                }
                else if (topOverlayMode == xaml_controls::LightDismissOverlayMode::Auto ||
                         bottomOverlayMode == xaml_controls::LightDismissOverlayMode::Auto)
                {
                    expectedMode = TestServices::Utilities->IsXBox ? xaml_controls::LightDismissOverlayMode::On : xaml_controls::LightDismissOverlayMode::Off;
                }

                RunOnUIThread([&]()
                {
                    page->TopAppBar->LightDismissOverlayMode = static_cast<xaml_controls::LightDismissOverlayMode>(topMode);
                    page->TopAppBar->IsOpen = true;

                    page->BottomAppBar->LightDismissOverlayMode = static_cast<xaml_controls::LightDismissOverlayMode>(bottomMode);
                    page->BottomAppBar->IsOpen = true;
                });
                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    auto popups = xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(page->XamlRoot);
                    WEX::Common::Throw::IfFalse(popups->Size == 1, E_FAIL, L"Expected exactly one open Popup.");

                    auto appBarServicePopup = popups->GetAt(0);
                    VERIFY_ARE_EQUAL(appBarServicePopup->LightDismissOverlayMode, expectedMode);

                    if (expectedMode == xaml_controls::LightDismissOverlayMode::On)
                    {
                        auto overlayElement = TestServices::Utilities->GetPopupOverlayElement(appBarServicePopup);
                        VERIFY_IS_NOT_NULL(overlayElement);
                    }
                });

                // The overlay mode only gets refreshed when you toggle the app bars open, so close it
                // in preparation for the next iteration.
                RunOnUIThread([&]()
                {
                    page->TopAppBar->IsOpen = false;
                    page->BottomAppBar->IsOpen = false;
                });
                TestServices::WindowHelper->WaitForIdle();
            }
        }
    }

    void AppBarIntegrationTests::IsAutoLightDismissOverlayModeVisibleOnXbox()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::AppBar^ appBar = nullptr;

        RunOnUIThread([&]()
        {
            appBar = ref new xaml_controls::AppBar();
            appBar->LightDismissOverlayMode = xaml_controls::LightDismissOverlayMode::Auto;
            appBar->IsOpen = true;

            TestServices::WindowHelper->WindowContent = appBar;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            ValidateVisibilityOfOverlayElement(appBar, true);
        });
    }

    void AppBarIntegrationTests::IsAutoLightDismissOverlayModeVisibleForTopBottomAppBarsOnXbox()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Page^ page = nullptr;

        RunOnUIThread([&]()
        {
            auto topAppBar = ref new xaml_controls::AppBar();
            topAppBar->LightDismissOverlayMode = xaml_controls::LightDismissOverlayMode::Auto;
            topAppBar->IsOpen = true;

            auto bottomAppBar = ref new xaml_controls::AppBar();
            bottomAppBar->LightDismissOverlayMode = xaml_controls::LightDismissOverlayMode::Auto;
            topAppBar->IsOpen = true;

            page = TestServices::WindowHelper->SetupSimulatedAppPage();
            page->TopAppBar = topAppBar;
            page->BottomAppBar = bottomAppBar;

            TestServices::WindowHelper->WindowContent = page;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto popups = xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(page->XamlRoot);
            WEX::Common::Throw::IfFalse(popups->Size == 1, E_FAIL, L"Expected exactly one open Popup.");

            auto appBarServicePopup = popups->GetAt(0);
            VERIFY_ARE_EQUAL(appBarServicePopup->LightDismissOverlayMode, xaml_controls::LightDismissOverlayMode::On);

            page->TopAppBar->IsOpen = false;
            page->BottomAppBar->IsOpen = false;
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    void AppBarIntegrationTests::ValidateOverlayDCompTree()
    {
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 400));
        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

        // Set the content first before injecting the touch input
        xaml_controls::Grid^ grid = nullptr;
        RunOnUIThread([&]()
        {
            grid = ref new xaml_controls::Grid();
            TestServices::WindowHelper->WindowContent = grid;
        });
        TestServices::WindowHelper->WaitForIdle();

        // Inject the touch input not to set the focus with keyboard state
        TestServices::InputHelper->Tap(grid);
        TestServices::WindowHelper->WaitForIdle();

        auto root = SetupOverlayTreeValidationTest();

        LOG_OUTPUT(L"Validate the dark theme of the overlay.");
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "Dark");

        LOG_OUTPUT(L"Validate the light theme of the overlay.");
        RunOnUIThread([&]()
        {
            root->RequestedTheme = xaml::ElementTheme::Light;
        });
        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "Light");

        LOG_OUTPUT(L"Validate the high-contrast theme of the overlay.");
        RunOnUIThread([&]()
        {
            TestServices::ThemingHelper->HighContrastTheme = HighContrastTheme::Test;
        });
        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, "HC");
    }

    void AppBarIntegrationTests::ValidateOverlayUIETree()
    {
        TestCleanupWrapper cleanup;

        ControlHelper::ValidateUIElementTree(
            wf::Size(400, 400),
            1.f,
            []()
            {
                return SetupOverlayTreeValidationTest();
            }
        );
    }

    xaml_controls::Panel^ AppBarIntegrationTests::SetupOverlayTreeValidationTest()
    {
        xaml_controls::Grid^ root = nullptr;
        xaml_controls::Button^ button = nullptr;

        RunOnUIThread([&]()
        {
            root = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
                LR"(<Grid xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"
                            Background="{ThemeResource SystemControlBackgroundAltHighBrush}" >
                            <AppBar IsOpen="True" LightDismissOverlayMode="On">
                                <AppBarButton x:Name="button"/>
                            </AppBar>
                        </Grid>)"));

            button = safe_cast<xaml_controls::Button^>(root->FindName(L"button"));

            TestServices::WindowHelper->WindowContent = root;
        });
        TestServices::WindowHelper->WaitForIdle();

        // Set focus to the button to prevent focus from defaulting ot the expand button, which
        // can cause the tooltip for "More options" to show up and destabilize these tree validations.
        RunOnUIThread([&]()
        {
            button->Focus(xaml::FocusState::Programmatic);
        });

        return root;
    }

    void AppBarIntegrationTests::ValidateOverlayBrush()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::AppBar^ appBar = nullptr;

        RunOnUIThread([&]()
        {
            appBar = ref new xaml_controls::AppBar();
            appBar->LightDismissOverlayMode = xaml_controls::LightDismissOverlayMode::On;
            appBar->IsOpen = true;

            TestServices::WindowHelper->WindowContent = appBar;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto expectedBrush = safe_cast<xaml_media::SolidColorBrush^>(xaml::Application::Current->Resources->Lookup(L"AppBarLightDismissOverlayBackground"));

            auto overlayElement = GetAppBarOverlayElement(appBar);
            THROW_IF_NULL_WITH_MSG(overlayElement, L"An overlay element should exist.");

            auto overlayRect = dynamic_cast<xaml_shapes::Rectangle^>(overlayElement);
            THROW_IF_NULL_WITH_MSG(overlayRect, L"The overlay element should be a rectangle.");

            auto overlayBrush = safe_cast<xaml_media::SolidColorBrush^>(overlayRect->Fill);
            VERIFY_IS_NOT_NULL(overlayBrush);
            VERIFY_IS_TRUE(overlayBrush->Equals(expectedBrush));
        });
    }

    void AppBarIntegrationTests::ValidateOverlayBrushForTopBottomAppBars()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Page^ page = nullptr;

        RunOnUIThread([&]()
        {
            auto topAppBar = ref new xaml_controls::AppBar();
            topAppBar->LightDismissOverlayMode = xaml_controls::LightDismissOverlayMode::On;
            topAppBar->IsOpen = true;

            page = TestServices::WindowHelper->SetupSimulatedAppPage();
            page->TopAppBar = topAppBar;

            TestServices::WindowHelper->WindowContent = page;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto expectedBrush = safe_cast<xaml_media::SolidColorBrush^>(xaml::Application::Current->Resources->Lookup(L"AppBarLightDismissOverlayBackground"));

            auto popups = xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(page->XamlRoot);
            WEX::Common::Throw::IfFalse(popups->Size == 1, E_FAIL, L"Expected exactly one open Popup.");

            auto appBarServicePopup = popups->GetAt(0);
            auto overlayElement = TestServices::Utilities->GetPopupOverlayElement(appBarServicePopup);
            THROW_IF_NULL_WITH_MSG(overlayElement, L"An overlay element should exist.");

            auto overlayRect = dynamic_cast<xaml_shapes::Rectangle^>(overlayElement);
            THROW_IF_NULL_WITH_MSG(overlayRect, L"The overlay element should be a rectangle.");

            auto overlayBrush = safe_cast<xaml_media::SolidColorBrush^>(overlayRect->Fill);
            VERIFY_IS_NOT_NULL(overlayBrush);
            VERIFY_IS_TRUE(overlayBrush->Equals(expectedBrush));
        });
    }

    xaml::FrameworkElement^ AppBarIntegrationTests::GetAppBarOverlayElement(xaml_controls::AppBar^ appBar)
    {
        WEX::Common::Throw::IfFalse(appBar->IsOpen, E_FAIL, L"AppBar should be opened before calling this helper.");

        // Get the layoutRoot element of the app bar.
        auto layoutRoot = safe_cast<xaml_controls::Grid^>(xaml_media::VisualTreeHelper::GetChild(appBar, 0));

        // When open, the overlay element should be the first child under the layout root.
        return safe_cast<xaml::FrameworkElement^>(layoutRoot->Children->GetAt(0));
    }

    void AppBarIntegrationTests::ValidateVisibilityOfOverlayElement(xaml_controls::AppBar^ appBar, bool expectedIsVisible)
    {
        auto overlayElement = GetAppBarOverlayElement(appBar);
        THROW_IF_NULL_WITH_MSG(overlayElement, L"An overlay element should exist.");

        auto overlayRect = dynamic_cast<xaml_shapes::Rectangle^>(overlayElement);
        THROW_IF_NULL_WITH_MSG(overlayRect, L"The overlay element should be a rectangle.");

        auto overlayBrush = safe_cast<xaml_media::SolidColorBrush^>(overlayRect->Fill);
        THROW_IF_NULL_WITH_MSG(overlayBrush, L"The overlay element should have a brush.");

        auto brushColor = overlayBrush->Color;
        if (expectedIsVisible)
        {
            VERIFY_IS_GREATER_THAN(brushColor.A, 0);
        }
        else
        {
            VERIFY_ARE_EQUAL(brushColor.A, 0);
        }
    }

    void AppBarIntegrationTests::VerifyAutomationPeerIsOffscreenIsFalseForElementInOpenAppBar()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::AppBar^ appBar;
        xaml_controls::AppBarButton^ appBarButton;

        RunOnUIThread([&]()
        {
            auto root = safe_cast<xaml::FrameworkElement^>(xaml_markup::XamlReader::Load(
                LR"(<Grid xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"
                        Background="LightBlue" Width="400" Height="400">
                        <StackPanel>
                            <AppBar x:Name="appBar">
                                <AppBarButton x:Name="appBarButton" Icon="Add" Label="Add"/>
                            </AppBar>
                        </StackPanel>
                    </Grid>)"));

            appBar = safe_cast<xaml_controls::AppBar^>(root->FindName(L"appBar"));
            appBarButton = safe_cast<xaml_controls::AppBarButton^>(root->FindName(L"appBarButton"));

            TestServices::WindowHelper->WindowContent = root;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            appBar->IsOpen = true;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto automationPeer = xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(appBarButton);

            VERIFY_IS_FALSE(automationPeer->IsOffscreen());
        });

        RunOnUIThread([&]()
        {
            appBar->IsOpen = false;
        });
        TestServices::WindowHelper->WaitForIdle();
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Controls::AppBar
