// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#ifdef __DISABLE_EVENT_WAIT
#ifndef __DISABLE_EVENT_MUTEX
#define __DISABLE_EVENT_MUTEX // If Wait is disabled, then we also dont need the mutex
#endif
#endif

#ifndef __DISABLE_EVENT_WAIT
extern "C" __declspec(dllimport) HRESULT __stdcall Thread_Wait_For(HANDLE handle, unsigned long milliseconds);
#endif

#include <windows.h>
#include "Handle.h"

#include <WexAssert.h>
#include <wex.common.h>
#include <WexTestClass.h>

#include <Sddl.h>
#include <chrono>
#ifndef __DISABLE_EVENT_MUTEX
#include <mutex>
#endif
#include <string>
#include <wil/result_macros.h>

#include <XamlLogging.h>

#ifndef __DISABLE_EVENT_WAIT
using namespace std::chrono_literals;
#endif

#ifndef EV_LOG_OUTPUT
#define EV_LOG_OUTPUT LOG_OUTPUT
#endif

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Common {

#ifndef __DISABLE_SCREEN_CAPTURE
        // We forward declare this method because this Event class is used all over the place
        // and capturing a screenshot means different things in different contexts:
        // 1. Running in a modern TAEF process, have to go through language projection
        //    and call into TestServices.Utilities.CaptureScreen
        // 2. Inside of Private.Infrastructure.Client - need to make RPC call to server
        // 3. In isolated test, capturing a screenshot has no meaning
        // 4. Inside of Private.Infrastructure.Server - where the magic happens
        namespace ScreenCapture {
            void TakeScreenshot(const wchar_t* filename);
        }
#endif

        enum class EventOptions : int
        {
            None                        = 0x0000,       // No options, default event
            Named                       = 0x0001,       // Named event, creates event using SecurityAttributes or opens existing named if OpenExisting is specified
            ManualReset                 = 0x0002,       // Event requires manual reset after triggered
            InitiallySet                = 0x0004,       // Event is initially set
            OpenExisting                = 0x0008,       // Opens an existing event, requires a name.
            CaptureScreenOnTimeout      = 0x0010,       // Like none, except will capture screenshot when timeout occurs
            FailFastOnTimeout           = 0x0020,       // Like none, except will fail fast when timeout occurs
            CaptureAndFailFastOnTimeout = CaptureScreenOnTimeout | FailFastOnTimeout,
        };
        DEFINE_ENUM_FLAG_OPERATORS(EventOptions);

        // Some of the most common combinations of events options.
        namespace EventOptionConstants {
            const EventOptions c_namedWithReset = EventOptions::Named | EventOptions::ManualReset;
            const EventOptions c_openNamed = EventOptions::OpenExisting | EventOptions::Named;
            const EventOptions c_openNamedWithReset = EventOptions::ManualReset | c_openNamed;
            const EventOptions c_captureNamed = EventOptions::Named | EventOptions::CaptureScreenOnTimeout;
            const EventOptions c_openNamedCapture = EventOptions::OpenExisting | c_captureNamed;
            const EventOptions c_setWithReset = EventOptions::ManualReset| EventOptions::InitiallySet;
        }

        struct EventData
        {
            EventData(EventOptions options, const wchar_t* name = L"")
                : Handle(nullptr)
                , Options(options)
                , Name(name)
                , TimesFired(0)
                , HasFired(HasOption(EventOptions::InitiallySet))
            {
                if (HasOption(EventOptions::OpenExisting))
                {
                    Handle = ::OpenEventW(EVENT_ALL_ACCESS, false, Name.c_str());
                }
                else if (HasOption(EventOptions::Named))
                {
                    const wchar_t eventAclSDDL[] = L"D:(A;;0x1F0003;;;WD)(A;;0x1F0003;;;S-1-15-2-1)S:(ML;;NX;;;LW)";
                    PSECURITY_DESCRIPTOR pSD = NULL;
                    WEX::Common::Throw::LastErrorIf(
                        !::ConvertStringSecurityDescriptorToSecurityDescriptorW(
                            eventAclSDDL, SDDL_REVISION_1, (PSECURITY_DESCRIPTOR*)&pSD, NULL),
                        L"Failed to create the security descriptor for the event.");
                    SECURITY_ATTRIBUTES sa;
                    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
                    sa.lpSecurityDescriptor = pSD;
                    sa.bInheritHandle = false;
                    Handle = ::CreateEventW(&sa, HasOption(EventOptions::ManualReset), HasOption(EventOptions::InitiallySet), Name.c_str());
                }
                else
                {
                    Handle = ::CreateEventW(nullptr, HasOption(EventOptions::ManualReset), HasOption(EventOptions::InitiallySet), nullptr);
                }

                WEX::Common::Throw::LastErrorIf(
                    !Handle.IsValid(), L"Failed to create the event");
            }

            bool HasOption(EventOptions option)
            {
                return (Options & option) == option;
            }

            std::wstring Name;
            EventOptions Options;
            int TimesFired;
            bool HasFired;
            Handle Handle;
        };

        class Event
        {
#ifdef __FORCE_NAMED_EVENTS
// When __FORCE_NAMED_EVENTS is active, all Event instances must have a name
// Please make sure to specify a name, e.g.:
//     Event myEvent(L"MyEvent");
        private:
#else
        public:
#endif
            Event()
                : m_data(EventOptions::None)
            {
            }

        public:

            Event(_In_ const wchar_t* name)
                : m_data(EventOptions::CaptureScreenOnTimeout, name)
            {
                if (IsBVT())
                {
                    EV_LOG_OUTPUT(L"Creating Event %s", Name());
                }
            }

            Event(EventOptions options, _In_opt_z_ const wchar_t* name
#ifndef __FORCE_NAMED_EVENTS
             = L""
#endif
            )
                : m_data(options, name)
            {
                if (IsBVT())
                {
                    EV_LOG_OUTPUT(L"Creating Event %s", Name());
                }
            }

            ~Event()
            {
                m_isClosed = true;
                {
#ifndef __DISABLE_EVENT_MUTEX
                    std::lock_guard<std::recursive_mutex> lock(m_mutex);
#endif
                    if (IsBVT())
                    {
                        EV_LOG_OUTPUT(L"Disposing Event %s, executions count: %d", Name(), m_data.TimesFired);
                    }
                }
            }

            void Set()
            {
                bool closed = this->m_isClosed;
                if (!closed)
                {
#ifndef __DISABLE_EVENT_MUTEX
                    std::lock_guard<std::recursive_mutex> lock(m_mutex);
                    closed = this->m_isClosed;
                    if (closed)
                    {
                        LOG_ERROR_SC(L"Setting event while closed and locked");
                    }
#endif
                    m_data.HasFired = true;
                    ++m_data.TimesFired;
                    auto setEventResult=::SetEvent(m_data.Handle);
                    WEX::Common::Throw::LastErrorIf(!setEventResult);
                }
                else
                {
                    LOG_ERROR_SC(L"Setting event while closed");
                }
            }

#ifndef __DISABLE_EVENT_WAIT

            static std::chrono::milliseconds GetDefaultTimeout();
            std::chrono::milliseconds Timeout = Event::GetDefaultTimeout();

            static std::chrono::milliseconds GetDefaultTextInputTimeout();

            void WaitFor(bool enforceUnderDebugger = false)
            {
                WaitFor(Timeout, enforceUnderDebugger);
            }

            void WaitFor(std::chrono::milliseconds timeout, bool enforceUnderDebugger = false)
            {
                long msCount = static_cast<long>(timeout.count());
                WEX::Common::Throw::IfFalse(msCount != INFINITE, E_FAIL, L"Timeout cannnot be infinite");

                // Event timeouts can be frustrating when trying to debug tests.
                // Whe under the debugger we disable all timeouts.
                if (IsDebuggerPresent() && !enforceUnderDebugger)
                {
                    msCount = INFINITE;
                }

                HRESULT hr = Thread_Wait_For(GetHandle(), msCount);
                if (FAILED(hr))
                {
                    if (IsBVT() && m_data.HasOption(EventOptions::CaptureScreenOnTimeout))
                    {
                        if (hr == HRESULT_FROM_WIN32(WAIT_TIMEOUT))
                        {
                            LOG_ERROR_SC(L"Event %s timeout", Name());
                        }
                        else
                        {
                            LOG_WARNING_SC(L"Event %s failed wait with [HRESULT 0x%08X]", Name(), hr);
                        }
                    }
#ifndef __DISABLE_SCREEN_CAPTURE
                    else if (m_data.HasOption(EventOptions::CaptureScreenOnTimeout))
                    {
                        ScreenCapture::TakeScreenshot(Name());
                    }
#endif

                    if (m_data.HasOption(EventOptions::FailFastOnTimeout))
                    {
                        FAIL_FAST_LAST_ERROR();
                    }
                    else
                    {
                        WEX::Common::Throw::Exception(hr, WEX_STRING(L"Event %s timeout or failed to wait", Name()));
                    }
                }
            }

            void WaitForDefault(bool enforceUnderDebugger = true)
            {
                WaitFor(Timeout, enforceUnderDebugger);
            }

            bool WaitForNoThrow(std::chrono::milliseconds timeout, bool enforceUnderDebugger = true)
            {
                long msCount = static_cast<long>(timeout.count());

                // Event timeouts can be frustrating when trying to debug tests.
                // Whe under the debugger we disable all timeouts.
                if (IsDebuggerPresent() && !enforceUnderDebugger)
                {
                    msCount = INFINITE;
                }

                return SUCCEEDED(Thread_Wait_For(GetHandle(), msCount));
            }

            bool WaitForNoThrow(bool enforceUnderDebugger = true)
            {
                return WaitForNoThrow(Timeout, enforceUnderDebugger);
            }

#endif

            bool HasFired()
            {
                return m_data.HasFired;
            }

            void Reset()
            {
                m_data.TimesFired = 0;
                m_data.HasFired = false;
                WEX::Common::Throw::LastErrorIf(!::ResetEvent(GetHandle()));
            }

            int TimesFired()
            {
                return m_data.TimesFired;
            }

            Event(const Event&) = delete;
            Event& operator=(const Event&) = delete;

            const wchar_t* Name() const
            {
                return m_data.Name.c_str();
            }

        private:
            bool m_isClosed = false;
            EventData m_data;

#ifndef __DISABLE_EVENT_MUTEX
            std::recursive_mutex m_mutex;
#endif

            HANDLE GetHandle()
            {
#ifndef __DISABLE_EVENT_MUTEX
                std::lock_guard<std::recursive_mutex> lock(m_mutex);
#endif
                return m_data.Handle;
            }

#ifndef __DISABLE_EVENT_WAIT
            static bool IsBVT();
#else
            bool IsBVT() const { return false; }
#endif
        };
    }
} } } }
