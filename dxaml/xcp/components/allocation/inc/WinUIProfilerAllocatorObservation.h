// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
#pragma once
#include "WinUIProfilerTracing.h"

namespace WinUIProfiler::AllocatorObservation
{
    enum class Operation { Allocate, Free, Reallocate };
    enum class Status { Disabled, Failed, Unmatched, Invalid, Emit };
    struct Context
    {
        bool Enabled;
        GUID Epoch;
        bool CompleteBaseline;
    };
    // Caller-owned entry in a future bounded registry; Address is only a transient join.
    struct Record
    {
        GUID Epoch{};
        uint64_t Id = 0;
        uintptr_t Address = 0;
        uint64_t Requested = 0;
        Optional<uint64_t> Actual{};
    };
    struct Input
    {
        Operation Kind;
        uintptr_t PreviousAddress;
        uintptr_t ResultAddress;
        uint64_t Requested;
        Optional<uint64_t> Actual;
        bool Succeeded;
        uint64_t NewId;
    };
    struct Result
    {
        Status Status;
        Allocation Payload{};
    };

    // Called after the real Heap* operation. Never queries a heap or invents a
    // predecessor for late-enable traffic. Non-Emit outcomes carry no wire event.
    inline Result Observe(const Context& context, const Input& input, Record& record) noexcept
    {
        if (!context.Enabled) return {Status::Disabled};
        if (!input.Succeeded) return {Status::Failed};
        if (context.Epoch == GUID_NULL) return {Status::Invalid};
        if (input.Kind != Operation::Allocate && input.Kind != Operation::Free &&
            input.Kind != Operation::Reallocate) return {Status::Invalid};
        if (input.Kind != Operation::Allocate &&
            (!input.PreviousAddress || !record.Id || record.Address != input.PreviousAddress ||
                record.Epoch != context.Epoch)) return {Status::Unmatched};
        if (input.Kind != Operation::Free &&
            (!input.ResultAddress || !input.NewId || input.NewId == record.Id ||
                (input.Actual.Present && input.Actual.Value < input.Requested)))
            return {Status::Invalid};
        if (input.Kind == Operation::Allocate && record.Id) return {Status::Invalid};

        Allocation payload{};
        payload.AllocatorEpoch = context.Epoch;
        payload.DataQuality = {0, static_cast<uint8_t>(context.CompleteBaseline ? 0 : 1),
            context.CompleteBaseline ? nullptr : "pre-enable allocations or observation loss not reconstructed"};
        if (input.Kind == Operation::Free)
        {
            payload.AllocationId = record.Id;
            payload.Operation = 1;
            payload.RequestedBytes = record.Requested;
            payload.ActualBytes = record.Actual;
            record = {};
        }
        else
        {
            payload.AllocationId = input.NewId;
            payload.Operation = input.Kind == Operation::Reallocate ? 2 : 0;
            if (input.Kind == Operation::Reallocate) payload.PreviousAllocationId = {true, record.Id};
            payload.RequestedBytes = input.Requested;
            payload.ActualBytes = input.Actual;
            record = {context.Epoch, input.NewId, input.ResultAddress, input.Requested, input.Actual};
        }
        return {Status::Emit, payload};
    }
}
