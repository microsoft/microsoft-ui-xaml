// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "InkTelemetry.h"
#include "MuxcTraceLogging.h"

#include <atomic>
#include <chrono>
#include <mutex>
#include <string>

#include <windows.h>

#pragma comment(lib, "version.lib")

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

        // Ephemeral, per-process, never persisted: it only groups this run's events together.
        uint64_t SessionId() noexcept
        {
            static uint64_t const id = [] {
                LARGE_INTEGER counter{};
                QueryPerformanceCounter(&counter);
                return (static_cast<uint64_t>(GetCurrentProcessId()) << 32) ^
                    static_cast<uint64_t>(counter.QuadPart);
            }();
            return id;
        }

        // File version of the module this code is linked into, which is the control version we ship.
        // Every caller is noexcept, so nothing here may escape: the allocations below can throw
        // bad_alloc, and a throw out of a noexcept frame would terminate the app over telemetry.
        char const* ControlVersion() noexcept
        {
            static std::string const version = [] () -> std::string {
                try
                {
                    HMODULE module{};
                    if (!GetModuleHandleExW(
                            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                            reinterpret_cast<LPCWSTR>(&g_nextId),
                            &module))
                    {
                        return "unknown";
                    }

                    wchar_t path[MAX_PATH]{};
                    if (GetModuleFileNameW(module, path, ARRAYSIZE(path)) == 0)
                    {
                        return "unknown";
                    }

                    DWORD handle{};
                    DWORD const size = GetFileVersionInfoSizeW(path, &handle);
                    if (size == 0)
                    {
                        return "unknown";
                    }

                    std::string buffer(size, '\0');
                    // The handle argument is ignored by the API and must be zero.
                    if (!GetFileVersionInfoW(path, 0, size, buffer.data()))
                    {
                        return "unknown";
                    }

                    VS_FIXEDFILEINFO* info{};
                    UINT length{};
                    if (!VerQueryValueW(buffer.data(), L"\\", reinterpret_cast<void**>(&info), &length) || !info)
                    {
                        return "unknown";
                    }

                    char text[64]{};
                    sprintf_s(text, "%u.%u.%u.%u",
                        HIWORD(info->dwFileVersionMS), LOWORD(info->dwFileVersionMS),
                        HIWORD(info->dwFileVersionLS), LOWORD(info->dwFileVersionLS));
                    return text;
                }
                catch (...)
                {
                    return "unknown";
                }
            }();

            return version.c_str();
        }

        // Must match what the events actually write, or this either skips events a session asked for
        // or does the work for events ETW will drop: the writes carry no explicit level, so
        // TraceLogging defaults them to VERBOSE, and their keyword is the measures keyword combined
        // with the control keyword.
        bool IsProviderEnabled(uint64_t keyword) noexcept
        {
            return !!TraceLoggingProviderEnabled(g_hTelemetryProvider, WINEVENT_LEVEL_VERBOSE, keyword);
        }
    }

