// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "DeviceResources.h"
#include "DX12Helper.h"
#include <microsoft.ui.xaml.media.dxinterop.h>
#include <winrt/microsoft.UI.Dispatching.h>

using namespace DX;

const float DeviceResources::QuadWidth = 20.0f;
const float DeviceResources::QuadHeight = 720.0f;
const float DeviceResources::LetterboxColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
const float DeviceResources::ClearColor[4] = { 0.0f, 0.2f, 0.4f, 1.0f };

DeviceResources::DeviceResources(HWND hwnd) :
    m_sceneViewport(CD3DX12_VIEWPORT()),
    m_sceneScissorRect(CD3DX12_RECT()),
    m_postViewport(CD3DX12_VIEWPORT()),
    m_postScissorRect(CD3DX12_RECT()),
    m_hwnd(hwnd)
{
}

void DeviceResources::SetSwapChainPanel(const winrt::Microsoft::UI::Xaml::Controls::SwapChainPanel& panel)
{
    m_swapChainPanel = panel;
    SetLogicalSize(static_cast<float>(panel.ActualWidth()), static_cast<float>(panel.ActualHeight()));
}

void DeviceResources::Initialize()
{
        LoadPipeline();
        LoadAssets();
}

// Load the rendering pipeline dependencies.
void DeviceResources::LoadPipeline()
{
    UINT dxgiFactoryFlags = 0;

#if defined(_DEBUG)
    // Enable the debug layer (requires the Graphics Tools "optional feature").
    // NOTE: Enabling the debug layer after device creation will invalidate the active device.
    {
        winrt::com_ptr<ID3D12Debug> debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
        {
            debugController->EnableDebugLayer();

            // Enable additional debug layers.
            dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
        }
    }
#endif

    winrt::com_ptr<IDXGIFactory4> factory;
    winrt::check_hresult(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(factory.put())));

    winrt::com_ptr<IDXGIAdapter1> hardwareAdapter;
    GetHardwareAdapter(factory.get(), hardwareAdapter.put());

    winrt::check_hresult(D3D12CreateDevice(
        hardwareAdapter.get(),
        D3D_FEATURE_LEVEL_11_0,
        IID_PPV_ARGS(&m_device)
    ));

    // Describe and create the command queue.
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    winrt::check_hresult(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));

    // This sample does not support fullscreen transitions.
    winrt::check_hresult(factory->MakeWindowAssociation(m_hwnd, DXGI_MWA_NO_ALT_ENTER));

    CreateSwapChain();

    // Create descriptor heaps.
    {
        // Describe and create a render target view (RTV) descriptor heap.
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
        rtvHeapDesc.NumDescriptors = FrameCount + 1; // + 1 for the intermediate render target.
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        winrt::check_hresult(m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));


        // Describe and create a constant buffer view (CBV) and shader resource view (SRV) descriptor heap.
        D3D12_DESCRIPTOR_HEAP_DESC cbvSrvHeapDesc = {};
        cbvSrvHeapDesc.NumDescriptors = FrameCount + 1; // One CBV per frame and one SRV for the intermediate render target.
        cbvSrvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        cbvSrvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        winrt::check_hresult(m_device->CreateDescriptorHeap(&cbvSrvHeapDesc, IID_PPV_ARGS(&m_cbvSrvHeap)));

        m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        m_cbvSrvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }

    // Create command allocators for each frame.
    for (UINT n = 0; n < FrameCount; n++)
    {
        winrt::check_hresult(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_sceneCommandAllocators[n])));
        winrt::check_hresult(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_postCommandAllocators[n])));
    }

}

