// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <cstddef>
#include <cstdint>
#include <dxgi.h>
#include <wrl/client.h>

// Hardware adapter selection for the D2D-HW immediate backend (D2DDeviceFactory.cpp). Device identity is
// measurement-critical: a D2D-HW run must be on a real GPU, never on WARP or the Microsoft Basic Render Driver, so
// CreateD3DDeviceForMode(ImmediateD2DHardware) fails with DXGI_ERROR_NOT_FOUND instead of falling back.
//
// Policy (see docs/design-notes/immediate-rendering-backends-proposal.md, "D2D-HW adapter selection"):
// - Adapters are considered in IDXGIFactory6::EnumAdapterByGpuPreference(HIGH_PERFORMANCE) order, falling back to
//   IDXGIFactory1::EnumAdapters1 order; the first qualifying adapter wins.
// - DXGI_ADAPTER_FLAG_SOFTWARE adapters are rejected.
// - Every Microsoft (VendorId 0x1414) adapter is rejected: the Basic Render Driver (DeviceId 0x8c), and also the
//   display-only adapters such as "Microsoft Hyper-V Video" and Remote Display. DXGI surfaces those as additional
//   "Microsoft Basic Render Driver" adapters *without* the software flag, and a D3D11 device created on them is the
//   Basic Render Driver (WARP-backed); verified on a Hyper-V VM by HardwareDeviceFactoryTests::
//   MicrosoftAdaptersDoNotProvideHardwareRendering. GPU paravirtualization (GPU-P) guests see the host GPU's own vendor
//   id, so they still qualify.
// - WINUI_RENDERING_ADAPTER_LUID (hex, (HighPart << 32) | LowPart, same packing as DeviceInfo.AdapterLuid) pins the
//   adapter on multi-GPU machines. It is strict: if the named adapter is missing or does not qualify, creation fails
//   with DXGI_ERROR_NOT_FOUND rather than picking another adapter. A malformed value fails with E_INVALIDARG.
namespace RenderingBackend::HardwareAdapter
{
    constexpr const wchar_t* AdapterLuidEnvironmentVariable = L"WINUI_RENDERING_ADAPTER_LUID";
    constexpr uint32_t MicrosoftVendorId = 0x1414;
    constexpr uint32_t BasicRenderDriverDeviceId = 0x8c;

    enum class Verdict : uint32_t
    {
        Qualifies,
        SoftwareAdapter,          // DXGI_ADAPTER_FLAG_SOFTWARE
        BasicRenderDriver,        // 0x1414:0x8c without the software flag (display-only adapters surface this way)
        MicrosoftVirtualAdapter,  // any other 0x1414 adapter (Hyper-V Video, Remote Display, Basic Display)
    };

    Verdict Classify(const DXGI_ADAPTER_DESC1& desc) noexcept;
    inline bool Qualifies(const DXGI_ADAPTER_DESC1& desc) noexcept { return Classify(desc) == Verdict::Qualifies; }
    const wchar_t* VerdictName(Verdict verdict) noexcept;

    constexpr uint64_t PackLuid(const LUID& luid) noexcept
    {
        return (static_cast<uint64_t>(static_cast<uint32_t>(luid.HighPart)) << 32) | luid.LowPart;
    }

    // Hex digits with an optional 0x prefix and surrounding spaces; at most 16 digits; zero is rejected.
    bool TryParseLuidOverride(_In_opt_z_ const wchar_t* text, _Out_ uint64_t& luid) noexcept;

    // S_OK with luid = 0 when the variable is unset; E_INVALIDARG when it is set but malformed.
    HRESULT ReadLuidOverrideFromEnvironment(_Out_ uint64_t& luid) noexcept;

    constexpr size_t NoAdapter = static_cast<size_t>(-1);

    // Pure selection over descs in preference order. overrideLuid == 0 means no override. Returns the chosen index or
    // NoAdapter.
    size_t ChooseAdapter(_In_reads_(count) const DXGI_ADAPTER_DESC1* descs, size_t count, uint64_t overrideLuid) noexcept;

    struct AdapterList
    {
        static constexpr size_t Capacity = 16;
        Microsoft::WRL::ComPtr<IDXGIAdapter1> adapters[Capacity];
        DXGI_ADAPTER_DESC1 descs[Capacity]{};
        size_t count = 0;
        bool usedGpuPreference = false; // true when IDXGIFactory6::EnumAdapterByGpuPreference supplied the order
    };

    // Every adapter on the machine (qualifying or not) in selection order.
    HRESULT EnumerateAdapters(_Out_ AdapterList& list) noexcept;
}