// Dimensions the control telemetry spec requires on every event, so Inking rows join the same
// schema as the other controls.
#define INK_TELEMETRY_COMMON_FIELDS \
    TraceLoggingUInt32(SchemaVersion, "SchemaVersion"), \
    TraceLoggingString("Inking", "ControlType"), \
    TraceLoggingString(ControlVersion(), "ControlVersion"), \
    TraceLoggingUInt64(SessionId(), "AppSessionId")

    bool IsCanvasEnabled() noexcept
    {
        return IsProviderEnabled(MICROSOFT_KEYWORD_MEASURES | KEYWORD_INKCANVAS);
    }

    bool IsToolbarEnabled() noexcept
    {
        return IsProviderEnabled(MICROSOFT_KEYWORD_MEASURES | KEYWORD_INKTOOLBAR);
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
            INK_TELEMETRY_COMMON_FIELDS,
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
            INK_TELEMETRY_COMMON_FIELDS,
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
            state.activationCount++;
            if (!state.activatedMicroseconds)
            {
                state.activatedMicroseconds = Timestamp();
            }

            TraceLoggingWrite(
                g_hTelemetryProvider,
                "InkCanvas_InitializationLatency",
                INK_TELEMETRY_COMMON_FIELDS,
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
            INK_TELEMETRY_COMMON_FIELDS,
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
            INK_TELEMETRY_COMMON_FIELDS,
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
            INK_TELEMETRY_COMMON_FIELDS,
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

    void ReportError(
        ErrorCategory category,
        Operation operation,
        bool isRecoverable,
        int32_t hr,
        CanvasState* state) noexcept
    {
        if (state)
        {
            state->errorCount++;
        }

        if (!IsCanvasEnabled())
        {
            return;
        }

        TraceLoggingWrite(
            g_hTelemetryProvider,
            "Ink_ControlError",
            INK_TELEMETRY_COMMON_FIELDS,
            TraceLoggingUInt64(state ? state->id : 0, "InkCanvasId"),
            TraceLoggingUInt32(static_cast<uint32_t>(category), "ErrorCategory"),
            TraceLoggingUInt32(static_cast<uint32_t>(operation), "Operation"),
            TraceLoggingBoolean(isRecoverable, "IsRecoverable"),
            TraceLoggingHResult(hr, "HResult"),
            TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance),
            TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES),
            TraceLoggingKeyword(KEYWORD_INKCANVAS));
    }

    void RecordStrokesCollected(CanvasState& state, uint32_t count) noexcept
    {
        state.strokesCollected += count;

        if (!state.firstInkMicroseconds && count > 0)
        {
            state.firstInkMicroseconds = Timestamp();
        }
    }

    void RecordStrokesErased(CanvasState& state, uint32_t count) noexcept
    {
        state.strokesErased += count;
    }

    void RecordToolSwitch(ToolbarState& state, uint32_t toolKind) noexcept
    {
        state.toolSwitchCount++;

        if (toolKind < 32)
        {
            state.toolsUsedMask |= (1u << toolKind);
        }
    }

    void ReportCanvasSessionSummary(CanvasState& state, CompositorEngine engine) noexcept
    {
        // Nothing to say about a canvas that never finished activating; its failure was already sent.
        if (state.summaryReported || !state.activatedMicroseconds || !IsCanvasEnabled())
        {
            return;
        }

        state.summaryReported = true;

        auto const activeMilliseconds = (Timestamp() - state.activatedMicroseconds) / 1000.0;
        auto const timeToFirstInkMs = state.firstInkMicroseconds
            ? (state.firstInkMicroseconds - state.activatedMicroseconds) / 1000.0
            : -1.0;

        TraceLoggingWrite(
            g_hTelemetryProvider,
            "InkCanvas_SessionSummary",
            INK_TELEMETRY_COMMON_FIELDS,
            TraceLoggingUInt64(state.id, "InkCanvasId"),
            TraceLoggingUInt32(static_cast<uint32_t>(engine), "CompositorEngine"),
            TraceLoggingUInt32(state.activationCount, "ActivationCount"),
            TraceLoggingUInt32(CountBucket(state.strokesCollected), "StrokesCollectedBucket"),
            TraceLoggingUInt32(CountBucket(state.strokesErased), "StrokesErasedBucket"),
            TraceLoggingUInt32(state.errorCount, "ErrorCount"),
            TraceLoggingBoolean(state.strokesCollected > 0, "ReceivedInk"),
            TraceLoggingFloat64(timeToFirstInkMs, "TimeToFirstInkMs"),
            TraceLoggingFloat64(activeMilliseconds, "ActiveDurationMs"),
            TelemetryPrivacyDataTag(PDT_ProductAndServiceUsage),
            TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES),
            TraceLoggingKeyword(KEYWORD_INKCANVAS));
    }

    void ReportToolbarSessionSummary(ToolbarState& state) noexcept
    {
        if (state.summaryReported || !state.usageReported || !IsToolbarEnabled())
        {
            return;
        }

        state.summaryReported = true;

        uint32_t distinctTools = 0;
        for (uint32_t mask = state.toolsUsedMask; mask; mask >>= 1)
        {
            distinctTools += (mask & 1u);
        }

        TraceLoggingWrite(
            g_hTelemetryProvider,
            "InkToolbar_SessionSummary",
            INK_TELEMETRY_COMMON_FIELDS,
            TraceLoggingUInt64(state.id, "InkToolbarId"),
            TraceLoggingUInt32(CountBucket(state.toolSwitchCount), "ToolSwitchCountBucket"),
            TraceLoggingUInt32(distinctTools, "DistinctToolsUsed"),
            TelemetryPrivacyDataTag(PDT_ProductAndServiceUsage),
            TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES),
            TraceLoggingKeyword(KEYWORD_INKTOOLBAR));
    }

    uint32_t CountBucket(uint32_t count) noexcept
    {
        if (count <= 1) { return count; }
        if (count <= 4) { return 2; }
        if (count <= 16) { return 3; }
        return 4;
    }
}
