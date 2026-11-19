// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <ImmediatePainter.h>
#include <D2DDeviceFactory.h>
#include <d3d11.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <cwctype>
#include <iterator>
#include <utility>

// D3D device creation for the D2D modes. ImmediateD2DSoftware => WARP. ImmediateD2DHardware => the adapter chosen by
// RenderingBackend::HardwareAdapter (see D2DDeviceFactory.h for the policy); never WARP or the Basic Render Driver.
namespace RenderingBackend
{
    namespace
    {
        constexpr D3D_FEATURE_LEVEL c_featureLevels[] =
        {
            D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0,
        };

        HRESULT FillIdentity(Mode mode, ID3D11Device* device, bool isWarp, DeviceIdentity* identity) noexcept
        {
            Microsoft::WRL::ComPtr<IDXGIDevice> dxgiDevice;
            Microsoft::WRL::ComPtr<IDXGIAdapter> adapter;
            DXGI_ADAPTER_DESC desc{};
            HRESULT hr = device->QueryInterface(IID_PPV_ARGS(&dxgiDevice));
            if (SUCCEEDED(hr)) hr = dxgiDevice->GetAdapter(&adapter);
            if (SUCCEEDED(hr)) hr = adapter->GetDesc(&desc);
            if (FAILED(hr))
            {
                return hr;
            }
            identity->mode = mode;
            identity->adapterLuid = HardwareAdapter::PackLuid(desc.AdapterLuid);
            identity->isWarp = isWarp;
            identity->featureLevel = static_cast<uint32_t>(device->GetFeatureLevel());
            return S_OK;
        }

        HRESULT CreateDeviceOnAdapter(IDXGIAdapter1* adapter, ID3D11Device** device) noexcept
        {
            HRESULT hr = D3D11CreateDevice(adapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT,
                c_featureLevels, static_cast<UINT>(std::size(c_featureLevels)), D3D11_SDK_VERSION, device, nullptr, nullptr);
            if (hr == E_INVALIDARG)
            {
                // D3D11.0 runtimes reject D3D_FEATURE_LEVEL_11_1 in the list.
                hr = D3D11CreateDevice(adapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT,
                    c_featureLevels + 1, static_cast<UINT>(std::size(c_featureLevels) - 1), D3D11_SDK_VERSION, device, nullptr, nullptr);
            }
            return hr;
        }

