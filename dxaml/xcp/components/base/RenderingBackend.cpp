// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <RenderingBackend.h>
#include <TraceLoggingProvider.h>
#include <atomic>
#include <iterator>

// {d18ce99c-dd66-5390-18da-068546739801} = EventSource name hash of "Microsoft-WinUI-RenderingBackend".
TRACELOGGING_DEFINE_PROVIDER(
    g_renderingBackendProvider,
    "Microsoft-WinUI-RenderingBackend",
    (0xd18ce99c, 0xdd66, 0x5390, 0x18, 0xda, 0x06, 0x85, 0x46, 0x73, 0x98, 0x01));

namespace RenderingBackend
{
    namespace
    {
        constexpr const wchar_t* ModeNames[] = { L"composition", L"gdiplus", L"d2d-hw", L"d2d-sw" };
        static_assert(std::size(ModeNames) == ModeCount);

        constexpr const wchar_t* ReasonNames[] =
        {
            L"None", L"NotImplemented", L"InvalidRequest", L"DeviceUnavailable", L"UnsupportedContent"
        };

        constexpr GUID c_providerId = { 0xd18ce99c, 0xdd66, 0x5390, { 0x18, 0xda, 0x06, 0x85, 0x46, 0x73, 0x98, 0x01 } };

        std::atomic<bool> s_wasEnabled{ false };

        struct ProviderRegistration
        {
            ProviderRegistration() noexcept { TraceLoggingRegister(g_renderingBackendProvider); }
            ~ProviderRegistration() noexcept { TraceLoggingUnregister(g_renderingBackendProvider); }
        };

        void EnsureRegistered() noexcept
        {
            static ProviderRegistration s_registration;
        }

        std::atomic<ModeProbeFn> s_probe{ nullptr };

        // Per mode: 0 = not probed yet, otherwise FallbackReason + 1.
        std::atomic<uint32_t> s_probeResults[ModeCount]{};

        FallbackReason ProbeMode(Mode mode) noexcept
        {
            if (mode == Mode::Composition)
            {
                return FallbackReason::None;
            }
            const auto index = static_cast<uint32_t>(mode);
            const uint32_t cached = s_probeResults[index].load(std::memory_order_acquire);
            if (cached != 0)
            {
                return static_cast<FallbackReason>(cached - 1);
            }
            const ModeProbeFn probe = s_probe.load(std::memory_order_acquire);
            if (probe == nullptr)
            {
                return FallbackReason::NotImplemented;
            }
            const FallbackReason result = probe(mode);
            s_probeResults[index].store(static_cast<uint32_t>(result) + 1, std::memory_order_release);
            return result;
        }

        // Available modes known so far without probing (never creates devices).
        uint32_t GetKnownAvailableModes() noexcept
        {
            uint32_t modes = ModeBit(Mode::Composition);
            for (uint32_t index = 1; index < ModeCount; ++index)
            {
                if (s_probeResults[index].load(std::memory_order_acquire) == static_cast<uint32_t>(FallbackReason::None) + 1)
                {
                    modes |= 1u << index;
                }
            }
            return modes;
        }
    }

    void SetImmediateModeProbe(ModeProbeFn probe) noexcept
    {
        s_probe.store(probe, std::memory_order_release);
    }

    bool TryParseMode(_In_opt_z_ const wchar_t* text, _Inout_ Mode& mode) noexcept
    {
        if (text == nullptr || *text == L'\0')
        {
            return false;
        }
        for (uint32_t index = 0; index < ModeCount; ++index)
        {
            if (_wcsicmp(text, ModeNames[index]) == 0)
            {
                mode = static_cast<Mode>(index);
                return true;
            }
        }
        return false;
    }

    const wchar_t* ModeName(Mode mode) noexcept
    {
        const auto index = static_cast<uint32_t>(mode);
        return index < ModeCount ? ModeNames[index] : L"unknown";
    }

    const wchar_t* FallbackReasonName(FallbackReason reason) noexcept
    {
        const auto index = static_cast<uint32_t>(reason);
        return index < std::size(ReasonNames) ? ReasonNames[index] : L"Unknown";
    }

    Selection Resolve(_In_opt_z_ const wchar_t* requestText, uint32_t availableModes) noexcept
    {
        return Resolve(requestText, availableModes, 0);
    }

    Selection Resolve(_In_opt_z_ const wchar_t* requestText, uint32_t availableModes, uint32_t deviceUnavailableModes) noexcept
    {
        Selection selection;
        if (requestText == nullptr || *requestText == L'\0')
        {
            return selection;
        }

        Mode requested{};
        if (!TryParseMode(requestText, requested))
        {
            selection.reason = FallbackReason::InvalidRequest;
            return selection;
        }

        selection.requested = requested;
        if (requested == Mode::Composition || (availableModes & ModeBit(requested)) != 0)
        {
            selection.effective = requested;
        }
        else
        {
            selection.reason = (deviceUnavailableModes & ModeBit(requested)) != 0
                ? FallbackReason::DeviceUnavailable
                : FallbackReason::NotImplemented;
        }
        return selection;
    }

