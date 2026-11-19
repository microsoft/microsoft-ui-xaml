// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "D2DDeviceFactoryTests.h"
#include <D2DDeviceFactory.h>
#include <ImmediatePainter.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <wrl/client.h>
#include <algorithm>
#include <optional>
#include <string>
#include <vector>

namespace RenderingBackendTests
{
    using namespace RenderingBackend;
    using namespace RenderingBackend::HardwareAdapter;
    using namespace WEX::Common;
    using namespace WEX::Logging;
    using Microsoft::WRL::ComPtr;

    namespace
    {
        DXGI_ADAPTER_DESC1 MakeDesc(const wchar_t* name, uint32_t vendorId, uint32_t deviceId, uint32_t flags, uint64_t luid)
        {
            DXGI_ADAPTER_DESC1 desc{};
            wcsncpy_s(desc.Description, name, _TRUNCATE);
            desc.VendorId = vendorId;
            desc.DeviceId = deviceId;
            desc.Flags = flags;
            desc.AdapterLuid.LowPart = static_cast<DWORD>(luid & 0xFFFFFFFF);
            desc.AdapterLuid.HighPart = static_cast<LONG>(luid >> 32);
            return desc;
        }

        const DXGI_ADAPTER_DESC1 c_warp = MakeDesc(L"Microsoft Basic Render Driver", 0x1414, 0x8c, DXGI_ADAPTER_FLAG_SOFTWARE, 0x6bdf);
        // What a Hyper-V VM actually enumerates for its display-only adapters: Basic Render Driver ids, no software flag.
        const DXGI_ADAPTER_DESC1 c_basicRenderNoFlag = MakeDesc(L"Microsoft Basic Render Driver", 0x1414, 0x8c, 0, 0x6b1d);
        const DXGI_ADAPTER_DESC1 c_hyperV = MakeDesc(L"Microsoft Hyper-V Video", 0x1414, 0x5353, 0, 0x400bb5ea9);
        const DXGI_ADAPTER_DESC1 c_remoteDisplay = MakeDesc(L"Microsoft Remote Display Adapter", 0x1414, 0x8e, 0, 0x7000);
        const DXGI_ADAPTER_DESC1 c_nvidia = MakeDesc(L"NVIDIA GeForce RTX 4070", 0x10de, 0x2786, 0, 0x1000000a1);
        const DXGI_ADAPTER_DESC1 c_amd = MakeDesc(L"AMD Radeon RX 7800 XT", 0x1002, 0x747e, 0, 0x2000000b2);
        const DXGI_ADAPTER_DESC1 c_intel = MakeDesc(L"Intel(R) Arc(TM) Graphics", 0x8086, 0x7d55, 0, 0xc3);

        // Sets (or clears, when value is null) an environment variable for the scope of a test.
        class ScopedEnvironment
        {
        public:
            ScopedEnvironment(const wchar_t* name, const wchar_t* value) : m_name(name)
            {
                wchar_t buffer[256]{};
                const DWORD length = GetEnvironmentVariableW(name, buffer, ARRAYSIZE(buffer));
                if (length > 0 && length < ARRAYSIZE(buffer))
                {
                    m_saved = buffer;
                }
                SetEnvironmentVariableW(name, value);
            }
            ~ScopedEnvironment()
            {
                SetEnvironmentVariableW(m_name.c_str(), m_saved ? m_saved->c_str() : nullptr);
            }

        private:
            std::wstring m_name;
            std::optional<std::wstring> m_saved;
        };

        struct MachineAdapter
        {
            DXGI_ADAPTER_DESC1 desc;
            ComPtr<IDXGIAdapter1> adapter;
        };