// Load the sample assets.
void DeviceResources::LoadAssets()
{
    D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};

    // This is the highest version the sample supports. If CheckFeatureSupport succeeds, the HighestVersion returned will not be greater than this.
    featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

    if (FAILED(m_device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
    {
        featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }

    // Create a root signature consisting of a descriptor table with a single CBV.
    {
        CD3DX12_DESCRIPTOR_RANGE1 ranges[1] = {};
        CD3DX12_ROOT_PARAMETER1 rootParameters[1] = {};

        ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
        rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_VERTEX);

        // Allow input layout and deny uneccessary access to certain pipeline stages.
        D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
        rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, 0, nullptr, rootSignatureFlags);

        winrt::com_ptr<ID3DBlob> signature;
        winrt::com_ptr<ID3DBlob> error;
        winrt::check_hresult(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, signature.put(), error.put()));
        winrt::check_hresult(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(m_sceneRootSignature.put())));
        NAME_D3D12_OBJECT(m_sceneRootSignature);
    }

    // Create a root signature consisting of a descriptor table with a SRV and a sampler.
    {
        CD3DX12_DESCRIPTOR_RANGE1 ranges[1];
        CD3DX12_ROOT_PARAMETER1 rootParameters[1];

        // We don't modify the SRV in the post-processing command list after
        // SetGraphicsRootDescriptorTable is executed on the GPU so we can use the default
        // range behavior: D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE
        ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
        rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL);

        // Allow input layout and pixel shader access and deny uneccessary access to certain pipeline stages.
        D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

        // Create a sampler.
        D3D12_STATIC_SAMPLER_DESC sampler = {};
        sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        sampler.MipLODBias = 0;
        sampler.MaxAnisotropy = 0;
        sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
        sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
        sampler.MinLOD = 0.0f;
        sampler.MaxLOD = D3D12_FLOAT32_MAX;
        sampler.ShaderRegister = 0;
        sampler.RegisterSpace = 0;
        sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
        rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, 1, &sampler, rootSignatureFlags);

        winrt::com_ptr<ID3DBlob> signature;
        winrt::com_ptr<ID3DBlob> error;
        winrt::check_hresult(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, signature.put(), error.put()));
        winrt::check_hresult(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(m_postRootSignature.put())));
        NAME_D3D12_OBJECT(m_postRootSignature);
    }

    // Create the pipeline state, which includes compiling and loading shaders.
    {
        winrt::com_ptr<ID3DBlob> sceneVertexShader;
        winrt::com_ptr<ID3DBlob> scenePixelShader;
        winrt::com_ptr<ID3DBlob> postVertexShader;
        winrt::com_ptr<ID3DBlob> postPixelShader;
        winrt::com_ptr<ID3DBlob> error;

#if defined(_DEBUG)
        // Enable better shader debugging with the graphics debugging tools.
        UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
        UINT compileFlags = 0;
#endif

        winrt::check_hresult(D3DCompileFromFile(GetAssetFullPath(L"sceneShaders.hlsl").c_str(), nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, sceneVertexShader.put(), error.put()));
        winrt::check_hresult(D3DCompileFromFile(GetAssetFullPath(L"sceneShaders.hlsl").c_str(), nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, scenePixelShader.put(), error.put()));

        winrt::check_hresult(D3DCompileFromFile(GetAssetFullPath(L"postShaders.hlsl").c_str(), nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, postVertexShader.put(), error.put()));
        winrt::check_hresult(D3DCompileFromFile(GetAssetFullPath(L"postShaders.hlsl").c_str(), nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, postPixelShader.put(), error.put()));

        // Define the vertex input layouts.
        D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
        };
        D3D12_INPUT_ELEMENT_DESC scaleInputElementDescs[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
        };

        // Describe and create the graphics pipeline state objects (PSOs).
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
        psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
        psoDesc.pRootSignature = m_sceneRootSignature.get();
        psoDesc.VS = CD3DX12_SHADER_BYTECODE(sceneVertexShader.get());
        psoDesc.PS = CD3DX12_SHADER_BYTECODE(scenePixelShader.get());
        psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
        psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
        psoDesc.DepthStencilState.DepthEnable = FALSE;
        psoDesc.DepthStencilState.StencilEnable = FALSE;
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDesc.SampleDesc.Count = 1;

        winrt::check_hresult(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(m_scenePipelineState.put())));
        NAME_D3D12_OBJECT(m_scenePipelineState);

        psoDesc.InputLayout = { scaleInputElementDescs, _countof(scaleInputElementDescs) };
        psoDesc.pRootSignature = m_postRootSignature.get();
        psoDesc.VS = CD3DX12_SHADER_BYTECODE(postVertexShader.get());
        psoDesc.PS = CD3DX12_SHADER_BYTECODE(postPixelShader.get());

        winrt::check_hresult(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(m_postPipelineState.put())));
        NAME_D3D12_OBJECT(m_postPipelineState);
    }

    // Single-use command allocator and command list for creating resources.
    winrt::com_ptr<ID3D12CommandAllocator> commandAllocator;
    winrt::com_ptr<ID3D12GraphicsCommandList> commandList;

    winrt::check_hresult(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(commandAllocator.put())));
    winrt::check_hresult(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.get(), nullptr, IID_PPV_ARGS(commandList.put())));

    // Create the command lists.
    {
        winrt::check_hresult(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_sceneCommandAllocators[m_frameIndex].get(), m_scenePipelineState.get(), IID_PPV_ARGS(m_sceneCommandList.put())));
        NAME_D3D12_OBJECT(m_sceneCommandList);

        winrt::check_hresult(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_postCommandAllocators[m_frameIndex].get(), m_postPipelineState.get(), IID_PPV_ARGS(m_postCommandList.put())));
        NAME_D3D12_OBJECT(m_postCommandList);

        // Close the command lists.
        winrt::check_hresult(m_sceneCommandList->Close());
        winrt::check_hresult(m_postCommandList->Close());
    }

    LoadSizeDependentResources();
    LoadSceneResolutionDependentResources();

    // Create/update the vertex buffer.
    winrt::com_ptr<ID3D12Resource> sceneVertexBufferUpload;
    {
        // Define the geometry for a thin quad that will animate across the screen.
        const float x = QuadWidth / 2.0f;
        const float y = QuadHeight / 2.0f;
        SceneVertex quadVertices[] =
        {
            { { -x, -y, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
            { { -x, y, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
            { { x, -y, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } },
            { { x, y, 1.0f }, { 1.0f, 1.0f, 1.0f, 1.0f } }
        };

        const UINT vertexBufferSize = sizeof(quadVertices);
        CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);
        CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);
        winrt::check_hresult(m_device->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(&m_sceneVertexBuffer)));

        CD3DX12_HEAP_PROPERTIES heapPropertiesUpload(D3D12_HEAP_TYPE_UPLOAD);
        CD3DX12_RESOURCE_DESC resourceDescUpload = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);
        winrt::check_hresult(m_device->CreateCommittedResource(
            &heapPropertiesUpload,
            D3D12_HEAP_FLAG_NONE,
            &resourceDescUpload,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(sceneVertexBufferUpload.put())));

        NAME_D3D12_OBJECT(m_sceneVertexBuffer);

        // Copy data to the intermediate upload heap and then schedule a copy 
        // from the upload heap to the vertex buffer.
        UINT8* pVertexDataBegin;
        CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
        winrt::check_hresult(sceneVertexBufferUpload->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
        memcpy(pVertexDataBegin, quadVertices, sizeof(quadVertices));
        sceneVertexBufferUpload->Unmap(0, nullptr);

        commandList->CopyBufferRegion(m_sceneVertexBuffer.get(), 0, sceneVertexBufferUpload.get(), 0, vertexBufferSize);
        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_sceneVertexBuffer.get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        commandList->ResourceBarrier(1, &barrier);

        // Initialize the vertex buffer views.
        m_sceneVertexBufferView.BufferLocation = m_sceneVertexBuffer->GetGPUVirtualAddress();
        m_sceneVertexBufferView.StrideInBytes = sizeof(SceneVertex);
        m_sceneVertexBufferView.SizeInBytes = vertexBufferSize;
    }

    // Create/update the fullscreen quad vertex buffer.
    winrt::com_ptr<ID3D12Resource> postVertexBufferUpload;
    {
        // Define the geometry for a fullscreen quad.
        PostVertex quadVertices[] =
        {
            { { -1.0f, -1.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },    // Bottom left.
            { { -1.0f, 1.0f, 0.0f, 1.0f }, { 0.0f, 1.0f } },    // Top left.
            { { 1.0f, -1.0f, 0.0f, 1.0f }, { 1.0f, 0.0f } },    // Bottom right.
            { { 1.0f, 1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } }        // Top right.
        };

        const UINT vertexBufferSize = sizeof(quadVertices);

        CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);
        CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);
        winrt::check_hresult(m_device->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(m_postVertexBuffer.put())));

        CD3DX12_HEAP_PROPERTIES heapPropertiesUpload(D3D12_HEAP_TYPE_UPLOAD);
        CD3DX12_RESOURCE_DESC resourceDescUpload = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);
        winrt::check_hresult(m_device->CreateCommittedResource(
            &heapPropertiesUpload,
            D3D12_HEAP_FLAG_NONE,
            &resourceDescUpload,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(postVertexBufferUpload.put())));

        NAME_D3D12_OBJECT(m_postVertexBuffer);

        // Copy data to the intermediate upload heap and then schedule a copy 
        // from the upload heap to the vertex buffer.
        UINT8* pVertexDataBegin;
        CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
        winrt::check_hresult(postVertexBufferUpload->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
        memcpy(pVertexDataBegin, quadVertices, sizeof(quadVertices));
        postVertexBufferUpload->Unmap(0, nullptr);

        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_postVertexBuffer.get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
        commandList->CopyBufferRegion(m_postVertexBuffer.get(), 0, postVertexBufferUpload.get(), 0, vertexBufferSize);
        commandList->ResourceBarrier(1, &barrier);

        // Initialize the vertex buffer views.
        m_postVertexBufferView.BufferLocation = m_postVertexBuffer->GetGPUVirtualAddress();
        m_postVertexBufferView.StrideInBytes = sizeof(PostVertex);
        m_postVertexBufferView.SizeInBytes = vertexBufferSize;
    }

    // Create the constant buffer.
    {
        CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_UPLOAD);
        CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(SceneConstantBuffer) * FrameCount);
        winrt::check_hresult(m_device->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(m_sceneConstantBuffer.put())));

        NAME_D3D12_OBJECT(m_sceneConstantBuffer);

        // Describe and create constant buffer views.
        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
        cbvDesc.BufferLocation = m_sceneConstantBuffer->GetGPUVirtualAddress();
        cbvDesc.SizeInBytes = sizeof(SceneConstantBuffer);

        CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle(m_cbvSrvHeap->GetCPUDescriptorHandleForHeapStart(), 1, m_cbvSrvDescriptorSize);

        for (UINT n = 0; n < FrameCount; n++)
        {
            m_device->CreateConstantBufferView(&cbvDesc, cpuHandle);

            cbvDesc.BufferLocation += sizeof(SceneConstantBuffer);
            cpuHandle.Offset(m_cbvSrvDescriptorSize);
        }

        // Map and initialize the constant buffer. We don't unmap this until the
        // app closes. Keeping things mapped for the lifetime of the resource is okay.
        CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
        winrt::check_hresult(m_sceneConstantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_pCbvDataBegin)));
        memcpy(m_pCbvDataBegin, &m_sceneConstantBufferData, sizeof(m_sceneConstantBufferData));
    }

    // Close the resource creation command list and execute it to begin the vertex buffer copy into
    // the default heap.
    winrt::check_hresult(commandList->Close());
    ID3D12CommandList* ppCommandLists[] = { commandList.get() };
    m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    // Create synchronization objects and wait until assets have been uploaded to the GPU.
    {
        winrt::check_hresult(m_device->CreateFence(m_fenceValues[m_frameIndex], D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_fence.put())));
        m_fenceValues[m_frameIndex]++;

        // Create an event handle to use for frame synchronization.
        m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (m_fenceEvent == nullptr)
        {
            winrt::check_hresult(HRESULT_FROM_WIN32(GetLastError()));
        }

        // Wait for the command list to execute before continuing.
        WaitForGpu();
    }
}
void DeviceResources::LoadSizeDependentResources()
{
    UpdatePostViewAndScissor();

    // Create frame resources.
    {
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());

        // Create a RTV for each frame.
        for (UINT n = 0; n < FrameCount; n++)
        {
            winrt::check_hresult(m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargets[n])));
            m_device->CreateRenderTargetView(m_renderTargets[n].get(), nullptr, rtvHandle);
            rtvHandle.Offset(1, m_rtvDescriptorSize);

            NAME_D3D12_OBJECT_INDEXED(m_renderTargets, n);
        }
    }
}