    Selection ReadSelectionFromEnvironment(uint32_t availableModes) noexcept
    {
        return ReadSelectionFromEnvironment(availableModes, 0);
    }

    Selection ReadSelectionFromEnvironment(uint32_t availableModes, uint32_t deviceUnavailableModes) noexcept
    {
        wchar_t buffer[32]{};
        const auto length = GetEnvironmentVariableW(EnvironmentVariableName, buffer, ARRAYSIZE(buffer));
        if (length == 0)
        {
            return Resolve(nullptr, availableModes, deviceUnavailableModes);
        }
        if (length >= ARRAYSIZE(buffer))
        {
            Selection invalid;
            invalid.reason = FallbackReason::InvalidRequest;
            return invalid;
        }
        return Resolve(buffer, availableModes, deviceUnavailableModes);
    }

    uint32_t GetAvailableModes() noexcept
    {
        uint32_t modes = ModeBit(Mode::Composition);
        for (uint32_t index = 1; index < ModeCount; ++index)
        {
            if (ProbeMode(static_cast<Mode>(index)) == FallbackReason::None)
            {
                modes |= 1u << index;
            }
        }
        return modes;
    }

    namespace
    {
        Selection ComputeProcessSelection() noexcept
        {
            // Only the requested immediate mode is probed so Composition (and an unset variable) never creates devices.
            const Selection request = ReadSelectionFromEnvironment(AllModesMask);
            if (request.reason != FallbackReason::None || request.requested == Mode::Composition)
            {
                return request;
            }
            const FallbackReason probe = ProbeMode(request.requested);
            if (probe == FallbackReason::None)
            {
                return request;
            }
            Selection fallback;
            fallback.requested = request.requested;
            fallback.reason = probe;
            return fallback;
        }
    }

    const Selection& GetProcessSelection() noexcept
    {
        static const Selection s_selection = ComputeProcessSelection();
        return s_selection;
    }

    bool IsImmediateModeActive() noexcept
    {
        static const bool s_active = GetProcessSelection().effective != Mode::Composition;
        return s_active;
    }

    const GUID& GetProviderId() noexcept
    {
        return c_providerId;
    }

    bool IsTracingEnabled(uint64_t keyword) noexcept
    {
        EnsureRegistered();
        return TraceLoggingProviderEnabled(g_renderingBackendProvider, 0, keyword);
    }

    void TraceSelectionIfNewSession() noexcept
    {
        const bool enabled = IsTracingEnabled(Keywords::Selection);
        if (s_wasEnabled.exchange(enabled, std::memory_order_relaxed) != enabled && enabled)
        {
            TraceBackendSelected(GetProcessSelection());
        }
    }

    void TraceBackendSelected(const Selection& selection) noexcept
    {
        EnsureRegistered();
        TraceLoggingWrite(
            g_renderingBackendProvider,
            "BackendSelected",
            TraceLoggingLevel(WINEVENT_LEVEL_INFO),
            TraceLoggingKeyword(Keywords::Selection),
            TraceLoggingUInt32(static_cast<uint32_t>(selection.requested), "RequestedMode"),
            TraceLoggingUInt32(static_cast<uint32_t>(selection.effective), "EffectiveMode"),
            TraceLoggingUInt32(static_cast<uint32_t>(selection.reason), "FallbackReason"),
            TraceLoggingWideString(ModeName(selection.requested), "RequestedName"),
            TraceLoggingWideString(ModeName(selection.effective), "EffectiveName"),
            TraceLoggingWideString(FallbackReasonName(selection.reason), "FallbackReasonName"),
            TraceLoggingUInt32(GetKnownAvailableModes(), "AvailableModes"));
    }

    void TraceBackendFrameBegin(uint32_t frameNumber) noexcept
    {
        EnsureRegistered();
        TraceLoggingWrite(
            g_renderingBackendProvider,
            "FrameBegin",
            TraceLoggingLevel(WINEVENT_LEVEL_INFO),
            TraceLoggingKeyword(Keywords::Frame),
            TraceLoggingUInt32(frameNumber, "FrameNumber"),
            TraceLoggingUInt32(static_cast<uint32_t>(GetProcessSelection().effective), "EffectiveMode"));
    }

