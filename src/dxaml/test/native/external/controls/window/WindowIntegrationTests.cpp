// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "WindowIntegrationTests.h"

#include <XamlTailored.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>
#include <ControlHelper.h>
#include <WindowAutoCloser.h>
#include <microsoft.ui.xaml.window.h> // for IWindowNative

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace Window {

    bool WindowIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool WindowIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool WindowIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //
    void WindowIntegrationTests::CanHideAndShowWindow()
    {
        TestCleanupWrapper cleanup;

        auto visibilityChangedEvent = std::make_shared<Event>();
        auto visibilityChangedRegistration = CreateSafeEventRegistration(xaml::Window, VisibilityChanged);

        bool expectedVisibility = false;

        RunOnUIThread([&]()
        {
            visibilityChangedRegistration.Attach(xaml::Window::Current,
                ref new wf::TypedEventHandler<Platform::Object^, xaml::WindowVisibilityChangedEventArgs^>([&](Platform::Object^, Microsoft::UI::Xaml::WindowVisibilityChangedEventArgs^ e)
                {
                    LOG_OUTPUT(L"Visibility changed - window is now %s.", (e->Visible ? L"visible" : L"hidden"));
                    VERIFY_ARE_EQUAL(expectedVisibility, e->Visible);
                    visibilityChangedEvent->Set();
                }));

            LOG_OUTPUT(L"Hiding the window.");
            TestServices::WindowHelper->HideWindow();
        });

        visibilityChangedEvent->WaitForDefault();
        expectedVisibility = true;

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Showing the window.");
            TestServices::WindowHelper->ShowWindow();
        });

        visibilityChangedEvent->WaitForDefault();
    }

    void WindowIntegrationTests::CanGetSetTitle()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]()
        {
            Platform::String^ newTitle = ref new Platform::String(L"This is the new test title for UWP Window");
            Platform::String^ newTitle2 = ref new Platform::String(L"This is another new test title for UWP Window: 123456");

            LOG_OUTPUT(L"Getting the Window Title...");
            auto currentTitle = xaml::Window::Current->Title;
            LOG_OUTPUT(L"Retrieved Window Title (should be NULL): %s", currentTitle->Data());
            VERIFY_IS_TRUE(currentTitle->IsEmpty());

            LOG_OUTPUT(L"Setting the Window Title: %s", newTitle->Data());
            xaml::Window::Current->Title = newTitle; 

            LOG_OUTPUT(L"Getting the Window Title...");
            currentTitle = xaml::Window::Current->Title;
            LOG_OUTPUT(L"Retrieved Window Title: %s", currentTitle->Data());
            VERIFY_IS_TRUE(Platform::String::CompareOrdinal(currentTitle, newTitle) == 0);

            LOG_OUTPUT(L"Setting the Window Title: %s", newTitle2->Data());
            xaml::Window::Current->Title = newTitle2; 

            LOG_OUTPUT(L"Getting the Window Title...");
            currentTitle = xaml::Window::Current->Title;
            LOG_OUTPUT(L"Retrieved Window Title: %s", currentTitle->Data());
            VERIFY_IS_TRUE(Platform::String::CompareOrdinal(currentTitle, newTitle2) == 0);
        });
    }

    void WindowIntegrationTests::CanMoveWindow()
    {
        TestCleanupWrapper cleanup;

        wf::Rect expectedWindowBounds = {200, 200, 300, 400};

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Moving window to (%.2f, %.2f, %.2f, %.2f).",
                expectedWindowBounds.X,
                expectedWindowBounds.Y,
                expectedWindowBounds.Width,
                expectedWindowBounds.Height);

            TestServices::WindowHelper->MoveWindow((int)expectedWindowBounds.X, (int)expectedWindowBounds.Y, (int)expectedWindowBounds.Width, (int)expectedWindowBounds.Height);
            auto windowBounds = TestServices::WindowHelper->WindowBounds;

            LOG_OUTPUT(L"Expected window bounds: (%.2f, %.2f, %.2f, %.2f)",
                expectedWindowBounds.X,
                expectedWindowBounds.Y,
                expectedWindowBounds.Width,
                expectedWindowBounds.Height);

            LOG_OUTPUT(L"Actual window bounds:   (%.2f, %.2f, %.2f, %.2f)",
                windowBounds.X,
                windowBounds.Y,
                windowBounds.Width,
                windowBounds.Height);

            VERIFY_IS_TRUE(ControlHelper::AreClose(expectedWindowBounds.X, windowBounds.X, 2 /*tolerance*/));
            VERIFY_IS_TRUE(ControlHelper::AreClose(expectedWindowBounds.Y, windowBounds.Y, 2 /*tolerance*/));
            VERIFY_IS_TRUE(ControlHelper::AreClose(expectedWindowBounds.Width, windowBounds.Width, 2 /*tolerance*/));
            VERIFY_IS_TRUE(ControlHelper::AreClose(expectedWindowBounds.Height, windowBounds.Height, 2 /*tolerance*/));
        });
    }

    void WindowIntegrationTests::SetTitleInMarkup()
    {
        TestCleanupWrapper cleanup;

        WindowAutoCloser window1;

        RunOnUIThread([&]()
        {
            window1.Attach(safe_cast<xaml::Window^> (xaml_markup::XamlReader::Load(
                L"<Window xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                L"  Title='Test Window Title'>"
                L"  <StackPanel x:Name='rootPanel'/>"
                L"</Window>")));
            
            window1->Activate();
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"----- Check the Window.Title property -----");
            LOG_OUTPUT(L"Window->Title = %s", window1->Title->Data());
            VERIFY_IS_TRUE(Platform::String::CompareOrdinal(window1->Title, "Test Window Title") == 0);

            LOG_OUTPUT(L"----- Check the HWND's text -----");
            IWindowNative* windowNative = nullptr;
            auto hr = reinterpret_cast<IUnknown*>(window1.get())->QueryInterface(__uuidof(IWindowNative), (void**)&windowNative);
            VERIFY_SUCCEEDED(hr);

            HWND windowHandle = nullptr;
            hr = windowNative->get_WindowHandle(&windowHandle);
            VERIFY_SUCCEEDED(hr);

            windowNative->Release();
            windowNative = nullptr;

            wchar_t buffer[128] = {};
            GetWindowText(windowHandle, buffer, ARRAYSIZE(buffer));
            LOG_OUTPUT(L"GetWindowText = %s", buffer);
            VERIFY_IS_TRUE(Platform::String::CompareOrdinal(ref new Platform::String(buffer), "Test Window Title") == 0);
        });
    }

    void WindowIntegrationTests::SetSystemBackdropInMarkup()
    {
        TestCleanupWrapper cleanup;

        WindowAutoCloser window1;

        xaml_controls::StackPanel^ rootPanel;

        RunOnUIThread([&]()
        {
            window1.Attach(safe_cast<xaml::Window^> (xaml_markup::XamlReader::Load(
                L"<Window xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                L"  >"
                L"  <Window.SystemBackdrop>"
                L"    <DesktopAcrylicBackdrop/>"
                L"  </Window.SystemBackdrop>"
                L"  <StackPanel x:Name='rootPanel'/>"
                L"</Window>")));

            rootPanel = safe_cast<xaml_controls::StackPanel^>(window1->Content);
            
            window1->Activate();
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"----- Check the Window.SystemBackdrop property -----");
            auto systemBackdrop = window1->SystemBackdrop;
            VERIFY_IS_NOT_NULL(systemBackdrop);
            VERIFY_IS_NOT_NULL(safe_cast<xaml_media::DesktopAcrylicBackdrop^>(systemBackdrop)); // must be DesktopAcrylicBackdrop

            LOG_OUTPUT(L"----- Get the ICompositionSupportsSystemBackdrop for the window and verify if a brush is set -----");
            auto compositionSupportsSystemBackdrop = safe_cast<Microsoft::UI::Composition::ICompositionSupportsSystemBackdrop^>(window1.get());
            VERIFY_IS_NOT_NULL(compositionSupportsSystemBackdrop);

            auto systemBackdropBrush = compositionSupportsSystemBackdrop->SystemBackdrop;
            VERIFY_IS_NOT_NULL(systemBackdropBrush);
        });
    }


} } } } } } // Microsoft::UI::Xaml::Tests::Controls::Window