        // Defense in depth: the device must sit on the adapter we chose, and that adapter must still qualify.
        bool DeviceIsOnQualifyingAdapter(ID3D11Device* device, uint64_t expectedLuid) noexcept
        {
            Microsoft::WRL::ComPtr<IDXGIDevice> dxgiDevice;
            Microsoft::WRL::ComPtr<IDXGIAdapter> adapter;
            Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter1;
            DXGI_ADAPTER_DESC1 desc{};
            return SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&dxgiDevice))) &&
                SUCCEEDED(dxgiDevice->GetAdapter(&adapter)) &&
                SUCCEEDED(adapter.As(&adapter1)) &&
                SUCCEEDED(adapter1->GetDesc1(&desc)) &&
                HardwareAdapter::Qualifies(desc) &&
                HardwareAdapter::PackLuid(desc.AdapterLuid) == expectedLuid;
        }

        HRESULT CreateHardwareDevice(ID3D11Device** device, DeviceIdentity* identity) noexcept
        {
            uint64_t overrideLuid = 0;
            HRESULT hr = HardwareAdapter::ReadLuidOverrideFromEnvironment(overrideLuid);
            if (FAILED(hr))
            {
                return hr;
            }

            HardwareAdapter::AdapterList list;
            hr = HardwareAdapter::EnumerateAdapters(list);
            if (FAILED(hr))
            {
                return hr;
            }

            const size_t index = HardwareAdapter::ChooseAdapter(list.descs, list.count, overrideLuid);
            if (index == HardwareAdapter::NoAdapter)
            {
                return DXGI_ERROR_NOT_FOUND;
            }

            Microsoft::WRL::ComPtr<ID3D11Device> hardware;
            hr = CreateDeviceOnAdapter(list.adapters[index].Get(), &hardware);
            if (FAILED(hr))
            {
                return hr;
            }
            if (!DeviceIsOnQualifyingAdapter(hardware.Get(), HardwareAdapter::PackLuid(list.descs[index].AdapterLuid)))
            {
                return DXGI_ERROR_NOT_FOUND;
            }

            hr = FillIdentity(Mode::ImmediateD2DHardware, hardware.Get(), false, identity);
            if (SUCCEEDED(hr))
            {
                TraceDeviceInfo(Mode::ImmediateD2DHardware, identity->adapterLuid, identity->isWarp, identity->featureLevel);
                *device = hardware.Detach();
            }
            return hr;
        }
    }

    namespace HardwareAdapter
    {
        Verdict Classify(const DXGI_ADAPTER_DESC1& desc) noexcept
        {
            if ((desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0)
            {
                return Verdict::SoftwareAdapter;
            }
            if (desc.VendorId == MicrosoftVendorId)
            {
                return desc.DeviceId == BasicRenderDriverDeviceId ? Verdict::BasicRenderDriver : Verdict::MicrosoftVirtualAdapter;
            }
            return Verdict::Qualifies;
        }

        const wchar_t* VerdictName(Verdict verdict) noexcept
        {
            switch (verdict)
            {
                case Verdict::Qualifies: return L"Qualifies";
                case Verdict::SoftwareAdapter: return L"SoftwareAdapter";
                case Verdict::BasicRenderDriver: return L"BasicRenderDriver";
                case Verdict::MicrosoftVirtualAdapter: return L"MicrosoftVirtualAdapter";
                default: return L"Unknown";
            }
        }

        bool TryParseLuidOverride(_In_opt_z_ const wchar_t* text, _Out_ uint64_t& luid) noexcept
        {
            luid = 0;
            if (text == nullptr)
            {
                return false;
            }
            while (*text == L' ' || *text == L'\t') ++text;
            if (text[0] == L'0' && (text[1] == L'x' || text[1] == L'X'))
            {
                text += 2;
            }

            uint64_t value = 0;
            uint32_t digits = 0;
            for (; std::iswxdigit(*text); ++text)
            {
                if (++digits > 16)
                {
                    return false;
                }
                const wchar_t c = *text;
                const uint32_t nibble = (c <= L'9') ? (c - L'0') : ((c | 0x20) - L'a' + 10);
                value = (value << 4) | nibble;
            }
            while (*text == L' ' || *text == L'\t') ++text;
            if (*text != L'\0' || digits == 0 || value == 0)
            {
                return false;
            }
            luid = value;
            return true;
        }

        HRESULT ReadLuidOverrideFromEnvironment(_Out_ uint64_t& luid) noexcept
        {
            luid = 0;
            wchar_t buffer[64]{};
            const DWORD length = GetEnvironmentVariableW(AdapterLuidEnvironmentVariable, buffer, ARRAYSIZE(buffer));
            if (length == 0)
            {
                return S_OK;
            }
            if (length >= ARRAYSIZE(buffer) || !TryParseLuidOverride(buffer, luid))
            {
                return E_INVALIDARG;
            }
            return S_OK;
        }

        size_t ChooseAdapter(_In_reads_(count) const DXGI_ADAPTER_DESC1* descs, size_t count, uint64_t overrideLuid) noexcept
        {
            for (size_t index = 0; index < count; ++index)
            {
                if (overrideLuid != 0 && PackLuid(descs[index].AdapterLuid) != overrideLuid)
                {
                    continue;
                }
                if (Qualifies(descs[index]))
                {
                    return index;
                }
                if (overrideLuid != 0)
                {
                    return NoAdapter; // the pinned adapter does not qualify; never substitute another
                }
            }
            return NoAdapter;
        }

        HRESULT EnumerateAdapters(_Out_ AdapterList& list) noexcept
        {
            for (auto& adapter : list.adapters) adapter.Reset();
            list.count = 0;
            list.usedGpuPreference = false;

            Microsoft::WRL::ComPtr<IDXGIFactory1> factory;
            HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&factory));
            if (FAILED(hr))
            {
                return hr;
            }

            Microsoft::WRL::ComPtr<IDXGIFactory6> factory6;
            const bool byPreference = SUCCEEDED(factory.As(&factory6));
            list.usedGpuPreference = byPreference;
            for (UINT index = 0; list.count < AdapterList::Capacity; ++index)
            {
                Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;
                hr = byPreference
                    ? factory6->EnumAdapterByGpuPreference(index, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter))
                    : factory->EnumAdapters1(index, &adapter);
                if (hr == DXGI_ERROR_NOT_FOUND)
                {
                    break;
                }
                if (FAILED(hr))
                {
                    return hr;
                }
                DXGI_ADAPTER_DESC1 desc{};
                if (SUCCEEDED(adapter->GetDesc1(&desc)))
                {
                    list.descs[list.count] = desc;
                    list.adapters[list.count] = std::move(adapter);
                    ++list.count;
                }
            }
            return S_OK;
        }
    }

    HRESULT CreateD3DDeviceForMode(Mode mode, ID3D11Device** device, DeviceIdentity* identity) noexcept
    {
        *device = nullptr;
        *identity = {};

        if (mode == Mode::ImmediateD2DSoftware)
        {
            Microsoft::WRL::ComPtr<ID3D11Device> warp;
            HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT,
                c_featureLevels, static_cast<UINT>(std::size(c_featureLevels)), D3D11_SDK_VERSION, &warp, nullptr, nullptr);
            if (SUCCEEDED(hr)) hr = FillIdentity(mode, warp.Get(), true, identity);
            if (SUCCEEDED(hr))
            {
                TraceDeviceInfo(mode, identity->adapterLuid, identity->isWarp, identity->featureLevel);
                *device = warp.Detach();
            }
            return hr;
        }

        if (mode == Mode::ImmediateD2DHardware)
        {
            const HRESULT hr = CreateHardwareDevice(device, identity);
            if (FAILED(hr))
            {
                *identity = {};
            }
            return hr;
        }

        return E_INVALIDARG;
    }
}
