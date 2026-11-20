// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <cstdint>
#include <guiddef.h>

// Experimental rendering-backend selection ("retained XAML, immediate rendering").
// See docs/design-notes/immediate-rendering-backends-proposal.md. Selection is internal: it is read once per process
// from the WINUI_RENDERING_BACKEND environment variable; there is no public API.
namespace RenderingBackend
{
    // Values are persisted in ETW payloads; append only.
    enum class Mode : uint32_t
    {
        Composition,
        ImmediateGdiPlus,
        ImmediateD2DHardware,
        ImmediateD2DSoftware,
        Count
    };

    // Values are persisted in ETW payloads; append only.
    enum class FallbackReason : uint32_t
    {
        None,
        NotImplemented,
        InvalidRequest,
        DeviceUnavailable,
        UnsupportedContent,
    };

    constexpr uint32_t ModeCount = static_cast<uint32_t>(Mode::Count);
    constexpr uint32_t ModeBit(Mode mode) noexcept { return 1u << static_cast<uint32_t>(mode); }
    constexpr uint32_t AllModesMask = (1u << ModeCount) - 1;

    constexpr const wchar_t* EnvironmentVariableName = L"WINUI_RENDERING_BACKEND";

    struct Selection
    {
        Mode requested = Mode::Composition;
        Mode effective = Mode::Composition;
        FallbackReason reason = FallbackReason::None;
    };

    bool TryParseMode(_In_opt_z_ const wchar_t* text, _Inout_ Mode& mode) noexcept;
    const wchar_t* ModeName(Mode mode) noexcept;
    const wchar_t* FallbackReasonName(FallbackReason reason) noexcept;

    // Composition is always available regardless of availableModes. An unavailable immediate mode falls back with
    // DeviceUnavailable when its bit is set in deviceUnavailableModes and NotImplemented otherwise.
    Selection Resolve(_In_opt_z_ const wchar_t* requestText, uint32_t availableModes) noexcept;
    Selection Resolve(_In_opt_z_ const wchar_t* requestText, uint32_t availableModes, uint32_t deviceUnavailableModes) noexcept;
    Selection ReadSelectionFromEnvironment(uint32_t availableModes) noexcept;
    Selection ReadSelectionFromEnvironment(uint32_t availableModes, uint32_t deviceUnavailableModes) noexcept;

    // Reports whether an immediate mode can render on this machine: None when its painter (and device) can be
    // created, DeviceUnavailable when the device cannot be created, NotImplemented when the painter is missing.
    // Registered by the core (ProbeImmediateMode in ImmediatePainter.cpp) so this library does not link painters.
    using ModeProbeFn = FallbackReason (*)(Mode mode); // must not throw
    void SetImmediateModeProbe(ModeProbeFn probe) noexcept;

    // Composition plus every immediate mode whose probe succeeds. Probes lazily (creates devices); results are cached.
    uint32_t GetAvailableModes() noexcept;

    // Read once, then cached for the process lifetime.
    const Selection& GetProcessSelection() noexcept;

    // True when the process renders with an immediate-mode painter (effective mode is not Composition). Cached.
    bool IsImmediateModeActive() noexcept;

    // ---- ETW: provider "Microsoft-WinUI-RenderingBackend" {d18ce99c-dd66-5390-18da-068546739801} ----
    namespace Keywords
    {
        constexpr uint64_t Selection = 0x1;
        constexpr uint64_t Frame = 0x2;
        constexpr uint64_t Paint = 0x4;
        constexpr uint64_t Resources = 0x8;
    }

    const GUID& GetProviderId() noexcept;
    bool IsTracingEnabled(uint64_t keyword) noexcept;

    // Re-emits BackendSelected whenever a new session enables the provider so late-attached captures see it.
    void TraceSelectionIfNewSession() noexcept;
    void TraceBackendSelected(const Selection& selection) noexcept;

