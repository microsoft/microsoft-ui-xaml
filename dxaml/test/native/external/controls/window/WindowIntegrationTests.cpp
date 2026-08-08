// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "WindowIntegrationTests.h"

#include <cmath>
#include <limits>
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

    void LogWindowGeometry(const wchar_t* label, HWND windowHandle, xaml::Window^ window)
    {
        RECT outerRect{};
        VERIFY_IS_TRUE(!!::GetWindowRect(windowHandle, &outerRect));

        RECT clientRect{};
        VERIFY_IS_TRUE(!!::GetClientRect(windowHandle, &clientRect));

        const UINT dpi = ::GetDpiForWindow(windowHandle);
        const double scale = static_cast<double>(dpi) / static_cast<double>(USER_DEFAULT_SCREEN_DPI);
        const double outerWidthInDips = static_cast<double>(outerRect.right - outerRect.left) / scale;
        const double outerHeightInDips = static_cast<double>(outerRect.bottom - outerRect.top) / scale;
        const double clientWidthInDips = static_cast<double>(clientRect.right - clientRect.left) / scale;
        const double clientHeightInDips = static_cast<double>(clientRect.bottom - clientRect.top) / scale;
#ifdef MUX_PRERELEASE
        const double windowWidth = window->Width;
        const double windowHeight = window->Height;
#else
        const double windowWidth = 0;
        const double windowHeight = 0;
#endif // MUX_PRERELEASE

        LOG_OUTPUT(
            L"%s: outerPx=(%ld,%ld)-(%ld,%ld) outerDips=%fx%f clientPx=%ldx%ld clientDips=%fx%f Window=%fx%f outerMinusWindow=%fx%f",
            label,
            outerRect.left,
            outerRect.top,
            outerRect.right,
            outerRect.bottom,
            outerWidthInDips,
            outerHeightInDips,
            clientRect.right - clientRect.left,
            clientRect.bottom - clientRect.top,
            clientWidthInDips,
            clientHeightInDips,
            windowWidth,
            windowHeight,
            outerWidthInDips - windowWidth,
            outerHeightInDips - windowHeight);
    }

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

