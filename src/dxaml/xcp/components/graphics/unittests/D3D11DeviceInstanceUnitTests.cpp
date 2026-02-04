// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "D3D11DeviceInstanceUnitTests.h"
#include "D3D11DeviceInstance.h"
#include "D3D11SharedDeviceGuard.h"
#include "PalGfx.h"
#include "D2d1_1.h"
#include "MockDXGI.h"
#include "MockD3D11.h"
#include "MockD2D1.h"
#include "MockPlatformServices.h"

using namespace ::Windows::UI::Xaml::Tests::Graphics;

/* ------------------------------------------------------------------------------------------
    D3D11DeviceInstance Unit Tests
-------------------------------------------------------------------------------------------*/

void D3D11DeviceInstanceUnitTests::Creation()
{
    std::weak_ptr<CD3D11DeviceInstance> weakDeviceInstance0;
    std::weak_ptr<CD3D11DeviceInstance> weakDeviceInstance1;
    std::weak_ptr<CD3D11DeviceInstance> weakDeviceInstance2;
    std::weak_ptr<CD3D11DeviceInstance> weakDeviceInstance3;
    void* previousSharedPointer = nullptr;

    {
        // Unique device
        std::shared_ptr<CD3D11DeviceInstance> deviceInstance0;
        VERIFY_SUCCEEDED(CD3D11DeviceInstance::GetInstance(deviceInstance0, true));
        VERIFY_IS_NOT_NULL(deviceInstance0.get());
        weakDeviceInstance0 = deviceInstance0;

        {
            // Shared device
            std::shared_ptr<CD3D11DeviceInstance> deviceInstance1;
            VERIFY_SUCCEEDED(CD3D11DeviceInstance::GetInstance(deviceInstance1, false));
            VERIFY_IS_NOT_NULL(deviceInstance1.get());
            VERIFY_ARE_NOT_EQUAL(deviceInstance0.get(), deviceInstance1.get());
            weakDeviceInstance1 = deviceInstance1;
            previousSharedPointer = deviceInstance1.get();

            {
                // Shared device
                std::shared_ptr<CD3D11DeviceInstance> deviceInstance2;
                VERIFY_SUCCEEDED(CD3D11DeviceInstance::GetInstance(deviceInstance2, false));
                VERIFY_IS_NOT_NULL(deviceInstance2.get());
                VERIFY_ARE_EQUAL(deviceInstance1.get(), deviceInstance2.get());
                weakDeviceInstance2 = deviceInstance2;

                {
                    // Unique device
                    std::shared_ptr<CD3D11DeviceInstance> deviceInstance3;
                    VERIFY_SUCCEEDED(CD3D11DeviceInstance::GetInstance(deviceInstance3, true));
                    VERIFY_IS_NOT_NULL(deviceInstance3.get());
                    VERIFY_ARE_NOT_EQUAL(deviceInstance0.get(), deviceInstance3.get());
                    VERIFY_ARE_NOT_EQUAL(deviceInstance1.get(), deviceInstance3.get());
                    weakDeviceInstance3 = deviceInstance3;
                }
                VERIFY_IS_TRUE(weakDeviceInstance3.expired());
                VERIFY_IS_FALSE(weakDeviceInstance2.expired());
                VERIFY_IS_FALSE(weakDeviceInstance1.expired());
                VERIFY_IS_FALSE(weakDeviceInstance0.expired());

            }
            VERIFY_IS_FALSE(weakDeviceInstance2.expired());
            VERIFY_IS_FALSE(weakDeviceInstance1.expired());
            VERIFY_IS_FALSE(weakDeviceInstance0.expired());
        }
        VERIFY_IS_TRUE(weakDeviceInstance2.expired());
        VERIFY_IS_TRUE(weakDeviceInstance1.expired());
        VERIFY_IS_FALSE(weakDeviceInstance0.expired());
    }
    VERIFY_IS_TRUE(weakDeviceInstance0.expired());

    // Verify we will recreate after all references have been removed
    {
        std::shared_ptr<CD3D11DeviceInstance> deviceInstance;
        VERIFY_SUCCEEDED(CD3D11DeviceInstance::GetInstance(deviceInstance, true));
        VERIFY_IS_NOT_NULL(deviceInstance.get());
        VERIFY_ARE_NOT_EQUAL(deviceInstance.get(), previousSharedPointer);
    }
}

