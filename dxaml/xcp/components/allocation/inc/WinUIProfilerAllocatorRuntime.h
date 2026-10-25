// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
#pragma once
#pragma push_macro("Trace")
#undef Trace
#include "WinUIProfilerAllocatorHeapAdapter.h"
#pragma pop_macro("Trace")
#include <atomic>

namespace WinUIProfiler::AllocatorObservation
{
    class AllocatorRuntime
    {
        uint16_t m_origin = 0;
        int64_t m_observationStartQpc = 0;
    public:
        AllocatorRuntime() noexcept = default;
        ~AllocatorRuntime() noexcept;
        AllocatorRuntime(const AllocatorRuntime&) = delete;
        AllocatorRuntime& operator=(const AllocatorRuntime&) = delete;
        bool Enabled() noexcept;
        bool GetContext(GUID& epoch, uint64_t& generation) noexcept;
        Registry<16384>& GetRegistry() noexcept;
        SIZE_T Size(HANDLE heap, const void* address) noexcept { return HeapSize(heap, 0, address); }
        bool Emit(const Allocation& payload, uint64_t generation) noexcept;
        void ReportLoss(uint64_t cumulative, uint64_t generation) noexcept;
    };
    struct HeapObservers
    {
        void* (*Allocate)(HANDLE, DWORD, SIZE_T) noexcept;
        void* (*Resize)(HANDLE, DWORD, void*, SIZE_T) noexcept;
        BOOL (*Free)(HANDLE, DWORD, void*) noexcept;
    };
    // The allocation library is also linked into tools without a XAML runtime.
    // Publish only a process-lifetime table; there is no allocating registration.
    inline std::atomic<const HeapObservers*> s_heapObservers{nullptr};
    static_assert(decltype(s_heapObservers)::is_always_lock_free);
    static_assert(std::is_trivially_destructible_v<decltype(s_heapObservers)>);
    inline void SetHeapObservers(const HeapObservers* observers) noexcept
    {
        s_heapObservers.store(observers, std::memory_order_release);
    }
    inline void* ObservedHeapAllocate(HANDLE heap, DWORD flags, SIZE_T requested) noexcept
    {
        const auto observers = s_heapObservers.load(std::memory_order_acquire);
        return observers ? observers->Allocate(heap, flags, requested) : HeapAlloc(heap, flags, requested);
    }
    inline void* ObservedHeapResize(HANDLE heap, DWORD flags, void* address, SIZE_T requested) noexcept
    {
        const auto observers = s_heapObservers.load(std::memory_order_acquire);
        return observers ? observers->Resize(heap, flags, address, requested) : HeapReAlloc(heap, flags, address, requested);
    }
    inline BOOL ObservedHeapFree(HANDLE heap, DWORD flags, void* address) noexcept
    {
        const auto observers = s_heapObservers.load(std::memory_order_acquire);
        return observers ? observers->Free(heap, flags, address) : HeapFree(heap, flags, address);
    }
#ifdef WINUI_PROFILER_STANDALONE
    void ConfigureAllocatorForTest(bool enabled, bool lossSelected = true) noexcept;
    void ArmAllocatorContextRaceForTest(unsigned attempts = 1) noexcept;
    void FailAllocatorLossWriteForTest() noexcept;
    void RenewAllocatorSelectionForTest() noexcept;
    void RenewAllocatorProducerForTest() noexcept;
    uint64_t AllocatorReportedLossForTest() noexcept;
#endif
}