// Update frame-based values.
void DeviceResources::UpdateFrame()
{
    const float translationSpeed = 0.0001f;
    const float offsetBounds = 1.0f;

    m_sceneConstantBufferData.offset.x += translationSpeed;
    if (m_sceneConstantBufferData.offset.x > offsetBounds)
    {
        m_sceneConstantBufferData.offset.x = -offsetBounds;
    }

    DirectX::XMMATRIX transform = DirectX::XMMatrixMultiply(
        DirectX::XMMatrixOrthographicLH(static_cast<float>(m_width), static_cast<float>(m_height), 0.0f, 100.0f),
        DirectX::XMMatrixTranslation(m_sceneConstantBufferData.offset.x, 0.0f, 0.0f));

    XMStoreFloat4x4(&m_sceneConstantBufferData.transform, XMMatrixTranspose(transform));

    UINT offset = m_frameIndex * sizeof(SceneConstantBuffer);
    memcpy(m_pCbvDataBegin + offset, &m_sceneConstantBufferData, sizeof(m_sceneConstantBufferData));
}


// Render the scene.
void DeviceResources::Render()
{
    try
    {

        // Record all the commands we need to render the scene into the command lists.
        PopulateCommandLists();

        // Execute the command lists.
        ID3D12CommandList* ppCommandLists[] = { m_sceneCommandList.get(), m_postCommandList.get() };
        m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

        // Present the frame.
        winrt::check_hresult(m_swapChain->Present(0, 0));

        MoveToNextFrame();
    }
    catch (...)
    {
        HRESULT hr = winrt::to_hresult();
        if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
        {
            RestoreD3DResources();
        }
        else
        {
            throw;
        }
    }
}