void D3D11DeviceInstanceUnitTests::EnsureDXGIAdapters()
{
    // Create our DXGI Mock helper
    CreateDXGIFactoryDetour mock;

    // Get a CD3D11Device
    std::shared_ptr<CD3D11DeviceInstance> deviceInstance;
    VERIFY_SUCCEEDED(CD3D11DeviceInstance::GetInstance(deviceInstance, false));
    VERIFY_IS_NOT_NULL(deviceInstance.get());

    // Each configuration is iterated over multiple times to ensure consistent state when
    // we don't actually load a new set of adapters.

    LOG_OUTPUT(L"No Adapters");
    {
        mock.InvalidateCurrentFactory();
        for (int i = 0; i < 2; i++)
        {
            VERIFY_ARE_EQUAL(DXGI_ERROR_NOT_FOUND, deviceInstance->EnsureDXGIAdapters());
        }
    }

    LOG_OUTPUT(L"Software Adapter (No Outputs) only");
    {
        mock.InvalidateCurrentFactory();
        LUID luid = mock.Factory->AddAdapter(MockDXGIAdapterType::WarpAdapterWithoutOutputs);
        for (int i = 0; i < 2; i++)
        {
            VERIFY_SUCCEEDED(deviceInstance->EnsureDXGIAdapters());
            VERIFY_IS_NOT_NULL(deviceInstance->m_warpAdapter);
            VERIFY_IS_NULL(deviceInstance->m_hardwareAdapter);
            VERIFY_IS_TRUE(AreAdapterLuidsEqual(GetAdapterLuid(deviceInstance->m_warpAdapter.Get()), luid));
        }
    }

    LOG_OUTPUT(L"Software Adapter (with Outputs) only");
    {
        mock.InvalidateCurrentFactory();
        LUID luid = mock.Factory->AddAdapter(MockDXGIAdapterType::WarpAdapterWithOutputs);
        for (int i = 0; i < 2; i++)
        {
            VERIFY_SUCCEEDED(deviceInstance->EnsureDXGIAdapters());
            VERIFY_IS_NOT_NULL(deviceInstance->m_warpAdapter);
            VERIFY_IS_NULL(deviceInstance->m_hardwareAdapter);
            VERIFY_IS_TRUE(AreAdapterLuidsEqual(GetAdapterLuid(deviceInstance->m_warpAdapter.Get()), luid));
        }
    }

    LOG_OUTPUT(L"Multiple Software Adapters (with and without Outputs)");
    {
        mock.InvalidateCurrentFactory();
        LUID WarpWithoutOutputsLuid = mock.Factory->AddAdapter(MockDXGIAdapterType::WarpAdapterWithoutOutputs);
        LUID WarpWithOutputsLuid = mock.Factory->AddAdapter(MockDXGIAdapterType::WarpAdapterWithOutputs);
        for (int i = 0; i < 2; i++)
        {
            VERIFY_SUCCEEDED(deviceInstance->EnsureDXGIAdapters());
            VERIFY_IS_NOT_NULL(deviceInstance->m_warpAdapter);
            VERIFY_IS_NULL(deviceInstance->m_hardwareAdapter);
            VERIFY_IS_TRUE(AreAdapterLuidsEqual(GetAdapterLuid(deviceInstance->m_warpAdapter.Get()), WarpWithOutputsLuid));
        }
    }

    LOG_OUTPUT(L"Hardware Only");  // probably not a real scenario, but here for completeness
    {
        mock.InvalidateCurrentFactory();
        LUID hardwareLuid = mock.Factory->AddAdapter(MockDXGIAdapterType::HardwareAdapter);
        for (int i = 0; i < 2; i++)
        {
            VERIFY_SUCCEEDED(deviceInstance->EnsureDXGIAdapters());
            VERIFY_IS_NULL(deviceInstance->m_warpAdapter);
            VERIFY_IS_NOT_NULL(deviceInstance->m_hardwareAdapter);
            VERIFY_IS_TRUE(AreAdapterLuidsEqual(GetAdapterLuid(deviceInstance->m_hardwareAdapter.Get()), hardwareLuid));
        }
    }

    LOG_OUTPUT(L"Hardware and Software Adapters");
    {
        mock.InvalidateCurrentFactory();
        LUID hardwareLuid = mock.Factory->AddAdapter(MockDXGIAdapterType::HardwareAdapter);
        LUID warpLuid = mock.Factory->AddAdapter(MockDXGIAdapterType::WarpAdapterWithOutputs);
        for (int i = 0; i < 2; i++)
        {
            VERIFY_SUCCEEDED(deviceInstance->EnsureDXGIAdapters());
            VERIFY_IS_NOT_NULL(deviceInstance->m_warpAdapter);
            VERIFY_IS_NOT_NULL(deviceInstance->m_hardwareAdapter);
            VERIFY_IS_TRUE(AreAdapterLuidsEqual(GetAdapterLuid(deviceInstance->m_warpAdapter.Get()), warpLuid));
            VERIFY_IS_TRUE(AreAdapterLuidsEqual(GetAdapterLuid(deviceInstance->m_hardwareAdapter.Get()), hardwareLuid));
        }
    }
}

