// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <TestEvent.h>
#include <wil\resource.h>
#include <ppltasks.h>
#include <chrono>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Common {

    class WindowWrapper
    {
    private:
        // When Jupiter is running without a CoreDispatcher in presenter mode it relies on the
        // client code to pump window messages.This main loop pumps messages and watches for
        // an event to be set to signal we should stop processing.
        void RunMessageLoop(const std::shared_ptr<Event>& windowSyncEvent)
        {
            MSG msg;
            int result;
            bool closed = false;
            while ((result = GetMessage(&msg, NULL, 0, 0)) != 0)
            {
                if (-1 == result)
                {
                    WEX::Common::Throw::LastError(L"GetMessage failed in WindowWrapper message loop");
                }

                TranslateMessage(&msg);
                DispatchMessage(&msg);

                if (!closed && windowSyncEvent->HasFired())
                {
                    LOG_OUTPUT(L"window sync event signaled");
                    // post close windowmessage`
                    PostMessage(m_hwnd.get(), WM_CLOSE, NULL, NULL);
                    closed = true;
                }
            }
        }

        // This method processes an individual window message. In this simple loop we specially
        // handle the Activate, Size, and Destroy messages to ensure our standard Win32 HWND
        // plays nicely with Windows. All the other messages are eventually routed to Jupiter's subclassed HWND.
        static LRESULT CALLBACK WindowProc(_In_ HWND hwnd, UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam)
        {
            switch (uMsg)
            {
            case WM_ACTIVATE:
            {
                if (LOWORD(wParam) != WA_INACTIVE)
                {
                    HWND hwndChild = reinterpret_cast<HWND>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
                    SetFocus(hwndChild);
                    return 0;
                }
                break;
            }

            case WM_SIZE:
                RECT rect;
                if (GetClientRect(hwnd, &rect))
                {
                    HWND hwndc = reinterpret_cast<HWND>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
                    SetWindowPos(hwndc, NULL, rect.left, rect.top, rect.left + rect.right, rect.top + rect.bottom, 0);
                }
                return 0;

            case WM_CLOSE:
            case WM_DESTROY:
                PostQuitMessage(0);
                return 0;
            }

            return DefWindowProc(hwnd, uMsg, wParam, lParam);
        }

        wil::unique_hwnd m_hwnd;

    public:
        WindowWrapper()
        {
            WNDCLASSEX wcex = { 0 };
            wcex.cbSize = sizeof(WNDCLASSEX);
            wcex.style = CS_HREDRAW | CS_VREDRAW;
            wcex.lpfnWndProc = WindowProc;
            wcex.hInstance = GetModuleHandle(NULL);
            wcex.hIcon = NULL;
            wcex.hCursor = NULL;
            wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
            wcex.lpszClassName = L"TestWindow";

            RegisterClassEx(&wcex);

            // m_hwnd.reset(CreateWindow(
            m_hwnd = wil::unique_hwnd(CreateWindow(
                wcex.lpszClassName,
                L"Test Window",
                WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                600,
                400,
                NULL,
                NULL,
                wcex.hInstance,
                NULL));

            if (!m_hwnd)
            {
                WEX::Common::Throw::Exception(E_FAIL);
            }

            ShowWindow(m_hwnd.get(), SW_SHOWNORMAL);
        }

        // create and show a window what will be alive until the windowSyncEvent is signaled or the timeout elapses
        void Show(const std::shared_ptr<Event>& windowSyncEvent, std::chrono::milliseconds timeout = std::chrono::milliseconds(100))
        {
            bool result = true;
            concurrency::create_task([windowSyncEvent, timeout, &result]()
            {
                LOG_OUTPUT(L"waiting on window");

                // wait for windowsyncevent to be set with out throwing. since this thread
                // is not waited on in order to allow the message loop to run, we will not
                // throw until after we shutdown the message loop
                result = windowSyncEvent->WaitForNoThrow(timeout);
                if (result)
                {
                    // wait timed out. setting signal
                    LOG_OUTPUT(L"window wait timed out");
                    windowSyncEvent->Set();
                }
            });

            // run the message loop while the wait task is running.
            RunMessageLoop(windowSyncEvent);
            WEX::Common::Throw::IfFalse(result, E_FAIL, L"windowSyncEvent timed out");
        }

        HWND GetHWND() { return m_hwnd.get(); }
    };

} } } } }
