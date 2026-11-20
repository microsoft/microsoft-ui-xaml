// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// S14: windowed popups in immediate modes. Composition mode hosts the popup child's comp node visual as the popup
// window's public root visual (CPopup::SetRootVisualForWindowedPopupWindow); immediate modes host a sprite showing a
// swap chain the popup subtree is painted into (ImmediateRenderer::RenderWindowedPopups), sized to the popup window.
// See docs/design-notes/immediate-rendering-backends-proposal.md, "Full-backend decisions (S11-S23)".

#include "precomp.h"
#include "ImmediateRenderer.h"
#include <cmath>

using Microsoft::WRL::ComPtr;

bool ImmediateRenderer::PaintWindowedPopupsInIsland() noexcept
{
    static const bool inIsland = []
    {
        wchar_t value[8] = {};
        return GetEnvironmentVariableW(L"WINUI_IMMEDIATE_INLINE_POPUPS", value, ARRAYSIZE(value)) > 0 && value[0] == L'1';
    }();
    return inIsland;
}

_Check_return_ HRESULT ImmediateRenderer::GetWindowedPopupGeometry(
    _In_ CPopup* popup,
    float scale,
    _Out_ WindowedPopupGeometry* geometry)
{
    *geometry = {};
    // Same bounds CPopup::PositionAndSizeWindowForWindowedPopup gives the popup window (physical pixels, island space).
    XRECTF bounds{};
    float scaleX = 1.0f;
    float scaleY = 1.0f;
    IFC_RETURN(popup->GetPhysicalBounds(popup, &bounds, &scaleX, &scaleY));
    XTHICKNESS insets{};
    if (popup->ShouldPopupRendersDropShadow())
    {
        // The window is inflated by the drop shadow's insets (DIPs, rounded to the root scale).
        insets = popup->GetInsetsForDropShadow();
    }
    geometry->originX = std::ceil(bounds.X) / scale - insets.left;
    geometry->originY = std::ceil(bounds.Y) / scale - insets.top;
    const float width = std::ceil(bounds.Width + (insets.left + insets.right) * scale);
    const float height = std::ceil(bounds.Height + (insets.top + insets.bottom) * scale);
    geometry->width = width > 0 ? static_cast<uint32_t>(width) : 0;
    geometry->height = height > 0 ? static_cast<uint32_t>(height) : 0;
    return S_OK;
}

_Check_return_ HRESULT ImmediateRenderer::AttachWindowedPopupVisual(
    _In_ CPopup* popup,
    _In_ IUnknown* visualUnknown,
    uint32_t width,
    uint32_t height,
    _Out_ bool* flipped)
{
    *flipped = false;
    ComPtr<WUComp::IVisual> visual;
    IFC_RETURN(visualUnknown->QueryInterface(IID_PPV_ARGS(&visual)));

    if (popup->m_publicRootVisual)
    {
        ComPtr<IUnknown> current;
        IFC_RETURN(popup->m_publicRootVisual.As(&current));
        ComPtr<IUnknown> ours;
        IFC_RETURN(visual.As(&ours));
        if (current.Get() != ours.Get())
        {
            // A visual from an earlier device / painter generation is still hosted.
            DetachWindowedPopupVisual(popup, popup->m_publicRootVisual.Get());
        }
    }

    // Creates the popup's ContentIsland, root visual tree and window position on first use; no-op afterwards.
    IFC_RETURN(popup->SetRootVisualForWindowedPopupWindow(visual.Get()));

    // m_contentIslandRootVisual maps island-space DIPs into the window (scale and the "undo" of the popup's position,
    // see PositionAndSizeWindowForWindowedPopup). Place the sprite so it covers the window from its top-left: find the
    // point P with P * M = (0, 0), and the size that M scales to the window's pixel size.
    if (!popup->m_contentIslandRootVisual)
    {
        return S_OK;
    }
    wfn::Matrix4x4 m{};
    IFC_RETURN(popup->m_contentIslandRootVisual->get_TransformMatrix(&m));
    const float det = m.M11 * m.M22 - m.M12 * m.M21;
    if (std::fabs(det) < 1e-6f)
    {
        return S_OK;
    }
    const float sx = std::sqrt(m.M11 * m.M11 + m.M12 * m.M12);
    const float sy = std::sqrt(m.M21 * m.M21 + m.M22 * m.M22);
    // RTL parented popups get an in-place flip (M11 < 0); the content is painted unflipped, mapped from the window's
    // top-right corner instead.
    *flipped = m.M11 < 0;
    const float targetX = *flipped ? static_cast<float>(width) : 0.0f;
    const float px = ((targetX - m.M41) * m.M22 + m.M42 * m.M21) / det;
    const float py = (-m.M42 * m.M11 - (targetX - m.M41) * m.M12) / det;
    IFC_RETURN(visual->put_Offset({ px, py, 0 }));
    IFC_RETURN(visual->put_Size({ static_cast<float>(width) / sx, static_cast<float>(height) / sy }));
    return S_OK;
}

void ImmediateRenderer::DetachWindowedPopupVisual(_In_ CPopup* popup, _In_ IUnknown* visualUnknown) noexcept
{
    ComPtr<WUComp::IVisual> visual;
    if (popup == nullptr || visualUnknown == nullptr || FAILED(visualUnknown->QueryInterface(IID_PPV_ARGS(&visual))))
    {
        return;
    }
    // Closing the popup (ReleaseDCompResourcesForWindowedPopup) already dropped the window's visual tree.
    if (!popup->m_publicRootVisual)
    {
        return;
    }
    ComPtr<IUnknown> current;
    ComPtr<IUnknown> ours;
    if (FAILED(popup->m_publicRootVisual.As(&current)) || FAILED(visual.As(&ours)) || current.Get() != ours.Get())
    {
        return;
    }
    ComPtr<WUComp::IContainerVisual> container;
    ComPtr<WUComp::IVisualCollection> children;
    if (popup->m_animationRootVisual && SUCCEEDED(popup->m_animationRootVisual.As(&container))
        && SUCCEEDED(container->get_Children(&children)))
    {
        IGNOREHR(children->Remove(visual.Get()));
    }
    popup->m_publicRootVisual.Reset();
}
