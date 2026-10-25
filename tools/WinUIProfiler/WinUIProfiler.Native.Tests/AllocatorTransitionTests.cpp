#include <cstdio>
#if __has_include("../../../dxaml/xcp/components/allocation/inc/WinUIProfilerAllocatorObservation.h")
#include "../../../dxaml/xcp/components/allocation/inc/WinUIProfilerAllocatorObservation.h"
#define HAS_ALLOCATOR_OBSERVATION 1
#endif
static int checks, failures;
#define CHECK(x) do { ++checks; if (!(x)) { ++failures; std::printf("FAIL line %d: %s\n", __LINE__, #x); } } while (0)
int main()
{
#ifndef HAS_ALLOCATOR_OBSERVATION
    CHECK(false && "allocator transition foundation not implemented");
#else
    using namespace WinUIProfiler::AllocatorObservation;
    GUID epoch{1}, other{2};
    Context context{true, epoch, false};
    Record record{};
    Input input{Operation::Allocate, 0, 100, 12, {false, 0}, true, 1};
    auto result = Observe(context, input, record);
    CHECK(result.Status == Status::Emit && result.Payload.Operation == 0);
    CHECK(record.Id == 1 && record.Address == 100 && record.Requested == 12);
    CHECK(!result.Payload.ActualBytes.Present && result.Payload.DataQuality.Coverage == 1);
    CHECK(!result.Payload.BaselineCaptureId.Present);
    input = {Operation::Reallocate, 100, 0, 99, {}, false, 2};
    result = Observe(context, input, record);
    CHECK(result.Status == Status::Failed && record.Id == 1 && record.Requested == 12);
    input = {Operation::Reallocate, 100, 100, 8, {true, 16}, true, 2};
    result = Observe(context, input, record);
    CHECK(result.Status == Status::Emit && record.Id == 2 && record.Address == 100);
    CHECK(result.Payload.Operation == 2 && result.Payload.PreviousAllocationId.Value == 1);
    CHECK(result.Payload.ActualBytes.Present && result.Payload.ActualBytes.Value == 16);
    input = {Operation::Reallocate, 100, 200, 24, {}, true, 3};
    result = Observe(context, input, record);
    CHECK(result.Status == Status::Emit && record.Id == 3 && record.Address == 200);
    CHECK(result.Payload.PreviousAllocationId.Value == 2);
    input = {Operation::Free, 200, 0, 0, {}, false, 0};
    CHECK(Observe(context, input, record).Status == Status::Failed && record.Id == 3);
    input.Succeeded = true;
    result = Observe(context, input, record);
    CHECK(result.Status == Status::Emit && result.Payload.AllocationId == 3 && record.Id == 0);
    CHECK(result.Payload.RequestedBytes == 24 && !result.Payload.PreviousAllocationId.Present);
    CHECK(Observe(context, input, record).Status == Status::Unmatched);
    input = {Operation::Allocate, 0, 0, 12, {}, false, 4};
    CHECK(Observe(context, input, record).Status == Status::Failed && record.Id == 0);
    input = {Operation::Allocate, 0, 300, 0, {}, true, 4};
    CHECK(Observe(context, input, record).Status == Status::Emit && record.Requested == 0);
    input = {Operation::Reallocate, 300, 300, 0, {}, true, 5};
    CHECK(Observe(context, input, record).Status == Status::Emit && record.Id == 5);
    input = {Operation::Reallocate, 0, 400, 12, {}, true, 6};
    CHECK(Observe(context, input, record).Status == Status::Unmatched && record.Id == 5);
    input = {Operation::Reallocate, 300, 400, 12, {}, true, 5};
    CHECK(Observe(context, input, record).Status == Status::Invalid && record.Id == 5);
    input.NewId = 6;
    context.Epoch = other;
    CHECK(Observe(context, input, record).Status == Status::Unmatched);
    context.Epoch = epoch;
    context.Enabled = false;
    CHECK(Observe(context, input, record).Status == Status::Disabled && record.Id == 5);
    context.Enabled = true;
    input.Actual = {true, 8};
    CHECK(Observe(context, input, record).Status == Status::Invalid && record.Id == 5);
    input.Actual = {};
    context.CompleteBaseline = true;
    result = Observe(context, input, record);
    CHECK(result.Status == Status::Emit && result.Payload.DataQuality.Coverage == 0);
    CHECK(record.Id == 6);
#endif
    std::printf("%d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
