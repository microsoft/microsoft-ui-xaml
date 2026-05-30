// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "MockD3D11.h"

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace Graphics {
 
    //  MockDXGIDeviceChildBase - Base class for all mock D33D11 objects that implement IDeviceChile.  
    //                            It adds common  methods to the MockComObjectBase
    template<class T>
    class MockDXGIDeviceChildBase : public MockComObjectBase<T>
    {
    public:
        MockDXGIDeviceChildBase(ID3D11Device* device) : m_device(device)
        {
        }

        ~MockDXGIDeviceChildBase()
        {
        }

        //  ID3D11DeviceChild
        virtual void STDMETHODCALLTYPE GetDevice(_Out_ ID3D11Device **ppDevice)
        {
            m_device.CopyTo(ppDevice);
        }

        virtual HRESULT STDMETHODCALLTYPE GetPrivateData(_In_ REFGUID guid, _Inout_ UINT *pDataSize, _Out_writes_bytes_opt_(*pDataSize) void *pData)
        {
            UNREFERENCED_PARAMETER(guid);
            UNREFERENCED_PARAMETER(pDataSize);
            UNREFERENCED_PARAMETER(pData);
            return E_NOTIMPL;
        }

        virtual HRESULT STDMETHODCALLTYPE SetPrivateData(_In_ REFGUID guid, _In_ UINT DataSize, _In_reads_bytes_opt_(DataSize) const void *pData)
        {
            UNREFERENCED_PARAMETER(guid);
            UNREFERENCED_PARAMETER(DataSize);
            UNREFERENCED_PARAMETER(pData);
            return E_NOTIMPL;
        }

        virtual HRESULT STDMETHODCALLTYPE SetPrivateDataInterface(_In_ REFGUID guid, _In_opt_ const IUnknown *pData)
        {
            UNREFERENCED_PARAMETER(guid);
            UNREFERENCED_PARAMETER(pData);
            return E_NOTIMPL;
        }

    private:
        Microsoft::WRL::ComPtr<ID3D11Device> m_device;
    };

    // MockD3D11DeviceContext
    class MockD3D11DeviceContext : public MockDXGIDeviceChildBase<IMockD3D11DeviceContext>
    {
    public:
        MockD3D11DeviceContext(ID3D11Device* device) : MockDXGIDeviceChildBase<IMockD3D11DeviceContext>(device)
        {
        }

        ~MockD3D11DeviceContext()
        {
        }

        virtual void* CastTo(REFIID iid)
        {
            if (iid == __uuidof(ID3D11DeviceContext)) return static_cast<ID3D11DeviceContext*>(this);
            return __super::CastTo(iid);
        }

        //
        //  ID3D11DeviceContext
        //
        virtual void STDMETHODCALLTYPE VSSetConstantBuffers(_In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1) UINT StartSlot, _In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot) UINT NumBuffers, _In_reads_opt_(NumBuffers) ID3D11Buffer *const *ppConstantBuffers)
        {
            UNREFERENCED_PARAMETER(StartSlot);
            UNREFERENCED_PARAMETER(NumBuffers);
            UNREFERENCED_PARAMETER(ppConstantBuffers);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE PSSetShaderResources(_In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1) UINT StartSlot, _In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot) UINT NumViews, _In_reads_opt_(NumViews) ID3D11ShaderResourceView *const *ppShaderResourceViews)
        {
            UNREFERENCED_PARAMETER(StartSlot);
            UNREFERENCED_PARAMETER(NumViews);
            UNREFERENCED_PARAMETER(ppShaderResourceViews);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE PSSetShader(_In_opt_ ID3D11PixelShader *pPixelShader, _In_reads_opt_(NumClassInstances) ID3D11ClassInstance *const *ppClassInstances, UINT NumClassInstances)
        {
            UNREFERENCED_PARAMETER(pPixelShader);
            UNREFERENCED_PARAMETER(ppClassInstances);
            UNREFERENCED_PARAMETER(NumClassInstances);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE PSSetSamplers(_In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1) UINT StartSlot, _In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot) UINT NumSamplers, _In_reads_opt_(NumSamplers) ID3D11SamplerState *const *ppSamplers)
        {
            UNREFERENCED_PARAMETER(StartSlot);
            UNREFERENCED_PARAMETER(NumSamplers);
            UNREFERENCED_PARAMETER(ppSamplers);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE VSSetShader(_In_opt_ ID3D11VertexShader *pVertexShader, _In_reads_opt_(NumClassInstances) ID3D11ClassInstance *const *ppClassInstances, UINT NumClassInstances)
        {
            UNREFERENCED_PARAMETER(pVertexShader);
            UNREFERENCED_PARAMETER(ppClassInstances);
            UNREFERENCED_PARAMETER(NumClassInstances);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE DrawIndexed(_In_ UINT IndexCount, _In_ UINT StartIndexLocation, _In_ INT BaseVertexLocation)
        {
            UNREFERENCED_PARAMETER(IndexCount);
            UNREFERENCED_PARAMETER(StartIndexLocation);
            UNREFERENCED_PARAMETER(BaseVertexLocation);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE Draw(_In_ UINT VertexCount, _In_ UINT StartVertexLocation)
        {
            UNREFERENCED_PARAMETER(VertexCount);
            UNREFERENCED_PARAMETER(StartVertexLocation);
            VERIFY_FAIL();
        }

        virtual HRESULT STDMETHODCALLTYPE Map(_In_ ID3D11Resource *pResource, _In_ UINT Subresource, _In_ D3D11_MAP MapType, _In_ UINT MapFlags, _Out_ D3D11_MAPPED_SUBRESOURCE *pMappedResource)
        {
            UNREFERENCED_PARAMETER(pResource);
            UNREFERENCED_PARAMETER(Subresource);
            UNREFERENCED_PARAMETER(MapType);
            UNREFERENCED_PARAMETER(MapFlags);
            UNREFERENCED_PARAMETER(pMappedResource);
            return E_NOTIMPL;
        }

        virtual void STDMETHODCALLTYPE Unmap(_In_ ID3D11Resource *pResource, _In_ UINT Subresource)
        {
            UNREFERENCED_PARAMETER(pResource);
            UNREFERENCED_PARAMETER(Subresource);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE PSSetConstantBuffers(_In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1) UINT StartSlot, _In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot) UINT NumBuffers, _In_reads_opt_(NumBuffers) ID3D11Buffer *const *ppConstantBuffers)
        {
            UNREFERENCED_PARAMETER(StartSlot);
            UNREFERENCED_PARAMETER(NumBuffers);
            UNREFERENCED_PARAMETER(ppConstantBuffers);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE IASetInputLayout(_In_opt_ ID3D11InputLayout *pInputLayout)
        {
            UNREFERENCED_PARAMETER(pInputLayout);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE IASetVertexBuffers(_In_range_(0, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT - 1) UINT StartSlot, _In_range_(0, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT - StartSlot) UINT NumBuffers, _In_reads_opt_(NumBuffers) ID3D11Buffer *const *ppVertexBuffers, _In_reads_opt_(NumBuffers) const UINT *pStrides, _In_reads_opt_(NumBuffers) const UINT *pOffsets)
        {
            UNREFERENCED_PARAMETER(StartSlot);
            UNREFERENCED_PARAMETER(NumBuffers);
            UNREFERENCED_PARAMETER(ppVertexBuffers);
            UNREFERENCED_PARAMETER(pStrides);
            UNREFERENCED_PARAMETER(pOffsets);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE IASetIndexBuffer(_In_opt_ ID3D11Buffer *pIndexBuffer, _In_ DXGI_FORMAT Format, _In_ UINT Offset)
        {
            UNREFERENCED_PARAMETER(pIndexBuffer);
            UNREFERENCED_PARAMETER(Format);
            UNREFERENCED_PARAMETER(Offset);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE DrawIndexedInstanced(_In_ UINT IndexCountPerInstance, _In_ UINT InstanceCount, _In_ UINT StartIndexLocation, _In_ INT BaseVertexLocation, _In_ UINT StartInstanceLocation)
        {
            UNREFERENCED_PARAMETER(IndexCountPerInstance);
            UNREFERENCED_PARAMETER(InstanceCount);
            UNREFERENCED_PARAMETER(StartIndexLocation);
            UNREFERENCED_PARAMETER(BaseVertexLocation);
            UNREFERENCED_PARAMETER(StartInstanceLocation);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE DrawInstanced(_In_ UINT VertexCountPerInstance, _In_ UINT InstanceCount, _In_ UINT StartVertexLocation, _In_ UINT StartInstanceLocation)
        {
            UNREFERENCED_PARAMETER(VertexCountPerInstance);
            UNREFERENCED_PARAMETER(InstanceCount);
            UNREFERENCED_PARAMETER(StartVertexLocation);
            UNREFERENCED_PARAMETER(StartInstanceLocation);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE GSSetConstantBuffers(_In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1) UINT StartSlot, _In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot) UINT NumBuffers, _In_reads_opt_(NumBuffers) ID3D11Buffer *const *ppConstantBuffers)
        {
            UNREFERENCED_PARAMETER(StartSlot);
            UNREFERENCED_PARAMETER(NumBuffers);
            UNREFERENCED_PARAMETER(ppConstantBuffers);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE GSSetShader(_In_opt_ ID3D11GeometryShader *pShader, _In_reads_opt_(NumClassInstances) ID3D11ClassInstance *const *ppClassInstances, UINT NumClassInstances)
        {
            UNREFERENCED_PARAMETER(pShader);
            UNREFERENCED_PARAMETER(ppClassInstances);
            UNREFERENCED_PARAMETER(NumClassInstances);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE IASetPrimitiveTopology(_In_ D3D11_PRIMITIVE_TOPOLOGY Topology)
        {
            UNREFERENCED_PARAMETER(Topology);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE VSSetShaderResources(_In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1) UINT StartSlot, _In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot) UINT NumViews, _In_reads_opt_(NumViews) ID3D11ShaderResourceView *const *ppShaderResourceViews)
        {
            UNREFERENCED_PARAMETER(StartSlot);
            UNREFERENCED_PARAMETER(NumViews);
            UNREFERENCED_PARAMETER(ppShaderResourceViews);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE VSSetSamplers(_In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1) UINT StartSlot, _In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot) UINT NumSamplers, _In_reads_opt_(NumSamplers) ID3D11SamplerState *const *ppSamplers)
        {
            UNREFERENCED_PARAMETER(StartSlot);
            UNREFERENCED_PARAMETER(NumSamplers);
            UNREFERENCED_PARAMETER(ppSamplers);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE Begin(_In_ ID3D11Asynchronous *pAsync)
        {
            UNREFERENCED_PARAMETER(pAsync);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE End(_In_ ID3D11Asynchronous *pAsync)
        {
            UNREFERENCED_PARAMETER(pAsync);
            VERIFY_FAIL();
        }

        virtual HRESULT STDMETHODCALLTYPE GetData(_In_ ID3D11Asynchronous *pAsync, _Out_writes_bytes_opt_(DataSize) void *pData, _In_ UINT DataSize, _In_ UINT GetDataFlags)
        {
            UNREFERENCED_PARAMETER(pAsync);
            UNREFERENCED_PARAMETER(pData);
            UNREFERENCED_PARAMETER(DataSize);
            UNREFERENCED_PARAMETER(GetDataFlags);
            return E_NOTIMPL;
        }

        virtual void STDMETHODCALLTYPE SetPredication(_In_opt_ ID3D11Predicate *pPredicate, _In_ BOOL PredicateValue)
        {
            UNREFERENCED_PARAMETER(pPredicate);
            UNREFERENCED_PARAMETER(PredicateValue);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE GSSetShaderResources(_In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1) UINT StartSlot, _In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot) UINT NumViews, _In_reads_opt_(NumViews) ID3D11ShaderResourceView *const *ppShaderResourceViews)
        {
            UNREFERENCED_PARAMETER(StartSlot);
            UNREFERENCED_PARAMETER(NumViews);
            UNREFERENCED_PARAMETER(ppShaderResourceViews);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE GSSetSamplers(_In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1) UINT StartSlot, _In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot) UINT NumSamplers, _In_reads_opt_(NumSamplers) ID3D11SamplerState *const *ppSamplers)
        {
            UNREFERENCED_PARAMETER(StartSlot);
            UNREFERENCED_PARAMETER(NumSamplers);
            UNREFERENCED_PARAMETER(ppSamplers);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE OMSetRenderTargets(_In_range_(0, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT) UINT NumViews, _In_reads_opt_(NumViews) ID3D11RenderTargetView *const *ppRenderTargetViews, _In_opt_ ID3D11DepthStencilView *pDepthStencilView)
        {
            UNREFERENCED_PARAMETER(NumViews);
            UNREFERENCED_PARAMETER(ppRenderTargetViews);
            UNREFERENCED_PARAMETER(pDepthStencilView);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE OMSetRenderTargetsAndUnorderedAccessViews(_In_ UINT NumRTVs, _In_reads_opt_(NumRTVs) ID3D11RenderTargetView *const *ppRenderTargetViews, _In_opt_ ID3D11DepthStencilView *pDepthStencilView, _In_range_(0, D3D11_1_UAV_SLOT_COUNT - 1) UINT UAVStartSlot, _In_ UINT NumUAVs, _In_reads_opt_(NumUAVs) ID3D11UnorderedAccessView *const *ppUnorderedAccessViews, _In_reads_opt_(NumUAVs) const UINT *pUAVInitialCounts)
        {
            UNREFERENCED_PARAMETER(NumRTVs);
            UNREFERENCED_PARAMETER(ppRenderTargetViews);
            UNREFERENCED_PARAMETER(pDepthStencilView);
            UNREFERENCED_PARAMETER(UAVStartSlot);
            UNREFERENCED_PARAMETER(NumUAVs);
            UNREFERENCED_PARAMETER(ppUnorderedAccessViews);
            UNREFERENCED_PARAMETER(pUAVInitialCounts);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE OMSetBlendState(_In_opt_ ID3D11BlendState *pBlendState, _In_opt_ const FLOAT BlendFactor[4], _In_ UINT SampleMask)
        {
            UNREFERENCED_PARAMETER(pBlendState);
            UNREFERENCED_PARAMETER(BlendFactor);
            UNREFERENCED_PARAMETER(SampleMask);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE OMSetDepthStencilState(_In_opt_ ID3D11DepthStencilState *pDepthStencilState, _In_ UINT StencilRef)
        {
            UNREFERENCED_PARAMETER(pDepthStencilState);
            UNREFERENCED_PARAMETER(StencilRef);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE SOSetTargets(_In_range_(0, D3D11_SO_BUFFER_SLOT_COUNT) UINT NumBuffers, _In_reads_opt_(NumBuffers) ID3D11Buffer *const *ppSOTargets, _In_reads_opt_(NumBuffers) const UINT *pOffsets)
        {
            UNREFERENCED_PARAMETER(NumBuffers);
            UNREFERENCED_PARAMETER(ppSOTargets);
            UNREFERENCED_PARAMETER(pOffsets);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE DrawAuto(void)
        {
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE DrawIndexedInstancedIndirect(_In_ ID3D11Buffer *pBufferForArgs, _In_ UINT AlignedByteOffsetForArgs)
        {
            UNREFERENCED_PARAMETER(pBufferForArgs);
            UNREFERENCED_PARAMETER(AlignedByteOffsetForArgs);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE DrawInstancedIndirect(_In_ ID3D11Buffer *pBufferForArgs, _In_ UINT AlignedByteOffsetForArgs)
        {
            UNREFERENCED_PARAMETER(pBufferForArgs);
            UNREFERENCED_PARAMETER(AlignedByteOffsetForArgs);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE Dispatch(_In_ UINT ThreadGroupCountX, _In_ UINT ThreadGroupCountY, _In_ UINT ThreadGroupCountZ)
        {
            UNREFERENCED_PARAMETER(ThreadGroupCountX);
            UNREFERENCED_PARAMETER(ThreadGroupCountY);
            UNREFERENCED_PARAMETER(ThreadGroupCountZ);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE DispatchIndirect(_In_ ID3D11Buffer *pBufferForArgs, _In_ UINT AlignedByteOffsetForArgs)
        {
            UNREFERENCED_PARAMETER(pBufferForArgs);
            UNREFERENCED_PARAMETER(AlignedByteOffsetForArgs);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE RSSetState(_In_opt_ ID3D11RasterizerState *pRasterizerState)
        {
            UNREFERENCED_PARAMETER(pRasterizerState);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE RSSetViewports(_In_range_(0, D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE) UINT NumViewports, _In_reads_opt_(NumViewports) const D3D11_VIEWPORT *pViewports)
        {
            UNREFERENCED_PARAMETER(NumViewports);
            UNREFERENCED_PARAMETER(pViewports);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE RSSetScissorRects(_In_range_(0, D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE) UINT NumRects, _In_reads_opt_(NumRects) const D3D11_RECT *pRects)
        {
            UNREFERENCED_PARAMETER(NumRects);
            UNREFERENCED_PARAMETER(pRects);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE CopySubresourceRegion(_In_ ID3D11Resource *pDstResource, _In_ UINT DstSubresource, _In_ UINT DstX, _In_ UINT DstY, _In_ UINT DstZ, _In_ ID3D11Resource *pSrcResource, _In_ UINT SrcSubresource, _In_opt_ const D3D11_BOX *pSrcBox)
        {
            UNREFERENCED_PARAMETER(pDstResource);
            UNREFERENCED_PARAMETER(DstSubresource);
            UNREFERENCED_PARAMETER(DstX);
            UNREFERENCED_PARAMETER(DstY);
            UNREFERENCED_PARAMETER(DstZ);
            UNREFERENCED_PARAMETER(pSrcResource);
            UNREFERENCED_PARAMETER(SrcSubresource);
            UNREFERENCED_PARAMETER(pSrcBox);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE CopyResource(_In_ ID3D11Resource *pDstResource, _In_ ID3D11Resource *pSrcResource)
        {
            UNREFERENCED_PARAMETER(pDstResource);
            UNREFERENCED_PARAMETER(pSrcResource);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE UpdateSubresource(_In_ ID3D11Resource *pDstResource, _In_ UINT DstSubresource, _In_opt_ const D3D11_BOX *pDstBox, _In_ const void *pSrcData, _In_ UINT SrcRowPitch, _In_ UINT SrcDepthPitch)
        {
            UNREFERENCED_PARAMETER(pDstResource);
            UNREFERENCED_PARAMETER(DstSubresource);
            UNREFERENCED_PARAMETER(pDstBox);
            UNREFERENCED_PARAMETER(pSrcData);
            UNREFERENCED_PARAMETER(SrcRowPitch);
            UNREFERENCED_PARAMETER(SrcDepthPitch);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE CopyStructureCount(_In_ ID3D11Buffer *pDstBuffer, _In_ UINT DstAlignedByteOffset, _In_ ID3D11UnorderedAccessView *pSrcView)
        {
            UNREFERENCED_PARAMETER(pDstBuffer);
            UNREFERENCED_PARAMETER(DstAlignedByteOffset);
            UNREFERENCED_PARAMETER(pSrcView);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE ClearRenderTargetView(_In_ ID3D11RenderTargetView *pRenderTargetView, _In_ const FLOAT ColorRGBA[4])
        {
            UNREFERENCED_PARAMETER(pRenderTargetView);
            UNREFERENCED_PARAMETER(ColorRGBA);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE ClearUnorderedAccessViewUint(_In_ ID3D11UnorderedAccessView *pUnorderedAccessView, _In_ const UINT Values[4])
        {
            UNREFERENCED_PARAMETER(pUnorderedAccessView);
            UNREFERENCED_PARAMETER(Values);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE ClearUnorderedAccessViewFloat(_In_ ID3D11UnorderedAccessView *pUnorderedAccessView, _In_ const FLOAT Values[4])
        {
            UNREFERENCED_PARAMETER(pUnorderedAccessView);
            UNREFERENCED_PARAMETER(Values);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE ClearDepthStencilView(_In_ ID3D11DepthStencilView *pDepthStencilView, _In_ UINT ClearFlags, _In_ FLOAT Depth, _In_ UINT8 Stencil)
        {
            UNREFERENCED_PARAMETER(pDepthStencilView);
            UNREFERENCED_PARAMETER(ClearFlags);
            UNREFERENCED_PARAMETER(Depth);
            UNREFERENCED_PARAMETER(Stencil);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE GenerateMips(_In_ ID3D11ShaderResourceView *pShaderResourceView)
        {
            UNREFERENCED_PARAMETER(pShaderResourceView);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE SetResourceMinLOD(_In_ ID3D11Resource *pResource, FLOAT MinLOD)
        {
            UNREFERENCED_PARAMETER(pResource);
            UNREFERENCED_PARAMETER(MinLOD);
            VERIFY_FAIL();
        }

        virtual FLOAT STDMETHODCALLTYPE GetResourceMinLOD(_In_ ID3D11Resource *pResource)
        {
            UNREFERENCED_PARAMETER(pResource);
            VERIFY_FAIL();
            return (FLOAT)0;
        }

        virtual void STDMETHODCALLTYPE ResolveSubresource(_In_ ID3D11Resource *pDstResource, _In_ UINT DstSubresource, _In_ ID3D11Resource *pSrcResource, _In_ UINT SrcSubresource, _In_ DXGI_FORMAT Format)
        {
            UNREFERENCED_PARAMETER(pDstResource);
            UNREFERENCED_PARAMETER(DstSubresource);
            UNREFERENCED_PARAMETER(pSrcResource);
            UNREFERENCED_PARAMETER(SrcSubresource);
            UNREFERENCED_PARAMETER(Format);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE ExecuteCommandList(_In_ ID3D11CommandList *pCommandList, BOOL RestoreContextState)
        {
            UNREFERENCED_PARAMETER(pCommandList);
            UNREFERENCED_PARAMETER(RestoreContextState);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE HSSetShaderResources(_In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1) UINT StartSlot, _In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot) UINT NumViews, _In_reads_opt_(NumViews) ID3D11ShaderResourceView *const *ppShaderResourceViews)
        {
            UNREFERENCED_PARAMETER(StartSlot);
            UNREFERENCED_PARAMETER(NumViews);
            UNREFERENCED_PARAMETER(ppShaderResourceViews);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE HSSetShader(_In_opt_ ID3D11HullShader *pHullShader, _In_reads_opt_(NumClassInstances) ID3D11ClassInstance *const *ppClassInstances, UINT NumClassInstances)
        {
            UNREFERENCED_PARAMETER(pHullShader);
            UNREFERENCED_PARAMETER(ppClassInstances);
            UNREFERENCED_PARAMETER(NumClassInstances);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE HSSetSamplers(_In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1) UINT StartSlot, _In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot) UINT NumSamplers, _In_reads_opt_(NumSamplers) ID3D11SamplerState *const *ppSamplers)
        {
            UNREFERENCED_PARAMETER(StartSlot);
            UNREFERENCED_PARAMETER(NumSamplers);
            UNREFERENCED_PARAMETER(ppSamplers);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE HSSetConstantBuffers(_In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1) UINT StartSlot, _In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot) UINT NumBuffers, _In_reads_opt_(NumBuffers) ID3D11Buffer *const *ppConstantBuffers)
        {
            UNREFERENCED_PARAMETER(StartSlot);
            UNREFERENCED_PARAMETER(NumBuffers);
            UNREFERENCED_PARAMETER(ppConstantBuffers);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE DSSetShaderResources(_In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1) UINT StartSlot, _In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot) UINT NumViews, _In_reads_opt_(NumViews) ID3D11ShaderResourceView *const *ppShaderResourceViews)
        {
            UNREFERENCED_PARAMETER(StartSlot);
            UNREFERENCED_PARAMETER(NumViews);
            UNREFERENCED_PARAMETER(ppShaderResourceViews);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE DSSetShader(_In_opt_ ID3D11DomainShader *pDomainShader, _In_reads_opt_(NumClassInstances) ID3D11ClassInstance *const *ppClassInstances, UINT NumClassInstances)
        {
            UNREFERENCED_PARAMETER(pDomainShader);
            UNREFERENCED_PARAMETER(ppClassInstances);
            UNREFERENCED_PARAMETER(NumClassInstances);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE DSSetSamplers(_In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1) UINT StartSlot, _In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot) UINT NumSamplers, _In_reads_opt_(NumSamplers) ID3D11SamplerState *const *ppSamplers)
        {
            UNREFERENCED_PARAMETER(StartSlot);
            UNREFERENCED_PARAMETER(NumSamplers);
            UNREFERENCED_PARAMETER(ppSamplers);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE DSSetConstantBuffers(_In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1) UINT StartSlot, _In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot) UINT NumBuffers, _In_reads_opt_(NumBuffers) ID3D11Buffer *const *ppConstantBuffers)
        {
            UNREFERENCED_PARAMETER(StartSlot);
            UNREFERENCED_PARAMETER(NumBuffers);
            UNREFERENCED_PARAMETER(ppConstantBuffers);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE CSSetShaderResources(_In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1) UINT StartSlot, _In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot) UINT NumViews, _In_reads_opt_(NumViews) ID3D11ShaderResourceView *const *ppShaderResourceViews)
        {
            UNREFERENCED_PARAMETER(StartSlot);
            UNREFERENCED_PARAMETER(NumViews);
            UNREFERENCED_PARAMETER(ppShaderResourceViews);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE CSSetUnorderedAccessViews(_In_range_(0, D3D11_1_UAV_SLOT_COUNT - 1) UINT StartSlot, _In_range_(0, D3D11_1_UAV_SLOT_COUNT - StartSlot) UINT NumUAVs, _In_reads_opt_(NumUAVs) ID3D11UnorderedAccessView *const *ppUnorderedAccessViews, _In_reads_opt_(NumUAVs) const UINT *pUAVInitialCounts)
        {
            UNREFERENCED_PARAMETER(StartSlot);
            UNREFERENCED_PARAMETER(NumUAVs);
            UNREFERENCED_PARAMETER(ppUnorderedAccessViews);
            UNREFERENCED_PARAMETER(pUAVInitialCounts);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE CSSetShader(_In_opt_ ID3D11ComputeShader *pComputeShader, _In_reads_opt_(NumClassInstances) ID3D11ClassInstance *const *ppClassInstances, UINT NumClassInstances)
        {
            UNREFERENCED_PARAMETER(pComputeShader);
            UNREFERENCED_PARAMETER(ppClassInstances);
            UNREFERENCED_PARAMETER(NumClassInstances);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE CSSetSamplers(_In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1) UINT StartSlot, _In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot) UINT NumSamplers, _In_reads_opt_(NumSamplers) ID3D11SamplerState *const *ppSamplers)
        {
            UNREFERENCED_PARAMETER(StartSlot);
            UNREFERENCED_PARAMETER(NumSamplers);
            UNREFERENCED_PARAMETER(ppSamplers);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE CSSetConstantBuffers(_In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1) UINT StartSlot, _In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot) UINT NumBuffers, _In_reads_opt_(NumBuffers) ID3D11Buffer *const *ppConstantBuffers)
        {
            UNREFERENCED_PARAMETER(StartSlot);
            UNREFERENCED_PARAMETER(NumBuffers);
            UNREFERENCED_PARAMETER(ppConstantBuffers);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE VSGetConstantBuffers(_In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1) UINT StartSlot, _In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot) UINT NumBuffers, _Out_writes_opt_(NumBuffers) ID3D11Buffer **ppConstantBuffers)
        {
            UNREFERENCED_PARAMETER(StartSlot);
            UNREFERENCED_PARAMETER(NumBuffers);
            UNREFERENCED_PARAMETER(ppConstantBuffers);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE PSGetShaderResources(_In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1) UINT StartSlot, _In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot) UINT NumViews, _Out_writes_opt_(NumViews) ID3D11ShaderResourceView **ppShaderResourceViews)
        {
            UNREFERENCED_PARAMETER(StartSlot);
            UNREFERENCED_PARAMETER(NumViews);
            UNREFERENCED_PARAMETER(ppShaderResourceViews);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE PSGetShader(_Out_ ID3D11PixelShader **ppPixelShader, _Out_writes_opt_(*pNumClassInstances) ID3D11ClassInstance **ppClassInstances, _Inout_opt_ UINT *pNumClassInstances)
        {
            UNREFERENCED_PARAMETER(ppPixelShader);
            UNREFERENCED_PARAMETER(ppClassInstances);
            UNREFERENCED_PARAMETER(pNumClassInstances);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE PSGetSamplers(_In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1) UINT StartSlot, _In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot) UINT NumSamplers, _Out_writes_opt_(NumSamplers) ID3D11SamplerState **ppSamplers)
        {
            UNREFERENCED_PARAMETER(StartSlot);
            UNREFERENCED_PARAMETER(NumSamplers);
            UNREFERENCED_PARAMETER(ppSamplers);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE VSGetShader(_Out_ ID3D11VertexShader **ppVertexShader, _Out_writes_opt_(*pNumClassInstances) ID3D11ClassInstance **ppClassInstances, _Inout_opt_ UINT *pNumClassInstances)
        {
            UNREFERENCED_PARAMETER(ppVertexShader);
            UNREFERENCED_PARAMETER(ppClassInstances);
            UNREFERENCED_PARAMETER(pNumClassInstances);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE PSGetConstantBuffers(_In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1) UINT StartSlot, _In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot) UINT NumBuffers, _Out_writes_opt_(NumBuffers) ID3D11Buffer **ppConstantBuffers)
        {
            UNREFERENCED_PARAMETER(StartSlot);
            UNREFERENCED_PARAMETER(NumBuffers);
            UNREFERENCED_PARAMETER(ppConstantBuffers);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE IAGetInputLayout(_Out_ ID3D11InputLayout **ppInputLayout)
        {
            UNREFERENCED_PARAMETER(ppInputLayout);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE IAGetVertexBuffers(_In_range_(0, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT - 1) UINT StartSlot, _In_range_(0, D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT - StartSlot) UINT NumBuffers, _Out_writes_opt_(NumBuffers) ID3D11Buffer **ppVertexBuffers, _Out_writes_opt_(NumBuffers) UINT *pStrides, _Out_writes_opt_(NumBuffers) UINT *pOffsets)
        {
            UNREFERENCED_PARAMETER(StartSlot);
            UNREFERENCED_PARAMETER(NumBuffers);
            UNREFERENCED_PARAMETER(ppVertexBuffers);
            UNREFERENCED_PARAMETER(pStrides);
            UNREFERENCED_PARAMETER(pOffsets);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE IAGetIndexBuffer(_Out_opt_ ID3D11Buffer **pIndexBuffer, _Out_opt_ DXGI_FORMAT *Format, _Out_opt_ UINT *Offset)
        {
            UNREFERENCED_PARAMETER(pIndexBuffer);
            UNREFERENCED_PARAMETER(Format);
            UNREFERENCED_PARAMETER(Offset);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE GSGetConstantBuffers(_In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1) UINT StartSlot, _In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot) UINT NumBuffers, _Out_writes_opt_(NumBuffers) ID3D11Buffer **ppConstantBuffers)
        {
            UNREFERENCED_PARAMETER(StartSlot);
            UNREFERENCED_PARAMETER(NumBuffers);
            UNREFERENCED_PARAMETER(ppConstantBuffers);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE GSGetShader(_Out_ ID3D11GeometryShader **ppGeometryShader, _Out_writes_opt_(*pNumClassInstances) ID3D11ClassInstance **ppClassInstances, _Inout_opt_ UINT *pNumClassInstances)
        {
            UNREFERENCED_PARAMETER(ppGeometryShader);
            UNREFERENCED_PARAMETER(ppClassInstances);
            UNREFERENCED_PARAMETER(pNumClassInstances);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE IAGetPrimitiveTopology(_Out_ D3D11_PRIMITIVE_TOPOLOGY *pTopology)
        {
            UNREFERENCED_PARAMETER(pTopology);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE VSGetShaderResources(_In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1) UINT StartSlot, _In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot) UINT NumViews, _Out_writes_opt_(NumViews) ID3D11ShaderResourceView **ppShaderResourceViews)
        {
            UNREFERENCED_PARAMETER(StartSlot);
            UNREFERENCED_PARAMETER(NumViews);
            UNREFERENCED_PARAMETER(ppShaderResourceViews);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE VSGetSamplers(_In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1) UINT StartSlot, _In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot) UINT NumSamplers, _Out_writes_opt_(NumSamplers) ID3D11SamplerState **ppSamplers)
        {
            UNREFERENCED_PARAMETER(StartSlot);
            UNREFERENCED_PARAMETER(NumSamplers);
            UNREFERENCED_PARAMETER(ppSamplers);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE GetPredication(_Out_opt_ ID3D11Predicate **ppPredicate, _Out_opt_ BOOL *pPredicateValue)
        {
            UNREFERENCED_PARAMETER(ppPredicate);
            UNREFERENCED_PARAMETER(pPredicateValue);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE GSGetShaderResources(_In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1) UINT StartSlot, _In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot) UINT NumViews, _Out_writes_opt_(NumViews) ID3D11ShaderResourceView **ppShaderResourceViews)
        {
            UNREFERENCED_PARAMETER(StartSlot);
            UNREFERENCED_PARAMETER(NumViews);
            UNREFERENCED_PARAMETER(ppShaderResourceViews);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE GSGetSamplers(_In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1) UINT StartSlot, _In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot) UINT NumSamplers, _Out_writes_opt_(NumSamplers) ID3D11SamplerState **ppSamplers)
        {
            UNREFERENCED_PARAMETER(StartSlot);
            UNREFERENCED_PARAMETER(NumSamplers);
            UNREFERENCED_PARAMETER(ppSamplers);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE OMGetRenderTargets(_In_range_(0, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT) UINT NumViews, _Out_writes_opt_(NumViews) ID3D11RenderTargetView **ppRenderTargetViews, _Out_opt_ ID3D11DepthStencilView **ppDepthStencilView)
        {
            UNREFERENCED_PARAMETER(NumViews);
            UNREFERENCED_PARAMETER(ppRenderTargetViews);
            UNREFERENCED_PARAMETER(ppDepthStencilView);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE OMGetRenderTargetsAndUnorderedAccessViews(_In_range_(0, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT) UINT NumRTVs, _Out_writes_opt_(NumRTVs) ID3D11RenderTargetView **ppRenderTargetViews, _Out_opt_ ID3D11DepthStencilView **ppDepthStencilView, _In_range_(0, D3D11_PS_CS_UAV_REGISTER_COUNT - 1) UINT UAVStartSlot, _In_range_(0, D3D11_PS_CS_UAV_REGISTER_COUNT - UAVStartSlot) UINT NumUAVs, _Out_writes_opt_(NumUAVs) ID3D11UnorderedAccessView **ppUnorderedAccessViews)
        {
            UNREFERENCED_PARAMETER(NumRTVs);
            UNREFERENCED_PARAMETER(ppRenderTargetViews);
            UNREFERENCED_PARAMETER(ppDepthStencilView);
            UNREFERENCED_PARAMETER(UAVStartSlot);
            UNREFERENCED_PARAMETER(NumUAVs);
            UNREFERENCED_PARAMETER(ppUnorderedAccessViews);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE OMGetBlendState(_Out_opt_ ID3D11BlendState **ppBlendState, _Out_opt_ FLOAT BlendFactor[4], _Out_opt_ UINT *pSampleMask)
        {
            UNREFERENCED_PARAMETER(ppBlendState);
            UNREFERENCED_PARAMETER(BlendFactor);
            UNREFERENCED_PARAMETER(pSampleMask);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE OMGetDepthStencilState(_Out_opt_ ID3D11DepthStencilState **ppDepthStencilState, _Out_opt_ UINT *pStencilRef)
        {
            UNREFERENCED_PARAMETER(ppDepthStencilState);
            UNREFERENCED_PARAMETER(pStencilRef);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE SOGetTargets(_In_range_(0, D3D11_SO_BUFFER_SLOT_COUNT) UINT NumBuffers, _Out_writes_opt_(NumBuffers) ID3D11Buffer **ppSOTargets)
        {
            UNREFERENCED_PARAMETER(NumBuffers);
            UNREFERENCED_PARAMETER(ppSOTargets);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE RSGetState(_Out_ ID3D11RasterizerState **ppRasterizerState)
        {
            UNREFERENCED_PARAMETER(ppRasterizerState);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE RSGetViewports(_Inout_ UINT *pNumViewports, _Out_writes_opt_(*pNumViewports) D3D11_VIEWPORT *pViewports)
        {
            UNREFERENCED_PARAMETER(pNumViewports);
            UNREFERENCED_PARAMETER(pViewports);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE RSGetScissorRects(_Inout_ UINT *pNumRects, _Out_writes_opt_(*pNumRects) D3D11_RECT *pRects)
        {
            UNREFERENCED_PARAMETER(pNumRects);
            UNREFERENCED_PARAMETER(pRects);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE HSGetShaderResources(_In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1) UINT StartSlot, _In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot) UINT NumViews, _Out_writes_opt_(NumViews) ID3D11ShaderResourceView **ppShaderResourceViews)
        {
            UNREFERENCED_PARAMETER(StartSlot);
            UNREFERENCED_PARAMETER(NumViews);
            UNREFERENCED_PARAMETER(ppShaderResourceViews);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE HSGetShader(_Out_ ID3D11HullShader **ppHullShader, _Out_writes_opt_(*pNumClassInstances) ID3D11ClassInstance **ppClassInstances, _Inout_opt_ UINT *pNumClassInstances)
        {
            UNREFERENCED_PARAMETER(ppHullShader);
            UNREFERENCED_PARAMETER(ppClassInstances);
            UNREFERENCED_PARAMETER(pNumClassInstances);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE HSGetSamplers(_In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1) UINT StartSlot, _In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot) UINT NumSamplers, _Out_writes_opt_(NumSamplers) ID3D11SamplerState **ppSamplers)
        {
            UNREFERENCED_PARAMETER(StartSlot);
            UNREFERENCED_PARAMETER(NumSamplers);
            UNREFERENCED_PARAMETER(ppSamplers);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE HSGetConstantBuffers(_In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1) UINT StartSlot, _In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot) UINT NumBuffers, _Out_writes_opt_(NumBuffers) ID3D11Buffer **ppConstantBuffers)
        {
            UNREFERENCED_PARAMETER(StartSlot);
            UNREFERENCED_PARAMETER(NumBuffers);
            UNREFERENCED_PARAMETER(ppConstantBuffers);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE DSGetShaderResources(_In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1) UINT StartSlot, _In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot) UINT NumViews, _Out_writes_opt_(NumViews) ID3D11ShaderResourceView **ppShaderResourceViews)
        {
            UNREFERENCED_PARAMETER(StartSlot);
            UNREFERENCED_PARAMETER(NumViews);
            UNREFERENCED_PARAMETER(ppShaderResourceViews);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE DSGetShader(_Out_ ID3D11DomainShader **ppDomainShader, _Out_writes_opt_(*pNumClassInstances) ID3D11ClassInstance **ppClassInstances, _Inout_opt_ UINT *pNumClassInstances)
        {
            UNREFERENCED_PARAMETER(ppDomainShader);
            UNREFERENCED_PARAMETER(ppClassInstances);
            UNREFERENCED_PARAMETER(pNumClassInstances);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE DSGetSamplers(_In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1) UINT StartSlot, _In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot) UINT NumSamplers, _Out_writes_opt_(NumSamplers) ID3D11SamplerState **ppSamplers)
        {
            UNREFERENCED_PARAMETER(StartSlot);
            UNREFERENCED_PARAMETER(NumSamplers);
            UNREFERENCED_PARAMETER(ppSamplers);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE DSGetConstantBuffers(_In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1) UINT StartSlot, _In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot) UINT NumBuffers, _Out_writes_opt_(NumBuffers) ID3D11Buffer **ppConstantBuffers)
        {
            UNREFERENCED_PARAMETER(StartSlot);
            UNREFERENCED_PARAMETER(NumBuffers);
            UNREFERENCED_PARAMETER(ppConstantBuffers);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE CSGetShaderResources(_In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - 1) UINT StartSlot, _In_range_(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT - StartSlot) UINT NumViews, _Out_writes_opt_(NumViews) ID3D11ShaderResourceView **ppShaderResourceViews)
        {
            UNREFERENCED_PARAMETER(StartSlot);
            UNREFERENCED_PARAMETER(NumViews);
            UNREFERENCED_PARAMETER(ppShaderResourceViews);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE CSGetUnorderedAccessViews(_In_range_(0, D3D11_PS_CS_UAV_REGISTER_COUNT - 1) UINT StartSlot, _In_range_(0, D3D11_PS_CS_UAV_REGISTER_COUNT - StartSlot) UINT NumUAVs, _Out_writes_opt_(NumUAVs) ID3D11UnorderedAccessView **ppUnorderedAccessViews)
        {
            UNREFERENCED_PARAMETER(StartSlot);
            UNREFERENCED_PARAMETER(NumUAVs);
            UNREFERENCED_PARAMETER(ppUnorderedAccessViews);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE CSGetShader(_Out_ ID3D11ComputeShader **ppComputeShader, _Out_writes_opt_(*pNumClassInstances) ID3D11ClassInstance **ppClassInstances, _Inout_opt_ UINT *pNumClassInstances)
        {
            *ppComputeShader = nullptr;
            UNREFERENCED_PARAMETER(ppClassInstances);
            UNREFERENCED_PARAMETER(pNumClassInstances);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE CSGetSamplers(_In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - 1) UINT StartSlot, _In_range_(0, D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT - StartSlot) UINT NumSamplers, _Out_writes_opt_(NumSamplers) ID3D11SamplerState **ppSamplers)
        {
            UNREFERENCED_PARAMETER(StartSlot);
            UNREFERENCED_PARAMETER(NumSamplers);
            UNREFERENCED_PARAMETER(ppSamplers);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE CSGetConstantBuffers(_In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - 1) UINT StartSlot, _In_range_(0, D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT - StartSlot) UINT NumBuffers, _Out_writes_opt_(NumBuffers) ID3D11Buffer **ppConstantBuffers)
        {
            UNREFERENCED_PARAMETER(StartSlot);
            UNREFERENCED_PARAMETER(NumBuffers);
            UNREFERENCED_PARAMETER(ppConstantBuffers);
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE ClearState(void)
        {
            VERIFY_FAIL();
        }

        virtual void STDMETHODCALLTYPE Flush(void)
        {
            VERIFY_FAIL();
        }

        virtual D3D11_DEVICE_CONTEXT_TYPE STDMETHODCALLTYPE GetType(void)
        {
            VERIFY_FAIL();
            return (D3D11_DEVICE_CONTEXT_TYPE)0;
        }

        virtual UINT STDMETHODCALLTYPE GetContextFlags(void)
        {
            VERIFY_FAIL();
            return 0;
        }

        virtual HRESULT STDMETHODCALLTYPE FinishCommandList(BOOL RestoreDeferredContextState, _Out_opt_ ID3D11CommandList **ppCommandList)
        {
            UNREFERENCED_PARAMETER(RestoreDeferredContextState);
            UNREFERENCED_PARAMETER(ppCommandList);
            return E_NOTIMPL;
        }
    };

    HRESULT CreateMockD3D11DeviceContext(_In_ ID3D11Device * device, _Out_ IMockD3D11DeviceContext ** context)
    {
        *context = new MockD3D11DeviceContext(device);
        return S_OK;
    }

} } } } }