        // Independent of HardwareAdapter::EnumerateAdapters so the factory is checked against the machine, not itself.
        std::vector<MachineAdapter> EnumerateMachine()
        {
            ComPtr<IDXGIFactory1> factory;
            VERIFY_SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory)));
            std::vector<MachineAdapter> adapters;
            ComPtr<IDXGIAdapter1> adapter;
            for (UINT index = 0; factory->EnumAdapters1(index, &adapter) != DXGI_ERROR_NOT_FOUND; ++index)
            {
                DXGI_ADAPTER_DESC1 desc{};
                VERIFY_SUCCEEDED(adapter->GetDesc1(&desc));
                Log::Comment(String().Format(L"adapter %u: '%s' %04x:%04x flags 0x%x luid 0x%016llx -> %s", index,
                    desc.Description, desc.VendorId, desc.DeviceId, desc.Flags, PackLuid(desc.AdapterLuid),
                    VerdictName(Classify(desc))));
                adapters.push_back({ desc, adapter });
                adapter.Reset();
            }
            return adapters;
        }

        std::vector<uint64_t> QualifyingLuids(const std::vector<MachineAdapter>& adapters)
        {
            std::vector<uint64_t> luids;
            for (const auto& adapter : adapters)
            {
                if (Qualifies(adapter.desc))
                {
                    luids.push_back(PackLuid(adapter.desc.AdapterLuid));
                }
            }
            return luids;
        }

        DXGI_ADAPTER_DESC1 DeviceAdapterDesc(ID3D11Device* device)
        {
            ComPtr<IDXGIDevice> dxgiDevice;
            ComPtr<IDXGIAdapter> adapter;
            ComPtr<IDXGIAdapter1> adapter1;
            DXGI_ADAPTER_DESC1 desc{};
            VERIFY_SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&dxgiDevice)));
            VERIFY_SUCCEEDED(dxgiDevice->GetAdapter(&adapter));
            VERIFY_SUCCEEDED(adapter.As(&adapter1));
            VERIFY_SUCCEEDED(adapter1->GetDesc1(&desc));
            return desc;
        }

        void VerifyHardwareDevice(HRESULT hr, ID3D11Device* device, const DeviceIdentity& identity, const std::vector<uint64_t>& qualifying)
        {
            VERIFY_SUCCEEDED(hr);
            VERIFY_IS_NOT_NULL(device);
            VERIFY_IS_TRUE(identity.mode == Mode::ImmediateD2DHardware);
            VERIFY_IS_FALSE(identity.isWarp);
            VERIFY_IS_TRUE(identity.featureLevel >= static_cast<uint32_t>(D3D_FEATURE_LEVEL_10_0));
            VERIFY_IS_TRUE(std::find(qualifying.begin(), qualifying.end(), identity.adapterLuid) != qualifying.end(),
                String().Format(L"device LUID 0x%016llx is a qualifying adapter", identity.adapterLuid));
            const auto desc = DeviceAdapterDesc(device);
            VERIFY_IS_TRUE(Qualifies(desc), String().Format(L"device adapter '%s' qualifies", desc.Description));
            VERIFY_ARE_EQUAL(identity.adapterLuid, PackLuid(desc.AdapterLuid));
        }

        void VerifyNotFound(HRESULT hr, ID3D11Device* device, const DeviceIdentity& identity)
        {
            VERIFY_ARE_EQUAL(DXGI_ERROR_NOT_FOUND, hr);
            VERIFY_IS_NULL(device);
            VERIFY_ARE_EQUAL(0ull, identity.adapterLuid);
            VERIFY_IS_FALSE(identity.isWarp);
        }
    }

    // ---- HardwareAdapterSelectionTests ----

    void HardwareAdapterSelectionTests::RejectsSoftwareFlag()
    {
        VERIFY_IS_TRUE(Classify(c_warp) == Verdict::SoftwareAdapter);
        // The flag wins even on a vendor that would otherwise qualify.
        VERIFY_IS_TRUE(Classify(MakeDesc(L"Software NVIDIA", 0x10de, 0x1, DXGI_ADAPTER_FLAG_SOFTWARE, 1)) == Verdict::SoftwareAdapter);
        VERIFY_IS_FALSE(Qualifies(c_warp));
    }

    void HardwareAdapterSelectionTests::RejectsBasicRenderDriverWithoutSoftwareFlag()
    {
        VERIFY_IS_TRUE(Classify(c_basicRenderNoFlag) == Verdict::BasicRenderDriver);
        VERIFY_IS_FALSE(Qualifies(c_basicRenderNoFlag));
    }

    void HardwareAdapterSelectionTests::RejectsHyperVAndRemoteDisplayAdapters()
    {
        VERIFY_IS_TRUE(Classify(c_hyperV) == Verdict::MicrosoftVirtualAdapter);
        VERIFY_IS_TRUE(Classify(c_remoteDisplay) == Verdict::MicrosoftVirtualAdapter);
        VERIFY_IS_FALSE(Qualifies(c_hyperV));
        VERIFY_IS_FALSE(Qualifies(c_remoteDisplay));
    }

    void HardwareAdapterSelectionTests::AcceptsNvidiaAmdAndIntel()
    {
        VERIFY_IS_TRUE(Classify(c_nvidia) == Verdict::Qualifies);
        VERIFY_IS_TRUE(Classify(c_amd) == Verdict::Qualifies);
        VERIFY_IS_TRUE(Classify(c_intel) == Verdict::Qualifies);
    }

    void HardwareAdapterSelectionTests::ChoosesFirstQualifyingInPreferenceOrder()
    {
        // EnumAdapterByGpuPreference(HIGH_PERFORMANCE) lists the discrete GPU first; the factory keeps that order.
        const DXGI_ADAPTER_DESC1 highPerformanceOrder[] = { c_hyperV, c_nvidia, c_intel, c_warp };
        VERIFY_ARE_EQUAL(size_t{ 1 }, ChooseAdapter(highPerformanceOrder, ARRAYSIZE(highPerformanceOrder), 0));
        const DXGI_ADAPTER_DESC1 integratedFirst[] = { c_warp, c_intel, c_amd };
        VERIFY_ARE_EQUAL(size_t{ 1 }, ChooseAdapter(integratedFirst, ARRAYSIZE(integratedFirst), 0));
    }

    void HardwareAdapterSelectionTests::ChoosesNothingWhenNoneQualifies()
    {
        // The adapter set of a Hyper-V VM without GPU-P.
        const DXGI_ADAPTER_DESC1 vm[] = { c_warp, c_basicRenderNoFlag, c_hyperV, c_remoteDisplay };
        VERIFY_ARE_EQUAL(NoAdapter, ChooseAdapter(vm, ARRAYSIZE(vm), 0));
        VERIFY_ARE_EQUAL(NoAdapter, ChooseAdapter(nullptr, 0, 0));
    }

    void HardwareAdapterSelectionTests::OverridePinsTheNamedAdapter()
    {
        const DXGI_ADAPTER_DESC1 adapters[] = { c_nvidia, c_amd, c_intel, c_warp };
        VERIFY_ARE_EQUAL(size_t{ 2 }, ChooseAdapter(adapters, ARRAYSIZE(adapters), PackLuid(c_intel.AdapterLuid)));
        VERIFY_ARE_EQUAL(size_t{ 1 }, ChooseAdapter(adapters, ARRAYSIZE(adapters), PackLuid(c_amd.AdapterLuid)));
    }

    void HardwareAdapterSelectionTests::OverrideNeverSubstitutesAnotherAdapter()
    {
        const DXGI_ADAPTER_DESC1 adapters[] = { c_nvidia, c_basicRenderNoFlag, c_warp };
        VERIFY_ARE_EQUAL(NoAdapter, ChooseAdapter(adapters, ARRAYSIZE(adapters), 0xdeadbeefull));
        VERIFY_ARE_EQUAL(NoAdapter, ChooseAdapter(adapters, ARRAYSIZE(adapters), PackLuid(c_warp.AdapterLuid)));
        VERIFY_ARE_EQUAL(NoAdapter, ChooseAdapter(adapters, ARRAYSIZE(adapters), PackLuid(c_basicRenderNoFlag.AdapterLuid)));
    }

    void HardwareAdapterSelectionTests::ParsesLuidOverride()
    {
        uint64_t luid = 0;
        VERIFY_IS_TRUE(TryParseLuidOverride(L"1a2b", luid));
        VERIFY_ARE_EQUAL(0x1a2bull, luid);
        VERIFY_IS_TRUE(TryParseLuidOverride(L"0x0000000400BB5EA9", luid));
        VERIFY_ARE_EQUAL(0x400bb5ea9ull, luid);
        VERIFY_IS_TRUE(TryParseLuidOverride(L"0XFFFFFFFFFFFFFFFF", luid));
        VERIFY_ARE_EQUAL(0xFFFFFFFFFFFFFFFFull, luid);
        VERIFY_IS_TRUE(TryParseLuidOverride(L"  0x6b1d \t", luid));
        VERIFY_ARE_EQUAL(0x6b1dull, luid);
    }

    void HardwareAdapterSelectionTests::RejectsMalformedLuidOverride()
    {
        for (const wchar_t* text : { L"", L"   ", L"0x", L"0", L"0x0", L"xyz", L"12g4", L"0x 12", L"1 2", L"-1",
                 L"12345678901234567", L"0x10000000000000000" })
        {
            uint64_t luid = 7;
            VERIFY_IS_FALSE(TryParseLuidOverride(text, luid), String().Format(L"'%s' is rejected", text));
            VERIFY_ARE_EQUAL(0ull, luid);
        }
        uint64_t luid = 7;
        VERIFY_IS_FALSE(TryParseLuidOverride(nullptr, luid));
    }

    void HardwareAdapterSelectionTests::PacksLuidLikeDeviceInfo()
    {
        LUID luid{};
        luid.LowPart = 0xbb5ea9;
        luid.HighPart = 4;
        VERIFY_ARE_EQUAL(0x400bb5ea9ull, PackLuid(luid));
        luid.LowPart = 0xFFFFFFFF;
        luid.HighPart = -1;
        VERIFY_ARE_EQUAL(0xFFFFFFFFFFFFFFFFull, PackLuid(luid));
    }

    // ---- HardwareDeviceFactoryTests ----

    void HardwareDeviceFactoryTests::HardwareDeviceMatchesMachineEnumeration()
    {
        ScopedEnvironment noOverride(AdapterLuidEnvironmentVariable, nullptr);
        const auto machine = EnumerateMachine();
        const auto qualifying = QualifyingLuids(machine);

        AdapterList list;
        VERIFY_SUCCEEDED(EnumerateAdapters(list));
        VERIFY_ARE_EQUAL(machine.size(), list.count);
        Log::Comment(String().Format(L"factory order from %s", list.usedGpuPreference ? L"EnumAdapterByGpuPreference" : L"EnumAdapters1"));

        ComPtr<ID3D11Device> device;
        DeviceIdentity identity{};
        const HRESULT hr = CreateD3DDeviceForMode(Mode::ImmediateD2DHardware, &device, &identity);
        if (qualifying.empty())
        {
            Log::Comment(L"No qualifying hardware adapter on this machine: D2D-HW must report DXGI_ERROR_NOT_FOUND.");
            VerifyNotFound(hr, device.Get(), identity);
        }
        else
        {
            VerifyHardwareDevice(hr, device.Get(), identity, qualifying);
            // With no override the factory takes the first qualifying adapter in its own preference order.
            const size_t expected = ChooseAdapter(list.descs, list.count, 0);
            VERIFY_ARE_NOT_EQUAL(NoAdapter, expected);
            VERIFY_ARE_EQUAL(PackLuid(list.descs[expected].AdapterLuid), identity.adapterLuid);
        }
    }

    void HardwareDeviceFactoryTests::HardwareDeviceIsNeverWarpOrBasicRender()
    {
        ScopedEnvironment noOverride(AdapterLuidEnvironmentVariable, nullptr);
        ComPtr<ID3D11Device> device;
        DeviceIdentity identity{};
        const HRESULT hr = CreateD3DDeviceForMode(Mode::ImmediateD2DHardware, &device, &identity);
        if (hr == DXGI_ERROR_NOT_FOUND)
        {
            VerifyNotFound(hr, device.Get(), identity);
            return;
        }
        VERIFY_SUCCEEDED(hr);
        VERIFY_IS_FALSE(identity.isWarp);
        const auto desc = DeviceAdapterDesc(device.Get());
        VERIFY_ARE_EQUAL(0u, desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE);
        VERIFY_ARE_NOT_EQUAL(MicrosoftVendorId, desc.VendorId);
    }

    void HardwareDeviceFactoryTests::MicrosoftAdaptersDoNotProvideHardwareRendering()
    {
        // Evidence for rejecting every 0x1414 adapter: a hardware-type device on a Microsoft display-only adapter
        // (Hyper-V Video, Remote Display) is the Basic Render Driver. If this ever fails, revisit the policy.
        uint32_t probed = 0;
        for (const auto& candidate : EnumerateMachine())
        {
            if (candidate.desc.VendorId != MicrosoftVendorId || (candidate.desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0)
            {
                continue;
            }
            ComPtr<ID3D11Device> device;
            const HRESULT hr = D3D11CreateDevice(candidate.adapter.Get(), D3D_DRIVER_TYPE_UNKNOWN, nullptr,
                D3D11_CREATE_DEVICE_BGRA_SUPPORT, nullptr, 0, D3D11_SDK_VERSION, &device, nullptr, nullptr);
            ++probed;
            if (FAILED(hr))
            {
                Log::Comment(String().Format(L"'%s': D3D11CreateDevice failed 0x%08x (no rendering)", candidate.desc.Description, hr));
                continue;
            }
            const auto desc = DeviceAdapterDesc(device.Get());
            Log::Comment(String().Format(L"'%s' -> device on '%s' %04x:%04x, feature level 0x%x", candidate.desc.Description,
                desc.Description, desc.VendorId, desc.DeviceId, device->GetFeatureLevel()));
            VERIFY_ARE_EQUAL(MicrosoftVendorId, desc.VendorId);
            VERIFY_ARE_EQUAL(BasicRenderDriverDeviceId, desc.DeviceId);
        }
        if (probed == 0)
        {
            Log::Result(TestResults::Skipped, L"No non-software Microsoft adapters on this machine.");
        }
    }

    void HardwareDeviceFactoryTests::OverrideToUnknownOrRejectedAdapterIsNotFound()
    {
        {
            ScopedEnvironment pin(AdapterLuidEnvironmentVariable, L"0xFFFFFFF0DEADBEEF");
            ComPtr<ID3D11Device> device;
            DeviceIdentity identity{};
            VerifyNotFound(CreateD3DDeviceForMode(Mode::ImmediateD2DHardware, &device, &identity), device.Get(), identity);
        }
        for (const auto& candidate : EnumerateMachine())
        {
            if (Qualifies(candidate.desc))
            {
                continue;
            }
            wchar_t text[32]{};
            swprintf_s(text, L"0x%016llx", PackLuid(candidate.desc.AdapterLuid));
            ScopedEnvironment pin(AdapterLuidEnvironmentVariable, text);
            ComPtr<ID3D11Device> device;
            DeviceIdentity identity{};
            Log::Comment(String().Format(L"pinning rejected adapter '%s' (%s)", candidate.desc.Description, text));
            VerifyNotFound(CreateD3DDeviceForMode(Mode::ImmediateD2DHardware, &device, &identity), device.Get(), identity);
        }
    }

    void HardwareDeviceFactoryTests::OverrideToQualifyingAdapterIsHonored()
    {
        const auto machine = EnumerateMachine();
        const auto qualifying = QualifyingLuids(machine);
        if (qualifying.empty())
        {
            Log::Result(TestResults::Skipped, L"No hardware D3D adapter on this machine.");
            return;
        }
        for (const uint64_t luid : qualifying)
        {
            wchar_t text[32]{};
            swprintf_s(text, L"%llx", luid);
            ScopedEnvironment pin(AdapterLuidEnvironmentVariable, text);
            ComPtr<ID3D11Device> device;
            DeviceIdentity identity{};
            const HRESULT hr = CreateD3DDeviceForMode(Mode::ImmediateD2DHardware, &device, &identity);
            VerifyHardwareDevice(hr, device.Get(), identity, qualifying);
            VERIFY_ARE_EQUAL(luid, identity.adapterLuid);
        }
    }

    void HardwareDeviceFactoryTests::MalformedOverrideFails()
    {
        ScopedEnvironment pin(AdapterLuidEnvironmentVariable, L"not-a-luid");
        ComPtr<ID3D11Device> device;
        DeviceIdentity identity{};
        VERIFY_ARE_EQUAL(E_INVALIDARG, CreateD3DDeviceForMode(Mode::ImmediateD2DHardware, &device, &identity));
        VERIFY_IS_NULL(device.Get());
    }

    void HardwareDeviceFactoryTests::WarpPathStillWorks()
    {
        // The override only affects the hardware path.
        ScopedEnvironment pin(AdapterLuidEnvironmentVariable, L"0xFFFFFFF0DEADBEEF");
        ComPtr<ID3D11Device> device;
        DeviceIdentity identity{};
        VERIFY_SUCCEEDED(CreateD3DDeviceForMode(Mode::ImmediateD2DSoftware, &device, &identity));
        VERIFY_IS_NOT_NULL(device.Get());
        VERIFY_IS_TRUE(identity.mode == Mode::ImmediateD2DSoftware);
        VERIFY_IS_TRUE(identity.isWarp);
        VERIFY_IS_TRUE(identity.featureLevel != 0);
        const auto desc = DeviceAdapterDesc(device.Get());
        VERIFY_ARE_EQUAL(identity.adapterLuid, PackLuid(desc.AdapterLuid));
        // The WARP adapter is exactly what the hardware policy rejects.
        VERIFY_IS_FALSE(Qualifies(desc));
    }
}