void D3D11DeviceInstanceUnitTests::EnsureResources()
{
    // Create our Mocks
    PlatformServicesMocksHelper platformMock;
    CreateDXGIFactoryDetour DXGIMock;
    CreateD3D11DeviceDetour D3DMock;

    LOG_OUTPUT(L"Hardware - Feature Level 9_1 - No Native Resource Manipulation");
    {
        DXGIMock.InvalidateCurrentFactory();
        LUID hardwareLuid = DXGIMock.Factory->AddAdapter(MockDXGIAdapterType::HardwareAdapter);
        LUID WarpLuid = DXGIMock.Factory->AddAdapter(MockDXGIAdapterType::WarpAdapterWithOutputs);

        Microsoft::WRL::ComPtr<IMockDXGIAdapter> adapter;
        VERIFY_SUCCEEDED(DXGIMock.Factory->GetMockAdapter(hardwareLuid, &adapter));
        adapter->SetFeatureLevel(D3D_FEATURE_LEVEL_9_1);
        adapter->SetPartnerCaps(0);

        std::shared_ptr<CD3D11DeviceInstance> deviceInstance;
        VERIFY_SUCCEEDED(CD3D11DeviceInstance::GetInstance(deviceInstance, false));
        VERIFY_IS_NOT_NULL(deviceInstance.get());

        VERIFY_SUCCEEDED(deviceInstance->EnsureResources());

        VERIFY_IS_TRUE(!!deviceInstance->IsOnSameAdapter(&hardwareLuid));
        VERIFY_IS_TRUE(!!deviceInstance->UseIntermediateUploadSurface());

        VERIFY_IS_TRUE(deviceInstance->m_fIsHardwareOutput);

        Microsoft::WRL::ComPtr<IMockDXGIDevice> mockDevice;
        CD3D11SharedDeviceGuard guard;
        VERIFY_SUCCEEDED(deviceInstance->TakeLockAndCheckDeviceLost(&guard));
        VERIFY_SUCCEEDED(deviceInstance->GetDevice(&guard)->QueryInterface(__uuidof(IMockDXGIDevice), &mockDevice));
        VERIFY_ARE_EQUAL(mockDevice->GetDriverType(), D3D_DRIVER_TYPE_UNKNOWN);
        VERIFY_ARE_EQUAL(mockDevice->GetFlags(), D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_PREVENT_INTERNAL_THREADING_OPTIMIZATIONS);

        Microsoft::WRL::ComPtr<ID3D10Multithread> multithread;
        VERIFY_SUCCEEDED(deviceInstance->GetDevice(&guard)->QueryInterface(__uuidof(ID3D10Multithread), &multithread));
        VERIFY_IS_FALSE(!!multithread->GetMultithreadProtected());
    }

    LOG_OUTPUT(L"Hardware - Feature Level 9_1 - Native Resource Manipulation");
    {
        DXGIMock.InvalidateCurrentFactory();
        LUID hardwareLuid = DXGIMock.Factory->AddAdapter(MockDXGIAdapterType::HardwareAdapter);
        LUID WarpLuid = DXGIMock.Factory->AddAdapter(MockDXGIAdapterType::WarpAdapterWithOutputs);

        Microsoft::WRL::ComPtr<IMockDXGIAdapter> adapter;
        VERIFY_SUCCEEDED(DXGIMock.Factory->GetMockAdapter(hardwareLuid, &adapter));
        adapter->SetFeatureLevel(D3D_FEATURE_LEVEL_9_1);
        adapter->SetPartnerCaps(1);

        std::shared_ptr<CD3D11DeviceInstance> deviceInstance;
        VERIFY_SUCCEEDED(CD3D11DeviceInstance::GetInstance(deviceInstance, false));
        VERIFY_IS_NOT_NULL(deviceInstance.get());

        VERIFY_SUCCEEDED(deviceInstance->EnsureResources());

        VERIFY_IS_TRUE(!!deviceInstance->IsOnSameAdapter(&hardwareLuid));
        VERIFY_IS_TRUE(!!deviceInstance->UseIntermediateUploadSurface());

        VERIFY_IS_TRUE(deviceInstance->m_fIsHardwareOutput);

        Microsoft::WRL::ComPtr<IMockDXGIDevice> mockDevice;
        CD3D11SharedDeviceGuard guard;
        VERIFY_SUCCEEDED(deviceInstance->TakeLockAndCheckDeviceLost(&guard));
        VERIFY_SUCCEEDED(deviceInstance->GetDevice(&guard)->QueryInterface(__uuidof(IMockDXGIDevice), &mockDevice));
        VERIFY_ARE_EQUAL(mockDevice->GetDriverType(), D3D_DRIVER_TYPE_UNKNOWN);
        VERIFY_ARE_EQUAL(mockDevice->GetFlags(), D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_PREVENT_INTERNAL_THREADING_OPTIMIZATIONS);

        Microsoft::WRL::ComPtr<ID3D10Multithread> multithread;
        VERIFY_SUCCEEDED(deviceInstance->GetDevice(&guard)->QueryInterface(__uuidof(ID3D10Multithread), &multithread));
        VERIFY_IS_FALSE(!!multithread->GetMultithreadProtected());
    }

    LOG_OUTPUT(L"Warp - Feature Level 9_1 - Native Resource Manipulation");
    {
        DXGIMock.InvalidateCurrentFactory();
        LUID WarpLuid = DXGIMock.Factory->AddAdapter(MockDXGIAdapterType::WarpAdapterWithOutputs);

        Microsoft::WRL::ComPtr<IMockDXGIAdapter> adapter;
        VERIFY_SUCCEEDED(DXGIMock.Factory->GetMockAdapter(WarpLuid, &adapter));
        adapter->SetFeatureLevel(D3D_FEATURE_LEVEL_9_1);
        adapter->SetPartnerCaps(1);

        std::shared_ptr<CD3D11DeviceInstance> deviceInstance;
        VERIFY_SUCCEEDED(CD3D11DeviceInstance::GetInstance(deviceInstance, false));
        VERIFY_IS_NOT_NULL(deviceInstance.get());

        VERIFY_SUCCEEDED(deviceInstance->EnsureResources());

        VERIFY_IS_TRUE(!!deviceInstance->IsOnSameAdapter(&WarpLuid));
        VERIFY_IS_FALSE(!!deviceInstance->UseIntermediateUploadSurface());

        VERIFY_IS_FALSE(deviceInstance->m_fIsHardwareOutput);

        Microsoft::WRL::ComPtr<IMockDXGIDevice> mockDevice;
        CD3D11SharedDeviceGuard guard;
        VERIFY_SUCCEEDED(deviceInstance->TakeLockAndCheckDeviceLost(&guard));
        VERIFY_SUCCEEDED(deviceInstance->GetDevice(&guard)->QueryInterface(__uuidof(IMockDXGIDevice), &mockDevice));
        VERIFY_ARE_EQUAL(mockDevice->GetDriverType(), D3D_DRIVER_TYPE_UNKNOWN);
        VERIFY_ARE_EQUAL(mockDevice->GetFlags(), D3D11_CREATE_DEVICE_BGRA_SUPPORT);

        Microsoft::WRL::ComPtr<ID3D10Multithread> multithread;
        VERIFY_SUCCEEDED(deviceInstance->GetDevice(&guard)->QueryInterface(__uuidof(ID3D10Multithread), &multithread));
        VERIFY_IS_FALSE(!!multithread->GetMultithreadProtected());
    }

    LOG_OUTPUT(L"Hardware - Feature Level 11_0 - No Native Resource Manipulation");
    {
        DXGIMock.InvalidateCurrentFactory();
        LUID hardwareLuid = DXGIMock.Factory->AddAdapter(MockDXGIAdapterType::HardwareAdapter);
        LUID WarpLuid = DXGIMock.Factory->AddAdapter(MockDXGIAdapterType::WarpAdapterWithOutputs);

        Microsoft::WRL::ComPtr<IMockDXGIAdapter> adapter;
        VERIFY_SUCCEEDED(DXGIMock.Factory->GetMockAdapter(hardwareLuid, &adapter));
        adapter->SetFeatureLevel(D3D_FEATURE_LEVEL_11_0);
        adapter->SetPartnerCaps(0);

        std::shared_ptr<CD3D11DeviceInstance> deviceInstance;
        VERIFY_SUCCEEDED(CD3D11DeviceInstance::GetInstance(deviceInstance, false));
        VERIFY_IS_NOT_NULL(deviceInstance.get());

        VERIFY_SUCCEEDED(deviceInstance->EnsureResources());

        VERIFY_IS_TRUE(!!deviceInstance->IsOnSameAdapter(&hardwareLuid));
        VERIFY_IS_FALSE(!!deviceInstance->UseIntermediateUploadSurface());

        VERIFY_IS_TRUE(deviceInstance->m_fIsHardwareOutput);

        Microsoft::WRL::ComPtr<IMockDXGIDevice> mockDevice;
        CD3D11SharedDeviceGuard guard;
        VERIFY_SUCCEEDED(deviceInstance->TakeLockAndCheckDeviceLost(&guard));
        VERIFY_SUCCEEDED(deviceInstance->GetDevice(&guard)->QueryInterface(__uuidof(IMockDXGIDevice), &mockDevice));
        VERIFY_ARE_EQUAL(mockDevice->GetDriverType(), D3D_DRIVER_TYPE_UNKNOWN);
        VERIFY_ARE_EQUAL(mockDevice->GetFlags(), D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_PREVENT_INTERNAL_THREADING_OPTIMIZATIONS);

        Microsoft::WRL::ComPtr<ID3D10Multithread> multithread;
        VERIFY_SUCCEEDED(deviceInstance->GetDevice(&guard)->QueryInterface(__uuidof(ID3D10Multithread), &multithread));
        VERIFY_IS_FALSE(!!multithread->GetMultithreadProtected());
    }

    LOG_OUTPUT(L"Warp Fallback");
    {
        DXGIMock.InvalidateCurrentFactory();
        LUID hardwareLuid = DXGIMock.Factory->AddAdapter(MockDXGIAdapterType::HardwareAdapter);
        LUID warpLuid = DXGIMock.Factory->AddAdapter(MockDXGIAdapterType::WarpAdapterWithOutputs);

        Microsoft::WRL::ComPtr<IMockDXGIAdapter> adapter;
        VERIFY_SUCCEEDED(DXGIMock.Factory->GetMockAdapter(hardwareLuid, &adapter));
        adapter->Remove();

        std::shared_ptr<CD3D11DeviceInstance> deviceInstance;
        VERIFY_SUCCEEDED(CD3D11DeviceInstance::GetInstance(deviceInstance, false));
        VERIFY_IS_NOT_NULL(deviceInstance.get());

        VERIFY_SUCCEEDED(deviceInstance->EnsureResources());

        VERIFY_IS_TRUE(!!deviceInstance->IsOnSameAdapter(&warpLuid));
        VERIFY_IS_TRUE(deviceInstance->m_fIsHardwareOutput); // This is probably wrong, but is the way it currently works.  Is Hardware Output is based on whether there is a hardware adapter, not whether it is used.

        Microsoft::WRL::ComPtr<IMockDXGIDevice> mockDevice;
        CD3D11SharedDeviceGuard guard;
        VERIFY_SUCCEEDED(deviceInstance->TakeLockAndCheckDeviceLost(&guard));
        VERIFY_SUCCEEDED(deviceInstance->GetDevice(&guard)->QueryInterface(__uuidof(IMockDXGIDevice), &mockDevice));
        VERIFY_ARE_EQUAL(mockDevice->GetDriverType(), D3D_DRIVER_TYPE_UNKNOWN);
        VERIFY_ARE_EQUAL(mockDevice->GetFlags(), D3D11_CREATE_DEVICE_BGRA_SUPPORT);
    }

    LOG_OUTPUT(L"Warp Fallback Failure");
    {
        DXGIMock.InvalidateCurrentFactory();
        LUID hardwareLuid = DXGIMock.Factory->AddAdapter(MockDXGIAdapterType::HardwareAdapter);
        LUID warpLuid = DXGIMock.Factory->AddAdapter(MockDXGIAdapterType::WarpAdapterWithOutputs);

        Microsoft::WRL::ComPtr<IMockDXGIAdapter> adapter;
        VERIFY_SUCCEEDED(DXGIMock.Factory->GetMockAdapter(hardwareLuid, &adapter));
        adapter->Remove();

        adapter.Reset();
        VERIFY_SUCCEEDED(DXGIMock.Factory->GetMockAdapter(warpLuid, &adapter));
        adapter->Remove();

        std::shared_ptr<CD3D11DeviceInstance> deviceInstance;

        VERIFY_SUCCEEDED(CD3D11DeviceInstance::GetInstance(deviceInstance, false));

        VERIFY_ARE_EQUAL(deviceInstance->EnsureResources(), DXGI_ERROR_DEVICE_REMOVED);
    }
}

