// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "PivotHeaderManager.h"

XAML_ABI_NAMESPACE_BEGIN namespace Microsoft { namespace UI { namespace Xaml { namespace Controls { namespace Primitives
{

class PivotHeaderPanel:
    public PivotHeaderPanelGenerated
{

public:
    PivotHeaderPanel();

    _Check_return_ HRESULT SetHeaderManagerCallbacks(_In_ xaml_controls::IPivotHeaderManagerPanelEvents* pManager);
    _Check_return_ HRESULT SetCurrentIndex(_In_ UINT idx);
    _Check_return_ HRESULT SetCurrentOffset(_In_ DOUBLE offset);

    bool IsContentClipped() const;

private:
    ~PivotHeaderPanel();

    _Check_return_ HRESULT InitializeImpl() override;

    // IFrameworkElementOverrides methods
    _Check_return_ HRESULT ArrangeOverrideImpl(
        _In_ wf::Size finalSize,
        _Out_ wf::Size* returnValue) override;
    _Check_return_ HRESULT MeasureOverrideImpl(
        _In_ wf::Size finalSize,
        _Out_ wf::Size* returnValue) override;

    _Check_return_ HRESULT SetCanvasTopLeftOnChild(
        _In_ xaml::IUIElement* child,
        _In_ const wf::Size& desiredSize,
        _In_ const wf::Rect& arrangeRect);

    xaml_controls::IPivotHeaderManagerPanelEvents* m_pManagerNoRef;

    // During a manipulation the header offset exhibits a form
    // of hysteresis in that it is not only determined by the
    // widths of the current Pivot header items, but also by
    // the position it was in when previous header items had their
    // size changed or were added/removed. While when the viewport
    // is idle this offset is not needed, it's vital to allow for updating
    // of the XAML offset of the HeaderPanel during manipulation.
    UINT m_currentIdx;
    DOUBLE m_currentOffset;

    float m_clippedDesiredWidth;
    float m_unclippedDesiredWidth;

    wrl::ComPtr<xaml_controls::ICanvasStatics> m_canvasStatics;
};

ActivatableClass(PivotHeaderPanel);

} } } } } XAML_ABI_NAMESPACE_END
