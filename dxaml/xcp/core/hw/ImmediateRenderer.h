// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <ImmediatePainter.h>
#include <wrl/client.h>
#include <cstdio>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class CUIElement;
class CXamlIslandRoot;
class CPopup;
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

    // ---- dmanip ---- Drops an island's state when it is removed, before its ContentIsland closes: the island's
    // Composition tree is disposed with it, and the ScrollPresenter hand-off visuals parented under it as hit-test
    // proxies must be detached first (XAML detaches from them when the elements are destroyed later).
    void OnIslandRemoved(_In_ CXamlIslandRoot* island) noexcept;

    // Releases painters, swap chains, devices and visuals (device lost / compositor recreated). Recreated on demand.
    void ReleaseResources() noexcept;

    // S27: called by the render walk instead of RenderIsland while the island's host is not visible (minimized or
    // hidden; XamlRoot.IsHostVisible is false). Once it has been hidden for c_trimHiddenDelayMs, the island's painter,
    // swap chain and cached bitmaps are released, the swap chain is detached from its sprite and the device is trimmed
    // (the device is kept for a fast restore). The next RenderIsland recreates them with full damage. Returns the
    // milliseconds until the trim is due (the caller schedules a frame then), or 0 once the island is trimmed.
    static constexpr uint32_t c_trimHiddenDelayMs = 2000;
    uint32_t OnIslandHidden(_In_ CXamlIslandRoot* island) noexcept;

    // gdilayers: called by the render walk after RenderIsland (when hidden-window trim is enabled). Releases painter
    // layers and scratch surfaces idle for ~2 s; returns the milliseconds after which the caller should request a walk
    // for the next trim, or 0 when nothing is held.
    uint32_t TrimIdleResources(_In_ CXamlIslandRoot* island) noexcept;

    // S11/S16 per-window fallback (ImmediateRenderer.Fallback.cpp): an island whose walk found Composition-only
    // content is rendered by HWWalk from then on. ConsumeFallbackRedraw is true once, for the first HWWalk pass, which
    // must force a full redraw (the island's subtree never had comp nodes).
    bool IsFallenBack(_In_ CXamlIslandRoot* island) const noexcept;
    bool ConsumeFallbackRedraw(_In_ CXamlIslandRoot* island) noexcept;

    void ResetStats() noexcept { m_elementsVisited = 0; m_elementsRendered = 0; m_elementsCulled = 0; }
    uint32_t GetElementsVisited() const noexcept { return m_elementsVisited; }
    uint32_t GetElementsRendered() const noexcept { return m_elementsRendered; }

    // S22: RenderTargetBitmap (RenderingBackend::CaptureElementFn, registered by the constructor). Paints the element
    // into a temporary offscreen painter of the same mode and reads its pixels back.
    static HRESULT CaptureElementCallback(_In_ void* element, _In_reads_(6) const float* transform, uint32_t width,
        uint32_t height, _Out_writes_bytes_(stride * height) uint8_t* pixels, uint32_t stride, _Out_ bool* handled);
    _Check_return_ HRESULT CaptureElement(_In_ CUIElement* element, _In_reads_(6) const float* transform, uint32_t width,
        uint32_t height, _Out_writes_bytes_(stride * height) uint8_t* pixels, uint32_t stride, _Out_ bool* handled);
    // True while a device-lost recovery is pending: the caller must schedule another frame.
    bool NeedsFrame() const noexcept;