void D3D11DeviceInstanceUnitTests::CheckForStaleD3DDevice()
{
    PlatformServicesMocksHelper platformMock;

    CreateDXGIFactoryDetour DXGIMock;
    CreateD3D11DeviceDetour D3DMock;
    CreateD2D1FactoryDetour D2DMock;

    // Create a bunch of adapters that we will use to populate the factory on various subtests
    Microsoft::WRL::ComPtr<IMockDXGIAdapter> warpAdapter;
    Microsoft::WRL::ComPtr<IMockDXGIAdapter> warpAdapter2;
    Microsoft::WRL::ComPtr<IMockDXGIAdapter> hardwareAdapter;
    Microsoft::WRL::ComPtr<IMockDXGIAdapter> hardwareAdapter2;
    Microsoft::WRL::ComPtr<IMockDXGIAdapter> badHardwareAdapter;

    CreateMockDXGIAdapter(MockDXGIAdapterType::WarpAdapterWithOutputs, &warpAdapter);
    CreateMockDXGIAdapter(MockDXGIAdapterType::WarpAdapterWithOutputs, &warpAdapter2);
    CreateMockDXGIAdapter(MockDXGIAdapterType::HardwareAdapter, &hardwareAdapter);
    CreateMockDXGIAdapter(MockDXGIAdapterType::HardwareAdapter, &hardwareAdapter2);
    CreateMockDXGIAdapter(MockDXGIAdapterType::HardwareAdapter, &badHardwareAdapter);
    badHardwareAdapter->Remove();

    LOG_OUTPUT(L"Warp adapter to same warp adapter");
    CheckForStaleDeviceSubTest(DXGIMock, warpAdapter.Get(), nullptr, warpAdapter.Get(), nullptr, false /* deviceShouldBeStale */);

    LOG_OUTPUT(L"Warp adapter to different warp adapter");
    CheckForStaleDeviceSubTest(DXGIMock, warpAdapter.Get(), nullptr, warpAdapter2.Get(), nullptr, true /* deviceShouldBeStale */);

    LOG_OUTPUT(L"Warp adapter to hardware adapter");
    CheckForStaleDeviceSubTest(DXGIMock, warpAdapter.Get(), nullptr, hardwareAdapter.Get(), warpAdapter.Get(), true /* deviceShouldBeStale */);

    LOG_OUTPUT(L"Warp adapter to bad hardware adapter");
    CheckForStaleDeviceSubTest(DXGIMock, warpAdapter.Get(), nullptr, badHardwareAdapter.Get(), nullptr, false /* deviceShouldBeStale */);

    LOG_OUTPUT(L"Hardware adapter to same hardware adapter");
    CheckForStaleDeviceSubTest(DXGIMock, hardwareAdapter.Get(), warpAdapter.Get(), hardwareAdapter.Get(), warpAdapter.Get(), false /* deviceShouldBeStale */);

    LOG_OUTPUT(L"Hardware adapter to different hardware adapter");
    CheckForStaleDeviceSubTest(DXGIMock, hardwareAdapter.Get(), warpAdapter.Get(), hardwareAdapter2.Get(), warpAdapter.Get(), true /* deviceShouldBeStale */);

    LOG_OUTPUT(L"Hardware adapter to bad hardware adapter");
    CheckForStaleDeviceSubTest(DXGIMock, hardwareAdapter.Get(), warpAdapter.Get(), badHardwareAdapter.Get(), warpAdapter.Get(), false /* deviceShouldBeStale */);

    // One more test to ensure that CheckForStaleDevice will return device lost if the device is not longer the
    // current one.
    // Create a device and ensure the d3d resources
    LOG_OUTPUT(L"Post Record Device Lost call");
    DXGIMock.InvalidateCurrentFactory();
    DXGIMock.Factory->AddAdapter(hardwareAdapter.Get());
    DXGIMock.Factory->AddAdapter(warpAdapter.Get());

    std::shared_ptr<CD3D11DeviceInstance> deviceInstance;
    VERIFY_SUCCEEDED(CD3D11DeviceInstance::GetInstance(deviceInstance, false));
    VERIFY_IS_NOT_NULL(deviceInstance.get());
    VERIFY_SUCCEEDED(deviceInstance->EnsureResources());

    deviceInstance->RecordDeviceAsLost();
    VERIFY_ARE_EQUAL(DXGI_ERROR_DEVICE_RESET, deviceInstance->CheckForStaleD3DDevice());
}

