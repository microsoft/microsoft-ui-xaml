#include <Windows.h>
#include <cstdio>
#if __has_include("../../../dxaml/xcp/components/allocation/inc/WinUIProfilerAllocatorHeapAdapter.h")
#include "../../../dxaml/xcp/components/allocation/inc/WinUIProfilerAllocatorHeapAdapter.h"
#define HAS_HEAP_ADAPTER 1
#endif
static int checks, failures;
#define CHECK(x) do { ++checks; if (!(x)) { ++failures; std::printf("FAIL line %d: %s\n", __LINE__, #x); } } while (0)
#ifdef HAS_HEAP_ADAPTER
using namespace WinUIProfiler;
using namespace WinUIProfiler::AllocatorObservation;
struct Runtime
{
    Registry<2> Table;
    bool EnabledValue = true, Deliver = true, ContextAvailable = true, SizeFails = false;
    uint64_t Generation = 1, Losses = 0;
    unsigned ContextCalls = 0, RegistryCalls = 0, SizeCalls = 0, Emissions = 0;
    Allocation Last{};
    bool Enabled() noexcept { SetLastError(111); return EnabledValue; }
    bool GetContext(GUID& epoch, uint64_t& generation) noexcept
    { ++ContextCalls; epoch = {1}; generation = Generation; return ContextAvailable; }
    Registry<2>& GetRegistry() noexcept { ++RegistryCalls; return Table; }
    SIZE_T Size(HANDLE heap, const void* address) noexcept
    { ++SizeCalls; return SizeFails ? SIZE_MAX : HeapSize(heap, 0, address); }
    bool Emit(const Allocation& payload, uint64_t generation) noexcept
    { ++Emissions; Last = payload; return Deliver && generation == Generation; }
    void ReportLoss(uint64_t total, uint64_t) noexcept { Losses = total; SetLastError(222); }
};
#endif
int main()
{
#ifndef HAS_HEAP_ADAPTER
    CHECK(false && "real Windows heap adapter not implemented");
#else
    HANDLE heap = HeapCreate(0, 0, 0);
    CHECK(heap != nullptr);
    Runtime runtime;
    HeapAdapter<Runtime> adapter(runtime);
    runtime.EnabledValue = false;
    SetLastError(901);
    void* disabled = adapter.Allocate(heap, 0, 32);
    CHECK(disabled != nullptr && GetLastError() == 901);
    CHECK(runtime.ContextCalls == 0 && runtime.RegistryCalls == 0 && runtime.SizeCalls == 0);
    CHECK(adapter.Free(heap, 0, disabled));
    runtime.EnabledValue = true;
    {
        ObserverSuppression::Scope scope;
        void* suppressed = adapter.Allocate(heap, 0, 32);
        CHECK(suppressed != nullptr);
        CHECK(adapter.Free(heap, 0, suppressed));
        CHECK(runtime.ContextCalls == 0 && runtime.RegistryCalls == 0 && runtime.SizeCalls == 0);
    }
    void* address = adapter.Allocate(heap, HEAP_ZERO_MEMORY, 32);
    CHECK(address != nullptr && runtime.Last.Operation == 0 && runtime.Last.RequestedBytes == 32);
    CHECK(runtime.Last.ActualBytes.Present && runtime.Last.ActualBytes.Value >= 32);
    const auto firstId = runtime.Last.AllocationId;
    const auto emissions = runtime.Emissions;
    SetLastError(777);
    CHECK(HeapReAlloc(heap, 0, address, SIZE_MAX) == nullptr);
    const DWORD expectedError = GetLastError();
    SetLastError(777);
    CHECK(adapter.Resize(heap, 0, address, SIZE_MAX) == nullptr);
    CHECK(GetLastError() == expectedError);
    CHECK(runtime.Emissions == emissions);
    void* resized = adapter.Resize(heap, 0, address, 64);
    CHECK(resized != nullptr && runtime.Last.Operation == 2);
    CHECK(runtime.Last.PreviousAllocationId.Value == firstId && runtime.Last.AllocationId != firstId);
    CHECK(adapter.Free(heap, 0, resized) && runtime.Last.Operation == 1);
    CHECK(runtime.Last.RequestedBytes == 64);
    runtime.Deliver = false;
    void* lost = adapter.Allocate(heap, 0, 8);
    CHECK(lost != nullptr && runtime.Losses == 1);
    runtime.Deliver = true;
    ++runtime.Generation;
    CHECK(adapter.Free(heap, 0, lost));
    CHECK(runtime.Losses == 2); // Pre-enable predecessor was invalidated.
    CHECK(runtime.Last.DataQuality.Coverage == 1);
    runtime.SizeFails = true;
    void* unknown = adapter.Allocate(heap, 0, 16);
    CHECK(unknown != nullptr && !runtime.Last.ActualBytes.Present);
    CHECK(adapter.Free(heap, 0, unknown));
    const auto beforeFailure = runtime.Emissions;
    CHECK(adapter.Allocate(heap, 0, SIZE_MAX) == nullptr);
    CHECK(runtime.Emissions == beforeFailure);
    runtime.ContextAvailable = false;
    const auto registryCalls = runtime.RegistryCalls;
    void* noContext = adapter.Allocate(heap, 0, 8);
    CHECK(noContext != nullptr && runtime.RegistryCalls == registryCalls);
    CHECK(adapter.Free(heap, 0, noContext));
    CHECK(runtime.RegistryCalls == registryCalls);
    CHECK(!ObserverSuppression::IsSuppressed());
    CHECK(HeapDestroy(heap));
#endif
    std::printf("%d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
