// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

//Potential improvement: The TimelineTimer should raise a Tick event
//so there can be other consumers than the RepeatButton.

namespace DirectUI
{
    class TimelineCompletedHandler;
    class RepeatButton;

    class TimelineTimer: public ctl::WeakReferenceSource
    {
        friend class TimelineCompletedHandler;

        public:
            TimelineTimer();
            ~TimelineTimer() override;

            _Check_return_ HRESULT Initialize(_In_ RepeatButton* pOwner);
            using ctl::WeakReferenceSource::Initialize;

            // Timer interval in milliseconds
            UINT32 get_Interval();
            _Check_return_ HRESULT put_Interval(_In_ UINT32 interval);

            BOOLEAN get_IsEnabled();

            _Check_return_ HRESULT Start();
            _Check_return_ HRESULT Stop();
            _Check_return_ HRESULT TimerCallback();

        private:
            _Check_return_ HRESULT CreateTimerWindow();
            _Check_return_ HRESULT EnsureTimerWindowClass();

            static LRESULT CALLBACK TimerWindowProc(
              _In_  HWND window,
              _In_  UINT message,
              _In_  WPARAM wParam,
              _In_  LPARAM lParam
            );

            UINT32        m_interval;
            BOOLEAN       m_isEnabled;
            RepeatButton* m_pOwner;

            // Message only window used for timer callbacks.
            HWND        m_timerWindow;
            static ATOM s_timerWindowClass;

            // ID used to identify the timer used.
            // The ID needs to be non-zero, but otherwise can be any value,
            // so we'll use 1 as our ID.  This is arbitrary, but since we'll only
            // ever have one timer going at a time, the exact value doesn't matter.
            // We basically just need an ID of some sort so we can call KillTimer
            // with that same ID to stop the timer if we need to.
            static constexpr UINT_PTR c_timerId = 1;
    };
}