void D3D11DeviceInstanceUnitTests::CheckForStaleDeviceSubTest(_In_ CreateDXGIFactoryDetour& DXGIMock,
    _In_opt_ IMockDXGIAdapter* adapter1,
    _In_opt_ IMockDXGIAdapter* adapter2,
    _In_opt_ IMockDXGIAdapter* newAdapter1,
    _In_opt_ IMockDXGIAdapter* newAdapter2,
    bool deviceShouldBeStale)
{
    // Populate the factory with the original entries
    DXGIMock.InvalidateCurrentFactory();
    if (adapter1 != nullptr) DXGIMock.Factory->AddAdapter(adapter1);
    if (adapter2 != nullptr) DXGIMock.Factory->AddAdapter(adapter2);

    // Create a device and ensure the d3d resources
    std::shared_ptr<CD3D11DeviceInstance> deviceInstance;
    VERIFY_SUCCEEDED(CD3D11DeviceInstance::GetInstance(deviceInstance, false));
    VERIFY_IS_NOT_NULL(deviceInstance.get());
    VERIFY_SUCCEEDED(deviceInstance->EnsureResources());

    // Invalidate the current factory and populate with new adapters
    DXGIMock.InvalidateCurrentFactory();
    if (newAdapter1 != nullptr) DXGIMock.Factory->AddAdapter(newAdapter1);
    if (newAdapter2 != nullptr) DXGIMock.Factory->AddAdapter(newAdapter2);

    // Call EnsureDXGIAdapters to update the device's adapters and check for staleness
    if (!deviceShouldBeStale)
    {
        VERIFY_SUCCEEDED(deviceInstance->EnsureDXGIAdapters());
        VERIFY_ARE_EQUAL(CD3D11DeviceInstance::s_sharedDeviceWeak.lock().get(), deviceInstance.get());
    }
    else
    {
        VERIFY_ARE_EQUAL(DXGI_ERROR_DEVICE_REMOVED, deviceInstance->EnsureDXGIAdapters());
        VERIFY_IS_NULL(CD3D11DeviceInstance::s_sharedDeviceWeak.lock().get());
    }
}

