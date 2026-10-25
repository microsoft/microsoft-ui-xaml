// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
#pragma once
#include "WinUIProfilerAllocatorObservation.h"
#define WINUI_PROFILER_HASH_REGISTRY 1

namespace WinUIProfiler::AllocatorObservation
{
    enum class RegistryStatus { Disabled, Failed, Unmatched, Invalid, Capacity, Exhausted, Emit };
    struct RegistryResult
    {
        RegistryStatus Code;
        Allocation Payload{};
        size_t Probes = 0; // Lookup buckets examined, not deletion-compaction work.
    };

    template<size_t Capacity>
    class Registry
    {
        static_assert(Capacity > 0);
        static_assert(Capacity <= (SIZE_MAX - 1) / 2);
        static constexpr size_t BucketCount = Capacity * 2 + 1;
        SRWLOCK m_lock = SRWLOCK_INIT;
        // Keep the live load below one half; backward-shift deletion avoids
        // accumulating tombstones that make every miss traverse the whole table.
        Record m_records[BucketCount]{};
        size_t m_count = 0;
        GUID m_epoch{};
        uint64_t m_generation = 0;
        uint64_t m_lastId;
        uint64_t m_losses = 0;
        void Loss() noexcept { if (m_losses != UINT64_MAX) ++m_losses; }
        static size_t Next(size_t index) noexcept { return index + 1 == BucketCount ? 0 : index + 1; }
        static size_t Distance(size_t end, size_t begin) noexcept
        { return end >= begin ? end - begin : BucketCount - (begin - end); }
        size_t Find(uintptr_t address, size_t& probes) const noexcept
        {
            size_t index = HomeBucket(address);
            for (size_t visited = 0; visited < BucketCount; ++visited)
            {
                ++probes;
                if (!m_records[index].Id || m_records[index].Address == address) return index;
                index = Next(index);
            }
            return BucketCount;
        }
        void Remove(size_t index) noexcept
        {
            size_t hole = index;
            m_records[hole] = {};
            size_t scan = Next(hole);
            for (size_t visited = 0; visited < BucketCount && m_records[scan].Id;
                ++visited, scan = Next(scan))
            {
                const size_t home = HomeBucket(m_records[scan].Address);
                if (Distance(hole, home) < Distance(scan, home))
                {
                    m_records[hole] = m_records[scan];
                    m_records[scan] = {};
                    hole = scan;
                }
            }
            --m_count;
        }
    public:
        static size_t HomeBucket(uintptr_t address) noexcept
        {
            uint64_t mixed = static_cast<uint64_t>(address);
            mixed ^= mixed >> 30;
            mixed *= 0xbf58476d1ce4e5b9ULL;
            mixed ^= mixed >> 27;
            mixed *= 0x94d049bb133111ebULL;
            mixed ^= mixed >> 31;
            return static_cast<size_t>(mixed % BucketCount);
        }
        // A restored high-water mark may be supplied; IDs never reset on enable.
        explicit Registry(uint64_t lastId = 0) noexcept : m_lastId(lastId) {}
        Registry(const Registry&) = delete;
        Registry& operator=(const Registry&) = delete;

        class Transaction
        {
            Registry& m_registry;
            RegistryStatus m_state = RegistryStatus::Disabled;
            bool m_locked = false;
            bool m_completed = false;
            bool m_pending = false;
        public:
            // MUST precede the real Heap* call, including free/realloc. Keep alive
            // through ordered emission. Not recursive: caller supplies suppression.
            Transaction(Registry& registry, bool enabled, const GUID& allocatorEpoch, uint64_t generation) noexcept :
                m_registry(registry)
            {
                if (!enabled) return;
                AcquireSRWLockExclusive(&registry.m_lock);
                m_locked = true;
                m_state = RegistryStatus::Invalid;
                if (!generation || allocatorEpoch == GUID_NULL || generation < registry.m_generation ||
                    (generation == registry.m_generation && allocatorEpoch != registry.m_epoch)) return;
                if (generation != registry.m_generation)
                {
                    for (auto& record : registry.m_records) record = {};
                    registry.m_count = 0;
                    registry.m_generation = generation;
                    registry.m_epoch = allocatorEpoch;
                }
                m_state = RegistryStatus::Emit;
            }
            ~Transaction() noexcept
            {
                if (m_pending) m_registry.Loss();
                if (m_locked) ReleaseSRWLockExclusive(&m_registry.m_lock);
            }
            Transaction(const Transaction&) = delete;
            Transaction& operator=(const Transaction&) = delete;
            uint64_t LossCount() const noexcept { return m_locked ? m_registry.m_losses : 0; }
            void Delivery(bool delivered) noexcept
            {
                if (!m_pending) return;
                if (!delivered) m_registry.Loss();
                m_pending = false;
            }
            RegistryResult Complete(Input input) noexcept
            {
                size_t probes = 0;
                auto fail = [&](RegistryStatus code) noexcept
                {
                    if (m_locked && code != RegistryStatus::Failed) m_registry.Loss();
                    return RegistryResult{code, {}, probes};
                };
                if (m_state != RegistryStatus::Emit) return fail(m_state);
                if (m_completed) return fail(RegistryStatus::Invalid);
                m_completed = true;
                if (!input.Succeeded) return {RegistryStatus::Failed};
                if (input.Kind != Operation::Allocate && input.Kind != Operation::Free &&
                    input.Kind != Operation::Reallocate) return fail(RegistryStatus::Invalid);
                const auto address = input.Kind == Operation::Allocate ? input.ResultAddress : input.PreviousAddress;
                size_t index = m_registry.Find(address, probes);
                if (index == BucketCount) return fail(RegistryStatus::Capacity);
                Record* found = &m_registry.m_records[index];
                if (input.Kind == Operation::Allocate)
                {
                    if (found->Id || !input.ResultAddress) return fail(RegistryStatus::Invalid);
                    if (m_registry.m_count == Capacity) return fail(RegistryStatus::Capacity);
                }
                else if (!found->Id) return fail(RegistryStatus::Unmatched);
                const bool moved = input.Kind == Operation::Reallocate && input.ResultAddress != input.PreviousAddress;
                if (moved)
                {
                    const size_t target = m_registry.Find(input.ResultAddress, probes);
                    if (target == BucketCount || m_registry.m_records[target].Id) return fail(RegistryStatus::Invalid);
                }
                if (input.Kind != Operation::Free)
                {
                    if (m_registry.m_lastId == UINT64_MAX) return fail(RegistryStatus::Exhausted);
                    input.NewId = m_registry.m_lastId + 1;
                }
                Record candidate = *found;
                const auto result = Observe({true, m_registry.m_epoch, false}, input, candidate);
                if (result.Status != Status::Emit) return fail(
                    result.Status == Status::Unmatched ? RegistryStatus::Unmatched : RegistryStatus::Invalid);
                if (input.Kind == Operation::Free) m_registry.Remove(index);
                else if (moved)
                {
                    // Re-key in place at the live-capacity limit: removing the
                    // predecessor makes space without reserving another lifetime.
                    m_registry.Remove(index);
                    index = m_registry.Find(candidate.Address, probes);
                    m_registry.m_records[index] = candidate;
                    ++m_registry.m_count;
                }
                else
                {
                    *found = candidate;
                    if (input.Kind == Operation::Allocate) ++m_registry.m_count;
                }
                if (input.Kind != Operation::Free) m_registry.m_lastId = input.NewId;
                m_pending = true;
                return {RegistryStatus::Emit, result.Payload, probes};
            }
        };
    };
}
