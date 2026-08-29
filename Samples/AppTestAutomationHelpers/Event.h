// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Windows.h>
#include <synchapi.h>

#include "AutoHandle.h"
#include <winrt\base.h>

namespace AppTestAutomationHelpers 
{
    class Event
    {
    public:
        Event()
            : m_handle(::CreateEvent(nullptr, FALSE, FALSE, nullptr))
            , m_hasFired(false)
            , m_timesFired(0)
        {
            if (!m_handle.IsValid())
            {
                throw winrt::hresult_error(E_FAIL, winrt::hstring(L"Failed to create the event"));
            }
        }

        Event(bool manualReset, bool initialState)
            : m_handle(::CreateEvent(nullptr, manualReset, initialState, nullptr))
            , m_hasFired(initialState)
            , m_timesFired(0)
        {
            if (!m_handle.IsValid())
            {
                throw winrt::hresult_error(E_FAIL, winrt::hstring(L"Failed to create the event"));
            }
        }

        void Set()
        {
            m_hasFired = true;
            m_timesFired++;

            if (SetEvent(m_handle) == 0)
            {
                throw winrt::hresult_error(E_FAIL, winrt::hstring(L"Failed to set the event"));
            }
        }

        void Wait()
        {
            if (WaitForSingleObject(m_handle, INFINITE) != WAIT_OBJECT_0)
            {
                throw winrt::hresult_error(E_FAIL, winrt::hstring(L"Failed to wait for the event"));
            }
        }

        void WaitFor(long timeoutValueMs, bool enforceUnderDebugger = false)
        {
            if (timeoutValueMs == INFINITE)
            {
                throw winrt::hresult_error(E_FAIL, winrt::hstring(L"Timeout cannnot be infinite."));
            }

            // Event timeouts can be frustrating when trying to debug tests. 
            // When under the debugger we disable all timeouts.
            if (IsDebuggerPresent() && !enforceUnderDebugger)
            {
                timeoutValueMs = INFINITE;
            }

            if (WaitForSingleObject(m_handle, timeoutValueMs) != WAIT_OBJECT_0)
            {
                throw winrt::hresult_error(E_FAIL, winrt::hstring(L"Timed out or failed to wait for the event."));
            }
        }

        void WaitForDefault()
        {
            WaitFor(5000 /* timeoutValueMs */);
        }

        bool WaitForNoThrow(long timeoutValueMs, bool enforceUnderDebugger = true)
        {
            // Event timeouts can be frustrating when trying to debug tests. 
            // When under the debugger we disable all timeouts.
            if (IsDebuggerPresent() && !enforceUnderDebugger)
            {
                timeoutValueMs = INFINITE;
            }

            return WaitForSingleObject(m_handle, timeoutValueMs) == WAIT_OBJECT_0;
        }

        bool HasFired()
        {
            return m_hasFired;
        }

        void Reset()
        {
            m_hasFired = false;
            m_timesFired = 0;

            if (ResetEvent(m_handle) == 0)
            {
                throw winrt::hresult_error(E_FAIL, winrt::hstring(L"Failed to reset the event"));

            }
        }

        int TimesFired()
        {
            return m_timesFired;
        }

        Event(const Event&) = delete;
        Event& operator=(const Event&) = delete;

    protected:
        Handle m_handle;
        bool m_hasFired;
        int m_timesFired;
    };
}
