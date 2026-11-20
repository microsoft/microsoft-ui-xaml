// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <ImmediatePainter.h>
#include <wrl/client.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class CUIElement;
class CXamlIslandRoot;
class DCompTreeHost;
class CMILMatrix;

namespace ImmediateRenderingDetail
{
    struct DisplayList;
    struct WalkContext;
    struct IndependentState;
    struct IndependentProbe;
}

// "Retained XAML, immediate rendering" prototype (docs/design-notes/immediate-rendering-backends-proposal.md).
// When an immediate mode is selected, CCoreServices::RenderWalk calls RenderIsland instead of HWWalk::RenderRoot for
// each XamlIslandRoot. The element tree is walked into a display list, diffed against the previous frame to compute
// damage, and replayed through an IImmediatePainter that presents into one composition swap chain per island. The only
// Composition content for the island is a single SpriteVisual hosting that swap chain; no comp nodes are created.
class ImmediateRenderer
{
public:
    explicit ImmediateRenderer(RenderingBackend::Mode mode) noexcept;
    ~ImmediateRenderer();

    ImmediateRenderer(const ImmediateRenderer&) = delete;
    ImmediateRenderer& operator=(const ImmediateRenderer&) = delete;

    _Check_return_ HRESULT RenderIsland(
        _In_ DCompTreeHost* dcompTreeHost,
        _In_ CXamlIslandRoot* island,
        uint32_t frameNumber);

    // Drops state for islands that are no longer registered with the DCompTreeHost.
    void PruneIslands(_In_ DCompTreeHost* dcompTreeHost);

    // Releases painters, swap chains, devices and visuals (device lost / compositor recreated). Recreated on demand.
    void ReleaseResources() noexcept;

    void ResetStats() noexcept { m_elementsVisited = 0; m_elementsRendered = 0; m_elementsCulled = 0; }
    uint32_t GetElementsVisited() const noexcept { return m_elementsVisited; }
    uint32_t GetElementsRendered() const noexcept { return m_elementsRendered; }

    // True while a device-lost recovery is pending: the caller must schedule another frame.
    bool NeedsFrame() const noexcept;

private:
    struct IslandState;

    _Check_return_ HRESULT EnsureDevice();
    _Check_return_ HRESULT EnsureIslandResources(
        _In_ DCompTreeHost* dcompTreeHost,
        _In_ CXamlIslandRoot* island,
        _Inout_ IslandState& state,
        uint32_t width,
        uint32_t height,
        float scale);

    void Walk(
        _In_ CUIElement* element,
        const ImmediateRenderingDetail::WalkContext& parent,
        _Inout_ ImmediateRenderingDetail::DisplayList& list);

    // Copies a clean subtree's output from the previous frame's display list; false when it cannot be reused.
    bool ReuseSubtree(
        _In_ CUIElement* element,
        const ImmediateRenderingDetail::WalkContext& parent,
        _Inout_ ImmediateRenderingDetail::DisplayList& list);

    // Emits the element's own drawing. deferBorder: the border stroke is emitted by EmitBorder after the children
    // (CornerRadius-clipped elements draw their border over their clipped children, like Composition mode).
    void EmitContent(
        _In_ CUIElement* element,
        const ImmediateRenderingDetail::WalkContext& context,
        bool deferBorder,
        _Inout_ ImmediateRenderingDetail::DisplayList& list);

    void EmitBorder(
        _In_ CUIElement* element,
        const ImmediateRenderingDetail::WalkContext& context,
        _Inout_ ImmediateRenderingDetail::DisplayList& list);

    static void SaveScrollers(_Inout_ IslandState& state, const ImmediateRenderingDetail::DisplayList& list);
    void ReportUnsupported(_In_ CUIElement* element, _In_z_ const wchar_t* detail);

    RenderingBackend::Mode m_mode;
    Microsoft::WRL::ComPtr<ID3D11Device> m_device;
    bool m_deviceTraced = false;
    std::unordered_map<CXamlIslandRoot*, std::unique_ptr<IslandState>> m_islands;
    std::unordered_set<std::wstring> m_reportedUnsupported;
    uint32_t m_elementsVisited = 0;
    uint32_t m_elementsRendered = 0;   // elements with at least one command replayed (painted) this frame
    uint32_t m_elementsCulled = 0;
    uint32_t m_subtreesReused = 0;
    // Previous frame's display list of the island being walked: clean subtrees reuse their commands from it.
    ImmediateRenderingDetail::DisplayList* m_previousWalk = nullptr;

    // ---- Render thread (S15), device-lost recovery (S21), target changes (S20): ImmediateRenderer.Independent.inl ----
    void BuildAnimationIndex(_In_ CXamlIslandRoot* island);
    void ProbeIndependent(_In_ CUIElement* element, _Out_ ImmediateRenderingDetail::IndependentProbe& probe);
    void FinishIndependentLayer(
        _In_ CUIElement* element,
        size_t layerIndex,
        ImmediateRenderingDetail::IndependentProbe& probe,
        const ImmediateRenderingDetail::WalkContext& ctx,
        _Inout_ ImmediateRenderingDetail::DisplayList& list);
    void AppendIndependentDamage(_Inout_ IslandState& state, _Inout_ std::vector<RenderingBackend::RectF>& damage, bool hasBlits, _Inout_ bool& fullDamage);
    void OnIslandPresented(_Inout_ IslandState& state, uint32_t frameNumber);
    void StartRenderThreadIfNeeded();
    void StopRenderThread() noexcept;
    void RenderThreadMain();
    void RenderIndependentFrame(_In_ CXamlIslandRoot* island, _Inout_ IslandState& state, int64_t now);
    HRESULT ApplyDeviceLostSimulation(HRESULT hr, uint32_t frameNumber);
    void OnDeviceLost(HRESULT hr, uint32_t frameNumber);
    void CheckTargetChanges(_In_ CXamlIslandRoot* island, uint32_t frameNumber, uint32_t width, uint32_t height, float scale);

    std::unique_ptr<ImmediateRenderingDetail::IndependentState> m_independent;
};
