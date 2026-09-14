// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <cstdint>

// Usage, reliability and performance signals for the inking stack. See docs/Telemetry.md for the
// proposal and the measures each event feeds.
namespace InkTelemetry
{
    // Bumped when a field is added, removed or redefined so queries can pin a shape.
    inline constexpr uint32_t SchemaVersion = 1;

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

    enum class Result : uint32_t
    {
        Started = 0,
        Success = 1,
        Failure = 2,
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
    };

    // Owned by one InkToolbar and only touched on its UI thread.
    struct ToolbarState
    {
        ToolbarState() = default;
        ToolbarState(ToolbarState const&) = delete;
        ToolbarState& operator=(ToolbarState const&) = delete;

        uint64_t id{};
        bool usageReported{};
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
}
