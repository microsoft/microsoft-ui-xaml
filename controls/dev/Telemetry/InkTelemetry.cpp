// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "InkTelemetry.h"
#include "MuxcTraceLogging.h"

#include <atomic>
#include <chrono>

namespace InkTelemetry
{
    namespace
    {
        // Instance ids are process-local and let a start be matched to its outcome. They are not
        // stable across runs and carry no user identity.
        std::atomic<uint64_t> g_nextId{ 0 };

        uint64_t Timestamp() noexcept
        {
            return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count());
        }

        uint64_t NextId() noexcept
        {
            return g_nextId.fetch_add(1, std::memory_order_relaxed) + 1;
        }

        bool IsProviderEnabled(uint64_t keyword) noexcept
        {
            return g_IsTelemetryProviderEnabled &&
                g_TelemetryProviderLevel >= WINEVENT_LEVEL_INFO &&
                (g_TelemetryProviderMatchAnyKeyword & keyword || g_TelemetryProviderMatchAnyKeyword == 0);
        }
    }

    bool IsCanvasEnabled() noexcept
    {
        return IsProviderEnabled(KEYWORD_INKCANVAS);
    }

    bool IsToolbarEnabled() noexcept
    {
        return IsProviderEnabled(KEYWORD_INKTOOLBAR);
    }

    void BeginCanvasInitialization(CanvasState& state) noexcept
    {
        if (state.initializationInFlight || !IsCanvasEnabled())
        {
            return;
        }

        if (!state.id)
        {
            state.id = NextId();
        }

        state.initializationInFlight = true;
        state.stage = InitializationStage::None;
        state.initializationStartedMicroseconds = Timestamp();

        TraceLoggingWrite(
            g_hTelemetryProvider,
            "InkCanvas_Initialization",
            TraceLoggingUInt32(SchemaVersion, "SchemaVersion"),
            TraceLoggingUInt64(state.id, "InkCanvasId"),
            TraceLoggingUInt32(static_cast<uint32_t>(Result::Started), "Result"),
            TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance),
            TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES),
            TraceLoggingKeyword(KEYWORD_INKCANVAS));
    }

    void SetCanvasInitializationStage(CanvasState& state, InitializationStage stage) noexcept
    {
        if (state.initializationInFlight)
        {
            state.stage = stage;
        }
    }

    void CompleteCanvasInitialization(CanvasState& state, Result result, CompositorEngine engine, int32_t hr) noexcept
    {
        if (!state.initializationInFlight)
        {
            return;
        }

        state.initializationInFlight = false;

        auto const started = state.initializationStartedMicroseconds;
        state.initializationStartedMicroseconds = 0;

        if (!IsCanvasEnabled())
        {
            return;
        }

        auto const elapsedMicroseconds = Timestamp() - started;

        TraceLoggingWrite(
            g_hTelemetryProvider,
            "InkCanvas_Initialization",
            TraceLoggingUInt32(SchemaVersion, "SchemaVersion"),
            TraceLoggingUInt64(state.id, "InkCanvasId"),
            TraceLoggingUInt32(static_cast<uint32_t>(result), "Result"),
            TraceLoggingUInt32(static_cast<uint32_t>(state.stage), "FailureStage"),
            TraceLoggingHResult(hr, "HResult"),
            TraceLoggingUInt32(static_cast<uint32_t>(engine), "CompositorEngine"),
            TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance),
            TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES),
            TraceLoggingKeyword(KEYWORD_INKCANVAS));

        if (result == Result::Success)
        {
            TraceLoggingWrite(
                g_hTelemetryProvider,
                "InkCanvas_InitializationLatency",
                TraceLoggingUInt32(SchemaVersion, "SchemaVersion"),
                TraceLoggingUInt64(state.id, "InkCanvasId"),
                TraceLoggingFloat64(elapsedMicroseconds / 1000.0, "ElapsedMs"),
                TraceLoggingUInt32(static_cast<uint32_t>(engine), "CompositorEngine"),
                TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance),
                TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES),
                TraceLoggingKeyword(KEYWORD_INKCANVAS));
        }
    }

    void ReportCanvasUsage(
        CanvasState& state,
        CompositorEngine engine,
        uint32_t inputDeviceTypes,
        uint32_t highContrastAdjustment) noexcept
    {
        if (state.usageReported || !IsCanvasEnabled())
        {
            return;
        }

        if (!state.id)
        {
            state.id = NextId();
        }

        state.usageReported = true;

        TraceLoggingWrite(
            g_hTelemetryProvider,
            "InkCanvas_UsageSummary",
            TraceLoggingUInt32(SchemaVersion, "SchemaVersion"),
            TraceLoggingUInt64(state.id, "InkCanvasId"),
            TraceLoggingUInt32(static_cast<uint32_t>(engine), "CompositorEngine"),
            TraceLoggingUInt32(inputDeviceTypes, "InputDeviceTypes"),
            TraceLoggingUInt32(highContrastAdjustment, "HighContrastAdjustment"),
            TelemetryPrivacyDataTag(PDT_ProductAndServiceUsage),
            TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES),
            TraceLoggingKeyword(KEYWORD_INKCANVAS));
    }

    void ReportCustomDryingActivation(Result result, int32_t hr) noexcept
    {
        if (!IsCanvasEnabled())
        {
            return;
        }

        TraceLoggingWrite(
            g_hTelemetryProvider,
            "InkPresenter_CustomDryingActivation",
            TraceLoggingUInt32(SchemaVersion, "SchemaVersion"),
            TraceLoggingUInt32(static_cast<uint32_t>(result), "Result"),
            TraceLoggingHResult(hr, "HResult"),
            TelemetryPrivacyDataTag(PDT_ProductAndServiceUsage),
            TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES),
            TraceLoggingKeyword(KEYWORD_INKCANVAS));
    }

    void ReportToolbarUsage(
        ToolbarState& state,
        uint32_t initialControls,
        uint32_t orientation,
        uint32_t activeToolKind,
        bool targetsCanvas,
        bool targetsPresenter) noexcept
    {
        if (state.usageReported || !IsToolbarEnabled())
        {
            return;
        }

        if (!state.id)
        {
            state.id = NextId();
        }

        state.usageReported = true;

        TraceLoggingWrite(
            g_hTelemetryProvider,
            "InkToolbar_UsageSummary",
            TraceLoggingUInt32(SchemaVersion, "SchemaVersion"),
            TraceLoggingUInt64(state.id, "InkToolbarId"),
            TraceLoggingUInt32(initialControls, "InitialControls"),
            TraceLoggingUInt32(orientation, "Orientation"),
            TraceLoggingUInt32(activeToolKind, "ActiveToolKind"),
            TraceLoggingBoolean(targetsCanvas, "TargetsInkCanvas"),
            TraceLoggingBoolean(targetsPresenter, "TargetsInkPresenter"),
            TelemetryPrivacyDataTag(PDT_ProductAndServiceUsage),
            TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES),
            TraceLoggingKeyword(KEYWORD_INKTOOLBAR));
    }
}
