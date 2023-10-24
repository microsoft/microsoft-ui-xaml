// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "TimelineTimer.h"
#include "RepeatButton.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

extern HINSTANCE g_hInstance;

ATOM TimelineTimer::s_timerWindowClass = 0;

TimelineTimer::TimelineTimer()
    : m_interval(0)
    , m_isEnabled(FALSE)
    , m_pOwner(NULL)
    , m_timerWindow(nullptr)
{
    //Note: TimelineTimer does not AddRef its owning RepeatButton
    //here and release it in ~TimelineTimer() to avoid a reference
    //cycle. The RepeatButton would never be released otherwise.
    //The RepeatButton has a strong reference to the TimelineTimer
    //and delete the object in its destructor.
    //TODO: Make this an official weak reference
}

TimelineTimer::~TimelineTimer()
{
    if (m_timerWindow != nullptr)
    {
        DestroyWindow(m_timerWindow);
    }
}

_Check_return_ HRESULT TimelineTimer::Initialize(_In_ RepeatButton* pOwner)
{
    m_pOwner = pOwner;
    RRETURN(S_OK);
}

UINT32 TimelineTimer::get_Interval()
{
    return m_interval;
}

_Check_return_ HRESULT TimelineTimer::put_Interval(_In_ UINT32 interval)
{
    m_interval = interval;
    return S_OK;
}

BOOLEAN TimelineTimer::get_IsEnabled()
{
    return m_isEnabled;
}

_Check_return_ HRESULT TimelineTimer::Start()
{
    if (m_timerWindow == nullptr)
    {
        IFC_RETURN(CreateTimerWindow());
    }

    if (!SetTimer(m_timerWindow, c_timerId, static_cast<UINT>(m_interval), NULL))
    {
        IFC_RETURN(E_FAIL);
    }

    m_isEnabled = TRUE;
    return S_OK;
}

_Check_return_ HRESULT TimelineTimer::Stop()
{
    if (m_timerWindow != nullptr)
    {
        KillTimer(m_timerWindow, c_timerId);
    }

    m_isEnabled = FALSE;
    return S_OK;
}

_Check_return_ HRESULT TimelineTimer::TimerCallback()
{
    HRESULT hr = S_OK;

    if (m_isEnabled)
    {
        {
            auto pegOwner = try_make_autopeg(m_pOwner);

            if (pegOwner)
            {
                IFC(m_pOwner->TickCallback());
            }
        }

        // Restart the animation for the next tick.
        IFC(Start());
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT TimelineTimer::CreateTimerWindow()
{
    ASSERT(m_timerWindow == nullptr);

    IFC_RETURN(EnsureTimerWindowClass());

    m_timerWindow = CreateWindow(
        reinterpret_cast<const WCHAR *>(s_timerWindowClass),
        NULL,
        0, 0, 0, 0, 0, HWND_MESSAGE, NULL, g_hInstance, NULL);

    if (m_timerWindow == nullptr)
    {
        IFC_RETURN(::HResultFromKnownLastError());
    }

    SetLastError(0);
    if (SetWindowLongPtr(m_timerWindow, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this)) == 0 &&
        GetLastError() != 0)
    {
        IFC_RETURN(E_FAIL);
    }

    return S_OK;
}

_Check_return_ HRESULT TimelineTimer::EnsureTimerWindowClass()
{
    // RegisterClass should not be called on multiple threads at the same time. If it does,
    // one thread will get an "already registered" failure. Use a mutex to ensure that
    // doesn't happen.
    static std::mutex s_mutex;
    std::lock_guard<std::mutex> lock(s_mutex);

    // Skip the expensive path in the common case that the window class is already registered.
    if (s_timerWindowClass == 0)
    {
        WNDCLASS windowClass = { 0 };
        windowClass.lpfnWndProc = TimelineTimer::TimerWindowProc;
        windowClass.hInstance = g_hInstance;
        windowClass.lpszClassName = L"WinUI_TimelineTimerClass";

        s_timerWindowClass = RegisterClass(&windowClass);

        if (s_timerWindowClass == 0)
        {
            IFC_RETURN(E_FAIL);
        }
    }

    return S_OK;
}

LRESULT CALLBACK TimelineTimer::TimerWindowProc(
    _In_  HWND window,
    _In_  UINT message,
    _In_  WPARAM wParam,
    _In_  LPARAM lParam
    )
{
    LRESULT result = 0;

    switch (message)
    {
        case WM_TIMER:
            {
                TimelineTimer* timelineTimer = reinterpret_cast<TimelineTimer *>(GetWindowLongPtr(window, GWLP_USERDATA));
                IGNOREHR(timelineTimer->TimerCallback());
            }
            break;

        default:
            result = DefWindowProc(window, message, wParam, lParam);
            break;
    }

    return result;
}

