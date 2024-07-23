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
        _Check_return_ IFACEMETHOD(Close)() override;
        _Check_return_ HRESULT CloseImpl(bool synchronous);

        _Check_return_ HRESULT GetXamlShutdownCompletedOnThreadEventSourceNoRef(
            _Outptr_ XamlShutdownCompletedOnThreadEventSourceType** ppEventSource) override
        {
            if (!m_xamlShutdownCompletedOnThreadEventSource)
            {
                IFC_RETURN(ctl::make(&m_xamlShutdownCompletedOnThreadEventSource));
                m_xamlShutdownCompletedOnThreadEventSource->Initialize(
                    KnownEventIndex::WindowsXamlManager_XamlShutdownCompletedOnThread,
                    this,
                    FALSE);
            }
            *ppEventSource = m_xamlShutdownCompletedOnThreadEventSource.Get();
            return S_OK;
        }

        // Get the WindowsXamlManager for the current thread.
        // (always returns null in legacy shutdown mode)
        static ctl::ComPtr<WindowsXamlManager> GetForCurrentThread();

    private:

        void RaiseXamlShutdownCompletedOnThreadEvent(
            _In_ msy::IDispatcherQueueShutdownStartingEventArgs* shutdownStartingArgs);

        ctl::ComPtr<XamlShutdownCompletedOnThreadEventSourceType> m_xamlShutdownCompletedOnThreadEventSource;

        // XamlCore is a base class for the XamlCoreLegacyShutdown and XamlCoreNewShutdown classes.
        // There is at most one of these objects per-thread, stored in tls_xamlCore.  This object manages
        // the lifetime of the Xaml runtime on the thread.
        class XamlCore
        {
        public:
            virtual ~XamlCore();
            _Check_return_ HRESULT Initialize(msy::IDispatcherQueue* dq);
            _Check_return_ HRESULT Close();

            enum class State {
                Normal,
                Closing,
                Closed
            };
            State GetState() const { return m_state; }
            void SetState(State state) { m_state = state; }
           
            virtual void RegisterManager(_In_ WindowsXamlManager* manager) = 0;
            virtual void OnManagerClosed(_In_ WindowsXamlManager* manager) = 0;
            virtual ctl::ComPtr<DirectUI::WindowsXamlManager> GetForCurrentThread() = 0;
            virtual bool ReadyForEarlyShutdown() const = 0;

            virtual _Check_return_ HRESULT OnFrameworkShutdownStarting(
                _In_ msy::IDispatcherQueueShutdownStartingEventArgs* args) = 0;

        private:
            wrl::ComPtr<msy::IDispatcherQueue> m_dispatcherQueue;
            wrl::ComPtr<msy::IDispatcherQueue3> m_dispatcherQueue3;
            EventRegistrationToken m_frameworkShutdownStartingToken {};
            State m_state {State::Normal};

            inline static std::atomic<int> s_instancesInProcess{};
        };

        friend class XamlCoreLegacyShutdown;
        friend class XamlCoreNewShutdown;

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

