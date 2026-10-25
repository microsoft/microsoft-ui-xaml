#include <Windows.h>
#include <cstdio>
#include <cstring>
#include <thread>
#include "../../../dxaml/test/native/external/framework/profiler/ProfilerLifecycleEtw.h"
#if __has_include("../../../dxaml/xcp/components/allocation/inc/WinUIProfilerAllocatorRuntime.h")
#include "../../../dxaml/xcp/components/allocation/inc/WinUIProfilerAllocatorRuntime.h"
#define HAS_ALLOCATOR_RUNTIME 1
#endif
static int checks, failures;
#define CHECK(x) do { ++checks; if (!(x)) { ++failures; std::printf("FAIL line %d: %s\n", __LINE__, #x); } } while (0)
int main(int argc, char** argv)
{
#ifndef HAS_ALLOCATOR_RUNTIME
    CHECK(false && "actual allocator runtime context not implemented");
#else
    using namespace WinUIProfiler;
    using namespace WinUIProfiler::AllocatorObservation;
    AllocatorRuntime runtime;
    if (argc > 1)
    {
        if (std::strcmp(argv[1], "--inflight-origin") == 0)
        {
            LifecycleEtw::Session session;
            session.Enable(65);
            ProtocolInfo original{};
            CHECK(GetProtocolInfo(ProtocolVersion, sizeof(original), &original) == S_OK);
            HANDLE ready = CreateEventW(nullptr, TRUE, FALSE, nullptr);
            HANDLE resume = CreateEventW(nullptr, TRUE, FALSE, nullptr);
            LifecycleEtw::Require(ready && resume, "create inflight origin test fences");
            bool acquired = false;
            bool resumed = false;
            LARGE_INTEGER acquiredAt{};
            std::thread observer([&]
            {
                AllocatorRuntime held;
                GUID allocatorEpoch{};
                uint64_t generation = 0;
                acquired = held.GetContext(allocatorEpoch, generation);
                QueryPerformanceCounter(&acquiredAt);
                SetEvent(ready);
                resumed = WaitForSingleObject(resume, 10'000) == WAIT_OBJECT_0;
                if (acquired && resumed) held.ReportLoss(1, generation);
            });
            CHECK(WaitForSingleObject(ready, 10'000) == WAIT_OBJECT_0);
            session.Disable();
            session.Enable(65);
            ProtocolInfo current{};
            CHECK(GetProtocolInfo(ProtocolVersion, sizeof(current), &current) == S_OK &&
                current.ProducerEpoch != original.ProducerEpoch);
            SetEvent(resume);
            observer.join();
            CloseHandle(ready);
            CloseHandle(resume);
            CHECK(acquired && resumed && AllocatorReportedLossForTest() == 1);
            const auto observations = session.Read();
            CHECK(!observations.EventsLost && !observations.LogBuffersLost && !observations.RealTimeBuffersLost);
            CHECK(observations.Losses.size() == 1);
            for (const auto& loss : observations.Losses)
            {
                CHECK(loss.Epoch == original.ProducerEpoch && loss.Sequence == 1 && !loss.HasCount);
                CHECK(loss.Begin <= acquiredAt.QuadPart && loss.End >= acquiredAt.QuadPart &&
                    loss.End <= loss.Timestamp);
            }
            std::printf("%d checks, %d failures\n", checks, failures);
            return failures ? 1 : 0;
        }
        if (std::strcmp(argv[1], "--loss-evidence") == 0)
        {
            LifecycleEtw::Session session(argc > 2 && std::strcmp(argv[2], "--retain-etl") == 0);
            session.Enable(65);
            void* first = HeapAlloc(GetProcessHeap(), 0, 8);
            CHECK(first != nullptr && ObservedHeapFree(GetProcessHeap(), 0, first));
            ProtocolInfo original{};
            CHECK(GetProtocolInfo(ProtocolVersion, sizeof(original), &original) == S_OK);
            Emission ordinary;
            CHECK(BeginEmission(Event::Loss, &ordinary) == S_OK);
            LARGE_INTEGER ordinaryTime{};
            QueryPerformanceCounter(&ordinaryTime);
            const DomainReference ordinaryDomains[] = {{9, EventVersion}};
            Loss ordinaryLoss;
            ordinaryLoss.StartQpc = ordinaryTime.QuadPart - 1;
            ordinaryLoss.EndQpc = ordinaryTime.QuadPart;
            ordinaryLoss.Reason = "Test fixture nonallocator coverage gap";
            ordinaryLoss.HasDomains = true;
            ordinaryLoss.Domains = {ordinaryDomains, 1};
            CHECK(Write(ordinary, ordinaryLoss) == S_OK);
            EndEmission(&ordinary);
            session.Enable(64);
            void* missed = HeapAlloc(GetProcessHeap(), 0, 8);
            CHECK(missed != nullptr && ObservedHeapFree(GetProcessHeap(), 0, missed));
            LARGE_INTEGER afterMissed{};
            QueryPerformanceCounter(&afterMissed);
            session.Disable();
            session.Enable(65);
            ProtocolInfo middle{};
            CHECK(GetProtocolInfo(ProtocolVersion, sizeof(middle), &middle) == S_OK &&
                middle.ProducerEpoch != original.ProducerEpoch);
            session.Enable(64);
            void* middleMissed = HeapAlloc(GetProcessHeap(), 0, 8);
            CHECK(middleMissed != nullptr && ObservedHeapFree(GetProcessHeap(), 0, middleMissed));
            session.Disable();
            session.Enable(65);
            void* resumed = ObservedHeapAllocate(GetProcessHeap(), 0, 8);
            CHECK(resumed != nullptr && ObservedHeapFree(GetProcessHeap(), 0, resumed));
            const auto observations = session.Read();
            CHECK(!observations.EventsLost && !observations.LogBuffersLost && !observations.RealTimeBuffersLost);
            bool delayedOriginal = false;
            bool delayedMiddle = false;
            uint64_t originalSequence = 0;
            for (const auto& loss : observations.Losses)
            {
                CHECK(!loss.HasCount); // Coverage gaps are not an exact number of dropped events.
                CHECK(loss.Begin < loss.End && loss.End <= loss.Timestamp);
                if (loss.Epoch == original.ProducerEpoch)
                {
                    CHECK(loss.Sequence == ++originalSequence);
                    if (loss.Begin <= afterMissed.QuadPart && loss.End >= afterMissed.QuadPart)
                        delayedOriginal = true;
                }
                if (loss.Epoch == middle.ProducerEpoch) delayedMiddle = true;
            }
            CHECK(originalSequence == 3 && delayedOriginal && delayedMiddle);
            std::printf("%d checks, %d failures\n", checks, failures);
            return failures ? 1 : 0;
        }
        ConfigureAllocatorForTest(true, std::strcmp(argv[1], "--loss") != 0);
        if (std::strcmp(argv[1], "--loss-overflow") == 0)
        {
            runtime.ReportLoss(UINT64_MAX, 1);
            ProtocolInfo info{};
            CHECK(GetProtocolInfo(ProtocolVersion, sizeof(info), &info) ==
                HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW));
            CHECK(AllocatorReportedLossForTest() == 0);
        }
        else if (std::strcmp(argv[1], "--origin-reuse") == 0)
        {
            for (unsigned i = 0; i != 192; ++i)
            {
                ConfigureAllocatorForTest(true);
                GUID epoch{};
                uint64_t generation = 0;
                CHECK(runtime.GetContext(epoch, generation));
            }
            ProtocolInfo info{};
            CHECK(GetProtocolInfo(ProtocolVersion, sizeof(info), &info) == S_OK);
        }
        else if (std::strcmp(argv[1], "--pending-bound") == 0)
        {
            for (unsigned i = 0; i != 64; ++i)
            {
                ConfigureAllocatorForTest(true, false);
                GUID epoch{};
                uint64_t generation = 0;
                CHECK(runtime.GetContext(epoch, generation));
                runtime.ReportLoss(i + 1, generation);
                CHECK(AllocatorReportedLossForTest() == 0);
            }
            ConfigureAllocatorForTest(true, false);
            GUID epoch{};
            uint64_t generation = 0;
            CHECK(!runtime.GetContext(epoch, generation));
            ProtocolInfo info{};
            CHECK(GetProtocolInfo(ProtocolVersion, sizeof(info), &info) == HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW));
            CHECK(AllocatorReportedLossForTest() == 0);
        }
        else if (std::strcmp(argv[1], "--epochs") == 0)
        {
            ProtocolInfo producer{};
            CHECK(GetProtocolInfo(ProtocolVersion, sizeof(producer), &producer) == S_OK);
            GUID first{}, stable{}, selection{}, renewed{};
            uint64_t firstGeneration = 0, stableGeneration = 0, selectionGeneration = 0, renewedGeneration = 0;
            CHECK(runtime.GetContext(first, firstGeneration) && first != producer.ProducerEpoch);
            CHECK(runtime.GetContext(stable, stableGeneration) && stable == first && stableGeneration == firstGeneration);
            RenewAllocatorSelectionForTest();
            CHECK(runtime.GetContext(selection, selectionGeneration) && selection != first && selectionGeneration > firstGeneration);
            ProtocolInfo sameProducer{};
            CHECK(GetProtocolInfo(ProtocolVersion, sizeof(sameProducer), &sameProducer) == S_OK &&
                sameProducer.ProducerEpoch == producer.ProducerEpoch);
            Allocation previous{};
            previous.AllocationId = 1; previous.AllocatorEpoch = first;
            CHECK(!runtime.Emit(previous, firstGeneration));
            runtime.ReportLoss(1, firstGeneration);
            CHECK(AllocatorReportedLossForTest() == 1);
            void* old = ObservedHeapAllocate(GetProcessHeap(), 0, 8);
            CHECK(old != nullptr);
            RenewAllocatorProducerForTest();
            CHECK(runtime.GetContext(renewed, renewedGeneration) && renewed != selection && renewedGeneration > selectionGeneration);
            ProtocolInfo newProducer{};
            CHECK(GetProtocolInfo(ProtocolVersion, sizeof(newProducer), &newProducer) == S_OK &&
                newProducer.ProducerEpoch != producer.ProducerEpoch && renewed != newProducer.ProducerEpoch);
            previous.AllocatorEpoch = selection;
            CHECK(!runtime.Emit(previous, selectionGeneration));
            CHECK(ObservedHeapFree(GetProcessHeap(), 0, old));
            SequenceCut before{}, after{};
            CHECK(GetSequenceCut(ProtocolVersion, 12, sizeof(before), &before) == S_OK);
            void* current = ObservedHeapAllocate(GetProcessHeap(), 0, 8);
            CHECK(current != nullptr && ObservedHeapFree(GetProcessHeap(), 0, current));
            CHECK(GetSequenceCut(ProtocolVersion, 12, sizeof(after), &after) == S_OK &&
                after.ReservedThrough == after.SettledThrough && after.ReservedThrough == before.ReservedThrough + 2);
        }
        else if (std::strcmp(argv[1], "--race") == 0)
        {
            ArmAllocatorContextRaceForTest();
            GUID snapshot{};
            uint64_t observedGeneration = 0;
            CHECK(runtime.GetContext(snapshot, observedGeneration));
            ProtocolInfo info{};
            CHECK(GetProtocolInfo(ProtocolVersion, sizeof(info), &info) == S_OK);
            ArmAllocatorContextRaceForTest(3);
            CHECK(!runtime.GetContext(snapshot, observedGeneration)); // Bounded exhaustion is explicit loss.
            CHECK(GetProtocolInfo(ProtocolVersion, sizeof(info), &info) == S_OK);
            void* resumed = ObservedHeapAllocate(GetProcessHeap(), 0, 8);
            CHECK(resumed != nullptr && AllocatorReportedLossForTest() == 1);
            CHECK(ObservedHeapFree(GetProcessHeap(), 0, resumed));
        }
        else if (std::strcmp(argv[1], "--writefailure") == 0)
        {
            FailAllocatorLossWriteForTest();
            runtime.ReportLoss(1, 1);
            CHECK(AllocatorReportedLossForTest() == 0);
            ProtocolInfo info{};
            CHECK(GetProtocolInfo(ProtocolVersion, sizeof(info), &info) == E_ACCESSDENIED);
        }
        else
        {
            void* preEnable = HeapAlloc(GetProcessHeap(), 0, 8);
            CHECK(ObservedHeapFree(GetProcessHeap(), 0, preEnable));
            CHECK(AllocatorReportedLossForTest() == 0);
            ProtocolInfo info{};
            CHECK(GetProtocolInfo(ProtocolVersion, sizeof(info), &info) == S_OK);
            ConfigureAllocatorForTest(true);
            void* next = ObservedHeapAllocate(GetProcessHeap(), 0, 8);
            CHECK(next != nullptr);
            CHECK(AllocatorReportedLossForTest() == 1);
            CHECK(ObservedHeapFree(GetProcessHeap(), 0, next));
        }
        std::printf("%d checks, %d failures\n", checks, failures);
        return failures ? 1 : 0;
    }
    CHECK(!runtime.Enabled());
    ConfigureAllocatorForTest(true);
    GUID epoch{};
    uint64_t generation = 0;
    CHECK(runtime.GetContext(epoch, generation) && epoch != GUID_NULL && generation != 0);
    HeapAdapter<AllocatorRuntime> adapter(runtime);
    void* address = adapter.Allocate(GetProcessHeap(), 0, 32);
    CHECK(address != nullptr);
    void* resized = adapter.Resize(GetProcessHeap(), 0, address, 64);
    CHECK(resized != nullptr);
    CHECK(adapter.Resize(GetProcessHeap(), 0, resized, SIZE_MAX) == nullptr);
    CHECK(adapter.Free(GetProcessHeap(), 0, resized));
    void* wired = ObservedHeapAllocate(GetProcessHeap(), 0, 16);
    CHECK(wired != nullptr);
    wired = ObservedHeapResize(GetProcessHeap(), 0, wired, 24);
    CHECK(wired != nullptr && ObservedHeapFree(GetProcessHeap(), 0, wired));
    Allocation stale{};
    stale.AllocationId = 42; stale.AllocatorEpoch = epoch;
    ConfigureAllocatorForTest(false);
    ConfigureAllocatorForTest(true);
    CHECK(!runtime.Emit(stale, generation));
    runtime.ReportLoss(1, generation);
    CHECK(AllocatorReportedLossForTest() == 1);
    runtime.ReportLoss(1, generation);
    CHECK(AllocatorReportedLossForTest() == 1);
    runtime.ReportLoss(2, generation);
    CHECK(AllocatorReportedLossForTest() == 2);
    GUID currentEpoch{};
    uint64_t currentGeneration = 0;
    CHECK(runtime.GetContext(currentEpoch, currentGeneration) && currentEpoch != epoch);
    CHECK(!runtime.Emit(stale, currentGeneration)); // Rejected AFTER reserving an emission.
    runtime.ReportLoss(3, currentGeneration);
    CHECK(AllocatorReportedLossForTest() == 3);
    SequenceCut cut{};
    CHECK(GetSequenceCut(ProtocolVersion, 12, sizeof(cut), &cut) == S_OK);
    CHECK(cut.ReservedThrough == cut.SettledThrough);
    CHECK(!ObserverSuppression::IsSuppressed());
    ConfigureAllocatorForTest(false);
#endif
    std::printf("%d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
