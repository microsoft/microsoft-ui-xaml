// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "DebugSettings.g.h"
#include <map>
#include <wil\resource.h>

namespace DirectUI
{
    PARTIAL_CLASS(DebugSettings)
    {
        public:
            DebugSettings();
        public:
            _Check_return_ HRESULT get_EnableFrameRateCounterImpl(_Out_ BOOLEAN* pValue);
            _Check_return_ HRESULT put_EnableFrameRateCounterImpl(_In_ BOOLEAN value);
            _Check_return_ HRESULT get_IsBindingTracingEnabledImpl(_Out_ BOOLEAN* pValue);
            _Check_return_ HRESULT put_IsBindingTracingEnabledImpl(_In_ BOOLEAN value);
            _Check_return_ HRESULT get_IsXamlResourceReferenceTracingEnabledImpl(_Out_ BOOLEAN* pValue);
            _Check_return_ HRESULT put_IsXamlResourceReferenceTracingEnabledImpl(_In_ BOOLEAN value);
            _Check_return_ HRESULT get_IsTextPerformanceVisualizationEnabledImpl(_Out_ BOOLEAN* pValue);
            _Check_return_ HRESULT put_IsTextPerformanceVisualizationEnabledImpl(_In_ BOOLEAN value);
            _Check_return_ HRESULT get_FailFastOnErrorsImpl(_Out_ BOOLEAN* pValue);
            _Check_return_ HRESULT put_FailFastOnErrorsImpl(_In_ BOOLEAN value);
            _Check_return_ HRESULT get_LayoutCycleDebugBreakLevelImpl(_Out_ ABI::Microsoft::UI::Xaml::LayoutCycleDebugBreakLevel* pValue);
            _Check_return_ HRESULT put_LayoutCycleDebugBreakLevelImpl(_In_ ABI::Microsoft::UI::Xaml::LayoutCycleDebugBreakLevel value);
            _Check_return_ HRESULT get_LayoutCycleTracingLevelImpl(_Out_ ABI::Microsoft::UI::Xaml::LayoutCycleTracingLevel* pValue);
            _Check_return_ HRESULT put_LayoutCycleTracingLevelImpl(_In_ ABI::Microsoft::UI::Xaml::LayoutCycleTracingLevel value);

            void OnThreadInitialized();
            void OnThreadDeinitialized();

            std::vector<Microsoft::WRL::ComPtr<msy::IDispatcherQueue>> GetDispatcherQueues();
            Microsoft::WRL::ComPtr<msy::IDispatcherQueue> GetDispatcherQueueForThreadId(DWORD threadId);
            

        private:
            BOOLEAN m_fIsBindingTracingEnabled : 1;
            BOOLEAN m_fIsXamlResourceReferenceTracingEnabled : 1;
#if XCP_MONITOR
            // We can't clear these maps while XamlDiagnostics is alive in the case that we re-attach.
            typedef std::map<DWORD, Microsoft::WRL::WeakRef,
                    std::less<DWORD>,
                    XcpAllocation::LeakIgnoringAllocator<std::pair<DWORD, Microsoft::WRL::WeakRef>>> DispatcherQueueMap;
#else
            typedef std::map<DWORD, Microsoft::WRL::WeakRef> DispatcherQueueMap;
#endif
            wil::srwlock m_maplock;

            DispatcherQueueMap m_dispatcherQueues;
    };
}
