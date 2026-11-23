// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// F7 (imaging branch): element chrome drawn by the element itself rather than by child elements. Included by
// ImmediateRenderer.cpp after the Emitter (it emits display-list commands).

#include "CalendarViewBaseItemChrome.h"
#include <Microsoft.UI.Composition.h>

namespace
{
    ColorF ToColorF(const wu::Color& color, float opacity) noexcept
    {
        return ToColor((static_cast<uint32_t>(color.A) << 24) | (static_cast<uint32_t>(color.R) << 16)
            | (static_cast<uint32_t>(color.G) << 8) | color.B, opacity);
    }

    // CCalendarViewBaseItemChrome::DrawDensityBar: up to ten bars stacked upward from the bottom edge.
    void EmitCalendarDensityBars(Emitter& emitter, CCalendarViewBaseItemChrome* chrome, const RectF& bounds,
        const std::array<wu::Color, 10>& colors, uint32_t count, bool outOfScope, const CMILMatrix& world)
    {
        if (count == 0 || chrome->GetOwner() == nullptr)
        {
            return;
        }
        const float opacity = outOfScope ? 0.10f : 0.35f; // Out/InScopeDensityBarOpacity
        const float width = bounds.right - bounds.left;
        const float height = std::min((bounds.bottom - bounds.top) / 10 - 1, 5.0f); // MaxDensityBarHeight
        if (!(height > 0) || !(width > 2))
        {
            return;
        }
        float y = bounds.bottom - height;
        for (uint32_t i = 0; i < count && i < colors.size(); ++i)
        {
            const ColorF color = ToColorF(colors[i], opacity);
            if (color.a > 0)
            {
                emitter.Fill({ 1, y, width - 1, y + height }, 0, 0, color, world);
            }
            y -= height + 1;
        }
    }

    // A hollow nine-grid border (CCalendarViewBaseItemChrome::DrawBorder): four edge rectangles inside `rect`.
    void EmitHollowBorder(Emitter& emitter, const RectF& rect, const XTHICKNESS& t, const ColorF& color, const CMILMatrix& world)
    {
        const float innerTop = std::min(rect.top + t.top, rect.bottom);
        const float innerBottom = std::max(rect.bottom - t.bottom, innerTop);
        emitter.Fill({ rect.left, rect.top, rect.right, innerTop }, 0, 0, color, world);
        emitter.Fill({ rect.left, innerBottom, rect.right, rect.bottom }, 0, 0, color, world);
        emitter.Fill({ rect.left, innerTop, std::min(rect.left + t.left, rect.right), innerBottom }, 0, 0, color, world);
        emitter.Fill({ std::max(rect.right - t.right, rect.left), innerTop, rect.right, innerBottom }, 0, 0, color, world);
    }

    // F13: true when the island's ContentIsland carries a SystemBackdrop (Window.SystemBackdrop = Mica/DesktopAcrylic).
    bool IslandHasSystemBackdrop(_In_ CXamlIslandRoot* islandRoot) noexcept
    {
        ixp::IContentIsland* contentIsland = islandRoot->GetContentIsland();
        Microsoft::WRL::ComPtr<ixp::ICompositionSupportsSystemBackdrop> target;
        Microsoft::WRL::ComPtr<RealWUComp::ICompositionBrush> backdrop;
        return contentIsland != nullptr
            && SUCCEEDED(contentIsland->QueryInterface(IID_PPV_ARGS(&target)))
            && SUCCEEDED(target->get_SystemBackdrop(&backdrop))
            && backdrop != nullptr;
    }
}

