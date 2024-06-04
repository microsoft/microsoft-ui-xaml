// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "PivotHeaderPanel_Partial.h"
#include "Pivot_Partial.h"
#include "DoubleUtil.h"

//#define PVTRACE(...) TRACE(TraceAlways, __VA_ARGS__)
#define PVTRACE(...)
#undef min
#undef max

XAML_ABI_NAMESPACE_BEGIN namespace Microsoft { namespace UI { namespace Xaml { namespace Controls { namespace Primitives
{

// Rounding error can creep into desired sizes, so we shouldn't
// test for strict equality with desired sizes when determining
// whether to use static or dynamic headers.
// Instead, we should apply a reasonable tolerance to ensure
// effective equality.  Testing has indicated that all rounding errors
// result in values differing by at least two orders of magnitude
// smaller than this value, so this should be a safe value to use
// while still not allowing genuinely different values.
const float PivotHeaderDesiredSizeRoundingTolerance = 0.001f;

PivotHeaderPanel::PivotHeaderPanel()
    : m_pManagerNoRef(nullptr)
    , m_currentOffset(0.0)
    , m_currentIdx(0)
    , m_clippedDesiredWidth(0.0f)
    , m_unclippedDesiredWidth(0.0f)
{
}

_Check_return_
HRESULT
PivotHeaderPanel::InitializeImpl()
{
    HRESULT hr = S_OK;
    wrl::ComPtr<xaml_controls::ICanvasFactory> spInnerFactory;
    wrl::ComPtr<xaml_controls::ICanvas> spInnerInstance;
    wrl::ComPtr<IInspectable> spInnerInspectable;

    IFC(PivotHeaderPanelGenerated::InitializeImpl());

    IFC(wf::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Controls_Canvas).Get(),
        &spInnerFactory));

    IFC(spInnerFactory.As(&m_canvasStatics));

    IFC(spInnerFactory->CreateInstance(
        static_cast<IInspectable*>(static_cast<IPivotHeaderPanel*>(this)),
        &spInnerInspectable,
        &spInnerInstance));

    IFC(SetComposableBasePointers(
            spInnerInspectable.Get(),
            spInnerFactory.Get()));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
PivotHeaderPanel::SetHeaderManagerCallbacks(_In_ xaml_controls::IPivotHeaderManagerPanelEvents* pManager)
{
    m_pManagerNoRef = pManager;
    RRETURN(S_OK);
}

_Check_return_ HRESULT
PivotHeaderPanel::ArrangeOverrideImpl(
    _In_ wf::Size finalSize,
    _Out_ wf::Size* returnValue)
{
    wrl::ComPtr<xaml_controls::IPanel> spThisAsPanel;
    wrl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildrenVect;

    if (m_pManagerNoRef)
    {
        IFC_RETURN(m_pManagerNoRef->HeaderPanelMeasureEvent(m_clippedDesiredWidth));
    }

    IFC_RETURN(QueryInterface(
        __uuidof(xaml_controls::IPanel),
        &spThisAsPanel));
    IFC_RETURN(spThisAsPanel->get_Children(&spChildrenVect));
    UINT itemCount = 0;
    IFC_RETURN(spChildrenVect->get_Size(&itemCount));
    DOUBLE currentOffset = m_currentOffset;
    PVTRACE(L"[PHP]: currentOffset for layout: %f", currentOffset);
    UINT currentIdx = m_currentIdx;

    // In addition to the children items of this Panel we'll handle
    // calculating the offsets for the LTEs here as well. The LTEs
    // allow us to display the last Pivot header in two places:
    // - It's regular position at the end of the header panel
    // - In the ghost peak-behind position in front of the panel
    // Because an LTE causes the original item to be hidden we
    // create two, one to stand in place of the original, and one
    // to actually create the duplicate 'ghost' item.
    DOUBLE primaryItemOffset = 0.0;
    DOUBLE ghostItemOffset = m_currentOffset;
    DOUBLE lteVerticalOffset = 0.0;

    for (UINT childIdx = 0; childIdx < itemCount; childIdx++)
    {
        wrl::ComPtr<xaml::IUIElement> spChild;

        UINT idx = PositiveMod(currentIdx++, itemCount);
        IFC_RETURN(spChildrenVect->GetAt(idx, &spChild));

        wf::Size desiredSize = {};
        IFC_RETURN(spChild->get_DesiredSize(&desiredSize));

        if (childIdx == itemCount - 1)
        {
            wrl::ComPtr<xaml::IFrameworkElement> spChildAsFE;
            xaml::Thickness thickness = {};
            IFC_RETURN(spChild.As(&spChildAsFE));
            IFC_RETURN(spChildAsFE->get_Margin(&thickness));

            // The ghostLte is set to an offset just before
            // the beginning of the PivotHeaderItems.
            ghostItemOffset += -desiredSize.Width + thickness.Left;

            // The primaryLte is placed in a position identical
            // to its original. Because LTEs don't take margins
            // into account as part of a layout pass we do so here.
            primaryItemOffset = currentOffset + thickness.Left;

            lteVerticalOffset = thickness.Top;
        }

        wf::Rect arrangeRect = {};
        arrangeRect.Width = desiredSize.Width;
        arrangeRect.Height = std::max(finalSize.Height, desiredSize.Height);
        arrangeRect.X = static_cast<FLOAT>(currentOffset);

        if (idx == 0)
        {
            PVTRACE(L"[PHP]: First element position: x: %f, w: %f", arrangeRect.X, arrangeRect.Width);
        }

        IFC_RETURN(spChild->Arrange(arrangeRect));
        IFC_RETURN(SetCanvasTopLeftOnChild(spChild.Get(), desiredSize, arrangeRect));

        currentOffset += desiredSize.Width;
    }

    if (m_pManagerNoRef)
    {
        IFC_RETURN(m_pManagerNoRef->HeaderPanelSetLteOffsetEvent(primaryItemOffset, ghostItemOffset, lteVerticalOffset));
    }

    *returnValue = finalSize;

    return S_OK;
}

_Check_return_ HRESULT
PivotHeaderPanel::MeasureOverrideImpl(
    _In_ wf::Size availableSize,
    _Out_ wf::Size* desiredSize)
{
    wrl::ComPtr<xaml_controls::IPanel> spThisAsPanel;
    wrl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildrenVect;

    IFC_RETURN(QueryInterface(
        __uuidof(xaml_controls::IPanel),
        &spThisAsPanel));
    IFC_RETURN(spThisAsPanel->get_Children(&spChildrenVect));

    UINT vectSize = 0;
    IFC_RETURN(spChildrenVect->get_Size(&vectSize));

    desiredSize->Height = 0.0;
    desiredSize->Width = 0.0;

    for (UINT childIdx = 0; childIdx < vectSize; childIdx++)
    {
        wrl::ComPtr<xaml::IUIElement> spChild;
        wf::Size eltDesiredSize = {};
        IFC_RETURN(spChildrenVect->GetAt(childIdx, &spChild));
        IFC_RETURN(spChild->Measure(availableSize));
        IFC_RETURN(spChild->get_DesiredSize(&eltDesiredSize));

        wrl::ComPtr<xaml_primitives::IPivotHeaderItem> spItemAsHeaderItem;
        IFC_RETURN(spChild.As(&spItemAsHeaderItem));

        desiredSize->Height = std::max(eltDesiredSize.Height, desiredSize->Height);
        desiredSize->Width += eltDesiredSize.Width;
    }

    m_unclippedDesiredWidth = desiredSize->Width;
    m_clippedDesiredWidth = std::min(availableSize.Width, m_unclippedDesiredWidth);

    return S_OK;
}

_Check_return_ HRESULT
PivotHeaderPanel::SetCurrentIndex(_In_ UINT idx)
{
    if (m_currentIdx != idx)
    {
        PVTRACE(L"[PHP]: idx updated from %d to %d", m_currentIdx, idx);
        m_currentIdx = idx;
        wrl::ComPtr<xaml::IUIElement> spThisAsUIE;
        IFC_RETURN(QueryInterface(__uuidof(xaml::IUIElement), &spThisAsUIE));
        IFC_RETURN(spThisAsUIE->InvalidateArrange());
    }

    return S_OK;
}

_Check_return_ HRESULT
PivotHeaderPanel::SetCurrentOffset(_In_ DOUBLE offset)
{
    HRESULT hr = S_OK;

    if (m_currentOffset != offset)
    {
        PVTRACE(L"[PHP]: offset updated from %f to %f", m_currentOffset, offset);
        m_currentOffset = offset;
        wrl::ComPtr<xaml::IUIElement> spThisAsUIE;
        IFC(QueryInterface(__uuidof(xaml::IUIElement), &spThisAsUIE));
        IFC(spThisAsUIE->InvalidateArrange());
    }

Cleanup:
    RRETURN(hr);
}

bool PivotHeaderPanel::IsContentClipped() const
{
    return
        m_unclippedDesiredWidth > m_clippedDesiredWidth &&
        !DirectUI::DoubleUtil::AreWithinTolerance(m_unclippedDesiredWidth, m_clippedDesiredWidth, PivotHeaderDesiredSizeRoundingTolerance);
}

PivotHeaderPanel::~PivotHeaderPanel()
{
}

_Check_return_ HRESULT
PivotHeaderPanel::SetCanvasTopLeftOnChild(
    _In_ xaml::IUIElement* child,
    _In_ const wf::Size& desiredSize,
    _In_ const wf::Rect& arrangeRect)
{
    // PivotHeaderPanel, being a Canvas, ignores the vertical/horizontal alignment of its children
    // during rendering. We need to do it manually here. We are not concerned about horizontal
    // alignment because we only give the desired width when we arrange.

    float top = arrangeRect.Y;
    const float left = arrangeRect.X;

    {
        wrl::ComPtr<xaml::IFrameworkElement> childAsFE;
        IFC_RETURN(child->QueryInterface<xaml::IFrameworkElement>(&childAsFE));
        if (childAsFE)
        {
            xaml::VerticalAlignment va;
            IFC_RETURN(childAsFE->get_VerticalAlignment(&va));
            if (va == xaml::VerticalAlignment::VerticalAlignment_Center)
            {
                top = (arrangeRect.Height - desiredSize.Height) / 2.0f;
            }
            else if (va == xaml::VerticalAlignment::VerticalAlignment_Bottom)
            {
                top = arrangeRect.Height - desiredSize.Height;
            }
        }
    }

    IFC_RETURN(m_canvasStatics->SetTop(child, top));
    IFC_RETURN(m_canvasStatics->SetLeft(child, left));

    return S_OK;
}

}
} } } } XAML_ABI_NAMESPACE_END
