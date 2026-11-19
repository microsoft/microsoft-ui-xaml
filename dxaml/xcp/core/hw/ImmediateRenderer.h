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

    void ResetStats() noexcept { m_elementsVisited = 0; m_elementsRendered = 0; }
    uint32_t GetElementsVisited() const noexcept { return m_elementsVisited; }
    uint32_t GetElementsRendered() const noexcept { return m_elementsRendered; }

    // Display-list command; public so the walk helpers in the .cpp can build them.
    enum class CommandType : uint8_t { FillRect, FillRounded, StrokeRounded, Text };

    struct Command
    {
        CommandType type = CommandType::FillRect;
        RenderingBackend::Matrix3x2F transform;  // DIPs, element -> island
        RenderingBackend::RectF clip;            // DIPs, island space; always set (unclipped = island bounds)
        RenderingBackend::RectF rect;            // DIPs, element space
        RenderingBackend::RectF deviceBounds;    // physical pixels, island space (clipped)
        RenderingBackend::ColorF color;
        float radiusX = 0;
        float radiusY = 0;
        float thickness = 0;
        float fontSize = 0;
        uint32_t fontWeight = 400;
        uint32_t textIndex = 0;                  // into DisplayList::strings (text, then family)
    };

    struct ElementRecord
    {
        RenderingBackend::RectF bounds;          // physical pixels; union of the element's commands
        uint64_t signature = 0;
    };

    struct DisplayList
    {
        std::vector<Command> commands;
        std::vector<std::wstring> strings;
        std::unordered_map<const CUIElement*, ElementRecord> records;
    };

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
        const CMILMatrix& parentWorld,
        const RenderingBackend::RectF& parentClip,
        float parentOpacity,
        float scale,
        _Inout_ DisplayList& list);

    void EmitContent(
        _In_ CUIElement* element,
        const CMILMatrix& world,
        const RenderingBackend::RectF& clip,
        float opacity,
        float scale,
        _Inout_ DisplayList& list);

    void ReportUnsupported(_In_ CUIElement* element, _In_z_ const wchar_t* detail);

    RenderingBackend::Mode m_mode;
    Microsoft::WRL::ComPtr<ID3D11Device> m_device;
    bool m_deviceTraced = false;
    std::unordered_map<CXamlIslandRoot*, std::unique_ptr<IslandState>> m_islands;
    std::unordered_set<std::wstring> m_reportedUnsupported;
    uint32_t m_elementsVisited = 0;
    uint32_t m_elementsRendered = 0;
};
