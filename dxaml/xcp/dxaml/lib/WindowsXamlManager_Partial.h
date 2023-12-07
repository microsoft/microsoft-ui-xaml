// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "WindowsXamlManager.g.h"
#include "FrameworkApplication.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(WindowsXamlManager)
    {
        friend class WindowsXamlManagerFactory; // To access EnsureCoreWindow (CodeGen does not allow partial factories)

    public:
        WindowsXamlManager();
        ~WindowsXamlManager() override;

    public:
        _Check_return_ HRESULT Initialize() override;
        IFACEMETHOD(Close)() override;
        HRESULT CloseImpl(bool synchronous);

    private:
        // XamlCore is a per-thread object that represents the the running Xaml Core on the thread.
        // It tracks the WindowsXamlManager objects on the thread, and shuts down the XAML
        // state on the thread when all WXM instances on the thread are closed.  It may outlive
        // all the instances of WindowsXamlManager.
        class XamlCore
        {
        public:
            ~XamlCore();
            _Check_return_ HRESULT Initialize(msy::IDispatcherQueue* dq);
            _Check_return_ HRESULT Close();

            enum class State {
                Normal,
                Closing,
                Closed
            };
            State GetState() const { return m_state; }
            void SetState(State state) { m_state = state; }

            void AddManager(_In_ WindowsXamlManager* wxm);
            void RemoveManager(_In_ WindowsXamlManager* wxm);
            int ManagerCount() const { return m_managers.size(); }
            void SyncCloseAllManagers();

        private:
            wrl::ComPtr<msy::IDispatcherQueue> m_dispatcherQueue;
            wrl::ComPtr<msy::IDispatcherQueue3> m_dispatcherQueue3;
            EventRegistrationToken m_frameworkShutdownStartingToken {};
            State m_state {State::Normal};

            // These are raw non-ref-counted pointers, the WindowsXamlManager objects add and remove themselves.
            std::vector<DirectUI::WindowsXamlManager*> m_managers;

            inline static std::atomic<int> s_instancesInProcess{};
        };

        _Check_return_ HRESULT EnqueueClose();
        _Check_return_ HRESULT CheckWindowingModelPolicy();

        wrl::ComPtr<msy::IDispatcherQueue> m_dispatcherQueue;
        bool m_bIsClosed{ false };

        // This shared_ptr is just to keep tls_xamlCore alive in cases where the thread is rudely shut down.
        std::shared_ptr<XamlCore> m_xamlCore;

        // Note this is thread-local.  Each thread has zero or one tls_xamlCore.
        inline static thread_local std::shared_ptr<WindowsXamlManager::XamlCore> tls_xamlCore;
    };
}