    void TraceBackendFrameEnd(uint32_t frameNumber, bool frameDrawn) noexcept
    {
        EnsureRegistered();
        TraceLoggingWrite(
            g_renderingBackendProvider,
            "FrameEnd",
            TraceLoggingLevel(WINEVENT_LEVEL_INFO),
            TraceLoggingKeyword(Keywords::Frame),
            TraceLoggingUInt32(frameNumber, "FrameNumber"),
            TraceLoggingBoolean(frameDrawn, "FrameDrawn"));
    }

    void TraceRenderWalkStats(uint32_t frameNumber, uint32_t elementsVisited, uint32_t elementsRendered) noexcept
    {
        EnsureRegistered();
        TraceLoggingWrite(
            g_renderingBackendProvider,
            "RenderWalkStats",
            TraceLoggingLevel(WINEVENT_LEVEL_INFO),
            TraceLoggingKeyword(Keywords::Frame),
            TraceLoggingUInt32(frameNumber, "FrameNumber"),
            TraceLoggingUInt32(elementsVisited, "ElementsVisited"),
            TraceLoggingUInt32(elementsRendered, "ElementsRendered"));
    }

    void TraceDeviceInfo(Mode mode, uint64_t adapterLuid, bool isWarp, uint32_t featureLevel) noexcept
    {
        EnsureRegistered();
        TraceLoggingWrite(
            g_renderingBackendProvider,
            "DeviceInfo",
            TraceLoggingLevel(WINEVENT_LEVEL_INFO),
            TraceLoggingKeyword(Keywords::Selection),
            TraceLoggingUInt32(static_cast<uint32_t>(mode), "Mode"),
            TraceLoggingUInt64(adapterLuid, "AdapterLuid"),
            TraceLoggingBoolean(isWarp, "IsWarp"),
            TraceLoggingHexUInt32(featureLevel, "FeatureLevel"));
    }

    void TracePaintBegin(uint32_t frameNumber, uint64_t rootId) noexcept
    {
        EnsureRegistered();
        TraceLoggingWrite(
            g_renderingBackendProvider,
            "PaintBegin",
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE),
            TraceLoggingKeyword(Keywords::Paint),
            TraceLoggingUInt32(frameNumber, "FrameNumber"),
            TraceLoggingUInt64(rootId, "RootId"));
    }

    void TracePaintEnd(
        uint32_t frameNumber,
        uint64_t rootId,
        uint64_t damagedPixels,
        uint64_t paintedPixels,
        uint32_t primitiveCount,
        uint32_t damageRectCount) noexcept
    {
        EnsureRegistered();
        TraceLoggingWrite(
            g_renderingBackendProvider,
            "PaintEnd",
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE),
            TraceLoggingKeyword(Keywords::Paint),
            TraceLoggingUInt32(frameNumber, "FrameNumber"),
            TraceLoggingUInt64(rootId, "RootId"),
            TraceLoggingUInt64(damagedPixels, "DamagedPixels"),
            TraceLoggingUInt64(paintedPixels, "PaintedPixels"),
            TraceLoggingUInt32(primitiveCount, "PrimitiveCount"),
            TraceLoggingUInt32(damageRectCount, "DamageRectCount"));
    }

    void TraceResourceInventory(
        Mode mode,
        uint64_t surfaceBytes,
        uint64_t cacheBytes,
        uint32_t cachedObjectCount,
        uint64_t uploadBytesThisFrame) noexcept
    {
        EnsureRegistered();
        TraceLoggingWrite(
            g_renderingBackendProvider,
            "ResourceInventory",
            TraceLoggingLevel(WINEVENT_LEVEL_INFO),
            TraceLoggingKeyword(Keywords::Resources),
            TraceLoggingUInt32(static_cast<uint32_t>(mode), "Mode"),
            TraceLoggingUInt64(surfaceBytes, "SurfaceBytes"),
            TraceLoggingUInt64(cacheBytes, "CacheBytes"),
            TraceLoggingUInt32(cachedObjectCount, "CachedObjectCount"),
            TraceLoggingUInt64(uploadBytesThisFrame, "UploadBytes"));
    }

    void TraceUnsupportedContent(Mode mode, _In_opt_z_ const wchar_t* typeName, _In_opt_z_ const wchar_t* detail) noexcept
    {
        EnsureRegistered();
        TraceLoggingWrite(
            g_renderingBackendProvider,
            "UnsupportedContent",
            TraceLoggingLevel(WINEVENT_LEVEL_WARNING),
            TraceLoggingKeyword(Keywords::Selection),
            TraceLoggingUInt32(static_cast<uint32_t>(mode), "Mode"),
            TraceLoggingWideString(typeName ? typeName : L"", "TypeName"),
            TraceLoggingWideString(detail ? detail : L"", "Detail"));
    }

    // ---- Begin: W6 resource churn (appended by immediate-rendering-gdiplus) ----
    void TraceResourceChurn(Mode mode, uint32_t frameNumber, uint32_t created, uint32_t destroyed) noexcept
    {
        EnsureRegistered();
        TraceLoggingWrite(
            g_renderingBackendProvider,
            "ResourceChurn",
            TraceLoggingLevel(WINEVENT_LEVEL_INFO),
            TraceLoggingKeyword(Keywords::Resources),
            TraceLoggingUInt32(static_cast<uint32_t>(mode), "Mode"),
            TraceLoggingUInt32(frameNumber, "FrameNumber"),
            TraceLoggingUInt32(created, "Created"),
            TraceLoggingUInt32(destroyed, "Destroyed"));
    }
    // ---- End: W6 resource churn ----

    // ---- Added by immediate-rendering-d2d-sw ----
    void TraceDeviceLost(Mode mode, HRESULT reason, HRESULT removedReason, uint32_t frameNumber) noexcept
    {
        EnsureRegistered();
        TraceLoggingWrite(
            g_renderingBackendProvider,
            "DeviceLost",
            TraceLoggingLevel(WINEVENT_LEVEL_WARNING),
            TraceLoggingKeyword(Keywords::Selection),
            TraceLoggingUInt32(static_cast<uint32_t>(mode), "Mode"),
            TraceLoggingHResult(reason, "Reason"),
            TraceLoggingHResult(removedReason, "RemovedReason"),
            TraceLoggingUInt32(frameNumber, "FrameNumber"));
    }
    // ---- End immediate-rendering-d2d-sw ----
}

