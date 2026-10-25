#include <cstdio>
#if __has_include("../../../dxaml/xcp/components/allocation/inc/WinUIProfilerAllocatorRegistry.h")
#include "../../../dxaml/xcp/components/allocation/inc/WinUIProfilerAllocatorRegistry.h"
#define HAS_REGISTRY 1
#endif
static int checks, failures;
#define CHECK(x) do { ++checks; if (!(x)) { ++failures; std::printf("FAIL line %d: %s\n", __LINE__, #x); } } while (0)
#ifdef HAS_REGISTRY
using namespace WinUIProfiler::AllocatorObservation;
using Table = Registry<2>;
static GUID epoch{1};
static Input Allocate(uintptr_t address) { return {Operation::Allocate, 0, address, 8, {}, true, 0}; }
struct Worker { Table* Table; HANDLE Started; HANDLE Finished; uint64_t Id; };
static DWORD WINAPI Reuse(void* value)
{
    auto& worker = *static_cast<Worker*>(value);
    SetEvent(worker.Started);
    Table::Transaction transaction(*worker.Table, true, epoch, 1);
    auto result = transaction.Complete(Allocate(100));
    worker.Id = result.Payload.AllocationId;
    transaction.Delivery(true);
    SetEvent(worker.Finished);
    return 0;
}
#endif
int main()
{
#ifndef HAS_REGISTRY
    CHECK(false && "bounded allocator registry not implemented");
#else
    Table table;
    uint64_t first = 0;
    {
        Table::Transaction tx(table, true, epoch, 1);
        auto result = tx.Complete(Allocate(100));
        CHECK(result.Code == RegistryStatus::Emit && result.Payload.DataQuality.Coverage == 1);
        first = result.Payload.AllocationId;
        tx.Delivery(true);
    }
    {
        Table::Transaction tx(table, true, epoch, 1);
        CHECK(tx.Complete(Allocate(200)).Code == RegistryStatus::Emit);
        tx.Delivery(true);
    }
    {
        Table::Transaction tx(table, true, epoch, 1);
        CHECK(tx.Complete(Allocate(300)).Code == RegistryStatus::Capacity);
        CHECK(tx.LossCount() == 1);
    }
    {
        Table::Transaction tx(table, true, epoch, 1);
        auto input = Allocate(100);
        input.Kind = Operation::Reallocate; input.PreviousAddress = 100;
        input.Succeeded = false;
        CHECK(tx.Complete(input).Code == RegistryStatus::Failed);
    }
    Worker worker{&table, CreateEvent(nullptr, TRUE, FALSE, nullptr),
        CreateEvent(nullptr, TRUE, FALSE, nullptr), 0};
    HANDLE thread = nullptr;
    {
        Table::Transaction tx(table, true, epoch, 1); // Before the simulated HeapFree.
        thread = CreateThread(nullptr, 0, Reuse, &worker, 0, nullptr);
        CHECK(thread != nullptr);
        CHECK(WaitForSingleObject(worker.Started, 2000) == WAIT_OBJECT_0);
        CHECK(WaitForSingleObject(worker.Finished, 30) == WAIT_TIMEOUT);
        Input input{Operation::Free, 100, 0, 0, {}, true, 0};
        auto result = tx.Complete(input);
        CHECK(result.Code == RegistryStatus::Emit && result.Payload.AllocationId == first);
        tx.Delivery(true);
    }
    CHECK(WaitForSingleObject(thread, 2000) == WAIT_OBJECT_0);
    CHECK(worker.Id > first);
    CloseHandle(thread); CloseHandle(worker.Started); CloseHandle(worker.Finished);
    {
        Table::Transaction tx(table, true, epoch, 2);
        CHECK(tx.Complete({Operation::Free, 100, 0, 0, {}, true, 0}).Code == RegistryStatus::Unmatched);
    }
    {
        Table::Transaction tx(table, true, epoch, 2);
        auto result = tx.Complete(Allocate(100));
        CHECK(result.Code == RegistryStatus::Emit && result.Payload.AllocationId > worker.Id);
        // No Delivery acknowledgement: destructor explicitly records loss.
    }
    {
        Table::Transaction tx(table, true, epoch, 2);
        CHECK(tx.LossCount() == 3);
        CHECK(tx.Complete(Allocate(100)).Code == RegistryStatus::Invalid);
    }
    {
        Table::Transaction tx(table, false, epoch, 3);
        CHECK(tx.Complete(Allocate(400)).Code == RegistryStatus::Disabled);
    }
    {
        Table::Transaction tx(table, true, epoch, 1);
        CHECK(tx.Complete(Allocate(400)).Code == RegistryStatus::Invalid);
    }
    Registry<1> exhausted(UINT64_MAX);
    {
        Registry<1>::Transaction tx(exhausted, true, epoch, 1);
        CHECK(tx.Complete(Allocate(1)).Code == RegistryStatus::Exhausted);
    }
    Registry<1> finalId(UINT64_MAX - 1);
    {
        Registry<1>::Transaction tx(finalId, true, epoch, 1);
        CHECK(tx.Complete(Allocate(1)).Payload.AllocationId == UINT64_MAX);
        tx.Delivery(false);
        CHECK(tx.LossCount() == 1);
    }
#endif
#ifdef HAS_REGISTRY
#ifndef WINUI_PROFILER_HASH_REGISTRY
    CHECK(false && "hash registry lookup not implemented");
#else
    using CollisionTable = Registry<3>;
    CollisionTable collisions;
    uintptr_t addresses[4]{};
    size_t count = 0;
    for (uintptr_t address = 16; count != 4; address += 16)
        if (CollisionTable::HomeBucket(address) == 0) addresses[count++] = address;
    auto apply = [&](Input input, uint64_t generation = 1)
    {
        CollisionTable::Transaction tx(collisions, true, epoch, generation);
        auto result = tx.Complete(input);
        if (result.Code == RegistryStatus::Emit) tx.Delivery(true);
        return result;
    };
    auto a = apply(Allocate(addresses[0]));
    auto b = apply(Allocate(addresses[1]));
    auto c = apply(Allocate(addresses[2]));
    CHECK(a.Probes == 1 && b.Probes == 2 && c.Probes == 3);
    CHECK(apply({Operation::Free, addresses[0], 0, 0, {}, true, 0}).Code == RegistryStatus::Emit);
    CHECK(apply({Operation::Free, addresses[2], 0, 0, {}, true, 0}).Payload.AllocationId == c.Payload.AllocationId);
    CHECK(apply(Allocate(addresses[0])).Payload.AllocationId > c.Payload.AllocationId);
    CHECK(apply(Allocate(addresses[2])).Code == RegistryStatus::Emit);
    auto same = apply({Operation::Reallocate, addresses[1], addresses[1], 16, {}, true, 0});
    CHECK(same.Code == RegistryStatus::Emit && same.Payload.PreviousAllocationId.Value == b.Payload.AllocationId);
    auto moved = apply({Operation::Reallocate, addresses[1], addresses[3], 24, {}, true, 0});
    CHECK(moved.Code == RegistryStatus::Emit && moved.Payload.PreviousAllocationId.Value == same.Payload.AllocationId);
    CHECK(apply({Operation::Reallocate, addresses[3], addresses[1], 40, {}, false, 0}).Code == RegistryStatus::Failed);
    CHECK(apply({Operation::Free, addresses[3], 0, 0, {}, true, 0}).Payload.AllocationId == moved.Payload.AllocationId);
    CHECK(apply(Allocate(addresses[1])).Code == RegistryStatus::Emit);
    CHECK(apply(Allocate(addresses[3])).Code == RegistryStatus::Capacity);
    auto reset = apply(Allocate(addresses[0]), 2);
    CHECK(reset.Code == RegistryStatus::Emit && reset.Probes == 1);
    CHECK(apply({Operation::Free, addresses[2], 0, 0, {}, true, 0}, 2).Code == RegistryStatus::Unmatched);
    CollisionTable wraparound;
    uintptr_t wrap[3]{};
    count = 0;
    for (uintptr_t address = 16; count != 3; address += 16)
        if (CollisionTable::HomeBucket(address) == 6) wrap[count++] = address;
    auto applyWrap = [&](Input input)
    {
        CollisionTable::Transaction tx(wraparound, true, epoch, 1);
        auto result = tx.Complete(input);
        if (result.Code == RegistryStatus::Emit) tx.Delivery(true);
        return result;
    };
    CHECK(applyWrap(Allocate(wrap[0])).Probes == 1);
    CHECK(applyWrap(Allocate(wrap[1])).Probes == 2);
    auto tail = applyWrap(Allocate(wrap[2]));
    CHECK(tail.Probes == 3);
    CHECK(applyWrap({Operation::Free, wrap[1], 0, 0, {}, true, 0}).Code == RegistryStatus::Emit);
    CHECK(applyWrap({Operation::Free, wrap[2], 0, 0, {}, true, 0}).Payload.AllocationId == tail.Payload.AllocationId);
    CHECK(applyWrap(Allocate(wrap[1])).Code == RegistryStatus::Emit);
#endif
#endif
    std::printf("%d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
