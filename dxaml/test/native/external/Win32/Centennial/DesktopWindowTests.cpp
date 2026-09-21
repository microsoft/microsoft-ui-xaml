// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "DesktopWindowTests.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include <ppltasks.h>
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>
#include <WindowAutoCloser.h>
#include <microsoft.ui.xaml.window.h>

using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Input;

using namespace test_infra;

namespace Microsoft::UI::Xaml::Tests::DesktopWindow {

        bool DesktopWindowTests::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();
            return true;
        }

        bool DesktopWindowTests::TestSetup()
        {
            test_infra::TestServices::WindowHelper->InitializeXaml();
            return true;
        }

        bool DesktopWindowTests::TestCleanup()
        {
            test_infra::TestServices::WindowHelper->ShutdownXaml();
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        void DesktopWindowTests::ValidateDesktopWindowLifeTime()
        {
            TestCleanupWrapper cleanup;

            Button^ btn = nullptr;
            RunOnUIThread([&]()
                {
                    StackPanel^ mainStackPanel = ref new StackPanel();
                    btn = ref new Button();
                    btn->Width = 150;
                    btn->Height = 50;
                    btn->Content = "Button";
                    btn->HorizontalAlignment = HorizontalAlignment::Center;
                    mainStackPanel->Children->Append(btn);
                    void* rawWinPtr = nullptr; // don't care what is in address.

                    // This scope will make desktopWindow to get released. However, as now we have pegged window, window will be still alive
                    {
                        Window^ desktopWindow = ref new Window();
                        desktopWindow->Content = mainStackPanel;

                        // Get the address of window pointed by hat pointer before we release it
                        rawWinPtr = reinterpret_cast<void*>(safe_cast<Platform::Object^>(desktopWindow));
                        desktopWindow->Activate();
                        desktopWindow = nullptr; 
                    }

                    // Close the desktop window
                    Window^ hatWinPtr = reinterpret_cast<Window^>(rawWinPtr);
                    hatWinPtr->Close();
                });
            TestServices::WindowHelper->WaitForIdle();
        }

        static void VerifyWindowRedirectionSurface(bool changeEnabled)
        {
            TestCleanupWrapper cleanup;
            WindowAutoCloser window;

            RunOnUIThread([&]()
            {
                const auto changeId = xaml_settings::XamlChangeId::SkipWindowRedirectionSurface;
                VERIFY_ARE_EQUAL(changeEnabled, xaml_settings::XamlOptionalChanges::IsChangeEnabled(changeId));

                const auto compositor = xaml_media::CompositionTarget::GetCompositorForCurrentThread();
                VERIFY_IS_NOT_NULL(compositor);
                const bool isSystemCompositor =
                    Microsoft::UI::Composition::CompositionEngine::GetForSystemEngine(compositor) != nullptr;
                const bool skipRedirectionSurface = changeEnabled && isSystemCompositor;
                LOG_OUTPUT(L"Using the %s compositor.", isSystemCompositor ? L"system" : L"lifted");

                window.Attach(ref new Window());
                wrl::ComPtr<IWindowNative> windowNative;
                VERIFY_SUCCEEDED(reinterpret_cast<IUnknown*>(window.get())->QueryInterface(IID_PPV_ARGS(&windowNative)));

                HWND windowHandle = nullptr;
                VERIFY_SUCCEEDED(windowNative->get_WindowHandle(&windowHandle));
                VERIFY_IS_NOT_NULL(windowHandle);

                wil::unique_hdc memoryDC(::CreateCompatibleDC(nullptr));
                VERIFY_IS_NOT_NULL(memoryDC.get());
                wil::unique_hbitmap bitmap(::CreateBitmap(1, 1, 1, 1, nullptr));
                VERIFY_IS_NOT_NULL(bitmap.get());
                const auto previousBitmap = ::SelectObject(memoryDC.get(), bitmap.get());
                VERIFY_IS_NOT_NULL(previousBitmap);
                VERIFY_ARE_NOT_EQUAL(HGDI_ERROR, previousBitmap);
                auto restoreBitmap = wil::scope_exit([&]()
                {
                    ::SelectObject(memoryDC.get(), previousBitmap);
                });

                auto verifyWindowState = [&]()
                {
                    const bool hasNoRedirectionBitmap =
                        (::GetWindowLongPtrW(windowHandle, GWL_EXSTYLE) & WS_EX_NOREDIRECTIONBITMAP) != 0;
                    VERIFY_ARE_EQUAL(skipRedirectionSurface, hasNoRedirectionBitmap);

                    auto eraseBackground = [&](COLORREF initialColor)
                    {
                        VERIFY_IS_TRUE(!!::SetPixelV(memoryDC.get(), 0, 0, initialColor));
                        VERIFY_ARE_EQUAL(
                            static_cast<LRESULT>(1),
                            ::SendMessageW(windowHandle, WM_ERASEBKGND, reinterpret_cast<WPARAM>(memoryDC.get()), 0));
                        const auto color = ::GetPixel(memoryDC.get(), 0, 0);
                        VERIFY_ARE_NOT_EQUAL(CLR_INVALID, color);
                        return color;
                    };

                    // Two starting colors distinguish a themed fill from no painting, regardless of the theme.
                    const auto fromBlack = eraseBackground(RGB(0, 0, 0));
                    const auto fromWhite = eraseBackground(RGB(255, 255, 255));
                    if (skipRedirectionSurface)
                    {
                        VERIFY_ARE_EQUAL(RGB(0, 0, 0), fromBlack);
                        VERIFY_ARE_EQUAL(RGB(255, 255, 255), fromWhite);
                    }
                    else
                    {
                        VERIFY_ARE_EQUAL(fromBlack, fromWhite);
                    }
                };

                verifyWindowState();

                TestServices::Utilities->ResetOptionalChanges();
                auto restoreLock = wil::scope_exit([]()
                {
                    xaml_settings::XamlOptionalChanges::Lock();
                });
                if (!changeEnabled)
                {
                    VERIFY_IS_TRUE(xaml_settings::XamlOptionalChanges::EnableChange(changeId));
                }
                VERIFY_ARE_EQUAL(!changeEnabled, xaml_settings::XamlOptionalChanges::IsChangeEnabled(changeId));

                verifyWindowState();
            });
        }

        void DesktopWindowTests::ValidateDefaultRedirectionSurface()
        {
            VerifyWindowRedirectionSurface(false);
        }

        void DesktopWindowTests::ValidateOptedInRedirectionSurface()
        {
            VerifyWindowRedirectionSurface(true);
        }

}
