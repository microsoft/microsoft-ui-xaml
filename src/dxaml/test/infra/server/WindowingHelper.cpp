// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <objbase.h>
#include <winuser.h>
#include "WindowingHelper.h"
#include "UtilitiesRoutineHelper.h"
#include "InputRoutineHelper.h"

#include <wil\resource.h>
#include <Wincodec.h>
#include <ScreenCapture.h>

using namespace WEX::Common;
using namespace WEX::TestExecution;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::WRL;
using namespace WEX::Logging;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Common {

        namespace ScreenCapture {

            // Here is the actual implementation for TakeScreenshot.
            void TakeScreenshot(const wchar_t* filename)
            {
                Log::Trace(CommentTrace{ filename }.WithAttachment(LogTraceAttachment::ScreenCapture));
            }
        }

        // Finds the HWND of the test app process by matching its title text against the known
        // window titles for the test app.
        BOOL CALLBACK WindowingHelper::FindTopLevelForegroundWindow(_In_ HWND hwnd, _In_ LPARAM lParam)
        {
            // Most tests will have this for their window title:
            const static wchar_t windowTitle[] = L"TAEF Tailored Application Host Process";

            // Design mode tests use this magic string for the title:
            const static wchar_t windowTitleDesignMode[] = L"NoUIEntryPoints-DesignModeV2";

            // Name of the window class for Core Windows
            const static wchar_t coreWindowClassName[] = L"Windows.UI.Core.CoreWindow";

            BOOL continueSearching = TRUE;
            auto result = reinterpret_cast<HWND*>(lParam);

            wchar_t actualWindowTitle[256];
            static_assert(ARRAYSIZE(actualWindowTitle) > ARRAYSIZE(windowTitle));
            static_assert(ARRAYSIZE(actualWindowTitle) > ARRAYSIZE(windowTitleDesignMode));

            if (::GetWindowTextW(hwnd, actualWindowTitle, ARRAYSIZE(actualWindowTitle)))
            {
                if ((::CompareStringOrdinal(actualWindowTitle, -1, windowTitle, -1, TRUE /*bIgnoreCase*/) == CSTR_EQUAL))
                {
                    *result = hwnd;
                    // We run under a number of different scenarios.  In one of those scenarios we have multiple windows captioned
                    // with "TAEF Tailored Application Host Process" because CoreWindow appears to assume the title of the host.
                    // However, in other hosted scenarios we only have the one "TAEF Tailored Application Host Process", so what we
                    // will do is continue looking if we find a core window.  If we find something else we will use it and if
                    // we don't we will end up using the core window.
                    wchar_t actualWindowClassName[256];
                    static_assert(ARRAYSIZE(actualWindowClassName) > ARRAYSIZE(coreWindowClassName));
                    if (::GetClassName(hwnd, actualWindowClassName, ARRAYSIZE(actualWindowClassName)) &&
                        ::CompareStringOrdinal(actualWindowClassName, -1, coreWindowClassName, -1, TRUE /*bIgnoreCase*/) != CSTR_EQUAL)
                    {
                        continueSearching = FALSE;
                    }
                }
                else if (::CompareStringOrdinal(actualWindowTitle, -1, windowTitleDesignMode, -1, TRUE /*bIgnoreCase*/) == CSTR_EQUAL)
                {
                *result = hwnd;
                continueSearching = FALSE;
                }
            }

            return continueSearching;
        }

        HWND WindowingHelper::GetTopLevelForegroundWindow()
        {
            HWND hForegroundWindow = nullptr;
            LOG_OUTPUT(L"Finding top level foreground Window...");
            // This function will find the first application frame
            ::EnumWindows(WindowingHelper::FindTopLevelForegroundWindow,  reinterpret_cast<LPARAM>(&hForegroundWindow));
            if (hForegroundWindow == nullptr)
            {
                LOG_WARNING_SC(L"Could not find any ApplicationHost window");
            }
            else
            {
                LOG_OUTPUT(L"Found top level foreground Window");
            }
            return hForegroundWindow;
        }

        void WindowingHelper::MaximizeDesktopWindow(HWND handle)
        {
            if (handle == nullptr)
            {
                handle = WindowingHelper::GetTopLevelForegroundWindow();
            }

            SetDesktopWindowSize(handle, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
            Throw::LastErrorIfFalse(!!::ShowWindow(handle, SW_SHOWMAXIMIZED), L"MaximizeDesktopWindow: ShowWindow failed on the desktop window.");
        }

        void WindowingHelper::GetRightsAndSetForegroundWindow(HWND handle)
        {
            if (handle == nullptr)
            {
                handle = WindowingHelper::GetTopLevelForegroundWindow();
            }

            if (handle == nullptr)
            {
                LOG_WARNING_SC(L"SetForegroundWindow: Could not find window, ignore and continue");
                return;
            }

            // Try to set the foreground window
            BOOL success = ::SetForegroundWindow(handle);
            DWORD dwSetForegroundError = ERROR_SUCCESS;

            if(!success)
            {
                dwSetForegroundError = GetLastError();
                LOG_WARNING_SC(L"SetForegroundWindow: Failed to set the foreground window: GLE:0x%x.", dwSetForegroundError);

                // If failed the first time, then
                // send a keypress from our process. The act of sending or receiving input gives you
                // foreground rights, so this is necessary to ensure that we have such rights, which
                // are needed to make the tailored test window the foreground window.
                // There maybe delay from when we call SendInput to successful return from SetForegroundWindow, so we
                // wait and retry for 10 seconds if SetForegroundWindow call keeps returning ERROR_CANT_OPEN_ANONYMOUS
                if (dwSetForegroundError == ERROR_INVALID_WINDOW_HANDLE)
                {
                    success = TRUE;
                    LOG_WARNING_SC(L"SetForegroundWindow: Failed to with invalid window handle, Window has been closed");
                }
                else
                {
                    LOG_OUTPUT(L"Sending key input to obtain foreground rights");
                    INPUT ip = {};
                    ip.type = INPUT_KEYBOARD;
                    ip.ki.wVk = 0x41; // Keycode for 'a'
                    Throw::LastErrorIfFalse(!!::SendInput(1 /* number of INPUT structs in array */, &ip, sizeof(ip)));
                    ip.ki.dwFlags = KEYEVENTF_KEYUP; // KEYEVENTF_KEYUP for key release
                    Throw::LastErrorIfFalse(!!::SendInput(1 /* number of INPUT structs in array */, &ip, sizeof(ip)));

                    const auto inputDevice = InputDevice::Mouse;
                    const POINT pt = { 40, 40 };
                    const auto durationMs = 32;
                    const auto pressCount = 1;
                    const auto pressDeltaMs = 0;

                    // Retry SetForegroundWindow for several times with delay, it may take several seconds for it to work after injecting keypress
                    const int setForegroundWindowRetry = 10; // retry to maximum 10 seconds
                    for (int retry = 0; retry < setForegroundWindowRetry; retry++)
                    {
                        InputRoutineHelper::InjectPress(inputDevice, pt, durationMs, pressCount, pressDeltaMs);
                        Sleep(1000);
                        success = ::SetForegroundWindow(handle);
                        if (!success)
                        {
                            dwSetForegroundError = GetLastError();
                            LOG_WARNING_SC(L"SetForegroundWindow: Failed to set the foreground window: GLE:0x%x.", dwSetForegroundError);
                        }
                        else
                        {
                            break;
                        }
                    }
                }
            }

            if (!success)
            {
                // Capture screen shot before throw exception
                LOG_ERROR_SC(L"SetForegroundWindow failed:0x%x", dwSetForegroundError);
                Throw::IfFalse(success, HRESULT_FROM_WIN32(dwSetForegroundError), L"SetForegroundWindow: Failed to set the foreground window");
            }

            LOG_OUTPUT(L"::SetForegroundWindow succeeded");
        }

        void WindowingHelper::SetDesktopWindowSize(HWND handle, unsigned int width, unsigned int height)
        {
            if (handle == nullptr)
            {
                handle = GetTopLevelForegroundWindow();
            }

            Throw::LastErrorIfFalse(!!::ShowWindow(handle, SW_SHOWNORMAL), L"SetDesktopWindowSize: Failed to call ShowWindow on the app window.");
            Throw::LastErrorIfFalse(!!::SetWindowPos(handle, 0, 0, 0, width, height, SWP_NOMOVE|SWP_SHOWWINDOW), L"SetDesktopWindowSize: Failed to resize the foreground window.");
        }

        void WindowingHelper::MoveDesktopWindow(HWND handle, unsigned int x, unsigned int y)
        {
            if (handle == nullptr)
            {
                handle = GetTopLevelForegroundWindow();
            }

            Throw::LastErrorIfFalse(!!::ShowWindow(handle, SW_SHOWNORMAL), L"SetDesktopWindowSize: Failed to call ShowWindow on the app window.");
            Throw::LastErrorIfFalse(!!::SetWindowPos(handle, 0, x, y, 0, 0, SWP_NOSIZE|SWP_SHOWWINDOW), L"SetDesktopWindowSize: Failed to resize the foreground window.");
        }

        BOOL WindowingHelper::IsDesktopWindowMaximized(HWND hwnd)
        {
            WINDOWPLACEMENT placement;
            ZeroMemory(&placement, sizeof(WINDOWPLACEMENT));
            placement.length = sizeof(WINDOWPLACEMENT);
            Throw::LastErrorIfFalse(!!::GetWindowPlacement(hwnd, &placement), L"IsDesktopWindowMaximized: GetWindowPlacement failed.");

            return (placement.showCmd == SW_SHOWMAXIMIZED);
        }
    }
} } } }

