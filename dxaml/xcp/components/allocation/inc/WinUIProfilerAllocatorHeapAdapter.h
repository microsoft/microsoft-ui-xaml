// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
#pragma once
#include "WinUIProfilerAllocatorRegistry.h"
#include "WinUIProfilerObserverSuppression.h"
#include <type_traits>

namespace WinUIProfiler::AllocatorObservation
{
    // Windows heap boundary adapter; runtime context is sampled before locking.
    // Runtime callbacks are nonthrowing. GetContext releases every runtime lock
    // before returning; Emit rejects a stale generation without taking this lock.
    template<typename Runtime>
    class HeapAdapter
    {
        Runtime& m_runtime;
        struct HeapResult { void* Address = nullptr; BOOL Freed = FALSE; };
        struct LastErrorRestore
        {
            DWORD Value = GetLastError();
            ~LastErrorRestore() noexcept { SetLastError(Value); }
        };
        HeapResult Run(HANDLE heap, DWORD flags, Input input) noexcept
        {
            LastErrorRestore lastError;
            const DWORD incoming = lastError.Value;
            auto call = [&]() noexcept
            {
                SetLastError(incoming);
                HeapResult result;
                switch (input.Kind)
                {
                case Operation::Allocate: result.Address = HeapAlloc(heap, flags, static_cast<SIZE_T>(input.Requested)); break;
                case Operation::Reallocate:
                    result.Address = HeapReAlloc(heap, flags, reinterpret_cast<void*>(input.PreviousAddress),
                        static_cast<SIZE_T>(input.Requested));
                    break;
                case Operation::Free:
                    result.Freed = HeapFree(heap, flags, reinterpret_cast<void*>(input.PreviousAddress));
                    break;
                }
                lastError.Value = GetLastError();
                return result;
            };
            if (ObserverSuppression::IsSuppressed()) return call();
            ObserverSuppression::Scope observer;
            if (!m_runtime.Enabled()) return call();
            GUID allocatorEpoch{};
            uint64_t generation = 0;
            if (!m_runtime.GetContext(allocatorEpoch, generation)) return call();
            auto& registry = m_runtime.GetRegistry();
            typename std::remove_reference_t<decltype(registry)>::Transaction transaction(
                registry, true, allocatorEpoch, generation);
            // Serialize before the actual operation, including address retirement.
            const HeapResult result = call();
            input.ResultAddress = reinterpret_cast<uintptr_t>(result.Address);
            input.Succeeded = input.Kind == Operation::Free ? result.Freed != FALSE : result.Address != nullptr;
            if (input.Succeeded && input.Kind != Operation::Free)
            {
                const SIZE_T actual = m_runtime.Size(heap, result.Address);
                if (actual != static_cast<SIZE_T>(-1)) input.Actual = {true, actual};
            }
            const auto observation = transaction.Complete(input);
            if (observation.Code == RegistryStatus::Emit)
                transaction.Delivery(m_runtime.Emit(observation.Payload, generation));
            const uint64_t after = transaction.LossCount();
            // Retry pending publication even when this operation adds no loss.
            m_runtime.ReportLoss(after, generation);
            return result;
        }
    public:
        explicit HeapAdapter(Runtime& runtime) noexcept : m_runtime(runtime) {}
        void* Allocate(HANDLE heap, DWORD flags, SIZE_T requested) noexcept
        { return Run(heap, flags, {Operation::Allocate, 0, 0, requested, {}, false, 0}).Address; }
        void* Resize(HANDLE heap, DWORD flags, void* address, SIZE_T requested) noexcept
        { return Run(heap, flags, {Operation::Reallocate, reinterpret_cast<uintptr_t>(address), 0, requested, {}, false, 0}).Address; }
        BOOL Free(HANDLE heap, DWORD flags, void* address) noexcept
        { return Run(heap, flags, {Operation::Free, reinterpret_cast<uintptr_t>(address), 0, 0, {}, false, 0}).Freed; }
    };
}