// ---- Added by the D2D-HW branch: GPU time per paint (test plan W3) ----
namespace RenderingBackend
{
    void TraceGpuPaintTime(uint32_t frameNumber, uint64_t gpuMicroseconds) noexcept
    {
        EnsureRegistered();
        TraceLoggingWrite(
            g_renderingBackendProvider,
            "GpuPaintTime",
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE),
            TraceLoggingKeyword(Keywords::Paint),
            TraceLoggingUInt32(frameNumber, "FrameNumber"),
            TraceLoggingUInt64(gpuMicroseconds, "GpuMicroseconds"));
    }
}
// ---- Added by immediate-rendering-renderthread (S15/S21) ----
namespace RenderingBackend
{
    void TraceIndependentFrame(uint32_t independentFrame, uint64_t rootId, uint32_t layerCount, uint32_t uiIdleMs,
        uint64_t damagePixels, uint64_t paintMicroseconds, HRESULT result) noexcept
    {
        EnsureRegistered();
        TraceLoggingWrite(
            g_renderingBackendProvider,
            "IndependentFrame",
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE),
            TraceLoggingKeyword(Keywords::Frame),
            TraceLoggingUInt32(independentFrame, "IndependentFrame"),
            TraceLoggingUInt64(rootId, "RootId"),
            TraceLoggingUInt32(layerCount, "LayerCount"),
            TraceLoggingUInt32(uiIdleMs, "UiIdleMs"),
            TraceLoggingUInt64(damagePixels, "DamagePixels"),
            TraceLoggingUInt64(paintMicroseconds, "PaintMicroseconds"),
            TraceLoggingHResult(result, "Result"));
    }

    void TraceDeviceRecovered(Mode mode, uint32_t elapsedMs, uint32_t frameNumber, bool simulated, uint32_t lossCount) noexcept
    {
        EnsureRegistered();
        TraceLoggingWrite(
            g_renderingBackendProvider,
            "DeviceRecovered",
            TraceLoggingLevel(WINEVENT_LEVEL_WARNING),
            TraceLoggingKeyword(Keywords::Selection),
            TraceLoggingUInt32(static_cast<uint32_t>(mode), "Mode"),
            TraceLoggingUInt32(elapsedMs, "ElapsedMs"),
            TraceLoggingUInt32(frameNumber, "FrameNumber"),
            TraceLoggingBool(simulated, "Simulated"),
            TraceLoggingUInt32(lossCount, "LossCount"));
    }

    void TraceTargetChanged(uint64_t rootId, uint32_t width, uint32_t height, float scale, uint32_t reason, uint32_t frameNumber) noexcept
    {
        EnsureRegistered();
        TraceLoggingWrite(
            g_renderingBackendProvider,
            "TargetChanged",
            TraceLoggingLevel(WINEVENT_LEVEL_INFO),
            TraceLoggingKeyword(Keywords::Frame),
            TraceLoggingUInt64(rootId, "RootId"),
            TraceLoggingUInt32(width, "Width"),
            TraceLoggingUInt32(height, "Height"),
            TraceLoggingFloat32(scale, "Scale"),
            TraceLoggingUInt32(reason, "Reason"),
            TraceLoggingUInt32(frameNumber, "FrameNumber"));
    }
}