// Release sample's D3D objects.
void DeviceResources::ReleaseD3DResources()
{
    m_fence = nullptr;
    m_commandQueue = nullptr;
    m_swapChain = nullptr;
    m_device = nullptr;

    for (auto& i : m_renderTargets)
    {
        i = nullptr;
    }
}
// Tears down D3D resources and reinitializes them.
void DeviceResources::RestoreD3DResources()
{
    std::lock_guard lock(m_mutex);

    // Give GPU a chance to finish its execution in progress.
    try
    {
        WaitForGpu();
    }
    catch (...)
    {
        // Do nothing, currently attached adapter is unresponsive.
    }
    ReleaseD3DResources();
    Initialize();
}
void DeviceResources::SetLogicalSize(float width, float height)
{
    UINT newWidth = static_cast<UINT>(width);
    UINT newHeight = static_cast<UINT>(height);

    // Determine if the swap buffers and other resources need to be resized or not.
    if (newWidth == m_width && newHeight == m_height) return;

    m_width = newWidth;
    m_height = newHeight;
    m_aspectRatio = width / height;
  
    // If we haven't created our swap chain yet, we can't resize it.
    if (m_swapChain == nullptr) return;

    std::lock_guard lock(m_mutex);

    // Flush all current GPU commands.
    WaitForGpu();

    // Release the resources holding references to the swap chain (requirement of
    // IDXGISwapChain::ResizeBuffers) and reset the frame fence values to the
    // current fence value.
    for (UINT n = 0; n < FrameCount; n++)
    {
        m_renderTargets[n] = nullptr;
        m_fenceValues[n] = m_fenceValues[m_frameIndex];
    }

    // Resize the swap chain to the desired dimensions.
    DXGI_SWAP_CHAIN_DESC desc = {};
    m_swapChain->GetDesc(&desc);
    HRESULT hr = m_swapChain->ResizeBuffers(FrameCount, m_width, m_height, desc.BufferDesc.Format, desc.Flags);
    
    if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
    {
        // If the device was removed for any reason, a new device and swap chain will need to be created.
        RestoreD3DResources();
        return;
    }
    winrt::check_hresult(hr);

    // Reset the frame index to the current back buffer index.
    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

    LoadSizeDependentResources();
}

   

