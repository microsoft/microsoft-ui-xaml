// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <WexTestClass.h>
#include "dxgi1_2.h"
#include "d3d11_2.h"
#include "XamlLogging.h"
#include "CreateMockDetourBase.h"

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace Graphics {

    struct IMockDXGIFactory;
    struct IMockDXGIAdapter;
    struct IMockDXGIOutput;

    //  MockDXGIAdapterType - The different types of adapters that we could encounter

    enum class MockDXGIAdapterType
    {
        HardwareAdapter,
        WarpAdapterWithOutputs,
        WarpAdapterWithoutOutputs
    };

    // Helper function for comparing Adapter LUIDS
    bool AreAdapterLuidsEqual(LUID luid1, LUID luid2);


    // IMockDXGITimer - Interface for timer object used to simulate VBlanks
    struct __declspec(uuid("833DC952-B847-43E3-8448-A9DC3BAAEE52")) IMockDXGIVBlankTimer : IUnknown
    {
        virtual void WaitForVBlank() = 0;
    };

    //  DXGIDevice Mock - This also doubles as the D3D11 Device as well so we don't have to implement complex encapsulation.

    struct __declspec(uuid("55F316D1-D040-4AAA-B241-61EDDB20B8C0")) IMockDXGIDevice : public IDXGIDevice2
    {
        virtual D3D_DRIVER_TYPE GetDriverType() = 0;
        virtual UINT GetFlags() = 0;
        virtual void SetDeviceLostReason(HRESULT hr) = 0;
    };

    HRESULT CreateMockDXGIDevice(_In_ IDXGIAdapter* adapter, D3D_DRIVER_TYPE driverType, UINT flags, _Out_ IMockDXGIDevice ** device);

    // DXGIOutput Mock

    struct __declspec(uuid("02C9300F-8A23-43FD-BF6E-3D94CDD04C14")) IMockDXGIOutput : public IDXGIOutput
    {
        virtual LONG GetVBlankCount() = 0;
        virtual void SetVBlankTimer(IMockDXGIVBlankTimer* timer) = 0;
    };

    HRESULT CreateMockDXGIOutput(_In_ IDXGIObject* parent, _Out_ IMockDXGIOutput ** output);

    // DXGIAdapter Mock

    struct __declspec(uuid("DEA1104B-986F-44CD-BF0E-A1F72DCDDF45")) IMockDXGIAdapter : public IDXGIAdapter1
    {
        __declspec(property (get = GetOutput)) IMockDXGIOutput* Output;
        virtual IMockDXGIOutput* GetOutput() = 0;
        virtual void SetFeatureLevel(D3D_FEATURE_LEVEL featureLevel) = 0;
        virtual D3D_FEATURE_LEVEL GetFeatureLevel() = 0;
        virtual void SetPartnerCaps(UINT partnerCaps) = 0;
        virtual UINT GetPartnerCaps() = 0;
        virtual void Remove() = 0;
        virtual bool IsRemoved() = 0;
    };

    HRESULT CreateMockDXGIAdapter(MockDXGIAdapterType type, _Out_ IMockDXGIAdapter ** adapter);

    // DXGIFactory Mock

    struct __declspec(uuid("9D20F665-DFC2-4E60-86C5-79F83A97A8BF")) IMockDXGIFactory : public IDXGIFactory1
    {
        virtual void InvalidateFactory() = 0;
        virtual LUID AddAdapter(MockDXGIAdapterType type) = 0;
        virtual LUID AddAdapter(IMockDXGIAdapter * adapter) = 0;
        virtual HRESULT GetMockAdapter(LUID luid, IMockDXGIAdapter** ppAdapter) = 0;
    };

    HRESULT CreateMockDXGIFactory(_Out_ IMockDXGIFactory ** factory);

    //  CreateDXGIFactoryDetour - RAII object to create a detour for the CreateDXGIFactory1 function so we can return a mock.

    typedef HRESULT(WINAPI CREATEDXGIFACTORY1)(REFIID iid, void **);

    class CreateDXGIFactoryDetour : public CreateMockDetourBase<CREATEDXGIFACTORY1>
    {
    public:
        CreateDXGIFactoryDetour() : CreateMockDetourBase(L"dxgi.dll", "CreateDXGIFactory1")
        {
            SetDetour([](REFIID iid, void** dxgiFactory) -> HRESULT
            {
                VERIFY_IS_TRUE(!!m_factory->IsCurrent());
                return m_factory.Get()->QueryInterface(iid, dxgiFactory);
            });
            IFCFAILFAST(CreateMockDXGIFactory(&m_factory));
        }

        ~CreateDXGIFactoryDetour()
        {
            m_factory->InvalidateFactory();
            m_factory.Reset();
        }

        __declspec(property (get = GetFactory)) IMockDXGIFactory* Factory;
        IMockDXGIFactory* GetFactory()
        {
            return m_factory.Get();
        }

        void InvalidateCurrentFactory()
        {
            m_factory->InvalidateFactory();
            m_factory.Reset();
            IFCFAILFAST(CreateMockDXGIFactory(&m_factory));
        }

    private:
        static Microsoft::WRL::ComPtr<IMockDXGIFactory> m_factory;
    };

} } } } }