private:
    struct IslandState;

    void SyncHitProxies(_In_ DCompTreeHost* dcompTreeHost, _Inout_ IslandState& state) noexcept;
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

    // Paints LTE targets that are "secret" children (focus rect canvas) and so never reached by the normal walk.
    void WalkSecretTransitionTargets(
        _In_ CUIElement* transitionRoot,
        const ImmediateRenderingDetail::WalkContext& parent,
        _Inout_ ImmediateRenderingDetail::DisplayList& list);

    // Copies a clean subtree's output from the previous frame's display list; false when it cannot be reused.
    bool ReuseSubtree(
        _In_ CUIElement* element,
        const ImmediateRenderingDetail::WalkContext& parent,
        _Inout_ ImmediateRenderingDetail::DisplayList& list);
    // ---- scrollgaps ---- gap 1 (WINUI_IMMEDIATE_OFFTHREAD_SCROLL=1): overscan walk of a DManip-moving scroller.
    int BeginOffThreadScroller(
        _In_ CUIElement* presenter,
        _Inout_ ImmediateRenderingDetail::WalkContext& childCtx,
        _Inout_ ImmediateRenderingDetail::DisplayList& list);
    void FinishOffThreadScroller(
        uint32_t index,
        uint32_t cmdBegin,
        uint32_t recordBegin,
        size_t layerBegin,
        float scale,
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

    // FocusVisualKind.DottedLine: the focus target draws its legacy dotted focus rect after its children
    // (HWWalk via CFocusManager::RenderFocusRectForElementIfNeeded).
    void EmitLegacyFocusRect(
        _In_ CUIElement* element,
        const ImmediateRenderingDetail::WalkContext& context,
        _Inout_ ImmediateRenderingDetail::DisplayList& list);

    static void SaveScrollers(_Inout_ IslandState& state, const ImmediateRenderingDetail::DisplayList& list);
    void ReportUnsupported(_In_ CUIElement* element, _In_z_ const wchar_t* detail);
    static FILE* StatsFile() noexcept;

    // ImmediateRenderer.Fallback.cpp
    void CheckFallback(_In_ CUIElement* element) noexcept;
    void FallBackIsland(_In_ DCompTreeHost* dcompTreeHost, _In_ CXamlIslandRoot* island) noexcept;
    void PruneFallenBack(_In_ DCompTreeHost* dcompTreeHost) noexcept;
    static bool IsBuiltInProgressRingPlayer(_In_ CUIElement* element) noexcept;
    static bool IsHandOffVisualTouched(_In_ CUIElement* element) noexcept;

    // Walks into a display list, diffs, and paints one swap chain (an island, or a windowed popup's window).
    _Check_return_ HRESULT RenderSurface(
        _Inout_ IslandState& state,
        uint32_t frameNumber,
        uint64_t surfaceId,
        const ImmediateRenderingDetail::WalkContext& root,
        _In_ void (*walk)(_In_ ImmediateRenderer* self, _In_ void* walkContext,
            const ImmediateRenderingDetail::WalkContext& root, _Inout_ ImmediateRenderingDetail::DisplayList& list),
        _In_ void* walkContext,
        _Out_ bool* fellBack);
    _Check_return_ HRESULT RenderWindowedPopups(
        _In_ DCompTreeHost* dcompTreeHost,
        _Inout_ IslandState& islandState,
        _Inout_ ImmediateRenderingDetail::DisplayList& islandList,
        uint32_t frameNumber,
        _Out_ bool* fellBack);

    // S14 (ImmediateRenderer.Popups.cpp): windowed popups get their own swap chain + sprite, hosted as the popup
    // window's root visual (where HWWalk hosts the popup child's comp node visual).
    struct WindowedPopupGeometry
    {
        float originX = 0;   // DIPs, island space: the popup window's top-left
        float originY = 0;
        uint32_t width = 0;  // physical pixels
        uint32_t height = 0;
    };
    static bool PaintWindowedPopupsInIsland() noexcept;
    static _Check_return_ HRESULT GetWindowedPopupGeometry(_In_ CPopup* popup, float scale, _Out_ WindowedPopupGeometry* geometry);
    // Hosts the visual in the popup window and maps it to cover the window; *flipped: RTL-flipped root (approximated).
    static _Check_return_ HRESULT AttachWindowedPopupVisual(_In_ CPopup* popup, _In_ IUnknown* visual, uint32_t width,
        uint32_t height, _Out_ bool* flipped);
    static void DetachWindowedPopupVisual(_In_ CPopup* popup, _In_ IUnknown* visual) noexcept;

    RenderingBackend::Mode m_mode;
    Microsoft::WRL::ComPtr<ID3D11Device> m_device;
    bool m_deviceTraced = false;
    std::unordered_map<CXamlIslandRoot*, std::unique_ptr<IslandState>> m_islands;
    // S27: islands whose host is not visible, with the time (QPC microseconds) they were first seen hidden.
    std::unordered_map<CXamlIslandRoot*, int64_t> m_hiddenSinceUs;
    std::unordered_set<std::wstring> m_reportedUnsupported;
    uint32_t m_elementsVisited = 0;
    // > 0 while walking a secret LTE target (the focus rect canvas).
    uint32_t m_secretTargetDepth = 0;
    uint32_t m_elementsRendered = 0;   // elements with at least one command replayed (painted) this frame
    uint32_t m_elementsCulled = 0;
    uint32_t m_subtreesReused = 0;
    // Previous frame's display list of the island being walked: clean subtrees reuse their commands from it.
    ImmediateRenderingDetail::DisplayList* m_previousWalk = nullptr;
    // Fallback trigger found by the current walk (static reason string) and the element's class name.
    const wchar_t* m_pendingFallback = nullptr;
    std::wstring m_pendingFallbackClass;
    // Islands rendered by HWWalk; value: the next HWWalk pass must force a full redraw.
    std::unordered_map<CXamlIslandRoot*, bool> m_fallenBack;

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

    // ---- transitions ----
    // F10 (ImmediateRenderer.Transitions.cpp): XAML-owned property transitions evaluated by XAML.
    static bool IsElementImmediateCallback(_In_ const void* element) noexcept;
    static bool HasTransitions(_In_ CUIElement* element) noexcept;
    // Applies running transitions (opacity override); requests the next frame. False once they finished.
    static bool ApplyTransitions(_In_ CUIElement* element, _Inout_ float* opacity) noexcept;
    static bool TryGetTransitionColor(_In_ CUIElement* element, _Inout_ RenderingBackend::ColorF* color) noexcept;
    // ImmediateRenderer.Connected.inl: ConnectedAnimation hosts emulated by CConnectedAnimation, painted by the root.
    void EmitConnectedAnimations(
        _In_ CUIElement* root,
        const ImmediateRenderingDetail::WalkContext& parent,
        _Inout_ ImmediateRenderingDetail::DisplayList& list);
    static bool IsConnectedHidden(_In_ CUIElement* element) noexcept;
    static bool IsConnectedEmulated(_In_ CUIElement* element) noexcept;
    // The destination being drawn through its connected animation host (walked although hidden in place).
    const CUIElement* m_connectedOverlayElement = nullptr;
    float m_connectedOverlayOpacity = 1;
    // ---- imaging (ImmediateRenderer.Imaging.inl) ----
    // CalendarViewBaseItem: walks the children in HWWalk::CalendarViewBaseItemRenderChildren order with the chrome the item
    // draws itself (density bars; legacy chrome backgrounds, borders and focus). Returns the child count.
    uint32_t WalkCalendarViewItem(
        _In_ CUIElement* element,
        const ImmediateRenderingDetail::WalkContext& context,
        const ImmediateRenderingDetail::WalkContext& childContext,
        _Inout_ ImmediateRenderingDetail::DisplayList& list);
    // F11: unsupported content found while walking a surface is reported only once the walk is known not to fall the
    // window back (a fallen-back window renders that content in Composition). Null outside RenderSurface's walk.
    std::vector<std::pair<std::wstring, std::wstring>>* m_deferredUnsupported = nullptr;
};