// Fill the command list with all the render commands and dependent state.
void DeviceResources::PopulateCommandLists()
{
    this->m_sceneCommandAllocators[m_frameIndex]->Reset();

    // Command list allocators can only be reset when the associated 
    // command lists have finished execution on the GPU; apps should use 
    // fences to determine GPU execution progress.
    
    winrt::check_hresult(m_sceneCommandAllocators[m_frameIndex]->Reset());
    winrt::check_hresult(m_postCommandAllocators[m_frameIndex]->Reset());
    
    // However, when ExecuteCommandList() is called on a particular command 
    // list, that command list can then be reset at any time and must be before 
    // re-recording.
    winrt::check_hresult(m_sceneCommandList->Reset(m_sceneCommandAllocators[m_frameIndex].get(), m_scenePipelineState.get()));
    winrt::check_hresult(m_postCommandList->Reset(m_postCommandAllocators[m_frameIndex].get(), m_postPipelineState.get()));

    // Populate m_sceneCommandList to render scene to intermediate render target.
    {
        // Set necessary state.
        m_sceneCommandList->SetGraphicsRootSignature(m_sceneRootSignature.get());

        ID3D12DescriptorHeap* ppHeaps[] = { m_cbvSrvHeap.get() };
        m_sceneCommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

        CD3DX12_GPU_DESCRIPTOR_HANDLE cbvHandle(m_cbvSrvHeap->GetGPUDescriptorHandleForHeapStart(), m_frameIndex + 1, m_cbvSrvDescriptorSize);
        m_sceneCommandList->SetGraphicsRootDescriptorTable(0, cbvHandle);
        m_sceneCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        m_sceneCommandList->RSSetViewports(1, &m_sceneViewport);
        m_sceneCommandList->RSSetScissorRects(1, &m_sceneScissorRect);

        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), FrameCount, m_rtvDescriptorSize);
        m_sceneCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

        // Record commands.
        m_sceneCommandList->ClearRenderTargetView(rtvHandle, ClearColor, 0, nullptr);
        m_sceneCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        m_sceneCommandList->IASetVertexBuffers(0, 1, &m_sceneVertexBufferView);

        m_sceneCommandList->DrawInstanced(4, 1, 0, 0);
    }

    winrt::check_hresult(m_sceneCommandList->Close());

    // Populate m_postCommandList to scale intermediate render target to screen.
    {
        // Set necessary state.
        m_postCommandList->SetGraphicsRootSignature(m_postRootSignature.get());

        ID3D12DescriptorHeap* ppHeaps[] = { m_cbvSrvHeap.get() };
        m_postCommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

        // Indicate that the back buffer will be used as a render target and the
        // intermediate render target will be used as a SRV.
        D3D12_RESOURCE_BARRIER barriers[] = {
            CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET),
            CD3DX12_RESOURCE_BARRIER::Transition(m_intermediateRenderTarget.get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
        };

        m_postCommandList->ResourceBarrier(_countof(barriers), barriers);

        m_postCommandList->SetGraphicsRootDescriptorTable(0, m_cbvSrvHeap->GetGPUDescriptorHandleForHeapStart());
        m_postCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        m_postCommandList->RSSetViewports(1, &m_postViewport);
        m_postCommandList->RSSetScissorRects(1, &m_postScissorRect);

        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize);
        m_postCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

        // Record commands.
        m_postCommandList->ClearRenderTargetView(rtvHandle, LetterboxColor, 0, nullptr);
        m_postCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        m_postCommandList->IASetVertexBuffers(0, 1, &m_postVertexBufferView);

        m_postCommandList->DrawInstanced(4, 1, 0, 0);

        // Revert resource states back to original values.
        barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

        m_postCommandList->ResourceBarrier(_countof(barriers), barriers);
    }

    winrt::check_hresult(m_postCommandList->Close());
}
void DeviceResources::Present()
{
}

