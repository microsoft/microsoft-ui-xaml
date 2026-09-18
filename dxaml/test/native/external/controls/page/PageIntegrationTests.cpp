// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "PageIntegrationTests.h"

#include <generic\DependencyObjectTests.h>
#include <generic\FrameworkElementTests.h>

#include <XamlTailored.h>
#include <TestEvent.h>
#include <TestCleanupWrapper.h>
#include <ControlHelper.h>

#include "FileLoader.h"
#include "WUCRenderingScopeGuard.h"

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Media;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace Page {

    bool PageIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool PageIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool PageIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //
    void PageIntegrationTests::CanInstantiate()
    {
        Generic::DependencyObjectTests<xaml_controls::Page>::CanInstantiate();
    }

    void PageIntegrationTests::CanEnterAndLeaveLiveTree()
    {
        Generic::FrameworkElementTests<xaml_controls::Page>::CanEnterAndLeaveLiveTree();
    }

    void PageIntegrationTests::CanGetAndSetProperties()
    {
        TestCleanupWrapper cleanup;
        RunOnUIThread([&]()
        {
            auto page = ref new xaml_controls::Page();
            auto commandBar = ref new xaml_controls::CommandBar();

            //NavigationCacheMode
            VERIFY_IS_TRUE(page->NavigationCacheMode == xaml_navigation::NavigationCacheMode::Disabled);
            page->NavigationCacheMode = xaml_navigation::NavigationCacheMode::Required;
            VERIFY_IS_TRUE(page->NavigationCacheMode == xaml_navigation::NavigationCacheMode::Required);
            page->NavigationCacheMode = xaml_navigation::NavigationCacheMode::Enabled;
            VERIFY_IS_TRUE(page->NavigationCacheMode == xaml_navigation::NavigationCacheMode::Enabled);

            //BottomAppBar
            VERIFY_IS_NULL(page->BottomAppBar);
            page->BottomAppBar = commandBar;
            VERIFY_ARE_EQUAL(page->BottomAppBar, commandBar);
        });
    }

    void PageIntegrationTests::CanGetAndSetDesktopOnlyProperties()
    {
        RunOnUIThread([&]()
        {
            auto page = ref new xaml_controls::Page();
            auto appBar = ref new xaml_controls::AppBar();

            //TopAppBar
            VERIFY_IS_NULL(page->TopAppBar);
            page->TopAppBar = appBar;
            VERIFY_ARE_EQUAL(page->TopAppBar, appBar);
        });
    }

    void PageIntegrationTests::LayoutToLayoutBoundsTest()
    {
        TestCleanupWrapper cleanup;

        auto rootPage = safe_cast<xaml_controls::Page^>(LoadXamlFileOnUIThread(GetPackageFolder() + L"resources\\native\\controls\\Page\\PageLayoutToWindowBoundsTest.xaml"));
        VERIFY_IS_NOT_NULL(rootPage);

        auto loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::Page, Loaded);

        RunOnUIThread([&]()
        {
            loadedRegistration.Attach(rootPage,
                ref new xaml::RoutedEventHandler([loadedEvent](Platform::Object^ sender, xaml::RoutedEventArgs^ args)
            {
                loadedEvent->Set();
            }));

            TestServices::WindowHelper->WindowContent = rootPage;
        });

        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        ValidateRectanglePlacement(rootPage);
        ValidateToolTipPlacement(rootPage);
    }

    void PageIntegrationTests::LayoutUpdateTriggeredByCoreWindowResize()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Page^ page = nullptr;
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::Page, Loaded);
        auto layoutRegistration = CreateSafeEventRegistration(xaml_controls::Page, LayoutUpdated);
        auto loadedEvent = std::make_shared<Event>();
        auto layoutEvent = std::make_shared<Event>();

        RunOnUIThread([&]()
        {
            page = TestServices::WindowHelper->SetupSimulatedAppPage();
            loadedRegistration.Attach(page, [loadedEvent]() { loadedEvent->Set(); });

            layoutRegistration.Attach(page, ref new wf::EventHandler<Platform::Object^>([layoutEvent](Platform::Object^, Platform::Object^)
            {
                layoutEvent->Set();
            }));
        });

        loadedEvent->WaitForDefault();
        layoutEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        // We need to wait until the window is active before we can set its size. However, there's no way to
        // explicitly wait for that to happen. As such, we'll just sleep for a second to ensure that it has
        // enough time to become active.
        Sleep(1000);

        //Reduce window size
        TestServices::WindowHelper->SetDesktopWindowSize(400, 400);
        layoutEvent->WaitForDefault();

        //increase window size
        TestServices::WindowHelper->MaximizeDesktopWindow();
        layoutEvent->WaitForDefault();

        TestServices::WindowHelper->WaitForIdle();
    }

    void PageIntegrationTests::ValidatePageStackEntryCollectionBehavior()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Frame^ frame = nullptr;

        Microsoft::UI::Xaml::Navigation::PageStackEntry^ backwardStackEntry = nullptr;
        Microsoft::UI::Xaml::Navigation::PageStackEntry^ forwardStackEntry = nullptr;

        Microsoft::UI::Xaml::Media::Animation::CommonNavigationTransitionInfo^ commonNTI = nullptr;

        wxaml_interop::TypeName pageType = { L"Microsoft.UI.Xaml.Controls.Page", wxaml_interop::TypeKind::Primitive };

        RunOnUIThread([&]()
        {
            frame = ref new xaml_controls::Frame();
            TestServices::WindowHelper->WindowContent = frame;
        });

        TestServices::WindowHelper->WaitForIdle();

        //Navigate three times with a slide transition then go back one with a common transition, this will give us a PageStackEntry in the ForwardStack and the BackStack
        RunOnUIThread([&]()
        {
            frame->Navigate(pageType);
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            frame->Navigate(pageType);
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            frame->Navigate(pageType);
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            frame->GoBack(commonNTI);
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            //Verify both stacks are the right size.
            VERIFY_IS_TRUE(frame->BackStack->Size == 1);
            VERIFY_IS_TRUE(frame->ForwardStack->Size == 1);

            forwardStackEntry = frame->ForwardStack->GetAt(frame->ForwardStack->Size - 1);
            backwardStackEntry = frame->BackStack->GetAt(frame->BackStack->Size - 1);

            //Add onto backstack
            frame->BackStack->Append(backwardStackEntry);
            VERIFY_IS_TRUE(frame->BackStack->Size == 2);
            VERIFY_IS_TRUE(frame->BackStack->GetAt(1) == backwardStackEntry);

            //Change the new entry
            frame->BackStack->SetAt(1, forwardStackEntry);
            VERIFY_IS_TRUE(frame->BackStack->Size == 2);
            VERIFY_IS_TRUE(frame->BackStack->GetAt(1) == forwardStackEntry);

            //Insert between the two entries
            frame->BackStack->InsertAt(1, backwardStackEntry);
            VERIFY_IS_TRUE(frame->BackStack->Size == 3);
            VERIFY_IS_TRUE(frame->BackStack->GetAt(1) == backwardStackEntry);
            VERIFY_IS_TRUE(frame->BackStack->GetAt(2) == forwardStackEntry);

            //Remove from between the two entries
            frame->BackStack->RemoveAt(1);
            VERIFY_IS_TRUE(frame->BackStack->Size == 2);
            VERIFY_IS_TRUE(frame->BackStack->GetAt(1) == forwardStackEntry);

            //Remove from end of backstack
            frame->BackStack->RemoveAtEnd();
            VERIFY_IS_TRUE(frame->BackStack->Size == 1);

            //Append a few items
            frame->BackStack->Append(backwardStackEntry);
            frame->BackStack->Append(backwardStackEntry);
            frame->BackStack->Append(backwardStackEntry);
            VERIFY_IS_TRUE(frame->BackStack->Size == 4);

            //Clear the backstack
            frame->BackStack->Clear();
            VERIFY_IS_TRUE(frame->BackStack->Size == 0);
        });
    }

    void PageIntegrationTests::PageRendersBackgroundCommon()
    {
        const auto& windowHelper = TestServices::WindowHelper;
        const auto& utilities = TestServices::Utilities;

        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

        xaml_controls::Page^ page = nullptr;
        RunOnUIThread([&]()
        {
            page = ref new xaml_controls::Page();
            page->Background = ref new SolidColorBrush(Microsoft::UI::Colors::Red);

            windowHelper->WindowContent = page;
        });
        windowHelper->WaitForIdle();
        utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Empty");

        RunOnUIThread([&]()
        {
            page->Content = ref new xaml_controls::Grid();
        });
        windowHelper->WaitForIdle();
        utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Default");

        RunOnUIThread([&]()
        {
            page->Width = 100;
            page->Height = 100;
        });
        windowHelper->WaitForIdle();
        utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"Explicit");
    }

    void PageIntegrationTests::PageRendersBackground()
    {
        PageRendersBackgroundCommon();
    }

    // Rectangle is placed entirely by Page layout
    void PageIntegrationTests::ValidateRectanglePlacement(xaml_controls::Page^ rootPage)
    {
        xaml_shapes::Rectangle^ rectangle = nullptr;

        RunOnUIThread([&]()
        {
            rectangle = dynamic_cast<xaml_shapes::Rectangle^>(rootPage->FindName(L"Rectangle1"));
            VERIFY_IS_NOT_NULL(rectangle);
        });

        RunOnUIThread([&]()
        {
            // Validate Rectangle position
            //
            // A rectangle will always be a rectangle - even in future UX changes -so we can verify its position precisely
            wf::Rect rectangleBounds = ControlHelper::GetBounds(safe_cast<xaml::FrameworkElement^>(rectangle));
            LOG_OUTPUT(L"Rectangle1 bounds left=%f top=%f width=%f height=%f", rectangleBounds.Left, rectangleBounds.Top, rectangleBounds.Width, rectangleBounds.Height);

            // wf::Rect windowBounds = TestServices::WindowHelper->WindowBounds;

            // Account for 2px border size
            wf::Rect expectedBounds = wf::Rect{ 2 + TitleSafe_Left, 2 + TitleSafe_Top, 200, 100 };

            LOG_OUTPUT(L"Expected bounds left=%f top=%f width=%f height=%f", expectedBounds.Left, expectedBounds.Top, expectedBounds.Width, expectedBounds.Height);
            VERIFY_ARE_EQUAL(rectangleBounds, expectedBounds);
        });

        TestServices::WindowHelper->WaitForIdle();
    }

    // ToolTip, like most popup-based controls, should use LayoutBounds to calculate placement.
    void PageIntegrationTests::ValidateToolTipPlacement(xaml_controls::Page^ rootPage)
    {
        xaml_controls::ToolTip^ toolTip = nullptr;
        xaml_controls::Button^ button = nullptr;

        auto toolTipOpenedEvent = std::make_shared<Event>();
        auto toolTipOpenedRegistration = CreateSafeEventRegistration(xaml_controls::ToolTip, Opened);
        auto toolTipClosedEvent = std::make_shared<Event>();
        auto toolTipClosedRegistration = CreateSafeEventRegistration(xaml_controls::ToolTip, Closed);

        RunOnUIThread([&]()
        {
            button = dynamic_cast<xaml_controls::Button^>(rootPage->FindName(L"Button1"));
            VERIFY_IS_NOT_NULL(button);

            toolTip = dynamic_cast<xaml_controls::ToolTip^>(rootPage->FindName(L"ToolTip1"));
            VERIFY_IS_NOT_NULL(toolTip);

            toolTipOpenedRegistration.Attach(
                toolTip,
                ref new xaml::RoutedEventHandler([toolTipOpenedEvent](Platform::Object^, xaml::RoutedEventArgs^)
            {
                toolTipOpenedEvent->Set();
            }));

            toolTipClosedRegistration.Attach(
                toolTip,
                ref new xaml::RoutedEventHandler([toolTipClosedEvent](Platform::Object^, xaml::RoutedEventArgs^)
            {
                toolTipClosedEvent->Set();
            }));

            toolTip->IsOpen = true;
        });

        toolTipOpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // Validate ToolTip position
            wf::Rect toolTipBounds = ControlHelper::GetBounds(safe_cast<xaml::FrameworkElement^>(toolTip));
            LOG_OUTPUT(L"ToolTip1 bounds left=%f top=%f width=%f height=%f", toolTipBounds.Left, toolTipBounds.Top, toolTipBounds.Width, toolTipBounds.Height);

            wf::Rect buttonBounds = ControlHelper::GetBounds(safe_cast<xaml::FrameworkElement^>(button));
            LOG_OUTPUT(L"Button1 bounds left=%f top=%f width=%f height=%f", buttonBounds.Left, buttonBounds.Top, buttonBounds.Width, buttonBounds.Height);

            // Tooltip specifies Placement == Left, but the target is aligned to the left edge of the screen.
            // Regardless of Page.LayoutToWindowBounds setting, we expect that tooltip will be not be placed to the left, since there is no space there.
            // The case of LayoutToWindowBounds == false is more interesting, since tooltip can then fit into the overscan area to the left,
            // and we are verifying that the control logic uses the visible bounds to place the popup.
            VERIFY_IS_TRUE(toolTipBounds.Left > buttonBounds.Left);

            toolTip->IsOpen = false;
        });

        toolTipClosedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Controls::Page
