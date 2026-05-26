// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <TestEvent.h>

class HostingDispatcher
{
    public:
        static HostingDispatcher* Get();

        void Init(wrl::ComPtr<msy::IDispatcherQueue> dispatcher, HWND mainWindowHandle);
        void DeInit();

        wrl::ComPtr<wuc::ICoreWindow> GetCoreWindow();
        HWND GetMainWindowHandle() const;
        void SetMainWindowHandle(HWND handle);

        void SetDispatcher(_In_ msy::IDispatcherQueue* dispatcher);
        wrl::ComPtr<msy::IDispatcherQueue> GetDispatcher() const;

        bool IsUIThread() const { return m_uiThreadId == ::GetCurrentThreadId(); }
        DWORD GetUIThreadId() const { return m_uiThreadId; }

        void SetSecondaryView(wac::ICoreApplicationView* view);
        void ResetSecondaryView();

    private:
        HostingDispatcher() = default;

        wrl::ComPtr<wuc::ICoreWindow> m_coreWindow;
        wrl::ComPtr<msy::IDispatcherQueue> m_dispatcherQueue;
        DWORD m_uiThreadId = 0;
        HWND mainWindowHandle = {};

        wrl::ComPtr<wuc::ICoreWindow> m_secondaryViewCoreWindow;
};