void D3D11DeviceInstanceUnitTests::EnsureD2DResources()
{
    PlatformServicesMocksHelper platformMock;

    CreateDXGIFactoryDetour DXGIMock;
    CreateD3D11DeviceDetour D3DMock;
    CreateD2D1FactoryDetour D2DMock;

    DXGIMock.InvalidateCurrentFactory();
    LUID hardwareLuid = DXGIMock.Factory->AddAdapter(MockDXGIAdapterType::HardwareAdapter);
    LUID WarpLuid = DXGIMock.Factory->AddAdapter(MockDXGIAdapterType::WarpAdapterWithOutputs);

    Microsoft::WRL::ComPtr<IMockDXGIAdapter> adapter;
    VERIFY_SUCCEEDED(DXGIMock.Factory->GetMockAdapter(hardwareLuid, &adapter));
    adapter->SetFeatureLevel(D3D_FEATURE_LEVEL_9_1);
    adapter->SetPartnerCaps(1);

    std::shared_ptr<CD3D11DeviceInstance> deviceInstance;
    VERIFY_SUCCEEDED(CD3D11DeviceInstance::GetInstance(deviceInstance, false));
    VERIFY_IS_NOT_NULL(deviceInstance.get());

    VERIFY_SUCCEEDED(deviceInstance->EnsureResources());
    VERIFY_SUCCEEDED(deviceInstance->EnsureD2DResources());
}

