// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "dxgi1_2.h"
#include "d3d11_2.h"
#include "MockDXGI.h"
#include "MockComObjectBase.h"

struct IGraphicsUnknown;

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace Graphics {

    // Static intializations

    Microsoft::WRL::ComPtr<IMockDXGIFactory> CreateDXGIFactoryDetour::m_factory;

    // Helper function for comparing Adapter LUIDS
    bool AreAdapterLuidsEqual(LUID luid1, LUID luid2)
    {
        return luid1.HighPart == luid2.HighPart && luid1.LowPart == luid2.LowPart;
    }

    //  MockDXGIBase - Base class for all mock DXGI objects.  It adds common DXGI methods to the MockComObjectBase
   
    template<class T>
    class MockDXGIBase : public MockComObjectBase<T>
    {
    public:
        MockDXGIBase(_In_ IDXGIObject* parent) : m_parent(parent)
        {
        }

        MockDXGIBase()
        {
        }

        virtual ~MockDXGIBase()
        {
        }

        virtual void* CastTo(REFIID iid)
        {
            if (iid == __uuidof(IDXGIObject)) return static_cast<IDXGIObject*>(this);
            return __super::CastTo(iid);
        }

        // IDXGIObject

        virtual HRESULT STDMETHODCALLTYPE GetParent(_In_ REFIID riid, _Out_ void  **ppParent)
        {
            if (m_parent) return m_parent.CopyTo(riid, ppParent);
            return DXGI_ERROR_NOT_FOUND;
        }

        virtual HRESULT STDMETHODCALLTYPE GetPrivateData(_In_ REFGUID Name, _Inout_ UINT *pDataSize, _Out_ void *pData)
        {
            UNREFERENCED_PARAMETER(Name);
            UNREFERENCED_PARAMETER(pDataSize);
            UNREFERENCED_PARAMETER(pData);
            return E_NOTIMPL;
        }

        virtual HRESULT STDMETHODCALLTYPE SetPrivateData(_In_ REFGUID Name, UINT DataSize, _In_ const void *pData)
        {
            UNREFERENCED_PARAMETER(Name);
            UNREFERENCED_PARAMETER(DataSize);
            UNREFERENCED_PARAMETER(pData);
            return E_NOTIMPL;
        }

        virtual HRESULT STDMETHODCALLTYPE SetPrivateDataInterface(_In_ REFGUID  Name, _In_ const IUnknown *pUnknown)
        {
            UNREFERENCED_PARAMETER(Name);
            UNREFERENCED_PARAMETER(pUnknown);
            return E_NOTIMPL;
        }

    private:
        Microsoft::WRL::ComPtr<IDXGIObject> m_parent;
    };

    // DXGIFactory Mock

    class MockDXGIFactory : public MockDXGIBase<IMockDXGIFactory>
    {
        typedef std::vector<Microsoft::WRL::ComPtr<IMockDXGIAdapter>> AdapterList;

    public:
        MockDXGIFactory() : m_isCurrent(TRUE)
        {
        }

        ~MockDXGIFactory()
        {
        }

        virtual void* CastTo(REFIID iid)
        {
            if (iid == __uuidof(IDXGIFactory)) return static_cast<IDXGIFactory*>(this);
            if (iid == __uuidof(IDXGIFactory1)) return static_cast<IDXGIFactory1*>(this);
            return __super::CastTo(iid);
        }

        // IMockDXGIFactory
        void InvalidateFactory()
        {
            m_isCurrent = false;
        }

        LUID AddAdapter(MockDXGIAdapterType type)
        {
            Microsoft::WRL::ComPtr<IMockDXGIAdapter> adapter;
            VERIFY_SUCCEEDED(CreateMockDXGIAdapter(type, &adapter));
            m_adapters.push_back(adapter);
            DXGI_ADAPTER_DESC description;
            adapter->GetDesc(&description);
            return description.AdapterLuid;
        }

        LUID AddAdapter(IMockDXGIAdapter * adapter)
        {
            m_adapters.push_back(adapter);
            DXGI_ADAPTER_DESC description;
            adapter->GetDesc(&description);
            return description.AdapterLuid;
        }

        HRESULT GetMockAdapter(LUID luid, IMockDXGIAdapter** ppAdapter)
        {
            for (AdapterList::iterator it = m_adapters.begin(); it != m_adapters.end(); ++it)
            {
                DXGI_ADAPTER_DESC description;
                VERIFY_SUCCEEDED((*it)->GetDesc(&description));
                if (AreAdapterLuidsEqual(description.AdapterLuid, luid))
                {
                    *ppAdapter = (*it).Get();
                    (*ppAdapter)->AddRef();
                    return S_OK;
                }
            }
            return DXGI_ERROR_NOT_FOUND;
        }

        // IDXGIFactory
        virtual HRESULT STDMETHODCALLTYPE CreateSoftwareAdapter(HMODULE Module, _Out_ IDXGIAdapter **ppAdapter)
        {
            return E_NOTIMPL;
        }

        virtual HRESULT STDMETHODCALLTYPE CreateSwapChain(
            _In_  IUnknown             *pDevice,
            _In_  DXGI_SWAP_CHAIN_DESC *pDesc,
            _Out_ IDXGISwapChain       **ppSwapChain
            )
        {
            UNREFERENCED_PARAMETER(pDesc);
            UNREFERENCED_PARAMETER(ppSwapChain);
            return E_NOTIMPL;
        }

        virtual HRESULT STDMETHODCALLTYPE EnumAdapters(UINT Adapter, _Out_ IDXGIAdapter **ppAdapter)
        {
            return EnumAdapters1(Adapter, (IDXGIAdapter1**)ppAdapter);
        }

        virtual HRESULT STDMETHODCALLTYPE GetWindowAssociation(_Out_ HWND *pWindowHandle)
        {
            UNREFERENCED_PARAMETER(pWindowHandle);
            return E_NOTIMPL;
        }

        virtual HRESULT STDMETHODCALLTYPE MakeWindowAssociation(HWND WindowHandle, UINT Flags)
        {
            UNREFERENCED_PARAMETER(WindowHandle);
            UNREFERENCED_PARAMETER(Flags);
            return E_NOTIMPL;
        }

        // IDXGIFactory1
        virtual HRESULT STDMETHODCALLTYPE EnumAdapters1(UINT Adapter, _Out_ IDXGIAdapter1 **ppAdapter)
        {
            if (!ppAdapter) return DXGI_ERROR_INVALID_CALL;
            if (Adapter >= m_adapters.size()) return DXGI_ERROR_NOT_FOUND;
            return m_adapters.at(Adapter).CopyTo(ppAdapter);
        }

        virtual BOOL STDMETHODCALLTYPE IsCurrent()
        {
            return m_isCurrent;
        }

    private:
        AdapterList m_adapters;
        BOOL m_isCurrent;
    };

    HRESULT CreateMockDXGIFactory(_Out_ IMockDXGIFactory ** factory)
    {
        *factory = new MockDXGIFactory();
        return S_OK;
    }

    // DXGIAdapter Mock

    class MockDXGIAdapter : public MockDXGIBase<IMockDXGIAdapter>
    {
    public:
        MockDXGIAdapter(MockDXGIAdapterType type) :
            m_output(nullptr),
            m_partnerCaps(0),
            m_featureLevel(D3D_FEATURE_LEVEL_10_1),
            m_isRemoved(false)
        {
            static LONG nextAdapterId = 0xADA00000;
            ZeroMemory(&m_description, sizeof(m_description));

            // Construct a LUID based upon the type and id 
            m_description.AdapterLuid.LowPart = (ULONG)type;
            m_description.AdapterLuid.HighPart = ++nextAdapterId;

            // If we want a warp adapter set the vendor/device info
            if (type == MockDXGIAdapterType::WarpAdapterWithOutputs || type == MockDXGIAdapterType::WarpAdapterWithoutOutputs)
            {
                m_description.VendorId = 0x1414;
                m_description.DeviceId = 0x8c;
            }

            // Everything gets an output except for warp without one.
            if (type != MockDXGIAdapterType::WarpAdapterWithoutOutputs)
            {
                VERIFY_SUCCEEDED(CreateMockDXGIOutput(this, &m_output));
            }
        }

        ~MockDXGIAdapter()
        {
        }

        virtual void* CastTo(REFIID iid)
        {
            if (iid == __uuidof(IDXGIAdapter)) return static_cast<IDXGIAdapter*>(this);
            if (iid == __uuidof(IDXGIAdapter1)) return static_cast<IDXGIAdapter1*>(this);
            return __super::CastTo(iid);
        }

        // IMockDXGIAdapter
        __declspec(property (get = GetOutput)) IMockDXGIOutput* Output;
        virtual IMockDXGIOutput* GetOutput()
        {
            return m_output.Get();
        }

        virtual void SetFeatureLevel(D3D_FEATURE_LEVEL featureLevel)
        {
            m_featureLevel = featureLevel;
        }

        virtual D3D_FEATURE_LEVEL GetFeatureLevel()
        {
            return m_featureLevel;
        }

        virtual void SetPartnerCaps(UINT partnerCaps)
        {
            m_partnerCaps = partnerCaps;
        }

        virtual UINT GetPartnerCaps()
        {
            return m_partnerCaps;
        }

        virtual void Remove()
        {
            m_isRemoved = true;
        }

        virtual bool IsRemoved()
        {
            return m_isRemoved;
        }

        // IDXGIAdapter
        virtual HRESULT STDMETHODCALLTYPE CheckInterfaceSupport(_In_ REFGUID InterfaceName, _Out_ LARGE_INTEGER *pUMDVersion)
        {
            UNREFERENCED_PARAMETER(InterfaceName); 
            UNREFERENCED_PARAMETER(pUMDVersion);
            return E_NOTIMPL;
        }

        virtual HRESULT STDMETHODCALLTYPE EnumOutputs(UINT Output, _Out_ IDXGIOutput **ppOutput)
        {
            if (!m_output || Output > 0) return DXGI_ERROR_NOT_FOUND;
            return m_output.CopyTo(ppOutput);
        }

        virtual HRESULT STDMETHODCALLTYPE GetDesc(_Out_ DXGI_ADAPTER_DESC *pDesc)
        {
            if (!pDesc) return E_POINTER;
            memcpy(pDesc, &m_description, sizeof(DXGI_ADAPTER_DESC));
            return S_OK;
        }

        //
        // IDXGIAdapter1
        //
        virtual HRESULT STDMETHODCALLTYPE GetDesc1(_Out_ DXGI_ADAPTER_DESC1 *pDesc)
        {
            if (!pDesc) return E_POINTER;
            memcpy(pDesc, &m_description, sizeof(DXGI_ADAPTER_DESC1));
            return S_OK;
        }

    protected:

    private:
        DXGI_ADAPTER_DESC1 m_description;
        Microsoft::WRL::ComPtr<IMockDXGIOutput> m_output;
        D3D_FEATURE_LEVEL m_featureLevel;
        UINT m_partnerCaps;
        bool m_isRemoved;

    };

    HRESULT CreateMockDXGIAdapter(MockDXGIAdapterType type, _Out_ IMockDXGIAdapter ** adapter)
    {
        *adapter = new MockDXGIAdapter(type);
        return S_OK;
    }

    // DXGIOutput Mock

    class MockDXGIOutput : public MockDXGIBase<IMockDXGIOutput>
    {
    public:
        MockDXGIOutput(IDXGIObject* parent) :
            m_vblankCount(0),
            MockDXGIBase<IMockDXGIOutput>(parent)
        {
        }

        virtual ~MockDXGIOutput()
        {
        }

        LONG GetVBlankCount()
        {
            return m_vblankCount;
        }

        virtual void SetVBlankTimer(IMockDXGIVBlankTimer* timer)
        {
            m_vBlankTimer = timer;
        }

        virtual void* CastTo(REFIID iid)
        {
            if (iid == __uuidof(IDXGIOutput)) return static_cast<IDXGIOutput*>(this);
            return __super::CastTo(iid);
        }

        // IDXGIOutput

        virtual HRESULT STDMETHODCALLTYPE FindClosestMatchingMode(_In_ const DXGI_MODE_DESC *pModeToMatch, _Out_ DXGI_MODE_DESC *pClosestMatch, _In_opt_ IUnknown *pConcernedDevice)
        {
            UNREFERENCED_PARAMETER(pModeToMatch);
            UNREFERENCED_PARAMETER(pClosestMatch);
            UNREFERENCED_PARAMETER(pConcernedDevice);
            return E_NOTIMPL;
        }

        virtual HRESULT STDMETHODCALLTYPE GetDesc(_Out_ DXGI_OUTPUT_DESC *pDesc)
        {
            UNREFERENCED_PARAMETER(pDesc);
            return E_NOTIMPL;
        }

        virtual HRESULT STDMETHODCALLTYPE GetDisplayModeList(DXGI_FORMAT EnumFormat, UINT Flags, _Inout_ UINT *pNumModes, _Out_opt_ DXGI_MODE_DESC *pDesc)
        {
            UNREFERENCED_PARAMETER(EnumFormat);
            UNREFERENCED_PARAMETER(Flags);
            UNREFERENCED_PARAMETER(pNumModes);
            UNREFERENCED_PARAMETER(pDesc);
            return E_NOTIMPL;
        }

        virtual HRESULT STDMETHODCALLTYPE GetDisplaySurfaceData(_In_ IDXGISurface *pDestination)
        {
            UNREFERENCED_PARAMETER(pDestination);
            return E_NOTIMPL;
        }

        virtual HRESULT STDMETHODCALLTYPE GetFrameStatistics(_Out_ DXGI_FRAME_STATISTICS *pStats)
        {
            UNREFERENCED_PARAMETER(pStats);
            return E_NOTIMPL;
        }

        virtual HRESULT STDMETHODCALLTYPE GetGammaControl(_Out_ DXGI_GAMMA_CONTROL *pArray)
        {
            UNREFERENCED_PARAMETER(pArray);
            return E_NOTIMPL;
        }

        virtual HRESULT STDMETHODCALLTYPE GetGammaControlCapabilities(_Out_ DXGI_GAMMA_CONTROL_CAPABILITIES *pGammaCaps)
        {
            UNREFERENCED_PARAMETER(pGammaCaps);
            return E_NOTIMPL;
        }

        virtual void STDMETHODCALLTYPE ReleaseOwnership() {}

        virtual HRESULT STDMETHODCALLTYPE SetDisplaySurface(_In_ IDXGISurface *pScanoutSurface)
        {
            UNREFERENCED_PARAMETER(pScanoutSurface);
            return E_NOTIMPL;
        }

        virtual HRESULT STDMETHODCALLTYPE SetGammaControl(_In_ const DXGI_GAMMA_CONTROL *pArray)
        {
            UNREFERENCED_PARAMETER(pArray);
            return E_NOTIMPL;
        }

        virtual HRESULT STDMETHODCALLTYPE TakeOwnership(_In_ IUnknown *pDevice, BOOL Exclusive)
        {
            UNREFERENCED_PARAMETER(Exclusive);
            return E_NOTIMPL;
        }

        virtual HRESULT STDMETHODCALLTYPE WaitForVBlank()
        {
            m_vblankCount++;
            if (m_vBlankTimer) m_vBlankTimer->WaitForVBlank();
            return S_OK;
        }

    private:
        LONG m_vblankCount;
        Microsoft::WRL::ComPtr<IMockDXGIVBlankTimer> m_vBlankTimer;
    };

    HRESULT CreateMockDXGIOutput(_In_ IDXGIObject * parent, _Out_ IMockDXGIOutput ** output)
    {
        *output = new MockDXGIOutput(parent);
        return S_OK;
    }

    // DXGIDevice Mock
    class MockDXGIDevice :
        public MockDXGIBase<IMockDXGIDevice>,
        public MockAlsoImplementsComInterface<ID3D10Multithread>,
        public MockAlsoImplementsComInterface<ID3D11Device>
    {
    public:
        MockDXGIDevice(IDXGIAdapter* adapter, D3D_DRIVER_TYPE driverType, UINT flags) :
            MockAlsoImplementsComInterface<ID3D10Multithread>((IMockDXGIDevice*)this),
            MockAlsoImplementsComInterface<ID3D11Device>((IMockDXGIDevice*)this),
            MockDXGIBase<IMockDXGIDevice>(adapter),
            m_driverType(driverType),
            m_flags(flags),
            m_multithreadProtected(FALSE),
            m_deviceLostReason(S_OK)
        {
        }

        virtual ~MockDXGIDevice()
        {
        }

        virtual void* CastTo(REFIID iid)
        {
            if (iid == __uuidof(IDXGIDevice)) return static_cast<IDXGIDevice*>(this);
            if (iid == __uuidof(IDXGIDevice1)) return static_cast<IDXGIDevice1*>(this);
            if (iid == __uuidof(IDXGIDevice2)) return static_cast<IDXGIDevice2*>(this);
            if (iid == __uuidof(ID3D11Device)) return static_cast<ID3D11Device*>(this);
            if (iid == __uuidof(ID3D10Multithread)) return static_cast<ID3D10Multithread*>(this);
            return __super::CastTo(iid);
        }
        // IMockDXGIDevice
        virtual D3D_DRIVER_TYPE GetDriverType()
        {
            return m_driverType;
        }

        virtual UINT GetFlags()
        {
            return m_flags;
        }

        virtual void SetDeviceLostReason(HRESULT hr)
        {
            m_deviceLostReason = hr;
        }

        // ID3D11Device
        virtual HRESULT STDMETHODCALLTYPE CreateBuffer(_In_ const D3D11_BUFFER_DESC* pDesc, _In_opt_ const D3D11_SUBRESOURCE_DATA* pInitialData, _Out_opt_ ID3D11Buffer** ppBuffer)
        {
            UNREFERENCED_PARAMETER(pDesc);
            UNREFERENCED_PARAMETER(pInitialData);
            UNREFERENCED_PARAMETER(ppBuffer);
            return E_NOTIMPL;
        }

        virtual HRESULT STDMETHODCALLTYPE CreateTexture1D(_In_ const D3D11_TEXTURE1D_DESC* pDesc, _In_reads_opt_(_Inexpressible_(pDesc->MipLevels * pDesc->ArraySize)) const D3D11_SUBRESOURCE_DATA* pInitialData, _Out_opt_ ID3D11Texture1D** ppTexture1D)
        {
            UNREFERENCED_PARAMETER(pDesc);
            UNREFERENCED_PARAMETER(pInitialData);
            UNREFERENCED_PARAMETER(ppTexture1D);
            return E_NOTIMPL;
        }

        virtual HRESULT STDMETHODCALLTYPE CreateTexture2D(_In_ const D3D11_TEXTURE2D_DESC* pDesc, _In_reads_opt_(_Inexpressible_(pDesc->MipLevels * pDesc->ArraySize)) const D3D11_SUBRESOURCE_DATA* pInitialData, _Out_opt_ ID3D11Texture2D** ppTexture2D)
        {
            UNREFERENCED_PARAMETER(pDesc);
            UNREFERENCED_PARAMETER(pInitialData);
            UNREFERENCED_PARAMETER(ppTexture2D);
            return E_NOTIMPL;
        }

        virtual HRESULT STDMETHODCALLTYPE CreateTexture3D(_In_ const D3D11_TEXTURE3D_DESC* pDesc, _In_reads_opt_(_Inexpressible_(pDesc->MipLevels)) const D3D11_SUBRESOURCE_DATA* pInitialData, _Out_opt_ ID3D11Texture3D** ppTexture3D)
        {
            UNREFERENCED_PARAMETER(pDesc);
            UNREFERENCED_PARAMETER(pInitialData);
            UNREFERENCED_PARAMETER(ppTexture3D);
            return E_NOTIMPL;
        }

        virtual HRESULT STDMETHODCALLTYPE CreateShaderResourceView(_In_ ID3D11Resource* pResource, _In_opt_ const D3D11_SHADER_RESOURCE_VIEW_DESC* pDesc, _Out_opt_ ID3D11ShaderResourceView** ppSRView)
        {
            UNREFERENCED_PARAMETER(pResource);
            UNREFERENCED_PARAMETER(pDesc);
            UNREFERENCED_PARAMETER(ppSRView);
            return E_NOTIMPL;
        }

        virtual HRESULT STDMETHODCALLTYPE CreateUnorderedAccessView(_In_ ID3D11Resource* pResource, _In_opt_ const D3D11_UNORDERED_ACCESS_VIEW_DESC* pDesc, _Out_opt_ ID3D11UnorderedAccessView** ppUAView)
        {
            UNREFERENCED_PARAMETER(pResource);
            UNREFERENCED_PARAMETER(pDesc);
            UNREFERENCED_PARAMETER(ppUAView);
            return E_NOTIMPL;
        }

        virtual HRESULT STDMETHODCALLTYPE CreateRenderTargetView(_In_ ID3D11Resource* pResource, _In_opt_ const D3D11_RENDER_TARGET_VIEW_DESC* pDesc, _Out_opt_ ID3D11RenderTargetView** ppRTView)
        {
            UNREFERENCED_PARAMETER(pResource);
            UNREFERENCED_PARAMETER(pDesc);
            UNREFERENCED_PARAMETER(ppRTView);
            return E_NOTIMPL;
        }

        virtual HRESULT STDMETHODCALLTYPE CreateDepthStencilView(_In_ ID3D11Resource* pResource, _In_opt_ const D3D11_DEPTH_STENCIL_VIEW_DESC* pDesc, _Out_opt_ ID3D11DepthStencilView** ppDepthStencilView)
        {
            UNREFERENCED_PARAMETER(pResource);
            UNREFERENCED_PARAMETER(pDesc);
            UNREFERENCED_PARAMETER(ppDepthStencilView);
            return E_NOTIMPL;
        }

        virtual HRESULT STDMETHODCALLTYPE CreateInputLayout(_In_reads_(NumElements) const D3D11_INPUT_ELEMENT_DESC* pInputElementDescs, _In_range_(0, D3D11_IA_VERTEX_INPUT_STRUCTURE_ELEMENT_COUNT) UINT NumElements, _In_reads_(BytecodeLength) const void* pShaderBytecodeWithInputSignature, _In_ SIZE_T BytecodeLength, _Out_opt_ ID3D11InputLayout** ppInputLayout)
        {
            UNREFERENCED_PARAMETER(pInputElementDescs);
            UNREFERENCED_PARAMETER(NumElements);
            UNREFERENCED_PARAMETER(pShaderBytecodeWithInputSignature);
            UNREFERENCED_PARAMETER(BytecodeLength);
            UNREFERENCED_PARAMETER(ppInputLayout);
            return E_NOTIMPL;
        }

        virtual HRESULT STDMETHODCALLTYPE CreateVertexShader(_In_reads_(BytecodeLength) const void* pShaderBytecode, _In_ SIZE_T BytecodeLength, _In_opt_ ID3D11ClassLinkage* pClassLinkage, _Out_opt_ ID3D11VertexShader** ppVertexShader)
        {
            UNREFERENCED_PARAMETER(pShaderBytecode);
            UNREFERENCED_PARAMETER(BytecodeLength);
            UNREFERENCED_PARAMETER(pClassLinkage);
            UNREFERENCED_PARAMETER(ppVertexShader);
            return E_NOTIMPL;
        }

        virtual HRESULT STDMETHODCALLTYPE CreateGeometryShader(_In_reads_(BytecodeLength) const void* pShaderBytecode, _In_ SIZE_T BytecodeLength, _In_opt_ ID3D11ClassLinkage* pClassLinkage, _Out_opt_ ID3D11GeometryShader** ppGeometryShader)
        {
            UNREFERENCED_PARAMETER(pShaderBytecode);
            UNREFERENCED_PARAMETER(BytecodeLength);
            UNREFERENCED_PARAMETER(pClassLinkage);
            UNREFERENCED_PARAMETER(ppGeometryShader);
            return E_NOTIMPL;
        }

        virtual HRESULT STDMETHODCALLTYPE CreateGeometryShaderWithStreamOutput(_In_reads_(BytecodeLength) const void* pShaderBytecode, _In_ SIZE_T BytecodeLength, _In_reads_opt_(NumEntries) const D3D11_SO_DECLARATION_ENTRY* pSODeclaration, _In_range_(0, D3D11_SO_STREAM_COUNT * D3D11_SO_OUTPUT_COMPONENT_COUNT) UINT NumEntries, _In_reads_opt_(NumStrides) const UINT* pBufferStrides, _In_range_(0, D3D11_SO_BUFFER_SLOT_COUNT) UINT NumStrides, _In_ UINT RasterizedStream, _In_opt_ ID3D11ClassLinkage* pClassLinkage, _Out_opt_ ID3D11GeometryShader** ppGeometryShader)
        {
            UNREFERENCED_PARAMETER(pShaderBytecode);
            UNREFERENCED_PARAMETER(BytecodeLength);
            UNREFERENCED_PARAMETER(pSODeclaration);
            UNREFERENCED_PARAMETER(NumEntries);
            UNREFERENCED_PARAMETER(pBufferStrides);
            UNREFERENCED_PARAMETER(NumStrides);
            UNREFERENCED_PARAMETER(RasterizedStream);
            UNREFERENCED_PARAMETER(pClassLinkage);
            UNREFERENCED_PARAMETER(ppGeometryShader);
            return E_NOTIMPL;
        }

        virtual HRESULT STDMETHODCALLTYPE CreatePixelShader(_In_reads_(BytecodeLength) const void* pShaderBytecode, _In_ SIZE_T BytecodeLength, _In_opt_ ID3D11ClassLinkage* pClassLinkage, _Out_opt_ ID3D11PixelShader** ppPixelShader)
        {
            UNREFERENCED_PARAMETER(pShaderBytecode);
            UNREFERENCED_PARAMETER(BytecodeLength);
            UNREFERENCED_PARAMETER(pClassLinkage);
            UNREFERENCED_PARAMETER(ppPixelShader);
            return E_NOTIMPL;
        }

        virtual HRESULT STDMETHODCALLTYPE CreateHullShader(_In_reads_(BytecodeLength) const void* pShaderBytecode, _In_ SIZE_T BytecodeLength, _In_opt_ ID3D11ClassLinkage* pClassLinkage, _Out_opt_ ID3D11HullShader** ppHullShader)
        {
            UNREFERENCED_PARAMETER(pShaderBytecode);
            UNREFERENCED_PARAMETER(BytecodeLength);
            UNREFERENCED_PARAMETER(pClassLinkage);
            UNREFERENCED_PARAMETER(ppHullShader);
            return E_NOTIMPL;
        }

        virtual HRESULT STDMETHODCALLTYPE CreateDomainShader(_In_reads_(BytecodeLength) const void* pShaderBytecode, _In_ SIZE_T BytecodeLength, _In_opt_ ID3D11ClassLinkage* pClassLinkage, _Out_opt_ ID3D11DomainShader** ppDomainShader)
        {
            UNREFERENCED_PARAMETER(pShaderBytecode);
            UNREFERENCED_PARAMETER(BytecodeLength);
            UNREFERENCED_PARAMETER(pClassLinkage);
            UNREFERENCED_PARAMETER(ppDomainShader);
            return E_NOTIMPL;
        }

        virtual HRESULT STDMETHODCALLTYPE CreateComputeShader(_In_reads_(BytecodeLength) const void* pShaderBytecode, _In_ SIZE_T BytecodeLength, _In_opt_ ID3D11ClassLinkage* pClassLinkage, _Out_opt_ ID3D11ComputeShader** ppComputeShader)
        {
            UNREFERENCED_PARAMETER(pShaderBytecode);
            UNREFERENCED_PARAMETER(BytecodeLength);
            UNREFERENCED_PARAMETER(pClassLinkage);
            UNREFERENCED_PARAMETER(ppComputeShader);
            return E_NOTIMPL;
        }

        virtual HRESULT STDMETHODCALLTYPE CreateClassLinkage(_Out_ ID3D11ClassLinkage** ppLinkage)
        {
            UNREFERENCED_PARAMETER(ppLinkage);
            return E_NOTIMPL;
        }

        virtual HRESULT STDMETHODCALLTYPE CreateBlendState(_In_ const D3D11_BLEND_DESC* pBlendStateDesc, _Out_opt_ ID3D11BlendState** ppBlendState)
        {
            UNREFERENCED_PARAMETER(pBlendStateDesc);
            UNREFERENCED_PARAMETER(ppBlendState);
            return E_NOTIMPL;
        }

        virtual HRESULT STDMETHODCALLTYPE CreateDepthStencilState(_In_ const D3D11_DEPTH_STENCIL_DESC* pDepthStencilDesc, _Out_opt_ ID3D11DepthStencilState** ppDepthStencilState)
        {
            UNREFERENCED_PARAMETER(pDepthStencilDesc);
            UNREFERENCED_PARAMETER(ppDepthStencilState);
            return E_NOTIMPL;
        }

        virtual HRESULT STDMETHODCALLTYPE CreateRasterizerState(_In_ const D3D11_RASTERIZER_DESC* pRasterizerDesc, _Out_opt_ ID3D11RasterizerState** ppRasterizerState)
        {
            UNREFERENCED_PARAMETER(pRasterizerDesc);
            UNREFERENCED_PARAMETER(ppRasterizerState);
            return E_NOTIMPL;
        }

        virtual HRESULT STDMETHODCALLTYPE CreateSamplerState(_In_ const D3D11_SAMPLER_DESC* pSamplerDesc, _Out_opt_ ID3D11SamplerState** ppSamplerState)
        {
            UNREFERENCED_PARAMETER(pSamplerDesc);
            UNREFERENCED_PARAMETER(ppSamplerState);
            return E_NOTIMPL;
        }

        virtual HRESULT STDMETHODCALLTYPE CreateQuery(_In_ const D3D11_QUERY_DESC* pQueryDesc, _Out_opt_ ID3D11Query** ppQuery)
        {
            UNREFERENCED_PARAMETER(pQueryDesc);
            UNREFERENCED_PARAMETER(ppQuery);
            return E_NOTIMPL;
        }

        virtual HRESULT STDMETHODCALLTYPE CreatePredicate(_In_ const D3D11_QUERY_DESC* pPredicateDesc, _Out_opt_ ID3D11Predicate** ppPredicate)
        {
            UNREFERENCED_PARAMETER(pPredicateDesc);
            UNREFERENCED_PARAMETER(ppPredicate);
            return E_NOTIMPL;
        }

        virtual HRESULT STDMETHODCALLTYPE CreateCounter(_In_ const D3D11_COUNTER_DESC* pCounterDesc, _Out_opt_ ID3D11Counter** ppCounter)
        {
            UNREFERENCED_PARAMETER(pCounterDesc);
            UNREFERENCED_PARAMETER(ppCounter);
            return E_NOTIMPL;
        }

        virtual HRESULT STDMETHODCALLTYPE CreateDeferredContext(UINT ContextFlags, _Out_ ID3D11DeviceContext** ppDeferredContext)
        {
            UNREFERENCED_PARAMETER(ContextFlags);
            UNREFERENCED_PARAMETER(ppDeferredContext);
            return E_NOTIMPL;
        }

        virtual HRESULT STDMETHODCALLTYPE OpenSharedResource(_In_ HANDLE hResource, _In_ REFIID ReturnedInterface, _Out_opt_ void** ppResource)
        {
            UNREFERENCED_PARAMETER(hResource);
            UNREFERENCED_PARAMETER(ReturnedInterface);
            UNREFERENCED_PARAMETER(ppResource);
            return E_NOTIMPL;
        }

        virtual HRESULT STDMETHODCALLTYPE CheckFormatSupport(_In_ DXGI_FORMAT Format, _Out_ UINT* pFormatSupport)
        {
            UNREFERENCED_PARAMETER(Format);
            UNREFERENCED_PARAMETER(pFormatSupport);
            return E_NOTIMPL;
        }

        virtual HRESULT STDMETHODCALLTYPE CheckMultisampleQualityLevels(_In_ DXGI_FORMAT Format, _In_ UINT SampleCount, _Out_ UINT* pNumQualityLevels)
        {
            UNREFERENCED_PARAMETER(Format);
            UNREFERENCED_PARAMETER(SampleCount);
            UNREFERENCED_PARAMETER(pNumQualityLevels);
            return E_NOTIMPL;
        }

        virtual void STDMETHODCALLTYPE CheckCounterInfo(_Out_ D3D11_COUNTER_INFO* pCounterInfo)
        {
            UNREFERENCED_PARAMETER(pCounterInfo);
            VERIFY_FAIL();
        }

        virtual HRESULT STDMETHODCALLTYPE CheckCounter(_In_ const D3D11_COUNTER_DESC* pDesc, _Out_ D3D11_COUNTER_TYPE* pType, _Out_ UINT* pActiveCounters, _Out_writes_opt_(*pNameLength) LPSTR szName, _Inout_opt_ UINT* pNameLength, _Out_writes_opt_(*pUnitsLength) LPSTR szUnits, _Inout_opt_ UINT* pUnitsLength, _Out_writes_opt_(*pDescriptionLength) LPSTR szDescription, _Inout_opt_ UINT* pDescriptionLength)
        {
            UNREFERENCED_PARAMETER(pDesc);
            UNREFERENCED_PARAMETER(pType);
            UNREFERENCED_PARAMETER(pActiveCounters);
            UNREFERENCED_PARAMETER(szName);
            UNREFERENCED_PARAMETER(pNameLength);
            UNREFERENCED_PARAMETER(szUnits);
            UNREFERENCED_PARAMETER(pUnitsLength);
            UNREFERENCED_PARAMETER(szDescription);
            UNREFERENCED_PARAMETER(pDescriptionLength);
            return E_NOTIMPL;
        }

        virtual HRESULT STDMETHODCALLTYPE CheckFeatureSupport(D3D11_FEATURE Feature, _Out_writes_bytes_(FeatureSupportDataSize) void* pFeatureSupportData, UINT FeatureSupportDataSize)
        {
            UNREFERENCED_PARAMETER(Feature);
            UNREFERENCED_PARAMETER(pFeatureSupportData);
            UNREFERENCED_PARAMETER(FeatureSupportDataSize);
            return E_NOTIMPL;
        }

        virtual HRESULT STDMETHODCALLTYPE GetPrivateData(_In_ REFGUID guid, _Inout_ UINT* pDataSize, _Out_writes_bytes_opt_(*pDataSize) void* pData)
        {
            UNREFERENCED_PARAMETER(guid);
            UNREFERENCED_PARAMETER(pDataSize);
            UNREFERENCED_PARAMETER(pData);
            return E_NOTIMPL;
        }

        virtual HRESULT STDMETHODCALLTYPE SetPrivateData(_In_ REFGUID guid, _In_ UINT DataSize, _In_reads_bytes_opt_(DataSize) const void* pData)
        {
            UNREFERENCED_PARAMETER(guid);
            UNREFERENCED_PARAMETER(DataSize);
            UNREFERENCED_PARAMETER(pData);
            // XAML sets some private data, but doesn't ever read it back and there are no expected
            // side effects, so we can just return S_OK without actually doing anything.
            return S_OK;
        }

        virtual HRESULT STDMETHODCALLTYPE SetPrivateDataInterface(_In_ REFGUID guid, _In_opt_ const IUnknown* pData)
        {
            UNREFERENCED_PARAMETER(guid);
            UNREFERENCED_PARAMETER(pData);
            return E_NOTIMPL;
        }

        virtual HRESULT STDMETHODCALLTYPE SetPrivateDataInterfaceGraphics(_In_ REFGUID guid, _In_opt_ const IGraphicsUnknown* pData)
        {
            UNREFERENCED_PARAMETER(guid);
            UNREFERENCED_PARAMETER(pData);
            return E_NOTIMPL;
        }

        virtual D3D_FEATURE_LEVEL STDMETHODCALLTYPE GetFeatureLevel()
        {
            VERIFY_FAIL();
            return (D3D_FEATURE_LEVEL)0;
        }

        virtual UINT STDMETHODCALLTYPE GetCreationFlags()
        {
            VERIFY_FAIL();
            return 0;
        }

        virtual HRESULT STDMETHODCALLTYPE GetDeviceRemovedReason()
        {
            return m_deviceLostReason;
        }

        virtual void STDMETHODCALLTYPE GetImmediateContext(_Out_ ID3D11DeviceContext** ppImmediateContext)
        {
            UNREFERENCED_PARAMETER(ppImmediateContext);
            VERIFY_FAIL();
        }

        virtual HRESULT STDMETHODCALLTYPE SetExceptionMode(UINT RaiseFlags)
        {
            UNREFERENCED_PARAMETER(RaiseFlags);
            return E_NOTIMPL;
        }

        virtual UINT STDMETHODCALLTYPE GetExceptionMode()
        {
            VERIFY_FAIL();
            return 0;
        }

        //
        //  IDXGIDevice
        //
        virtual HRESULT STDMETHODCALLTYPE GetAdapter(_COM_Outptr_ IDXGIAdapter **pAdapter)
        {
            return GetParent(__uuidof(IDXGIAdapter), (void**)pAdapter);
        }

        virtual HRESULT STDMETHODCALLTYPE CreateSurface(_In_ const DXGI_SURFACE_DESC *pDesc, UINT NumSurfaces, _In_ DXGI_USAGE Usage, _In_opt_ const DXGI_SHARED_RESOURCE *pSharedResource, _COM_Outptr_ IDXGISurface **ppSurface)
        {
            UNREFERENCED_PARAMETER(pDesc);
            UNREFERENCED_PARAMETER(NumSurfaces);
            UNREFERENCED_PARAMETER(Usage);
            UNREFERENCED_PARAMETER(pSharedResource);
            *ppSurface = nullptr;
            return E_NOTIMPL;
        }

        virtual HRESULT STDMETHODCALLTYPE QueryResourceResidency(_In_reads_(NumResources) IUnknown *const *ppResources, _Out_writes_(NumResources) DXGI_RESIDENCY *pResidencyStatus, UINT NumResources)
        {
            UNREFERENCED_PARAMETER(ppResources);
            UNREFERENCED_PARAMETER(pResidencyStatus);
            UNREFERENCED_PARAMETER(NumResources);
            return E_NOTIMPL;
        }

        virtual HRESULT STDMETHODCALLTYPE SetGPUThreadPriority(INT Priority)
        {
            UNREFERENCED_PARAMETER(Priority);
            return E_NOTIMPL;
        }

        virtual HRESULT STDMETHODCALLTYPE GetGPUThreadPriority(_Out_ INT *pPriority)
        {
            UNREFERENCED_PARAMETER(pPriority);
            return E_NOTIMPL;
        }

        //
        //  IDXGIDevice1
        //
        virtual HRESULT STDMETHODCALLTYPE SetMaximumFrameLatency(UINT MaxLatency)
        {
            UNREFERENCED_PARAMETER(MaxLatency);
            return E_NOTIMPL;
        }

        virtual HRESULT STDMETHODCALLTYPE GetMaximumFrameLatency(_Out_ UINT *pMaxLatency)
        {
            UNREFERENCED_PARAMETER(pMaxLatency);
            return E_NOTIMPL;
        }

        //
        //  IDXGIDevice2
        //
        virtual HRESULT STDMETHODCALLTYPE OfferResources(_In_ UINT NumResources, _In_reads_(NumResources) IDXGIResource *const *ppResources, _In_ DXGI_OFFER_RESOURCE_PRIORITY Priority)
        {
            UNREFERENCED_PARAMETER(NumResources);
            UNREFERENCED_PARAMETER(ppResources);
            UNREFERENCED_PARAMETER(Priority);
            return E_NOTIMPL;
        }

        virtual HRESULT STDMETHODCALLTYPE ReclaimResources(_In_ UINT NumResources, _In_reads_(NumResources) IDXGIResource *const *ppResources, _Out_writes_all_opt_(NumResources) BOOL *pDiscarded)
        {
            UNREFERENCED_PARAMETER(NumResources);
            UNREFERENCED_PARAMETER(ppResources);
            UNREFERENCED_PARAMETER(pDiscarded);
            return E_NOTIMPL;
        }

        virtual HRESULT STDMETHODCALLTYPE EnqueueSetEvent(_In_ HANDLE hEvent)
        {
            UNREFERENCED_PARAMETER(hEvent);
            return E_NOTIMPL;
        }

        //
        //  ID3D10MultiThread
        //
        virtual void STDMETHODCALLTYPE Enter(void)
        {
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE Leave(void)
        {
            VERIFY_FAIL();
        }

        virtual BOOL STDMETHODCALLTYPE SetMultithreadProtected(_In_ BOOL bMTProtect)
        {
            BOOL previous = m_multithreadProtected;
            m_multithreadProtected = bMTProtect;
            return previous;
        }

        virtual BOOL STDMETHODCALLTYPE GetMultithreadProtected(void)
        {
            return m_multithreadProtected;
        }

    private:
        D3D_DRIVER_TYPE m_driverType;
        UINT m_flags;
        BOOL m_multithreadProtected;
        HRESULT m_deviceLostReason;
    };

    HRESULT CreateMockDXGIDevice(_In_ IDXGIAdapter * adapter, D3D_DRIVER_TYPE driverType, UINT flags, _Out_ IMockDXGIDevice ** device)
    {
        Microsoft::WRL::ComPtr<IMockDXGIAdapter> mockAdapter;
        if (adapter)
        {
            IFC_RETURN(adapter->QueryInterface(__uuidof(IMockDXGIAdapter), &mockAdapter));
            if (mockAdapter->IsRemoved())
            {
                return DXGI_ERROR_DEVICE_REMOVED;
            }
        }
        *device = new MockDXGIDevice(adapter, driverType, flags);
        return S_OK;
    }

} } } } }