// Wait for pending GPU work to complete.
void DeviceResources::WaitForGpu()
{
    // Schedule a Signal command in the queue.
    winrt::check_hresult(m_commandQueue->Signal(m_fence.get(), m_fenceValues[m_frameIndex]));

    // Wait until the fence has been processed.
    winrt::check_hresult(m_fence->SetEventOnCompletion(m_fenceValues[m_frameIndex], m_fenceEvent));
    WaitForSingleObjectEx(m_fenceEvent, INFINITE, FALSE);

    // Increment the fence value for the current frame.
    m_fenceValues[m_frameIndex]++;
}

// Prepare to render the next frame.
void DeviceResources::MoveToNextFrame()
{
    // Schedule a Signal command in the queue.
    const UINT64 currentFenceValue = m_fenceValues[m_frameIndex];
    winrt::check_hresult(m_commandQueue->Signal(m_fence.get(), currentFenceValue));

    // Update the frame index.
    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

    // If the next frame is not ready to be rendered yet, wait until it is ready.
    if (m_fence->GetCompletedValue() < m_fenceValues[m_frameIndex])
    {
        winrt::check_hresult(m_fence->SetEventOnCompletion(m_fenceValues[m_frameIndex], m_fenceEvent));
        WaitForSingleObjectEx(m_fenceEvent, INFINITE, FALSE);
    }

    // Set the fence value for the next frame.
    m_fenceValues[m_frameIndex] = currentFenceValue + 1;
}
// Set up the screen viewport and scissor rect to match the current window size and scene rendering resolution.
void DeviceResources::UpdatePostViewAndScissor()
{
    float viewWidthRatio = 1;
    float viewHeightRatio = 1;

    float x = 1.0f;
    float y = 1.0f;

    if (viewWidthRatio < viewHeightRatio)
    {
        // The scaled image's height will fit to the viewport's height and 
        // its width will be smaller than the viewport's width.
        x = viewWidthRatio / viewHeightRatio;
    }
    else
    {
        // The scaled image's width will fit to the viewport's width and 
        // its height may be smaller than the viewport's height.
        y = viewHeightRatio / viewWidthRatio;
    }

    m_postViewport.TopLeftX = m_width * (1.0f - x) / 2.0f;
    m_postViewport.TopLeftY = m_height * (1.0f - y) / 2.0f;
    m_postViewport.Width = x * m_width;
    m_postViewport.Height = y * m_height;

    m_postScissorRect.left = static_cast<LONG>(m_postViewport.TopLeftX);
    m_postScissorRect.right = static_cast<LONG>(m_postViewport.TopLeftX + m_postViewport.Width);
    m_postScissorRect.top = static_cast<LONG>(m_postViewport.TopLeftY);
    m_postScissorRect.bottom = static_cast<LONG>(m_postViewport.TopLeftY + m_postViewport.Height);

}
// Set up appropriate views for the intermediate render target.
void DeviceResources::LoadSceneResolutionDependentResources()
{

    // Set up the scene viewport and scissor rect to match the current scene rendering resolution.
    {
        m_sceneViewport.Width = static_cast<float>(m_width);
        m_sceneViewport.Height = static_cast<float>(m_height);

        m_sceneScissorRect.right = static_cast<LONG>(m_width);
        m_sceneScissorRect.bottom = static_cast<LONG>(m_height);
    }

    // Update post-process viewport and scissor rectangle.
    UpdatePostViewAndScissor();

    // Create RTV for the intermediate render target.
    {
        D3D12_RESOURCE_DESC swapChainDesc = m_renderTargets[m_frameIndex]->GetDesc();
        const CD3DX12_CLEAR_VALUE clearValue(swapChainDesc.Format, ClearColor);
        const CD3DX12_RESOURCE_DESC renderTargetDesc = CD3DX12_RESOURCE_DESC::Tex2D(
            swapChainDesc.Format,
            m_width,
            m_height,
            1u, 1u,
            swapChainDesc.SampleDesc.Count,
            swapChainDesc.SampleDesc.Quality,
            D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,
            D3D12_TEXTURE_LAYOUT_UNKNOWN, 0u);

        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), FrameCount, m_rtvDescriptorSize);
        CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);
        winrt::check_hresult(m_device->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &renderTargetDesc,
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            &clearValue,
            IID_PPV_ARGS(&m_intermediateRenderTarget)));
        m_device->CreateRenderTargetView(m_intermediateRenderTarget.get(), nullptr, rtvHandle);
        NAME_D3D12_OBJECT(m_intermediateRenderTarget);
    }

    // Create SRV for the intermediate render target.
    m_device->CreateShaderResourceView(m_intermediateRenderTarget.get(), nullptr, m_cbvSrvHeap->GetCPUDescriptorHandleForHeapStart());
}

