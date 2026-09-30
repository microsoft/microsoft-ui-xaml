// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <cstdint>
#include <guiddef.h>

namespace TableViewTelemetry
{
    enum class Result : uint32_t { Started = 0, Success = 1, Failure = 2, Cancelled = 3 };
    enum class Operation : uint32_t { InitialLayout = 0, ReplaceSource = 1, Sort = 2, HeaderRefresh = 3, Layout = 4 };
    enum class Origin : uint32_t { Template = 0, Loaded = 1 };
    enum class Content : uint32_t { Empty = 0, Rows = 1, GroupHeaders = 2 };
    enum class Stage : uint32_t { None = 0, Template = 1, Layout = 2, Source = 3, Sort = 4, HeaderRefresh = 5 };
    enum class InitialState { NotStarted, Started, Finished };

    struct Configuration
    {
        Content content{};
        uint32_t columnCountBucket{};
        bool grouped{};
        bool available{ true };
    };

    // One control's UI-thread state; never retains an item, column or visual.
    struct State
    {
        State() = default;
        State(State const&) = delete;
        State& operator=(State const&) = delete;

        GUID id{};
        InitialState initial{};
        Origin origin{};
        uint64_t initialStarted{};
        uint64_t operationStarted{};
        uint64_t operationGeneration{};
        Operation operation{};
        uint32_t reportedErrors{};
        bool usageReported{};
    };

    bool IsEnabled() noexcept;
    bool NeedsLayout(State const& state) noexcept;
    void BeginInitial(State& state, Origin origin) noexcept;
    void CompleteInitial(State& state, Result result, Stage stage = Stage::None) noexcept;
    void ReportUsage(State& state, Configuration const& configuration) noexcept;
    uint64_t BeginOperation(State& state, Operation operation) noexcept;
    void CompleteOperation(State& state, Result result) noexcept;
    void FailOperation(State& state, Operation operation, uint64_t generation, Stage stage) noexcept;
    void ReportError(State& state, Operation operation, Stage stage, bool recoverable) noexcept;
    uint32_t CountBucket(uint32_t count) noexcept;
}