#ifdef MUX_PRERELEASE
    void WindowIntegrationTests::CanGetSetWindowWidthHeight()
    {
        TestCleanupWrapper cleanup;

        WindowAutoCloser window1;

        // Try a couple of distinct sizes to make sure we are actually moving the values around
        // rather than just reading back a default that happens to match.
        const double firstWidth = 400.0;
        const double firstHeight = 300.0;
        const double secondWidth = 640.0;
        const double secondHeight = 480.0;

        RunOnUIThread([&]()
        {
            window1.Attach(safe_cast<xaml::Window^> (xaml_markup::XamlReader::Load(
                L"<Window xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                L"  >"
                L"  <StackPanel x:Name='rootPanel'/>"
                L"</Window>")));
            window1->Activate();
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Setting Window->Width=%f, Window->Height=%f", firstWidth, firstHeight);
            window1->Width = firstWidth;
            window1->Height = firstHeight;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            const double actualWidth = window1->Width;
            const double actualHeight = window1->Height;
            const auto bounds = window1->Bounds;
            LOG_OUTPUT(L"Read back Width=%f, Height=%f", actualWidth, actualHeight);
            LOG_OUTPUT(L"Bounds: (%f, %f, %f, %f)", bounds.X, bounds.Y, bounds.Width, bounds.Height);

            VERIFY_ARE_EQUAL(std::lround(firstWidth), std::lround(actualWidth));
            VERIFY_ARE_EQUAL(std::lround(firstHeight), std::lround(actualHeight));
            VERIFY_ARE_EQUAL(std::lround(actualWidth), std::lround(bounds.Width));
            VERIFY_ARE_EQUAL(std::lround(actualHeight), std::lround(bounds.Height));
        });

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Setting Window->Width=%f, Window->Height=%f", secondWidth, secondHeight);
            window1->Width = secondWidth;
            window1->Height = secondHeight;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            const double actualWidth = window1->Width;
            const double actualHeight = window1->Height;
            LOG_OUTPUT(L"Read back Width=%f, Height=%f", actualWidth, actualHeight);

            VERIFY_ARE_EQUAL(std::lround(secondWidth), std::lround(actualWidth));
            VERIFY_ARE_EQUAL(std::lround(secondHeight), std::lround(actualHeight));
        });
    }

    void WindowIntegrationTests::SetWindowWidthHeightInMarkup()
    {
        TestCleanupWrapper cleanup;

        WindowAutoCloser window1;

        // The spec calls out setting the initial Window size in XAML
        // markup as a primary scenario. Width/Height are plain WinRT properties, so
        // the XAML parser sets them on the <Window> element just like Title.
        const double markupWidth = 640.0;
        const double markupHeight = 480.0;

        RunOnUIThread([&]()
        {
            window1.Attach(safe_cast<xaml::Window^> (xaml_markup::XamlReader::Load(
                L"<Window xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                L"  Width='640' Height='480'>"
                L"  <StackPanel x:Name='rootPanel'/>"
                L"</Window>")));
            window1->Activate();
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            const double actualWidth = window1->Width;
            const double actualHeight = window1->Height;
            const auto bounds = window1->Bounds;
            LOG_OUTPUT(L"Markup-set size read back: Width=%f, Height=%f", actualWidth, actualHeight);
            LOG_OUTPUT(L"Bounds: (%f, %f, %f, %f)", bounds.X, bounds.Y, bounds.Width, bounds.Height);

            // The markup values should have driven the client-area size.
            VERIFY_ARE_EQUAL(std::lround(markupWidth), std::lround(actualWidth));
            VERIFY_ARE_EQUAL(std::lround(markupHeight), std::lround(actualHeight));

            // In the Normal state, Width/Height equal Bounds.
            VERIFY_ARE_EQUAL(std::lround(actualWidth), std::lround(bounds.Width));
            VERIFY_ARE_EQUAL(std::lround(actualHeight), std::lround(bounds.Height));
        });
    }

    void WindowIntegrationTests::WindowWidthHeightRejectsInvalidValues()
    {
        TestCleanupWrapper cleanup;

        WindowAutoCloser window1;

        RunOnUIThread([&]()
        {
            window1.Attach(safe_cast<xaml::Window^> (xaml_markup::XamlReader::Load(
                L"<Window xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                L"  >"
                L"  <StackPanel x:Name='rootPanel'/>"
                L"</Window>")));
            window1->Activate();
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            const double nan = std::numeric_limits<double>::quiet_NaN();
            const double inf = std::numeric_limits<double>::infinity();
            const double negInf = -std::numeric_limits<double>::infinity();

            // Width
            VERIFY_THROWS_SPECIFIC_WINRT(window1->Width = -1.0, Platform::Exception^, [](Platform::Exception^ ex){ return ex->HResult == E_INVALIDARG; });
            VERIFY_THROWS_SPECIFIC_WINRT(window1->Width = nan, Platform::Exception^, [](Platform::Exception^ ex){ return ex->HResult == E_INVALIDARG; });
            VERIFY_THROWS_SPECIFIC_WINRT(window1->Width = inf, Platform::Exception^, [](Platform::Exception^ ex){ return ex->HResult == E_INVALIDARG; });
            VERIFY_THROWS_SPECIFIC_WINRT(window1->Width = negInf, Platform::Exception^, [](Platform::Exception^ ex){ return ex->HResult == E_INVALIDARG; });

            // Height
            VERIFY_THROWS_SPECIFIC_WINRT(window1->Height = -1.0, Platform::Exception^, [](Platform::Exception^ ex){ return ex->HResult == E_INVALIDARG; });
            VERIFY_THROWS_SPECIFIC_WINRT(window1->Height = nan, Platform::Exception^, [](Platform::Exception^ ex){ return ex->HResult == E_INVALIDARG; });
            VERIFY_THROWS_SPECIFIC_WINRT(window1->Height = inf, Platform::Exception^, [](Platform::Exception^ ex){ return ex->HResult == E_INVALIDARG; });
            VERIFY_THROWS_SPECIFIC_WINRT(window1->Height = negInf, Platform::Exception^, [](Platform::Exception^ ex){ return ex->HResult == E_INVALIDARG; });

            // Absurdly large values are rejected too: they'd overflow the int pixel math
            // (value * dpiScale) used by the Win32 sizing calls. The cap is derived from INT_MAX.
            const double tooLarge = 1.0e18;
            VERIFY_THROWS_SPECIFIC_WINRT(window1->Width = tooLarge, Platform::Exception^, [](Platform::Exception^ ex){ return ex->HResult == E_INVALIDARG; });
            VERIFY_THROWS_SPECIFIC_WINRT(window1->Height = tooLarge, Platform::Exception^, [](Platform::Exception^ ex){ return ex->HResult == E_INVALIDARG; });

            // The E_INVALIDARG carries a specific, developer-friendly message (originated via
            // RoOriginateError), not just the generic "The parameter is incorrect." Both Width and
            // Height share the message, so a substring check keeps this robust.
            const wchar_t* expectedPhrase = L"finite number";
            auto verifyHelpfulMessage = [&](const wchar_t* axis, auto setter)
            {
                bool threw = false;
                try
                {
                    setter();
                }
                catch (Platform::Exception^ ex)
                {
                    threw = true;
                    VERIFY_ARE_EQUAL(E_INVALIDARG, ex->HResult);

                    Platform::String^ message = ex->Message;
                    LOG_OUTPUT(L"%s error message: \"%s\"", axis, message ? message->Data() : L"(null)");
                    VERIFY_IS_NOT_NULL(message, L"Exception should carry a message");
                    VERIFY_IS_TRUE(wcsstr(message->Data(), expectedPhrase) != nullptr,
                        L"Error message should explain the value constraint, not be a generic COM error");
                }
                VERIFY_IS_TRUE(threw, L"Setting an invalid value should throw");
            };
            verifyHelpfulMessage(L"Width", [&]() { window1->Width = nan; });
            verifyHelpfulMessage(L"Height", [&]() { window1->Height = nan; });

            // Zero is allowed per spec - the OS may clamp to a minimum tracking size.
            LOG_OUTPUT(L"Setting Width=0, Height=0 (should not throw)");
            window1->Width = 0.0;
            window1->Height = 0.0;
        });
    }

    void WindowIntegrationTests::WindowWidthHeightRejectsBindingInMarkup()
    {
        TestCleanupWrapper cleanup;

        // A Window has no DataContext and isn't in a tree, so a classic {Binding} on Width/Height can
        // never resolve. Rather than silently pushing the binding's default (0) and collapsing the
        // window, the parser should fail the load with a helpful, property-named error.
        const wchar_t* expectedPhrase = L"Bindings are not permitted";

        auto verifyBindingRejected = [&](const wchar_t* axis, const wchar_t* markup)
        {
            bool threw = false;
            RunOnUIThread([&]()
            {
                try
                {
                    auto window = safe_cast<xaml::Window^>(xaml_markup::XamlReader::Load(
                        ref new Platform::String(markup)));
                    // If we somehow didn't throw, close the window so we don't leak it.
                    if (window) { window->Close(); }
                }
                catch (Platform::Exception^ ex)
                {
                    threw = true;
                    Platform::String^ message = ex->Message;
                    LOG_OUTPUT(L"%s binding load threw: hr=0x%08x message=\"%s\"",
                        axis, ex->HResult, message ? message->Data() : L"(null)");
                    VERIFY_IS_TRUE(message != nullptr && wcsstr(message->Data(), expectedPhrase) != nullptr,
                        L"Error message should explain that bindings aren't permitted on the property");
                }
            });
            VERIFY_IS_TRUE(threw, L"A classic {Binding} on Window.Width/Height should fail to parse");
        };

        verifyBindingRejected(L"Width",
            L"<Window xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
            L"  Width='{Binding NonExistentProp}'>"
            L"  <StackPanel x:Name='rootPanel'/>"
            L"</Window>");

        verifyBindingRejected(L"Height",
            L"<Window xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
            L"  Height='{Binding NonExistentProp}'>"
            L"  <StackPanel x:Name='rootPanel'/>"
            L"</Window>");
    }

    void WindowIntegrationTests::SetWindowWidthHeightWhileMinimizedUpdatesRestoreSize()
    {
        TestCleanupWrapper cleanup;

        WindowAutoCloser window1;

        const double initialWidth = 400.0;
        const double initialHeight = 300.0;
        const double restoreWidth = 640.0;
        const double restoreHeight = 480.0;

        RunOnUIThread([&]()
        {
            window1.Attach(safe_cast<xaml::Window^> (xaml_markup::XamlReader::Load(
                L"<Window xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                L"  >"
                L"  <StackPanel x:Name='rootPanel'/>"
                L"</Window>")));
            window1->Activate();
        });
        TestServices::WindowHelper->WaitForIdle();

        // Establish a known normal-state size to start from.
        RunOnUIThread([&]()
        {
            window1->Width = initialWidth;
            window1->Height = initialHeight;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            IWindowNative* windowNative = nullptr;
            VERIFY_SUCCEEDED(reinterpret_cast<IUnknown*>(window1.get())->QueryInterface(__uuidof(IWindowNative), (void**)&windowNative));
            VERIFY_IS_NOT_NULL(windowNative);
            HWND windowHandle = nullptr;
            VERIFY_SUCCEEDED(windowNative->get_WindowHandle(&windowHandle));
            windowNative->Release();
            VERIFY_IS_TRUE(windowHandle != nullptr);

            // Minimize, then set Width/Height while minimized. Per WPF parity this must update
            // the *restore* size and leave the window minimized - it must not resize the live
            // (minimized) window.
            ::ShowWindow(windowHandle, SW_MINIMIZE);
            VERIFY_IS_TRUE(!!::IsIconic(windowHandle), L"Window should be minimized");

            LOG_OUTPUT(L"Setting Window->Width=%f, Window->Height=%f while minimized", restoreWidth, restoreHeight);
            window1->Width = restoreWidth;
            window1->Height = restoreHeight;

            VERIFY_IS_TRUE(!!::IsIconic(windowHandle), L"Window should still be minimized after setting Width/Height");

            // Getter should return the restore size while still minimized.
            const double wWhileMin = window1->Width;
            const double hWhileMin = window1->Height;
            LOG_OUTPUT(L"Getter while minimized: Width=%f, Height=%f", wWhileMin, hWhileMin);
            VERIFY_ARE_EQUAL(std::lround(restoreWidth), std::lround(wWhileMin), L"Width getter should return restore width while minimized");
            VERIFY_ARE_EQUAL(std::lround(restoreHeight), std::lround(hWhileMin), L"Height getter should return restore height while minimized");

            // Restore: the window should come back at the size we set while minimized.
            ::ShowWindow(windowHandle, SW_RESTORE);
            VERIFY_IS_FALSE(!!::IsIconic(windowHandle), L"Window should be restored");
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            const double actualWidth = window1->Width;
            const double actualHeight = window1->Height;
            const auto bounds = window1->Bounds;
            LOG_OUTPUT(L"After restore: Width=%f, Height=%f, Bounds=(%f, %f, %f, %f)", actualWidth, actualHeight, bounds.X, bounds.Y, bounds.Width, bounds.Height);

            // The restored client size must match what we set while minimized, not the
            // pre-minimize size.
            VERIFY_ARE_EQUAL(std::lround(restoreWidth), std::lround(actualWidth));
            VERIFY_ARE_EQUAL(std::lround(restoreHeight), std::lround(actualHeight));
        });
    }

    void WindowIntegrationTests::CanGetSetWindowWidthHeightBothChromeModes()
    {
        TestCleanupWrapper cleanup;

        WindowAutoCloser window1;


        RunOnUIThread([&]()
        {
            window1.Attach(safe_cast<xaml::Window^>(xaml_markup::XamlReader::Load(
                L"<Window xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                L"  >"
                L"  <StackPanel x:Name='rootPanel'/>"
                L"</Window>")));
            window1->Activate();
        });
        TestServices::WindowHelper->WaitForIdle();

        HWND windowHandle = nullptr;
        RunOnUIThread([&]()
        {
            IWindowNative* windowNative = nullptr;
            VERIFY_SUCCEEDED(reinterpret_cast<IUnknown*>(window1.get())->QueryInterface(
                __uuidof(IWindowNative), (void**)&windowNative));
            VERIFY_SUCCEEDED(windowNative->get_WindowHandle(&windowHandle));
            windowNative->Release();
        });

        // --- Mode 1: ExtendsContentIntoTitleBar OFF (standard Win32 title bar) ---
        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"--- ECITB OFF (standard title bar) ---");
            window1->ExtendsContentIntoTitleBar = false;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            window1->Width = 800.0;
            window1->Height = 600.0;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            const double w = window1->Width;
            const double h = window1->Height;
            LOG_OUTPUT(L"ECITB off: set 800x600, read back %fx%f", w, h);
            VERIFY_ARE_EQUAL(std::lround(800.0), std::lround(w));
            VERIFY_ARE_EQUAL(std::lround(600.0), std::lround(h));
        });

        // --- Mode 2: ExtendsContentIntoTitleBar ON (custom title bar) ---
        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"--- ECITB ON (custom title bar) ---");
            window1->ExtendsContentIntoTitleBar = true;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LogWindowGeometry(L"ECITB on before setting 700x500", windowHandle, window1.get());
        });

        RunOnUIThread([&]()
        {
            window1->Width = 700.0;
            window1->Height = 500.0;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            const double w = window1->Width;
            const double h = window1->Height;
            LOG_OUTPUT(L"ECITB on: set 700x500, read back %fx%f", w, h);
            LogWindowGeometry(L"ECITB on after setting 700x500", windowHandle, window1.get());
            VERIFY_ARE_EQUAL(std::lround(700.0), std::lround(w));
            VERIFY_ARE_EQUAL(std::lround(500.0), std::lround(h));
        });

        // --- Mode 2b: ECITB ON with a Tall title bar ---
        // AppWindowTitleBar.PreferredHeightOption only takes effect with ECITB on, and it changes
        // layout *within* the client area, not the Win32 window/client boundary. So the measured
        // chrome is unchanged and Width/Height must still round-trip. (Verified separately: Tall
        // moves the window/client split by 0px.)
        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"--- ECITB ON + Tall title bar ---");
            window1->AppWindow->TitleBar->PreferredHeightOption = Microsoft::UI::Windowing::TitleBarHeightOption::Tall;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            window1->Width = 720.0;
            window1->Height = 520.0;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            const double w = window1->Width;
            const double h = window1->Height;
            LOG_OUTPUT(L"ECITB on + Tall: set 720x520, read back %fx%f (Bounds=%fx%f)", w, h, window1->Bounds.Width, window1->Bounds.Height);
            VERIFY_ARE_EQUAL(std::lround(720.0), std::lround(w));
            VERIFY_ARE_EQUAL(std::lround(520.0), std::lround(h));
        });

        // Reset the title bar height back to Standard before toggling ECITB off.
        RunOnUIThread([&]()
        {
            window1->AppWindow->TitleBar->PreferredHeightOption = Microsoft::UI::Windowing::TitleBarHeightOption::Standard;
        });
        TestServices::WindowHelper->WaitForIdle();

        // --- Mode 3: Toggle back to ECITB OFF to confirm it still works ---
        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"--- ECITB OFF again ---");
            window1->ExtendsContentIntoTitleBar = false;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            window1->Width = 640.0;
            window1->Height = 480.0;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            const double w = window1->Width;
            const double h = window1->Height;
            LOG_OUTPUT(L"ECITB off (again): set 640x480, read back %fx%f", w, h);
            VERIFY_ARE_EQUAL(std::lround(640.0), std::lround(w));
            VERIFY_ARE_EQUAL(std::lround(480.0), std::lround(h));
        });
    }

    void WindowIntegrationTests::SetWindowWidthWhileMaximizedUpdatesRestoreSize()
    {
        TestCleanupWrapper cleanup;

        WindowAutoCloser window1;

        const double initialWidth = 500.0;
        const double initialHeight = 400.0;
        const double newRestoreWidth = 900.0;

        RunOnUIThread([&]()
        {
            window1.Attach(safe_cast<xaml::Window^>(xaml_markup::XamlReader::Load(
                L"<Window xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                L"  >"
                L"  <StackPanel x:Name='rootPanel'/>"
                L"</Window>")));
            window1->Activate();
        });
        TestServices::WindowHelper->WaitForIdle();

        // Get the HWND once for maximize/restore via ShowWindow.
        HWND windowHandle = nullptr;
        RunOnUIThread([&]()
        {
            IWindowNative* windowNative = nullptr;
            VERIFY_SUCCEEDED(reinterpret_cast<IUnknown*>(window1.get())->QueryInterface(
                __uuidof(IWindowNative), (void**)&windowNative));
            VERIFY_SUCCEEDED(windowNative->get_WindowHandle(&windowHandle));
            windowNative->Release();
        });

        // Test both chrome modes: the caption height changes how the client area
        // is computed, so we want to make sure both paths work.
        bool ecitbModes[] = { false, true };
        for (int modeIndex = 0; modeIndex < 2; modeIndex++)
        {
            bool ecitb = ecitbModes[modeIndex];
            LOG_OUTPUT(L"=== ExtendsContentIntoTitleBar = %s ===", ecitb ? L"true" : L"false");

            RunOnUIThread([&]()
            {
                window1->ExtendsContentIntoTitleBar = ecitb;
            });
            TestServices::WindowHelper->WaitForIdle();

            // Set a known normal-state size.
            RunOnUIThread([&]()
            {
                window1->Width = initialWidth;
                window1->Height = initialHeight;
            });
            TestServices::WindowHelper->WaitForIdle();

            // Maximize, set only Width, then restore.
            RunOnUIThread([&]()
            {
                ::ShowWindow(windowHandle, SW_MAXIMIZE);
                VERIFY_IS_TRUE(!!::IsZoomed(windowHandle), L"Window should be maximized");

                LOG_OUTPUT(L"Setting Width=%f while maximized (Height unchanged)", newRestoreWidth);
                window1->Width = newRestoreWidth;

                VERIFY_IS_TRUE(!!::IsZoomed(windowHandle), L"Window should still be maximized after setting Width");

                // Getter should reflect the new restore size while still maximized.
                const double wWhileMax = window1->Width;
                const double hWhileMax = window1->Height;
                LOG_OUTPUT(L"Getter while maximized: Width=%f, Height=%f", wWhileMax, hWhileMax);
                VERIFY_ARE_EQUAL(std::lround(newRestoreWidth), std::lround(wWhileMax), L"Width getter should return new restore width while maximized");
                VERIFY_ARE_EQUAL(std::lround(initialHeight), std::lround(hWhileMax), L"Height getter should return original restore height (not clobbered)");

                ::ShowWindow(windowHandle, SW_RESTORE);
                VERIFY_IS_FALSE(!!::IsZoomed(windowHandle), L"Window should be restored");
            });
            TestServices::WindowHelper->WaitForIdle();

            // Verify the restore size: Width should be newRestoreWidth, Height should
            // be the original initialHeight (not clobbered by the maximized live height).
            RunOnUIThread([&]()
            {
                const double w = window1->Width;
                const double h = window1->Height;
                LOG_OUTPUT(L"After restore: Width=%f, Height=%f", w, h);

                VERIFY_ARE_EQUAL(std::lround(newRestoreWidth), std::lround(w));
                VERIFY_ARE_EQUAL(std::lround(initialHeight), std::lround(h));
            });
        }
    }

    void WindowIntegrationTests::SetWindowWidthHeightWhileMaximizedWithECITBPreservesOuterClientOffset()
    {
        TestCleanupWrapper cleanup;

        WindowAutoCloser window1;

        const double initialWidth = 500.0;
        const double initialHeight = 400.0;
        const double restoreWidth = 720.0;
        const double restoreHeight = 560.0;
        const double tolerance = 2.0;

        RunOnUIThread([&]()
        {
            window1.Attach(safe_cast<xaml::Window^>(xaml_markup::XamlReader::Load(
                L"<Window xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                L"  >"
                L"  <StackPanel x:Name='rootPanel'/>"
                L"</Window>")));
            window1->Activate();
        });
        TestServices::WindowHelper->WaitForIdle();

        HWND windowHandle = nullptr;
        RunOnUIThread([&]()
        {
            IWindowNative* windowNative = nullptr;
            VERIFY_SUCCEEDED(reinterpret_cast<IUnknown*>(window1.get())->QueryInterface(
                __uuidof(IWindowNative), (void**)&windowNative));
            VERIFY_SUCCEEDED(windowNative->get_WindowHandle(&windowHandle));
            windowNative->Release();
        });

        auto captureOuterClientOffset = [&](double& widthOffset, double& heightOffset)
        {
            RECT outerRect{};
            VERIFY_IS_TRUE(!!::GetWindowRect(windowHandle, &outerRect));

            const UINT dpi = ::GetDpiForWindow(windowHandle);
            const double scale = static_cast<double>(dpi) / static_cast<double>(USER_DEFAULT_SCREEN_DPI);
            const double outerWidthInDips = static_cast<double>(outerRect.right - outerRect.left) / scale;
            const double outerHeightInDips = static_cast<double>(outerRect.bottom - outerRect.top) / scale;

            widthOffset = outerWidthInDips - window1->Width;
            heightOffset = outerHeightInDips - window1->Height;
            LogWindowGeometry(L"ECITB restore-size offset capture", windowHandle, window1.get());
        };

        double initialWidthOffset = 0.0;
        double initialHeightOffset = 0.0;
        RunOnUIThread([&]()
        {
            window1->ExtendsContentIntoTitleBar = true;
            window1->Width = initialWidth;
            window1->Height = initialHeight;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            captureOuterClientOffset(initialWidthOffset, initialHeightOffset);
        });

        RunOnUIThread([&]()
        {
            ::ShowWindow(windowHandle, SW_MAXIMIZE);
            VERIFY_IS_TRUE(!!::IsZoomed(windowHandle), L"Window should be maximized");

            window1->Width = restoreWidth;
            window1->Height = restoreHeight;

            VERIFY_IS_TRUE(!!::IsZoomed(windowHandle), L"Window should still be maximized after setting Width/Height");
            VERIFY_ARE_EQUAL(std::lround(restoreWidth), std::lround(window1->Width), L"Getter should return restore width while maximized");
            VERIFY_ARE_EQUAL(std::lround(restoreHeight), std::lround(window1->Height), L"Getter should return restore height while maximized");

            ::ShowWindow(windowHandle, SW_RESTORE);
            VERIFY_IS_FALSE(!!::IsZoomed(windowHandle), L"Window should be restored");
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(std::lround(restoreWidth), std::lround(window1->Width));
            VERIFY_ARE_EQUAL(std::lround(restoreHeight), std::lround(window1->Height));

            double restoredWidthOffset = 0.0;
            double restoredHeightOffset = 0.0;
            captureOuterClientOffset(restoredWidthOffset, restoredHeightOffset);

            VERIFY_IS_TRUE(std::abs(initialWidthOffset - restoredWidthOffset) <= tolerance,
                L"Restored HWND outer width should have the same chrome offset from Window.Width as before maximize");
            VERIFY_IS_TRUE(std::abs(initialHeightOffset - restoredHeightOffset) <= tolerance,
                L"Restored HWND outer height should have the same chrome offset from Window.Height as before maximize");
        });
    }

    void WindowIntegrationTests::GetWindowWidthHeightWhileMaximizedReportsRestoreSize()
    {
        TestCleanupWrapper cleanup;

        // While maximized the getter returns the *restore* size (what the window un-maximizes to),
        // never the live maximized size. This holds whether or not the app ever set Width/Height,
        // because the OS tracks the restore rect in WINDOWPLACEMENT - the deliberate contrast to
        // FullScreen/CompactOverlay, where there is no OS-tracked restore size. This walks both:
        //   Phase 1: never set - the getter returns the OS-tracked normal restore size.
        //   Phase 2: set a value, then set again while maximized - the getter tracks the restore size.
        const double setRestoreWidth = 500.0;
        const double setRestoreHeight = 400.0;
        const double setWhileMaxWidth = 640.0;
        const double setWhileMaxHeight = 480.0;
        const double tolerance = 2.0;
        auto areClose = [tolerance](double a, double b) { return std::abs(a - b) <= tolerance; };

        WindowAutoCloser window1;
        HWND windowHandle = nullptr;

        RunOnUIThread([&]()
        {
            window1.Attach(safe_cast<xaml::Window^>(xaml_markup::XamlReader::Load(
                L"<Window xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                L"  >"
                L"  <StackPanel x:Name='rootPanel'/>"
                L"</Window>")));
            window1->Activate();
        });
        TestServices::WindowHelper->WaitForIdle();

        // ---- Phase 1: never set Width/Height. ----
        double normalWidth = 0, normalHeight = 0;
        RunOnUIThread([&]()
        {
            IWindowNative* windowNative = nullptr;
            VERIFY_SUCCEEDED(reinterpret_cast<IUnknown*>(window1.get())->QueryInterface(
                __uuidof(IWindowNative), (void**)&windowNative));
            VERIFY_SUCCEEDED(windowNative->get_WindowHandle(&windowHandle));
            windowNative->Release();

            normalWidth = window1->Width;
            normalHeight = window1->Height;
            LOG_OUTPUT(L"Normal (never set): Width=%f, Height=%f", normalWidth, normalHeight);
        });

        RunOnUIThread([&]() { ::ShowWindow(windowHandle, SW_MAXIMIZE); });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            const double maxBoundsW = window1->Bounds.Width;
            const double maxBoundsH = window1->Bounds.Height;
            const double w = window1->Width;
            const double h = window1->Height;
            LOG_OUTPUT(L"Maximized (never set): Bounds=%fx%f Width/Height=%fx%f (normal was %fx%f)", maxBoundsW, maxBoundsH, w, h, normalWidth, normalHeight);

            VERIFY_IS_TRUE(areClose(normalWidth, w), L"Maximized getter should return the restore width even when never set");
            VERIFY_IS_TRUE(areClose(normalHeight, h), L"Maximized getter should return the restore height even when never set");

            // Sanity: the live maximized window is larger than the restore size, so the checks above
            // aren't vacuous and the getter clearly isn't returning the live maximized size.
            VERIFY_IS_TRUE(maxBoundsW > normalWidth + tolerance || maxBoundsH > normalHeight + tolerance,
                L"Maximized live Bounds should be larger than the restore size");
            VERIFY_IS_FALSE(areClose(maxBoundsW, w) && areClose(maxBoundsH, h),
                L"Maximized getter should not return the live maximized size");
        });

        RunOnUIThread([&]() { ::ShowWindow(windowHandle, SW_RESTORE); });
        TestServices::WindowHelper->WaitForIdle();

        // ---- Phase 2: set a value, maximize, then set again while maximized. ----
        RunOnUIThread([&]()
        {
            window1->Width = setRestoreWidth;
            window1->Height = setRestoreHeight;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]() { ::ShowWindow(windowHandle, SW_MAXIMIZE); });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            const double w = window1->Width;
            const double h = window1->Height;
            LOG_OUTPUT(L"Maximized (after set): Width=%f, Height=%f", w, h);
            VERIFY_IS_TRUE(areClose(setRestoreWidth, w), L"Getter should return the set restore width while maximized");
            VERIFY_IS_TRUE(areClose(setRestoreHeight, h), L"Getter should return the set restore height while maximized");

            // Setting while maximized updates the restore size only; the live window stays maximized.
            window1->Width = setWhileMaxWidth;
            window1->Height = setWhileMaxHeight;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            const double w = window1->Width;
            const double h = window1->Height;
            LOG_OUTPUT(L"Maximized (after set-while-maximized): Width=%f, Height=%f", w, h);
            VERIFY_IS_TRUE(areClose(setWhileMaxWidth, w), L"Getter should return the restore width we just set while maximized");
            VERIFY_IS_TRUE(areClose(setWhileMaxHeight, h), L"Getter should return the restore height we just set while maximized");

            ::ShowWindow(windowHandle, SW_RESTORE);
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // After restore, the window comes back at the size we set while maximized.
            const double w = window1->Width;
            const double h = window1->Height;
            LOG_OUTPUT(L"After restore: Width=%f, Height=%f", w, h);
            VERIFY_IS_TRUE(areClose(setWhileMaxWidth, w), L"After restore, width should be the size set while maximized");
            VERIFY_IS_TRUE(areClose(setWhileMaxHeight, h), L"After restore, height should be the size set while maximized");
        });
    }

    void WindowIntegrationTests::GetWindowWidthHeightAfterSettingWhileMaximizedThenFullScreenReportsSetSize()
    {
        TestCleanupWrapper cleanup;

        // Regression: set Width/Height while maximized, then switch straight into FullScreen (a
        // non-sizing presenter) without un-maximizing first. FullScreen's getter reads the tracked
        // restored size (m_lastRestoredClientSizeDips), so setting while maximized must keep that in
        // sync - otherwise the getter reports the previously tracked size instead of the value just set.
        const double initialWidth = 500.0;
        const double initialHeight = 400.0;
        const double setWhileMaxWidth = 720.0;
        const double setWhileMaxHeight = 560.0;
        const double tolerance = 2.0;
        auto areClose = [tolerance](double a, double b) { return std::abs(a - b) <= tolerance; };

        WindowAutoCloser window1;
        HWND windowHandle = nullptr;

        RunOnUIThread([&]()
        {
            window1.Attach(safe_cast<xaml::Window^>(xaml_markup::XamlReader::Load(
                L"<Window xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                L"  >"
                L"  <StackPanel x:Name='rootPanel'/>"
                L"</Window>")));
            window1->Activate();
        });
        TestServices::WindowHelper->WaitForIdle();

        // Establish a known restored size, then capture the HWND.
        RunOnUIThread([&]()
        {
            IWindowNative* windowNative = nullptr;
            VERIFY_SUCCEEDED(reinterpret_cast<IUnknown*>(window1.get())->QueryInterface(
                __uuidof(IWindowNative), (void**)&windowNative));
            VERIFY_SUCCEEDED(windowNative->get_WindowHandle(&windowHandle));
            windowNative->Release();

            window1->Width = initialWidth;
            window1->Height = initialHeight;
        });
        TestServices::WindowHelper->WaitForIdle();

        // Maximize, then set a new size while maximized (updates the restore size only).
        RunOnUIThread([&]() { ::ShowWindow(windowHandle, SW_MAXIMIZE); });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(!!::IsZoomed(windowHandle), L"Window should be maximized");
            window1->Width = setWhileMaxWidth;
            window1->Height = setWhileMaxHeight;
        });
        TestServices::WindowHelper->WaitForIdle();

        // Switch straight from maximized into FullScreen (no un-maximize in between).
        RunOnUIThread([&]()
        {
            window1->AppWindow->SetPresenter(Microsoft::UI::Windowing::AppWindowPresenterKind::FullScreen);
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            const double w = window1->Width;
            const double h = window1->Height;
            LOG_OUTPUT(L"In FullScreen after set-while-maximized: Width=%f Height=%f (set was %fx%f)", w, h, setWhileMaxWidth, setWhileMaxHeight);
            VERIFY_IS_TRUE(areClose(setWhileMaxWidth, w), L"FullScreen getter should report the width set while maximized, not a stale tracked size");
            VERIFY_IS_TRUE(areClose(setWhileMaxHeight, h), L"FullScreen getter should report the height set while maximized, not a stale tracked size");
        });
    }

    void WindowIntegrationTests::SetWindowWidthHeightInNonDefaultPresenterIsDeferred()
    {
        TestCleanupWrapper cleanup;

        // Setting Width/Height while a non-sizing presenter (FullScreen/CompactOverlay) is active
        // no longer throws. The requested client size is remembered (and returned by the getter),
        // then applied when the window returns to the Default/Overlapped presenter - mirroring how
        // SetWindowPlacement remembers rcNormalPosition while maximized/minimized.
        const double originalWidth = 500.0;
        const double originalHeight = 400.0;
        const double deferredWidth = 900.0;
        const double deferredHeight = 700.0;

        auto runForPresenter = [&](Microsoft::UI::Windowing::AppWindowPresenterKind presenterKind, const wchar_t* presenterName)
        {
            LOG_OUTPUT(L"===== Presenter: %s =====", presenterName);

            WindowAutoCloser window1;
            wf::Rect liveBoundsInPresenter{};

            RunOnUIThread([&]()
            {
                window1.Attach(safe_cast<xaml::Window^>(xaml_markup::XamlReader::Load(
                    L"<Window xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                    L"  >"
                    L"  <StackPanel x:Name='rootPanel'/>"
                    L"</Window>")));
                window1->Activate();
            });
            TestServices::WindowHelper->WaitForIdle();

            // Set a known normal-state size.
            RunOnUIThread([&]()
            {
                window1->Width = originalWidth;
                window1->Height = originalHeight;
            });
            TestServices::WindowHelper->WaitForIdle();

            // Enter the non-default presenter via AppWindow.
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Entering %s via AppWindow.SetPresenter", presenterName);
                window1->AppWindow->SetPresenter(presenterKind);
            });
            TestServices::WindowHelper->WaitForIdle();

            // Capture the live (presenter) bounds so we can prove the deferred set below does not
            // resize the live window - e.g. yank a FullScreen window back to a restored rect.
            RunOnUIThread([&]()
            {
                liveBoundsInPresenter = window1->Bounds;
                LOG_OUTPUT(L"Live bounds in %s: %f x %f", presenterName, liveBoundsInPresenter.Width, liveBoundsInPresenter.Height);
            });

            // Setting Width/Height in a non-sizing presenter is remembered, not rejected, and the
            // getter returns the remembered value while that presenter is active.
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Setting Width/Height in %s (should be remembered, not throw)", presenterName);
                window1->Width = deferredWidth;
                window1->Height = deferredHeight;

                LOG_OUTPUT(L"After set in %s: Width=%f, Height=%f", presenterName, window1->Width, window1->Height);
                VERIFY_ARE_EQUAL(std::lround(deferredWidth), std::lround(window1->Width), L"Width getter should return the remembered value in a non-sizing presenter");
                VERIFY_ARE_EQUAL(std::lround(deferredHeight), std::lround(window1->Height), L"Height getter should return the remembered value in a non-sizing presenter");

                // The remembered set must NOT touch the live window: it should still fill the
                // presenter (e.g. still be full-screen), not snap to the deferred/restore size.
                const auto liveBoundsAfterSet = window1->Bounds;
                LOG_OUTPUT(L"Live bounds after set in %s: %f x %f", presenterName, liveBoundsAfterSet.Width, liveBoundsAfterSet.Height);
                VERIFY_ARE_EQUAL(std::lround(liveBoundsInPresenter.Width), std::lround(liveBoundsAfterSet.Width), L"Setting Width in a non-sizing presenter must not resize the live window");
                VERIFY_ARE_EQUAL(std::lround(liveBoundsInPresenter.Height), std::lround(liveBoundsAfterSet.Height), L"Setting Height in a non-sizing presenter must not resize the live window");
            });
            TestServices::WindowHelper->WaitForIdle();

            // Return to the Default presenter; the remembered size should now be applied to the window.
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Returning %s to the Default presenter", presenterName);
                window1->AppWindow->SetPresenter(Microsoft::UI::Windowing::AppWindowPresenterKind::Default);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"After returning to Default: Width=%f, Height=%f", window1->Width, window1->Height);
                VERIFY_ARE_EQUAL(std::lround(deferredWidth), std::lround(window1->Width), L"After returning to Default, width should be the remembered value");
                VERIFY_ARE_EQUAL(std::lround(deferredHeight), std::lround(window1->Height), L"After returning to Default, height should be the remembered value");
            });
        };

        runForPresenter(Microsoft::UI::Windowing::AppWindowPresenterKind::FullScreen, L"FullScreen");
        runForPresenter(Microsoft::UI::Windowing::AppWindowPresenterKind::CompactOverlay, L"CompactOverlay");
    }

    void WindowIntegrationTests::GetWindowWidthHeightInNonDefaultPresenterWithoutSetReturnsRestoreSize()
    {
        TestCleanupWrapper cleanup;

        // Per spec: in a non-default presenter (FullScreen/CompactOverlay) the getter returns the
        // restore size - the size the window had in its restored state - even if the app never set
        // Width/Height. (The opt-in only governs whether the runtime *resizes* the window, not what
        // the getter reports.) The window still round-trips through the presenter untouched, because
        // for a never-set app the runtime imposes no size on exit - the presenter restores it.
        const double tolerance = 2.0;
        auto areClose = [tolerance](double a, double b) { return std::abs(a - b) <= tolerance; };

        auto runForPresenter = [&](Microsoft::UI::Windowing::AppWindowPresenterKind presenterKind, const wchar_t* presenterName)
        {
            LOG_OUTPUT(L"===== Presenter: %s =====", presenterName);

            WindowAutoCloser window1;

            RunOnUIThread([&]()
            {
                window1.Attach(safe_cast<xaml::Window^>(xaml_markup::XamlReader::Load(
                    L"<Window xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                    L"  >"
                    L"  <StackPanel x:Name='rootPanel'/>"
                    L"</Window>")));
                window1->Activate();
            });
            TestServices::WindowHelper->WaitForIdle();

            // Capture the restored-state size. We deliberately never set Width/Height anywhere.
            double normalBoundsW = 0, normalBoundsH = 0;
            RunOnUIThread([&]()
            {
                normalBoundsW = window1->Bounds.Width;
                normalBoundsH = window1->Bounds.Height;
                const double normalWidth = window1->Width;
                const double normalHeight = window1->Height;
                LOG_OUTPUT(L"Restored: Bounds=%fx%f Width/Height=%fx%f", normalBoundsW, normalBoundsH, normalWidth, normalHeight);

                // In the restored state, the getter equals Bounds (restore size == live size).
                VERIFY_IS_TRUE(areClose(normalBoundsW, normalWidth), L"Restored: Width should equal Bounds.Width");
                VERIFY_IS_TRUE(areClose(normalBoundsH, normalHeight), L"Restored: Height should equal Bounds.Height");
            });

            // Enter the non-default presenter.
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Entering %s via AppWindow.SetPresenter", presenterName);
                window1->AppWindow->SetPresenter(presenterKind);
            });
            TestServices::WindowHelper->WaitForIdle();

            // Even though nothing was ever set, the getter reports the RESTORE size (the pre-presenter
            // restored size), NOT the live full-screen / compact size.
            RunOnUIThread([&]()
            {
                const double liveBoundsW = window1->Bounds.Width;
                const double liveBoundsH = window1->Bounds.Height;
                const double w = window1->Width;
                const double h = window1->Height;
                LOG_OUTPUT(L"%s: Bounds=%fx%f Width/Height=%fx%f (restore was %fx%f)", presenterName, liveBoundsW, liveBoundsH, w, h, normalBoundsW, normalBoundsH);

                VERIFY_IS_TRUE(areClose(normalBoundsW, w), L"Width should report the restore size in a non-default presenter even when never set");
                VERIFY_IS_TRUE(areClose(normalBoundsH, h), L"Height should report the restore size in a non-default presenter even when never set");

                // Sanity: the presenter actually changed the live size, so the getter is clearly
                // returning the restore size and not just the live size.
                VERIFY_IS_FALSE(areClose(liveBoundsW, normalBoundsW) && areClose(liveBoundsH, normalBoundsH),
                    L"Presenter should have changed the live size relative to the restore size");
                VERIFY_IS_FALSE(areClose(liveBoundsW, w) && areClose(liveBoundsH, h),
                    L"Getter should not be returning the live presenter size");
            });

            // Return to the Default presenter. Since we never set Width/Height, WinUI imposes nothing;
            // the window restores to its original size on its own (the presenter's own restore).
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Returning %s to the Default presenter", presenterName);
                window1->AppWindow->SetPresenter(Microsoft::UI::Windowing::AppWindowPresenterKind::Default);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                const double w = window1->Width;
                const double h = window1->Height;
                LOG_OUTPUT(L"After return to Default: Bounds=%fx%f Width/Height=%fx%f", window1->Bounds.Width, window1->Bounds.Height, w, h);

                VERIFY_IS_TRUE(areClose(normalBoundsW, w), L"After return, width should be back to the original restored size");
                VERIFY_IS_TRUE(areClose(normalBoundsH, h), L"After return, height should be back to the original restored size");
            });
        };

        runForPresenter(Microsoft::UI::Windowing::AppWindowPresenterKind::FullScreen, L"FullScreen");
        runForPresenter(Microsoft::UI::Windowing::AppWindowPresenterKind::CompactOverlay, L"CompactOverlay");
    }

    void WindowIntegrationTests::ToggleECITBPreservesSetClientSize()
    {
        TestCleanupWrapper cleanup;

        // Once a Width/Height has been set, toggling ExtendsContentIntoTitleBar (ECITB) preserves the
        // client size - the physical window shrinks/grows by the caption rather than the client area
        // changing. This covers two facets:
        //   Facet 1 (order independence): set-then-toggle and toggle-then-set end with the same
        //            client size, which is the value that was set.
        //   Facet 2 (round-trip): toggling ECITB on then off again keeps the set size at each step.
        const double targetWidth = 700.0;
        const double targetHeight = 500.0;
        const double tolerance = 2.0;
        auto areClose = [tolerance](double a, double b) { return std::abs(a - b) <= tolerance; };

        auto measure = [&](bool toggleEcitbFirst, const wchar_t* label, double& outWidth, double& outHeight)
        {
            LOG_OUTPUT(L"===== %s =====", label);

            WindowAutoCloser window1;

            RunOnUIThread([&]()
            {
                window1.Attach(safe_cast<xaml::Window^>(xaml_markup::XamlReader::Load(
                    L"<Window xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                    L"  >"
                    L"  <StackPanel x:Name='rootPanel'/>"
                    L"</Window>")));
                window1->Activate();
                // Start from a known state: standard title bar (ECITB off).
                window1->ExtendsContentIntoTitleBar = false;
            });
            TestServices::WindowHelper->WaitForIdle();

            if (toggleEcitbFirst)
            {
                RunOnUIThread([&]() { window1->ExtendsContentIntoTitleBar = true; });
                TestServices::WindowHelper->WaitForIdle();
                RunOnUIThread([&]()
                {
                    window1->Width = targetWidth;
                    window1->Height = targetHeight;
                });
                TestServices::WindowHelper->WaitForIdle();
            }
            else
            {
                RunOnUIThread([&]()
                {
                    window1->Width = targetWidth;
                    window1->Height = targetHeight;
                });
                TestServices::WindowHelper->WaitForIdle();
                RunOnUIThread([&]() { window1->ExtendsContentIntoTitleBar = true; });
                TestServices::WindowHelper->WaitForIdle();
            }

            RunOnUIThread([&]()
            {
                outWidth = window1->Width;
                outHeight = window1->Height;
                LOG_OUTPUT(L"%s result: Width=%f, Height=%f (Bounds=%fx%f)", label, outWidth, outHeight, window1->Bounds.Width, window1->Bounds.Height);
            });
        };

        double widthA = 0, heightA = 0; // set, then toggle ECITB
        double widthB = 0, heightB = 0; // toggle ECITB, then set
        measure(false, L"Order A: set Height then toggle ECITB", widthA, heightA);
        measure(true, L"Order B: toggle ECITB then set Height", widthB, heightB);

        // Both orders should land on the same client size, and that size should be what we set.
        VERIFY_IS_TRUE(areClose(targetHeight, heightA), L"Order A: client Height should equal the value set, even though ECITB was toggled afterward");
        VERIFY_IS_TRUE(areClose(targetHeight, heightB), L"Order B: client Height should equal the value set");
        VERIFY_IS_TRUE(areClose(heightA, heightB), L"Client Height should be the same regardless of order");

        VERIFY_IS_TRUE(areClose(targetWidth, widthA), L"Order A: client Width should equal the value set");
        VERIFY_IS_TRUE(areClose(targetWidth, widthB), L"Order B: client Width should equal the value set");
        VERIFY_IS_TRUE(areClose(widthA, widthB), L"Client Width should be the same regardless of order");

        // --- Facet 2: round-trip - set with ECITB off, toggle on then off, size preserved each step. ---
        WindowAutoCloser window1;

        RunOnUIThread([&]()
        {
            window1.Attach(safe_cast<xaml::Window^>(xaml_markup::XamlReader::Load(
                L"<Window xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                L"  >"
                L"  <StackPanel x:Name='rootPanel'/>"
                L"</Window>")));
            window1->Activate();
            window1->ExtendsContentIntoTitleBar = false;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            window1->Width = targetWidth;
            window1->Height = targetHeight;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"ECITB off, after set: Width=%f Height=%f", window1->Width, window1->Height);
            VERIFY_IS_TRUE(areClose(targetWidth, window1->Width), L"Width should be the set value (ECITB off)");
            VERIFY_IS_TRUE(areClose(targetHeight, window1->Height), L"Height should be the set value (ECITB off)");
        });

        // Toggle ECITB ON - client size should be preserved.
        RunOnUIThread([&]() { window1->ExtendsContentIntoTitleBar = true; });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"ECITB on: Width=%f Height=%f", window1->Width, window1->Height);
            VERIFY_IS_TRUE(areClose(targetWidth, window1->Width), L"Width should be preserved when toggling ECITB on");
            VERIFY_IS_TRUE(areClose(targetHeight, window1->Height), L"Height should be preserved when toggling ECITB on");
        });

        // Toggle ECITB OFF again - client size should still be preserved.
        RunOnUIThread([&]() { window1->ExtendsContentIntoTitleBar = false; });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"ECITB off again: Width=%f Height=%f", window1->Width, window1->Height);
            VERIFY_IS_TRUE(areClose(targetWidth, window1->Width), L"Width should be preserved when toggling ECITB back off");
            VERIFY_IS_TRUE(areClose(targetHeight, window1->Height), L"Height should be preserved when toggling ECITB back off");
        });
    }

    void WindowIntegrationTests::ToggleECITBWithoutSetLeavesWindowSizeUnchanged()
    {
        TestCleanupWrapper cleanup;

        // If the app never set Width/Height, toggling ExtendsContentIntoTitleBar
        // does NOT change the window size. The outer window rect stays put; only the client area
        // changes as the caption folds in/out. This is the compat guarantee for non-users, and it
        // guards the opt-in gate on the Width/Height-aware ECITB resize.
        const double tolerance = 3.0;
        auto areClose = [tolerance](double a, double b) { return std::abs(a - b) <= tolerance; };

        WindowAutoCloser window1;
        HWND windowHandle = nullptr;

        RunOnUIThread([&]()
        {
            window1.Attach(safe_cast<xaml::Window^>(xaml_markup::XamlReader::Load(
                L"<Window xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                L"  >"
                L"  <StackPanel x:Name='rootPanel'/>"
                L"</Window>")));
            window1->Activate();
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            IWindowNative* windowNative = nullptr;
            VERIFY_SUCCEEDED(reinterpret_cast<IUnknown*>(window1.get())->QueryInterface(
                __uuidof(IWindowNative), (void**)&windowNative));
            VERIFY_SUCCEEDED(windowNative->get_WindowHandle(&windowHandle));
            windowNative->Release();

            // Start from a known state: standard title bar (ECITB off). We never set Width/Height.
            window1->ExtendsContentIntoTitleBar = false;
        });
        TestServices::WindowHelper->WaitForIdle();

        // Helper: read the outer window size and the client height in DIPs.
        auto readGeometryInDips = [&](double& outerW, double& outerH, double& clientH)
        {
            RECT outer{};
            VERIFY_IS_TRUE(!!::GetWindowRect(windowHandle, &outer));
            RECT client{};
            VERIFY_IS_TRUE(!!::GetClientRect(windowHandle, &client));
            const double scale = static_cast<double>(::GetDpiForWindow(windowHandle)) / static_cast<double>(USER_DEFAULT_SCREEN_DPI);
            outerW = (outer.right - outer.left) / scale;
            outerH = (outer.bottom - outer.top) / scale;
            clientH = (client.bottom - client.top) / scale;
        };

        double outerWBefore = 0, outerHBefore = 0, clientHBefore = 0;
        RunOnUIThread([&]()
        {
            readGeometryInDips(outerWBefore, outerHBefore, clientHBefore);
            LOG_OUTPUT(L"Before toggle: outer=%fx%f clientH=%f", outerWBefore, outerHBefore, clientHBefore);
        });

        RunOnUIThread([&]()
        {
            window1->ExtendsContentIntoTitleBar = true;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            double outerWAfter = 0, outerHAfter = 0, clientHAfter = 0;
            readGeometryInDips(outerWAfter, outerHAfter, clientHAfter);
            LOG_OUTPUT(L"After toggle: outer=%fx%f clientH=%f", outerWAfter, outerHAfter, clientHAfter);

            // The outer window rect must not change for an app that never set Width/Height.
            VERIFY_IS_TRUE(areClose(outerWBefore, outerWAfter), L"Outer window width should not change when toggling ECITB without setting Width/Height");
            VERIFY_IS_TRUE(areClose(outerHBefore, outerHAfter), L"Outer window height should not change when toggling ECITB without setting Width/Height");

            // Sanity: the toggle had an effect - the caption folded into the client area, so the
            // client height grew. (If this didn't change, the outer-unchanged check would be vacuous.)
            VERIFY_IS_TRUE(std::abs(clientHAfter - clientHBefore) > 5.0, L"Client height should change as the caption folds into the client area");
        });
    }

    void WindowIntegrationTests::GetWindowWidthHeightInPresenterHonorsValueSetBeforeSwitching()
    {
        TestCleanupWrapper cleanup;

        // Per spec: in FullScreen/CompactOverlay the getter returns the live size ONLY if the app
        // never set Width/Height. If you set a value before switching presenters, the getter must
        // still honor that value (the size the window will return to), not jump to the live
        // full-screen / compact size.
        const double setWidth = 640.0;
        const double setHeight = 480.0;
        const double tolerance = 2.0;
        auto areClose = [tolerance](double a, double b) { return std::abs(a - b) <= tolerance; };

        auto runForPresenter = [&](Microsoft::UI::Windowing::AppWindowPresenterKind presenterKind, const wchar_t* presenterName)
        {
            LOG_OUTPUT(L"===== Presenter: %s =====", presenterName);

            WindowAutoCloser window1;

            RunOnUIThread([&]()
            {
                window1.Attach(safe_cast<xaml::Window^>(xaml_markup::XamlReader::Load(
                    L"<Window xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                    L"  >"
                    L"  <StackPanel x:Name='rootPanel'/>"
                    L"</Window>")));
                window1->Activate();
            });
            TestServices::WindowHelper->WaitForIdle();

            // Set Width/Height while in the default (Overlapped) presenter - applied to the live window.
            RunOnUIThread([&]()
            {
                window1->Width = setWidth;
                window1->Height = setHeight;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(areClose(setWidth, window1->Width), L"Sanity: normal-state Width should be the set value");
                VERIFY_IS_TRUE(areClose(setHeight, window1->Height), L"Sanity: normal-state Height should be the set value");
            });

            // Switch to the non-sizing presenter WITHOUT setting anything new.
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Switching to %s (value was set beforehand)", presenterName);
                window1->AppWindow->SetPresenter(presenterKind);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                const double liveBoundsW = window1->Bounds.Width;
                const double liveBoundsH = window1->Bounds.Height;
                const double w = window1->Width;
                const double h = window1->Height;
                LOG_OUTPUT(L"%s: Bounds=%fx%f Width/Height=%fx%f (set was %fx%f)", presenterName, liveBoundsW, liveBoundsH, w, h, setWidth, setHeight);

                // The getter must still return the value we set before switching presenters.
                VERIFY_IS_TRUE(areClose(setWidth, w), L"Width should honor the value set before switching to the presenter");
                VERIFY_IS_TRUE(areClose(setHeight, h), L"Height should honor the value set before switching to the presenter");

                // Sanity: the presenter changed the live size, so the check above isn't vacuous.
                VERIFY_IS_FALSE(areClose(liveBoundsW, setWidth) && areClose(liveBoundsH, setHeight),
                    L"Presenter should have changed the live size relative to the set size");
            });

            // On return to Default, the window should be at the value we set.
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Returning %s to the Default presenter", presenterName);
                window1->AppWindow->SetPresenter(Microsoft::UI::Windowing::AppWindowPresenterKind::Default);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"After return to Default: Width=%f Height=%f", window1->Width, window1->Height);
                VERIFY_IS_TRUE(areClose(setWidth, window1->Width), L"After return, width should be the set value");
                VERIFY_IS_TRUE(areClose(setHeight, window1->Height), L"After return, height should be the set value");
            });
        };

        runForPresenter(Microsoft::UI::Windowing::AppWindowPresenterKind::FullScreen, L"FullScreen");
        runForPresenter(Microsoft::UI::Windowing::AppWindowPresenterKind::CompactOverlay, L"CompactOverlay");
    }

    void WindowIntegrationTests::GetWindowWidthHeightInPresenterReflectsResizeBeforeSwitching()
    {
        TestCleanupWrapper cleanup;

        // A user resize supersedes an app-set size on presenter exit: once the app has opted
        // in, the getter in a non-sizing presenter reports the window's last *normal-state* size - so
        // a user resize that happens after the app sets Width/Height, but before switching presenters,
        // is reflected by the getter (and is what the window restores to), not the original set value.
        const double setWidth = 500.0;
        const double setHeight = 400.0;
        const double tolerance = 2.0;
        auto areClose = [tolerance](double a, double b) { return std::abs(a - b) <= tolerance; };

        auto runForPresenter = [&](Microsoft::UI::Windowing::AppWindowPresenterKind presenterKind, const wchar_t* presenterName)
        {
            LOG_OUTPUT(L"===== Presenter: %s =====", presenterName);

            WindowAutoCloser window1;
            HWND windowHandle = nullptr;

            RunOnUIThread([&]()
            {
                window1.Attach(safe_cast<xaml::Window^>(xaml_markup::XamlReader::Load(
                    L"<Window xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                    L"  >"
                    L"  <StackPanel x:Name='rootPanel'/>"
                    L"</Window>")));
                window1->Activate();
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                IWindowNative* windowNative = nullptr;
                VERIFY_SUCCEEDED(reinterpret_cast<IUnknown*>(window1.get())->QueryInterface(
                    __uuidof(IWindowNative), (void**)&windowNative));
                VERIFY_SUCCEEDED(windowNative->get_WindowHandle(&windowHandle));
                windowNative->Release();

                // App sets Width/Height (opts in).
                window1->Width = setWidth;
                window1->Height = setHeight;
            });
            TestServices::WindowHelper->WaitForIdle();

            // Simulate a user resize via a direct SetWindowPos to a clearly different window size.
            RunOnUIThread([&]()
            {
                ::SetWindowPos(windowHandle, nullptr, 0, 0, 820, 660, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
            });
            TestServices::WindowHelper->WaitForIdle();

            // Capture the resized normal-state client size (what the window will restore to).
            double resizedWidth = 0, resizedHeight = 0;
            RunOnUIThread([&]()
            {
                resizedWidth = window1->Bounds.Width;
                resizedHeight = window1->Bounds.Height;
                LOG_OUTPUT(L"After user resize: Bounds=%fx%f (set was %fx%f)", resizedWidth, resizedHeight, setWidth, setHeight);

                // Sanity: the resize actually changed the size away from the set value.
                VERIFY_IS_FALSE(areClose(resizedWidth, setWidth), L"Resize should have changed the width away from the set value");
            });

            // Switch to the non-sizing presenter.
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Switching to %s", presenterName);
                window1->AppWindow->SetPresenter(presenterKind);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                const double w = window1->Width;
                const double h = window1->Height;
                LOG_OUTPUT(L"%s: Width/Height=%fx%f (resized was %fx%f, set was %fx%f)", presenterName, w, h, resizedWidth, resizedHeight, setWidth, setHeight);

                // The getter reports the resized (last normal) size, not the original set value.
                VERIFY_IS_TRUE(areClose(resizedWidth, w), L"Width should reflect the user resize, not the original set value");
                VERIFY_IS_TRUE(areClose(resizedHeight, h), L"Height should reflect the user resize, not the original set value");
                VERIFY_IS_FALSE(areClose(setWidth, w), L"Width should not be the superseded set value");
            });

            // On return to Default, the window is at the resized size (the OS restore).
            RunOnUIThread([&]()
            {
                window1->AppWindow->SetPresenter(Microsoft::UI::Windowing::AppWindowPresenterKind::Default);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"After return to Default: Width=%f Height=%f", window1->Width, window1->Height);
                VERIFY_IS_TRUE(areClose(resizedWidth, window1->Width), L"After return, width should be the resized size");
                VERIFY_IS_TRUE(areClose(resizedHeight, window1->Height), L"After return, height should be the resized size");
            });
        };

        runForPresenter(Microsoft::UI::Windowing::AppWindowPresenterKind::FullScreen, L"FullScreen");
        runForPresenter(Microsoft::UI::Windowing::AppWindowPresenterKind::CompactOverlay, L"CompactOverlay");
    }

    void WindowIntegrationTests::RestoreFromMaximizedWithECITBIsExact()
    {
        TestCleanupWrapper cleanup;

        // Regression test for the cached-chrome restore path. With ExtendsContentIntoTitleBar on, set
        // a restore size while the window is maximized, then restore. The actual restored client size
        // (Window.Bounds) must match the requested size. This exercises the maximized restore path,
        // which converts the requested client size to a window rect using the chrome we measured from
        // the live window while it was normal (GetWindowRect - GetClientRect, which reflects ECITB's
        // WM_NCCALCSIZE handling) rather than synthesizing chrome from window styles.
        const double initialWidth = 640.0;
        const double initialHeight = 480.0;
        const double restoreWidth = 760.0;
        const double restoreHeight = 580.0;
        const double tolerance = 2.0;
        auto areClose = [tolerance](double a, double b) { return std::abs(a - b) <= tolerance; };

        WindowAutoCloser window1;
        HWND windowHandle = nullptr;

        RunOnUIThread([&]()
        {
            window1.Attach(safe_cast<xaml::Window^>(xaml_markup::XamlReader::Load(
                L"<Window xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                L"  >"
                L"  <StackPanel x:Name='rootPanel'/>"
                L"</Window>")));
            window1->Activate();
        });
        TestServices::WindowHelper->WaitForIdle();

        // Turn ECITB on and set an initial size in the normal state. This is where the runtime
        // measures and caches the true (ECITB-aware) chrome from the live window.
        RunOnUIThread([&]()
        {
            IWindowNative* windowNative = nullptr;
            VERIFY_SUCCEEDED(reinterpret_cast<IUnknown*>(window1.get())->QueryInterface(
                __uuidof(IWindowNative), (void**)&windowNative));
            VERIFY_SUCCEEDED(windowNative->get_WindowHandle(&windowHandle));
            windowNative->Release();

            window1->ExtendsContentIntoTitleBar = true;
            window1->Width = initialWidth;
            window1->Height = initialHeight;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Normal+ECITB before maximize: Bounds=%fx%f", window1->Bounds.Width, window1->Bounds.Height);
            VERIFY_IS_TRUE(areClose(initialWidth, window1->Bounds.Width), L"Initial client width should match in normal state");
            VERIFY_IS_TRUE(areClose(initialHeight, window1->Bounds.Height), L"Initial client height should match in normal state");
        });

        // Maximize, set a new restore size while maximized, then restore.
        RunOnUIThread([&]()
        {
            ::ShowWindow(windowHandle, SW_MAXIMIZE);
            VERIFY_IS_TRUE(!!::IsZoomed(windowHandle), L"Window should be maximized");

            window1->Width = restoreWidth;
            window1->Height = restoreHeight;

            // Getter reports the restore size while maximized.
            VERIFY_IS_TRUE(areClose(restoreWidth, window1->Width), L"Getter should report restore width while maximized");
            VERIFY_IS_TRUE(areClose(restoreHeight, window1->Height), L"Getter should report restore height while maximized");
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            ::ShowWindow(windowHandle, SW_RESTORE);
            VERIFY_IS_FALSE(!!::IsZoomed(windowHandle), L"Window should be restored");
        });
        TestServices::WindowHelper->WaitForIdle();

        // The live restored client area (Bounds) must match the requested restore size exactly
        // (within rounding tolerance). If the ECITB chrome were mishandled, the window would come
        // back ~31 DIP off in height.
        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"After restore: Bounds=%fx%f Width/Height=%fx%f", window1->Bounds.Width, window1->Bounds.Height, window1->Width, window1->Height);
            VERIFY_IS_TRUE(areClose(restoreWidth, window1->Bounds.Width), L"Restored client width should match the requested restore width");
            VERIFY_IS_TRUE(areClose(restoreHeight, window1->Bounds.Height), L"Restored client height should match the requested restore height");
        });
    }

    void WindowIntegrationTests::GetWindowWidthHeightHonorsValueSetSameTurnAsPresenterSwitch()
    {
        TestCleanupWrapper cleanup;

        // Regression for the same-turn edge: an app sets Width/Height and then switches to a
        // non-sizing presenter in the *same* message-loop turn, with no intervening idle. The
        // deferred WM_SIZE capture hasn't run yet by the time the presenter changes, so the getter
        // must rely on the size the setter recorded synchronously. (The other presenter tests put a
        // WaitForIdle between the set and the switch, so they don't exercise this path.)
        const double setWidth = 540.0;
        const double setHeight = 420.0;
        const double tolerance = 2.0;
        auto areClose = [tolerance](double a, double b) { return std::abs(a - b) <= tolerance; };

        auto runForPresenter = [&](Microsoft::UI::Windowing::AppWindowPresenterKind presenterKind, const wchar_t* presenterName)
        {
            LOG_OUTPUT(L"===== Presenter: %s =====", presenterName);

            WindowAutoCloser window1;

            RunOnUIThread([&]()
            {
                window1.Attach(safe_cast<xaml::Window^>(xaml_markup::XamlReader::Load(
                    L"<Window xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                    L"  >"
                    L"  <StackPanel x:Name='rootPanel'/>"
                    L"</Window>")));
                window1->Activate();
            });
            TestServices::WindowHelper->WaitForIdle();

            // Set Width/Height AND switch presenter in the same turn - deliberately no WaitForIdle
            // between them, so the deferred WM_SIZE capture can't have run.
            RunOnUIThread([&]()
            {
                window1->Width = setWidth;
                window1->Height = setHeight;
                window1->AppWindow->SetPresenter(presenterKind);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                const double w = window1->Width;
                const double h = window1->Height;
                LOG_OUTPUT(L"%s: Width/Height=%fx%f (set was %fx%f, live Bounds=%fx%f)", presenterName, w, h, setWidth, setHeight, window1->Bounds.Width, window1->Bounds.Height);

                VERIFY_IS_TRUE(areClose(setWidth, w), L"Width should honor the value set in the same turn as the presenter switch");
                VERIFY_IS_TRUE(areClose(setHeight, h), L"Height should honor the value set in the same turn as the presenter switch");
            });
        };

        runForPresenter(Microsoft::UI::Windowing::AppWindowPresenterKind::FullScreen, L"FullScreen");
        runForPresenter(Microsoft::UI::Windowing::AppWindowPresenterKind::CompactOverlay, L"CompactOverlay");
    }

    void WindowIntegrationTests::SimulatedUserDragResizeIsTrackedForPresenter()
    {
        TestCleanupWrapper cleanup;

        // A user border-drag fires a burst of WM_SIZE between WM_ENTERSIZEMOVE and WM_EXITSIZEMOVE.
        // The runtime suppresses the per-frame restore-size capture during the drag (to avoid
        // enqueuing a dispatcher callback per frame) and captures once at WM_EXITSIZEMOVE. This test
        // simulates that bracketing and verifies the final dragged size is captured: in a non-sizing
        // presenter the getter must report the dragged size, not the pre-drag size. If the
        // WM_EXITSIZEMOVE capture were missing, the getter would report the stale pre-drag value.
        const double preDragWidth = 500.0;
        const double preDragHeight = 400.0;
        const double tolerance = 2.0;
        auto areClose = [tolerance](double a, double b) { return std::abs(a - b) <= tolerance; };

        WindowAutoCloser window1;
        HWND windowHandle = nullptr;

        RunOnUIThread([&]()
        {
            window1.Attach(safe_cast<xaml::Window^>(xaml_markup::XamlReader::Load(
                L"<Window xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                L"  >"
                L"  <StackPanel x:Name='rootPanel'/>"
                L"</Window>")));
            window1->Activate();
        });
        TestServices::WindowHelper->WaitForIdle();

        // Establish a known pre-drag tracked size.
        RunOnUIThread([&]()
        {
            IWindowNative* windowNative = nullptr;
            VERIFY_SUCCEEDED(reinterpret_cast<IUnknown*>(window1.get())->QueryInterface(
                __uuidof(IWindowNative), (void**)&windowNative));
            VERIFY_SUCCEEDED(windowNative->get_WindowHandle(&windowHandle));
            windowNative->Release();

            window1->Width = preDragWidth;
            window1->Height = preDragHeight;
        });
        TestServices::WindowHelper->WaitForIdle();

        // Simulate a user drag-resize: bracket a SetWindowPos with WM_ENTERSIZEMOVE / WM_EXITSIZEMOVE,
        // exactly as the modal move/size loop would. During the bracket the per-frame capture is
        // suppressed; the capture happens on WM_EXITSIZEMOVE.
        RunOnUIThread([&]()
        {
            ::SendMessageW(windowHandle, WM_ENTERSIZEMOVE, 0, 0);
            ::SetWindowPos(windowHandle, nullptr, 0, 0, 820, 660, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
            ::SendMessageW(windowHandle, WM_EXITSIZEMOVE, 0, 0);
        });
        TestServices::WindowHelper->WaitForIdle();

        double draggedClientW = 0, draggedClientH = 0;
        RunOnUIThread([&]()
        {
            draggedClientW = window1->Bounds.Width;
            draggedClientH = window1->Bounds.Height;
            LOG_OUTPUT(L"After simulated drag: Bounds=%fx%f (pre-drag was %fx%f)", draggedClientW, draggedClientH, preDragWidth, preDragHeight);

            // Sanity: the drag actually changed the size away from the pre-drag value.
            VERIFY_IS_FALSE(areClose(draggedClientW, preDragWidth), L"Drag should have changed the width away from the pre-drag value");
        });

        RunOnUIThread([&]()
        {
            window1->AppWindow->SetPresenter(Microsoft::UI::Windowing::AppWindowPresenterKind::FullScreen);
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            const double w = window1->Width;
            const double h = window1->Height;
            LOG_OUTPUT(L"In FullScreen after drag: Width/Height=%fx%f (dragged was %fx%f)", w, h, draggedClientW, draggedClientH);

            VERIFY_IS_TRUE(areClose(draggedClientW, w), L"Width in presenter should reflect the dragged size captured at WM_EXITSIZEMOVE");
            VERIFY_IS_TRUE(areClose(draggedClientH, h), L"Height in presenter should reflect the dragged size captured at WM_EXITSIZEMOVE");
            VERIFY_IS_FALSE(areClose(preDragWidth, w), L"Width should not be the stale pre-drag value");
        });
    }

    void WindowIntegrationTests::MinMaxSizeGetSet()
    {
        TestCleanupWrapper cleanup;

        WindowAutoCloser window1;

        RunOnUIThread([&]()
        {
            window1.Attach(safe_cast<xaml::Window^>(xaml_markup::XamlReader::Load(
                L"<Window xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <StackPanel x:Name='rootPanel'/>"
                L"</Window>")));

            window1->Activate();
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"----- Check the liberal defaults -----");
            VERIFY_ARE_EQUAL(0.0, window1->MinWidth);
            VERIFY_ARE_EQUAL(0.0, window1->MinHeight);
            VERIFY_IS_TRUE(std::isinf(window1->MaxWidth) && window1->MaxWidth > 0.0);
            VERIFY_IS_TRUE(std::isinf(window1->MaxHeight) && window1->MaxHeight > 0.0);

            LOG_OUTPUT(L"----- Set and read back valid values -----");
            window1->MinWidth = 320.0;
            window1->MinHeight = 240.0;
            window1->MaxWidth = 1600.0;
            window1->MaxHeight = 1200.0;

            VERIFY_ARE_EQUAL(320.0, window1->MinWidth);
            VERIFY_ARE_EQUAL(240.0, window1->MinHeight);
            VERIFY_ARE_EQUAL(1600.0, window1->MaxWidth);
            VERIFY_ARE_EQUAL(1200.0, window1->MaxHeight);

            LOG_OUTPUT(L"----- Min=0 removes the minimum, Max=+Infinity removes the maximum -----");
            window1->MinWidth = 0.0;
            window1->MaxWidth = std::numeric_limits<double>::infinity();
            VERIFY_ARE_EQUAL(0.0, window1->MinWidth);
            VERIFY_IS_TRUE(std::isinf(window1->MaxWidth) && window1->MaxWidth > 0.0);

            LOG_OUTPUT(L"----- Min wins over max for effective sizing, but both stored values are preserved -----");
            window1->MinWidth = 500.0;
            window1->MaxWidth = 300.0;
            VERIFY_ARE_EQUAL(500.0, window1->MinWidth);
            VERIFY_ARE_EQUAL(300.0, window1->MaxWidth);
        });
    }

    void WindowIntegrationTests::MinMaxSizeInvalidValuesThrow()
    {
        TestCleanupWrapper cleanup;

        WindowAutoCloser window1;

        RunOnUIThread([&]()
        {
            window1.Attach(safe_cast<xaml::Window^>(xaml_markup::XamlReader::Load(
                L"<Window xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <StackPanel x:Name='rootPanel'/>"
                L"</Window>")));

            window1->Activate();
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            const double nan = std::numeric_limits<double>::quiet_NaN();
            const double posInf = std::numeric_limits<double>::infinity();
            const double negInf = -std::numeric_limits<double>::infinity();

            LOG_OUTPUT(L"----- Min rejects negative, NaN, and both infinities -----");
            VERIFY_THROWS_WINRT(window1->MinWidth = -1.0, Platform::InvalidArgumentException^);
            VERIFY_THROWS_WINRT(window1->MinHeight = nan, Platform::InvalidArgumentException^);
            VERIFY_THROWS_WINRT(window1->MinWidth = posInf, Platform::InvalidArgumentException^);
            VERIFY_THROWS_WINRT(window1->MinHeight = negInf, Platform::InvalidArgumentException^);

            LOG_OUTPUT(L"----- Max rejects negative, NaN, and negative infinity, but allows positive infinity -----");
            VERIFY_THROWS_WINRT(window1->MaxWidth = -1.0, Platform::InvalidArgumentException^);
            VERIFY_THROWS_WINRT(window1->MaxHeight = nan, Platform::InvalidArgumentException^);
            VERIFY_THROWS_WINRT(window1->MaxWidth = negInf, Platform::InvalidArgumentException^);
            window1->MaxHeight = posInf; // no throw

            LOG_OUTPUT(L"----- A rejected set must not change the stored value -----");
            window1->MinWidth = 200.0;
            VERIFY_THROWS_WINRT(window1->MinWidth = -5.0, Platform::InvalidArgumentException^);
            VERIFY_ARE_EQUAL(200.0, window1->MinWidth);
        });
    }

    void WindowIntegrationTests::SetMinMaxSizeInMarkup()
    {
        TestCleanupWrapper cleanup;

        WindowAutoCloser window1;

        RunOnUIThread([&]()
        {
            window1.Attach(safe_cast<xaml::Window^>(xaml_markup::XamlReader::Load(
                L"<Window xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                L"  MinWidth='320' MinHeight='240' MaxWidth='1600' MaxHeight='1200'>"
                L"  <StackPanel x:Name='rootPanel'/>"
                L"</Window>")));

            window1->Activate();
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"----- Check the Min/Max size properties set in markup -----");
            VERIFY_ARE_EQUAL(320.0, window1->MinWidth);
            VERIFY_ARE_EQUAL(240.0, window1->MinHeight);
            VERIFY_ARE_EQUAL(1600.0, window1->MaxWidth);
            VERIFY_ARE_EQUAL(1200.0, window1->MaxHeight);
        });
    }

    void WindowIntegrationTests::MinMaxSizeSurvivesPresenterSwap()
    {
        using namespace Microsoft::UI::Windowing;

        TestCleanupWrapper cleanup;

        WindowAutoCloser window1;

        RunOnUIThread([&]()
        {
            window1.Attach(safe_cast<xaml::Window^>(xaml_markup::XamlReader::Load(
                L"<Window xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <StackPanel x:Name='rootPanel'/>"
                L"</Window>")));

            window1->Activate();
        });
        TestServices::WindowHelper->WaitForIdle();

        // Helper: fetch the window's current presenter as an OverlappedPresenter (or null if it's some other kind).
        auto currentOverlapped = [&]() -> OverlappedPresenter^
        {
            AppWindow^ appWindow = window1->AppWindow;
            VERIFY_IS_NOT_NULL(appWindow);
            return dynamic_cast<OverlappedPresenter^>(appWindow->Presenter);
        };

        // Captured from the baseline presenter so we can confirm the *same* pixel value survives the
        // presenter churn (not just that some non-null value came back).
        int baselineMinWidthPx = 0;
        int baselineMaxWidthPx = 0;

        // A window starts life with an OverlappedPresenter. Setting a constraint while overlapped should
        // push straight through to that presenter (no swap needed).
        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"----- Baseline: constraints set while overlapped land on the overlapped presenter -----");
            OverlappedPresenter^ op = currentOverlapped();
            VERIFY_IS_NOT_NULL(op); // a fresh window is overlapped

            window1->MinWidth = 400.0;
            window1->MaxWidth = 900.0;

            VERIFY_IS_NOT_NULL(op->PreferredMinimumWidth);
            VERIFY_IS_NOT_NULL(op->PreferredMaximumWidth);

            baselineMinWidthPx = op->PreferredMinimumWidth->Value;
            baselineMaxWidthPx = op->PreferredMaximumWidth->Value;
        });
        TestServices::WindowHelper->WaitForIdle();

        // Direction 1: overlapped -> compact overlay. There is no OverlappedPresenter now, so a constraint
        // we set here has to be *stored* and applied later. Then come back to overlapped and confirm both the
        // old (MinWidth/MaxWidth) and the new (MinHeight) constraints show up on the (brand new) presenter.
        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"----- overlapped -> compact overlay: set a constraint while deferred -----");
            AppWindow^ appWindow = window1->AppWindow;
            appWindow->SetPresenter(AppWindowPresenterKind::CompactOverlay);

            // No overlapped presenter right now - this value is stored, not pushed.
            VERIFY_IS_NULL(currentOverlapped());
            window1->MinHeight = 300.0;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"----- compact overlay -> overlapped: deferred constraints get re-applied -----");
            AppWindow^ appWindow = window1->AppWindow;
            appWindow->SetPresenter(AppWindowPresenterKind::Overlapped);
        });
        // The re-apply happens on the AppWindow.Changed callback, which is raised while the message queue
        // drains - so wait for idle before reading the presenter back.
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            OverlappedPresenter^ op = currentOverlapped();
            VERIFY_IS_NOT_NULL(op);
            VERIFY_IS_NOT_NULL(op->PreferredMinimumWidth);  // from the baseline, re-applied onto the new presenter
            VERIFY_IS_NOT_NULL(op->PreferredMaximumWidth);
            VERIFY_IS_NOT_NULL(op->PreferredMinimumHeight); // set while in compact overlay, applied on the way back

            // It's not just non-null - it's the *same* pixel value we saw on the original presenter. DPI is
            // constant across the swap, so the pixel constraint should round-trip exactly.
            VERIFY_ARE_EQUAL(baselineMinWidthPx, op->PreferredMinimumWidth->Value);
            VERIFY_ARE_EQUAL(baselineMaxWidthPx, op->PreferredMaximumWidth->Value);

            // The public getters report the stored DIP values, untouched by all the presenter churn.
            VERIFY_ARE_EQUAL(400.0, window1->MinWidth);
            VERIFY_ARE_EQUAL(300.0, window1->MinHeight);
            VERIFY_ARE_EQUAL(900.0, window1->MaxWidth);
        });
        TestServices::WindowHelper->WaitForIdle();

        // Direction 2: overlapped -> full screen -> overlapped, without touching any constraint in between.
        // The round trip must leave the constraints intact (idempotent re-apply).
        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"----- overlapped -> full screen -----");
            AppWindow^ appWindow = window1->AppWindow;
            appWindow->SetPresenter(AppWindowPresenterKind::FullScreen);
            VERIFY_IS_NULL(currentOverlapped());
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"----- full screen -> overlapped: constraints survive the round trip -----");
            AppWindow^ appWindow = window1->AppWindow;
            appWindow->SetPresenter(AppWindowPresenterKind::Overlapped);
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            OverlappedPresenter^ op = currentOverlapped();
            VERIFY_IS_NOT_NULL(op);
            VERIFY_IS_NOT_NULL(op->PreferredMinimumWidth);
            VERIFY_IS_NOT_NULL(op->PreferredMinimumHeight);
            VERIFY_IS_NOT_NULL(op->PreferredMaximumWidth);
        });
        TestServices::WindowHelper->WaitForIdle();

        // Direction 3: clearing constraints (Min=0, Max=+Infinity) while a non-overlapped presenter is active
        // must also be honored when we come back - i.e. the presenter ends up with no min/max.
        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"----- overlapped -> compact overlay: clear the width constraints while deferred -----");
            AppWindow^ appWindow = window1->AppWindow;
            appWindow->SetPresenter(AppWindowPresenterKind::CompactOverlay);

            window1->MinWidth = 0.0;                                        // remove the minimum
            window1->MaxWidth = std::numeric_limits<double>::infinity();    // remove the maximum
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            AppWindow^ appWindow = window1->AppWindow;
            appWindow->SetPresenter(AppWindowPresenterKind::Overlapped);
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"----- compact overlay -> overlapped: cleared width constraints stay cleared, height stays set -----");
            OverlappedPresenter^ op = currentOverlapped();
            VERIFY_IS_NOT_NULL(op);
            VERIFY_IS_NULL(op->PreferredMinimumWidth);      // cleared
            VERIFY_IS_NULL(op->PreferredMaximumWidth);      // cleared
            VERIFY_IS_NOT_NULL(op->PreferredMinimumHeight); // never cleared

            VERIFY_ARE_EQUAL(0.0, window1->MinWidth);
            VERIFY_ARE_EQUAL(300.0, window1->MinHeight);
        });
    }

    void WindowIntegrationTests::MinMaxSizeClampsWindowSize()
    {
        using namespace Microsoft::UI::Windowing;

        TestCleanupWrapper cleanup;

        WindowAutoCloser window1;

        RunOnUIThread([&]()
        {
            window1.Attach(safe_cast<xaml::Window^>(xaml_markup::XamlReader::Load(
                L"<Window xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <StackPanel x:Name='rootPanel'/>"
                L"</Window>")));

            window1->Activate();
        });
        TestServices::WindowHelper->WaitForIdle();

        // Minimum: asking for a window far smaller than the minimum must be clamped back up.
        // We read the presenter's pushed value (physical, outer-window pixels) and compare against it,
        // so the check is DPI-independent - it works at 100%, 150%, 200%, whatever.
        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"----- Shrinking below MinWidth/MinHeight is clamped up -----");
            AppWindow^ appWindow = window1->AppWindow;
            VERIFY_IS_NOT_NULL(appWindow);

            window1->MinWidth = 600.0;
            window1->MinHeight = 500.0;

            OverlappedPresenter^ op = dynamic_cast<OverlappedPresenter^>(appWindow->Presenter);
            VERIFY_IS_NOT_NULL(op);
            VERIFY_IS_NOT_NULL(op->PreferredMinimumWidth);
            VERIFY_IS_NOT_NULL(op->PreferredMinimumHeight);
            const int minW = op->PreferredMinimumWidth->Value;
            const int minH = op->PreferredMinimumHeight->Value;

            ::Windows::Graphics::SizeInt32 tiny;
            tiny.Width = 50;
            tiny.Height = 50;
            appWindow->Resize(tiny);

            ::Windows::Graphics::SizeInt32 sz = appWindow->Size;
            LOG_OUTPUT(L"    requested 50x50 -> got %dx%d (min is %dx%d)", sz.Width, sz.Height, minW, minH);

            // A small rounding slack keeps this off the razor's edge without letting a real 50px result pass.
            VERIFY_IS_GREATER_THAN_OR_EQUAL(sz.Width, minW - 4);
            VERIFY_IS_GREATER_THAN_OR_EQUAL(sz.Height, minH - 4);
        });
        TestServices::WindowHelper->WaitForIdle();

        // Maximum: asking for a window far larger than the maximum must be clamped back down.
        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"----- Growing beyond MaxWidth/MaxHeight is clamped down -----");
            AppWindow^ appWindow = window1->AppWindow;

            // Drop the minimum so it doesn't fight the maximum, then set a small maximum.
            window1->MinWidth = 0.0;
            window1->MinHeight = 0.0;
            window1->MaxWidth = 400.0;
            window1->MaxHeight = 350.0;

            OverlappedPresenter^ op = dynamic_cast<OverlappedPresenter^>(appWindow->Presenter);
            VERIFY_IS_NOT_NULL(op);
            VERIFY_IS_NOT_NULL(op->PreferredMaximumWidth);
            VERIFY_IS_NOT_NULL(op->PreferredMaximumHeight);
            const int maxW = op->PreferredMaximumWidth->Value;
            const int maxH = op->PreferredMaximumHeight->Value;

            ::Windows::Graphics::SizeInt32 huge;
            huge.Width = 5000;
            huge.Height = 5000;
            appWindow->Resize(huge);

            ::Windows::Graphics::SizeInt32 sz = appWindow->Size;
            LOG_OUTPUT(L"    requested 5000x5000 -> got %dx%d (max is %dx%d)", sz.Width, sz.Height, maxW, maxH);

            // It clearly didn't honor 5000, and it landed at (about) the maximum we set.
            VERIFY_IS_LESS_THAN(sz.Width, 5000);
            VERIFY_IS_LESS_THAN(sz.Height, 5000);
            VERIFY_IS_LESS_THAN_OR_EQUAL(sz.Width, maxW + 4);
            VERIFY_IS_LESS_THAN_OR_EQUAL(sz.Height, maxH + 4);
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    void WindowIntegrationTests::MinMaxSizeLeavesUnsetConstraintsAlone()
    {
        using namespace Microsoft::UI::Windowing;

        TestCleanupWrapper cleanup;

        WindowAutoCloser window1;

        RunOnUIThread([&]()
        {
            window1.Attach(safe_cast<xaml::Window^>(xaml_markup::XamlReader::Load(
                L"<Window xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <StackPanel x:Name='rootPanel'/>"
                L"</Window>")));

            window1->Activate();
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"----- Setting one Window constraint must not disturb the presenter constraints the app manages directly -----");
            AppWindow^ appWindow = window1->AppWindow;
            VERIFY_IS_NOT_NULL(appWindow);

            OverlappedPresenter^ op = dynamic_cast<OverlappedPresenter^>(appWindow->Presenter);
            VERIFY_IS_NOT_NULL(op);

            // The app configures a couple of presenter constraints directly, bypassing the Window API entirely.
            op->PreferredMinimumHeight = ref new Platform::Box<int>(700);
            op->PreferredMaximumHeight = ref new Platform::Box<int>(1500);

            // The app opts into exactly one Window constraint.
            window1->MinWidth = 400.0;

            // We applied the property the app set through us...
            VERIFY_IS_NOT_NULL(op->PreferredMinimumWidth);

            // ...and left the ones it manages directly completely untouched (not overwritten with null).
            VERIFY_IS_NOT_NULL(op->PreferredMinimumHeight);
            VERIFY_ARE_EQUAL(700, op->PreferredMinimumHeight->Value);
            VERIFY_IS_NOT_NULL(op->PreferredMaximumHeight);
            VERIFY_ARE_EQUAL(1500, op->PreferredMaximumHeight->Value);

            // MaxWidth was never set through the Window API, so we never wrote it.
            VERIFY_IS_NULL(op->PreferredMaximumWidth);
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    void WindowIntegrationTests::MinMaxSizeTracksTitleBarToggle()
    {
        using namespace Microsoft::UI::Windowing;

        TestCleanupWrapper cleanup;

        WindowAutoCloser window1;

        RunOnUIThread([&]()
        {
            window1.Attach(safe_cast<xaml::Window^>(xaml_markup::XamlReader::Load(
                L"<Window xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <StackPanel x:Name='rootPanel'/>"
                L"</Window>")));

            window1->Activate();
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"----- Constraints re-apply when ExtendsContentIntoTitleBar toggles -----");
            AppWindow^ appWindow = window1->AppWindow;
            VERIFY_IS_NOT_NULL(appWindow);
            OverlappedPresenter^ op = dynamic_cast<OverlappedPresenter^>(appWindow->Presenter);
            VERIFY_IS_NOT_NULL(op);

            // Start with the standard title bar and a minimum height constraint. The constraint is a
            // *client-area* size in DIPs; we push it to the presenter as an *outer-window* pixel value,
            // which folds in the non-client chrome (caption + borders).
            window1->ExtendsContentIntoTitleBar = false;
            window1->MinHeight = 500.0;
            VERIFY_IS_NOT_NULL(op->PreferredMinimumHeight);
            const int minHeightStandardChrome = op->PreferredMinimumHeight->Value;

            // Enabling ExtendsContentIntoTitleBar folds the caption into the client area, so the
            // non-client chrome shrinks. The same 500-DIP client minimum must now map to a smaller
            // outer-window pixel minimum. If we didn't re-apply on the toggle this value would be stale
            // (unchanged), leaving the enforced minimum off by about the caption height.
            window1->ExtendsContentIntoTitleBar = true;
            VERIFY_IS_NOT_NULL(op->PreferredMinimumHeight);
            const int minHeightExtendedChrome = op->PreferredMinimumHeight->Value;

            LOG_OUTPUT(L"    presenter min height: standard chrome=%d, extended chrome=%d", minHeightStandardChrome, minHeightExtendedChrome);

            // The constraint tracked the chrome change instead of drifting.
            VERIFY_IS_LESS_THAN(minHeightExtendedChrome, minHeightStandardChrome);
        });
        TestServices::WindowHelper->WaitForIdle();
    }

#endif // MUX_PRERELEASE


} } } } } } // Microsoft::UI::Xaml::Tests::Controls::Window