void DeviceResources::CreateSwapChain()
{
    winrt::com_ptr<IDXGIFactory4> dxgiFactory;
    winrt::check_hresult(CreateDXGIFactory1(IID_PPV_ARGS(dxgiFactory.put())));

    // Describe and create the swap chain.
    // The resolution of the swap chain buffers will match the resolution of the window, enabling the
    // app to enter iFlip when in fullscreen mode. We will also keep a separate buffer that is not part
    // of the swap chain as an intermediate render target, whose resolution will control the rendering
    // resolution of the scene.
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.BufferCount = FrameCount;
    swapChainDesc.Width = m_width;
    swapChainDesc.Height = m_height;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;

    winrt::com_ptr<IDXGISwapChain1> swapChain;
    winrt::check_hresult(dxgiFactory->CreateSwapChainForHwnd(
        m_commandQueue.get(),        // Swap chain needs the queue so that it can force a flush on it.
        m_hwnd,
        &swapChainDesc,
        nullptr,
        nullptr,
        swapChain.put()
    ));

    winrt::check_hresult(swapChain.as(IID_PPV_ARGS(m_swapChain.put())));
    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

    // Associate swap chain with SwapChainPanel
    // UI changes will need to be dispatched back to the UI thread
    m_swapChainPanel.DispatcherQueue().TryEnqueue(winrt::Microsoft::UI::Dispatching::DispatcherQueuePriority::Normal, winrt::Microsoft::UI::Dispatching::DispatcherQueueHandler([=]()
            {
            // Get backing native interface for SwapChainPanel
            winrt::com_ptr<ISwapChainPanelNative> panelNative;
            winrt::check_hresult(winrt::get_unknown(m_swapChainPanel)->QueryInterface(IID_PPV_ARGS(&panelNative)));
            winrt::check_hresult(panelNative->SetSwapChain(m_swapChain.get()));
        }));


    // Ensure that DXGI does not queue more than one frame at a time. This both reduces latency and
    // ensures that the application will only render after each VSync, minimizing power consumption.
    winrt::check_hresult(m_swapChain->SetMaximumFrameLatency(1));
}