uint32_t ImmediateRenderer::WalkCalendarViewItem(
    _In_ CUIElement* element,
    const WalkContext& ctx,
    const WalkContext& childCtx,
    _Inout_ DisplayList& list)
{
    auto chrome = static_cast<CCalendarViewBaseItemChrome*>(element);
    Emitter emitter{ list, element, ctx };
    const bool legacy = !chrome->IsRoundedCalendarViewBaseItemChromeEnabled();

    RectF bounds{ 0, 0, element->GetActualWidth(), element->GetActualHeight() };
    if (chrome->ShouldUseLayoutRounding())
    {
        bounds.right = chrome->LayoutRound(bounds.right);
        bounds.bottom = chrome->LayoutRound(bounds.bottom);
    }

    // Pre layer (legacy chrome): CalendarView's item background, then the item's own Control.Background.
    if (legacy)
    {
        ColorF color;
        if (ResolveBrush(chrome->GetItemBackgroundBrush(), 1.0f, &color) == BrushResult::Color)
        {
            emitter.Fill(bounds, 0, 0, color, ctx.world);
        }
        CValue value;
        if (SUCCEEDED(chrome->GetValueByIndex(KnownPropertyIndex::Control_Background, &value)) && value.GetType() == valueObject
            && ResolveBrush(do_pointer_cast<CBrush>(value.AsObject()), 1.0f, &color) == BrushResult::Color)
        {
            emitter.Fill(bounds, 0, 0, color, ctx.world);
        }
    }

    // HWWalk::CalendarViewBaseItemRenderChildren: outer border, template child, density bars, then the other children.
    CUIElement* outerBorder = chrome->m_outerBorder.get();
    CUIElement* templateChild = chrome->GetFirstChildNoAddRef();
    XUINT32 childCount = 0;
    CUIElement** children = nullptr;
    element->GetChildrenInRenderOrder(&children, &childCount);
    if (outerBorder == nullptr && templateChild == nullptr)
    {
        EmitCalendarDensityBars(emitter, chrome, bounds, chrome->m_densityBarColors, chrome->m_numberOfDensityBar, chrome->m_isOutOfScope, ctx.world);
    }
    if (!IsEmptyRect(childCtx.clip))
    {
        if (outerBorder != nullptr)
        {
            Walk(outerBorder, childCtx, list);
        }
        if (templateChild != nullptr && templateChild != outerBorder)
        {
            Walk(templateChild, childCtx, list);
        }
    }
    if (outerBorder != nullptr || templateChild != nullptr)
    {
        // Density bars belong to the item (not the template child) so damage and reuse follow the item.
        Emitter bars{ list, element, ctx };
        EmitCalendarDensityBars(bars, chrome, bounds, chrome->m_densityBarColors, chrome->m_numberOfDensityBar, chrome->m_isOutOfScope, ctx.world);
    }
    if (!IsEmptyRect(childCtx.clip))
    {
        for (XUINT32 i = 0; i < childCount; ++i)
        {
            if (children[i] != nullptr && children[i] != outerBorder && children[i] != templateChild)
            {
                Walk(children[i], childCtx, list);
            }
        }
    }

    // Post layer (legacy chrome): the border (or keyboard focus border), then the Today+Selected inner border.
    if (legacy)
    {
        Emitter post{ list, element, ctx };
        bool dotted = false;
        IGNOREHR(chrome->ShouldDrawDottedLinesFocusVisual(&dotted));
        ColorF color;
        if (chrome->m_isKeyboardFocused && dotted)
        {
            // RenderFocusRectangle draws two interleaved 2px dotted rings; painted as the solid primary ring.
            if (ResolveBrush(chrome->GetItemBorderBrush(true), 1.0f, &color) == BrushResult::Color)
            {
                EmitHollowBorder(post, bounds, { 2, 2, 2, 2 }, color, ctx.world);
            }
            ReportUnsupported(element, L"CalendarViewBaseItem dotted focus:approximated");
        }
        else
        {
            const XTHICKNESS thickness = chrome->GetItemBorderThickness();
            if (thickness.left != 0 && thickness.top != 0 && thickness.right != 0 && thickness.bottom != 0
                && ResolveBrush(chrome->GetItemBorderBrush(false), 1.0f, &color) == BrushResult::Color)
            {
                EmitHollowBorder(post, bounds, thickness, color, ctx.world);
            }
        }
        if (chrome->m_isToday && chrome->m_isSelected && ResolveBrush(chrome->GetItemInnerBorderBrush(), 1.0f, &color) == BrushResult::Color)
        {
            const XTHICKNESS outer = chrome->GetItemBorderThickness();
            const RectF inner{ bounds.left + outer.left, bounds.top + outer.top, bounds.right - outer.right, bounds.bottom - outer.bottom };
            if (inner.right > inner.left && inner.bottom > inner.top)
            {
                EmitHollowBorder(post, inner, { 2, 2, 2, 2 }, color, ctx.world); // TodaySelectedInnerBorderThickness
            }
        }
    }
    return childCount;
}
