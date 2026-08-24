// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

struct CD3DX12_VIEWPORT;
struct CD3DX12_RECT;

namespace DX
{
    struct SceneConstantBuffer
    {
        DirectX::XMFLOAT4X4 transform;
        DirectX::XMFLOAT4 offset;
        UINT padding[44];
    };

    struct SceneVertex
    {
        DirectX::XMFLOAT3 position;
        DirectX::XMFLOAT4 color;
    };

    struct PostVertex
    {
        DirectX::XMFLOAT4 position;
        DirectX::XMFLOAT2 uv;
    };

    class DeviceResources
    {
    public:
        DeviceResources(HWND hwnd);
        void CreateSwapChain();
        void Initialize();
        void SetSwapChainPanel(const winrt::Microsoft::UI::Xaml::Controls::SwapChainPanel& panel);
        void SetLogicalSize(float width, float height);
        void LoadSizeDependentResources();
        void UpdateFrame();
        void UpdatePostViewAndScissor();
        void ReleaseD3DResources();
        void Render();
        void Present();
        
        std::recursive_mutex& GetMutex() { return m_mutex; }

    private:
        void LoadPipeline();
        void LoadAssets();
        void LoadSceneResolutionDependentResources();
        void GetHardwareAdapter(IDXGIFactory1* pFactory, IDXGIAdapter1** ppAdapter);
        void PopulateCommandLists();
        void MoveToNextFrame();
        void WaitForGpu();
        void RestoreD3DResources();

        static const UINT FrameCount = 2;
        static const float QuadWidth;
        static const float QuadHeight;
        static const float LetterboxColor[4];
        static const float ClearColor[4];

        // Pipeline objects.
        D3D12_VIEWPORT m_sceneViewport;
        D3D12_VIEWPORT m_postViewport;
        D3D12_RECT m_sceneScissorRect;
        D3D12_RECT m_postScissorRect;
        winrt::com_ptr<IDXGISwapChain3> m_swapChain;
        winrt::com_ptr<ID3D12Device> m_device;
        winrt::com_ptr<ID3D12Resource> m_renderTargets[FrameCount];
        winrt::com_ptr<ID3D12Resource> m_intermediateRenderTarget;
        winrt::com_ptr<ID3D12CommandAllocator> m_sceneCommandAllocators[FrameCount];
        winrt::com_ptr<ID3D12CommandAllocator> m_postCommandAllocators[FrameCount];
        winrt::com_ptr<ID3D12CommandQueue> m_commandQueue;
        winrt::com_ptr<ID3D12RootSignature> m_sceneRootSignature;
        winrt::com_ptr<ID3D12RootSignature> m_postRootSignature;
        winrt::com_ptr<ID3D12DescriptorHeap> m_rtvHeap;
        winrt::com_ptr<ID3D12DescriptorHeap> m_cbvSrvHeap;
        winrt::com_ptr<ID3D12PipelineState> m_scenePipelineState;
        winrt::com_ptr<ID3D12PipelineState> m_postPipelineState;
        winrt::com_ptr<ID3D12GraphicsCommandList> m_sceneCommandList;
        winrt::com_ptr<ID3D12GraphicsCommandList> m_postCommandList;
        UINT m_rtvDescriptorSize = 0;
        UINT m_cbvSrvDescriptorSize = 0;

        // App resources.
        winrt::com_ptr<ID3D12Resource> m_sceneVertexBuffer;
        D3D12_VERTEX_BUFFER_VIEW m_sceneVertexBufferView;
        winrt::com_ptr<ID3D12Resource> m_postVertexBuffer;
        D3D12_VERTEX_BUFFER_VIEW m_postVertexBufferView;
        winrt::com_ptr<ID3D12Resource> m_sceneConstantBuffer;
        SceneConstantBuffer m_sceneConstantBufferData{};
        UINT8* m_pCbvDataBegin = nullptr;

        // Synchronization objects.
        UINT m_frameIndex = 0;
        HANDLE m_fenceEvent;
        winrt::com_ptr<ID3D12Fence> m_fence;
        UINT64 m_fenceValues[FrameCount]{};

        // Swapchain members
        winrt::Microsoft::UI::Xaml::Controls::SwapChainPanel m_swapChainPanel;

        UINT                                            m_width{};
        UINT                                            m_height{};
        float                                           m_compositionScaleX = 1;
        float                                           m_compositionScaleY = 1;
        float                                           m_aspectRatio = 1;
        HWND                                            m_hwnd;

        std::recursive_mutex                            m_mutex;

    };
}