void DeviceResources::GetHardwareAdapter(
    IDXGIFactory1* pFactory,
    IDXGIAdapter1** ppAdapter)
{
    *ppAdapter = nullptr;

    winrt::com_ptr<IDXGIAdapter1> adapter;

    winrt::com_ptr<IDXGIFactory6> factory6;
    if (SUCCEEDED(pFactory->QueryInterface(IID_PPV_ARGS(factory6.put()))))
    {
        for (
            UINT adapterIndex = 0;
            DXGI_ERROR_NOT_FOUND != factory6->EnumAdapterByGpuPreference(
                adapterIndex,
                DXGI_GPU_PREFERENCE_UNSPECIFIED,
                IID_PPV_ARGS(adapter.put()));
            ++adapterIndex)
        {
            DXGI_ADAPTER_DESC1 desc;
            adapter->GetDesc1(&desc);

            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                // Don't select the Basic Render Driver adapter.
                // If you want a software adapter, pass in "/warp" on the command line.
                continue;
            }

            // Check to see whether the adapter supports Direct3D 12, but don't create the
            // actual device yet.
            if (SUCCEEDED(D3D12CreateDevice(adapter.get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
            {
                break;
            }
        }
    }
    else
    {
        for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != pFactory->EnumAdapters1(adapterIndex, adapter.put()); ++adapterIndex)
        {
            DXGI_ADAPTER_DESC1 desc;
            adapter->GetDesc1(&desc);

            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                // Don't select the Basic Render Driver adapter.
                // If you want a software adapter, pass in "/warp" on the command line.
                continue;
            }

            // Check to see whether the adapter supports Direct3D 12, but don't create the
            // actual device yet.
            if (SUCCEEDED(D3D12CreateDevice(adapter.get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
            {
                break;
            }
        }
    }

    if (adapter == nullptr) {
        RaiseException(0, EXCEPTION_NONCONTINUABLE, 0, NULL);
    }

    *ppAdapter = adapter.detach();
}