void D3D11DeviceInstanceUnitTests::InitializeVideoDevice()
{
    // Create our Mocks
    PlatformServicesMocksHelper platformMock;
    CreateDXGIFactoryDetour DXGIMock;
    CreateD3D11DeviceDetour D3DMock;

    LOG_OUTPUT(L"Hardware");
    {
        DXGIMock.InvalidateCurrentFactory();
        LUID hardwareLuid = DXGIMock.Factory->AddAdapter(MockDXGIAdapterType::HardwareAdapter);
        LUID WarpLuid = DXGIMock.Factory->AddAdapter(MockDXGIAdapterType::WarpAdapterWithOutputs);

        std::shared_ptr<CD3D11DeviceInstance> deviceInstance;
        VERIFY_SUCCEEDED(CD3D11DeviceInstance::GetInstance(deviceInstance, false));
        VERIFY_IS_NOT_NULL(deviceInstance.get());

        VERIFY_SUCCEEDED(deviceInstance->InitializeVideoDevice());

        Microsoft::WRL::ComPtr<IMockDXGIDevice> mockDevice;
        CD3D11SharedDeviceGuard guard;
        VERIFY_SUCCEEDED(deviceInstance->TakeLockAndCheckDeviceLost(&guard));
        VERIFY_SUCCEEDED(deviceInstance->GetDevice(&guard)->QueryInterface(__uuidof(IMockDXGIDevice), &mockDevice));
        VERIFY_ARE_EQUAL(mockDevice->GetDriverType(), D3D_DRIVER_TYPE_HARDWARE);
        VERIFY_ARE_EQUAL(mockDevice->GetFlags(), D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_PREVENT_INTERNAL_THREADING_OPTIMIZATIONS | D3D11_CREATE_DEVICE_VIDEO_SUPPORT);

        Microsoft::WRL::ComPtr<ID3D10Multithread> multithread;
        VERIFY_SUCCEEDED(deviceInstance->GetDevice(&guard)->QueryInterface(__uuidof(ID3D10Multithread), &multithread));
        VERIFY_IS_TRUE(!!multithread->GetMultithreadProtected());
    }

    LOG_OUTPUT(L"Software");
    {
        DXGIMock.InvalidateCurrentFactory();
        LUID WarpLuid = DXGIMock.Factory->AddAdapter(MockDXGIAdapterType::WarpAdapterWithOutputs);

        std::shared_ptr<CD3D11DeviceInstance> deviceInstance;
        VERIFY_SUCCEEDED(CD3D11DeviceInstance::GetInstance(deviceInstance, false));
        VERIFY_IS_NOT_NULL(deviceInstance.get());

        VERIFY_SUCCEEDED(deviceInstance->InitializeVideoDevice());

        Microsoft::WRL::ComPtr<IMockDXGIDevice> mockDevice;
        CD3D11SharedDeviceGuard guard;
        VERIFY_SUCCEEDED(deviceInstance->TakeLockAndCheckDeviceLost(&guard));
        VERIFY_SUCCEEDED(deviceInstance->GetDevice(&guard)->QueryInterface(__uuidof(IMockDXGIDevice), &mockDevice));
        VERIFY_ARE_EQUAL(mockDevice->GetDriverType(), D3D_DRIVER_TYPE_HARDWARE);
        VERIFY_ARE_EQUAL(mockDevice->GetFlags(), D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_VIDEO_SUPPORT);

        Microsoft::WRL::ComPtr<ID3D10Multithread> multithread;
        VERIFY_SUCCEEDED(deviceInstance->GetDevice(&guard)->QueryInterface(__uuidof(ID3D10Multithread), &multithread));
        VERIFY_IS_TRUE(!!multithread->GetMultithreadProtected());
    }
}

LUID D3D11DeviceInstanceUnitTests::GetAdapterLuid(IDXGIAdapter* pAdapter)
{
    DXGI_ADAPTER_DESC description;
    pAdapter->GetDesc(&description);
    return description.AdapterLuid;
}