    void TraceBackendFrameBegin(uint32_t frameNumber) noexcept;
    void TraceBackendFrameEnd(uint32_t frameNumber, bool frameDrawn) noexcept;
    void TraceRenderWalkStats(uint32_t frameNumber, uint32_t elementsVisited, uint32_t elementsRendered) noexcept;

    // Emitted by the backend that owns the device. adapterLuid is LUID packed as (HighPart << 32) | LowPart.
    void TraceDeviceInfo(Mode mode, uint64_t adapterLuid, bool isWarp, uint32_t featureLevel) noexcept;

    // Immediate backends bracket the work for one root (target) per frame.
    void TracePaintBegin(uint32_t frameNumber, uint64_t rootId) noexcept;
    void TracePaintEnd(
        uint32_t frameNumber,
        uint64_t rootId,
        uint64_t damagedPixels,
        uint64_t paintedPixels,
        uint32_t primitiveCount,
        uint32_t damageRectCount) noexcept;

    // Backend-owned memory; Composition memory is attributed via the Microsoft-Windows-XAML-Profiler provider.
    void TraceResourceInventory(
        Mode mode,
        uint64_t surfaceBytes,
        uint64_t cacheBytes,
        uint32_t cachedObjectCount,
        uint64_t uploadBytesThisFrame) noexcept;

    // An element subtree that the immediate backend cannot draw; the root falls back to Composition.
    void TraceUnsupportedContent(Mode mode, _In_opt_z_ const wchar_t* typeName, _In_opt_z_ const wchar_t* detail) noexcept;

    // ---- Begin: W6 resource churn (appended by immediate-rendering-gdiplus) ----
    // Backend-owned resources (brushes, pens, fonts, text formats, bitmaps, surfaces, textures) created and destroyed
    // while painting frameNumber. Event "ResourceChurn", Resources keyword. The analyzer reports per-phase rates.
    void TraceResourceChurn(Mode mode, uint32_t frameNumber, uint32_t created, uint32_t destroyed) noexcept;
    // ---- End: W6 resource churn ----

    // ---- Added by immediate-rendering-d2d-sw ----
    // The painter's device was removed or reset (test plan C4/T21). reason is the HRESULT the painter observed
    // (for example DXGI_ERROR_DEVICE_REMOVED) and removedReason is ID3D11Device::GetDeviceRemovedReason().
    void TraceDeviceLost(Mode mode, HRESULT reason, HRESULT removedReason, uint32_t frameNumber) noexcept;
    // ---- End immediate-rendering-d2d-sw ----
}

// ---- Added by the D2D-HW branch: GPU time per paint (test plan W3) ----
namespace RenderingBackend
{
    // GPU execution time of one root's paint, from D3D11 timestamp queries (GpuTimestampProfiler). Emitted when the
    // result is read back, typically a frame or two after the paint; FrameNumber identifies the painted frame.
    void TraceGpuPaintTime(uint32_t frameNumber, uint64_t gpuMicroseconds) noexcept;
}
// ---- Added by immediate-rendering-renderthread (S15/S21) ----
namespace RenderingBackend
{
    // A frame presented by the immediate render thread without the UI thread (independent animations, spinners).
    // uiIdleMs: time since the UI thread last presented this root. Event "IndependentFrame", Frame keyword.
    void TraceIndependentFrame(uint32_t independentFrame, uint64_t rootId, uint32_t layerCount, uint32_t uiIdleMs,
        uint64_t damagePixels, uint64_t paintMicroseconds, HRESULT result) noexcept;

    // The immediate renderer presented again after a device loss (real or WINUI_IMMEDIATE_SIMULATE_DEVICE_LOST).
    // elapsedMs: from the loss to the first successful present. Event "DeviceRecovered", Selection keyword.
    void TraceDeviceRecovered(Mode mode, uint32_t elapsedMs, uint32_t frameNumber, bool simulated, uint32_t lossCount) noexcept;

    // An immediate root's target changed size, rasterization scale or theme (S20). Event "TargetChanged".
    void TraceTargetChanged(uint64_t rootId, uint32_t width, uint32_t height, float scale, uint32_t reason, uint32_t frameNumber) noexcept;
}