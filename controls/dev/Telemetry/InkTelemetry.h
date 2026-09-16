// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <cstdint>

// Usage, reliability and performance signals for the inking stack.
namespace InkTelemetry
{
    // Bumped when a field is added, removed or redefined so queries can pin a shape.
    inline constexpr uint32_t SchemaVersion = 2;

    // Which composition engine the canvas attached through. This is the dimension we most need:
    // the two paths have materially different rendering and input behaviour.
    enum class CompositorEngine : uint32_t
    {
        Unknown = 0,
        Lifted = 1,
        System = 2,
    };

    // Last initialization step entered, so a failure can be attributed without a stack.
    enum class InitializationStage : uint32_t
    {
        None = 0,
        InkPresenter = 1,
        CompositionDevice = 2,
        VisualLink = 3,
        PresenterSize = 4,
    };

    // Terminal outcomes match the WinUI 3 controls telemetry spec's Outcome dimension
    // (Started is the non-terminal begin marker). Cancelled covers a recoverable operation that
    // never proceeded (e.g. an API call made before the control was ready).
    enum class Result : uint32_t
    {
        Started = 0,
        Success = 1,
        Failure = 2,
        Cancelled = 3,
    };

    // Coarse bucket for a control-attributable failure, so error rate can be split without a stack.
    enum class ErrorCategory : uint32_t
    {
        Unknown = 0,
        Initialization = 1,
        Composition = 2,
        InkThread = 3,
        ApiMisuse = 4,
        Rendering = 5,
    };

    // The operation that failed. Kept as an enum rather than a string so no caller can leak content.
    enum class Operation : uint32_t
    {
        Unknown = 0,
        AttachToCompositor = 1,
        CreateInkPresenter = 2,
        SizePresenter = 3,
        ActivateCustomDrying = 4,
        BeginDry = 5,
        EndDry = 6,
    };

    // Owned by one InkCanvas and only touched on its UI thread.
    struct CanvasState
    {
        CanvasState() = default;
        CanvasState(CanvasState const&) = delete;
        CanvasState& operator=(CanvasState const&) = delete;

        uint64_t id{};
        uint64_t initializationStartedMicroseconds{};
        InitializationStage stage{};
        bool initializationInFlight{};
        bool usageReported{};

        // Engagement counters, rolled up once in the session summary rather than emitted per stroke.
        uint64_t activatedMicroseconds{};
        uint64_t firstInkMicroseconds{};
        uint32_t activationCount{};
        uint32_t strokesCollected{};
        uint32_t strokesErased{};
        uint32_t errorCount{};
        bool summaryReported{};
    };

    // Owned by one InkToolbar and only touched on its UI thread.
    struct ToolbarState
    {
        ToolbarState() = default;
        ToolbarState(ToolbarState const&) = delete;
        ToolbarState& operator=(ToolbarState const&) = delete;

        uint64_t id{};
        bool usageReported{};

        uint32_t toolSwitchCount{};
        // Bit per InkToolbarTool value, so "how many distinct tools were used" needs no allocation.
        uint32_t toolsUsedMask{};
        bool summaryReported{};
    };

    bool IsCanvasEnabled() noexcept;
    bool IsToolbarEnabled() noexcept;

    void BeginCanvasInitialization(CanvasState& state) noexcept;
    void SetCanvasInitializationStage(CanvasState& state, InitializationStage stage) noexcept;

    // Records the outcome and, on success, the elapsed initialization latency. Safe to call when no
    // attempt is in flight; it no-ops.
    void CompleteCanvasInitialization(CanvasState& state, Result result, CompositorEngine engine, int32_t hr = 0) noexcept;

    void ReportCanvasUsage(
        CanvasState& state,
        CompositorEngine engine,
        uint32_t inputDeviceTypes,
        uint32_t highContrastAdjustment) noexcept;

    // Custom drying is an explicit app action with its own failure mode, so it is reported where it
    // happens rather than sampled as state (activation ordering against Loaded is not guaranteed).
    void ReportCustomDryingActivation(Result result, int32_t hr = 0) noexcept;

    void ReportToolbarUsage(
        ToolbarState& state,
        uint32_t initialControls,
        uint32_t orientation,
        uint32_t activeToolKind,
        bool targetsCanvas,
        bool targetsPresenter) noexcept;

    // Control-attributable failure, reported wherever it is detected. Also increments the canvas
    // error count when a canvas is in scope, so the session summary carries an error total.
    void ReportError(
        ErrorCategory category,
        Operation operation,
        bool isRecoverable,
        int32_t hr = 0,
        CanvasState* state = nullptr) noexcept;

    void RecordStrokesCollected(CanvasState& state, uint32_t count) noexcept;
    void RecordStrokesErased(CanvasState& state, uint32_t count) noexcept;
    void RecordToolSwitch(ToolbarState& state, uint32_t toolKind) noexcept;

    // One roll-up per control instance, emitted as it is torn down.
    void ReportCanvasSessionSummary(CanvasState& state, CompositorEngine engine) noexcept;
    void ReportToolbarSessionSummary(ToolbarState& state) noexcept;

    // Collapses a raw count to a bucket (0, 1, 2–4, 5–16, 17+), matching the Charts telemetry
    // convention so stroke/tool counts are comparable across controls and bounded in cardinality.
    uint32_t CountBucket(uint32_t count) noexcept;
}
